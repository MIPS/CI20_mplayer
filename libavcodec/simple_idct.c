/*
 * Simple IDCT
 *
 * Copyright (c) 2001 Michael Niedermayer <michaelni@gmx.at>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * simpleidct in C.
 */

/*
  based upon some outcommented c code from mpeg2dec (idct_mmx.c
  written by Aaron Holtzman <aholtzma@ess.engr.uvic.ca>)
 */
#include "avcodec.h"
#include "dsputil.h"
#include "mathops.h"
#include "simple_idct.h"
#define JZC_MXU_OPT
#include "../libjzcommon/jzmedia.h"
#define  WM44  0x5A825A82   //(W4,W4)
#define  WM26  0x764230FC   //(W2,W6)
#define  WM13  0x7D8A6A6E   //(W1,W3)
#define  WM57  0x471D18F9   //(W5,W7)

#if 0
#define W1 2841 /* 2048*sqrt (2)*cos (1*pi/16) */
#define W2 2676 /* 2048*sqrt (2)*cos (2*pi/16) */
#define W3 2408 /* 2048*sqrt (2)*cos (3*pi/16) */
#define W4 2048 /* 2048*sqrt (2)*cos (4*pi/16) */
#define W5 1609 /* 2048*sqrt (2)*cos (5*pi/16) */
#define W6 1108 /* 2048*sqrt (2)*cos (6*pi/16) */
#define W7 565  /* 2048*sqrt (2)*cos (7*pi/16) */
#define ROW_SHIFT 8
#define COL_SHIFT 17
#else
#define W1  22725  //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W2  21407  //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W3  19266  //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W4  16383  //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W5  12873  //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W6  8867   //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W7  4520   //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define ROW_SHIFT 11
#define COL_SHIFT 20 // 6
#endif

static inline void idctRowCondDC (DCTELEM * row)
{
        int a0, a1, a2, a3, b0, b1, b2, b3;
#if HAVE_FAST_64BIT
        uint64_t temp;
#else
        uint32_t temp;
#endif

#if HAVE_FAST_64BIT
#if HAVE_BIGENDIAN
#define ROW0_MASK 0xffff000000000000LL
#else
#define ROW0_MASK 0xffffLL
#endif
        if(sizeof(DCTELEM)==2){
            if ( ((((uint64_t *)row)[0] & ~ROW0_MASK) |
                  ((uint64_t *)row)[1]) == 0) {
                temp = (row[0] << 3) & 0xffff;
                temp += temp << 16;
                temp += temp << 32;
                ((uint64_t *)row)[0] = temp;
                ((uint64_t *)row)[1] = temp;
                return;
            }
        }else{
            if (!(row[1]|row[2]|row[3]|row[4]|row[5]|row[6]|row[7])) {
                row[0]=row[1]=row[2]=row[3]=row[4]=row[5]=row[6]=row[7]= row[0] << 3;
                return;
            }
        }
#else
        if(sizeof(DCTELEM)==2){
            if (!(((uint32_t*)row)[1] |
                  ((uint32_t*)row)[2] |
                  ((uint32_t*)row)[3] |
                  row[1])) {
                temp = (row[0] << 3) & 0xffff;
                temp += temp << 16;
                ((uint32_t*)row)[0]=((uint32_t*)row)[1] =
                ((uint32_t*)row)[2]=((uint32_t*)row)[3] = temp;
                return;
            }
        }else{
            if (!(row[1]|row[2]|row[3]|row[4]|row[5]|row[6]|row[7])) {
                row[0]=row[1]=row[2]=row[3]=row[4]=row[5]=row[6]=row[7]= row[0] << 3;
                return;
            }
        }
#endif

        a0 = (W4 * row[0]) + (1 << (ROW_SHIFT - 1));
        a1 = a0;
        a2 = a0;
        a3 = a0;

        /* no need to optimize : gcc does it */
        a0 += W2 * row[2];
        a1 += W6 * row[2];
        a2 -= W6 * row[2];
        a3 -= W2 * row[2];

        b0 = MUL16(W1, row[1]);
        MAC16(b0, W3, row[3]);
        b1 = MUL16(W3, row[1]);
        MAC16(b1, -W7, row[3]);
        b2 = MUL16(W5, row[1]);
        MAC16(b2, -W1, row[3]);
        b3 = MUL16(W7, row[1]);
        MAC16(b3, -W5, row[3]);

#if HAVE_FAST_64BIT
        temp = ((uint64_t*)row)[1];
#else
        temp = ((uint32_t*)row)[2] | ((uint32_t*)row)[3];
#endif
        if (temp != 0) {
            a0 += W4*row[4] + W6*row[6];
            a1 += - W4*row[4] - W2*row[6];
            a2 += - W4*row[4] + W2*row[6];
            a3 += W4*row[4] - W6*row[6];

            MAC16(b0, W5, row[5]);
            MAC16(b0, W7, row[7]);

            MAC16(b1, -W1, row[5]);
            MAC16(b1, -W5, row[7]);

            MAC16(b2, W7, row[5]);
            MAC16(b2, W3, row[7]);

            MAC16(b3, W3, row[5]);
            MAC16(b3, -W1, row[7]);
        }

        row[0] = (a0 + b0) >> ROW_SHIFT;
        row[7] = (a0 - b0) >> ROW_SHIFT;
        row[1] = (a1 + b1) >> ROW_SHIFT;
        row[6] = (a1 - b1) >> ROW_SHIFT;
        row[2] = (a2 + b2) >> ROW_SHIFT;
        row[5] = (a2 - b2) >> ROW_SHIFT;
        row[3] = (a3 + b3) >> ROW_SHIFT;
        row[4] = (a3 - b3) >> ROW_SHIFT;
}

