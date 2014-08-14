/*
 * RV30 decoder motion compensation functions
 * Copyright (c) 2007 Konstantin Shishkov
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
 * RV30 decoder motion compensation functions
 */

#include "avcodec.h"
#include "dsputil.h"
#define JZC_MXU_OPT
#include "rv30_tcsm0.h"
#include "../libjzcommon/jz4760e_2ddma_hw.h"
#ifdef JZC_MXU_OPT
#include "../libjzcommon/jzmedia.h"
#endif

#define op_avg(a, b)  a = (((a)+cm[b]+1)>>1)
#define op_put(a, b)  a = cm[b]

#define JZC_PMON_P0
#define STA_CCLK
#ifdef JZC_PMON_P0
#include "../libjzcommon/jz4760e_pmon.h"
extern uint32_t rv8vlc_pmon_val;
extern uint32_t rv8vlc_pmon_val_ex;
#endif

//#define MC_USE_TCSM

#undef printf
static av_unused void put_rv30_tpel8_h_lowpass(uint8_t *dst, uint8_t *src, int dstStride, int srcStride, const int C1, const int C2){
#ifdef JZC_PMON_P0
  //PMON_ON(rv8vlc);
#endif
  const int h=8;
  //uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;
  int i,j;
  S8LDD(xr13,&C1,0,7);
  S8LDD(xr14,&C2,0,7);
  //S8LDD(xr14,&C2,0,3);
  S32I2M(xr15,0x00080008);
  for(i=0; i<h; i++){
#if 1
    S8LDD(xr1,src,-1,3);S8LDD(xr3,src,1,3);
    S8LDD(xr1,src,0,2);S8LDD(xr3,src,2,2);
    S8LDD(xr1,src,1,1);S8LDD(xr3,src,3,1);
    S8LDD(xr1,src,2,0);S8LDD(xr3,src,4,0);
    D32SLL(xr2,xr1,xr1,xr2,8);
    D32SLL(xr4,xr3,xr3,xr4,8);
    S8LDD(xr2,src,3,0);
    S8LDD(xr4,src,5,0);
    Q8MUL(xr5,xr2,xr13,xr6);
    Q8MUL(xr7,xr3,xr14,xr8);

    Q8ADDE_AA(xr9,xr1,xr4,xr10);
    Q16ACCM_AA(xr5,xr15,xr15,xr6);
    Q16ACCM_SS(xr7,xr9,xr10,xr8);

    S8LDD(xr1,src,3,3);S8LDD(xr3,src,5,3);
    Q16ACCM_AA(xr5,xr7,xr8,xr6);
    S8LDD(xr1,src,4,2);S8LDD(xr3,src,6,2);
    Q16SAR(xr5,xr5,xr6,xr6,4);
    S8LDD(xr1,src,5,1);S8LDD(xr3,src,7,1);
    Q16SAT(xr9,xr5,xr6);
    S8LDD(xr1,src,6,0);S8LDD(xr3,src,8,0);
    S32STDR(xr9,dst,0);

    D32SLL(xr2,xr1,xr1,xr2,8);
    D32SLL(xr4,xr3,xr3,xr4,8);
    S8LDD(xr2,src,7,0);
    S8LDD(xr4,src,9,0);

    Q8MUL(xr5,xr2,xr13,xr6);
    Q8MUL(xr7,xr3,xr14,xr8);

    Q8ADDE_AA(xr9,xr1,xr4,xr10);
    Q16ACCM_AA(xr5,xr15,xr15,xr6);
    Q16ACCM_SS(xr7,xr9,xr10,xr8);
    Q16ACCM_AA(xr5,xr7,xr8,xr6);
    Q16SAR(xr5,xr5,xr6,xr6,4);
    Q16SAT(xr9,xr5,xr6);
    S32STDR(xr9,dst,4);
    dst[6] = (-(src[ 5]+src[8]) + src[6]*C1 + src[7]*C2 + 8)>>4;
    j = (-(src[ 5]+src[8]) + src[6] + src[7] + 8);
#else
    op_put(dst[0], (-(src[-1]+src[2]) + src[0]*C1 + src[1]*C2 + 8)>>4);
    op_put(dst[1], (-(src[ 0]+src[3]) + src[1]*C1 + src[2]*C2 + 8)>>4);
    op_put(dst[2], (-(src[ 1]+src[4]) + src[2]*C1 + src[3]*C2 + 8)>>4);
    op_put(dst[3], (-(src[ 2]+src[5]) + src[3]*C1 + src[4]*C2 + 8)>>4);
    op_put(dst[4], (-(src[ 3]+src[6]) + src[4]*C1 + src[5]*C2 + 8)>>4);
    op_put(dst[5], (-(src[ 4]+src[7]) + src[5]*C1 + src[6]*C2 + 8)>>4);
    op_put(dst[6], (-(src[ 5]+src[8]) + src[6]*C1 + src[7]*C2 + 8)>>4);
    op_put(dst[7], (-(src[ 6]+src[9]) + src[7]*C1 + src[8]*C2 + 8)>>4);
#endif
#ifdef MC_USE_TCSM
      dst+= 16;
#else
      dst+= dstStride;
#endif
    src+=srcStride;
  }
#ifdef JZC_PMON_P0
  //PMON_OFF(rv8vlc);
#endif
}

