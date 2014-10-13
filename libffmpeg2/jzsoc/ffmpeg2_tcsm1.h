/*
 * Copyright (C) 2006-2014 Ingenic Semiconductor CO.,LTD.
 *
 * This file is part of MPlayer.
 *
 * MPlayer is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * MPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with MPlayer; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*****************************************************************************
 *
 * JZ4760 TCSM1 Space Seperate
 *
 * $Id: ffmpeg2_tcsm1.h,v 1.1.1.1 2012/03/27 04:02:56 dqliu Exp $
 *
 ****************************************************************************/

#ifndef __FFMPEG2_TCSM1_H__
#define __FFMPEG2_TCSM1_H__

#define TCSM1_BANK0 0xF4000000
#define TCSM1_BANK1 0xF4001000
#define TCSM1_BANK2 0xF4002000
#define TCSM1_BANK3 0xF4003000 //16KB
#define TCSM1_BANK4 0xF4004000
#define TCSM1_BANK5 0xF4005000
#define TCSM1_BANK6 0xF4006000
#define TCSM1_BANK7 0xF4007000

extern volatile unsigned char * tcsm1_base;
#define TCSM1_PADDR(a)         ((((unsigned)(a)) & 0xFFFF) | 0x132C0000)
#define TCSM1_VCADDR(a)        (tcsm1_base + ((a) & 0xFFFF))

#define MPEG2_P1_MAIN           (TCSM1_BANK0)

#define TCSM1_FIFO_RP           (TCSM1_BANK4) 

#define UNKNOWN_BLOCK           (TCSM1_FIFO_RP + 4)
#define TCSM1_MBNUM_WP          (UNKNOWN_BLOCK + 4)
#define TCSM1_MBNUM_RP          (TCSM1_MBNUM_WP + 4)

/* ------------------  define for DMA  -------------------- */
#define TSA       0
#define TDA       4
#define STRD      8
#define UNIT      12

#define DDMA_GP0_SET_LEN        (4 * 4)
#define DDMA_GP0_SET            (TCSM1_MBNUM_RP + 4)   // using for tcsm0 to tcsm1

#define DDMA_GP1_SET_LEN        (4 * 4 * 3)
#define DDMA_GP1_SET            (DDMA_GP0_SET + DDMA_GP0_SET_LEN)

/* -------------------  define for FIFO  ---------------------*/
#define DFRM_BUF_LEN            (((sizeof(struct MPEG2_FRAME_GloARGs)+31)/32)*32)
#define TCSM1_DFRM_BUF          (DDMA_GP1_SET + DDMA_GP1_SET_LEN)//must be cache align

#define TASK_BUF_LEN            ((sizeof(struct MPEG2_MB_DecARGs) + 3) & 0xFFFFFFFC)
#define TASK_DMB_BUF            (TCSM1_DFRM_BUF + DFRM_BUF_LEN)
#define TASK_DMB_BUF1           (TASK_DMB_BUF + TASK_BUF_LEN)
#define TASK_DMB_BUF2           (TASK_DMB_BUF1 + TASK_BUF_LEN)

//vamu chain
#define STEP                    (4)
#define VMAU_CHN_ONELEN         (9<<2)         
#define VMAU_CHN_DEP            (1) 
#define VMAU_CHN_LEN            (VMAU_CHN_ONELEN   * VMAU_CHN_DEP)

#define VMAU_CHAIN              (TCSM1_BANK6)
#define VMAU_CHAIN1             (VMAU_CHAIN + VMAU_CHN_LEN + STEP)
#define VMAU_DEC_END_FLAG       (VMAU_CHAIN1 + VMAU_CHN_LEN + STEP)
#define IDCT_YOUT               (VMAU_DEC_END_FLAG + 64*6*2 + STEP)
#define IDCT_COUT               (IDCT_YOUT  + 64*4)

#define IDCT_YOUT1              (IDCT_COUT  + 64*2)
#define IDCT_COUT1              (IDCT_YOUT1 + 64*4)

#define IDCT_YOUT2              (IDCT_COUT1 + 64*2)
#define IDCT_COUT2              (IDCT_YOUT2 + 64*4)

#define VMAU_END                (IDCT_COUT2 + 64*2  + STEP)

//motion chain                  
#define MOTION_DHA_LEN          (0x40) // at most 0x40, normaly, it is 0x30
#define MOTION_DHA_DEP          (4)
#define MOTION_ONELEN           (MOTION_DHA_LEN * MOTION_DHA_DEP)

#define MOTION_DHA              (VMAU_END + STEP )           
#define MOTION_DHA1             (MOTION_DHA  + MOTION_ONELEN + STEP)
#define MOTION_DSA              (MOTION_DHA1 + MOTION_ONELEN + STEP)
#define MOTION_YOUT             (MOTION_DSA  + STEP) 
#define MOTION_COUT             (MOTION_YOUT + 64*4 + STEP)
#define MOTION_END              (MOTION_COUT + 64*2 + STEP)        

#endif//ffmpeg2_tcsm1
