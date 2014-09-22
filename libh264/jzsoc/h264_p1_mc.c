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
 * JZ4760 MC HardWare Core Accelerate
 *
 * $Id: h264_p1_mc.c,v 1.1.1.1 2012/03/27 04:02:56 dqliu Exp $
 *
 ****************************************************************************/

#define EDGE_WIDTH 24  //ensure its same with libavcode/mpegvideo.h

#include "h264_tcsm1.h"
#include "jz4760_mc_hw.h"
//#include "h264_cavlc_sram.h"
#include "../../libjzcommon/jz4760e_2ddma_hw.h"
#include "../../libjzcommon/jz4760e_spu.h"


#define MC_CP_MIN 0
#define MC_CP_MID 1
#define MC_CP_Y 0
#define MC_CP_C 1
#define MC_CP_Cb 1
#define MC_CP_Cr 2
#define MC_CP_LIST0 0
#define MC_CP_LIST1 1
#define H264_MC_PRE_GP_BASE 0
#define H264_MC_DISTRI_GP_BASE 0
#define MC_CP_YBUF_STR (MC_H_YDIS_STR)
#define MC_CP_CBUF_STR (MC_H_CDIS_THRED)
#define MC_MIN(x,y) ((x)>(y)?(y):(x))
#define MC_MAX(x,y) ((y)>(x)?(y):(x))


//retan_pix_mb retan_buf_mb[2][3];
typedef struct retan_pix{
  short int x0;
  short int y0;
  short int xe;
  short int ye;
}retan_pix;
retan_pix retan_buf_mb[2][3];
#ifdef MC_NO_FETED 
#else
retan_pix fetched_retan_mb[2][3];
#endif

unsigned int sram_mc_dist_addr;

unsigned int *gp2_chn_ptr;

const uint32_t MC_CP_BUF[3][2][3] = {
  //Y/////////////////////////////////////////////
  {
    {
      SRAM_PADDR(SRAM_MC_PRE_BUF), 
      SRAM_PADDR(SRAM_MC_PRE_BUF+2*MC_YBUF_SIZE),
      SRAM_PADDR(SRAM_MC_PRE_BUF+4*MC_YBUF_SIZE),
    },
    {
      SRAM_PADDR(SRAM_MC_PRE_BUF+MC_H_YDIS_THRED), 
      SRAM_PADDR(SRAM_MC_PRE_BUF+2*MC_YBUF_SIZE + MC_H_YDIS_THRED),
      SRAM_PADDR(SRAM_MC_PRE_BUF+4*MC_YBUF_SIZE + MC_H_YDIS_THRED),
    },
  },
  //U/////////////////////////////////////////////
  {
    {
      SRAM_PADDR(SRAM_MC_PRE_BUF+6*MC_YBUF_SIZE), 
      SRAM_PADDR(SRAM_MC_PRE_BUF+6*MC_YBUF_SIZE+2*MC_CBUF_SIZE),
      SRAM_PADDR(SRAM_MC_PRE_BUF+6*MC_YBUF_SIZE+4*MC_CBUF_SIZE),
    },
    {
      SRAM_PADDR(SRAM_MC_PRE_BUF+6*MC_YBUF_SIZE+6*MC_CBUF_SIZE), 
      SRAM_PADDR(SRAM_MC_PRE_BUF+6*MC_YBUF_SIZE+8*MC_CBUF_SIZE),
      SRAM_PADDR(SRAM_MC_PRE_BUF+6*MC_YBUF_SIZE+10*MC_CBUF_SIZE),
    },
  },
  //V/////////////////////////////////////////////
  {
    {
      SRAM_PADDR(SRAM_MC_PRE_BUF+6*MC_YBUF_SIZE+MC_H_CDIS_WIDTH), 
      SRAM_PADDR(SRAM_MC_PRE_BUF+6*MC_YBUF_SIZE+2*MC_CBUF_SIZE+MC_H_CDIS_WIDTH),
      SRAM_PADDR(SRAM_MC_PRE_BUF+6*MC_YBUF_SIZE+4*MC_CBUF_SIZE+MC_H_CDIS_WIDTH),
    },
    {
      SRAM_PADDR(SRAM_MC_PRE_BUF+6*MC_YBUF_SIZE+6*MC_CBUF_SIZE+MC_H_CDIS_WIDTH), 
      SRAM_PADDR(SRAM_MC_PRE_BUF+6*MC_YBUF_SIZE+8*MC_CBUF_SIZE+MC_H_CDIS_WIDTH),
      SRAM_PADDR(SRAM_MC_PRE_BUF+6*MC_YBUF_SIZE+10*MC_CBUF_SIZE+MC_H_CDIS_WIDTH),
    },
  },
};

unsigned char REF_YQUEUE[32*2];
unsigned char REF_CQUEUE[32*2];

typedef struct mc_ref_queue{
unsigned int REF_MCC_ADDR[32*2];
}mc_ref_queue;


unsigned int mc_ref_Yqueue_blk[32*2];
int mc_Yqueue_cnt; 
unsigned int mc_ref_Uqueue_blk[32*2];
unsigned int mc_ref_Vqueue_blk[32*2];
int mc_Cqueue_cnt; 

typedef struct gp_chn{
  unsigned int src;
  unsigned int dst;
  unsigned int str;
  unsigned int width_num;
}gp_chn;

typedef struct gp2_mc_dis_pref{
  gp_chn *tcsm_pref_chn_ptr;
  unsigned int *pref_chn_last_ptr;
  unsigned int *sram_pref_chn_ptr;
  //unsigned int *sram_dha_ptr;
  //unsigned int *tcsm1_chn_tail_ptr;
  int valid_num;
}gp2_mc_dis_pref;

gp2_mc_dis_pref *gp2_mc_dis_pref_cur_ptr;

typedef struct mc_hw_chn_nod{
  unsigned int _MCC_YFID;
  unsigned int _MCC_YPID;//         (_MCC_YFID+4)
  unsigned int _MCC_YSF    ;//      (_MCC_YPID+4)
  unsigned int _MCC_YPS[H264_NLEN];//(_MCC_YSF +4)
  unsigned int _MCC_YFS[H264_NLEN];// (_MCC_YPS+H264_NLEN*4)
  unsigned int _MCC_YSN;//          (_MCC_YFS+H264_NLEN*4) 

  unsigned int _MCC_CFID;//         (_MCC_YSN +4) 
  unsigned int _MCC_CPID;//         (_MCC_CFID+4)
  unsigned int _MCC_CSF;//          (_MCC_CPID+4)
  unsigned int _MCC_UPS[H264_NLEN];//          (_MCC_CSF +4)
  unsigned int _MCC_UFS[H264_NLEN];//          (_MCC_UPS+H264_NLEN*4)
  unsigned int _MCC_VPS[H264_NLEN];//          (_MCC_UFS+H264_NLEN*4)
  unsigned int _MCC_VFS[H264_NLEN];//          (_MCC_VPS+H264_NLEN*4)
  unsigned int _MCC_CSN;//          (_MCC_VFS+H264_NLEN*4) 

}mc_hw_chn_nod;

typedef struct mc_hw_chn{
  mc_hw_chn_nod* mc_hw_chn_nod_ptr;
  uint8_t _Wlist0_x[4];
  uint8_t _Wlist1_x[4];    
}mc_hw_chn;
mc_hw_chn *mc_hw_chn_ptr;

extern uint8_t * _Wlist0;
extern uint8_t * _Wlist1;

volatile unsigned int *MCYP, *MCYF, *MCCP, *MCCF;
unsigned int isBIDIR; 

const static uint16_t Yofst_p[] = {0,8,PREVIOUS_LUMA_STRIDE*8,PREVIOUS_LUMA_STRIDE*8+8};
const static uint16_t Cofst_p[] = {0,4,PREVIOUS_CHROMA_STRIDE*4,PREVIOUS_CHROMA_STRIDE*4+4};
const static uint16_t Yofst_f[] = {0,8,FUTURE_LUMA_STRIDE*8,FUTURE_LUMA_STRIDE*8+8};
const static uint16_t Cofst_f[] = {0,4,FUTURE_CHROMA_STRIDE*4,FUTURE_CHROMA_STRIDE*4+4};