static inline void idctSparseColPut (uint8_t *dest, int line_size,
                                     DCTELEM * col)
{
        int a0, a1, a2, a3, b0, b1, b2, b3;
        uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

        /* XXX: I did that only to give same values as previous code */
        a0 = W4 * (col[8*0] + ((1<<(COL_SHIFT-1))/W4));
        a1 = a0;
        a2 = a0;
        a3 = a0;

        a0 +=  + W2*col[8*2];
        a1 +=  + W6*col[8*2];
        a2 +=  - W6*col[8*2];
        a3 +=  - W2*col[8*2];

        b0 = MUL16(W1, col[8*1]);
        b1 = MUL16(W3, col[8*1]);
        b2 = MUL16(W5, col[8*1]);
        b3 = MUL16(W7, col[8*1]);

        MAC16(b0, + W3, col[8*3]);
        MAC16(b1, - W7, col[8*3]);
        MAC16(b2, - W1, col[8*3]);
        MAC16(b3, - W5, col[8*3]);

        if(col[8*4]){
            a0 += + W4*col[8*4];
            a1 += - W4*col[8*4];
            a2 += - W4*col[8*4];
            a3 += + W4*col[8*4];
        }

        if (col[8*5]) {
            MAC16(b0, + W5, col[8*5]);
            MAC16(b1, - W1, col[8*5]);
            MAC16(b2, + W7, col[8*5]);
            MAC16(b3, + W3, col[8*5]);
        }

        if(col[8*6]){
            a0 += + W6*col[8*6];
            a1 += - W2*col[8*6];
            a2 += + W2*col[8*6];
            a3 += - W6*col[8*6];
        }

        if (col[8*7]) {
            MAC16(b0, + W7, col[8*7]);
            MAC16(b1, - W5, col[8*7]);
            MAC16(b2, + W3, col[8*7]);
            MAC16(b3, - W1, col[8*7]);
        }

        dest[0] = cm[(a0 + b0) >> COL_SHIFT];
        dest += line_size;
        dest[0] = cm[(a1 + b1) >> COL_SHIFT];
        dest += line_size;
        dest[0] = cm[(a2 + b2) >> COL_SHIFT];
        dest += line_size;
        dest[0] = cm[(a3 + b3) >> COL_SHIFT];
        dest += line_size;
        dest[0] = cm[(a3 - b3) >> COL_SHIFT];
        dest += line_size;
        dest[0] = cm[(a2 - b2) >> COL_SHIFT];
        dest += line_size;
        dest[0] = cm[(a1 - b1) >> COL_SHIFT];
        dest += line_size;
        dest[0] = cm[(a0 - b0) >> COL_SHIFT];
}

static inline void idctSparseColAdd (uint8_t *dest, int line_size,
                                     DCTELEM * col)
{
        int a0, a1, a2, a3, b0, b1, b2, b3;
        uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

        /* XXX: I did that only to give same values as previous code */
        a0 = W4 * (col[8*0] + ((1<<(COL_SHIFT-1))/W4));
        a1 = a0;
        a2 = a0;
        a3 = a0;

        a0 +=  + W2*col[8*2];
        a1 +=  + W6*col[8*2];
        a2 +=  - W6*col[8*2];
        a3 +=  - W2*col[8*2];

        b0 = MUL16(W1, col[8*1]);
        b1 = MUL16(W3, col[8*1]);
        b2 = MUL16(W5, col[8*1]);
        b3 = MUL16(W7, col[8*1]);

        MAC16(b0, + W3, col[8*3]);
        MAC16(b1, - W7, col[8*3]);
        MAC16(b2, - W1, col[8*3]);
        MAC16(b3, - W5, col[8*3]);

        if(col[8*4]){
            a0 += + W4*col[8*4];
            a1 += - W4*col[8*4];
            a2 += - W4*col[8*4];
            a3 += + W4*col[8*4];
        }

        if (col[8*5]) {
            MAC16(b0, + W5, col[8*5]);
            MAC16(b1, - W1, col[8*5]);
            MAC16(b2, + W7, col[8*5]);
            MAC16(b3, + W3, col[8*5]);
        }

        if(col[8*6]){
            a0 += + W6*col[8*6];
            a1 += - W2*col[8*6];
            a2 += + W2*col[8*6];
            a3 += - W6*col[8*6];
        }

        if (col[8*7]) {
            MAC16(b0, + W7, col[8*7]);
            MAC16(b1, - W5, col[8*7]);
            MAC16(b2, + W3, col[8*7]);
            MAC16(b3, - W1, col[8*7]);
        }

        dest[0] = cm[dest[0] + ((a0 + b0) >> COL_SHIFT)];
        dest += line_size;
        dest[0] = cm[dest[0] + ((a1 + b1) >> COL_SHIFT)];
        dest += line_size;
        dest[0] = cm[dest[0] + ((a2 + b2) >> COL_SHIFT)];
        dest += line_size;
        dest[0] = cm[dest[0] + ((a3 + b3) >> COL_SHIFT)];
        dest += line_size;
        dest[0] = cm[dest[0] + ((a3 - b3) >> COL_SHIFT)];
        dest += line_size;
        dest[0] = cm[dest[0] + ((a2 - b2) >> COL_SHIFT)];
        dest += line_size;
        dest[0] = cm[dest[0] + ((a1 - b1) >> COL_SHIFT)];
        dest += line_size;
        dest[0] = cm[dest[0] + ((a0 - b0) >> COL_SHIFT)];
}

