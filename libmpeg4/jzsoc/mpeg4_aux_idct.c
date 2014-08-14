#include "../../libjzcommon/jzmedia.h"
#include "mpeg4_dcore.h"
extern MPEG4_Frame_GlbARGs *dFRM;
extern MPEG4_MB_DecARGs *dMB_L;

#define  wxr5  0x5A827642
#define  wxr6  0x5A8230FC
#define  wxr7  0x7D876A6E
#define  wxr8  0x18F9471D
#define  wxr9  0x6A6E18F9
#define  wxr10  0x471D7D87

static int32_t whirl_idct[6] = {wxr5, wxr6, wxr7, wxr8, wxr9, wxr10};

static void dct_unquantize_mpeg2_intra_c_opt(DCTELEM *block, int n, int qscale);
static void dct_unquantize_mpeg2_inter_c_opt(DCTELEM *block, int n, int qscale);
static void dct_unquantize_h263_intra_c_opt(DCTELEM *block, int n, int qscale);
static void dct_unquantize_h263_inter_c_opt(DCTELEM *block, int n, int qscale);

#define  WM44  0x5A825A82   //(W4,W4)
#define  WM26  0x764230FC   //(W2,W6)
#define  WM13  0x7D8A6A6E   //(W1,W3)
#define  WM57  0x471D18F9   //(W5,W7)

static void simple_idct_put_mxu(uint8_t *dest, int yuv_len, int line_size, DCTELEM *block)
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

static void simple_idct_add_mxu(uint8_t *dest, int yuv_len, int line_size, DCTELEM *block)
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