#ifdef MC_PMON
unsigned int mbnum, frmnum; 
unsigned int dmar, intpr, pendr, fddr;
FILE *mc_fp;
extern char* filename;
#endif

# define MC_BLK_16X16   0xF
# define MC_BLK_16X8    0xE
# define MC_BLK_8X16    0xB
# define MC_BLK_8X8     0xA
# define MC_BLK_8X4     0x9
# define MC_BLK_4X8     0x6
# define MC_BLK_4X4     0x5
# define MC_BLK_4X2     0x4
# define MC_BLK_2X4     0x1
# define MC_BLK_2X2     0x0

# define FDD_DAT_H264(d1d,cas,ctc,rs,bsize,bid,bpos)\
( FDD_MD_DATA |                                   \ 
  FDD_LK |                                        \
  FDD_AT |                                        \
  (d1d)<<24 |					  \
  (cas)<<20 |					  \
  (ctc)<<12 |                                     \
  (rs)<<16 |					  \
  (bsize)<<8 |                                    \
  (bid)<<4 |                                      \
  (bpos)<<0                                       \  
)

# define FDD2_DAT_H264(d1d,cas,ctc,ypos,xpos,rs,bsize,bid)\
( FDD2_MD_DATA |                                  \ 
  FDD2_LK |                                       \
  FDD2_AT |                                       \
  (d1d)<<26 |					  \
  (cas)<<23 |                                     \
  (ctc)<<20 |                                     \
  (ypos)<<16 |                                    \
  (xpos)<<0 |                                     \  
  (rs)<<12 |					  \
  (bsize)<<8 |                                    \
  (bid)<<4					  \
)

/*MC post process: AVG*/
static void mc_post_avg(H264_MB_Ctrl_DecARGs *dmb,
			const H264_Slice_GlbARGs *SLICE_T,
			const uint8_t *_yp, 
			const uint8_t *_yf,
			const uint8_t bofst)
{
  uint32_t i,j;
  uint8_t *src_yp, *src_yf, *src_up, *src_uf;
  src_yp = _yp + Yofst_p[bofst] - PREVIOUS_LUMA_STRIDE;
  src_yf = _yf + Yofst_f[bofst] - FUTURE_LUMA_STRIDE;
  src_up = _yp + PREVIOUS_OFFSET_U + Cofst_p[bofst] - PREVIOUS_CHROMA_STRIDE;
  src_uf = _yf + FUTURE_OFFSET_U + Cofst_f[bofst] - FUTURE_CHROMA_STRIDE;
  /*Y*/
  for(i=0;i<8;i++){
    S32LDI(xr1, src_yp, PREVIOUS_LUMA_STRIDE);
    S32LDI(xr2, src_yf, FUTURE_LUMA_STRIDE);
    S32LDD(xr3, src_yp, 4);
    S32LDD(xr4, src_yf, 4);
    Q8AVGR(xr11, xr1, xr2);
    S32STD(xr11, src_yp, 0);
    Q8AVGR(xr13, xr3, xr4);
    S32STD(xr13, src_yp, 4);
  }
  /*UV*/
  for(i=0;i<4;i++){
    S32LDI(xr1, src_up, PREVIOUS_CHROMA_STRIDE);
    S32LDI(xr2, src_uf, FUTURE_CHROMA_STRIDE);    //UF
    S32LDD(xr3, src_up, PREVIOUS_C_LEN);  //VP
    S32LDD(xr4, src_uf, FUTURE_OFFSET_U2V);  //VF
    Q8AVGR(xr11, xr1, xr2);
    S32STD(xr11, src_up, 0);
    Q8AVGR(xr13, xr3, xr4);
    S32STD(xr13, src_up, PREVIOUS_C_LEN);
  }
}

#define cfg_gp2_chn(src, dst,src_strd, dst_strd, w, num)\
  ({\
  *gp2_chn_ptr++ = get_p1_phy_addr(src);\
  *gp2_chn_ptr++ = (dst);					\
  *gp2_chn_ptr++ = GP_STRD(src_strd,GP_FRM_OPT,dst_strd);\
  *gp2_chn_ptr++ = GP_UNIT(GP_TAG_LK,w,num);\
  gp2_mc_dis_pref_cur_ptr->valid_num ++;\
  if ( unlikely(gp2_mc_dis_pref_cur_ptr->valid_num == DDMA_GP2_DIS_CHN_NOD_NUM)){\
    gp2_chn_ptr = gp2_mc_dis_pref_cur_ptr->sram_pref_chn_ptr;\
  }  \
})

void blk_copy_word(unsigned char *src, unsigned char *dst,
		     unsigned int src_strd 
		   )
{
  D16MUL_HW(xr0,xr7,xr7,xr8);
  //w's high 16 bits will be masked in GP_STRD and GP_UNIT
  //so w&0xffff is not needed
  int w = S32M2I(xr7);
  unsigned int num = S32M2I(xr8);
  cfg_gp2_chn(src, dst, src_strd, w, w, num);
  sram_mc_dist_addr = sram_mc_dist_addr + num;  
}


#define  fetch_new_tangle(need_fetch_retan,src,dst, src_linesize, dst_linesize)\
({									\
      int fet_w = need_fetch_retan->xe - need_fetch_retan->x0 ;\
      int fet_h = need_fetch_retan->ye - need_fetch_retan->y0 ;\
      cfg_gp2_chn(get_p1_phy_addr((src + need_fetch_retan->y0 * src_linesize + need_fetch_retan->x0)),\
		  (dst),\
		  src_linesize, \
		  dst_linesize, \
                  fet_w,\
                  (fet_w*fet_h)\
		  );\
})


void hl_motion_fetch_retangle(H264_Slice_GlbARGs *SLICE_T, int color_sel){
/*----------------------- configure GP for prefetch ---------------*/  
#if 0
  int ref_cnt;
  retan_pix *retan_buf_mb_ptr;
  for ( ref_cnt = 0; ref_cnt < 3; ref_cnt ++)
    {
      retan_buf_mb_ptr = &retan_buf_mb[color_sel][ref_cnt];
      if ( retan_buf_mb_ptr->x0 == 9999
	   ) {
	continue;
      }
  int ref_real;
  int list;
  ref_real = ref_cnt <2? ref_cnt: 0;
  list = ref_cnt <2? 0: 1;
  H264_Frame_Ptr *ref= &SLICE_T->ref_list[list][ ref_real ];

  if ( color_sel == MC_CP_Y){
    retan_buf_mb_ptr->xe = (retan_buf_mb_ptr->xe + 3) & 0xFFFFFFFC;
    fetch_new_tangle(retan_buf_mb_ptr,
		   ref->y_ptr,
		   MC_CP_BUF[MC_CP_Y][mc_Ybuf_index][ref_cnt],
		   SLICE_T->linesize,
		   MC_CP_YBUF_STR
		   );
  }else{
    retan_buf_mb_ptr = &retan_buf_mb[MC_CP_C][ref_cnt];
    
    retan_buf_mb_ptr->xe = (retan_buf_mb_ptr->xe + 3) & 0xFFFFFFFC;
    
    fetch_new_tangle( retan_buf_mb_ptr,
		    ref->u_ptr,
		    MC_CP_BUF[MC_CP_Cb][mc_Cbuf_index][ref_cnt],
		    SLICE_T->uvlinesize,
		    MC_CP_CBUF_STR
		    );

    fetch_new_tangle( retan_buf_mb_ptr,
		      ref->v_ptr,
		      MC_CP_BUF[MC_CP_Cr][mc_Cbuf_index][ref_cnt],
		      SLICE_T->uvlinesize,
		      MC_CP_CBUF_STR
		      );
  }
    }
#endif
}