static inline void idctSparseCol (DCTELEM * col)
{
        int a0, a1, a2, a3, b0, b1, b2, b3;

        /* XXX: I did that only to give same values as previous code */
        a0 = W4 * (col[8*0] + ((1<<(COL_SHIFT-1))/W4));
        a1 = a0;
        a2 = a0;
        a3 = a0;

        a0 +=  + W2*col[8*2];
        a1 +=  + W6*col[8*2];
        a2 +=  - W6*col[8*2];
        a3 +=  - W2*col[8*2];

        b0 = MUL16(W1, col[8*1]);
        b1 = MUL16(W3, col[8*1]);
        b2 = MUL16(W5, col[8*1]);
        b3 = MUL16(W7, col[8*1]);

        MAC16(b0, + W3, col[8*3]);
        MAC16(b1, - W7, col[8*3]);
        MAC16(b2, - W1, col[8*3]);
        MAC16(b3, - W5, col[8*3]);

        if(col[8*4]){
            a0 += + W4*col[8*4];
            a1 += - W4*col[8*4];
            a2 += - W4*col[8*4];
            a3 += + W4*col[8*4];
        }

        if (col[8*5]) {
            MAC16(b0, + W5, col[8*5]);
            MAC16(b1, - W1, col[8*5]);
            MAC16(b2, + W7, col[8*5]);
            MAC16(b3, + W3, col[8*5]);
        }

        if(col[8*6]){
            a0 += + W6*col[8*6];
            a1 += - W2*col[8*6];
            a2 += + W2*col[8*6];
            a3 += - W6*col[8*6];
        }

        if (col[8*7]) {
            MAC16(b0, + W7, col[8*7]);
            MAC16(b1, - W5, col[8*7]);
            MAC16(b2, + W3, col[8*7]);
            MAC16(b3, - W1, col[8*7]);
        }

        col[0 ] = ((a0 + b0) >> COL_SHIFT);
        col[8 ] = ((a1 + b1) >> COL_SHIFT);
        col[16] = ((a2 + b2) >> COL_SHIFT);
        col[24] = ((a3 + b3) >> COL_SHIFT);
        col[32] = ((a3 - b3) >> COL_SHIFT);
        col[40] = ((a2 - b2) >> COL_SHIFT);
        col[48] = ((a1 - b1) >> COL_SHIFT);
        col[56] = ((a0 - b0) >> COL_SHIFT);
}