static void ff_simple_idct_add_mxu (uint8_t *dest,int yuv_len, int line_size, DCTELEM *block)
{
  int i;
  int16_t  *blk;
  int32_t wf = (int32_t)whirl_idct;

  S32LDD(xr5, wf, 0x0);         // xr5(w7, w3)
  S32LDD(xr6, wf, 0x4);         // xr6(w9, w8)
  S32LDD(xr7, wf, 0x8);         // xr7(w11,w10)
  S32LDD(xr8, wf, 0xc);         // xr8(w13,w12)
  S32LDD(xr9, wf, 0x10);        // xr9(w6, w0)
  S32LDD(xr10,wf, 0x14);

  blk = block - 8;
  for(i=0; i<yuv_len; i++){
    S32LDI(xr1, blk, 0x10);        //  xr1 (x4, x0)
    S32LDD(xr2, blk, 0x4);        //  xr2 (x7, x3)
    S32LDD(xr3, blk, 0x8);        //  xr3 (x6, x1)
    S32LDD(xr4, blk, 0xc);        //  xr4 (x5, x2)
    
    S32SFL(xr1,xr1,xr2,xr2, ptn3);
    S32SFL(xr3,xr3,xr4,xr4, ptn3);
    
    D16MUL_WW(xr11, xr2, xr5, xr12);
    D16MAC_AA_WW(xr11,xr4,xr6,xr12);
    
    D16MUL_WW(xr13, xr2, xr6, xr14);
    D16MAC_SS_WW(xr13,xr4,xr5,xr14);
    
    D16MUL_HW(xr2, xr1, xr7, xr4);
    D16MAC_AS_LW(xr2,xr1,xr9,xr4);
    D16MAC_AS_HW(xr2,xr3,xr10,xr4);
    D16MAC_AS_LW(xr2,xr3,xr8,xr4);
    D16MACF_AA_WW(xr2, xr0, xr0, xr4);
    D16MACF_AA_WW(xr11, xr0, xr0, xr13);
    D16MACF_AA_WW(xr12, xr0, xr0, xr14);
    
    D16MUL_HW(xr4, xr1, xr8, xr15);
    D16MAC_SS_LW(xr4,xr1,xr10,xr15);
    D16MAC_AA_HW(xr4,xr3,xr9,xr15);
    D16MAC_SA_LW(xr4,xr3,xr7,xr15);
    Q16ADD_AS_WW(xr11,xr11,xr12,xr12); 
    D16MACF_AA_WW(xr15, xr0, xr0, xr4);
    
    Q16ADD_AS_WW(xr11, xr11, xr2, xr2);
    Q16ADD_AS_XW(xr12, xr12, xr15, xr15);
    
    S32SFL(xr11,xr11,xr12,xr12, ptn3);
    S32SFL(xr12,xr12,xr11,xr11, ptn3);
    
    S32STD(xr12, blk, 0x0);
    S32STD(xr11, blk, 0x4);
    S32STD(xr15, blk, 0x8);
    S32STD(xr2, blk, 0xc);
  } 

  blk  = block - 2;
  for (i = 0; i < 4; i++)               /* idct columns */
    {
      S32LDI(xr1, blk, 0x4);
      S32LDD(xr3, blk, 0x20);
      S32I2M(xr5,wxr5);
      S32LDD(xr11, blk, 0x40);
      S32LDD(xr13, blk, 0x60);

      D16MUL_HW(xr15, xr5, xr1, xr9);
      D16MAC_AA_HW(xr15,xr5,xr11,xr9);
      D16MACF_AA_WW(xr15, xr0, xr0, xr9);
      D16MUL_LW(xr10, xr5, xr3, xr9);
      D16MAC_AA_LW(xr10,xr6,xr13,xr9);
      D16MACF_AA_WW(xr10, xr0, xr0, xr9);
      S32LDD(xr2, blk, 0x10);
      S32LDD(xr4, blk, 0x30);
      Q16ADD_AS_WW(xr15,xr15,xr10,xr9);

      D16MUL_HW(xr10, xr5, xr1, xr1);
      D16MAC_SS_HW(xr10,xr5,xr11,xr1);
      D16MACF_AA_WW(xr10, xr0, xr0, xr1);
      D16MUL_LW(xr11, xr6, xr3, xr1);
      D16MAC_SS_LW(xr11,xr5,xr13,xr1);
      D16MACF_AA_WW(xr11, xr0, xr0, xr1);
      S32LDD(xr12, blk, 0x50);
      S32LDD(xr14, blk, 0x70);
      Q16ADD_AS_WW(xr10,xr10,xr11,xr1);

      D16MUL_HW(xr11, xr7, xr2, xr13);
      D16MAC_AA_LW(xr11,xr7,xr4,xr13);
      D16MAC_AA_LW(xr11,xr8,xr12,xr13);
      D16MAC_AA_HW(xr11,xr8,xr14,xr13);
      D16MACF_AA_WW(xr11, xr0, xr0, xr13);
     
       D16MUL_LW(xr3, xr7, xr2, xr13);
      D16MAC_SS_HW(xr3,xr8,xr4,xr13);
      D16MAC_SS_HW(xr3,xr7,xr12,xr13);
      D16MAC_SS_LW(xr3,xr8,xr14,xr13);
      D16MACF_AA_WW(xr3, xr0, xr0, xr13);

      D16MUL_LW(xr5, xr8, xr2, xr13);
      D16MAC_SS_HW(xr5,xr7,xr4,xr13);
      D16MAC_AA_HW(xr5,xr8,xr12,xr13);
      D16MAC_AA_LW(xr5,xr7,xr14,xr13);
      D16MACF_AA_WW(xr5, xr0, xr0, xr13);

      D16MUL_HW(xr2, xr8, xr2, xr13);
      D16MAC_SS_LW(xr2,xr8,xr4,xr13);
      D16MAC_AA_LW(xr2,xr7,xr12,xr13);
      D16MAC_SS_HW(xr2,xr7,xr14,xr13);
      D16MACF_AA_WW(xr2, xr0, xr0, xr13);

      Q16ADD_AS_WW(xr15,xr15,xr11,xr11);
      Q16ADD_AS_WW(xr10,xr10,xr3,xr3);
      Q16ADD_AS_WW(xr1,xr1,xr5,xr5);
      Q16ADD_AS_WW(xr9,xr9,xr2,xr2);

      S32STD(xr15, blk, 0x00);
      S32STD(xr10, blk, 0x10);
      S32STD(xr1, blk, 0x20);
      S32STD(xr9, blk, 0x30);
      S32STD(xr2, blk, 0x40);
      S32STD(xr5, blk, 0x50);
      S32STD(xr3, blk, 0x60);
      S32STD(xr11, blk, 0x70);
    }
  blk = block - 8;
  dest -= line_size;
  for (i=0; i< 8; i++) {
    S32LDIV(xr5, dest, line_size, 0x0);
    S32LDI(xr1, blk, 0x10);
    S32LDD(xr2, blk, 0x4);
    Q8ACCE_AA(xr2, xr5, xr0, xr1);

    S32LDD(xr6, dest, 0x4);
    S32LDD(xr3, blk, 0x8);
    S32LDD(xr4, blk, 0xc);
    Q8ACCE_AA(xr4, xr6, xr0, xr3);

    Q16SAT(xr5, xr2, xr1);
    S32STD(xr5, dest, 0x0);
    Q16SAT(xr6, xr4, xr3);
    S32STD(xr6, dest, 0x4);
  }
}