/*MC post process: Weight 1*/
static void mc_post_weight1(H264_MB_Ctrl_DecARGs *dmb,
			    const H264_Slice_GlbARGs *SLICE_T,
			    const uint8_t *_yp, 
			    const uint8_t *_yf,
			    const uint8_t bofst)
{
#if 0
  /*FIXME, yzhai, 2009-10-21: to be continued!*/
  uint32_t i,j;
  uint8_t *src_yp, *src_yf, *src_up, *src_uf;
  int8_t * ref0_base=(int8_t *)((uint32_t)dmb+sizeof(struct H264_MB_Ctrl_DecARGs)+4);
  int8_t wcoef0[4], wcoef1[4]; /*{y_w0,u_w0,v_w0,N/C,y_w1,u_w1,v_w1,N/C}*/
  int16_t wofst[3];

  int refn0 = ref0_base[bofst];
  int refn1 = ref0_base[8+bofst];
  int y_denom, c_denom;

  *(int32_t *)wcoef1 = 0x0;

  if(_Wlist0[bofst] && _Wlist1[bofst]){
    /*Y*/
    wcoef0[0]    = SLICE_T->luma_weight[0][refn0];
    wcoef1[0]    = SLICE_T->luma_weight[1][refn1];
    wofst[0]     = SLICE_T->luma_offset[0][refn0] + SLICE_T->luma_offset[1][refn1];
    wofst[0]     = ((wofst[0] + 1) | 1) << SLICE_T->luma_log2_weight_denom;
    /*U*/
    wcoef0[1]    = SLICE_T->chroma_weight[0][refn0][0];
    wcoef1[1]    = SLICE_T->chroma_weight[1][refn1][0];
    wofst[1]     = SLICE_T->chroma_offset[0][refn0][0] + SLICE_T->chroma_offset[1][refn1][0];
    wofst[1]     = ((wofst[1] + 1) | 1) << SLICE_T->chroma_log2_weight_denom;
    /*V*/
    wcoef0[2]    = SLICE_T->chroma_weight[0][refn0][1];
    wcoef1[2]    = SLICE_T->chroma_weight[1][refn1][1];
    wofst[2]     = SLICE_T->chroma_offset[0][refn0][1] + SLICE_T->chroma_offset[1][refn1][1];
    wofst[2]     = ((wofst[2] + 1) | 1) << SLICE_T->chroma_log2_weight_denom;
    y_denom      = SLICE_T->luma_log2_weight_denom - 1;
    c_denom      = SLICE_T->chroma_log2_weight_denom - 1;
  } else if(_Wlist0[bofst]){
    /*Y*/
    wcoef0[0]    = SLICE_T->luma_weight[0][refn0];
    wofst[0]     = SLICE_T->luma_offset[0][refn0]<<SLICE_T->luma_log2_weight_denom;
    if(SLICE_T->luma_log2_weight_denom)
      wofst[0]  += (1<<(SLICE_T->luma_log2_weight_denom-1));
    /*U*/
    wcoef0[1]    = SLICE_T->chroma_weight[0][refn0][0];
    wofst[1]     = SLICE_T->chroma_offset[0][refn0][0]<<SLICE_T->chroma_log2_weight_denom;
    /*V*/
    wcoef0[2]    = SLICE_T->chroma_weight[0][refn0][1];
    wofst[2]     = SLICE_T->chroma_offset[0][refn0][1]<<SLICE_T->chroma_log2_weight_denom;
    if(SLICE_T->chroma_log2_weight_denom){
      wofst[1]  += 1<<(SLICE_T->chroma_log2_weight_denom-1);
      wofst[2]  += 1<<(SLICE_T->chroma_log2_weight_denom-1);
    }
    y_denom      = SLICE_T->luma_log2_weight_denom;
    c_denom      = SLICE_T->chroma_log2_weight_denom;
  } else {
    /*Y*/
    wcoef0[0]    = SLICE_T->luma_weight[1][refn1];
    wofst[0]     = SLICE_T->luma_offset[1][refn1]<<SLICE_T->luma_log2_weight_denom;
    if(SLICE_T->luma_log2_weight_denom)
      wofst[0]  += (1<<(SLICE_T->luma_log2_weight_denom-1));
    /*U*/
    wcoef0[1]    = SLICE_T->chroma_weight[1][refn1][0];
    wofst[1]     = SLICE_T->chroma_offset[1][refn1][0]<<SLICE_T->chroma_log2_weight_denom;
    /*V*/
    wcoef0[2]    = SLICE_T->chroma_weight[1][refn1][1];
    wofst[2]     = SLICE_T->chroma_offset[1][refn1][1]<<SLICE_T->chroma_log2_weight_denom;
    if(SLICE_T->chroma_log2_weight_denom){
      wofst[1]  += 1<<(SLICE_T->chroma_log2_weight_denom-1);
      wofst[2]  += 1<<(SLICE_T->chroma_log2_weight_denom-1);
    }
    y_denom      = SLICE_T->luma_log2_weight_denom;
    c_denom      = SLICE_T->chroma_log2_weight_denom;
  }

  src_yp = _yp + Yofst_p[bofst] - PREVIOUS_LUMA_STRIDE;
  src_yf = _yf + Yofst_f[bofst] - FUTURE_LUMA_STRIDE;

  /*Y*/
  S8LDD(xr6,&wcoef0[0],0,7);   //W0
  S8LDD(xr7,&wcoef1[0],0,7);   //W1
  S16LDD(xr8,&wofst[0],0,3);   //offset
  for(i=0;i<8;i++){
    S32LDI(xr1, src_yp, PREVIOUS_LUMA_STRIDE);
    S32LDI(xr2, src_yf, FUTURE_LUMA_STRIDE);
    S32LDD(xr3, src_yp, 4);
    S32LDD(xr4, src_yf, 4);

    Q8MULSU(xr12,xr6,xr1,xr11);
    Q8MACSU_AA(xr12,xr7,xr2,xr11);          
    Q16ACCM_AA(xr12,xr8,xr8,xr11);

    Q8MULSU(xr14,xr6,xr3,xr13);
    Q8MACSU_AA(xr14,xr7,xr4,xr13);          
    Q16ACCM_AA(xr14,xr8,xr8,xr13);

    Q16SARV(xr12,xr11,y_denom);
    Q16SAT(xr1,xr12,xr11);

    Q16SARV(xr14,xr13,y_denom);
    Q16SAT(xr2,xr14,xr13);

    S32STD(xr1, src_yp, 0);
    S32STD(xr2, src_yp, 4);
  }

  /*UV*/
  if((_Wlist0[bofst] && _Wlist1[bofst]) || SLICE_T->use_weight_chroma){
    src_up = _yp + PREVIOUS_OFFSET_U + Cofst_p[bofst] - PREVIOUS_CHROMA_STRIDE;
    src_uf = _yf + FUTURE_OFFSET_U + Cofst_f[bofst] - FUTURE_CHROMA_STRIDE;
    S8LDD(xr6,&wcoef0[1],0,7);   //U.W0
    S8LDD(xr7,&wcoef1[1],0,7);   //U.W1
    S16LDD(xr8,&wofst[1],0,3);   //U.offset
    S8LDD(xr9,&wcoef0[2],0,7);   //V.W0
    S8LDD(xr10,&wcoef1[2],0,7);  //V.W1
    S16LDD(xr15,&wofst[2],0,3);  //V.offset
    for(i=0;i<4;i++){
      S32LDI(xr1, src_up, PREVIOUS_CHROMA_STRIDE);       //UP
      S32LDI(xr2, src_uf, FUTURE_CHROMA_STRIDE);    //UF
      S32LDD(xr3, src_up, PREVIOUS_C_LEN);  //VP
      S32LDD(xr4, src_uf, FUTURE_OFFSET_U2V);  //VF
	
      Q8MULSU(xr12,xr6,xr1,xr11);
      Q8MACSU_AA(xr12,xr7,xr2,xr11);          
      Q16ACCM_AA(xr12,xr8,xr8,xr11);

      Q8MULSU(xr14,xr9,xr3,xr13);
      Q8MACSU_AA(xr14,xr10,xr4,xr13);          
      Q16ACCM_AA(xr14,xr15,xr15,xr13);
	
      Q16SARV(xr12,xr11,c_denom);
      Q16SAT(xr1,xr12,xr11);
	
      Q16SARV(xr14,xr13,c_denom);
      Q16SAT(xr2,xr14,xr13);
      
      S32STD(xr1, src_up, 0);
      S32STD(xr2, src_up, PREVIOUS_C_LEN);
    }
  }
#endif
}

