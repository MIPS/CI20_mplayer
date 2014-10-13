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

#ifdef __T_MEM_DEF_FALCON_H__
#else
#define __T_MEM_DEF_FALCON_H__
#ifdef P1_USE_PADDR
#include "eyer_driver_aux.h"
#else
#include "eyer_driver.h"
#endif
#define MAU_SRC_TCSM1_OPT
//--++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#define MAU_SFIFO_DEP 8
#define DBK_SFIFO_DEP MAU_SFIFO_DEP
#define P0_AUX_FIFO_DEP MAU_SFIFO_DEP

#define MAU_DFIFO_DEP (MAU_SFIFO_DEP+1)

#define AUX_END_VALUE 0x5a5a
/*-------------TCSM0 --------------------*/
#define TCSM0_1VALUE ((unsigned int)C_TCSM0_BASE + 16)
#define GP0_DHA_BASE (TCSM0_1VALUE+4)
#define GP0_DHA_SIZE (4*4)
#define P1_START_BASE (GP0_DHA_BASE+GP0_DHA_SIZE)
#define TCSM0_END (P1_START_BASE+4)

/*-------------TCSM1 --------------*/
#define AUX_PRO_BASE ((unsigned int)C_TCSM1_BASE)
#define AUX_PRO_SIZE (0x2800)

#define MOTION_IWTA_BASE (AUX_PRO_BASE+AUX_PRO_SIZE)
#define MOTION_IWTA_SIZE (256)
//#define MOTION_IWTA_BASE ((DBLK_DESP_ADDR+DBLK_DESP_SIZE + 1024) & (~3))  same ass        jzm_h264_dec.h
//#define MOTION_IWTA_SIZE (2048)                                          same as  jzm_h264_dec.h

#define MOTION_DSA_BASE (MOTION_IWTA_BASE + MOTION_IWTA_SIZE)
#define MOTION_CHN_FIFO_ENTRY (MAU_SFIFO_DEP)
#define MOTION_BUF_BASE (MOTION_DSA_BASE + 4)
#define MOTION_CHN_FIFO_BASE (MOTION_BUF_BASE)
#define MOTION_CHN_SIZE (70*4)
#define MOTION_CHN_FIFO_SIZE (MOTION_CHN_FIFO_ENTRY * MOTION_CHN_SIZE)
#define MOTION_BUF_SIZE  (MOTION_CHN_FIFO_SIZE)//0x200

#define MAU_ENDF_BASE (MOTION_CHN_FIFO_SIZE+MOTION_BUF_BASE)//(MAU_SRC_BASE + MAU_SRC_SIZE)

#define MAU_CHN_BASE ((unsigned int)MAU_ENDF_BASE+4)
#define MAU_CHN_ONE_SIZE (10*sizeof(int))
#define MAU_CHN_SIZE (MAU_CHN_ONE_SIZE*MAU_SFIFO_DEP)

#define SDE_TCSM1_BASE ((unsigned int)MAU_CHN_BASE + MAU_CHN_SIZE)
#define SDE_SYNC_DOUT_ADDR      (SDE_TCSM1_BASE) //30

#if 0
#define SDE_NEIGHBOR_DIN_ADDR   (SDE_SYNC_DOUT_ADDR + 4) //27
#define SDE_NEIGHBOR_DIN_SIZE   (27*4)
#define SDE_CTRL_DOUT_ADDR      (SDE_NEIGHBOR_DIN_ADDR + SDE_NEIGHBOR_DIN_SIZE )//0x400) //80
#define SDE_CTRL_DOUT_SIZE      (76*4)
#define SDE_NEIGHBOR_DIN_ADDR1  (SDE_CTRL_DOUT_ADDR + SDE_CTRL_DOUT_SIZE) //27
#define SDE_CTRL_DOUT_ADDR1     (SDE_NEIGHBOR_DIN_ADDR1 + SDE_NEIGHBOR_DIN_SIZE )//0x400) //80
#define MAU_QP_BASE (SDE_CTRL_DOUT_ADDR1+SDE_CTRL_DOUT_SIZE)
#else
#define SDE_NEIGHBOR_DIN_ADDR   (SDE_SYNC_DOUT_ADDR + 4) //27
#define SDE_NEIGHBOR_DIN_SIZE   (27*4)
#define SDE_CTRL_DOUT_ADDR      (SDE_NEIGHBOR_DIN_ADDR + 16*SDE_NEIGHBOR_DIN_SIZE )//0x400) //80
#define SDE_CTRL_DOUT_SIZE      (76*4)
#define SDE_CTRL_DOUT_ONE_SIZE  (76*4)
#define SDE_NEIGHBOR_DIN_ADDR1  (SDE_NEIGHBOR_DIN_ADDR + SDE_NEIGHBOR_DIN_SIZE) //27
#define SDE_CTRL_DOUT_ADDR1     (SDE_CTRL_DOUT_ADDR + SDE_CTRL_DOUT_SIZE)//0x400) //80
#define MAU_QP_BASE (SDE_CTRL_DOUT_ADDR+16*SDE_CTRL_DOUT_SIZE)
#endif