static void put_rv30_tpel8_v_lowpass(uint8_t *dst, uint8_t *src, int dstStride, int srcStride, const int C1, const int C2){
#ifdef JZC_PMON_P0
  //PMON_ON(rv8vlc);
#endif
  const int w=8;
    //uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;
    int i;
    S8LDD(xr13,&C1,0,7);
    S8LDD(xr14,&C2,0,7);
    S32I2M(xr15,0x00080008);
    //uint8_t *srcl=RECON_YBUF;
    //uint8_t *dstl;
    //int a,b,c,d;

#ifdef MC_USE_TCSM
    dstStride = 16;
#endif

    for(i=0; i<w; i++){
#if 1
      S8LDD(xr1,&src[-1*srcStride],0,3);S8LDD(xr3,&src[1*srcStride],0,3);
      S8LDD(xr1,&src[0*srcStride],0,2); S8LDD(xr3,&src[2*srcStride],0,2);
      S8LDD(xr1,&src[1*srcStride],0,1); S8LDD(xr3,&src[3*srcStride],0,1);
      S8LDD(xr1,&src[2*srcStride],0,0); S8LDD(xr3,&src[4*srcStride],0,0);
      D32SLL(xr2,xr1,xr3,xr4,8);
      S8LDD(xr2,&src[3*srcStride],0,0);S8LDD(xr4,&src[5*srcStride],0,0);

      Q8MUL(xr5,xr2,xr13,xr6);
      Q8MUL(xr7,xr3,xr14,xr8);

      Q8ADDE_AA(xr9,xr1,xr4,xr10);
      Q16ACCM_AA(xr5,xr15,xr15,xr6);
      Q16ACCM_SS(xr7,xr9,xr10,xr8);
    
      S8LDD(xr1,&src[3*srcStride],0,3);S8LDD(xr3,&src[5*srcStride],0,3);
      Q16ACCM_AA(xr5,xr7,xr8,xr6);
      S8LDD(xr1,&src[4*srcStride],0,2); S8LDD(xr3,&src[6*srcStride],0,2);
      Q16SAR(xr5,xr5,xr6,xr6,4);
      S8LDD(xr1,&src[5*srcStride],0,1); S8LDD(xr3,&src[7*srcStride],0,1);
      Q16SAT(xr9,xr5,xr6);
      S8LDD(xr1,&src[6*srcStride],0,0); S8LDD(xr3,&src[8*srcStride],0,0);

      S8STD(xr9,&dst[0],0,3);
      D32SLL(xr2,xr1,xr3,xr4,8);//
      S8STD(xr9,&dst[dstStride],0,2);
      S8LDD(xr2,&src[7*srcStride],0,0);//
      S8STD(xr9,&dst[2*dstStride],0,1);
      S8LDD(xr4,&src[9*srcStride],0,0);//
      S8STD(xr9,&dst[3*dstStride],0,0);

      Q8MUL(xr5,xr2,xr13,xr6);
      Q8MUL(xr7,xr3,xr14,xr8);

      Q8ADDE_AA(xr9,xr1,xr4,xr10);
      Q16ACCM_AA(xr5,xr15,xr15,xr6);
      Q16ACCM_SS(xr7,xr9,xr10,xr8);
      Q16ACCM_AA(xr5,xr7,xr8,xr6);
      Q16SAR(xr5,xr5,xr6,xr6,4);
      Q16SAT(xr9,xr5,xr6);
      S8STD(xr9,&dst[4*dstStride],0,3);
      S8STD(xr9,&dst[5*dstStride],0,2);
      S8STD(xr9,&dst[6*dstStride],0,1);
      S8STD(xr9,&dst[7*dstStride],0,0);
#endif
#if 0
      op_put(dst[0*dstStride], (-(srcl[0]+srcl[3]) + srcl[1]*C1 + srcl[2]*C2 + 8)>>4);
      op_put(dst[1*dstStride], (-(srcl[1]+srcl[4]) + srcl[2]*C1 + srcl[3]*C2 + 8)>>4);
      op_put(dst[2*dstStride], (-(srcl[2]+srcl[5]) + srcl[3]*C1 + srcl[4]*C2 + 8)>>4);
      op_put(dst[3*dstStride], (-(srcl[3]+srcl[6]) + srcl[4]*C1 + srcl[5]*C2 + 8)>>4);
      op_put(dst[4*dstStride], (-(srcl[4]+srcl[7]) + srcl[5]*C1 + srcl[6]*C2 + 8)>>4);
      op_put(dst[5*dstStride], (-(srcl[5]+srcl[8]) + srcl[6]*C1 + srcl[7]*C2 + 8)>>4);
      op_put(dst[6*dstStride], (-(srcl[6]+srcl[9]) + srcl[7]*C1 + srcl[8]*C2 + 8)>>4);
      op_put(dst[7*dstStride], (-(srcl[7]+srcl[10]) + srcl[8]*C1 + srcl[9]*C2 + 8)>>4);
#endif
      dst++;
      src++;
    }
#ifdef JZC_PMON_P0
    //PMON_OFF(rv8vlc);
#endif
}
const uint8_t mc_coef[12]={144,72,72,36,12,0,6,0,128,0,0,0};
static void put_rv30_tpel8_hv_lowpass(uint8_t *dst, uint8_t *src, int dstStride, int srcStride){
#ifdef JZC_PMON_P0
  //PMON_ON(rv8vlc);
#endif
  const int w = 8;
  const int h = 8;
  uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;
  int i, j;

#ifdef MC_USE_TCSM
    dstStride = 16;
#endif

  S32LDD(xr13,mc_coef,0x0);
  S32LDD(xr14,mc_coef,0x4);
  S32LDD(xr15,mc_coef,0x8);
#if 0
  op_put(dst[i], (
                  src[srcStride*-1+i-1]  -12*src[srcStride*-1+i]  -6*src[srcStride*-1+i+1]    +src[srcStride*-1+i+2]+
		  -12*src[srcStride* 0+i-1] +144*src[srcStride* 0+i] +72*src[srcStride* 0+i+1] -12*src[srcStride* 0+i+2] +
		  -6*src[srcStride* 1+i-1]  +72*src[srcStride* 1+i] +36*src[srcStride* 1+i+1]  -6*src[srcStride* 1+i+2] +
                  src[srcStride* 2+i-1]  -12*src[srcStride* 2+i]  -6*src[srcStride* 2+i+1]    +src[srcStride* 2+i+2] +
                  128)>>8);
#endif
  for(j = 0; j < h; j++){
    for(i = 0; i < w; i+=2){
#if 1
      S8LDD(xr1,&src[srcStride*-1+i],-1,0);S8LDD(xr2,&src[srcStride*-1+i],0,0);
      S8LDD(xr1,&src[srcStride*-1+i],2,1);S8LDD(xr2,&src[srcStride*0+i],-1,1);//*12//*1
      S8LDD(xr1,&src[srcStride*2+i],-1,2);S8LDD(xr2,&src[srcStride*0+i],2,2);
      S8LDD(xr1,&src[srcStride*2+i],2,3);S8LDD(xr2,&src[srcStride*2+i],0,3);

      S8LDD(xr3,&src[srcStride*-1+i],1,0);S8LDD(xr4,&src[srcStride*0+i],0,0);
      S8LDD(xr3,&src[srcStride*1+i],-1,1);S8LDD(xr4,&src[srcStride*0+i],1,1);//*6//*114 *72
      S8LDD(xr3,&src[srcStride*1+i],2,2);S8LDD(xr4,&src[srcStride*1+i],0,2);
      S8LDD(xr3,&src[srcStride*2+i],1,3);S8LDD(xr4,&src[srcStride*1+i],1,3);//*72 *36

      D8SUM(xr2,xr3,xr2);
      D8SUM(xr1,xr0,xr1);
      D16MUL_WW(xr3,xr2,xr14,xr2);
      
      Q8MUL(xr5,xr4,xr13,xr4);
      S8LDD(xr6,&src[srcStride*-1+i],0,0);
      Q16ACCM_SS(xr5,xr3,xr2,xr4);

      S8LDD(xr2,&src[srcStride*-1+i],1,0);
      S8LDD(xr6,&src[srcStride*-1+i],3,1);
      S8LDD(xr2,&src[srcStride*0+i],0,1);
      S8LDD(xr6,&src[srcStride*2+i],0,2);
      S8LDD(xr2,&src[srcStride*0+i],3,2);
      S8LDD(xr6,&src[srcStride*2+i],3,3);
      S8LDD(xr2,&src[srcStride*2+i],1,3);

      S8LDD(xr3,&src[srcStride*-1+i],2,0);
      S8LDD(xr7,&src[srcStride*0+i],1,0);
      S8LDD(xr3,&src[srcStride*1+i],0,1);
      S8LDD(xr7,&src[srcStride*0+i],2,1);
      S8LDD(xr3,&src[srcStride*1+i],3,2);
      S8LDD(xr7,&src[srcStride*1+i],1,2);
      S8LDD(xr3,&src[srcStride*2+i],2,3);
      S8LDD(xr7,&src[srcStride*1+i],2,3);

      D8SUM(xr2,xr3,xr2);
      D8SUM(xr6,xr0,xr6);
      D16MUL_WW(xr3,xr2,xr14,xr2);
      
      Q8MUL(xr8,xr7,xr13,xr7);
      Q16ACCM_SS(xr8,xr3,xr2,xr7);

      D32SLL(xr3,xr0,xr0,xr2,0);
      D16ASUM_AA(xr1,xr4,xr7,xr6);
      D16ASUM_AA(xr2,xr5,xr8,xr3);
      D32ASUM_AA(xr2,xr1,xr6,xr3);

      D32ASUM_AA(xr2,xr15,xr15,xr3);
      D32SAR(xr2,xr2,xr3,xr3,8);
      Q16SAT(xr2,xr3,xr2);
      S8STD(xr2,&dst[i],0,0);
      S8STD(xr2,&dst[i],1,2);
#else
      op_put(dst[i], (
		      src[srcStride*-1+i-1]  -12*src[srcStride*-1+i]  -6*src[srcStride*-1+i+1]    +src[srcStride*-1+i+2]+
		      -12*src[srcStride* 0+i-1] +144*src[srcStride* 0+i] +72*src[srcStride* 0+i+1] -12*src[srcStride* 0+i+2] +
		      -6*src[srcStride* 1+i-1]  +72*src[srcStride* 1+i] +36*src[srcStride* 1+i+1]  -6*src[srcStride* 1+i+2] +
		      src[srcStride* 2+i-1]  -12*src[srcStride* 2+i]  -6*src[srcStride* 2+i+1]    +src[srcStride* 2+i+2] +
		      128)>>8);
#endif
    }
    src += srcStride;
    dst += dstStride;
  }
#ifdef JZC_PMON_P0
  //PMON_OFF(rv8vlc);
#endif
}