#ifdef JZC_MXU_OPT
void ff_simple_idct_put(uint8_t *dest, int line_size, DCTELEM *block)
{
   DCTELEM *inptr = block, *endptr;

   S32I2M(xr5,WM44) ;         // xr5 (W4, W4)
   S32I2M(xr6,WM26) ;         // xr6 (W2, W6)
   S32I2M(xr7,WM13) ;         // xr7 (W1, W3)
   S32I2M(xr8,WM57) ;         // xr8 (W5, W7)

   endptr = inptr + 8;
// calculate column
   S32LDD(xr1, inptr, 0x00);    //  xr1 (nx0, x0)
   do {
// calculate a0,a1,a2,a3
      S32LDD(xr2, inptr, 0x20);    //  xr2 (nx2, x2)
      S32LDD(xr3, inptr, 0x40);    //  xr3 (nx4, x4)
      S32LDD(xr4, inptr, 0x60);    //  xr4 (nx6, x6)

// computer ah0,a0 ah3,a3
      D16MUL_HW(xr11,  xr5, xr1, xr13);    //xr11 (W4*nx0) xr13(W4*x0)
      D16MAC_AA_HW(xr11, xr5, xr3, xr13);  //xr11(W4*nx0 + W4*nx4) xr13(W4*x0 + W4*x4)
      D16MUL_HW(xr12,xr6,xr2,xr14);        //xr12 (W2*nx2) xr14(W2*x2)
      D16MAC_AA_LW(xr12,xr6,xr4,xr14);     //xr12 (W2*nx2 + W6*nx6) xr14(W2*x2 + W6*x6)
      D32ADD_AS(xr11, xr11, xr12, xr12);   //xr11 (W4*nx0 + W4*nx4 + W2*nx2 + W6*nx6),na0
                                           //xr12 (W4*nx0 + W4*nx4 - W2*nx2 - W6*nx6),na3
      D32ADD_AS(xr13, xr13, xr14, xr14);   //xr13 (W4*x0 + W4*x4 + W2*x2 + W6*x6),a0
                                           //xr14 (W4*x0 + W4*x4 - W2*x2 - W6*x6),a3
      D16MACF_AA_WW(xr11, xr0, xr0, xr13); // r11 (na0,a0)
      D16MACF_AA_WW(xr12, xr0, xr0, xr14); // r12 (na3,a3)
//
      D16MUL_HW(xr13,  xr5, xr1, xr10);      //xr13(W4*nx0) xr10(W4*x0)
      D16MAC_SS_HW(xr13, xr5, xr3, xr10);    //xr13(W4*nx0 - W4*nx4) xr10(W4*x0 - W4*x4)

      D16MUL_LW(xr14,xr6,xr2,xr9);          //xr14 (W6*nx2)  xr9(W6*x2)
      D16MAC_SS_HW(xr14,xr6,xr4,xr9);       //xr14 (W6*nx2 - W2*nx6) xr9(W6*x2 - W2*x6)
      D32ADD_AS(xr13, xr13, xr14, xr14);    //xr13 (W4*nx0 - W4*nx4 + W6*nx2 - W2*nx6),na1
                                            //xr14 (W4*nx0 - W4*nx4 - W6*nx2 + W2*nx6),na2
      D32ADD_AS(xr10, xr10, xr9, xr9);      //xr10 (W4*x0 - W4*x4 + W6*x2 - W2*x6),a1
                                            //xr9 (W4*x0 - W4*x4 - W6*x2 + W2*x6),a2
      D16MACF_AA_WW(xr13, xr0, xr0, xr10);  // r13 (na1,a1)
      D16MACF_AA_WW(xr14, xr0, xr0, xr9);   // r14 (na2,a2)

//-------------------------------------------------------------------------------------
      S32LDD(xr1, inptr, 0x10);    //  xr1 (nx1, x1)
      S32LDD(xr2, inptr, 0x30);    //  xr2 (nx3, x3)
      S32LDD(xr3, inptr, 0x50);    //  xr3 (nx5, x5)
      S32LDD(xr4, inptr, 0x70);    //  xr4 (nx7, x7)

// calculate b0,b1,b2,b3
// calculate b0
      D16MUL_HW(xr9, xr7, xr1, xr10);      //xr9(nx1 * W1) xr10(x1 * W1)
      D16MAC_AA_LW(xr9, xr7, xr2, xr10);   //xr9(nx1*W1 + nx3*W3) xr10(x1*W1 + x3*W3)
      D16MAC_AA_HW(xr9, xr8, xr3, xr10);   //xr9(nx1*W1 + nx3*W3 + nx5*W5)
                                           //xr10(x1*W1 + x3*W3 + x5*W5)
      D16MAC_AA_LW(xr9, xr8, xr4, xr10);   //xr9(nx1*W1 + nx3*W3 + nx5*W5 + nx7*W7)
                                           //xr10(x1*W1 + x3*W3 + x5*W5 + nx7*W7)
      D16MACF_AA_WW(xr9, xr0, xr0, xr10);  //xr9 (nb0,b0)
// calculate b1
      D16MUL_LW(xr10, xr7, xr1, xr15);      //xr10(nx1 * W3) xr15(x1 * W3)
      D16MAC_SS_LW(xr10, xr8, xr2, xr15);   //xr10(nx1*W3 - nx3*W7) xr15(x1*W3 - x3*W7)
      D16MAC_SS_HW(xr10, xr7, xr3, xr15);   //xr10(nx1*W3 - nx3*W7 - nx5*W1)
                                            //xr15(x1*W3 - x3*W7 - x5*W1)
      D16MAC_SS_HW(xr10, xr8, xr4, xr15);   //xr10(nx1*W3 - nx3*W7 - nx5*W1 - nx7*W5)
                                            //xr15(x1*W3 - x3*W7 - x5*W1 - nx7*W5)
      D16MACF_AA_WW(xr10, xr0, xr0, xr15);  //xr10 (nb1,b1)
// store result
      Q16ADD_AS_WW(xr11,xr11,xr9,xr9);      //xr11(na0+nb0,a0+b0) xr9(na0-nb0, a0-b0)
      S32STD(xr11, inptr, 0x00);
      S32STD(xr9,  inptr, 0x70);
      Q16ADD_AS_WW(xr13,xr13,xr10,xr10);    //xr13(na1+nb1,a1+b1) xr10(na1-nb1, a1-b1)
      S32STD(xr13, inptr, 0x10);
      S32STD(xr10, inptr, 0x60);
// calculate b2
      D16MUL_HW(xr9, xr8, xr1, xr10);      //xr9(nx1 * W5) xr10(x1 * W5)
      D16MAC_SS_HW(xr9, xr7, xr2, xr10);   //xr9(nx1*W5 - nx3*W1) xr10(x1*W5 - x3*W1)
      D16MAC_AA_LW(xr9, xr8, xr3, xr10);   //xr9(nx1*W5 - nx3*W1 + nx5*W7)
                                           //xr10(x1*W5 - x3*W1 + x5*W7)
      D16MAC_AA_LW(xr9, xr7, xr4, xr10);   //xr9(nx1*W5 - nx3*W1 + nx5*W7 + nx7*W3)
                                           //xr10(x1*W5 - x3*W1 + x5*W7 + nx7*W3)
      D16MACF_AA_WW(xr9, xr0, xr0, xr10);  //xr9 (nb2,b2)
// calculate b3
      D16MUL_LW(xr10, xr8, xr1, xr15);      //xr10(nx1 * W7) xr15(x1 * W7)
      D16MAC_SS_HW(xr10, xr8, xr2, xr15);   //xr10(nx1*W7 - nx3*W5) xr15(x1*W7 - x3*W5)
      D16MAC_AA_LW(xr10, xr7, xr3, xr15);   //xr10(nx1*W7 - nx3*W5 + nx5*W3)
                                            //xr15(x1*W7 - x3*W5 + x5*W3)
      D16MAC_SS_HW(xr10, xr7, xr4, xr15);   //xr10(nx1*W7 - nx3*W5 + nx5*W3 - nx7*W1)
                                            //xr15(x1*W7 - x3*W5 + x5*W3 - nx7*W1)
      D16MACF_AA_WW(xr10, xr0, xr0, xr15);  //xr10 (nb3,b3)
// store result
      Q16ADD_AS_WW(xr14,xr14,xr9,xr9);      //xr14(na2+nb2,a2+b2) xr9(na2-nb2, a2-b2)
      S32STD(xr14, inptr, 0x20);
      S32STD(xr9,  inptr, 0x50);
      Q16ADD_AS_WW(xr12,xr12,xr10,xr10);    //xr12(na3+nb3,a3+b3) xr10(na3-nb3, a3-b3)
      S32LDI(xr1,  inptr, 0x04);       //  xr1 (nx0, x0)
      S32STD(xr12, inptr, 0x2c);
      S32STD(xr10, inptr, 0x3c);
   } while (inptr != endptr);

   inptr = block;
   endptr = inptr + 8*8;
// calculate line
   S32LDD(xr1, inptr, 0);          //  xr1 (x1, x0)
   do {
      S32LDD(xr2, inptr, 0x4);        //  xr2 (x3, x2)
      S32LDD(xr3, inptr, 0x8);        //  xr3 (x5, x4)
      S32LDD(xr4, inptr, 0xc);        //  xr4 (x7, x6)

// calculate b0,b1,b2,b3
      D16MUL_HW(xr11, xr1, xr7, xr12); // xr11 (W1*x1)  xr12 (W3*x1)
      D16MUL_HW(xr13, xr1, xr8, xr14); // xr13 (W5*x1)  xr14 (W7*x1)
      D16MAC_SA_HW(xr13,xr2,xr7,xr11); // xr13 (W5*x1 - W1*x3) xr11 (W1*x1 + W3*x3)
      D16MAC_SS_HW(xr14,xr2,xr8,xr12); // xr14 (W7*x1 - W5*x3) xr12 (W3*x1 - W7*x3)
      D16MAC_SA_HW(xr12,xr3,xr7,xr14); // xr12 (W3*x1 - W7*x3 - W1*x5)
                                       // xr14 (W7*x1 - W5*x3 + W3*x5)
      D16MAC_AA_HW(xr11,xr3,xr8,xr13); // xr11 (W1*x1 + W3*x3 + W5*x5)
                                       // xr13 (W5*x1 - W1*x3 + W7*x5)
//calculate a0,a1,a2,a3
      D16MUL_LW(xr9, xr1, xr5, xr10); // xr9 (W4*x0)  xr10 (W4*x0)
      D16MAC_AS_LW(xr9,xr3,xr5,xr10); // xr9 (W4*x0 + W4*x4) xr10 (W4*x0 - W4*x4)
      D16MUL_LW(xr1, xr2, xr6, xr3); // xr1 (W2*x2)  xr3 (W6*x2)
      D16MAC_SA_LW(xr3,xr4,xr6,xr1); // xr3 (W6*x2 - W2*x6) xr1 (W2*x2 + W6*x6)
//schedule b0 ~ b3
      D16MAC_SA_HW(xr12,xr4,xr8,xr11); // xr12 (W3*x1 - W7*x3 - W1*x5 - W5*x7),b1
                                       // xr11 (W1*x1 + W3*x3 + W5*x5 + W7*x7),b0
      D16MAC_SA_HW(xr14,xr4,xr7,xr13); // xr14 (W7*x1 - W5*x3 + W3*x5 - W1*x7),b3
                                       // xr13 (W5*x1 - W1*x3 + W7*x5 + W3*x7),b2
// continue a0 ~ a3
      D32ADD_AS(xr2, xr9, xr1, xr4);   //xr2 (W4*x0 + W4*x4 + W2*x2 + W6*x6),a0
                                       //xr4 (W4*x0 + W4*x4 - W2*x2 - W6*x6),a3
      D32ADD_AS(xr9,xr10,xr3, xr1);    //xr9(W4*x0 - W4*x4 + W6*x2 - W2*x6),a1
                                       //xr1(W4*x0 - W4*x4 - W6*x2 + W2*x6),a2
//calculate a +/- b
      D32ADD_AS(xr2, xr2, xr11, xr11); //xr2(a0 + b0)  xr11 (a0 - b0)
      D32ADD_AS(xr4, xr4, xr14, xr14); //xr4(a3 + b3)  xr14 (a3 - b3)
      D32ADD_AS(xr9, xr9, xr12, xr12); //xr9(a1 + b1)  xr12 (a1 - b1)
      D32ADD_AS(xr1, xr1, xr13, xr13); //xr1(a2 + b2)  xr13 (a2 - b2)
//padding and saturate
      D16MACF_AA_WW(xr9, xr0, xr0, xr2);  // r9 (a1+b1, a0+b0)
      D16MACF_AA_WW(xr4, xr0, xr0, xr1);  // r4 (a3+b3, a2+b2)
      D16MACF_AA_WW(xr13,xr0, xr0, xr14); // r13(a2-b2, a3-b3)
      D16MACF_AA_WW(xr11,xr0, xr0, xr12); // r11(a0-b0, a1-b1)

      Q16SAT(xr4, xr4, xr9);
      Q16SAT(xr11,xr11, xr13);
      S32LDI(xr1, inptr,0x10);          //  xr1 (x1, x0)
      S32STD(xr4, dest, 0x0);
      S32STD(xr11,dest, 0x4);
      dest += line_size;
   } while (inptr != endptr);
}
#else
void ff_simple_idct_put(uint8_t *dest, int line_size, DCTELEM *block)
{
    int i;
    for(i=0; i<8; i++)
        idctRowCondDC(block + i*8);

    for(i=0; i<8; i++)
        idctSparseColPut(dest + i, line_size, block + i);
}
#endif

