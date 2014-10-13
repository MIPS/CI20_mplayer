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

/****************************************************************
	JZMedia VPU H.264 decoder API
	<pwang@ingenic.cn>
	copyright (c) 2012 Ingenic
*****************************************************************/


#undef fprintf
#include"jzm_vpu.h" //@
#include"h264_cabac.c" //@  for  cabac_context_init_PB   and  cabac_context_init_I
//#include"../libjzcommon/jzasm.h"// @ for  write_reg
unsigned char *sde_bs_buffer;  //@ 

static inline int jzm_clip2(int a, int min, int max)
{
  if (a < min)      return min;
  else if (a > max) return max;
  else              return a;
}

void jzm_h264_slice_init_vdma(struct JZM_H264 * st_h264)
{

    // av_log(NULL,AV_LOG_WARNING,"[vdma] Entering  jzm_h264_slice_init_vdma\n ");
  int i, j;
  volatile unsigned int *chn = (volatile unsigned int *)st_h264->des_va; // pointer  where ? 

  GEN_VDMA_ACFG(chn, TCSM_FLUSH, 0, 0x0);

/*   GEN_VDMA_ACFG(chn, VPU_BASE | REG_SCH_GLBC, 0, SCH_GLBC_HIAXI */
/* 		| SCH_INTE_ACFGERR */
/* 		| SCH_INTE_BSERR */
/* 		| SCH_INTE_ENDF */
/* 		| SCH_INTE_TLBERR);  //@ */
  // GEN_VDMA_ACFG(chn, VPU_BASE , 0, AP_TLB_BASE);   //  @ can't find definition of AP_TLB_BASE
  //  GEN_VDMA_ACFG(chn, VPU_BASE, 0, 0);  //  @

  /*------------------------------------------------------
    scheduler
    ------------------------------------------------------*/
  GEN_VDMA_ACFG(chn, REG_SCH_SCHC, 0, 0x0);
  if (st_h264->slice_type == JZM_H264_I_TYPE) {
    GEN_VDMA_ACFG(chn, REG_SCH_BND, 0, SCH_CH3_HID(HID_DBLK) | SCH_CH2_HID(HID_VMAU) | SCH_DEPTH(DESP_FIFO_DEPTH));
    GEN_VDMA_ACFG(chn, REG_SCH_SCHG0, 0, 0x0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHE1, 0, 0x0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHE2, 0, 0x0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHE3, 0, 0x0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHE4, 0, 0x0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHC, 0, SCH_CH2_PE | SCH_CH3_PE);
    GEN_VDMA_ACFG(chn, REG_SCH_BND, 0, SCH_CH3_HID(HID_DBLK) | SCH_CH2_HID(HID_VMAU) | 
		  SCH_DEPTH(DESP_FIFO_DEPTH) | SCH_BND_G0F2 | SCH_BND_G0F3);
  } else {
    GEN_VDMA_ACFG(chn, REG_SCH_BND, 0, SCH_CH3_HID(HID_DBLK) | SCH_CH2_HID(HID_VMAU) | SCH_CH1_HID(HID_MCE) |
		  SCH_DEPTH(DESP_FIFO_DEPTH)); 
    GEN_VDMA_ACFG(chn, REG_SCH_SCHG0, 0, 0x0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHE1, 0, 0x0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHE2, 0, 0x0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHE3, 0, 0x0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHE4, 0, 0x0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHC, 0, SCH_CH1_PE | SCH_CH2_PE | SCH_CH3_PE);
    GEN_VDMA_ACFG(chn, REG_SCH_BND, 0, SCH_CH3_HID(HID_DBLK) | SCH_CH2_HID(HID_VMAU) | SCH_CH1_HID(HID_MCE) |
		  SCH_DEPTH(DESP_FIFO_DEPTH) | SCH_BND_G0F1 | SCH_BND_G0F2 | SCH_BND_G0F3);
  }
  /*------------------------------------------------------
    vmau
    ------------------------------------------------------*/
  GEN_VDMA_ACFG(chn, REG_VMAU_GBL_RUN, 0, VMAU_RESET);
  GEN_VDMA_ACFG(chn, REG_VMAU_GBL_CTR, 0, 0);
  GEN_VDMA_ACFG(chn, REG_VMAU_VIDEO_TYPE, 0, VMAU_FMT_H264);
  GEN_VDMA_ACFG(chn, REG_VMAU_NCCHN_ADDR, 0, VMAU_DESP_ADDR);
  GEN_VDMA_ACFG(chn, REG_VMAU_DEC_DONE, 0, VPU_BASE + REG_SCH_SCHE2);
  GEN_VDMA_ACFG(chn, REG_VMAU_Y_GS, 0, st_h264->mb_width*16);
  GEN_VDMA_ACFG(chn, REG_VMAU_GBL_CTR, 0, (VMAU_CTRL_FIFO_M | VMAU_CTRL_TO_DBLK));
  GEN_VDMA_ACFG(chn, REG_VMAU_POS, 0, (st_h264->start_mb_x & 0xff) | ((st_h264->start_mb_y & 0xff) << 16));// change KingKong
  unsigned int qt_ram_addr = REG_VMAU_QT;
  unsigned int *tbl_ptr = st_h264->scaling_matrix8;
  for ( i = 0 ; i < 32; i++) {
    GEN_VDMA_ACFG(chn, (qt_ram_addr+i*4), 0, tbl_ptr[i]);
  }
  qt_ram_addr = REG_VMAU_QT + 32*4;
  tbl_ptr = st_h264->scaling_matrix4;
  for ( i = 0 ; i < 24; i++) {
    GEN_VDMA_ACFG(chn, (qt_ram_addr+i*4), 0, tbl_ptr[i]);
  }
  /*------------------------------------------------------
    dblk
    ------------------------------------------------------*/
  GEN_VDMA_ACFG(chn, REG_DBLK_TRIG, 0, DBLK_RESET);
  GEN_VDMA_ACFG(chn, REG_DBLK_DHA, 0, DBLK_DESP_ADDR);
  GEN_VDMA_ACFG(chn, REG_DBLK_GENDA, 0, VPU_BASE + REG_SCH_SCHE3);
  GEN_VDMA_ACFG(chn, REG_DBLK_GSIZE, 0, st_h264->mb_width | (st_h264->mb_height << 16));
  //int normal_first_slice = (st_h264->start_mb_x == 0) && (st_h264->start_mb_y == 0);
  int normal_first_slice = 1;
  GEN_VDMA_ACFG(chn, REG_DBLK_GPOS, 0, ((st_h264->start_mb_x & 0x3ff) |
					((st_h264->start_mb_y & 0x3ff)<<16) |
					((!normal_first_slice)<<31))
		);
  GEN_VDMA_ACFG(chn, REG_DBLK_GPIC_YA, 0, st_h264->dec_result_y);
  GEN_VDMA_ACFG(chn, REG_DBLK_GPIC_CA, 0, st_h264->dec_result_uv);
  GEN_VDMA_ACFG(chn, REG_DBLK_GP_ENDA, 0, DBLK_GP_ENDF_BASE);
#define DEBLK_VTR_FMT_I 0
#define DEBLK_VTR_FMT_P (1<<3)
#define DEBLK_VTR_FMT_B (2<<3)
#define DEBLK_VTR_BETA_SFT (16)
#define DEBLK_VTR_BETA_MSK (0xff)
#define DEBLK_VTR_ALPHA_SFT (24)
#define DEBLK_VTR_ALPHA_MSK (0xff)
  unsigned int h264_vtr = ((st_h264->slice_type == JZM_H264_I_TYPE) ? DEBLK_VTR_FMT_I :
			   ((st_h264->slice_type == JZM_H264_P_TYPE) ? DEBLK_VTR_FMT_P : DEBLK_VTR_FMT_B)
			   ) | DBLK_FMT_H264;
  h264_vtr = h264_vtr | ((st_h264->slice_beta_offset & DEBLK_VTR_BETA_MSK) << DEBLK_VTR_BETA_SFT)
    | ((st_h264->slice_alpha_c0_offset & DEBLK_VTR_ALPHA_MSK) << DEBLK_VTR_ALPHA_SFT);
  GEN_VDMA_ACFG(chn, REG_DBLK_VTR, 0, h264_vtr);
  GEN_VDMA_ACFG(chn, REG_DBLK_GPIC_STR, 0, (st_h264->mb_width*256) | (st_h264->mb_width*128)<<16);
  GEN_VDMA_ACFG(chn, REG_DBLK_TRIG, 0, DBLK_SLICE_RUN);
  GEN_VDMA_ACFG(chn, REG_DBLK_CTRL, 0, 0x1);
  //write_reg(DBLK_GP_ENDF_BASE, -1);

  /*------------------------------------------------------
    motion
    ------------------------------------------------------*/
  //video_motion_init(H264_QPEL, H264_EPEL);
  int intpid  = H264_QPEL;
  int cintpid = H264_EPEL;
  for(i=0; i<16; i++){
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_ILUT+i*8, 0, 
		  MCE_CH1_IINFO(IntpFMT[intpid][i].intp[0],/*intp1*/
				IntpFMT[intpid][i].tap,/*tap*/
				IntpFMT[intpid][i].intp_pkg[0],/*intp1_pkg*/
				IntpFMT[intpid][i].hldgl,/*hldgl*/
				IntpFMT[intpid][i].avsdgl,/*avsdgl*/
				IntpFMT[intpid][i].intp_dir[0],/*intp0_dir*/
				IntpFMT[intpid][i].intp_rnd[0],/*intp0_rnd*/
				IntpFMT[intpid][i].intp_sft[0],/*intp0_sft*/
				IntpFMT[intpid][i].intp_sintp[0],/*sintp0*/
				IntpFMT[intpid][i].intp_srnd[0],/*sintp0_rnd*/
				IntpFMT[intpid][i].intp_sbias[0]/*sintp0_bias*/
				));
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_ILUT+i*8+4, 0, 
		  MCE_CH1_IINFO(IntpFMT[intpid][i].intp[1],/*intp1*/
				0,/*tap*/
				IntpFMT[intpid][i].intp_pkg[1],/*intp1_pkg*/
				IntpFMT[intpid][i].hldgl,/*hldgl*/
				IntpFMT[intpid][i].avsdgl,/*avsdgl*/
				IntpFMT[intpid][i].intp_dir[1],/*intp1_dir*/
				IntpFMT[intpid][i].intp_rnd[1],/*intp1_rnd*/
				IntpFMT[intpid][i].intp_sft[1],/*intp1_sft*/
				IntpFMT[intpid][i].intp_sintp[1],/*sintp1*/
				IntpFMT[intpid][i].intp_srnd[1],/*sintp1_rnd*/
				IntpFMT[intpid][i].intp_sbias[1]/*sintp1_bias*/
				));
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_CLUT+i*8+4, 0,
		  MCE_RLUT_WT(IntpFMT[intpid][i].intp_coef[0][7],/*coef8*/
			      IntpFMT[intpid][i].intp_coef[0][6],/*coef7*/
			      IntpFMT[intpid][i].intp_coef[0][5],/*coef6*/
			      IntpFMT[intpid][i].intp_coef[0][4]/*coef5*/
			      ));
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_CLUT+i*8, 0,
		  MCE_RLUT_WT(IntpFMT[intpid][i].intp_coef[0][3],/*coef8*/
			      IntpFMT[intpid][i].intp_coef[0][2],/*coef7*/
			      IntpFMT[intpid][i].intp_coef[0][1],/*coef6*/
			      IntpFMT[intpid][i].intp_coef[0][0]/*coef5*/
			      ));
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_CLUT+(i+16)*8+4, 0,
		  MCE_RLUT_WT(IntpFMT[intpid][i].intp_coef[1][7],/*coef8*/
			      IntpFMT[intpid][i].intp_coef[1][6],/*coef7*/
			      IntpFMT[intpid][i].intp_coef[1][5],/*coef6*/
			      IntpFMT[intpid][i].intp_coef[1][4]/*coef5*/
			      ));
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_CLUT+(i+16)*8, 0,
		  MCE_RLUT_WT(IntpFMT[intpid][i].intp_coef[1][3],/*coef8*/
			      IntpFMT[intpid][i].intp_coef[1][2],/*coef7*/
			      IntpFMT[intpid][i].intp_coef[1][1],/*coef6*/
			      IntpFMT[intpid][i].intp_coef[1][0]/*coef5*/
			      ));
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_ILUT+i*8, 0, 
		  MCE_CH2_IINFO(IntpFMT[cintpid][i].intp[0],
				IntpFMT[cintpid][i].intp_dir[0],
				IntpFMT[cintpid][i].intp_sft[0],
				IntpFMT[cintpid][i].intp_coef[0][0],
				IntpFMT[cintpid][i].intp_coef[0][1],
				IntpFMT[cintpid][i].intp_rnd[0]) );
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_ILUT+i*8+4, 0, 
		  MCE_CH2_IINFO(IntpFMT[cintpid][i].intp[1],
				IntpFMT[cintpid][i].intp_dir[1],
				IntpFMT[cintpid][i].intp_sft[1],
				IntpFMT[cintpid][i].intp_coef[1][0],
				IntpFMT[cintpid][i].intp_coef[1][1],
				IntpFMT[cintpid][i].intp_rnd[1]) );
  }