static void put_rv30_tpel8_hhv_lowpass(uint8_t *dst, uint8_t *src, int dstStride, int srcStride){
#ifdef JZC_PMON_P0
  //PMON_ON(rv8vlc);
#endif

#ifdef MC_USE_TCSM
  dstStride = 16;
#endif

  const int w = 8;
  const int h = 8;
  uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;
  int i, j;
  S32LDD(xr13,mc_coef,0x0);
  S32LDD(xr14,mc_coef,0x4);
  S32LDD(xr15,mc_coef,0x8);
  for(j = 0; j < h; j++){
    for(i = 0; i < w; i+=2){
#if 1
      S8LDD(xr1,&src[srcStride*-1+i],-1,0);S8LDD(xr1,&src[srcStride*-1+i],2,1);
      S8LDD(xr1,&src[srcStride*2+i],-1,2);S8LDD(xr1,&src[srcStride*2+i],2,3);

      S8LDD(xr2,&src[srcStride*-1+i],1,0);S8LDD(xr2,&src[srcStride*0+i],-1,1);
      S8LDD(xr2,&src[srcStride*0+i],2,2);S8LDD(xr2,&src[srcStride*2+i],1,3);

      S8LDD(xr3,&src[srcStride*-1+i],0,0);S8LDD(xr3,&src[srcStride*1+i],-1,1);
      S8LDD(xr3,&src[srcStride*1+i],2,2);S8LDD(xr3,&src[srcStride*2+i],0,3);
      S8LDD(xr4,&src[srcStride*0+i],1,0);
      D8SUM(xr2,xr3,xr2);
      D8SUM(xr1,xr0,xr1);
      D16MUL_WW(xr3,xr2,xr14,xr2);

      S8LDD(xr4,&src[srcStride*0+i],0,1);
      S8LDD(xr4,&src[srcStride*1+i],1,2);S8LDD(xr4,&src[srcStride*1+i],0,3);
      S8LDD(xr6,&src[srcStride*-1+i],0,0);
      Q8MUL(xr5,xr4,xr13,xr4);
      Q16ACCM_SS(xr5,xr3,xr2,xr4);

      S8LDD(xr6,&src[srcStride*-1+i],3,1);
      S8LDD(xr6,&src[srcStride*2+i],0,2);S8LDD(xr6,&src[srcStride*2+i],3,3);

      S8LDD(xr2,&src[srcStride*-1+i],2,0);S8LDD(xr2,&src[srcStride*0+i],0,1);
      S8LDD(xr2,&src[srcStride*0+i],3,2);S8LDD(xr2,&src[srcStride*2+i],2,3);

      S8LDD(xr3,&src[srcStride*-1+i],1,0);S8LDD(xr3,&src[srcStride*1+i],0,1);
      S8LDD(xr3,&src[srcStride*1+i],3,2);S8LDD(xr3,&src[srcStride*2+i],1,3);
      S8LDD(xr7,&src[srcStride*0+i],2,0);
      D8SUM(xr2,xr3,xr2);
      D8SUM(xr6,xr0,xr6);
      D16MUL_WW(xr3,xr2,xr14,xr2);

      S8LDD(xr7,&src[srcStride*0+i],1,1);
      S8LDD(xr7,&src[srcStride*1+i],2,2);S8LDD(xr7,&src[srcStride*1+i],1,3);
      Q8MUL(xr8,xr7,xr13,xr7);
      Q16ACCM_SS(xr8,xr3,xr2,xr7);

      D32SLL(xr3,xr0,xr0,xr2,0);
      D16ASUM_AA(xr1,xr4,xr7,xr6);
      D16ASUM_AA(xr2,xr5,xr8,xr3);
      D32ASUM_AA(xr2,xr1,xr6,xr3);

      D32ASUM_AA(xr2,xr15,xr15,xr3);
      D32SAR(xr2,xr2,xr3,xr3,8);
      Q16SAT(xr2,xr3,xr2);
      S8STD(xr2,&dst[i],0,0);
      S8STD(xr2,&dst[i],1,2);
#else
      op_put(dst[i], (
		      src[srcStride*-1+i-1]  -12*src[srcStride*-1+i+1]  -6*src[srcStride*-1+i]    +src[srcStride*-1+i+2]+
		      -12*src[srcStride* 0+i-1] +144*src[srcStride* 0+i+1] +72*src[srcStride* 0+i] -12*src[srcStride* 0+i+2]+
		      -6*src[srcStride* 1+i-1]  +72*src[srcStride* 1+i+1] +36*src[srcStride* 1+i]  -6*src[srcStride* 1+i+2]+
		      src[srcStride* 2+i-1]  -12*src[srcStride* 2+i+1]  -6*src[srcStride* 2+i]    +src[srcStride* 2+i+2]+
		      128)>>8);
#endif
    }
    src += srcStride;
    dst += dstStride;
  }
#ifdef JZC_PMON_P0
  //PMON_OFF(rv8vlc);
#endif
}

static void put_rv30_tpel8_hvv_lowpass(uint8_t *dst, uint8_t *src, int dstStride, int srcStride){
#ifdef JZC_PMON_P0
  //PMON_ON(rv8vlc);
#endif

#ifdef MC_USE_TCSM
  dstStride = 16;
#endif

  const int w = 8;
  const int h = 8;
  uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;
  int i, j;
  S32LDD(xr13,mc_coef,0x0);
  S32LDD(xr14,mc_coef,0x4);
  S32LDD(xr15,mc_coef,0x8);
  for(j = 0; j < h; j++){
    for(i = 0; i < w; i+=2){
#if 1
      S8LDD(xr1,&src[srcStride*-1+i],-1,0);S8LDD(xr1,&src[srcStride*-1+i],2,1);
      S8LDD(xr1,&src[srcStride*2+i],-1,2);S8LDD(xr1,&src[srcStride*2+i],2,3);

      S8LDD(xr2,&src[srcStride*-1+i],0,0);S8LDD(xr2,&src[srcStride*1+i],-1,1);
      S8LDD(xr2,&src[srcStride*1+i],2,2);S8LDD(xr2,&src[srcStride*2+i],0,3);
      D8SUM(xr1,xr0,xr1);

      S8LDD(xr3,&src[srcStride*-1+i],1,0);S8LDD(xr3,&src[srcStride*0+i],-1,1);
      S8LDD(xr3,&src[srcStride*0+i],2,2);S8LDD(xr3,&src[srcStride*2+i],1,3);
      D8SUM(xr2,xr3,xr2);
      D16MUL_WW(xr3,xr2,xr14,xr2);

      S8LDD(xr4,&src[srcStride*1+i],0,0);S8LDD(xr4,&src[srcStride*1+i],1,1);
      S8LDD(xr4,&src[srcStride*0+i],0,2);S8LDD(xr4,&src[srcStride*0+i],1,3);
      Q8MUL(xr5,xr4,xr13,xr4);
      Q16ACCM_SS(xr5,xr3,xr2,xr4);

      S8LDD(xr6,&src[srcStride*-1+i],0,0);S8LDD(xr6,&src[srcStride*-1+i],3,1);
      S8LDD(xr6,&src[srcStride*2+i],0,2);S8LDD(xr6,&src[srcStride*2+i],3,3);

      S8LDD(xr2,&src[srcStride*-1+i],1,0);S8LDD(xr2,&src[srcStride*1+i],0,1);
      S8LDD(xr2,&src[srcStride*1+i],3,2);S8LDD(xr2,&src[srcStride*2+i],1,3);
      D8SUM(xr6,xr0,xr6);

      S8LDD(xr3,&src[srcStride*-1+i],2,0);S8LDD(xr3,&src[srcStride*0+i],0,1);
      S8LDD(xr3,&src[srcStride*0+i],3,2);S8LDD(xr3,&src[srcStride*2+i],2,3);
      D8SUM(xr2,xr3,xr2);
      D16MUL_WW(xr3,xr2,xr14,xr2);

      S8LDD(xr7,&src[srcStride*1+i],1,0);S8LDD(xr7,&src[srcStride*1+i],2,1);
      S8LDD(xr7,&src[srcStride*0+i],1,2);S8LDD(xr7,&src[srcStride*0+i],2,3);
      Q8MUL(xr8,xr7,xr13,xr7);
      Q16ACCM_SS(xr8,xr3,xr2,xr7);

      D32SLL(xr3,xr0,xr0,xr2,0);
      D16ASUM_AA(xr1,xr4,xr7,xr6);
      D16ASUM_AA(xr2,xr5,xr8,xr3);
      D32ASUM_AA(xr2,xr1,xr6,xr3);

      D32ASUM_AA(xr2,xr15,xr15,xr3);
      D32SAR(xr2,xr2,xr3,xr3,8);
      Q16SAT(xr2,xr3,xr2);
      S8STD(xr2,&dst[i],0,0);
      S8STD(xr2,&dst[i],1,2);
#else
      op_put(dst[i], (
		      src[srcStride*-1+i-1]  -12*src[srcStride*-1+i]  -6*src[srcStride*-1+i+1]    +src[srcStride*-1+i+2]+
		      -6*src[srcStride* 0+i-1]  +72*src[srcStride* 0+i] +36*src[srcStride* 0+i+1]  -6*src[srcStride* 0+i+2]+
		      -12*src[srcStride* 1+i-1] +144*src[srcStride* 1+i] +72*src[srcStride* 1+i+1] -12*src[srcStride* 1+i+2]+
		      src[srcStride* 2+i-1]  -12*src[srcStride* 2+i]  -6*src[srcStride* 2+i+1]    +src[srcStride* 2+i+2]+
		      128)>>8);
#endif
    }
    src += srcStride;
    dst += dstStride;
  }
#ifdef JZC_PMON_P0
  //PMON_OFF(rv8vlc);
#endif
}