/*MC post process: Weight 2*/
static void mc_post_weight2(H264_MB_Ctrl_DecARGs *dmb,
			    const H264_Slice_GlbARGs *SLICE_T,
			    const uint8_t *_yp, 
			    const uint8_t *_yf,
			    const uint8_t bofst)
{
#if 0
  uint32_t i,j;
  uint8_t *src_yp, *src_yf, *src_up, *src_uf;
  int8_t * ref0_base=(int8_t *)((uint32_t)dmb+sizeof(struct H264_MB_Ctrl_DecARGs)+4);

  int refn0 = ref0_base[bofst];
  int refn1 = ref0_base[8+bofst];
  
  S32I2M(xr8, 0x200020);
  S32LUI(xr15,64,7);
  S8LDD(xr6, &SLICE_T->implicit_weight[refn0][refn1], 0, 7); //W0
  Q8ADD_SS(xr7,xr15,xr6);

  src_yp = _yp + Yofst_p[bofst] - PREVIOUS_LUMA_STRIDE;
  src_yf = _yf + Yofst_f[bofst] - FUTURE_LUMA_STRIDE;
  src_up = _yp + PREVIOUS_OFFSET_U + Cofst_p[bofst] - PREVIOUS_CHROMA_STRIDE;
  src_uf = _yf + FUTURE_OFFSET_U + Cofst_f[bofst] - FUTURE_CHROMA_STRIDE;
  /*Y*/
  for(i=0;i<8;i++){
    S32LDI(xr1, src_yp, PREVIOUS_LUMA_STRIDE);
    S32LDI(xr2, src_yf, FUTURE_LUMA_STRIDE);
    S32LDD(xr3, src_yp, 4);
    S32LDD(xr4, src_yf, 4);
      
    Q8MULSU(xr12,xr6,xr1,xr11);
    Q8MACSU_AA(xr12,xr7,xr2,xr11);          
    Q16ACCM_AA(xr12,xr8,xr8,xr11);
    
    Q8MULSU(xr14,xr6,xr3,xr13);
    Q8MACSU_AA(xr14,xr7,xr4,xr13);          
    Q16ACCM_AA(xr14,xr8,xr8,xr13);
    
    Q16SARV(xr12,xr11,6);
    Q16SAT(xr1,xr12,xr11);
    
    Q16SARV(xr14,xr13,6);
    Q16SAT(xr2,xr14,xr13);
      
    S32STD(xr1, src_yp, 0);
    S32STD(xr2, src_yp, 4);
  }
  /*UV*/
  for(i=0;i<4;i++){
    S32LDI(xr1, src_up, PREVIOUS_CHROMA_STRIDE);       //UP
    S32LDI(xr2, src_uf, FUTURE_CHROMA_STRIDE);    //UF
    S32LDD(xr3, src_up, PREVIOUS_C_LEN);  //VP
    S32LDD(xr4, src_uf, FUTURE_OFFSET_U2V);  //VF
    
    Q8MULSU(xr12,xr6,xr1,xr11);
    Q8MACSU_AA(xr12,xr7,xr2,xr11);          
    Q16ACCM_AA(xr12,xr8,xr8,xr11);
    
    Q8MULSU(xr14,xr6,xr3,xr13);
    Q8MACSU_AA(xr14,xr7,xr4,xr13);          
    Q16ACCM_AA(xr14,xr8,xr8,xr13);
    
    Q16SARV(xr12,xr11,6);
    Q16SAT(xr1,xr12,xr11);
    
    Q16SARV(xr14,xr13,6);
    Q16SAT(xr2,xr14,xr13);
    
    S32STD(xr1, src_up, 0);
    S32STD(xr2, src_up, PREVIOUS_C_LEN);
  }
#endif
}

typedef void (BLK_POSTP_PTR)(H264_MB_Ctrl_DecARGs *dmb,
			    const H264_Slice_GlbARGs *SLICE_T,
			    const uint8_t *_yp, 
			    const uint8_t *_yf,
			    const uint8_t bofst);

BLK_POSTP_PTR *BLK_POSTP[] = {
  mc_post_avg,
  mc_post_weight1,
  mc_post_weight2,
};