void ff_simple_idct_put_mxu(uint8_t *dest,int yuv_len, int line_size, DCTELEM *block)
{
  short *blk;
  unsigned int i;// mid0, mid1, tmp0, tmp1;
  uint8_t *dst_mid = dest;

  S32I2M(xr5,wxr5) ;         // xr5(w7, w3)
  S32I2M(xr6,wxr6) ;         // xr6(w9, w8)
  S32I2M(xr7,wxr7) ;         // xr7(w11,w10)
  S32I2M(xr8,wxr8) ;         // xr8(w13,w12)
  S32I2M(xr9,wxr9) ;         // xr9(w6, w0)  
  S32I2M(xr10,wxr10);       

  blk = block - 8;
  for (i = 0; i < yuv_len; i++)	/* idct rows */
    {
      int hi_b, lo_b, hi_c, lo_c;
       blk += 8;
      S32LDD(xr1, blk, 0x0);        //  xr1 (x4, x0)
      S32LDD(xr2, blk, 0x4);        //  xr2 (x7, x3)
      S32LDD(xr3, blk, 0x8);        //  xr3 (x6, x1)
      S32LDD(xr4, blk, 0xc);        //  xr4 (x5, x2)
      
      S32SFL(xr1,xr1,xr2,xr2, ptn3);  
      
      S32SFL(xr3,xr3,xr4,xr4, ptn3);  
      
      D16MUL_WW(xr11, xr2, xr5, xr12);         
      D16MAC_AA_WW(xr11,xr4,xr6,xr12);        
      D16MUL_WW(xr13, xr2, xr6, xr14);         
      D16MAC_SS_WW(xr13,xr4,xr5,xr14);        
      D16MUL_HW(xr2, xr1, xr7, xr4);         
      D16MAC_AS_LW(xr2,xr1,xr9,xr4);        
      D16MAC_AS_HW(xr2,xr3,xr10,xr4);        
      D16MAC_AS_LW(xr2,xr3,xr8,xr4);        

      D16MACF_AA_WW(xr2, xr0, xr0, xr4); 
      D16MACF_AA_WW(xr11, xr0, xr0, xr13);       
      D16MACF_AA_WW(xr12, xr0, xr0, xr14); 

      D16MUL_HW(xr4, xr1, xr8, xr15);         
      D16MAC_SS_LW(xr4,xr1,xr10,xr15);        
      D16MAC_AA_HW(xr4,xr3,xr9,xr15);        
      D16MAC_SA_LW(xr4,xr3,xr7,xr15);        
      Q16ADD_AS_WW(xr11,xr11,xr12,xr12);

      D16MACF_AA_WW(xr15, xr0, xr0, xr4); 
      Q16ADD_AS_WW(xr11, xr11, xr2, xr2);    
      Q16ADD_AS_XW(xr12, xr12, xr15, xr15);       


      S32SFL(xr11,xr11,xr12,xr12, ptn3);
      
      S32SFL(xr12,xr12,xr11,xr11, ptn3);
      
      S32STD(xr12, blk, 0x0);
      S32STD(xr11, blk, 0x4); 
      S32STD(xr15, blk, 0x8); 
      S32STD(xr2, blk, 0xc);       
    }

  blk = block - 2;

  if(yuv_len > 4)
    {
  for (i = 0; i < 4; i++)		/* idct columns */
    {
      int hi_b, lo_b, hi_c, lo_c;
       blk += 2;
      S32LDD(xr1, blk, 0x00);
      //S32LDI(xr1, blk, 0x04);
      S32LDD(xr3, blk, 0x20);
      S32I2M(xr5,wxr5);
      S32LDD(xr11, blk, 0x40);
      S32LDD(xr13, blk, 0x60);
       
      D16MUL_HW(xr15, xr5, xr1, xr9);
      D16MAC_AA_HW(xr15,xr5,xr11,xr9);        

      D16MACF_AA_WW(xr15, xr0, xr0, xr9); 

      D16MUL_LW(xr10, xr5, xr3, xr9);         
      D16MAC_AA_LW(xr10,xr6,xr13,xr9);        

      D16MACF_AA_WW(xr10, xr0, xr0, xr9); 

      S32LDD(xr2, blk, 0x10);
      S32LDD(xr4, blk, 0x30);
      Q16ADD_AS_WW(xr15,xr15,xr10,xr9);
      D16MUL_HW(xr10, xr5, xr1, xr1);         
      D16MAC_SS_HW(xr10,xr5,xr11,xr1);        

      D16MACF_AA_WW(xr10, xr0, xr0, xr1); 

      D16MUL_LW(xr11, xr6, xr3, xr1);         
      D16MAC_SS_LW(xr11,xr5,xr13,xr1);        

      D16MACF_AA_WW(xr11, xr0, xr0, xr1); 

      S32LDD(xr12, blk, 0x50);
      S32LDD(xr14, blk, 0x70);
      Q16ADD_AS_WW(xr10,xr10,xr11,xr1);
      D16MUL_HW(xr11, xr7, xr2, xr13);         
      D16MAC_AA_LW(xr11,xr7,xr4,xr13);        
      D16MAC_AA_LW(xr11,xr8,xr12,xr13);        
      D16MAC_AA_HW(xr11,xr8,xr14,xr13);        

      D16MACF_AA_WW(xr11, xr0, xr0, xr13); 

      D16MUL_LW(xr3, xr7, xr2, xr13);         
      D16MAC_SS_HW(xr3,xr8,xr4,xr13);        
      D16MAC_SS_HW(xr3,xr7,xr12,xr13);        
      D16MAC_SS_LW(xr3,xr8,xr14,xr13);        
      
      D16MACF_AA_WW(xr3, xr0, xr0, xr13); 

      D16MUL_LW(xr5, xr8, xr2, xr13);         
      D16MAC_SS_HW(xr5,xr7,xr4,xr13);        
      D16MAC_AA_HW(xr5,xr8,xr12,xr13);        
      D16MAC_AA_LW(xr5,xr7,xr14,xr13);        
      
      D16MACF_AA_WW(xr5, xr0, xr0, xr13); 

      D16MUL_HW(xr2, xr8, xr2, xr13);         
      D16MAC_SS_LW(xr2,xr8,xr4,xr13);        
      D16MAC_AA_LW(xr2,xr7,xr12,xr13);        
      D16MAC_SS_HW(xr2,xr7,xr14,xr13);        
      
      D16MACF_AA_WW(xr2, xr0, xr0, xr13); 

      Q16ADD_AS_WW(xr15,xr15,xr11,xr11);    
      Q16ADD_AS_WW(xr10,xr10,xr3,xr3);    
      Q16ADD_AS_WW(xr1,xr1,xr5,xr5);          
      Q16ADD_AS_WW(xr9,xr9,xr2,xr2);      
      // saturate
      Q16SAT(xr15, xr15, xr10);
      Q16SAT(xr1, xr1,  xr9);
      Q16SAT(xr2, xr2,  xr5);
      Q16SAT(xr3, xr3,  xr11);
  
      // store it
      //RCON BUFFER 
      dst_mid = dest + i * 2;
      S16STD(xr15, dst_mid, 0, 1);
      S16SDI(xr15, dst_mid, 16, 0);
      S16SDI(xr1, dst_mid, 16,  1);
      S16SDI(xr1, dst_mid, 16,  0);
      S16SDI(xr2, dst_mid, 16,  1);
      S16SDI(xr2, dst_mid, 16,  0);
      S16SDI(xr3, dst_mid, 16,  1);
      S16STD(xr3, dst_mid, 16,  0);
    }
    }
  else
    {
       	    for (i = 0; i < 4; i++)		/* idct columns */
	    {
	      int hi_b, lo_b, hi_c, lo_c;
	       blk += 2;
	      S32LDD(xr1, blk, 0x00);
	      //S32LDI(xr1, blk, 0x04);
	      S32LDD(xr3, blk, 0x20);
	      S32I2M(xr5,wxr5);
	      // S32LDD(xr11, blk, 0x40);
	      // S32LDD(xr13, blk, 0x60);

	      D16MUL_HW(xr15, xr5, xr1, xr9);
	      // D16MAC_AA_HW(xr15,xr5,xr11,xr9);        

	      D16MACF_AA_WW(xr15, xr0, xr0, xr9); 

	      D16MUL_LW(xr10, xr5, xr3, xr9);         
	      //  D16MAC_AA_LW(xr10,xr6,xr13,xr9);        

	      D16MACF_AA_WW(xr10, xr0, xr0, xr9); 

	      S32LDD(xr2, blk, 0x10);
	      S32LDD(xr4, blk, 0x30);
	      Q16ADD_AS_WW(xr15,xr15,xr10,xr9);
	      D16MUL_HW(xr10, xr5, xr1, xr1);         
	      // D16MAC_SS_HW(xr10,xr5,xr11,xr1);        

	      D16MACF_AA_WW(xr10, xr0, xr0, xr1); 

	      D16MUL_LW(xr11, xr6, xr3, xr1);         
	      // D16MAC_SS_LW(xr11,xr5,xr13,xr1);        

	      D16MACF_AA_WW(xr11, xr0, xr0, xr1); 

	      // S32LDD(xr12, blk, 0x50);
	      // S32LDD(xr14, blk, 0x70);
	      Q16ADD_AS_WW(xr10,xr10,xr11,xr1);
	      D16MUL_HW(xr11, xr7, xr2, xr13);         
	      D16MAC_AA_LW(xr11,xr7,xr4,xr13);        
	      //D16MAC_AA_LW(xr11,xr8,xr12,xr13);        
	      //D16MAC_AA_HW(xr11,xr8,xr14,xr13);        

	      D16MACF_AA_WW(xr11, xr0, xr0, xr13); 

	      D16MUL_LW(xr3, xr7, xr2, xr13);         
	      D16MAC_SS_HW(xr3,xr8,xr4,xr13);        
	      // D16MAC_SS_HW(xr3,xr7,xr12,xr13);        
	      //D16MAC_SS_LW(xr3,xr8,xr14,xr13);        

	      D16MACF_AA_WW(xr3, xr0, xr0, xr13); 

	      D16MUL_LW(xr5, xr8, xr2, xr13);         
	      D16MAC_SS_HW(xr5,xr7,xr4,xr13);        
	      //D16MAC_AA_HW(xr5,xr8,xr12,xr13);        
	      //D16MAC_AA_LW(xr5,xr7,xr14,xr13);        

	      D16MACF_AA_WW(xr5, xr0, xr0, xr13); 

	      D16MUL_HW(xr2, xr8, xr2, xr13);         
	      D16MAC_SS_LW(xr2,xr8,xr4,xr13);        
	      // D16MAC_AA_LW(xr2,xr7,xr12,xr13);        
	      //D16MAC_SS_HW(xr2,xr7,xr14,xr13);        

	      D16MACF_AA_WW(xr2, xr0, xr0, xr13); 

	      Q16ADD_AS_WW(xr15,xr15,xr11,xr11);    
	      Q16ADD_AS_WW(xr10,xr10,xr3,xr3);    
	      Q16ADD_AS_WW(xr1,xr1,xr5,xr5);          
	      Q16ADD_AS_WW(xr9,xr9,xr2,xr2);      
	      // saturate
	      Q16SAT(xr15, xr15, xr10);
	      Q16SAT(xr1, xr1,  xr9);
	      Q16SAT(xr2, xr2,  xr5);
	      Q16SAT(xr3, xr3,  xr11);

	      // store it
	      dst_mid = dest + i * 2;
	      S16STD(xr15, dst_mid, 0, 1);
	      S16SDI(xr15, dst_mid, 16, 0);
	      S16SDI(xr1, dst_mid, 16,  1);
	      S16SDI(xr1, dst_mid, 16,  0);
	      S16SDI(xr2, dst_mid, 16,  1);
	      S16SDI(xr2, dst_mid, 16,  0);
	      S16SDI(xr3, dst_mid, 16,  1);
	      S16STD(xr3, dst_mid, 16,  0);
	    } 
    }
}