static void put_rv30_tpel8_hhvv_lowpass(uint8_t *dst, uint8_t *src, int dstStride, int srcStride){
#ifdef JZC_PMON_P0
  //PMON_ON(rv8vlc);
#endif

#ifdef MC_USE_TCSM
  dstStride = 16;
#endif

  const int w = 8;
  const int h = 8;
  const uint8_t mc_coef[12]={36,54,6,54,81,9,6,9,128,0,0,0};
  uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;
  int i, j;
  S32LDD(xr13,mc_coef,0x0);
  S32LDD(xr14,mc_coef,0x4);
  S32LDD(xr15,mc_coef,0x8);
  for(j = 0; j < h; j++){
    for(i = 0; i < w; i+=2){
#if 1
      S8LDD(xr1,&src[srcStride*0+i],0,0);S8LDD(xr1,&src[srcStride*0+i],1,1);
      S8LDD(xr1,&src[srcStride*0+i],2,2);S8LDD(xr1,&src[srcStride*1+i],0,3);
      Q8MUL(xr2,xr1,xr13,xr1);

      S8LDD(xr3,&src[srcStride*1+i],1,0);S8LDD(xr3,&src[srcStride*1+i],2,1);
      S8LDD(xr3,&src[srcStride*2+i],0,2);S8LDD(xr3,&src[srcStride*2+i],1,3);
      Q8MUL(xr4,xr3,xr14,xr3);

      S8LDD(xr7,&src[srcStride*0+i],1,0);S8LDD(xr7,&src[srcStride*0+i],2,1);
      S8LDD(xr7,&src[srcStride*0+i],3,2);S8LDD(xr7,&src[srcStride*1+i],1,3);
      Q8MUL(xr8,xr7,xr13,xr7);

      S8LDD(xr9,&src[srcStride*1+i],2,0);S8LDD(xr9,&src[srcStride*1+i],3,1);
      S8LDD(xr9,&src[srcStride*2+i],1,2);S8LDD(xr9,&src[srcStride*2+i],2,3);
      Q8MUL(xr10,xr9,xr14,xr9);

      D32ADD_AA(xr5,xr0,xr0,xr6);
      S8LDD(xr5,&src[i+srcStride*2],2,0);
      S8LDD(xr6,&src[i+srcStride*2],3,0);
      Q16ACCM_AA(xr1,xr2,xr4,xr3);
      Q16ACCM_AA(xr7,xr8,xr10,xr9);
      Q16ACCM_AA(xr1,xr3,xr9,xr7);
      D16ASUM_AA(xr5,xr1,xr7,xr6);
      D32ASUM_AA(xr5,xr15,xr15,xr6);
      D32SLR(xr5,xr5,xr6,xr6,8);

      Q16SAT(xr5,xr6,xr5);
      S8STD(xr5,&dst[i],0,0);
      S8STD(xr5,&dst[i],1,2);
#else
      op_put(dst[i], (
		      36*src[i+srcStride*0] +54*src[i+1+srcStride*0] +6*src[i+2+srcStride*0]+
		      54*src[i+srcStride*1] +81*src[i+1+srcStride*1] +9*src[i+2+srcStride*1]+
		      6*src[i+srcStride*2] + 9*src[i+1+srcStride*2] +  src[i+2+srcStride*2]+
		      128)>>8);
#endif
    }
    src += srcStride;
    dst += dstStride;
  }
#ifdef JZC_PMON_P0
  //PMON_OFF(rv8vlc);
#endif
}

static av_unused void avg_rv30_tpel8_h_lowpass(uint8_t *dst, uint8_t *src, int dstStride, int srcStride, const int C1, const int C2){
#ifdef JZC_PMON_P0
  //PMON_ON(rv8vlc);
#endif

#ifdef MC_USE_TCSM
  dstStride = 16;
#endif

  const int h=8;
  uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;
  int i,j;
  S8LDD(xr13,&C1,0,7);
  S8LDD(xr14,&C2,0,7);
  //S8LDD(xr14,&C2,0,3);
  S32I2M(xr15,0x00080008);
  for(i=0; i<h; i++){
#if 1
    S8LDD(xr1,src,-1,3);S8LDD(xr3,src,1,3);
    S8LDD(xr1,src,0,2);S8LDD(xr3,src,2,2);
    S8LDD(xr1,src,1,1);S8LDD(xr3,src,3,1);
    S8LDD(xr1,src,2,0);S8LDD(xr3,src,4,0);
    D32SLL(xr2,xr1,xr1,xr2,8);
    D32SLL(xr4,xr3,xr3,xr4,8);
    S8LDD(xr2,src,3,0);
    S8LDD(xr4,src,5,0);
    Q8MUL(xr5,xr2,xr13,xr6);
    Q8MUL(xr7,xr3,xr14,xr8);

    Q8ADDE_AA(xr9,xr1,xr4,xr10);
    Q16ACCM_AA(xr5,xr15,xr15,xr6);
    Q16ACCM_SS(xr7,xr9,xr10,xr8);

    S8LDD(xr1,src,3,3);S8LDD(xr3,src,5,3);
    Q16ACCM_AA(xr5,xr7,xr8,xr6);
    S8LDD(xr1,src,4,2);S8LDD(xr3,src,6,2);
    Q16SAR(xr5,xr5,xr6,xr6,4);
    S8LDD(xr1,src,5,1);S8LDD(xr3,src,7,1);
    Q16SAT(xr9,xr5,xr6);
    S8LDD(xr1,src,6,0);S8LDD(xr3,src,8,0);
    S32LDDR(xr8,dst,0);
    Q8AVGR(xr9,xr8,xr9);
    S32STDR(xr9,dst,0);

    D32SLL(xr2,xr1,xr1,xr2,8);
    D32SLL(xr4,xr3,xr3,xr4,8);
    S8LDD(xr2,src,7,0);
    S8LDD(xr4,src,9,0);

    Q8MUL(xr5,xr2,xr13,xr6);
    Q8MUL(xr7,xr3,xr14,xr8);

    Q8ADDE_AA(xr9,xr1,xr4,xr10);
    Q16ACCM_AA(xr5,xr15,xr15,xr6);
    Q16ACCM_SS(xr7,xr9,xr10,xr8);
    Q16ACCM_AA(xr5,xr7,xr8,xr6);
    S32LDDR(xr8,dst,4);
    Q16SAR(xr5,xr5,xr6,xr6,4);
    Q16SAT(xr9,xr5,xr6);
    Q8AVGR(xr9,xr8,xr9);
    S32STDR(xr9,dst,4);
    op_avg(j, (-(src[ 5]+src[8]) + src[6]*C1 + src[7]*C2 + 8)>>4);
    op_put(j, (-(src[ 5]+src[8]) + src[6] + src[7] + 8));
#else
    op_avg(dst[0], (-(src[-1]+src[2]) + src[0]*C1 + src[1]*C2 + 8)>>4);
    op_avg(dst[1], (-(src[ 0]+src[3]) + src[1]*C1 + src[2]*C2 + 8)>>4);
    op_avg(dst[2], (-(src[ 1]+src[4]) + src[2]*C1 + src[3]*C2 + 8)>>4);
    op_avg(dst[3], (-(src[ 2]+src[5]) + src[3]*C1 + src[4]*C2 + 8)>>4);
    op_avg(dst[4], (-(src[ 3]+src[6]) + src[4]*C1 + src[5]*C2 + 8)>>4);
    op_avg(dst[5], (-(src[ 4]+src[7]) + src[5]*C1 + src[6]*C2 + 8)>>4);
    op_avg(dst[6], (-(src[ 5]+src[8]) + src[6]*C1 + src[7]*C2 + 8)>>4);
    op_avg(dst[7], (-(src[ 6]+src[9]) + src[7]*C1 + src[8]*C2 + 8)>>4);
#endif
    dst+=dstStride;
    src+=srcStride;
  }
#ifdef JZC_PMON_P0
  //PMON_OFF(rv8vlc);
#endif
}