#ifdef JZC_MXU_OPT
void ff_simple_idct_add(uint8_t *dest, int line_size, DCTELEM *block)
{
   DCTELEM *inptr = block, *endptr;

   S32I2M(xr5,WM44) ;         // xr5 (W4, W4)
   S32I2M(xr6,WM26) ;         // xr6 (W2, W6)
   S32I2M(xr7,WM13) ;         // xr7 (W1, W3)
   S32I2M(xr8,WM57) ;         // xr8 (W5, W7)

   endptr = inptr + 8;
// calculate column
   S32LDD(xr1, inptr, 0x00);    //  xr1 (nx0, x0)
   do {
// calculate a0,a1,a2,a3
      S32LDD(xr2, inptr, 0x20);    //  xr2 (nx2, x2)
      S32LDD(xr3, inptr, 0x40);    //  xr3 (nx4, x4)
      S32LDD(xr4, inptr, 0x60);    //  xr4 (nx6, x6)

// computer ah0,a0 ah3,a3
      D16MUL_HW(xr11,  xr5, xr1, xr13);    //xr11 (W4*nx0) xr13(W4*x0)
      D16MAC_AA_HW(xr11, xr5, xr3, xr13);  //xr11(W4*nx0 + W4*nx4) xr13(W4*x0 + W4*x4)
      D16MUL_HW(xr12,xr6,xr2,xr14);        //xr12 (W2*nx2) xr14(W2*x2)
      D16MAC_AA_LW(xr12,xr6,xr4,xr14);     //xr12 (W2*nx2 + W6*nx6) xr14(W2*x2 + W6*x6)
      D32ADD_AS(xr11, xr11, xr12, xr12);   //xr11 (W4*nx0 + W4*nx4 + W2*nx2 + W6*nx6),na0
                                           //xr12 (W4*nx0 + W4*nx4 - W2*nx2 - W6*nx6),na3
      D32ADD_AS(xr13, xr13, xr14, xr14);   //xr13 (W4*x0 + W4*x4 + W2*x2 + W6*x6),a0
                                           //xr14 (W4*x0 + W4*x4 - W2*x2 - W6*x6),a3
      D16MACF_AA_WW(xr11, xr0, xr0, xr13); // r11 (na0,a0)
      D16MACF_AA_WW(xr12, xr0, xr0, xr14); // r12 (na3,a3)
//
      D16MUL_HW(xr13,  xr5, xr1, xr10);      //xr13(W4*nx0) xr10(W4*x0)
      D16MAC_SS_HW(xr13, xr5, xr3, xr10);    //xr13(W4*nx0 - W4*nx4) xr10(W4*x0 - W4*x4)

      D16MUL_LW(xr14,xr6,xr2,xr9);          //xr14 (W6*nx2)  xr9(W6*x2)
      D16MAC_SS_HW(xr14,xr6,xr4,xr9);       //xr14 (W6*nx2 - W2*nx6) xr9(W6*x2 - W2*x6)
      D32ADD_AS(xr13, xr13, xr14, xr14);    //xr13 (W4*nx0 - W4*nx4 + W6*nx2 - W2*nx6),na1
                                            //xr14 (W4*nx0 - W4*nx4 - W6*nx2 + W2*nx6),na2
      D32ADD_AS(xr10, xr10, xr9, xr9);      //xr10 (W4*x0 - W4*x4 + W6*x2 - W2*x6),a1
                                            //xr9 (W4*x0 - W4*x4 - W6*x2 + W2*x6),a2
      D16MACF_AA_WW(xr13, xr0, xr0, xr10);  // r13 (na1,a1)
      D16MACF_AA_WW(xr14, xr0, xr0, xr9);   // r14 (na2,a2)

//-------------------------------------------------------------------------------------
      S32LDD(xr1, inptr, 0x10);    //  xr1 (nx1, x1)
      S32LDD(xr2, inptr, 0x30);    //  xr2 (nx3, x3)
      S32LDD(xr3, inptr, 0x50);    //  xr3 (nx5, x5)
      S32LDD(xr4, inptr, 0x70);    //  xr4 (nx7, x7)

// calculate b0,b1,b2,b3
// calculate b0
      D16MUL_HW(xr9, xr7, xr1, xr10);      //xr9(nx1 * W1) xr10(x1 * W1)
      D16MAC_AA_LW(xr9, xr7, xr2, xr10);   //xr9(nx1*W1 + nx3*W3) xr10(x1*W1 + x3*W3)
      D16MAC_AA_HW(xr9, xr8, xr3, xr10);   //xr9(nx1*W1 + nx3*W3 + nx5*W5)
                                           //xr10(x1*W1 + x3*W3 + x5*W5)
      D16MAC_AA_LW(xr9, xr8, xr4, xr10);   //xr9(nx1*W1 + nx3*W3 + nx5*W5 + nx7*W7)
                                           //xr10(x1*W1 + x3*W3 + x5*W5 + nx7*W7)
      D16MACF_AA_WW(xr9, xr0, xr0, xr10);  //xr9 (nb0,b0)
// calculate b1
      D16MUL_LW(xr10, xr7, xr1, xr15);      //xr10(nx1 * W3) xr15(x1 * W3)
      D16MAC_SS_LW(xr10, xr8, xr2, xr15);   //xr10(nx1*W3 - nx3*W7) xr15(x1*W3 - x3*W7)
      D16MAC_SS_HW(xr10, xr7, xr3, xr15);   //xr10(nx1*W3 - nx3*W7 - nx5*W1)
                                            //xr15(x1*W3 - x3*W7 - x5*W1)
      D16MAC_SS_HW(xr10, xr8, xr4, xr15);   //xr10(nx1*W3 - nx3*W7 - nx5*W1 - nx7*W5)
                                            //xr15(x1*W3 - x3*W7 - x5*W1 - nx7*W5)
      D16MACF_AA_WW(xr10, xr0, xr0, xr15);  //xr10 (nb1,b1)
// store result
      Q16ADD_AS_WW(xr11,xr11,xr9,xr9);      //xr11(na0+nb0,a0+b0) xr9(na0-nb0, a0-b0)
      S32STD(xr11, inptr, 0x00);
      S32STD(xr9,  inptr, 0x70);
      Q16ADD_AS_WW(xr13,xr13,xr10,xr10);    //xr13(na1+nb1,a1+b1) xr10(na1-nb1, a1-b1)
      S32STD(xr13, inptr, 0x10);
      S32STD(xr10, inptr, 0x60);
// calculate b2
      D16MUL_HW(xr9, xr8, xr1, xr10);      //xr9(nx1 * W5) xr10(x1 * W5)
      D16MAC_SS_HW(xr9, xr7, xr2, xr10);   //xr9(nx1*W5 - nx3*W1) xr10(x1*W5 - x3*W1)
      D16MAC_AA_LW(xr9, xr8, xr3, xr10);   //xr9(nx1*W5 - nx3*W1 + nx5*W7)
                                           //xr10(x1*W5 - x3*W1 + x5*W7)
      D16MAC_AA_LW(xr9, xr7, xr4, xr10);   //xr9(nx1*W5 - nx3*W1 + nx5*W7 + nx7*W3)
                                           //xr10(x1*W5 - x3*W1 + x5*W7 + nx7*W3)
      D16MACF_AA_WW(xr9, xr0, xr0, xr10);  //xr9 (nb2,b2)
// calculate b3
      D16MUL_LW(xr10, xr8, xr1, xr15);      //xr10(nx1 * W7) xr15(x1 * W7)
      D16MAC_SS_HW(xr10, xr8, xr2, xr15);   //xr10(nx1*W7 - nx3*W5) xr15(x1*W7 - x3*W5)
      D16MAC_AA_LW(xr10, xr7, xr3, xr15);   //xr10(nx1*W7 - nx3*W5 + nx5*W3)
                                            //xr15(x1*W7 - x3*W5 + x5*W3)
      D16MAC_SS_HW(xr10, xr7, xr4, xr15);   //xr10(nx1*W7 - nx3*W5 + nx5*W3 - nx7*W1)
                                            //xr15(x1*W7 - x3*W5 + x5*W3 - nx7*W1)
      D16MACF_AA_WW(xr10, xr0, xr0, xr15);  //xr10 (nb3,b3)
// store result
      Q16ADD_AS_WW(xr14,xr14,xr9,xr9);      //xr14(na2+nb2,a2+b2) xr9(na2-nb2, a2-b2)
      S32STD(xr14, inptr, 0x20);
      S32STD(xr9,  inptr, 0x50);
      Q16ADD_AS_WW(xr12,xr12,xr10,xr10);    //xr12(na3+nb3,a3+b3) xr10(na3-nb3, a3-b3)
      S32LDI(xr1, inptr, 0x04);    //  xr1 (nx0, x0)
      S32STD(xr12, inptr, 0x2c);
      S32STD(xr10, inptr, 0x3c);
   } while (inptr != endptr);

   inptr = block;
   endptr = inptr + 8*8;
   S32LDD(xr1, inptr, 0);          //  xr1 (x1, x0)
// calculate line
   do {
      S32LDD(xr2, inptr, 0x4);        //  xr2 (x3, x2)
      S32LDD(xr3, inptr, 0x8);        //  xr3 (x5, x4)
      S32LDD(xr4, inptr, 0xc);        //  xr4 (x7, x6)

// calculate b0,b1,b2,b3
      D16MUL_HW(xr11, xr1, xr7, xr12); // xr11 (W1*x1)  xr12 (W3*x1)
      D16MUL_HW(xr13, xr1, xr8, xr14); // xr13 (W5*x1)  xr14 (W7*x1)
      D16MAC_SA_HW(xr13,xr2,xr7,xr11); // xr13 (W5*x1 - W1*x3) xr11 (W1*x1 + W3*x3)
      D16MAC_SS_HW(xr14,xr2,xr8,xr12); // xr14 (W7*x1 - W5*x3) xr12 (W3*x1 - W7*x3)
      D16MAC_SA_HW(xr12,xr3,xr7,xr14); // xr12 (W3*x1 - W7*x3 - W1*x5)
                                       // xr14 (W7*x1 - W5*x3 + W3*x5)
      D16MAC_AA_HW(xr11,xr3,xr8,xr13); // xr11 (W1*x1 + W3*x3 + W5*x5)
                                       // xr13 (W5*x1 - W1*x3 + W7*x5)
//calculate a0,a1,a2,a3
      D16MUL_LW(xr9, xr1, xr5, xr10); // xr9 (W4*x0)  xr10 (W4*x0)
      D16MAC_AS_LW(xr9,xr3,xr5,xr10); // xr9 (W4*x0 + W4*x4) xr10 (W4*x0 - W4*x4)
      D16MUL_LW(xr1, xr2, xr6, xr3); // xr1 (W2*x2)  xr3 (W6*x2)
      D16MAC_SA_LW(xr3,xr4,xr6,xr1); // xr3 (W6*x2 - W2*x6) xr1 (W2*x2 + W6*x6)
//schedule b0 ~ b3
      D16MAC_SA_HW(xr12,xr4,xr8,xr11); // xr12 (W3*x1 - W7*x3 - W1*x5 - W5*x7),b1
                                       // xr11 (W1*x1 + W3*x3 + W5*x5 + W7*x7),b0
      D16MAC_SA_HW(xr14,xr4,xr7,xr13); // xr14 (W7*x1 - W5*x3 + W3*x5 - W1*x7),b3
                                       // xr13 (W5*x1 - W1*x3 + W7*x5 + W3*x7),b2
// continue a0 ~ a3
      D32ADD_AS(xr2, xr9, xr1, xr4);   //xr2 (W4*x0 + W4*x4 + W2*x2 + W6*x6),a0
                                       //xr4 (W4*x0 + W4*x4 - W2*x2 - W6*x6),a3
      D32ADD_AS(xr9,xr10,xr3, xr1);    //xr9(W4*x0 - W4*x4 + W6*x2 - W2*x6),a1
                                       //xr1(W4*x0 - W4*x4 - W6*x2 + W2*x6),a2
//calculate a +/- b
      D32ADD_AS(xr2, xr2, xr11, xr11); //xr2(a0 + b0)  xr11 (a0 - b0)
      D32ADD_AS(xr4, xr4, xr14, xr14); //xr4(a3 + b3)  xr14 (a3 - b3)
      D32ADD_AS(xr9, xr9, xr12, xr12); //xr9(a1 + b1)  xr12 (a1 - b1)
      D32ADD_AS(xr1, xr1, xr13, xr13); //xr1(a2 + b2)  xr13 (a2 - b2)
//padding and saturate
      D16MACF_AA_WW(xr9, xr0, xr0, xr2);  // r9 (a1+b1, a0+b0)
      D16MACF_AA_WW(xr4, xr0, xr0, xr1);  // r4 (a3+b3, a2+b2)
      D16MACF_AA_WW(xr13,xr0, xr0, xr14); // r13(a2-b2, a3-b3)
      D16MACF_AA_WW(xr11,xr0, xr0, xr12); // r11(a0-b0, a1-b1)
      S32LDD(xr1, dest, 0x00);    //  xr1 (x3, x2, x1, x0)
      S32LDD(xr2, dest, 0x04);    //  xr2 (x7, x6, x5, x4)
      Q8ACCE_AA(xr4, xr1,  xr0, xr9);
      Q8ACCE_AA(xr11, xr2, xr0, xr13);
      S32LDI(xr1, inptr, 0x10);          //  xr1 (x1, x0)

      Q16SAT(xr4, xr4, xr9);
      Q16SAT(xr11,xr11, xr13);
      S32STD(xr4, dest, 0x0);
      S32STD(xr11,dest, 0x4);
      dest += line_size;
   } while (inptr != endptr);
}
#else
void ff_simple_idct_add(uint8_t *dest, int line_size, DCTELEM *block)
{
    int i;
    for(i=0; i<8; i++)
        idctRowCondDC(block + i*8);

    for(i=0; i<8; i++)
        idctSparseColAdd(dest + i, line_size, block + i);
}
#endif