static inline void add_dequant_dct_opt(DCTELEM *block, int i, uint8_t *dest, int line_size, int qscale)
{
    //int yuv_len;
    //yuv_len = (dMB->val[i]>>3) + 1;    
    int yuv_len = 8; 
    if (dMB_L->block_last_index[i] >= 0) {
      if (dFRM->mpeg_quant || dFRM->codec_id == 2){
        dct_unquantize_mpeg2_inter_c_opt(block, i, qscale);
      }
      else if (dFRM->out_format == 2 || dFRM->out_format == 1){
        dct_unquantize_h263_inter_c_opt(block, i, qscale);
      }
      ff_simple_idct_add_mxu(dest,yuv_len, line_size, block);
      //simple_idct_add_mxu(dest,yuv_len, line_size, block);
    }
}

static void dct_unquantize_mpeg2_inter_c_opt(DCTELEM *block, int n, int qscale)
{
#if 0
  int i, level, nCoeffs;
  const uint16_t *quant_matrix;
  int sum=-1;

  if(dFRM->alternate_scan) nCoeffs= 63;
  else nCoeffs= dMB_L->block_last_index[n];

  quant_matrix = dFRM->inter_matrix;
  for(i=0; i<=nCoeffs; i++) {
    int j= dFRM->permutated[i];
    level = block[j];
    if (level) {
      if (level < 0) {
	level = -level;
	level = (((level << 1) + 1) * qscale *
		 ((int) (quant_matrix[j]))) >> 4;
	level = -level;
      } else {
	level = (((level << 1) + 1) * qscale *
		 ((int) (quant_matrix[j]))) >> 4;
      }
      block[j] = level;
      sum+=level;
    }
  }
  block[63]^=sum&1;
#else
   int i, level, nCoeffs,sum;
   const uint16_t *quant_matrix;
   //nCoeffs= dMB_L->val[n];
   nCoeffs= 63;
   quant_matrix = dFRM->inter_matrix;
   S32I2M(xr15,-1);
   S32I2M(xr5,1);
   for(i=0; i<=nCoeffs; i++) {
   level = block[i];  
   if (level) {
       S32I2M(xr1,level);                  
       S32CPS(xr2,xr1,xr1);
       S32MUL(xr0,xr3,qscale,quant_matrix[i]);         
       D32SLL(xr4,xr2,xr0,xr0,1);
       S32OR(xr6,xr4,xr5);
       D16MUL_WW(xr0,xr6,xr3,xr7);
       D32SLR(xr9,xr7,xr0,xr0,4);
       S32CPS(xr8,xr9,xr1); 
       block[i] = S32M2I(xr8);
       D32ASUM_AA(xr15,xr8,xr0,xr0);
    }
   }
   S32AND(xr13,xr15,xr5);
   S32I2M(xr14,block[63]);
   S32XOR(xr14,xr14,xr13);
   block[63] = S32M2I(xr14);
#endif
}

