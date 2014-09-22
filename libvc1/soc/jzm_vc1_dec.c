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

#ifndef __JZM_VC1_API__
#define __JZM_VC1_API__

#include "jzm_vc1_data.h"

#ifdef VPU_STEP_MODE
#include "t_sde.h"
#endif

void jzm_vc1_frm_init_vdma(struct JZM_VC1 * st_vc1)
{
    int i;
    volatile unsigned int *chn = (volatile unsigned int *)st_vc1->des_va;
    /*   GEN_VDMA_ACFG(chn, REG_SCH_GLBC, 0,  */
    /*                (SCH_INTE_ACFGERR | SCH_INTE_BSERR | SCH_INTE_ENDF) ); */
    /*   GEN_VDMA_ACFG(chn, VPU_V_BASE+VPU_GLBC, 0, */
    /* 		GET_VPU_GLBC() | GLBC_INTE_ENDF | GLBC_INTE_BSERR | GLBC_INTE_TLBERR | GLBC_INTE_TIMEOUT | (1<<14) */
    /* 		); */
    
    GEN_VDMA_ACFG(chn, TCSM_FLUSH, 0, 0x0);
    /*------------------------------------------------------
      scheduler
      ------------------------------------------------------*/
    GEN_VDMA_ACFG(chn, REG_SCH_SCHC, 0, 0x0);
    if (st_vc1->pict_type == FF_I_TYPE) {
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
      motion
      ------------------------------------------------------*/
    //motion_init_vdma(VC1_QPEL, H264_EPEL);
    int intpid  = VC1_QPEL;
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
  
    //motion_config_vc1_its_hw_vdma(st_vc1);
    uint8_t  its_scale = 0;
    uint16_t its_rnd_y = 0;
    uint16_t its_rnd_c = 0;
    int scale, shift;
    if(!st_vc1->lumscale) {
        scale = -64;
        shift = (255 - st_vc1->lumshift * 2) << 6;
        if(st_vc1->lumshift > 31)
            shift += 128 << 6;
    } else {
        scale = st_vc1->lumscale + 32;
        if(st_vc1->lumshift > 31)
            shift = (st_vc1->lumshift - 64) << 6;
        else
            shift = st_vc1->lumshift << 6;
    }
    its_scale = scale;
    its_rnd_y = shift + 32;
    its_rnd_c = 128*(64-scale) + 32;
    GEN_VDMA_ACFG(chn, (REG_MCE_CH1_PINFO), 0, MCE_PINFO(0, 0, 6, its_scale, its_rnd_y));
    GEN_VDMA_ACFG(chn, (REG_MCE_CH2_PINFO), 0, MCE_PINFO(0, 0, 6, its_scale, its_rnd_c));
    
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_RLUT+(0)*8, 0, 0);
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_RLUT+(0)*8+4, 0, st_vc1->last_picture_data_y);
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_RLUT+(16)*8, 0, 0);
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_RLUT+(16)*8+4, 0, st_vc1->next_picture_data_y);
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_RLUT+(0)*8, 0, 0);
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_RLUT+(0)*8+4, 0, st_vc1->last_picture_data_uv);
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_RLUT+(16)*8, 0, 0);
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_RLUT+(16)*8+4, 0, st_vc1->next_picture_data_uv);
    
    GEN_VDMA_ACFG(chn, (REG_MCE_CH1_WINFO), 0, MCE_WINFO(0, 0, IS_BIAVG, 1, 0, 0, 0, 0));
    GEN_VDMA_ACFG(chn, (REG_MCE_CH1_WTRND), 0, MCE_WTRND(0, 0));
    GEN_VDMA_ACFG(chn, (REG_MCE_CH2_WINFO1), 0, MCE_WINFO(0, 0, IS_BIAVG, 1, 0, 0, 0, 0));
    GEN_VDMA_ACFG(chn, (REG_MCE_CH2_WINFO2), 0, MCE_WINFO(0, 0, 0, 0, 0, 0, 0, 0));
    GEN_VDMA_ACFG(chn, (REG_MCE_CH2_WTRND), 0, MCE_WTRND(0, 0));
    
    GEN_VDMA_ACFG(chn, REG_MCE_CH1_STRD, 0, MCE_STRD(st_vc1->linesize, 0, DOUT_Y_STRD)); 
    GEN_VDMA_ACFG(chn, REG_MCE_GEOM, 0, MCE_GEOM(st_vc1->mb_height*16,st_vc1->mb_width*16)); 
    GEN_VDMA_ACFG(chn, REG_MCE_CH2_STRD, 0, MCE_STRD(st_vc1->linesize, 0, DOUT_C_STRD)); 
    
    GEN_VDMA_ACFG(chn, REG_MCE_DSA, 0, VPU_BASE + REG_SCH_SCHE1); 
    GEN_VDMA_ACFG(chn, REG_MCE_DDC, 0, MC_DESP_ADDR); 
    
    /*------------------------------------------------------
      vmau
      ------------------------------------------------------*/
    const uint8_t wmv3_dc_scale_table[32]={
        0, 2, 4, 8, 8, 8, 9, 9,10,10,11,11,12,12,13,13,14,14,15,15,16,16,17,17,18,18,19,19,20,20,21,21
    };
    unsigned int qt_ram_addr = REG_VMAU_QT;
    unsigned int * tbl_ptr = (unsigned int *) wmv3_dc_scale_table;
    for ( i = 0 ; i < 8; i++) {
        GEN_VDMA_ACFG(chn, (qt_ram_addr+i*4), 0, tbl_ptr[i]);
    }
    for ( i = 0 ; i < 8; i++) {
        GEN_VDMA_ACFG(chn, (qt_ram_addr+i*4+32), 0, tbl_ptr[i]);
    }
    GEN_VDMA_ACFG(chn, REG_VMAU_GBL_RUN, 0, VMAU_RESET);
    GEN_VDMA_ACFG(chn, REG_VMAU_VIDEO_TYPE, 0, VMAU_FMT_VC1);
    GEN_VDMA_ACFG(chn, REG_VMAU_NCCHN_ADDR, 0, VMAU_DESP_ADDR);
    GEN_VDMA_ACFG(chn, REG_VMAU_DEC_DONE, 0, VPU_BASE + REG_SCH_SCHE2);
    GEN_VDMA_ACFG(chn, REG_VMAU_Y_GS, 0, st_vc1->mb_width*16);
    GEN_VDMA_ACFG(chn, REG_VMAU_GBL_CTR, 0, (VMAU_CTRL_FIFO_M | VMAU_CTRL_TO_DBLK));
    GEN_VDMA_ACFG(chn, REG_VMAU_DEC_STR, 0, (16<<16)|16);

    /*------------------------------------------------------
      dblk
      ------------------------------------------------------*/
    GEN_VDMA_ACFG(chn, REG_DBLK_TRIG, 0, DBLK_RESET);
    GEN_VDMA_ACFG(chn, REG_DBLK_DHA, 0, DBLK_DESP_ADDR);
    GEN_VDMA_ACFG(chn, REG_DBLK_GENDA, 0, VPU_BASE + REG_SCH_SCHE3);
    GEN_VDMA_ACFG(chn, REG_DBLK_GSIZE, 0, st_vc1->mb_width | (st_vc1->mb_height << 16));
    GEN_VDMA_ACFG(chn, REG_DBLK_GPOS, 0, 0);
    GEN_VDMA_ACFG(chn, REG_DBLK_GPIC_YA, 0, st_vc1->curr_picture_data_y);
    GEN_VDMA_ACFG(chn, REG_DBLK_GPIC_CA, 0, st_vc1->curr_picture_data_uv);
    GEN_VDMA_ACFG(chn, REG_DBLK_GP_ENDA, 0, DBLK_GP_ENDF_BASE);
    //GEN_VDMA_ACFG(chn, REG_DBLK_SLICE_ENDA, 0, VRAM_DBLK_SLC_ENDA);
    GEN_VDMA_ACFG(chn, REG_DBLK_VTR, 0, DBLK_FMT_VC1);
    GEN_VDMA_ACFG(chn, REG_DBLK_GPIC_STR, 0, (st_vc1->mb_width*256) | (st_vc1->mb_width*128)<<16);
    GEN_VDMA_ACFG(chn, REG_DBLK_TRIG, 0, DBLK_SLICE_RUN);

    GEN_VDMA_ACFG(chn, REG_DBLK_CTRL, 0, 0x0);
    //write_reg(DBLK_GP_ENDF_BASE, -1);
    
    /*------------------------------------------------------
      sde
      ------------------------------------------------------*/
    GEN_VDMA_ACFG(chn, REG_SDE_STAT, 0, 0);
    GEN_VDMA_ACFG(chn, REG_SDE_SL_GEOM, 0, SDE_SL_GEOM(st_vc1->mb_height,st_vc1->mb_width,0,0));