static void avg_rv30_tpel8_v_lowpass(uint8_t *dst, uint8_t *src, int dstStride, int srcStride, const int C1, const int C2){
#ifdef JZC_PMON_P0
  //PMON_ON(rv8vlc);
#endif

#ifdef MC_USE_TCSM
  dstStride = 16;
#endif

  const int w=8;
  uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;
  int i;
  S8LDD(xr14,&C1,0,7);
  S8LDD(xr14,&C2,0,2);
  S8LDD(xr14,&C2,0,3);
  S32I2M(xr15,0x00080008);
  uint8_t *srcl=RECON_YBUF;
  uint8_t *dstl;
  for(i=0; i<w; i++){
    srcl[0]= src[-1*srcStride];
    srcl[1]= src[0 *srcStride];
    srcl[2]= src[1 *srcStride];
    srcl[3]= src[2 *srcStride];
    srcl[4]= src[3 *srcStride];
    srcl[5]= src[4 *srcStride];
    srcl[6]= src[5 *srcStride];
    srcl[7]= src[6 *srcStride];
    srcl[8]= src[7 *srcStride];
    srcl[9]= src[8 *srcStride];
    srcl[10]= src[9 *srcStride];

    S8LDD(xr1,srcl,2,7);S8LDD(xr1,srcl,1,0);S8LDD(xr1,srcl,3,3);
    S8LDD(xr2,srcl,4,7);S8LDD(xr2,srcl,3,0);S8LDD(xr2,srcl,5,3);
    Q8MUL(xr6,xr1,xr14,xr5);
    S32LDD(xr3,srcl,0);
    S8LDD(xr4,srcl,3,0);S8LDD(xr4,srcl,4,1);S8LDD(xr4,srcl,5,2);S8LDD(xr4,srcl,6,3);
    Q8MUL(xr8,xr2,xr14,xr7);
    Q8ADDE_AA(xr10,xr3,xr4,xr9);

    Q16ACCM_SS(xr5,xr9,xr10,xr7);
    Q16ACCM_AA(xr6,xr15,xr15,xr8);
    Q16ACCM_AA(xr5,xr6,xr8,xr7);
    Q16SAR(xr5,xr5,xr7,xr7,4);
    Q16SAT(xr5,xr7,xr5);

    S8LDD(xr2,dst,0,0);
    S8LDD(xr2,dst+dstStride,0,1);
    S8LDD(xr2,dst+2*dstStride,0,2);
    S8LDD(xr2,dst+3*dstStride,0,3);
    Q8AVGR(xr5,xr2,xr5);

    S8STD(xr5,dst,0,0);
    S8STD(xr5,dst+dstStride,0,1);
    S8STD(xr5,dst+2*dstStride,0,2);
    S8STD(xr5,dst+3*dstStride,0,3);

    S8LDD(xr1,srcl,6,7);S8LDD(xr1,srcl,5,0);S8LDD(xr1,srcl,7,3);
    S8LDD(xr2,srcl,8,7);S8LDD(xr2,srcl,7,0);S8LDD(xr2,srcl,9,3);
    Q8MUL(xr6,xr1,xr14,xr5);
    S32LDD(xr3,srcl,4);
    S8LDD(xr4,srcl,7,0);S8LDD(xr4,srcl,8,1);S8LDD(xr4,srcl,9,2);S8LDD(xr4,srcl,10,3);
    Q8MUL(xr8,xr2,xr14,xr7);
    Q8ADDE_AA(xr10,xr3,xr4,xr9);

    Q16ACCM_SS(xr5,xr9,xr10,xr7);
    Q16ACCM_AA(xr6,xr15,xr15,xr8);
    Q16ACCM_AA(xr5,xr6,xr8,xr7);
    Q16SAR(xr5,xr5,xr7,xr7,4);
    Q16SAT(xr5,xr7,xr5);

    S8LDD(xr2,dst+4*dstStride,0,0);
    S8LDD(xr2,dst+5*dstStride,0,1);
    S8LDD(xr2,dst+6*dstStride,0,2);
    S8LDD(xr2,dst+7*dstStride,0,3);
    Q8AVGR(xr5,xr2,xr5);

    S8STD(xr5,dst+4*dstStride,0,0);
    S8STD(xr5,dst+5*dstStride,0,1);
    S8STD(xr5,dst+6*dstStride,0,2);
    S8STD(xr5,dst+7*dstStride,0,3);

    dst++;
    src++;
  }
#ifdef JZC_PMON_P0
  //PMON_OFF(rv8vlc);
#endif
}

static void avg_rv30_tpel8_hv_lowpass(uint8_t *dst, uint8_t *src, int dstStride, int srcStride){///pmon 0
#ifdef JZC_PMON_P0
  //PMON_ON(rv8vlc);
#endif

#ifdef MC_USE_TCSM
  dstStride = 16;
#endif

  const int w = 8;
  const int h = 8;
  uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;
  int i, j, k;
  S32LDD(xr13,mc_coef,0x0);
  S32LDD(xr14,mc_coef,0x4);
  S32LDD(xr15,mc_coef,0x8);
  for(j = 0; j < h; j++){
    for(i = 0; i < w; i+=2){
#if 1
      S8LDD(xr1,&src[srcStride*-1+i],-1,0);S8LDD(xr1,&src[srcStride*-1+i],2,1);
      S8LDD(xr1,&src[srcStride*2+i],-1,2);S8LDD(xr1,&src[srcStride*2+i],2,3);

      S8LDD(xr2,&src[srcStride*-1+i],0,0);S8LDD(xr2,&src[srcStride*0+i],-1,1);
      S8LDD(xr2,&src[srcStride*0+i],2,2);S8LDD(xr2,&src[srcStride*2+i],0,3);
      D8SUM(xr1,xr0,xr1);

      S8LDD(xr3,&src[srcStride*-1+i],1,0);S8LDD(xr3,&src[srcStride*1+i],-1,1);
      S8LDD(xr3,&src[srcStride*1+i],2,2);S8LDD(xr3,&src[srcStride*2+i],1,3);
      D8SUM(xr2,xr3,xr2);
      D16MUL_WW(xr3,xr2,xr14,xr2);

      S8LDD(xr4,&src[srcStride*0+i],0,0);S8LDD(xr4,&src[srcStride*0+i],1,1);
      S8LDD(xr4,&src[srcStride*1+i],0,2);S8LDD(xr4,&src[srcStride*1+i],1,3);
      Q8MUL(xr5,xr4,xr13,xr4);
      Q16ACCM_SS(xr5,xr3,xr2,xr4);

      S8LDD(xr6,&src[srcStride*-1+i],0,0);S8LDD(xr6,&src[srcStride*-1+i],3,1);
      S8LDD(xr6,&src[srcStride*2+i],0,2);S8LDD(xr6,&src[srcStride*2+i],3,3);

      S8LDD(xr2,&src[srcStride*-1+i],1,0);S8LDD(xr2,&src[srcStride*0+i],0,1);
      S8LDD(xr2,&src[srcStride*0+i],3,2);S8LDD(xr2,&src[srcStride*2+i],1,3);
      D8SUM(xr6,xr0,xr6);

      S8LDD(xr3,&src[srcStride*-1+i],2,0);S8LDD(xr3,&src[srcStride*1+i],0,1);
      S8LDD(xr3,&src[srcStride*1+i],3,2);S8LDD(xr3,&src[srcStride*2+i],2,3);
      D8SUM(xr2,xr3,xr2);
      D16MUL_WW(xr3,xr2,xr14,xr2);

      S8LDD(xr7,&src[srcStride*0+i],1,0);S8LDD(xr7,&src[srcStride*0+i],2,1);
      S8LDD(xr7,&src[srcStride*1+i],1,2);S8LDD(xr7,&src[srcStride*1+i],2,3);
      Q8MUL(xr8,xr7,xr13,xr7);
      Q16ACCM_SS(xr8,xr3,xr2,xr7);

      D32SLL(xr3,xr0,xr0,xr2,0);
      D16ASUM_AA(xr1,xr4,xr7,xr6);
      D16ASUM_AA(xr2,xr5,xr8,xr3);
      D32ASUM_AA(xr2,xr1,xr6,xr3);

      D32ASUM_AA(xr2,xr15,xr15,xr3);
      D32SAR(xr2,xr2,xr3,xr3,8);
      Q16SAT(xr2,xr3,xr2);
      S8LDD(xr4,&dst[i],0,0);
      S8LDD(xr4,&dst[i],1,2);
      Q8AVGR(xr2,xr2,xr4);

      S8STD(xr2,&dst[i],0,0);
      S8STD(xr2,&dst[i],1,2);
#else
      op_avg(dst[i], (
		      src[srcStride*-1+i-1]  -12*src[srcStride*-1+i]  -6*src[srcStride*-1+i+1]    +src[srcStride*-1+i+2]+
		      -12*src[srcStride* 0+i-1] +144*src[srcStride* 0+i] +72*src[srcStride* 0+i+1] -12*src[srcStride* 0+i+2] +
		      -6*src[srcStride* 1+i-1]  +72*src[srcStride* 1+i] +36*src[srcStride* 1+i+1]  -6*src[srcStride* 1+i+2] +
		      src[srcStride* 2+i-1]  -12*src[srcStride* 2+i]  -6*src[srcStride* 2+i+1]    +src[srcStride* 2+i+2] +
		      128)>>8);
#endif
    }
    src += srcStride;
    dst += dstStride;
  }
#ifdef JZC_PMON_P0
  //PMON_ON(rv8vlc);
#endif
}