void ff_simple_idct(DCTELEM *block)
{
    int i;
    for(i=0; i<8; i++)
        idctRowCondDC(block + i*8);

    for(i=0; i<8; i++)
        idctSparseCol(block + i);
}

/* 2x4x8 idct */

#define CN_SHIFT 12
#define C_FIX(x) ((int)((x) * (1 << CN_SHIFT) + 0.5))
#define C1 C_FIX(0.6532814824)
#define C2 C_FIX(0.2705980501)

/* row idct is multiple by 16 * sqrt(2.0), col idct4 is normalized,
   and the butterfly must be multiplied by 0.5 * sqrt(2.0) */
#define C_SHIFT (4+1+12)

static inline void idct4col_put(uint8_t *dest, int line_size, const DCTELEM *col)
{
    int c0, c1, c2, c3, a0, a1, a2, a3;
    const uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    a0 = col[8*0];
    a1 = col[8*2];
    a2 = col[8*4];
    a3 = col[8*6];
    c0 = ((a0 + a2) << (CN_SHIFT - 1)) + (1 << (C_SHIFT - 1));
    c2 = ((a0 - a2) << (CN_SHIFT - 1)) + (1 << (C_SHIFT - 1));
    c1 = a1 * C1 + a3 * C2;
    c3 = a1 * C2 - a3 * C1;
    dest[0] = cm[(c0 + c1) >> C_SHIFT];
    dest += line_size;
    dest[0] = cm[(c2 + c3) >> C_SHIFT];
    dest += line_size;
    dest[0] = cm[(c2 - c3) >> C_SHIFT];
    dest += line_size;
    dest[0] = cm[(c0 - c1) >> C_SHIFT];
}