static void mc_part_hw(H264_Slice_GlbARGs *SLICE_T, int8_t *Inter_Dec_base,
		       int mv_n, int i8x8,
		       int xy_offset,
		       int blkwh,
		       int bid, int bsize,
		       int cbsize,
		       int list0, int list1){
#if 0

    int mv_num=*(uint32_t*)Inter_Dec_base;
    int8_t (*ref_base)[8]=Inter_Dec_base+4;
    short (*mv_base)[2]=Inter_Dec_base+4+4+((32+8)<<(SLICE_T->slice_type-2));

    uint32_t *fdd  = (uint32_t *)MCYP;
    uint32_t *fdd2 = (uint32_t *)MCCP; 

    S32I2M(xr15,blkwh);

    int start_list=1-(!!list0);
    int end_list=1+(!!list1);
    int list;

    for(list=start_list;list<end_list;list++){
      int is_list1_of_bidir=list0&&list;
      i_movn(fdd,MCYF,is_list1_of_bidir);
      i_movn(fdd2,MCCF,is_list1_of_bidir);
      i_movn(isBIDIR,1,is_list1_of_bidir);
      i_movn(mv_base,mv_base+mv_num,list);

      int ref_cnt= ref_base[list][i8x8];
      int ref_hit=!ref_cnt;
      i_movz(ref_hit,1,list||(ref_cnt-1));
      H264_Frame_Ptr *ref= &SLICE_T->ref_list[list][ ref_cnt ];
      i_movn(ref_cnt,2,list);
#if 0
      int mx= mv_base[mv_n][0] + x_offset*8;
      int my= mv_base[mv_n][1] + y_offset*8;

      const int full_mx= mx>>2;
      const int full_my= my>>2;

      if(full_mx < -EDGE_WIDTH + 2){
	mx += (-EDGE_WIDTH+2-full_mx)<<2;
      }
      if(full_mx + 16 > pic_width + EDGE_WIDTH - 3){
	mx -= (full_mx+16-pic_width-EDGE_WIDTH+3)<<2;
      }
      if(full_my < -EDGE_WIDTH + 2){
	my += (-EDGE_WIDTH+2-full_my)<<2;
      }
      if(full_my + 16 > pic_height + EDGE_WIDTH - 3){
	my -= (full_my+16-pic_height-EDGE_WIDTH+3)<<2;
      }
      const int luma_xy= (mx&3) + ((my&3)<<2);
      int chroma_copy = (((mx&7) + (my&7)) == 0)? 0 : 1;
      int cur_x0, cur_y0, cur_xe, cur_ye;
      int cur_x0_mc;
      int rectan_add_condi;
      retan_pix *retan_buf_mb_ptr;

      cur_x0 = (mx>>2) - tap6_pre_skirt[mx&3];
      cur_y0 = (my>>2) - tap6_pre_skirt[my&3];
      
      cur_xe = blkw + tap6_skirt[mx&3] + cur_x0;
      cur_ye = blkh + tap6_skirt[my&3] + cur_y0;

      cur_x0_mc = cur_x0;
      
      cur_x0 = cur_x0 & 0xFFFFFFFC;

      retan_buf_mb_ptr = &retan_buf_mb[MC_CP_Y][ref_cnt];
      
      rectan_add_condi = (((retan_buf_mb_ptr->x0 == 9999
			    && retan_buf_mb_ptr->y0 == 9999) ||
			   (abs(cur_x0 - retan_buf_mb_ptr->xe) <=  MC_H_YDIS_THRED
			    && abs(cur_xe - retan_buf_mb_ptr->x0) <=  MC_H_YDIS_THRED
			    && abs(cur_y0 - retan_buf_mb_ptr->ye) <=  MC_V_YDIS_THRED
			    && abs(cur_ye - retan_buf_mb_ptr->y0) <=  MC_V_YDIS_THRED
			    )) && 
			  ref_hit);

      if ( likely(rectan_add_condi == 1)){
	fdd[1] = cur_y0 << 16 | (cur_x0_mc & 0xFFFF);
	REF_YQUEUE[mc_Yqueue_cnt] = ref_cnt;// char
	mc_ref_Yqueue_blk[mc_Yqueue_cnt] = &fdd[1];
	mc_Yqueue_cnt++;

	retan_buf_mb_ptr->x0 = MC_MIN(cur_x0, retan_buf_mb_ptr->x0);
	retan_buf_mb_ptr->y0 = MC_MIN(cur_y0, retan_buf_mb_ptr->y0);
	
	retan_buf_mb_ptr->xe=MC_MAX(cur_xe,retan_buf_mb_ptr->xe);
	retan_buf_mb_ptr->ye = MC_MAX(cur_ye, retan_buf_mb_ptr->ye);

	fdd[0] = FDD_DAT_H264(0,(REG_REFA>>2),0,5,bsize,bid,luma_xy);
      } else {
	uint8_t * src_y = ref->y_ptr + (mx>>2) + (my>>2)*SLICE_T->linesize;
	src_y -= tap6_pre_skirt[my&3]*SLICE_T->linesize + tap6_pre_skirt[mx&3];
	int fetch_w = blkw + (((mx&3)!=0)? 8 : ((((unsigned int)src_y & 0x3)!= 0)*4));
	fdd[1] = SRAM_PADDR((sram_mc_dist_addr) | ((unsigned int)src_y & 0x3));
	blk_copy_word((unsigned int)src_y & 0xFFFFFFFC,
		      (uint8_t *)(sram_mc_dist_addr),
		      SLICE_T->linesize,
		      blkh+tap6_skirt[my&3],
		      fetch_w);
	fdd[0] = FDD_DAT_H264(1,(REG_REFA>>2),0,0,bsize,bid,luma_xy);
      }

      int blkw_chroma = blkw>>1;
      int blkh_chroma = blkh>>1;
      retan_buf_mb_ptr = &retan_buf_mb[MC_CP_C][ref_cnt];

      cur_x0 = (mx>>3);
      cur_y0 = (my>>3);
      
      cur_xe = cur_x0 + blkw_chroma + chroma_copy;
      cur_ye = cur_y0 + blkh_chroma + chroma_copy;

      cur_x0_mc = cur_x0;
      cur_x0 = cur_x0 & 0xFFFFFFFC;

      rectan_add_condi = (((retan_buf_mb_ptr->x0 == 9999
			    && retan_buf_mb_ptr->y0 == 9999) ||
			   (abs(cur_x0 - retan_buf_mb_ptr->xe) <=  MC_H_CDIS_WIDTH
			    && abs(cur_xe - retan_buf_mb_ptr->x0) <=  MC_H_CDIS_WIDTH
			    && abs(cur_y0 - retan_buf_mb_ptr->ye) <=  MC_V_CDIS_THRED
			    && abs(cur_ye - retan_buf_mb_ptr->y0) <=  MC_V_CDIS_THRED
			    )) && 
			  ref_hit);

      if (likely(rectan_add_condi == 1)){
	fdd2[1] = (cur_x0_mc & 0xFFFF) | (cur_y0 << 16);
	mc_ref_Uqueue_blk[mc_Cqueue_cnt] = &fdd2[1];

	retan_buf_mb_ptr->x0 = MC_MIN(cur_x0, retan_buf_mb_ptr->x0);
	retan_buf_mb_ptr->y0 = MC_MIN(cur_y0, retan_buf_mb_ptr->y0);
      
	retan_buf_mb_ptr->xe=MC_MAX(cur_xe,retan_buf_mb_ptr->xe);
	retan_buf_mb_ptr->ye = MC_MAX(cur_ye, retan_buf_mb_ptr->ye);

	fdd2[0] = FDD2_DAT_H264(0,(REG2_REFA>>2),0,(my&7),(mx&7),3,cbsize,bid);
      } else {
	uint8_t * src_cb= ref->u_ptr + (mx>>3) + (my>>3)*SLICE_T->uvlinesize;
	int fetch_w = ( ((((unsigned int)src_cb & 0x3) + blkw_chroma -1 + chroma_copy) | 0x3 ) + 1);
	fdd2[1] = SRAM_PADDR((sram_mc_dist_addr) | ((unsigned int)src_cb & 0x3));
	blk_copy_word((unsigned int)src_cb & 0xFFFFFFFC,
		      (uint8_t *)(sram_mc_dist_addr),
		      SLICE_T->uvlinesize,
		      blkh_chroma+chroma_copy,
		      fetch_w);
	fdd2[0] = FDD2_DAT_H264(1,(REG2_REFA>>2),0,(my&7),(mx&7),0,cbsize,bid);
      }

      if (likely(rectan_add_condi == 1)){
	fdd2[H264_NLEN*2+1] = (cur_x0_mc & 0xFFFF) | (cur_y0 << 16);
	REF_CQUEUE[mc_Cqueue_cnt] = ref_cnt;// char
	mc_ref_Vqueue_blk[mc_Cqueue_cnt] = &fdd2[H264_NLEN*2+1];
	mc_Cqueue_cnt ++ ;
	fdd2[H264_NLEN*2+0] = FDD2_DAT_H264(0,(REG2_REFA>>2),0,(my&7),(mx&7),3,cbsize,bid);
      } else {
	uint8_t * src_cr= ref->v_ptr + (mx>>3) + (my>>3)*SLICE_T->uvlinesize;
	int fetch_w = ( ((((unsigned int)src_cr & 0x3) + blkw_chroma -1 + chroma_copy) | 0x3 ) + 1);
	fdd2[H264_NLEN*2+1] = SRAM_PADDR((sram_mc_dist_addr)|((unsigned int)src_cr&0x3));
	blk_copy_word((unsigned int)src_cr & 0xFFFFFFFC,
		      (uint8_t *)(sram_mc_dist_addr),
		      SLICE_T->uvlinesize,
		      blkh_chroma+chroma_copy,
		      fetch_w);
	fdd2[H264_NLEN*2+0] = FDD2_DAT_H264(1,(REG2_REFA>>2),0,(my&7),(mx&7),0,cbsize,bid);
      }

      if(is_list1_of_bidir){
	MCYF+=2;
	MCCF+=2;
      } else {
	MCYP+=2;
	MCCP+=2;
      }

#else
      S32LDDV(xr4,mv_base,mv_n,2);
      S32I2M(xr5,xy_offset);
      Q16ACC_AA(xr4,xr3,xr5,xr0);//mv+xy_ofst+mb_xy,xr4={my,mx}
      Q16SAR(xr5,xr4,xr4,xr6,2);//xr5,xr6={full_my,full_mx};
      Q16ACCM_SS(xr5,xr1,xr2,xr6);//xr5=full_mxy-(-EDGE_WIDTH + 2);xr6=full_mxy+16-(pic_wh + EDGE_WIDTH - 3);
      D16SLT(xr7,xr5,xr0);//xr7=full_mxy<(-EDGE_WIDTH + 2)
      D16SLT(xr8,xr0,xr6);//xr8=full_mxy+16>(pic_wh + EDGE_WIDTH - 3)
      Q16SLL(xr5,xr5,xr6,xr6,2);//xr5=(full_mxy-(-EDGE_WIDTH + 2))<<2;xr6=(full_mxy+16-(pic_wh + EDGE_WIDTH - 3))<<2;
      D16MOVZ(xr5,xr7,xr0);//if(full_mxy < -EDGE_WIDTH + 2){xr5=xr5;}else{xr5=0;}
      D16MOVZ(xr6,xr8,xr0);//if(full_mxy+16>(pic_wh + EDGE_WIDTH - 3)){xr6=xr6;}else{xr6=0;}
      Q16ACCM_SS(xr4,xr5,xr0,xr0);//xr4=mxy+((-EDGE_WIDTH + 2-full_mxy)<<2);
      Q16ACCM_SS(xr4,xr6,xr0,xr0);//xr4=mxy-(full_mxy+16-pic_height-EDGE_WIDTH+3)<<2;
      S32AND(xr8,xr13,xr4);//xr8=mxy&3
      S32SFL(xr9,xr0,xr8,xr10,3);//xr9=my&3,xr10=mx&3;
      D32SLL(xr9,xr9,xr0,xr0,2);//xr9=((my&3)<<2);
      S32OR(xr9,xr9,xr10);//xr9=luma_xy=(mx&3) + ((my&3)<<2);
      const int luma_xy= S32M2I(xr9);

      S32I2M(xr7,0x20002);
      Q16ADD_AA_WW(xr5,xr7,xr13,xr0);//xr5=0x50005
      Q16ADD_AA_WW(xr12,xr7,xr5,xr0);//xr12=0x70007,used for chroma_copy
      S32AND(xr12,xr12,xr4);//xr12={my&7,mx&7}
      D16MOVZ(xr7,xr8,xr0);//tap6_pre_skirt
      D16MOVZ(xr5,xr8,xr0);//tap6_skirt
      Q16SAR(xr4,xr4,xr4,xr6,2);//xr4=xr6=mxy>>2. In chroma, blk>>1 and mxy>>3, so xr4>>2 here for reducing insn
      Q16ACCM_SS(xr6,xr7,xr0,xr0);//(mxy>>2) - tap6_pre_skirt;xr6={cur_y0,cur_x0}
      Q16ACC_AA(xr5,xr15,xr6,xr0);//blkwh+tap6_skirt+cur_xy0;xr5={cur_ye,cur_xe}
      S32AND(xr9,xr6,xr14);//xr9={cur_y0,cur_x0&0xfffc}

      int cur_x0, cur_y0, cur_xe, cur_ye;
      int cur_x0_mc;
      int rectan_add_condi;
      retan_pix *retan_buf_mb_ptr;

      retan_buf_mb_ptr = &retan_buf_mb[MC_CP_Y][ref_cnt];

      S32LDD(xr7,retan_buf_mb_ptr,4);//xr7={retan_buf_mb_ptr->ye,retan_buf_mb_ptr->xe}
      S32I2M(xr10,(MC_V_YDIS_THRED<<16)|MC_H_YDIS_THRED);//xr10=HV_YDIS_THRED
      Q16ADD_SS_WW(xr8,xr9,xr7,xr0);//xr8=cur_xy0 - retan_buf_mb_ptr->xye
      D16CPS(xr8,xr8,xr8);//abs(xr8)
      D16SLT(xr8,xr10,xr8);//xr8=HV_YDIS_THRED<abs(cur_xy0 - retan_buf_mb_ptr->xye)
      S32LDD(xr11,retan_buf_mb_ptr,0);//xr11={retan_buf_mb_ptr->y0,retan_buf_mb_ptr->x0}
      Q16ADD_SS_WW(xr11,xr5,xr11,xr0);//xr11=cur_xye - retan_buf_mb_ptr->xy0
      D16CPS(xr11,xr11,xr11);//abs(xr11)
      D16SLT(xr11,xr10,xr11);//xr11=HV_YDIS_THRED<abs(cur_xye - retan_buf_mb_ptr->xy0)
      S32OR(xr8,xr8,xr11);//xr8==1 means cross the boundry
      rectan_add_condi = (ref_hit && 
			  ((retan_buf_mb_ptr->x0 == 9999) || !S32M2I(xr8)));

      if ( likely(rectan_add_condi == 1)){
	fdd[1] = S32M2I(xr6);
	REF_YQUEUE[mc_Yqueue_cnt] = ref_cnt;// char
	mc_ref_Yqueue_blk[mc_Yqueue_cnt] = &fdd[1];
	mc_Yqueue_cnt++;

	S32LDD(xr11,retan_buf_mb_ptr,0);//xr11={retan_buf_mb_ptr->y0,retan_buf_mb_ptr->x0}
	D16MIN(xr9,xr9,xr11);//xr9=MC_MIN(cur_xy0, retan_buf_mb_ptr->xy0);
	D16MAX(xr5,xr5,xr7);//MC_MAX(cur_xye, retan_buf_mb_ptr->xye);
	S32STD(xr9,retan_buf_mb_ptr,0);
	S32STD(xr5,retan_buf_mb_ptr,4);

	fdd[0] = FDD_DAT_H264(0,(REG_REFA>>2),0,(MC_H_YDIS_STR>>4),bsize,bid,luma_xy);
      } else {
	//xr6={(my>>2)-tap6_pre_skirt[my&3],
	//     (mx>>2)-tap6_pre_skirt[mx&3];}
	S32I2M(xr10,SLICE_T->linesize);//xr10=SLICE_T->linesize
	D16MUL_XW(xr0,xr6,xr10,xr8);//xr8=((my>>2)-tap6_pre_skirt[my&3])*SLICE_T->linesize
	S32SFL(xr0,xr0,xr6,xr7,3);//xr7={16'b0,(mx>>2)-tap6_pre_skirt[mx&3]};
	D16ASUM_AA(xr8,xr7,xr0,xr0);//xr8=signed_ext(xr7[15:0])+xr8;
	uint8_t * src_y = ref->y_ptr + S32M2I(xr8);
	fdd[1] = ((sram_mc_dist_addr) | ((unsigned int)src_y & 0x3));
	S32NOR(xr7,xr14,xr0);//xr7=0x3
	Q16ACC_SS(xr7,xr5,xr9,xr0);//xr7={cur_ye-cur_y0,cur_xe-cur_x0+3}
	S32AND(xr7,xr14,xr7);//xr7&=0xFFFFFFFC,word_align fetch_w
	//xr7 will be a para of blk_copy_word
	//xr8 will be used in blk_copy_word
	blk_copy_word((unsigned int)src_y & 0xFFFFFFFC,
		      (uint8_t *)(sram_mc_dist_addr),
		      SLICE_T->linesize
		      );
	fdd[0] = FDD_DAT_H264(1,(REG_REFA>>2),0,0,bsize,bid,luma_xy);
      }

      const int mxy_frac=S32M2I(xr12);
      S32SLT(xr12,xr0,xr12);//xr12=((mx|my)&7!=0)
      S32SFL(xr0,xr12,xr12,xr12,3);//xr12=0x?000?,?=chroma_copy
      Q16SAR(xr5,xr15,xr4,xr6,1);//xr5=blkwh>>1,xr6=cur_xy0_mc=mxy>>3
      Q16ACC_AA(xr5,xr6,xr12,xr0);//xr5=xye
      S32AND(xr4,xr6,xr14);//xr4={cur_y0,cur_x0&0xfffc}

      retan_buf_mb_ptr = &retan_buf_mb[MC_CP_C][ref_cnt];

      S32LDD(xr9,retan_buf_mb_ptr,0);//xr9={retan_buf_mb_ptr->y0,retan_buf_mb_ptr->x0}
      S32LDD(xr7,retan_buf_mb_ptr,4);//xr7={retan_buf_mb_ptr->ye,retan_buf_mb_ptr->xe}
      S32I2M(xr10,(MC_V_CDIS_THRED<<16)|MC_H_CDIS_WIDTH/*MC_H_CDIS_THRED*/);//xr10=HV_YDIS_THRED
      Q16ADD_SS_WW(xr8,xr4,xr7,xr0);//xr8=cur_xy0 - retan_buf_mb_ptr->xye
      D16CPS(xr8,xr8,xr8);//abs(xr8)
      D16SLT(xr8,xr10,xr8);//xr8=HV_YDIS_THRED<abs(cur_xy0 - retan_buf_mb_ptr->xye)
      Q16ADD_SS_WW(xr11,xr5,xr9,xr0);//xr11=cur_xye - retan_buf_mb_ptr->xy0
      D16CPS(xr11,xr11,xr11);//abs(xr11)
      D16SLT(xr11,xr10,xr11);//xr11=HV_YDIS_THRED<abs(cur_xye - retan_buf_mb_ptr->xy0)
      S32OR(xr8,xr8,xr11);//xr8==1 means cross the boundry
      rectan_add_condi = (ref_hit && 
			  ((retan_buf_mb_ptr->x0 == 9999) || !S32M2I(xr8)));

      if (likely(rectan_add_condi == 1)){
	D16MIN(xr4,xr4,xr9);//MC_MIN(cur_xy0, retan_buf_mb_ptr->xy0);
	D16MAX(xr5,xr5,xr7);//MC_MAX(cur_xye, retan_buf_mb_ptr->xye);
	S32STD(xr4,retan_buf_mb_ptr,0);
	S32STD(xr5,retan_buf_mb_ptr,4);

	fdd2[H264_NLEN*2+1]=fdd2[1] = S32M2I(xr6);//(cur_x0_mc & 0xFFFF) | (cur_y0 << 16);
	fdd2[H264_NLEN*2+0]=fdd2[0] = FDD2_DAT_H264(0,(REG2_REFA>>2),0,0/*my&7*/,0/*mx&7*/,3,cbsize,bid)|mxy_frac;
	REF_CQUEUE[mc_Cqueue_cnt] = ref_cnt;// char
	mc_ref_Uqueue_blk[mc_Cqueue_cnt] = &fdd2[1];
	mc_ref_Vqueue_blk[mc_Cqueue_cnt] = &fdd2[H264_NLEN*2+1];
	mc_Cqueue_cnt ++ ;

      } else {
	//xr6={(my>>2)-tap6_pre_skirt[my&3],
	//     (mx>>2)-tap6_pre_skirt[mx&3];}
	S32I2M(xr10,SLICE_T->uvlinesize);//xr10=SLICE_T->uvlinesize
	D16MUL_XW(xr0,xr6,xr10,xr8);//xr8=(my>>3)*SLICE_T->uvlinesize
	S32SFL(xr0,xr0,xr6,xr7,3);//xr7={16'b0,(mx>>3)};
	D16ASUM_AA(xr8,xr7,xr0,xr0);//xr8=signed_ext(xr7[15:0])+xr8;
	int src_offset_chroma=S32M2I(xr8);
	uint8_t * src_cb= ref->u_ptr + src_offset_chroma;
	uint8_t * src_cr= ref->v_ptr + src_offset_chroma;

	S32NOR(xr7,xr14,xr0);//xr7=0x3
	Q16ACC_SS(xr7,xr5,xr4,xr0);//xr7={cur_ye-cur_y0,cur_xe-cur_x0+3}
	S32AND(xr7,xr14,xr7);//xr7&=0xFFFFFFFC,word_align fetch_w

	fdd2[1] = ((sram_mc_dist_addr) | ((unsigned int)src_cb & 0x3));
	fdd2[H264_NLEN*2+0]=fdd2[0] = FDD2_DAT_H264(1,(REG2_REFA>>2),0,0/*my&7*/,0/*mx&7*/,0,cbsize,bid)|mxy_frac;

	//xr7 will be a para of blk_copy_word
	//xr8 will be used in blk_copy_word
	blk_copy_word((unsigned int)src_cb & 0xFFFFFFFC,
		      (uint8_t *)(sram_mc_dist_addr),
		      SLICE_T->uvlinesize
		      );

	fdd2[H264_NLEN*2+1] = ((sram_mc_dist_addr)|((unsigned int)src_cr&0x3));

	//xr7 will be a para of blk_copy_word
	//xr8 will be used in blk_copy_word
	blk_copy_word((unsigned int)src_cr & 0xFFFFFFFC,
		      (uint8_t *)(sram_mc_dist_addr),
		      SLICE_T->uvlinesize
		      );
      }

      if(is_list1_of_bidir){
	MCYF+=2;
	MCCF+=2;
      } else {
	MCYP+=2;
	MCCP+=2;
      }
#endif
    }
#endif
}