static void dct_unquantize_h263_inter_c_opt(DCTELEM *block, int n, int qscale)
{
  int i, level, qmul, qadd;
  int nCoeffs;

  //assert(dMB->block_last_index[n]>=0);
  qadd = (qscale - 1) | 1;
  qmul = qscale << 1;

  nCoeffs= dFRM->raster_end[ dMB_L->block_last_index[n] ];
  //nCoeffs= s->inter_scantable.raster_end[ s->block_last_index[n] ];

  for(i=0; i<=nCoeffs; i++) {
    level = block[i];
    if (level) {
      if (level < 0) {
	level = level * qmul - qadd;
      } else {
	level = level * qmul + qadd;
      }
      block[i] = level;
    }
  }
}

static inline void add_dct_opt(DCTELEM *block, int i, uint8_t *dest, int line_size)
{
  int yuv_len;
  yuv_len = 8;
  if (dMB_L->block_last_index[i] >= 0) {
    ff_simple_idct_add_mxu(dest,yuv_len,line_size, block);
    //simple_idct_add_mxu(dest,yuv_len,line_size, block);
  }
}

static inline void put_dct_opt(DCTELEM *block, int i, uint8_t *dest, int qscale)
{
  int yuv_len;

  yuv_len = 8;
  if (dFRM->mpeg_quant || dFRM->codec_id == 2){
    dct_unquantize_mpeg2_intra_c_opt(block, i, qscale);
  }
  else if (dFRM->out_format == 2 || dFRM->out_format == 1){
    dct_unquantize_h263_intra_c_opt(block, i, qscale);
  }
  ff_simple_idct_put_mxu(dest,yuv_len,16,block);
  //simple_idct_put_mxu(dest,yuv_len,16,block);
}

