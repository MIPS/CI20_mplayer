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

#ifndef __JZM_MPEG2_DEC_C__
#define __JZM_MPEG2_DEC_C__

#include "jzm_mpeg2_dec.h"

extern int frame_num;
static void M2D_SliceInit(_M2D_SliceInfo *s)
{
  int i, j;
  _M2D_BitStream *bs= &s->bs_buf;
  volatile unsigned int *chn = (volatile unsigned int *)s->des_va;

  /*-------------------------sch init begin-------------------------------*/
  { /*configure sch*/
    GEN_VDMA_ACFG(chn, TCSM_FLUSH, 0, 0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHC, 0, 0);
    GEN_VDMA_ACFG(chn, REG_SCH_BND, 0, (SCH_CH3_HID(HID_DBLK) |
					SCH_CH2_HID(HID_VMAU) |
					SCH_CH1_HID(HID_MCE) |
					SCH_DEPTH(MPEG2_FIFO_DEP) 
					)
		  );
    
    GEN_VDMA_ACFG(chn, REG_SCH_SCHG0, 0, 0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHG1, 0, 0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHE1, 0, 0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHE2, 0, 0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHE3, 0, 0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHE4, 0, 0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHC, 0, (((s->coding_type != I_TYPE)<<2) | 
					 SCH_CH2_PE| 
					 SCH_CH3_PE					 
					 )
		  );
    GEN_VDMA_ACFG(chn, REG_SCH_BND, 0, (SCH_CH3_HID(HID_DBLK) |
					SCH_CH2_HID(HID_VMAU) |
					SCH_CH1_HID(HID_MCE) |
					SCH_DEPTH(MPEG2_FIFO_DEP) |
					((s->coding_type != I_TYPE)<<0) | 
					SCH_BND_G0F2 | 
					SCH_BND_G0F3 					
					) 		  
		  );
  }
  /*-------------------------sch init end-------------------------------*/  

  /*-------------------------mc init begin-------------------------------*/
  { /*configure motion*/
    int intpid= MPEG_HPEL; 
    int cintpid= MPEG_HPEL;
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
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_STAT, 0, (MCE_PREF_END |
					   MCE_LINK_END |
					   MCE_TASK_END ) );
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_STAT, 0, (MCE_PREF_END |
					   MCE_LINK_END |
					   MCE_TASK_END ) );
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_BINFO, 0, 0);
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_BINFO, 0, 0);
    GEN_VDMA_ACFG(chn, REG_MCE_CTRL, 0, ( MCE_COMP_AUTO_EXPD |
					  MCE_CH2_EN |
					  MCE_CLKG_EN |
					  MCE_OFA_EN |
					  MCE_CACHE_FLUSH |
					  MCE_EN 
					  ) );
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_PINFO, 0, MCE_PINFO(0, 0, 6, 0, 0));
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_PINFO, 0, MCE_PINFO(0, 0, 6, 0, 0));
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_WINFO, 0, MCE_WINFO(0, 0, 0, 1, 0, 0, 0, 0));
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_WTRND, 0, 0);
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_WINFO1, 0, MCE_WINFO(0, 0, 0, 1, 0, 0, 0, 0));
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_WINFO2, 0, 0);
    
    // configure ref frame.
    for (i= 0; i< 8; i++){
      GEN_VDMA_ACFG(chn, REG_MCE_CH1_RLUT+ i*8+ 4, 0, s->pic_ref[0][i]); 
    }
    for (i= 0; i< 8; i++){
      GEN_VDMA_ACFG(chn, REG_MCE_CH1_RLUT+ i*8+ 128 + 4, 0, s->pic_ref[0][i]);
    }
    for (i= 0; i< 8; i++){
      GEN_VDMA_ACFG(chn, REG_MCE_CH2_RLUT+ i*8+ 4, 0, s->pic_ref[1][i]); 
    }
    for (i= 0; i< 8; i++){
      GEN_VDMA_ACFG(chn, REG_MCE_CH2_RLUT+ i*8+ 128 + 4, 0, s->pic_ref[1][i]); 
    }

    GEN_VDMA_ACFG(chn, REG_MCE_CH2_WTRND, 0, 0);
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_STRD, 0, MCE_STRD(s->mb_width*16, 0, 16) );
    GEN_VDMA_ACFG(chn, REG_MCE_GEOM, 0, MCE_GEOM(s->mb_height*16, s->mb_width*16) );
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_STRD, 0, MCE_STRD(s->mb_width*16, 0, DOUT_C_STRD) );
    GEN_VDMA_ACFG(chn, REG_MCE_DSA, 0, SCH1_DSA);
    GEN_VDMA_ACFG(chn, REG_MCE_DDC, 0, TCSM1_MOTION_DHA);
  }
  /*-------------------------mc init end-------------------------------*/
 
  /*-------------------------vmau init begin-------------------------------*/
  { /*configure vmau*/
    for ( i = 0 ; i < 4; i++){ /*write qt table*/
      for ( j = 0 ; j < 16; j++){
	uint32_t idx= REG_VMAU_QT+ i*64 + (j<<2);
	GEN_VDMA_ACFG(chn, idx, 0, s->coef_qt[i][j]);
      }
    }
    GEN_VDMA_ACFG(chn, REG_VMAU_GBL_RUN, 0, VMAU_RESET);
    GEN_VDMA_ACFG(chn, REG_VMAU_VIDEO_TYPE, 0, VMAU_FMT_MPEG2);
    GEN_VDMA_ACFG(chn, REG_VMAU_DEC_STR, 0, (16<<16)|16);  
    GEN_VDMA_ACFG(chn, REG_VMAU_Y_GS, 0, s->mb_width*16);
    GEN_VDMA_ACFG(chn, REG_VMAU_GBL_CTR, 0, (VMAU_CTRL_TO_DBLK | VMAU_CTRL_FIFO_M) );
    GEN_VDMA_ACFG(chn, REG_VMAU_DEC_DONE, 0, SCH2_DSA);
    GEN_VDMA_ACFG(chn, REG_VMAU_NCCHN_ADDR, 0, VMAU_CHN_BASE);      
  }
  /*-------------------------vmau init end-------------------------------*/

  /*-------------------------dblk init begin-------------------------------*/
  { /*configure dblk*/
    GEN_VDMA_ACFG(chn, REG_DBLK_TRIG, 0, DBLK_RESET);
    GEN_VDMA_ACFG(chn, REG_DBLK_GPIC_STR, 0, DBLK_GPIC_STR(s->mb_width*128, s->mb_width*256) );
    GEN_VDMA_ACFG(chn, REG_DBLK_GPIC_YA, 0, s->pic_cur[0]);
    GEN_VDMA_ACFG(chn, REG_DBLK_GPIC_CA, 0, s->pic_cur[1]);
    GEN_VDMA_ACFG(chn, REG_DBLK_GSIZE, 0, DBLK_GSIZE(s->mb_height, s->mb_width) );
    GEN_VDMA_ACFG(chn, REG_DBLK_VTR, 0, DBLK_VTR(0, 0, 0, 0,
					       0, DBLK_FMT_MPEG2) );
    GEN_VDMA_ACFG(chn, REG_DBLK_GP_ENDA, 0, DBLK_END_FLAG);
    GEN_VDMA_ACFG(chn, REG_DBLK_GENDA, 0, SCH3_DSA);
    GEN_VDMA_ACFG(chn, REG_DBLK_GPOS, 0, DBLK_GPOS(s->mb_pos_y, s->mb_pos_x) );
    GEN_VDMA_ACFG(chn, REG_DBLK_DHA, 0, DBLK_CHN_BASE);
    GEN_VDMA_ACFG(chn, REG_DBLK_TRIG, 0, DBLK_SLICE_RUN);
  }
  /*-------------------------dblk init end-------------------------------*/

  /*-------------------------SDE init begin-------------------------------*/
  { /*configure sde*/
    GEN_VDMA_ACFG(chn, REG_SDE_CODEC_ID, 0, SDE_FMT_MPEG2_DEC);
    { /*fill vlc_ram*/
      // mb
      if (s->coding_type == I_TYPE) {
	for (i= 0; i< MB_I_LEN; i++){
	  uint32_t idx= REG_SDE_CTX_TBL+ ((MB_I_BASE+ i)<<2);
	  GEN_VDMA_ACFG(chn, idx, 0, MB_I_HW[i]);
	} 
      }
      if (s->coding_type == P_TYPE) {
	for (i= 0; i< MB_P_LEN; i++){
	  uint32_t idx= REG_SDE_CTX_TBL+ ((MB_P_BASE+ i)<<2);
	  GEN_VDMA_ACFG(chn, idx, 0, MB_P_HW[i]);
	} 
      }
      if (s->coding_type == B_TYPE) {
	for (i= 0; i< MB_B_LEN; i++){
	  uint32_t idx= REG_SDE_CTX_TBL+ ((MB_B_BASE+ i)<<2);
	  GEN_VDMA_ACFG(chn, idx, 0, MB_B_HW[i]);
	} 
      }
      // mba, dct
      for (i= 0; i< MBA_5_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((MBA_5_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, MBA_5_HW[i]);
      } 
      for (i= 0; i< MBA_11_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((MBA_11_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, MBA_11_HW[i]);
      } 
      for (i= 0; i< DC_lum_5_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((DC_lum_5_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DC_lum_5_HW[i]);
      } 
      for (i= 0; i< DC_long_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((DC_long_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DC_long_HW[i]);
      } 
      for (i= 0; i< DC_chrom_5_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((DC_chrom_5_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DC_chrom_5_HW[i]);
      } 
      for (i= 0; i< DCT_B15_8_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((DCT_B15_8_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_B15_8_HW[i]);
      } 
      for (i= 0; i< DCT_B15_10_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((DCT_B15_10_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_B15_10_HW[i]);
      } 
      for (i= 0; i< DCT_13_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((DCT_13_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_13_HW[i]);
      } 
      for (i= 0; i< DCT_15_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((DCT_15_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_15_HW[i]);
      } 
      for (i= 0; i< DCT_16_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((DCT_16_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_16_HW[i]);
      } 
      for (i= 0; i< DCT_B14AC_5_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((DCT_B14AC_5_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_B14AC_5_HW[i]);
      } 
      for (i= 0; i< DCT_B14DC_5_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((DCT_B14DC_5_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_B14DC_5_HW[i]);
      } 
      for (i= 0; i< DCT_B14_8_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((DCT_B14_8_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_B14_8_HW[i]);
      } 
      for (i= 0; i< DCT_B14_10_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((DCT_B14_10_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_B14_10_HW[i]);
      } 
      
      // cbp
      if (s->coding_type != I_TYPE) {
	for (i= 0; i< CBP_7_LEN; i++){
	  uint32_t idx= REG_SDE_CTX_TBL+ ((CBP_7_BASE+ i)<<2);
	  GEN_VDMA_ACFG(chn, idx, 0, CBP_7_HW[i]);
	} 
	for (i= 0; i< CBP_9_LEN; i++){
	  uint32_t idx= REG_SDE_CTX_TBL+ ((CBP_9_BASE+ i)<<2);
	  GEN_VDMA_ACFG(chn, idx, 0, CBP_9_HW[i]);
	}       
      }
      // mv
      for (i= 0; i< MV_4_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((MV_4_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, MV_4_HW[i]);
      }       
      for (i= 0; i< MV_10_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((MV_10_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, MV_10_HW[i]);
      }       
      for (i= 0; i< DMV_2_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((DMV_2_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DMV_2_HW[i]);
      }       
    }
    
    for (i= 0; i< 16; i++){ /*fill scan_ram*/
      uint32_t idx= REG_SDE_CTX_TBL+ 0x1000+ (i<<2);
      GEN_VDMA_ACFG(chn, idx, 0, s->scan[i]);
    }
     
    GEN_VDMA_ACFG(chn, REG_SDE_CFG2, 0, bs->buffer);
    GEN_VDMA_ACFG(chn, REG_SDE_CFG1, 0, bs->bit_ofst);
    GEN_VDMA_ACFG(chn, REG_SDE_CFG0, 0, GET_SL_INFO(s->coding_type, /*coding_type*/ 
						    s->fr_pred_fr_dct, /*fr_pred_fr_dct*/
						    s->pic_type, /*pic_type*/
						    s->conceal_mv, /*conceal_mv*/
						    s->intra_dc_pre, /*intra_dc_pre*/
						    s->intra_vlc_format, /*intra_vlc_format*/
						    s->mpeg1, /*mpeg1*/
						    s->top_fi_first, /*top_fi_first*/ 
						    s->q_scale_type, /*q_scale_type*/
						    s->sec_fld,
						    s->f_code[0][0], /*f_code_f0*/
						    s->f_code[0][1], /*f_code_f1*/
						    s->f_code[1][0], /*f_code_b0*/
						    s->f_code[1][1] /*f_code_b1*/));
    GEN_VDMA_ACFG(chn, REG_SDE_CFG3, 0, s->qs_code);
    GEN_VDMA_ACFG(chn, REG_SDE_CFG8, 0, TCSM1_MOTION_DHA);
    GEN_VDMA_ACFG(chn, REG_SDE_CFG11, 0, TCSM1_MSRC_BUF);
    GEN_VDMA_ACFG(chn, REG_SDE_CFG9, 0, VMAU_CHN_BASE);
    GEN_VDMA_ACFG(chn, REG_SDE_CFG10, 0, DBLK_CHN_BASE);
    GEN_VDMA_ACFG(chn, REG_SDE_SL_CTRL, 0, SDE_SLICE_INIT);
    
    GEN_VDMA_ACFG(chn, REG_SDE_GL_CTRL, 0, (SDE_MODE_AUTO | SDE_EN) );
    GEN_VDMA_ACFG(chn, REG_SDE_SL_GEOM, 0, SDE_SL_GEOM(s->mb_height, s->mb_width, 
						       s->mb_pos_y, s->mb_pos_x) );
    GEN_VDMA_ACFG(chn, REG_SDE_SL_CTRL, VDMA_ACFG_TERM, SDE_MB_RUN);
  }
  /*-------------------------SDE init end-------------------------------*/
}

static void M2D_SliceInit_ext(_M2D_SliceInfo *s)
{
  int i, j;
  _M2D_BitStream *bs= &s->bs_buf;
   int va = 0;

  volatile unsigned int *chn = (volatile unsigned int *)s->des_va;

  /*-------------------------sch init begin-------------------------------*/
  { /*configure sch*/
#if 0
    GEN_VDMA_ACFG(chn, TCSM_FLUSH, 0, 0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHC, 0, 0);
    GEN_VDMA_ACFG(chn, REG_SCH_BND, 0, (SCH_CH3_HID(HID_DBLK) |
					SCH_CH2_HID(HID_VMAU) |
					SCH_CH1_HID(HID_MCE) |
					SCH_DEPTH(MPEG2_FIFO_DEP) 
					)
		  );
    
    GEN_VDMA_ACFG(chn, REG_SCH_SCHG0, 0, 0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHG1, 0, 0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHE1, 0, 0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHE2, 0, 0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHE3, 0, 0);
    GEN_VDMA_ACFG(chn, REG_SCH_SCHE4, 0, 0);
#else
    chn += 18;
#endif
    va = chn;
    GEN_VDMA_ACFG(chn, REG_SCH_SCHC, 0, (((s->coding_type != I_TYPE)<<2) | 
					 SCH_CH2_PE| 
					 SCH_CH3_PE					 
					 )
		  );

    GEN_VDMA_ACFG(chn, REG_SCH_BND, 0, (SCH_CH3_HID(HID_DBLK) |
					SCH_CH2_HID(HID_VMAU) |
					SCH_CH1_HID(HID_MCE) |
					SCH_DEPTH(MPEG2_FIFO_DEP) |
					((s->coding_type != I_TYPE)<<0) | 
					SCH_BND_G0F2 | 
					SCH_BND_G0F3 					
					) 		  
		  );
  }
  i_cache(0x19, va, 0);
  /*-------------------------sch init end-------------------------------*/  

  /*-------------------------mc init begin-------------------------------*/
  { /*configure motion*/
    int intpid= MPEG_HPEL; 
    int cintpid= MPEG_HPEL;
#if 0
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
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_STAT, 0, (MCE_PREF_END |
					   MCE_LINK_END |
					   MCE_TASK_END ) );
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_STAT, 0, (MCE_PREF_END |
					   MCE_LINK_END |
					   MCE_TASK_END ) );
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_BINFO, 0, 0);
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_BINFO, 0, 0);
    GEN_VDMA_ACFG(chn, REG_MCE_CTRL, 0, ( MCE_COMP_AUTO_EXPD |
					  MCE_CH2_EN |
					  MCE_CLKG_EN |
					  MCE_OFA_EN |
					  MCE_CACHE_FLUSH |
					  MCE_EN 
					  ) );
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_PINFO, 0, MCE_PINFO(0, 0, 6, 0, 0));
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_PINFO, 0, MCE_PINFO(0, 0, 6, 0, 0));
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_WINFO, 0, MCE_WINFO(0, 0, 0, 1, 0, 0, 0, 0));
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_WTRND, 0, 0);
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_WINFO1, 0, MCE_WINFO(0, 0, 0, 1, 0, 0, 0, 0));
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_WINFO2, 0, 0);
#else
    chn += 0x116;
#endif
    
    va = chn;
    // configure ref frame.
    for (i= 0; i< 8; i++){
      GEN_VDMA_ACFG(chn, REG_MCE_CH1_RLUT+ i*8+ 4, 0, s->pic_ref[0][i]); 
    }
    for (i= 0; i< 8; i++){
      GEN_VDMA_ACFG(chn, REG_MCE_CH1_RLUT+ i*8+ 128 + 4, 0, s->pic_ref[0][i]);
    }
    for (i= 0; i< 8; i++){
      GEN_VDMA_ACFG(chn, REG_MCE_CH2_RLUT+ i*8+ 4, 0, s->pic_ref[1][i]); 
    }
    for (i= 0; i< 8; i++){
      GEN_VDMA_ACFG(chn, REG_MCE_CH2_RLUT+ i*8+ 128 + 4, 0, s->pic_ref[1][i]); 
    }
    for (i = 0; i < 8; i++){
      i_cache(0x19, va, 0);
      va+=32;
    }

#if 0
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_WTRND, 0, 0);
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_STRD, 0, MCE_STRD(s->mb_width*16, 0, 16) );
    GEN_VDMA_ACFG(chn, REG_MCE_GEOM, 0, MCE_GEOM(s->mb_height*16, s->mb_width*16) );
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_STRD, 0, MCE_STRD(s->mb_width*16, 0, DOUT_C_STRD) );
    GEN_VDMA_ACFG(chn, REG_MCE_DSA, 0, SCH1_DSA);
    GEN_VDMA_ACFG(chn, REG_MCE_DDC, 0, TCSM1_MOTION_DHA);
  }
  /*-------------------------mc init end-------------------------------*/
 
  /*-------------------------vmau init begin-------------------------------*/
  { /*configure vmau*/
    for ( i = 0 ; i < 4; i++){ /*write qt table*/
      for ( j = 0 ; j < 16; j++){
	uint32_t idx= REG_VMAU_QT+ i*64 + (j<<2);
	GEN_VDMA_ACFG(chn, idx, 0, s->coef_qt[i][j]);
      }
    }
    GEN_VDMA_ACFG(chn, REG_VMAU_GBL_RUN, 0, VMAU_RESET);
    GEN_VDMA_ACFG(chn, REG_VMAU_VIDEO_TYPE, 0, VMAU_FMT_MPEG2);
    GEN_VDMA_ACFG(chn, REG_VMAU_DEC_STR, 0, (16<<16)|16);  
    GEN_VDMA_ACFG(chn, REG_VMAU_Y_GS, 0, s->mb_width*16);
    GEN_VDMA_ACFG(chn, REG_VMAU_GBL_CTR, 0, (VMAU_CTRL_TO_DBLK | VMAU_CTRL_FIFO_M) );
    GEN_VDMA_ACFG(chn, REG_VMAU_DEC_DONE, 0, SCH2_DSA);
    GEN_VDMA_ACFG(chn, REG_VMAU_NCCHN_ADDR, 0, VMAU_CHN_BASE);      
  }
  /*-------------------------vmau init end-------------------------------*/

  /*-------------------------dblk init begin-------------------------------*/
  { /*configure dblk*/
    GEN_VDMA_ACFG(chn, REG_DBLK_TRIG, 0, DBLK_RESET);
    GEN_VDMA_ACFG(chn, REG_DBLK_GPIC_STR, 0, DBLK_GPIC_STR(s->mb_width*128, s->mb_width*256) );
    GEN_VDMA_ACFG(chn, REG_DBLK_GSIZE, 0, DBLK_GSIZE(s->mb_height, s->mb_width) );
    GEN_VDMA_ACFG(chn, REG_DBLK_VTR, 0, DBLK_VTR(0, 0, 0, 0,
					       0, DBLK_FMT_MPEG2) );
    GEN_VDMA_ACFG(chn, REG_DBLK_GP_ENDA, 0, DBLK_END_FLAG);
    GEN_VDMA_ACFG(chn, REG_DBLK_GENDA, 0, SCH3_DSA);
#else
    chn += 0xa6;
#endif

    va = chn;
    GEN_VDMA_ACFG(chn, REG_DBLK_GPOS, 0, DBLK_GPOS(s->mb_pos_y, s->mb_pos_x) );
    GEN_VDMA_ACFG(chn, REG_DBLK_GPIC_YA, 0, s->pic_cur[0]);
    GEN_VDMA_ACFG(chn, REG_DBLK_GPIC_CA, 0, s->pic_cur[1]);
    GEN_VDMA_ACFG(chn, REG_DBLK_DHA, 0, DBLK_CHN_BASE);
    GEN_VDMA_ACFG(chn, REG_DBLK_TRIG, 0, DBLK_SLICE_RUN);
  }
  /*-------------------------dblk init end-------------------------------*/

  /*-------------------------SDE init begin-------------------------------*/
  { /*configure sde*/
    GEN_VDMA_ACFG(chn, REG_SDE_CODEC_ID, 0, SDE_FMT_MPEG2_DEC);
    { /*fill vlc_ram*/
      // mb
      if (s->coding_type == I_TYPE) {
	for (i= 0; i< MB_I_LEN; i++){
	  uint32_t idx= REG_SDE_CTX_TBL+ ((MB_I_BASE+ i)<<2);
	  GEN_VDMA_ACFG(chn, idx, 0, MB_I_HW[i]);
	} 
      }
      if (s->coding_type == P_TYPE) {
	for (i= 0; i< MB_P_LEN; i++){
	  uint32_t idx= REG_SDE_CTX_TBL+ ((MB_P_BASE+ i)<<2);
	  GEN_VDMA_ACFG(chn, idx, 0, MB_P_HW[i]);
	} 
      }
      if (s->coding_type == B_TYPE) {
	for (i= 0; i< MB_B_LEN; i++){
	  uint32_t idx= REG_SDE_CTX_TBL+ ((MB_B_BASE+ i)<<2);
	  GEN_VDMA_ACFG(chn, idx, 0, MB_B_HW[i]);
	} 
      }

      for (i = 0; i < ((int)chn - va + 32) / 32; i++){
	i_cache(0x19, va, 0);
	va += 32;	
      }

#if 0
      // mba, dct
      for (i= 0; i< MBA_5_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((MBA_5_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, MBA_5_HW[i]);
      } 
      for (i= 0; i< MBA_11_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((MBA_11_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, MBA_11_HW[i]);
      } 
      for (i= 0; i< DC_lum_5_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((DC_lum_5_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DC_lum_5_HW[i]);
      } 
      for (i= 0; i< DC_long_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((DC_long_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DC_long_HW[i]);
      } 
      for (i= 0; i< DC_chrom_5_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((DC_chrom_5_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DC_chrom_5_HW[i]);
      } 
      for (i= 0; i< DCT_B15_8_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((DCT_B15_8_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_B15_8_HW[i]);
      } 
      for (i= 0; i< DCT_B15_10_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((DCT_B15_10_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_B15_10_HW[i]);
      } 
      for (i= 0; i< DCT_13_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((DCT_13_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_13_HW[i]);
      } 
      for (i= 0; i< DCT_15_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((DCT_15_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_15_HW[i]);
      } 
      for (i= 0; i< DCT_16_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((DCT_16_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_16_HW[i]);
      } 
      for (i= 0; i< DCT_B14AC_5_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((DCT_B14AC_5_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_B14AC_5_HW[i]);
      } 
      for (i= 0; i< DCT_B14DC_5_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((DCT_B14DC_5_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_B14DC_5_HW[i]);
      } 
      for (i= 0; i< DCT_B14_8_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((DCT_B14_8_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_B14_8_HW[i]);
      } 
      for (i= 0; i< DCT_B14_10_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((DCT_B14_10_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DCT_B14_10_HW[i]);
      } 
#else
      chn += 0x300;
#endif

      va = chn;
      // cbp
      if (s->coding_type != I_TYPE) {
	for (i= 0; i< CBP_7_LEN; i++){
	  uint32_t idx= REG_SDE_CTX_TBL+ ((CBP_7_BASE+ i)<<2);
	  GEN_VDMA_ACFG(chn, idx, 0, CBP_7_HW[i]);
	} 
	for (i= 0; i< CBP_9_LEN; i++){
	  uint32_t idx= REG_SDE_CTX_TBL+ ((CBP_9_BASE+ i)<<2);
	  GEN_VDMA_ACFG(chn, idx, 0, CBP_9_HW[i]);
	}       
      }
      for (i = 0; i < ((int)chn - va + 32) / 32; i++){
	i_cache(0x19, va, 0);
	va += 32;	
      }

#if 0
      // mv
      for (i= 0; i< MV_4_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((MV_4_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, MV_4_HW[i]);
      }       
      for (i= 0; i< MV_10_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((MV_10_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, MV_10_HW[i]);
      }       
      for (i= 0; i< DMV_2_LEN; i++){
	uint32_t idx= REG_SDE_CTX_TBL+ ((DMV_2_BASE+ i)<<2);
	GEN_VDMA_ACFG(chn, idx, 0, DMV_2_HW[i]);
      }       
#endif
    }
#if 0    
    for (i= 0; i< 16; i++){ /*fill scan_ram*/
      uint32_t idx= REG_SDE_CTX_TBL+ 0x1000+ (i<<2);
      GEN_VDMA_ACFG(chn, idx, 0, s->scan[i]);
    }
#else
    chn += 0x46;
#endif
     
    va = chn;
    GEN_VDMA_ACFG(chn, REG_SDE_CFG2, 0, bs->buffer);
    GEN_VDMA_ACFG(chn, REG_SDE_CFG1, 0, bs->bit_ofst);
    GEN_VDMA_ACFG(chn, REG_SDE_CFG0, 0, GET_SL_INFO(s->coding_type, /*coding_type*/ 
						    s->fr_pred_fr_dct, /*fr_pred_fr_dct*/
						    s->pic_type, /*pic_type*/
						    s->conceal_mv, /*conceal_mv*/
						    s->intra_dc_pre, /*intra_dc_pre*/
						    s->intra_vlc_format, /*intra_vlc_format*/
						    s->mpeg1, /*mpeg1*/
						    s->top_fi_first, /*top_fi_first*/ 
						    s->q_scale_type, /*q_scale_type*/
						    s->sec_fld,
						    s->f_code[0][0], /*f_code_f0*/
						    s->f_code[0][1], /*f_code_f1*/
						    s->f_code[1][0], /*f_code_b0*/
						    s->f_code[1][1] /*f_code_b1*/));
    GEN_VDMA_ACFG(chn, REG_SDE_CFG3, 0, s->qs_code);
    GEN_VDMA_ACFG(chn, REG_SDE_CFG8, 0, TCSM1_MOTION_DHA);
    GEN_VDMA_ACFG(chn, REG_SDE_CFG11, 0, TCSM1_MSRC_BUF);
    GEN_VDMA_ACFG(chn, REG_SDE_CFG9, 0, VMAU_CHN_BASE);
    GEN_VDMA_ACFG(chn, REG_SDE_CFG10, 0, DBLK_CHN_BASE);
    GEN_VDMA_ACFG(chn, REG_SDE_SL_CTRL, 0, SDE_SLICE_INIT);
    
    GEN_VDMA_ACFG(chn, REG_SDE_GL_CTRL, 0, (SDE_MODE_AUTO | SDE_EN) );
    GEN_VDMA_ACFG(chn, REG_SDE_SL_GEOM, 0, SDE_SL_GEOM(s->mb_height, s->mb_width, 
						       s->mb_pos_y, s->mb_pos_x) );
    GEN_VDMA_ACFG(chn, REG_SDE_SL_CTRL, VDMA_ACFG_TERM, SDE_MB_RUN);
  }
  for (i = 0; i < 4; i++){
    i_cache(0x19, va, 0);
    va += 32;
  }
  /*-------------------------SDE init end-------------------------------*/
}

#endif