#define STAT_PFE_SFT        2
#define STAT_PFE_MSK        0x1
#define STAT_LKE_SFT        1
#define STAT_LKE_MSK        0x1
#define STAT_TKE_SFT        0
#define STAT_TKE_MSK        0x1
#define MCE_PRI             (0x3 << 9)
  GEN_VDMA_ACFG(chn, REG_MCE_CH1_STAT, 0, ((1 & STAT_PFE_MSK )<<STAT_PFE_SFT ) |   
		((1 & STAT_LKE_MSK )<<STAT_LKE_SFT ) | 
		((1 & STAT_TKE_MSK )<<STAT_TKE_SFT ) );
  GEN_VDMA_ACFG(chn, REG_MCE_CH2_STAT, 0, ((1 & STAT_PFE_MSK )<<STAT_PFE_SFT ) |   
		((1 & STAT_LKE_MSK )<<STAT_LKE_SFT ) | 
		((1 & STAT_TKE_MSK )<<STAT_TKE_SFT ) );
  GEN_VDMA_ACFG(chn, (REG_MCE_CTRL), 0, MCE_COMP_AUTO_EXPD | MCE_CH2_EN | MCE_CLKG_EN | MCE_PRI |
		MCE_OFA_EN | MCE_CACHE_FLUSH | MCE_EN );
  GEN_VDMA_ACFG(chn, (REG_MCE_CH1_BINFO), 0, MCE_BINFO(AryFMT[intpid], 0, 0, 0, SubPel[intpid]-1));
  GEN_VDMA_ACFG(chn, (REG_MCE_CH2_BINFO), 0, MCE_BINFO(0, 0, 0, 0, 0));

  GEN_VDMA_ACFG(chn, (REG_MCE_CH1_PINFO), 0, 0);
  GEN_VDMA_ACFG(chn, (REG_MCE_CH2_PINFO), 0, 0);

  for(i=0; i<16; i++){
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_RLUT+(i)*8, 0, MCE_RLUT_WT(0,0,st_h264->luma_weight[0][i], st_h264->luma_offset[0][i]));
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_RLUT+(i)*8+4, 0, st_h264->mc_ref_y[0][i]);
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_RLUT+(i)*8, 0, MCE_RLUT_WT(st_h264->chroma_weight[0][i][1], st_h264->chroma_offset[0][i][1],
							      st_h264->chroma_weight[0][i][0], st_h264->chroma_offset[0][i][0]));
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_RLUT+(i)*8+4, 0, st_h264->mc_ref_c[0][i]);
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_RLUT+(16+i)*8, 0, MCE_RLUT_WT(0,0,st_h264->luma_weight[1][i], st_h264->luma_offset[1][i]));
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_RLUT+(16+i)*8+4, 0, st_h264->mc_ref_y[1][i]);
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_RLUT+(16+i)*8, 0, MCE_RLUT_WT(st_h264->chroma_weight[1][i][1], st_h264->chroma_offset[1][i][1],
								 st_h264->chroma_weight[1][i][0], st_h264->chroma_offset[1][i][0]));
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_RLUT+(16+i)*8+4, 0, st_h264->mc_ref_c[1][i]);
  }

  for(j=0; j<16; j++){
    for(i=0; i<16; i+=4){
      GEN_VDMA_ACFG(chn, (MOTION_IWTA_BASE+j*16+i), 0, *((int*)(&st_h264->implicit_weight[j][i])));
    }
  }      
  GEN_VDMA_ACFG(chn, REG_MCE_IWTA, 0, MOTION_IWTA_BASE);

  GEN_VDMA_ACFG(chn, (REG_MCE_CH1_WINFO), 0, MCE_WINFO(0, (st_h264->use_weight == IS_WT1),
						       st_h264->use_weight, 1, st_h264->luma_log2_weight_denom, 5, 0, 0));
  GEN_VDMA_ACFG(chn, (REG_MCE_CH1_WTRND), 0, MCE_WTRND(0, 1<<5));
  GEN_VDMA_ACFG(chn, (REG_MCE_CH2_WINFO1), 0, MCE_WINFO(0, st_h264->use_weight_chroma && (st_h264->use_weight == IS_WT1),
							st_h264->use_weight, 1, st_h264->chroma_log2_weight_denom, 5, 0, 0));
  GEN_VDMA_ACFG(chn, (REG_MCE_CH2_WINFO2), 0, MCE_WINFO(0, 0, 0, 0, 0, 5, 0, 0));
  GEN_VDMA_ACFG(chn, (REG_MCE_CH2_WTRND), 0, MCE_WTRND(1<<5, 1<<5));

  GEN_VDMA_ACFG(chn, REG_MCE_CH1_STRD, 0, MCE_STRD(st_h264->mb_width*16, 0, DOUT_Y_STRD)); 
  GEN_VDMA_ACFG(chn, REG_MCE_GEOM, 0, MCE_GEOM(st_h264->mb_height*16,st_h264->mb_width*16)); 
  GEN_VDMA_ACFG(chn, REG_MCE_CH2_STRD, 0, MCE_STRD(st_h264->mb_width*16, 0, DOUT_C_STRD)); 
  GEN_VDMA_ACFG(chn, REG_MCE_DSA, 0, VPU_BASE + REG_SCH_SCHE1); 
  GEN_VDMA_ACFG(chn, REG_MCE_DDC, 0, MC_DESP_ADDR); 

  /*------------------------------------------------------
    sde
    ------------------------------------------------------*/
  // multi-slice
  int start_mb_num= st_h264->start_mb_x + st_h264->start_mb_y * st_h264->mb_width;
  int slice_start_mx = st_h264->start_mb_x;
  int slice_start_my = st_h264->start_mb_y;
  int ref_frm_start_mb = 0;
  if (st_h264->slice_num == 0) {
    for (i=0; i<32; i++)
      st_h264->curr_frm_slice_start_mb[i] = (1<<30);
  }
  st_h264->curr_frm_slice_start_mb[st_h264->slice_num] = start_mb_num;
  if ( (st_h264->slice_type == JZM_H264_B_TYPE) && (st_h264->slice_num)) {
    for (i=0; i<32; i++){
      if ( (start_mb_num >= st_h264->ref_frm_slice_start_mb[i]) &&
	   (start_mb_num < st_h264->ref_frm_slice_start_mb[i+1]) )
	break;
    }
    ref_frm_start_mb = st_h264->ref_frm_slice_start_mb[i];
  }

  // bs init