static void dct_unquantize_mpeg2_intra_c_opt(DCTELEM *block, int n, int qscale)
{
#if 1
  int i, level, nCoeffs;
  const uint16_t *quant_matrix;
  //volatile int *tmp_dbg = TCSM1_DBG_BUF;
#if 0
  if(dFRM->alternate_scan) nCoeffs= 63;
  else nCoeffs= dMB_L->block_last_index[n];
#else
  nCoeffs= 63;
#endif
#if 1
  int dc_scale = (n<4) ? dMB_L->y_dc_scale:dMB_L->c_dc_scale; 
  block[0] = block[0] * dc_scale;
#else
  if (n < 4)
    block[0] = block[0] * dMB_L->y_dc_scale;
  else
    block[0] = block[0] * dMB_L->c_dc_scale;
#endif
  quant_matrix = dFRM->intra_matrix;
    
  for(i=1;i<=nCoeffs;i++) {
    //int j= dFRM->permutated[i];
    level = block[i];
    if (level) {
      if (level < 0) {
	level = -level;
	level = (int)(level * qscale * quant_matrix[i]) >> 3;
	level = -level;
      } else {
	level = (int)(level * qscale * quant_matrix[i]) >> 3;
      }
      block[i] = level;
    }
  }
#else
  int i, level, nCoeffs;
  const uint16_t *quant_matrix;
  nCoeffs=63;  
  int dc_scale = (n<4) ? dMB_L->y_dc_scale:dMB_L->c_dc_scale; 
  quant_matrix = dFRM->intra_matrix;
  S32I2M(xr5,qscale);
  S32LUI(xr9,1,0);
  S32MUL(xr0,xr6,block[0],dc_scale);
  //D16MUL_WW(xr0,xr6,xr9,xr6);
  block-=2;
  quant_matrix-=2;
  for(i=0;i<nCoeffs;i++) {
    S32LDI(xr1,block,4);
    S32LDI(xr2,quant_matrix,4);
    D16MUL_LW(xr13,xr9,xr1,xr14);
    D16CPS(xr1,xr1,xr1);
    D16MUL_LW(xr7,xr5,xr2,xr8);
    S32SFL(xr0,xr7,xr8,xr2,3);
    D16MUL_WW(xr7,xr1,xr2,xr8);
    D32SLR(xr7,xr7,xr8,xr8,3);
    S32CPS(xr10,xr7,xr13);
    S32CPS(xr11,xr8,xr14);
    S32SFL(xr0,xr10,xr11,xr12,3);
    S32STD(xr12,block,0);
  }	    
  S16STD(xr6,block-(nCoeffs*2-2),0,0);//xr6 to data[0]
#endif
}