int xy_frac_offset_blk4[4]={0,0x10,0x100000,0x100010};

static void hl_motion_hw(H264_Slice_GlbARGs *SLICE_T, H264_MB_Ctrl_DecARGs *dmb, 
			 uint8_t * previous_dst, uint8_t * future_dst)
{
  //xr1, xr2 used in mc_part_hw
  S32I2M(xr1,-EDGE_WIDTH + 2);
  S32I2M(xr2,16*SLICE_T->mb_width + EDGE_WIDTH-3-16);
  S32I2M(xr3,16*SLICE_T->mb_height + EDGE_WIDTH-3-16);
  S32SFL(xr0,xr1,xr1,xr1,3);//xr1=(-EDGE_WIDTH+2) from now to the end of this function.
  S32SFL(xr0,xr3,xr2,xr2,3);//xr2=(pic_wh+EDGE_WIDTH-3-16) from now to the end of this function. xr3 available
  S32I2M(xr3,dmb->mb_x);
  S32I2M(xr4,dmb->mb_y);
  S32SFL(xr0,xr4,xr3,xr3,3);//xr3={s->mb_y,s->mb_x}. xr3 occupied from now to the end of this function. 
  Q16SLL(xr0,xr0,xr3,xr3,6);//xr3={s->mb_y<<6,s->mb_x<<6}, fraction pixel.
  S32I2M(xr13,0x3);//xr13=3
  S32NOR(xr14,xr0,xr13);//xr14=0xFFFFFFFC
  S32SFL(xr0,xr13,xr13,xr13,3);//xr13=0x30003
  
  const int mb_type= dmb->mb_type;
  int8_t * Inter_Dec_base=(uint32_t)dmb+sizeof(struct H264_MB_Ctrl_DecARGs);
  int i, list0, list1;
  isBIDIR = 0;
  MCYP = (unsigned int *)(mc_hw_chn_ptr->mc_hw_chn_nod_ptr->_MCC_YPS);  
  MCYF = (unsigned int *)(mc_hw_chn_ptr->mc_hw_chn_nod_ptr->_MCC_YFS);

  mc_hw_chn_ptr->mc_hw_chn_nod_ptr->_MCC_YSF  = 0;
  mc_hw_chn_ptr->mc_hw_chn_nod_ptr->_MCC_YPID = 0;
  mc_hw_chn_ptr->mc_hw_chn_nod_ptr->_MCC_YFID = 0;
  mc_hw_chn_ptr->mc_hw_chn_nod_ptr->_MCC_CSF  = 0;
  mc_hw_chn_ptr->mc_hw_chn_nod_ptr->_MCC_CPID = 0;
  mc_hw_chn_ptr->mc_hw_chn_nod_ptr->_MCC_CFID = 0;  

  MCYP[2] = 0x0;                                      /*ensure next node pending*/
  MCYP[0] = FDD_DATA_CMD((REG_CURA>>2),0);
  MCYP[1] = TCSM1_PADDR(previous_dst);
  MCYF[0] = FDD_DATA_CMD((REG_CURA>>2),0);
  MCYF[1] = TCSM1_PADDR(future_dst);
  
  //  SET_FDDC(TCSM1_PADDR(TCSM1_MCC_YPS) | 0x1);         /*Startup FDD into pending*/

  MCYP += 2;
  MCYF += 2;

  MCCP = (unsigned int *)(mc_hw_chn_ptr->mc_hw_chn_nod_ptr->_MCC_UPS);  
  MCCF = (unsigned int *)(mc_hw_chn_ptr->mc_hw_chn_nod_ptr->_MCC_UFS);

  MCCP[0] = FDD2_DATA_CMD((REG2_CURA>>2),0);
  MCCP[1] = TCSM1_PADDR(previous_dst+PREVIOUS_OFFSET_U);
  MCCF[0] = FDD2_DATA_CMD((REG2_CURA>>2),0);
  MCCF[1] = TCSM1_PADDR(future_dst+FUTURE_OFFSET_U);
  MCCP[H264_NLEN*2+0] = FDD2_DATA_CMD((REG2_CURA>>2),0);
  MCCP[H264_NLEN*2+1] = TCSM1_PADDR(previous_dst+PREVIOUS_OFFSET_V);
  MCCF[H264_NLEN*2+0] = FDD2_DATA_CMD((REG2_CURA>>2),0);
  MCCF[H264_NLEN*2+1] = TCSM1_PADDR(future_dst+FUTURE_OFFSET_V);
  //  SET_C2_FDDC(TCSM1_PADDR(TCSM1_MCC_UPS) | 0x1);      /*Startup FDD into pending*/

  MCCP += 2;
  MCCF += 2;

  uint8_t * _Wlist0_ptr = mc_hw_chn_ptr->_Wlist0_x;
  uint8_t * _Wlist1_ptr = mc_hw_chn_ptr->_Wlist1_x;


  if(IS_16X16(mb_type)){
    mc_part_hw(SLICE_T, Inter_Dec_base, 0, 0, 
	       ((0x0<<2)<<16)|((0x0<<2)),//xy_offset
	       ((0x10)<<16)|(0x10),//blkwh
	       0, MC_BLK_16X16, MC_BLK_8X8, 
	       IS_DIR(mb_type, 0, 0), IS_DIR(mb_type, 0, 1)); 
    i_movn(*(uint32_t *)_Wlist0_ptr,0x01010101,IS_DIR(mb_type, 0, 0));
    i_movn(*(uint32_t *)_Wlist1_ptr,0x01010101,IS_DIR(mb_type, 0, 1));    
  }else if(IS_16X8(mb_type)){

    mc_part_hw(SLICE_T, Inter_Dec_base, 0, 0, 
	       ((0x0<<2)<<16)|((0x0<<2)),//xy_offset
	       ((0x8)<<16)|(0x10),//blkwh
	       0, MC_BLK_16X8, MC_BLK_8X4,
	       IS_DIR(mb_type, 0, 0), IS_DIR(mb_type, 0, 1)); 
    mc_part_hw(SLICE_T, Inter_Dec_base, 1, 2, 
	       ((0x8<<2)<<16)|((0x0<<2)),//xy_offset
	       ((0x8)<<16)|(0x10),//blkwh
	       8, MC_BLK_16X8, MC_BLK_8X4,
	       IS_DIR(mb_type, 1, 0), IS_DIR(mb_type, 1, 1)); 
    i_movn(*(uint16_t *)&_Wlist0_ptr[0],0x0101,IS_DIR(mb_type, 0, 0));
    i_movn(*(uint16_t *)&_Wlist1_ptr[0],0x0101,IS_DIR(mb_type, 0, 1));
    i_movn(*(uint16_t *)&_Wlist0_ptr[2],0x0101,IS_DIR(mb_type, 1, 0));
    i_movn(*(uint16_t *)&_Wlist1_ptr[2],0x0101,IS_DIR(mb_type, 1, 1));
  }else if(IS_8X16(mb_type)){
    mc_part_hw(SLICE_T, Inter_Dec_base, 0, 0, 
	       ((0x0<<2)<<16)|((0x0<<2)),//xy_offset
	       ((0x10)<<16)|(0x8),//blkwh
	       0, MC_BLK_8X16, MC_BLK_4X8,
	       IS_DIR(mb_type, 0, 0), IS_DIR(mb_type, 0, 1)); 
    mc_part_hw(SLICE_T, Inter_Dec_base, 1, 1, 
	       ((0x0<<2)<<16)|((0x8<<2)),//xy_offset
	       ((0x10)<<16)|(0x8),//blkwh
	       8, MC_BLK_8X16, MC_BLK_4X8,
	       IS_DIR(mb_type, 1, 0), IS_DIR(mb_type, 1, 1)); 

    i_movn(*(uint32_t *)&_Wlist0_ptr[0],0x010001,IS_DIR(mb_type, 0, 0));
    i_movn(*(uint32_t *)&_Wlist1_ptr[0],0x010001,IS_DIR(mb_type, 0, 1));

    i_movn(_Wlist0_ptr[1],0x01,IS_DIR(mb_type, 1, 0));
    i_movn(_Wlist0_ptr[3],0x01,IS_DIR(mb_type, 1, 0));
    i_movn(_Wlist1_ptr[1],0x01,IS_DIR(mb_type, 1, 1));
    i_movn(_Wlist1_ptr[3],0x01,IS_DIR(mb_type, 1, 1));

  }else{
    int mv_n=0;
    for(i=0; i<4; i++){
      const int sub_mb_type= dmb->sub_mb_type[i];
      const int n= 4*i;
      int xy_offset=xy_frac_offset_blk4[i]<<1;
      i_movn(_Wlist0_ptr[i],0x01,IS_DIR(sub_mb_type, 0, 0));
      i_movn(_Wlist1_ptr[i],0x01,IS_DIR(sub_mb_type, 0, 1));
      if(likely(IS_SUB_8X8(sub_mb_type))){
	mc_part_hw(SLICE_T, Inter_Dec_base, mv_n, i, 
		   xy_offset,//xy_offset
		   ((0x8)<<16)|(0x8),//blkwh
		   n, MC_BLK_8X8, MC_BLK_4X4,
		   IS_DIR(sub_mb_type, 0, 0), IS_DIR(sub_mb_type, 0, 1)); 
	mv_n++;
      }else if(IS_SUB_8X4(sub_mb_type)){
	mc_part_hw(SLICE_T, Inter_Dec_base, mv_n, i,
		   xy_offset,//xy_offset
		   ((0x4)<<16)|(0x8),//blkwh
		   n, MC_BLK_8X4, MC_BLK_4X2,
		   IS_DIR(sub_mb_type, 0, 0), IS_DIR(sub_mb_type, 0, 1)); 
	mv_n++;
	mc_part_hw(SLICE_T, Inter_Dec_base, mv_n, i,
		   xy_offset+((0x4<<2)<<16)|((0x0<<2)),//xy_offset
		   ((0x4)<<16)|(0x8),//blkwh
		   n+2, MC_BLK_8X4, MC_BLK_4X2,
		   IS_DIR(sub_mb_type, 0, 0), IS_DIR(sub_mb_type, 0, 1)); 
	mv_n++;
      }else if(IS_SUB_4X8(sub_mb_type)){
	mc_part_hw(SLICE_T, Inter_Dec_base, mv_n, i,
		   xy_offset,//xy_offset
		   ((0x8)<<16)|(0x4),//blkwh
		   n, MC_BLK_4X8, MC_BLK_2X4,
		   IS_DIR(sub_mb_type, 0, 0), IS_DIR(sub_mb_type, 0, 1)); 
	mv_n++;
	mc_part_hw(SLICE_T, Inter_Dec_base, mv_n, i,
		   xy_offset+((0x0<<2)<<16)|((0x4<<2)),//xy_offset
		   ((0x8)<<16)|(0x4),//blkwh
		   n+2, MC_BLK_4X8, MC_BLK_2X4,
		   IS_DIR(sub_mb_type, 0, 0), IS_DIR(sub_mb_type, 0, 1)); 
	mv_n++;
      }else{
	int j;
	for(j=0; j<4; j++){
	  mc_part_hw(SLICE_T, Inter_Dec_base, mv_n, i,
		     xy_offset+xy_frac_offset_blk4[j],//xy_offset
		     ((0x4)<<16)|(0x4),//blkwh
		     n+j, MC_BLK_4X4, MC_BLK_2X2,
		     IS_DIR(sub_mb_type, 0, 0), IS_DIR(sub_mb_type, 0, 1)); 
	  mv_n++;
	}
      }
    }
  }

  if(isBIDIR){
    MCYP[0]           = FDD_MD_JUMP | FDD_LK | ((TCSM1_PADDR(mc_hw_chn_ptr->mc_hw_chn_nod_ptr->_MCC_YFS)) & 0xFFFFFF);
    MCYF[0]           = FDD_MD_JUMP | FDD_LK | ((TCSM1_PADDR(&mc_hw_chn_ptr->mc_hw_chn_nod_ptr->_MCC_YSN)) & 0xFFFFFF);

    MCCP[0]           = FDD2_MD_JUMP | FDD2_LK | ((TCSM1_PADDR(mc_hw_chn_ptr->mc_hw_chn_nod_ptr->_MCC_UFS)) & 0xFFFFFF);
    MCCF[0]           = FDD2_MD_JUMP | FDD2_LK | ((TCSM1_PADDR(mc_hw_chn_ptr->mc_hw_chn_nod_ptr->_MCC_VPS)) & 0xFFFFFF);
    MCCP[H264_NLEN*2] = FDD2_MD_JUMP | FDD2_LK | ((TCSM1_PADDR(mc_hw_chn_ptr->mc_hw_chn_nod_ptr->_MCC_VFS)) & 0xFFFFFF);
    MCCF[H264_NLEN*2] = FDD2_MD_JUMP | FDD2_LK | ((TCSM1_PADDR(&mc_hw_chn_ptr->mc_hw_chn_nod_ptr->_MCC_CSN)) & 0xFFFFFF);
  }else{
    MCYP[0]           = FDD_MD_JUMP | FDD_LK | ((TCSM1_PADDR(&mc_hw_chn_ptr->mc_hw_chn_nod_ptr->_MCC_YSN)) & 0xFFFFFF);

    MCCP[0]           = FDD2_MD_JUMP | FDD2_LK | ((TCSM1_PADDR(mc_hw_chn_ptr->mc_hw_chn_nod_ptr->_MCC_VPS)) & 0xFFFFFF);
    MCCP[H264_NLEN*2] = FDD2_MD_JUMP | FDD2_LK | ((TCSM1_PADDR(&mc_hw_chn_ptr->mc_hw_chn_nod_ptr->_MCC_CSN)) & 0xFFFFFF);
  }
}