#if 0 // by pwang
  unsigned int bs_addr = st_h264->bs_buffer;
  unsigned int bs_ofst = st_h264->bs_index;
  // av_log(NULL,AV_LOG_WARNING,"[vdma] st_h264->bs_buffer: 0x%x \n ",st_h264->bs_buffer); 
  // av_log(NULL,AV_LOG_WARNING,"[vdma] st_h264->bs_index: 0x%x \n ",st_h264->bs_index); 

  { // transfer index to lessthan 32
    bs_addr = st_h264->bs_buffer + st_h264->bs_index/8;
    bs_ofst = (bs_addr & 0x3) << 3;
    bs_addr = bs_addr & 0xFFFFFFFC;
  }

  int sde_bs_len = (st_h264->bs_size_in_bits + 31) / 32;
  // av_log(NULL,AV_LOG_WARNING,"[vdma] sde_bs_len=%d\n",sde_bs_len);

  /*------------------
  for (i=0; i<sde_bs_len; i++) {
    ((int*)sde_bs_buffer)[i] = ((int*)bs_addr)[i];
    av_log(NULL,AV_LOG_WARNING,"loop... \n ");
 // sde_bs_buffer where defined ?
  }
  bs_addr = (unsigned int)sde_bs_buffer; 
  ------------------*/