#define BF(k) \
{\
    int a0, a1;\
    a0 = ptr[k];\
    a1 = ptr[8 + k];\
    ptr[k] = a0 + a1;\
    ptr[8 + k] = a0 - a1;\
}

/* only used by DV codec. The input must be interlaced. 128 is added
   to the pixels before clamping to avoid systematic error
   (1024*sqrt(2)) offset would be needed otherwise. */
/* XXX: I think a 1.0/sqrt(2) normalization should be needed to
   compensate the extra butterfly stage - I don't have the full DV
   specification */
void ff_simple_idct248_put(uint8_t *dest, int line_size, DCTELEM *block)
{
    int i;
    DCTELEM *ptr;

    /* butterfly */
    ptr = block;
    for(i=0;i<4;i++) {
        BF(0);
        BF(1);
        BF(2);
        BF(3);
        BF(4);
        BF(5);
        BF(6);
        BF(7);
        ptr += 2 * 8;
    }

    /* IDCT8 on each line */
    for(i=0; i<8; i++) {
        idctRowCondDC(block + i*8);
    }

    /* IDCT4 and store */
    for(i=0;i<8;i++) {
        idct4col_put(dest + i, 2 * line_size, block + i);
        idct4col_put(dest + line_size + i, 2 * line_size, block + 8 + i);
    }
}