static void dct_unquantize_h263_intra_c_opt(DCTELEM *block, int n, int qscale)
{
#if 1
  int i, level, qmul, qadd;
  int nCoeffs;

  qmul = qscale << 1;

  if (!dFRM->h263_aic) {
    if (n < 4)
      block[0] = block[0] * dMB_L->y_dc_scale;
    else
      block[0] = block[0] * dMB_L->c_dc_scale;
    qadd = (qscale - 1) | 1;
  }else{
    qadd = 0;
  }
  if(dMB_L->ac_pred)
    nCoeffs=63;
  else
    nCoeffs= dFRM->raster_end[ dMB_L->block_last_index[n] ];
  for(i=1; i<=nCoeffs; i++) {
    level = block[i];
    if (level) {
      if (level < 0) {
	level = level * qmul - qadd;
      } else {
	level = level * qmul + qadd;
      }
      block[i] = level;
    }
  }

#else
  int i, level, qmul, qadd;
  int nCoeffs;
  
  S32LUI(xr9,1,0);
    S32I2M(xr1,qscale);
    D32SLL(xr5,xr1,xr0,xr0,1);
    if (!dFRM->h263_aic) { 
      int dc_scale = (n<4) ? dMB_L->y_dc_scale:dMB_L->c_dc_scale;
      S32MUL(xr0,xr6,block[0],dc_scale);
      S32AND(xr15,xr1,xr9);
      //block[0] = S32M2I(xr6);  
      S32MOVN(xr2,xr15,xr1);
      D32ADD_SS(xr1,xr1,xr9,xr3);
      S32MOVZ(xr2,xr15,xr1);
    }else{
      S32I2M(xr2,0);
    }
    nCoeffs=63;
    S32SFL(xr0,xr2,xr2,xr2,3);  
    block-=2;
    for(i=0; i<nCoeffs; i++) {
      S32LDI(xr8,block,4);
      if (S32M2I(xr8)==0)
        continue;
      D16CPS(xr13,xr2,xr8); 
      D16MOVZ(xr13,xr8,xr0); 
      D16MADL_AA_LW(xr13,xr5,xr8,xr12);      
      S32STD(xr12,block,0x0); 
    }
    S16STD(xr6,block-(nCoeffs*2-2),0,0);//xr6 to data[0]
#endif

}