#else

  unsigned int bs_addr;
  unsigned int bs_ofst;
  if (st_h264->cabac) {
    int n = (-st_h264->bs_index) & 7;
    if(n) st_h264->bs_index += n;
    bs_addr = st_h264->bs_buffer + (st_h264->bs_index >> 3);
    bs_ofst = (bs_addr & 0x3) << 3;
    bs_addr = bs_addr & (~0x3);
  } else {
    bs_addr = (unsigned int)(st_h264->bs_buffer + (st_h264->bs_index >> 3)) & (~0x3);
    bs_ofst = ((((unsigned int)st_h264->bs_buffer & 0x3) << 3) + ((unsigned int)st_h264->bs_index & 0x1F)) & 0x1F;
    unsigned int slice_bs_size = st_h264->bs_size_in_bits - st_h264->bs_index + bs_ofst;
    st_h264->bs_size_in_bits = slice_bs_size;
  }

#endif

  GEN_VDMA_ACFG(chn, REG_SDE_STAT, 0, 0);
  GEN_VDMA_ACFG(chn, REG_SDE_SL_GEOM, 0, SDE_SL_GEOM(st_h264->mb_height,st_h264->mb_width,slice_start_my,slice_start_mx));
  GEN_VDMA_ACFG(chn, REG_SDE_GL_CTRL, 0, 1);
  GEN_VDMA_ACFG(chn, REG_SDE_CODEC_ID, 0, (1 << 0));
  unsigned int cfg_0 = (((!st_h264->cabac) << 0) +
			(((st_h264->slice_type) & 0x7) << 1) +
			(((st_h264->field_picture) & 0x1) << 4) +
			(((st_h264->transform_8x8_mode) & 0x1) << 5) +
			(((st_h264->constrained_intra_pred) & 0x1) << 6) +
			(((st_h264->direct_8x8_inference_flag) & 0x1) << 7) +
			(((st_h264->direct_spatial_mv_pred) & 0x1) << 8) +
			((1 & 0x1) << 9) + /*dir_max_word_64*/
			(((st_h264->x264_build > 33) & 0x1) << 10) +
			(((!st_h264->x264_build) & 0x1) << 11) +
			((st_h264->dblk_left_en & 0x1) << 14) +
			((st_h264->dblk_top_en & 0x1) << 15) +
			((st_h264->ref_count_0 & 0xF) << 16) +
			((st_h264->ref_count_1 & 0xF) << 20) +
			((bs_ofst & 0x1F) << 24) +
			0 );
  GEN_VDMA_ACFG(chn, REG_SDE_CFG0, 0, cfg_0);
  unsigned int cfg_1 = (((st_h264->qscale & 0xFF) << 0) +
			(((st_h264->deblocking_filter) & 0x1) << 8) +
			0);
  GEN_VDMA_ACFG(chn, REG_SDE_CFG1, 0, cfg_1);
  GEN_VDMA_ACFG(chn, REG_SDE_CFG2, 0, bs_addr);
  GEN_VDMA_ACFG(chn, REG_SDE_CFG3, 0, TOP_NEI_ADDR);
  GEN_VDMA_ACFG(chn, REG_SDE_CFG4, 0, RESIDUAL_DOUT_ADDR);
  GEN_VDMA_ACFG(chn, REG_SDE_CFG5, 0, VMAU_DESP_ADDR);
  GEN_VDMA_ACFG(chn, REG_SDE_CFG6, 0, DBLK_DESP_ADDR);
  GEN_VDMA_ACFG(chn, REG_SDE_CFG7, 0, DBLK_MV_ADDR);
  GEN_VDMA_ACFG(chn, REG_SDE_CFG8, 0, MC_DESP_ADDR);
  GEN_VDMA_ACFG(chn, REG_SDE_CFG9, 0,  st_h264->ref_frm_ctrl  + ref_frm_start_mb*2*4);
  GEN_VDMA_ACFG(chn, REG_SDE_CFG10, 0, st_h264->ref_frm_mv    + ref_frm_start_mb*32*4);
  GEN_VDMA_ACFG(chn, REG_SDE_CFG11, 0, st_h264->curr_frm_ctrl + start_mb_num*2*4);
  GEN_VDMA_ACFG(chn, REG_SDE_CFG12, 0, st_h264->curr_frm_mv   + start_mb_num*32*4);
  GEN_VDMA_ACFG(chn, REG_SDE_CFG13, 0, (start_mb_num & 0xFFFF) + (((start_mb_num - ref_frm_start_mb) & 0xFFFF) << 16));
  GEN_VDMA_ACFG(chn, REG_SDE_CFG14, 0, st_h264->bs_size_in_bits);

  // ctx table init
  unsigned int * sde_table_base = REG_SDE_CTX_TBL;
  if (st_h264->cabac) {
    unsigned char state;
    for( i= 0; i < 460; i++ ) {
      int pre;
      if( st_h264->slice_type == JZM_H264_I_TYPE )
	pre = jzm_clip2( ((cabac_context_init_I[i][0] * st_h264->qscale) >>4 ) + cabac_context_init_I[i][1], 1, 126 );
      else
	pre = jzm_clip2( ((cabac_context_init_PB[st_h264->cabac_init_idc][i][0] * st_h264->qscale) >>4 ) + cabac_context_init_PB[st_h264->cabac_init_idc][i][1], 1, 126 );
      if( pre <= 63 )
	state = 2 * ( 63 - pre ) + 0;
      else
	state = 2 * ( pre - 64 ) + 1;
      GEN_VDMA_ACFG(chn, sde_table_base+i, 0, lps_comb[state]);
    }
    GEN_VDMA_ACFG(chn, sde_table_base+276, 0, lps_comb[126]);
  } else {
    int tbl, size;
    for (tbl = 0; tbl < 7; tbl++) {
      unsigned int * hw_base = sde_table_base + (sde_vlc2_sta[tbl].ram_ofst >> 1);
      unsigned int * tbl_base = &sde_vlc2_table[tbl][0];
      int size = (sde_vlc2_sta[tbl].size + 1) >> 1;
      for (i = 0; i < size; i++){
	GEN_VDMA_ACFG(chn, hw_base+i, 0, tbl_base[i]);
      }
    }
  }
  // direct prediction scal table
  for (i=0; i<16; i++) {
    GEN_VDMA_ACFG(chn, sde_table_base + 480 + i, 0, st_h264->dir_scale_table[i]);
  }
  // set chroma_qp table
  unsigned int * sde_cqp_tbl = REG_SDE_CQP_TBL;
  for(i=0;i<128;i++) {
    GEN_VDMA_ACFG(chn, sde_cqp_tbl + i, 0, st_h264->chroma_qp_table[i]);
  }

  GEN_VDMA_ACFG(chn, (REG_SDE_SL_CTRL), VDMA_ACFG_TERM, SDE_MB_RUN);

}