/* 8x4 & 4x8 WMV2 IDCT */
#undef CN_SHIFT
#undef C_SHIFT
#undef C_FIX
#undef C1
#undef C2
#define CN_SHIFT 12
#define C_FIX(x) ((int)((x) * 1.414213562 * (1 << CN_SHIFT) + 0.5))
#define C1 C_FIX(0.6532814824)
#define C2 C_FIX(0.2705980501)
#define C3 C_FIX(0.5)
#define C_SHIFT (4+1+12)
static inline void idct4col_add(uint8_t *dest, int line_size, const DCTELEM *col)
{
    int c0, c1, c2, c3, a0, a1, a2, a3;
    const uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    a0 = col[8*0];
    a1 = col[8*1];
    a2 = col[8*2];
    a3 = col[8*3];
    c0 = (a0 + a2)*C3 + (1 << (C_SHIFT - 1));
    c2 = (a0 - a2)*C3 + (1 << (C_SHIFT - 1));
    c1 = a1 * C1 + a3 * C2;
    c3 = a1 * C2 - a3 * C1;
    dest[0] = cm[dest[0] + ((c0 + c1) >> C_SHIFT)];
    dest += line_size;
    dest[0] = cm[dest[0] + ((c2 + c3) >> C_SHIFT)];
    dest += line_size;
    dest[0] = cm[dest[0] + ((c2 - c3) >> C_SHIFT)];
    dest += line_size;
    dest[0] = cm[dest[0] + ((c0 - c1) >> C_SHIFT)];
}

#define RN_SHIFT 15
#define R_FIX(x) ((int)((x) * 1.414213562 * (1 << RN_SHIFT) + 0.5))
#define R1 R_FIX(0.6532814824)
#define R2 R_FIX(0.2705980501)
#define R3 R_FIX(0.5)
#define R_SHIFT 11
static inline void idct4row(DCTELEM *row)
{
    int c0, c1, c2, c3, a0, a1, a2, a3;
    //const uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    a0 = row[0];
    a1 = row[1];
    a2 = row[2];
    a3 = row[3];
    c0 = (a0 + a2)*R3 + (1 << (R_SHIFT - 1));
    c2 = (a0 - a2)*R3 + (1 << (R_SHIFT - 1));
    c1 = a1 * R1 + a3 * R2;
    c3 = a1 * R2 - a3 * R1;
    row[0]= (c0 + c1) >> R_SHIFT;
    row[1]= (c2 + c3) >> R_SHIFT;
    row[2]= (c2 - c3) >> R_SHIFT;
    row[3]= (c0 - c1) >> R_SHIFT;
}

void ff_simple_idct84_add(uint8_t *dest, int line_size, DCTELEM *block)
{
    int i;

    /* IDCT8 on each line */
    for(i=0; i<4; i++) {
        idctRowCondDC(block + i*8);
    }

    /* IDCT4 and store */
    for(i=0;i<8;i++) {
        idct4col_add(dest + i, line_size, block + i);
    }
}

void ff_simple_idct48_add(uint8_t *dest, int line_size, DCTELEM *block)
{
    int i;

    /* IDCT4 on each line */
    for(i=0; i<8; i++) {
        idct4row(block + i*8);
    }

    /* IDCT8 and store */
    for(i=0; i<4; i++){
        idctSparseColAdd(dest + i, line_size, block + i);
    }
}

void ff_simple_idct44_add(uint8_t *dest, int line_size, DCTELEM *block)
{
    int i;

    /* IDCT4 on each line */
    for(i=0; i<4; i++) {
        idct4row(block + i*8);
    }

    /* IDCT4 and store */
    for(i=0; i<4; i++){
        idct4col_add(dest + i, line_size, block + i);
    }
}