#define MAU_QP_SIZE (4)
#define CHR_QT_TAB_BASE (MAU_QP_BASE+MAU_QP_SIZE)
#define CHR_QT_TAB_SIZE (256*2)

#define DBLK_CHAIN_INIT_BASE (CHR_QT_TAB_BASE + CHR_QT_TAB_SIZE )
#define DBLK_CHAIN_INIT_SIZE (39*4)// words : a data node  

#define DBLK_CHAIN_WD 15 //a data node  + two address node 
#define DBLK_CHAIN_ONE_SIZE (DBLK_CHAIN_WD*4)

#define DBLK_CHAIN0_P_BASE (DBLK_CHAIN_INIT_BASE + DBLK_CHAIN_INIT_SIZE )
#define DBLK_CHAIN1_P_BASE (DBLK_CHAIN0_P_BASE + DBLK_CHAIN_ONE_SIZE*DBK_SFIFO_DEP)
#define DBLK_CHAIN_SIZE (DBLK_CHAIN_ONE_SIZE*2*DBK_SFIFO_DEP)

#define GP2_SFIFO_DEP DBK_SFIFO_DEP
#define GP2_DHA_BASE (DBLK_CHAIN0_P_BASE + DBLK_CHAIN_SIZE)
#define GP2_DHA_ONE_SIZE ((4*(6+3+3))*4) // Y, U, V, dha, end, endf
#define GP2_DHA_SPEC_ONE_SIZE ((4*(6+3+3))*4) // Y, U, V, dha, end, endf
#define GP2_DHA_SIZE (GP2_DHA_ONE_SIZE*(GP2_SFIFO_DEP-1)+GP2_DHA_SPEC_ONE_SIZE)
#define GP2_ENDF_BASE  (GP2_DHA_BASE + GP2_DHA_SIZE)

#define GP2_OUT_Y_BASE (GP2_ENDF_BASE + 4)
#define GP2_OUT_UV_BASE (GP2_OUT_Y_BASE + 4)

#define P2AUX_WIDX_BASE  ((unsigned int)GP2_OUT_UV_BASE + 4) //0
#define P2AUX_RIDX_BASE  (P2AUX_WIDX_BASE+4) //1
#define P2AUX_FIFO_DEP   1
#define AUX_START_BASE  (P2AUX_RIDX_BASE + 4) //2

#define AUX_SLICE_INFO_BASE ((unsigned int)P2AUX_WIDX_BASE+20)
#define AUX_SLICE_INFO_SIZE ((13)*4)

#define SDE_NEIGHBOR_INFO_TOP_BASE (AUX_SLICE_INFO_BASE+AUX_SLICE_INFO_SIZE)
#define SDE_NEIGHBOR_INFO_TOP_SIZE (sizeof(NEIGHBOR_INFO)*(128))

#define DBLK_MV_ADDR_V_BASE (SDE_NEIGHBOR_INFO_TOP_BASE+SDE_NEIGHBOR_INFO_TOP_SIZE)
#define DBLK_MV_SIZE (56 * 4 * MAU_SFIFO_DEP) //4 words ref , 1 mb_type , 48 words mv_cache

#define GP1_DHA_BASE (DBLK_MV_ADDR_V_BASE + DBLK_MV_SIZE)
#define GP1_DHA_SIZE (4*4)

#define TCSM1_1VALUE (GP1_DHA_BASE + GP1_DHA_SIZE)


#ifdef MAU_SRC_TCSM1_OPT
#define MAU_SRC_BASE (TCSM1_1VALUE + 4)
#define MAU_SRC_ONE_SIZE (((24+1)*16+(2*4))*(sizeof(short)))
#define MAU_SRC_SIZE (MAU_SRC_ONE_SIZE*2*MAU_SFIFO_DEP)

#define SDE_SRAM_BASE (MAU_SRC_BASE)
#define SDE_ERR_SIZE (MAU_SRC_SIZE)
#define TCSM1_END (MAU_SRC_BASE + MAU_SRC_SIZE)
#else
#define TCSM1_END (TCSM1_1VALUE + 4)
#endif