static void avg_rv30_tpel8_hhv_lowpass(uint8_t *dst, uint8_t *src, int dstStride, int srcStride){
#ifdef JZC_PMON_P0
  //PMON_ON(rv8vlc);
#endif

#ifdef MC_USE_TCSM
  dstStride = 16;
#endif

  const int w = 8;
  const int h = 8;
  uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;
  int i, j, k;
  //S32LDD(xr13,mc_coef,0x0);
  //S32LDD(xr14,mc_coef,0x4);
  //S32LDD(xr15,mc_coef,0x8);
  for(j = 0; j < h; j++){
    for(i = 0; i < w; i++){
#if 0
      S8LDD(xr1,&src[srcStride*-1+i],-1,0);S8LDD(xr1,&src[srcStride*-1+i],2,1);
      S8LDD(xr1,&src[srcStride*2+i],-1,2);S8LDD(xr1,&src[srcStride*2+i],2,3);

      S8LDD(xr2,&src[srcStride*-1+i],1,0);S8LDD(xr2,&src[srcStride*0+i],-1,1);
      S8LDD(xr2,&src[srcStride*0+i],2,2);S8LDD(xr2,&src[srcStride*2+i],1,3);
      D8SUM(xr1,xr0,xr1);

      S8LDD(xr3,&src[srcStride*-1+i],0,0);S8LDD(xr3,&src[srcStride*1+i],-1,1);
      S8LDD(xr3,&src[srcStride*1+i],2,2);S8LDD(xr3,&src[srcStride*2+i],0,3);
      D8SUM(xr2,xr3,xr2);
      D16MUL_WW(xr3,xr2,xr14,xr2);

      S8LDD(xr4,&src[srcStride*0+i],1,0);S8LDD(xr4,&src[srcStride*0+i],0,1);
      S8LDD(xr4,&src[srcStride*1+i],1,2);S8LDD(xr4,&src[srcStride*1+i],0,3);
      Q8MUL(xr5,xr4,xr13,xr4);
      Q16ACCM_SS(xr5,xr3,xr2,xr4);

      S8LDD(xr6,&src[srcStride*-1+i],0,0);S8LDD(xr6,&src[srcStride*-1+i],3,1);
      S8LDD(xr6,&src[srcStride*2+i],0,2);S8LDD(xr6,&src[srcStride*2+i],3,3);

      S8LDD(xr2,&src[srcStride*-1+i],2,0);S8LDD(xr2,&src[srcStride*0+i],0,1);
      S8LDD(xr2,&src[srcStride*0+i],3,2);S8LDD(xr2,&src[srcStride*2+i],2,3);
      D8SUM(xr6,xr0,xr6);

      S8LDD(xr3,&src[srcStride*-1+i],1,0);S8LDD(xr3,&src[srcStride*1+i],0,1);
      S8LDD(xr3,&src[srcStride*1+i],3,2);S8LDD(xr3,&src[srcStride*2+i],1,3);
      D8SUM(xr2,xr3,xr2);
      D16MUL_WW(xr3,xr2,xr14,xr2);

      S8LDD(xr7,&src[srcStride*0+i],2,0);S8LDD(xr7,&src[srcStride*0+i],1,1);
      S8LDD(xr7,&src[srcStride*1+i],2,2);S8LDD(xr7,&src[srcStride*1+i],1,3);
      Q8MUL(xr8,xr7,xr13,xr7);
      Q16ACCM_SS(xr8,xr3,xr2,xr7);

      D32SLL(xr3,xr0,xr0,xr2,0);
      D16ASUM_AA(xr1,xr4,xr7,xr6);
      D16ASUM_AA(xr2,xr5,xr8,xr3);
      D32ASUM_AA(xr2,xr1,xr6,xr3);

      D32ASUM_AA(xr2,xr15,xr15,xr3);
      D32SAR(xr2,xr2,xr3,xr3,8);
      Q16SAT(xr2,xr3,xr2);
      S8LDD(xr4,&dst[i],0,0);
      S8LDD(xr4,&dst[i],1,2);
      Q8AVGR(xr2,xr2,xr4);
      //op_put(k, (-(src[ 5]+src[8]) + src[6] + src[7] + 8)>>4);
      //op_put(k, (-(src[ 5]+src[8]) + src[6] + src[7] + 8));
      S8STD(xr2,&dst[i],0,0);
      S8STD(xr2,&dst[i],1,2);
#else
      op_avg(dst[i], (
		      src[srcStride*-1+i-1]  -12*src[srcStride*-1+i+1]  -6*src[srcStride*-1+i]    +src[srcStride*-1+i+2]+
		      -12*src[srcStride* 0+i-1] +144*src[srcStride* 0+i+1] +72*src[srcStride* 0+i] -12*src[srcStride* 0+i+2]+
		      -6*src[srcStride* 1+i-1]  +72*src[srcStride* 1+i+1] +36*src[srcStride* 1+i]  -6*src[srcStride* 1+i+2]+
		      src[srcStride* 2+i-1]  -12*src[srcStride* 2+i+1]  -6*src[srcStride* 2+i]    +src[srcStride* 2+i+2]+
		      128)>>8);
#endif
    }
    src += srcStride;
    dst += dstStride;
  }
#ifdef JZC_PMON_P0
  //PMON_OFF(rv8vlc);
#endif
}

static void avg_rv30_tpel8_hvv_lowpass(uint8_t *dst, uint8_t *src, int dstStride, int srcStride){
#ifdef JZC_PMON_P0
  //PMON_ON(rv8vlc);
#endif

#ifdef MC_USE_TCSM
  dstStride = 16;
#endif

    const int w = 8;
    const int h = 8;
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;
    int i, j;
    //S32LDD(xr13,mc_coef,0x0);
    //S32LDD(xr14,mc_coef,0x4);
    //S32LDD(xr15,mc_coef,0x8);
    for(j = 0; j < h; j++){
        for(i = 0; i < w; i++){
#if 0
	  S8LDD(xr1,&src[srcStride*-1+i],-1,0);S8LDD(xr1,&src[srcStride*-1+i],2,1);
	  S8LDD(xr1,&src[srcStride*2+i],-1,2);S8LDD(xr1,&src[srcStride*2+i],2,3);

	  S8LDD(xr2,&src[srcStride*-1+i],0,0);S8LDD(xr2,&src[srcStride*1+i],-1,1);
	  S8LDD(xr2,&src[srcStride*1+i],2,2);S8LDD(xr2,&src[srcStride*2+i],0,3);
	  D8SUM(xr1,xr0,xr1);

	  S8LDD(xr3,&src[srcStride*-1+i],1,0);S8LDD(xr3,&src[srcStride*0+i],-1,1);
	  S8LDD(xr3,&src[srcStride*0+i],2,2);S8LDD(xr3,&src[srcStride*2+i],1,3);
	  D8SUM(xr2,xr3,xr2);
	  D16MUL_WW(xr3,xr2,xr14,xr2);

	  S8LDD(xr4,&src[srcStride*1+i],0,0);S8LDD(xr4,&src[srcStride*1+i],1,1);
	  S8LDD(xr4,&src[srcStride*0+i],0,2);S8LDD(xr4,&src[srcStride*0+i],1,3);
	  Q8MUL(xr5,xr4,xr13,xr4);
	  Q16ACCM_SS(xr5,xr3,xr2,xr4);

	  S8LDD(xr6,&src[srcStride*-1+i],0,0);S8LDD(xr6,&src[srcStride*-1+i],3,1);
	  S8LDD(xr6,&src[srcStride*2+i],0,2);S8LDD(xr6,&src[srcStride*2+i],3,3);

	  S8LDD(xr2,&src[srcStride*-1+i],1,0);S8LDD(xr2,&src[srcStride*1+i],0,1);
	  S8LDD(xr2,&src[srcStride*1+i],3,2);S8LDD(xr2,&src[srcStride*2+i],1,3);
	  D8SUM(xr6,xr0,xr6);

	  S8LDD(xr3,&src[srcStride*-1+i],2,0);S8LDD(xr3,&src[srcStride*0+i],0,1);
	  S8LDD(xr3,&src[srcStride*0+i],3,2);S8LDD(xr3,&src[srcStride*2+i],2,3);
	  D8SUM(xr2,xr3,xr2);
	  D16MUL_WW(xr3,xr2,xr14,xr2);

	  S8LDD(xr7,&src[srcStride*1+i],1,0);S8LDD(xr7,&src[srcStride*1+i],2,1);
	  S8LDD(xr7,&src[srcStride*0+i],1,2);S8LDD(xr7,&src[srcStride*0+i],2,3);
	  Q8MUL(xr8,xr7,xr13,xr7);
	  Q16ACCM_SS(xr8,xr3,xr2,xr7);

	  D32SLL(xr3,xr0,xr0,xr2,0);
	  D16ASUM_AA(xr1,xr4,xr7,xr6);
	  D16ASUM_AA(xr2,xr5,xr8,xr3);
	  D32ASUM_AA(xr2,xr1,xr6,xr3);

	  D32ASUM_AA(xr2,xr15,xr15,xr3);
	  D32SAR(xr2,xr2,xr3,xr3,8);
	  Q16SAT(xr2,xr3,xr2);
	  S8LDD(xr4,&dst[i],0,0);
	  S8LDD(xr4,&dst[i],1,2);
	  Q8AVGR(xr2,xr2,xr4);
	  S8STD(xr2,&dst[i],0,0);
	  S8STD(xr2,&dst[i],1,2);
#else
            op_avg(dst[i], (
                  src[srcStride*-1+i-1]  -12*src[srcStride*-1+i]  -6*src[srcStride*-1+i+1]    +src[srcStride*-1+i+2]+
               -6*src[srcStride* 0+i-1]  +72*src[srcStride* 0+i] +36*src[srcStride* 0+i+1]  -6*src[srcStride* 0+i+2]+
              -12*src[srcStride* 1+i-1] +144*src[srcStride* 1+i] +72*src[srcStride* 1+i+1] -12*src[srcStride* 1+i+2]+
                  src[srcStride* 2+i-1]  -12*src[srcStride* 2+i]  -6*src[srcStride* 2+i+1]    +src[srcStride* 2+i+2]+
                  128)>>8);
#endif
        }
        src += srcStride;
        dst += dstStride;
    }
#ifdef JZC_PMON_P0
    //PMON_OFF(rv8vlc);
#endif
}