#ifdef VPU_STEP_MODE
    GEN_VDMA_ACFG(chn, REG_SDE_GL_CTRL, 0, 0x11);
#else
    GEN_VDMA_ACFG(chn, REG_SDE_GL_CTRL, 0, 1);
#endif
    GEN_VDMA_ACFG(chn, REG_SDE_CODEC_ID, 0, (1 << 3));
    
    // configure hw register 0
    int frm_adv              = st_vc1->profile_advanced;
    int frm_type             = (( (st_vc1->pict_type == FF_I_TYPE) || ((st_vc1->pict_type == FF_B_TYPE) && st_vc1->bi_type) ) ? 1 :
                                (st_vc1->pict_type == FF_P_TYPE) ? 2 :
                                (st_vc1->pict_type == FF_B_TYPE) ? 4 : 0);
    int frm_pq               = st_vc1->pq;
    int frm_altpq            = st_vc1->altpq;
    int frm_use_ic           = st_vc1->use_ic;
    int frm_halfpq           = st_vc1->halfpq;
    int frm_dquantfrm        = st_vc1->dquantfrm;
    int frm_dqprofile_all_mb = st_vc1->dqprofile == DQPROFILE_ALL_MBS;
    int frm_dqbilevel        = st_vc1->dqbilevel;
    int frm_edge             = st_vc1->frm_edge;
    int frm_codingset        = st_vc1->frm_codingset;
    int frm_codingset2       = st_vc1->frm_codingset2;
    int frm_no_dir_mv        = st_vc1->frm_no_dir_mv;
    //printf(" frm_no_dir_mv:%d \n",frm_no_dir_mv);
    int cfg_0;
    cfg_0 = ( ((frm_adv              & ((1<<1) - 1)) << 0) +
              ((frm_type             & ((1<<3) - 1)) << 1) +
              ((frm_pq               & ((1<<5) - 1)) << 4) +
              ((frm_altpq            & ((1<<5) - 1)) << 9) +
              ((frm_use_ic           & ((1<<1) - 1)) << 15) +
              ((frm_halfpq           & ((1<<1) - 1)) << 16) +
              ((frm_dquantfrm        & ((1<<1) - 1)) << 17) +
              ((frm_dqprofile_all_mb & ((1<<1) - 1)) << 18) +
              ((frm_dqbilevel        & ((1<<1) - 1)) << 19) +
              ((frm_edge             & ((1<<4) - 1)) << 20) +
              ((frm_codingset        & ((1<<3) - 1)) << 24) +
              ((frm_codingset2       & ((1<<3) - 1)) << 27) +
              ((frm_no_dir_mv        & ((1<<1) - 1)) << 30) +
              0 );
    GEN_VDMA_ACFG(chn, REG_SDE_CFG0, 0, cfg_0);

    // configure hw register 1
    unsigned int bs_addr = st_vc1->bs_buffer;
    unsigned int bs_ofst = st_vc1->bs_index;
    int frm_mvrange          = st_vc1->mvrange;
    int frm_quarter_sample   = st_vc1->quarter_sample;
    int frm_ttmbf            = st_vc1->ttmbf;
    int frm_ttfrm            = st_vc1->ttfrm;
    int frm_dmb_is_raw       = st_vc1->dmb_is_raw;
    int frm_mv_type_is_raw   = st_vc1->mv_type_is_raw;
    int frm_skip_is_raw      = st_vc1->skip_is_raw;
    int frm_acpred_is_raw    = st_vc1->acpred_is_raw;
    int frm_overflg_is_raw   = st_vc1->overflg_is_raw;
    int frm_condover_select  = st_vc1->condover == CONDOVER_SELECT;
    int frm_condover_all     = st_vc1->condover == CONDOVER_ALL;
    int frm_overlap          = st_vc1->overlap;
    int frm_bfraction        = st_vc1->bfraction;
    int frm_fastuvmc         = st_vc1->fastuvmc;
    int cfg_1;
    cfg_1 = ( ((frm_mvrange          & ((1<<2) - 1)) << 0) +
              ((frm_quarter_sample   & ((1<<1) - 1)) << 2) +
              ((frm_ttmbf            & ((1<<1) - 1)) << 3) +
              ((frm_ttfrm            & ((1<<4) - 1)) << 4) +
              ((frm_dmb_is_raw       & ((1<<1) - 1)) << 8) +
              ((frm_mv_type_is_raw   & ((1<<1) - 1)) << 9) +
              ((frm_skip_is_raw      & ((1<<1) - 1)) << 10) +
              ((frm_acpred_is_raw    & ((1<<1) - 1)) << 11) +
              ((frm_overflg_is_raw   & ((1<<1) - 1)) << 12) +
              ((frm_condover_select  & ((1<<1) - 1)) << 13) +
              ((frm_condover_all     & ((1<<1) - 1)) << 14) +
              ((frm_overlap          & ((1<<1) - 1)) << 15) +
              ((frm_bfraction        & ((1<<9) - 1)) << 16) +
              ((bs_ofst              & ((1<<5) - 1)) << 26) +
              ((frm_fastuvmc         & ((1<<1) - 1)) << 31) +
              0 );
    GEN_VDMA_ACFG(chn, REG_SDE_CFG1, 0, cfg_1);
    
    // configure hw register 2
    int frm_mb_width         = st_vc1->mb_width;
    int frm_mb_height        = st_vc1->mb_height;
    int frm_mv_table_index   = st_vc1->mv_table_index;
    int frm_dc_table_index   = st_vc1->dc_table_index;
    int frm_tt_index         = st_vc1->tt_index;
    int frm_cbpcy_index      = st_vc1->cbpcy_table_index;
    // vmau
    int frm_rangeredfrm      = !!st_vc1->rangeredfrm;
    int frm_pqgt9_overlap    = st_vc1->pq >= 9 && st_vc1->overlap;
    int frm_res_fasttx       = st_vc1->res_fasttx;
    int frm_res_x8           = st_vc1->res_x8;
    int frm_pquantizer       = st_vc1->pquantizer;
    int frm_flag_gray        = 0;
    int frm_mv_mode_its      = st_vc1->mv_mode_its;
    int frm_mspel            = st_vc1->mspel;
    int cfg_2;
    cfg_2 = ( ((frm_mb_width         & ((1<<7) - 1)) << 0) +
              ((frm_mb_height        & ((1<<7) - 1)) << 8) +
              ((frm_mv_table_index   & ((1<<2) - 1)) << 16) +
              ((frm_dc_table_index   & ((1<<1) - 1)) << 18) +
              ((frm_tt_index         & ((1<<2) - 1)) << 20) +
              ((frm_cbpcy_index      & ((1<<2) - 1)) << 22) +
              ((frm_rangeredfrm      & ((1<<1) - 1)) << 24) +
              ((frm_pqgt9_overlap    & ((1<<1) - 1)) << 25) +
              ((frm_res_fasttx       & ((1<<1) - 1)) << 26) +
              ((frm_res_x8           & ((1<<1) - 1)) << 27) +
              ((frm_pquantizer       & ((1<<1) - 1)) << 28) +
              ((frm_flag_gray        & ((1<<1) - 1)) << 29) +
              ((frm_mv_mode_its      & ((1<<1) - 1)) << 30) +
              ((frm_mspel            & ((1<<1) - 1)) << 31) +
              0 );
    GEN_VDMA_ACFG(chn, REG_SDE_CFG2, 0, cfg_2);
    
    GEN_VDMA_ACFG(chn, REG_SDE_CFG3, 0, bs_addr);
    GEN_VDMA_ACFG(chn, REG_SDE_CFG4, 0, RESIDUAL_DOUT_ADDR);
    GEN_VDMA_ACFG(chn, REG_SDE_CFG5, 0, FRM_PLANE_ADDR);
    GEN_VDMA_ACFG(chn, REG_SDE_CFG6, 0, TOP_NEI_ADDR);
    GEN_VDMA_ACFG(chn, REG_SDE_CFG7, 0, MC_DESP_ADDR);
    GEN_VDMA_ACFG(chn, REG_SDE_CFG8, 0, st_vc1->frm_mv_addr);
    GEN_VDMA_ACFG(chn, REG_SDE_CFG9, 0, VMAU_DESP_ADDR);
    GEN_VDMA_ACFG(chn, REG_SDE_CFG10, 0, DBLK_DESP_ADDR);
    
    { // set vlc tables
        unsigned int * hw_tbl_p;
        unsigned int * sw_tbl_p;
        int size;
#define AC_COEF_TBL_0_POS (0)
#define AC_COEF_TBL_0_OFST (2 << 10)
        int ac_coef_0_pos = AC_COEF_TBL_0_POS + frm_codingset;
        hw_tbl_p = (unsigned int *)(REG_SDE_CTX_TBL + AC_COEF_TBL_0_OFST);
        sw_tbl_p = (unsigned int *)(&vc1_hw_table_all[ac_coef_0_pos][0]);
        size = vc1_tbl_info[ac_coef_0_pos][1] >> 1;
        for(i=0; i<size; i++)
            GEN_VDMA_ACFG(chn, hw_tbl_p+i, 0, sw_tbl_p[i]);
        
#define AC_COEF_TBL_1_POS (0)
#define AC_COEF_TBL_1_OFST (0)
        int ac_coef_1_pos = AC_COEF_TBL_1_POS + frm_codingset2;
        hw_tbl_p = (unsigned int *)(REG_SDE_CTX_TBL + AC_COEF_TBL_1_OFST);
        sw_tbl_p = (unsigned int *)(&vc1_hw_table_all[ac_coef_1_pos][0]);
        size = vc1_tbl_info[ac_coef_1_pos][1] >> 1;
        for(i=0; i<size; i++)
            GEN_VDMA_ACFG(chn, hw_tbl_p+i, 0, sw_tbl_p[i]);
        
        if (frm_type == 1) {
#define MB_I_TBL_POS (29)
#define MB_I_TBL_OFST ((4 << 10))
            hw_tbl_p = (unsigned int *)(REG_SDE_CTX_TBL + MB_I_TBL_OFST);
            sw_tbl_p = (unsigned int *)(&vc1_hw_table_all[MB_I_TBL_POS][0]);
            size = vc1_tbl_info[MB_I_TBL_POS][1] >> 1;
            for(i=0; i<size; i++)
                GEN_VDMA_ACFG(chn, hw_tbl_p+i, 0, sw_tbl_p[i]);
        } else {
            
#define TTMB_TBL_POS (8)
#define TTMB_TBL_OFST ((3 << 10))
            hw_tbl_p = (unsigned int *)(REG_SDE_CTX_TBL + TTMB_TBL_OFST);
            sw_tbl_p = (unsigned int *)(&vc1_hw_table_all[TTMB_TBL_POS + frm_tt_index][0]);
            size = vc1_tbl_info[TTMB_TBL_POS + frm_tt_index][1] >> 1;
            for(i=0; i<size; i++)
                GEN_VDMA_ACFG(chn, hw_tbl_p+i, 0, sw_tbl_p[i]);
            
#define SBPAT_TBL_POS (14)
#define SBPAT_TBL_OFST ((3 << 10) + (1 << 9))
            hw_tbl_p = (unsigned int *)(REG_SDE_CTX_TBL + SBPAT_TBL_OFST);
            sw_tbl_p = (unsigned int *)(&vc1_hw_table_all[SBPAT_TBL_POS + frm_tt_index][0]);
            size = vc1_tbl_info[SBPAT_TBL_POS + frm_tt_index][1] >> 1;
            for(i=0; i<size; i++)
                GEN_VDMA_ACFG(chn, hw_tbl_p+i, 0, sw_tbl_p[i]);

#define TTBLK_TBL_POS (11)
#define TTBLK_TBL_OFST ((3 << 10) + (1 << 9) + (1 << 8))
            hw_tbl_p = (unsigned int *)(REG_SDE_CTX_TBL + TTBLK_TBL_OFST);
            sw_tbl_p = (unsigned int *)(&vc1_hw_table_all[TTBLK_TBL_POS + frm_tt_index][0]);
            size = vc1_tbl_info[TTBLK_TBL_POS + frm_tt_index][1] >> 1;
            for(i=0; i<size; i++)
                GEN_VDMA_ACFG(chn, hw_tbl_p+i, 0, sw_tbl_p[i]);

#define CBPCY_TBL_POS (21)
#define CBPCY_TBL_OFST ((4 << 10))
            hw_tbl_p = (unsigned int *)(REG_SDE_CTX_TBL + CBPCY_TBL_OFST);
            sw_tbl_p = (unsigned int *)(&vc1_hw_table_all[CBPCY_TBL_POS + frm_cbpcy_index][0]);
            size = vc1_tbl_info[CBPCY_TBL_POS + frm_cbpcy_index][1] >> 1;
            for(i=0; i<size; i++)
                GEN_VDMA_ACFG(chn, hw_tbl_p+i, 0, sw_tbl_p[i]);
            
#define MVDIFF_TBL_POS (17)
#define MVDIFF_TBL_OFST ((4 << 10) + (1 << 9))
            hw_tbl_p = (unsigned int *)(REG_SDE_CTX_TBL + MVDIFF_TBL_OFST);
            sw_tbl_p = (unsigned int *)(&vc1_hw_table_all[MVDIFF_TBL_POS + frm_mv_table_index][0]);
            size = vc1_tbl_info[MVDIFF_TBL_POS + frm_mv_table_index][1] >> 1;
            for(i=0; i<size; i++)
                GEN_VDMA_ACFG(chn, hw_tbl_p+i, 0, sw_tbl_p[i]);
        }
        
#define DC_LUMA_TBL_POS (25)
#define DC_LUMA_TBL_OFST ((5 << 10))
        int dc_luma_pos = DC_LUMA_TBL_POS + (2 * frm_dc_table_index);
        hw_tbl_p = (unsigned int *)(REG_SDE_CTX_TBL + DC_LUMA_TBL_OFST);
        sw_tbl_p = (unsigned int *)(&vc1_hw_table_all[dc_luma_pos][0]);
        size = vc1_tbl_info[dc_luma_pos][1] >> 1;
        for(i=0; i<size; i++)
            GEN_VDMA_ACFG(chn, hw_tbl_p+i, 0, sw_tbl_p[i]);
        
#define DC_CHROMA_TBL_POS (26)
#define DC_CHROMA_TBL_OFST ((5 << 10) + (1 << 9))
        int dc_chroma_pos = DC_CHROMA_TBL_POS + (2 * frm_dc_table_index);
        hw_tbl_p = (unsigned int *)(REG_SDE_CTX_TBL + DC_CHROMA_TBL_OFST);
        sw_tbl_p = (unsigned int *)(&vc1_hw_table_all[dc_chroma_pos][0]);
        size = vc1_tbl_info[dc_chroma_pos][1] >> 1;
        for(i=0; i<size; i++)
            GEN_VDMA_ACFG(chn, hw_tbl_p+i, 0, sw_tbl_p[i]);
        
        // set vc1 frame bit plane
        hw_tbl_p = (unsigned int *)FRM_PLANE_ADDR;
        sw_tbl_p = (unsigned int *)st_vc1->frm_plane;
        size = ((st_vc1->mb_width * st_vc1->mb_height) >> 4) + 1;
        for(i=0; i<size; i++)
            GEN_VDMA_ACFG(chn, hw_tbl_p+i, 0, sw_tbl_p[i]);
    }
    
#ifdef VPU_STEP_MODE
#else
    GEN_VDMA_ACFG(chn, (REG_SDE_SL_CTRL), VDMA_ACFG_TERM, SDE_MB_RUN);
#endif

}

#endif // __JZM_VC1_API__