#if 1
#define TCSM1_CTRL_FIFO (TCSM1_END + 4)
#endif  //by xqliang

 
/*------------SRAM----------------*/
#define SRAM_1VALUE_BASE ((unsigned int)C_SRAM_BASE)

#define DBLK_YDST_BUF (SRAM_1VALUE_BASE + 4)
#define DBLK_YDST_BUF_SIZE ((256)*MAU_DFIFO_DEP+4*16)
#define DBLK_YDST_BASE (DBLK_YDST_BUF + 4)

#define DBLK_YDST_STR  (16*MAU_DFIFO_DEP+4)
#define MAU_YDST_BASE (DBLK_YDST_BASE+16)
#define MAU_YDST_STR  (DBLK_YDST_STR)

#define DBLK_UDST_BUF (DBLK_YDST_BUF+DBLK_YDST_BUF_SIZE)
#define DBLK_UDST_BUF_SIZE ((64)*MAU_DFIFO_DEP+4*8)
#define DBLK_UDST_BASE (DBLK_UDST_BUF+4)
#define DBLK_UDST_STR  (8*MAU_DFIFO_DEP+4)
#define MAU_UDST_BASE (DBLK_UDST_BASE+8)
#define MAU_UVDST_STR  (DBLK_UDST_STR)

#define DBLK_VDST_BUF (DBLK_UDST_BUF+DBLK_UDST_BUF_SIZE)
#define DBLK_VDST_BUF_SIZE ((64)*MAU_DFIFO_DEP+4*8)
#define DBLK_VDST_BASE (DBLK_VDST_BUF+4)
#define MAU_VDST_BASE (DBLK_VDST_BASE+8)

#define GP2_DHA_VEC (DBLK_VDST_BUF + DBLK_VDST_BUF_SIZE)
#define GP2_DHA_VEC_SIZE (4*GP2_SFIFO_DEP)

#define GP2_INDEX_BASE (GP2_DHA_VEC+GP2_DHA_VEC_SIZE)
#define GP2_INDEX_SIZE (GP2_SFIFO_DEP*4)

#ifdef MAU_SRC_TCSM1_OPT
#define DBLK_BUF_BASE (GP2_INDEX_BASE+GP2_INDEX_SIZE)
#else
#define MAU_SRC_BASE (GP2_INDEX_BASE+GP2_INDEX_SIZE)
#define MAU_SRC_ONE_SIZE (((24+1)*16+(2*4))*(sizeof(short)))
#define MAU_SRC_SIZE (MAU_SRC_ONE_SIZE*MAU_SFIFO_DEP)

#define SDE_SRAM_BASE (MAU_SRC_BASE)
#define SDE_ERR_SIZE (MAU_SRC_SIZE)
#define DBLK_BUF_BASE (MAU_SRC_BASE+MAU_SRC_SIZE)
#endif

#define DBLK_Y_UPL_BASE (DBLK_BUF_BASE)
#define DBLK_Y_UPL_SIZE (4*1920+4*4)

#define DBLK_U_UPL_BASE (DBLK_Y_UPL_BASE+DBLK_Y_UPL_SIZE)
#define DBLK_U_UPL_SIZE (4*1920/2+4*4)

#define DBLK_V_UPL_BASE (DBLK_U_UPL_BASE+DBLK_U_UPL_SIZE)
#define DBLK_V_UPL_SIZE (4*1920/2+4*4)

#define DBLK_Y_UPOUT_BASE (DBLK_V_UPL_BASE+DBLK_V_UPL_SIZE)
#define DBLK_Y_UPOUT_SIZE (16*4*MAU_DFIFO_DEP)

#define DBLK_U_UPOUT_BASE (DBLK_Y_UPOUT_BASE+DBLK_Y_UPOUT_SIZE)
#define DBLK_U_UPOUT_SIZE (8*4*MAU_DFIFO_DEP)

#define DBLK_V_UPOUT_BASE (DBLK_U_UPOUT_BASE+DBLK_U_UPOUT_SIZE)
#define DBLK_V_UPOUT_SIZE (8*4*MAU_DFIFO_DEP)

//#define SRAM_END (DBLK_V_UPOUT_BASE+DBLK_V_UPOUT_SIZE)
#define DBLK_YEND_FLAG  (DBLK_V_UPOUT_BASE+DBLK_V_UPOUT_SIZE)
#define DBLK_CEND_FLAG  (DBLK_YEND_FLAG + 4)
#define SRAM_END        (DBLK_CEND_FLAG + 4)
//#define PMON_BUF (SRAM_END) 

#endif//__T_MEM_DEF_FALCON_H__


/*
  SDE
 */