static void avg_rv30_tpel8_hhvv_lowpass(uint8_t *dst, uint8_t *src, int dstStride, int srcStride){
#ifdef JZC_PMON_P0
  //PMON_ON(rv8vlc);
#endif

#ifdef MC_USE_TCSM
  dstStride = 16;
#endif

  const int w = 8;
  const int h = 8;
  const uint8_t mc_coef[12]={36,54,6,54,81,9,6,9,128,0,0,0};
  uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;
  int i, j;
  S32LDD(xr13,mc_coef,0x0);
  S32LDD(xr14,mc_coef,0x4);
  S32LDD(xr15,mc_coef,0x8);
  for(j = 0; j < h; j++){
    for(i = 0; i < w; i+=2){
#if 1
      S8LDD(xr1,&src[srcStride*0+i],0,0);S8LDD(xr1,&src[srcStride*0+i],1,1);
      S8LDD(xr1,&src[srcStride*0+i],2,2);S8LDD(xr1,&src[srcStride*1+i],0,3);
      Q8MUL(xr2,xr1,xr13,xr1);

      S8LDD(xr3,&src[srcStride*1+i],1,0);S8LDD(xr3,&src[srcStride*1+i],2,1);
      S8LDD(xr3,&src[srcStride*2+i],0,2);S8LDD(xr3,&src[srcStride*2+i],1,3);
      Q8MUL(xr4,xr3,xr14,xr3);

      S8LDD(xr7,&src[srcStride*0+i],1,0);S8LDD(xr7,&src[srcStride*0+i],2,1);
      S8LDD(xr7,&src[srcStride*0+i],3,2);S8LDD(xr7,&src[srcStride*1+i],1,3);
      Q8MUL(xr8,xr7,xr13,xr7);

      S8LDD(xr9,&src[srcStride*1+i],2,0);S8LDD(xr9,&src[srcStride*1+i],3,1);
      S8LDD(xr9,&src[srcStride*2+i],1,2);S8LDD(xr9,&src[srcStride*2+i],2,3);
      Q8MUL(xr10,xr9,xr14,xr9);

      D32ADD_AA(xr5,xr0,xr0,xr6);
      S8LDD(xr5,&src[i+srcStride*2],2,0);
      S8LDD(xr6,&src[i+srcStride*2],3,0);
      Q16ACCM_AA(xr1,xr2,xr4,xr3);
      Q16ACCM_AA(xr7,xr8,xr10,xr9);
      Q16ACCM_AA(xr1,xr3,xr9,xr7);
      D16ASUM_AA(xr5,xr1,xr7,xr6);
      D32ASUM_AA(xr5,xr15,xr15,xr6);
      D32SLR(xr5,xr5,xr6,xr6,8);

      Q16SAT(xr5,xr6,xr5);
      S8LDD(xr4,&dst[i],0,0);
      S8LDD(xr4,&dst[i],1,2);
      Q8AVGR(xr5,xr5,xr4);
      S8STD(xr5,&dst[i],0,0);
      S8STD(xr5,&dst[i],1,2);
#else
      op_avg(dst[i], (
		      36*src[i+srcStride*0] +54*src[i+1+srcStride*0] +6*src[i+2+srcStride*0]+
		      54*src[i+srcStride*1] +81*src[i+1+srcStride*1] +9*src[i+2+srcStride*1]+
		      6*src[i+srcStride*2] + 9*src[i+1+srcStride*2] +  src[i+2+srcStride*2]+
		      128)>>8);
#endif
    }
    src += srcStride;
    dst += dstStride;
  }
#ifdef JZC_PMON_P0
  //PMON_OFF(rv8vlc);
#endif
}

#define RV30_LOWPASS(OPNAME, OP) \
static void OPNAME ## rv30_tpel16_v_lowpass(uint8_t *dst, uint8_t *src, int dstStride, int srcStride, const int C1, const int C2){\
    OPNAME ## rv30_tpel8_v_lowpass(dst  , src  , dstStride, srcStride, C1, C2);\
    OPNAME ## rv30_tpel8_v_lowpass(dst+8, src+8, dstStride, srcStride, C1, C2);\
    src += 8*srcStride;\
    dst += 8*dstStride;\
    OPNAME ## rv30_tpel8_v_lowpass(dst  , src  , dstStride, srcStride, C1, C2);\
    OPNAME ## rv30_tpel8_v_lowpass(dst+8, src+8, dstStride, srcStride, C1, C2);\
}\
\
static void OPNAME ## rv30_tpel16_h_lowpass(uint8_t *dst, uint8_t *src, int dstStride, int srcStride, const int C1, const int C2){\
    OPNAME ## rv30_tpel8_h_lowpass(dst  , src  , dstStride, srcStride, C1, C2);\
    OPNAME ## rv30_tpel8_h_lowpass(dst+8, src+8, dstStride, srcStride, C1, C2);\
    src += 8*srcStride;\
    dst += 8*dstStride;\
    OPNAME ## rv30_tpel8_h_lowpass(dst  , src  , dstStride, srcStride, C1, C2);\
    OPNAME ## rv30_tpel8_h_lowpass(dst+8, src+8, dstStride, srcStride, C1, C2);\
}\
\
static void OPNAME ## rv30_tpel16_hv_lowpass(uint8_t *dst, uint8_t *src, int dstStride, int srcStride){\
    OPNAME ## rv30_tpel8_hv_lowpass(dst  , src  , dstStride, srcStride);\
    OPNAME ## rv30_tpel8_hv_lowpass(dst+8, src+8, dstStride, srcStride);\
    src += 8*srcStride;\
    dst += 8*dstStride;\
    OPNAME ## rv30_tpel8_hv_lowpass(dst  , src  , dstStride, srcStride);\
    OPNAME ## rv30_tpel8_hv_lowpass(dst+8, src+8, dstStride, srcStride);\
}\
\
static void OPNAME ## rv30_tpel16_hhv_lowpass(uint8_t *dst, uint8_t *src, int dstStride, int srcStride){\
    OPNAME ## rv30_tpel8_hhv_lowpass(dst  , src  , dstStride, srcStride);\
    OPNAME ## rv30_tpel8_hhv_lowpass(dst+8, src+8, dstStride, srcStride);\
    src += 8*srcStride;\
    dst += 8*dstStride;\
    OPNAME ## rv30_tpel8_hhv_lowpass(dst  , src  , dstStride, srcStride);\
    OPNAME ## rv30_tpel8_hhv_lowpass(dst+8, src+8, dstStride, srcStride);\
}\
\
static void OPNAME ## rv30_tpel16_hvv_lowpass(uint8_t *dst, uint8_t *src, int dstStride, int srcStride){\
    OPNAME ## rv30_tpel8_hvv_lowpass(dst  , src  , dstStride, srcStride);\
    OPNAME ## rv30_tpel8_hvv_lowpass(dst+8, src+8, dstStride, srcStride);\
    src += 8*srcStride;\
    dst += 8*dstStride;\
    OPNAME ## rv30_tpel8_hvv_lowpass(dst  , src  , dstStride, srcStride);\
    OPNAME ## rv30_tpel8_hvv_lowpass(dst+8, src+8, dstStride, srcStride);\
}\
\
static void OPNAME ## rv30_tpel16_hhvv_lowpass(uint8_t *dst, uint8_t *src, int dstStride, int srcStride){\
    OPNAME ## rv30_tpel8_hhvv_lowpass(dst  , src  , dstStride, srcStride);\
    OPNAME ## rv30_tpel8_hhvv_lowpass(dst+8, src+8, dstStride, srcStride);\
    src += 8*srcStride;\
    dst += 8*dstStride;\
    OPNAME ## rv30_tpel8_hhvv_lowpass(dst  , src  , dstStride, srcStride);\
    OPNAME ## rv30_tpel8_hhvv_lowpass(dst+8, src+8, dstStride, srcStride);\
}\
\