unsigned short fprint_frame(struct MpegEncContext *s){
    FILE * hw_rlt_fp = fopen("linux_frame","w+");
    int i, j, k;
    uint8_t *tile_y, *tile_c;
    int hw_crc, sw_crc;
    int crc_code = 0;
    int len;
  
    tile_y = s->current_picture.data[0];
    tile_c = s->current_picture.data[1];
    len = s->mb_height * s->mb_width * 256;

#if 1
    for (i=0; i<len; i++) {
	if (i%256 == 0) {
	    int mb_num = i/256; int mb_x = mb_num%s->mb_width; int mb_y = mb_num/s->mb_width;
	    fprintf(hw_rlt_fp," - y mb:%d (x:%d,y:%d) -\n",mb_num,mb_x,mb_y);
    }
	fprintf(hw_rlt_fp,"%x,",tile_y[i]);
	if (i%16 == 15) fprintf(hw_rlt_fp,"\n");
    }
#endif


    len = len >> 1;
  for (i=0; i<len; i++) {
      if (i%128 == 0) {
	  int mb_num = i/128; int mb_x = mb_num%s->mb_width; int mb_y = mb_num/s->mb_width;
	  fprintf(hw_rlt_fp," - c mb:%d (x:%d,y:%d) -\n",mb_num,mb_x,mb_y);
      }
      fprintf(hw_rlt_fp,"%x,",tile_c[i]);
      if (i%16 == 15) fprintf(hw_rlt_fp,"\n");
  }
  fclose(hw_rlt_fp);
}