#define RV30_MC(OPNAME, SIZE) \
static void OPNAME ## rv30_tpel ## SIZE ## _mc10_c(uint8_t *dst, uint8_t *src, int stride){\
    OPNAME ## rv30_tpel ## SIZE ## _h_lowpass(dst, src, stride, stride, 12, 6);\
}\
\
static void OPNAME ## rv30_tpel ## SIZE ## _mc20_c(uint8_t *dst, uint8_t *src, int stride){\
    OPNAME ## rv30_tpel ## SIZE ## _h_lowpass(dst, src, stride, stride, 6, 12);\
}\
\
static void OPNAME ## rv30_tpel ## SIZE ## _mc01_c(uint8_t *dst, uint8_t *src, int stride){\
    OPNAME ## rv30_tpel ## SIZE ## _v_lowpass(dst, src, stride, stride, 12, 6);\
}\
\
static void OPNAME ## rv30_tpel ## SIZE ## _mc02_c(uint8_t *dst, uint8_t *src, int stride){\
    OPNAME ## rv30_tpel ## SIZE ## _v_lowpass(dst, src, stride, stride, 6, 12);\
}\
\
static void OPNAME ## rv30_tpel ## SIZE ## _mc11_c(uint8_t *dst, uint8_t *src, int stride){\
    OPNAME ## rv30_tpel ## SIZE ## _hv_lowpass(dst, src, stride, stride);\
}\
\
static void OPNAME ## rv30_tpel ## SIZE ## _mc12_c(uint8_t *dst, uint8_t *src, int stride){\
    OPNAME ## rv30_tpel ## SIZE ## _hvv_lowpass(dst, src, stride, stride);\
}\
\
static void OPNAME ## rv30_tpel ## SIZE ## _mc21_c(uint8_t *dst, uint8_t *src, int stride){\
    OPNAME ## rv30_tpel ## SIZE ## _hhv_lowpass(dst, src, stride, stride);\
}\
\
static void OPNAME ## rv30_tpel ## SIZE ## _mc22_c(uint8_t *dst, uint8_t *src, int stride){\
    OPNAME ## rv30_tpel ## SIZE ## _hhvv_lowpass(dst, src, stride, stride);\
}\
\

RV30_LOWPASS(put_       , op_put)
RV30_LOWPASS(avg_       , op_avg)
RV30_MC(put_, 8)
RV30_MC(put_, 16)
RV30_MC(avg_, 8)
RV30_MC(avg_, 16)

av_cold void ff_rv30dsp_init(DSPContext* c, AVCodecContext *avctx) {
  c->put_rv30_tpel_pixels_tab[0][ 0] = c->put_h264_qpel_pixels_tab[0][0];
  //c->put_rv30_tpel_pixels_tab[0][ 0] = put_rv30_pixels16_mxu;
    c->put_rv30_tpel_pixels_tab[0][ 1] = put_rv30_tpel16_mc10_c;
    c->put_rv30_tpel_pixels_tab[0][ 2] = put_rv30_tpel16_mc20_c;
    c->put_rv30_tpel_pixels_tab[0][ 4] = put_rv30_tpel16_mc01_c;
    c->put_rv30_tpel_pixels_tab[0][ 5] = put_rv30_tpel16_mc11_c;
    c->put_rv30_tpel_pixels_tab[0][ 6] = put_rv30_tpel16_mc21_c;
    c->put_rv30_tpel_pixels_tab[0][ 8] = put_rv30_tpel16_mc02_c;
    c->put_rv30_tpel_pixels_tab[0][ 9] = put_rv30_tpel16_mc12_c;
    c->put_rv30_tpel_pixels_tab[0][10] = put_rv30_tpel16_mc22_c;
    c->avg_rv30_tpel_pixels_tab[0][ 0] = c->avg_h264_qpel_pixels_tab[0][0];
    //c->avg_rv30_tpel_pixels_tab[0][ 0] = avg_rv30_pixels16_mxu;
    c->avg_rv30_tpel_pixels_tab[0][ 1] = avg_rv30_tpel16_mc10_c;
    c->avg_rv30_tpel_pixels_tab[0][ 2] = avg_rv30_tpel16_mc20_c;
    c->avg_rv30_tpel_pixels_tab[0][ 4] = avg_rv30_tpel16_mc01_c;
    c->avg_rv30_tpel_pixels_tab[0][ 5] = avg_rv30_tpel16_mc11_c;
    c->avg_rv30_tpel_pixels_tab[0][ 6] = avg_rv30_tpel16_mc21_c;
    c->avg_rv30_tpel_pixels_tab[0][ 8] = avg_rv30_tpel16_mc02_c;
    c->avg_rv30_tpel_pixels_tab[0][ 9] = avg_rv30_tpel16_mc12_c;
    c->avg_rv30_tpel_pixels_tab[0][10] = avg_rv30_tpel16_mc22_c;
    c->put_rv30_tpel_pixels_tab[1][ 0] = c->put_h264_qpel_pixels_tab[1][0];
    //c->put_rv30_tpel_pixels_tab[1][ 0] = put_rv30_pixels8_mxu;
    c->put_rv30_tpel_pixels_tab[1][ 1] = put_rv30_tpel8_mc10_c;
    c->put_rv30_tpel_pixels_tab[1][ 2] = put_rv30_tpel8_mc20_c;
    c->put_rv30_tpel_pixels_tab[1][ 4] = put_rv30_tpel8_mc01_c;
    c->put_rv30_tpel_pixels_tab[1][ 5] = put_rv30_tpel8_mc11_c;
    c->put_rv30_tpel_pixels_tab[1][ 6] = put_rv30_tpel8_mc21_c;
    c->put_rv30_tpel_pixels_tab[1][ 8] = put_rv30_tpel8_mc02_c;
    c->put_rv30_tpel_pixels_tab[1][ 9] = put_rv30_tpel8_mc12_c;
    c->put_rv30_tpel_pixels_tab[1][10] = put_rv30_tpel8_mc22_c;
    c->avg_rv30_tpel_pixels_tab[1][ 0] = c->avg_h264_qpel_pixels_tab[1][0];
    //c->avg_rv30_tpel_pixels_tab[1][ 0] = avg_rv30_pixels8_mxu;
    c->avg_rv30_tpel_pixels_tab[1][ 1] = avg_rv30_tpel8_mc10_c;
    c->avg_rv30_tpel_pixels_tab[1][ 2] = avg_rv30_tpel8_mc20_c;
    c->avg_rv30_tpel_pixels_tab[1][ 4] = avg_rv30_tpel8_mc01_c;
    c->avg_rv30_tpel_pixels_tab[1][ 5] = avg_rv30_tpel8_mc11_c;
    c->avg_rv30_tpel_pixels_tab[1][ 6] = avg_rv30_tpel8_mc21_c;
    c->avg_rv30_tpel_pixels_tab[1][ 8] = avg_rv30_tpel8_mc02_c;
    c->avg_rv30_tpel_pixels_tab[1][ 9] = avg_rv30_tpel8_mc12_c;
    c->avg_rv30_tpel_pixels_tab[1][10] = avg_rv30_tpel8_mc22_c;
}
