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
 * $Id: h264_cavlc_tcsm1.h,v 1.1.1.1 2012/03/27 04:02:56 dqliu Exp $
 *
 ****************************************************************************/

#ifndef __H264_TCSM1_H__
#define __H264_TCSM1_H__

#define TCSM1_BANK0 0xF4000000
#define TCSM1_BANK1 0xF4001000
#define TCSM1_BANK2 0xF4002000
#define TCSM1_BANK3 0xF4003000

#define TCSM1_BANK4 0xF4004000
#define TCSM1_BANK5 0xF4005000
#define TCSM1_BANK6 0xF4006000
#define TCSM1_BANK7 0xF4007000

#define TCSM1_PADDR(a)        ((((unsigned)(a)) & 0xFFFF) | 0x132C0000) 
#define TCSM1_VCADDR(a)       ((((unsigned)(a)) & 0xFFFF) | 0xB32C0000) 
#define TCSM1_VUCADDR(a)      ((((unsigned)(a)) & 0xFFFF) | 0xB32C0000) 
#define TCSM1_INNER_ADDR(a)   ((((unsigned)(a)) & 0xFFFF) | 0xF4000000) 

#define P1_MAIN_ADDR (TCSM1_BANK0)

#define TCSM1_SLICE_BUF0 (TCSM1_BANK4 + 0x100)
#define TCSM1_SLICE_BUF1 (TCSM1_SLICE_BUF0+(SLICE_T_CC_LINE*32))

#define TCSM1_CMD_LEN           (16 << 2)
#define TCSM1_MBNUM_WP          (TCSM1_SLICE_BUF1+(SLICE_T_CC_LINE*32))
#define TCSM1_MBNUM_RP          (TCSM1_MBNUM_WP+4)
#define TCSM1_ADDR_RP           (TCSM1_MBNUM_RP+4)
#define TCSM1_DCORE_SHARE_ADDR  (TCSM1_ADDR_RP+4)
#define TCSM1_FIRST_MBLEN       (TCSM1_DCORE_SHARE_ADDR+4)
#define TCSM1_DBG_RESERVE       (TCSM1_FIRST_MBLEN+4)
#define TCSM1_P1_STOP           TCSM1_DBG_RESERVE       
#define TCSM1_BUG_W             (TCSM1_P1_STOP + 4)
#define TCSM1_BUG_H             (TCSM1_BUG_W + 4)
#define TCSM1_BUG_TRIG          (TCSM1_BUG_H + 4)

#define P1_T_BUF_LEN (sizeof(struct H264_XCH2_T))

#define TCSM1_XCH2_T_BUF0 (TCSM1_MBNUM_WP + TCSM1_CMD_LEN)
#define TCSM1_XCH2_T_BUF1 (TCSM1_XCH2_T_BUF0+P1_T_BUF_LEN)

//#define TASK_BUF_LEN            (sizeof(struct H264_MB_Ctrl_DecARGs)+sizeof(struct H264_MB_InterB_DecARGs)+(296<<2)+(68<<1))//7+53+296+34 words
#define TASK_BUF_LEN            (sizeof(struct H264_MB_Ctrl_DecARGs)+sizeof(struct H264_MB_InterB_DecARGs)+(296<<2)+(12<<2))

#define TASK_BUF0               (TCSM1_XCH2_T_BUF1+P1_T_BUF_LEN)
#define TASK_BUF1               (TASK_BUF0+TASK_BUF_LEN)
#define TASK_BUF2               (TASK_BUF1+TASK_BUF_LEN)



#define PREVIOUS_LUMA_STRIDE    (4+16)
#define PREVIOUS_CHROMA_STRIDE  (4+8)
#define PREVIOUS_C_LEN          (PREVIOUS_CHROMA_STRIDE<<3)
#define PREVIOUS_OFFSET_U       ((PREVIOUS_LUMA_STRIDE<<4))
#define PREVIOUS_OFFSET_V       (PREVIOUS_OFFSET_U+(PREVIOUS_CHROMA_STRIDE<<3))
#define DBLK_LEFT_Y0            (TASK_BUF2+TASK_BUF_LEN)
#define RECON_PREVIOUS_YBUF0    (DBLK_LEFT_Y0+4)
#define DBLK_LEFT_U0            (DBLK_LEFT_Y0+(PREVIOUS_LUMA_STRIDE<<4))
#define RECON_PREVIOUS_UBUF0    (DBLK_LEFT_U0+4)
#define DBLK_LEFT_V0            (DBLK_LEFT_U0+(PREVIOUS_CHROMA_STRIDE<<3))
#define RECON_PREVIOUS_VBUF0    (DBLK_LEFT_V0+4)

#define DBLK_LEFT_Y1            (DBLK_LEFT_V0+(PREVIOUS_CHROMA_STRIDE<<3))
#define RECON_PREVIOUS_YBUF1    (DBLK_LEFT_Y1+4)
#define DBLK_LEFT_U1            (DBLK_LEFT_Y1+(PREVIOUS_LUMA_STRIDE<<4))
#define RECON_PREVIOUS_UBUF1    (DBLK_LEFT_U1+4)
#define DBLK_LEFT_V1            (DBLK_LEFT_U1+(PREVIOUS_CHROMA_STRIDE<<3))
#define RECON_PREVIOUS_VBUF1    (DBLK_LEFT_V1+4)

#define DBLK_LEFT_Y2            (DBLK_LEFT_V1+(PREVIOUS_CHROMA_STRIDE<<3))
#define RECON_PREVIOUS_YBUF2    (DBLK_LEFT_Y2+4)
#define DBLK_LEFT_U2            (DBLK_LEFT_Y2+(PREVIOUS_LUMA_STRIDE<<4))
#define RECON_PREVIOUS_UBUF2    (DBLK_LEFT_U2+4)
#define DBLK_LEFT_V2            (DBLK_LEFT_U2+(PREVIOUS_CHROMA_STRIDE<<3))
#define RECON_PREVIOUS_VBUF2    (DBLK_LEFT_V2+4)

#define IDCT_DES_CHAIN_LEN      (28<<2)
#define IDCT_DES_CHAIN          (DBLK_LEFT_V2+(PREVIOUS_CHROMA_STRIDE<<3)+4)
#define IDCT_END_FLAG           (IDCT_DES_CHAIN+IDCT_DES_CHAIN_LEN)
#define IDCT_SYN_VALUE          (0x1dc7)
#define DBLK_DES_CHAIN_LEN      (19<<2)
#define DBLK_MV_CHAIN_LEN       (53<<2)
#define DBLK_END_FLAG           (IDCT_END_FLAG+4)
#define DBLK_SYN_VALUE          (0xa5a5)
#define DBLK_MV_CHAIN0          (DBLK_END_FLAG+4)
#define DBLK_MV_CHAIN1          (DBLK_MV_CHAIN0+DBLK_MV_CHAIN_LEN)
#define DBLK_DES_CHAIN0         (DBLK_MV_CHAIN1+DBLK_MV_CHAIN_LEN)
#define DBLK_DES_CHAIN1         (DBLK_DES_CHAIN0+DBLK_DES_CHAIN_LEN)
//#define DDMA_GP0_DES_CHAIN_LEN  (4<<2)
#define DDMA_GP0_DES_CHAIN_LEN  (8<<2)
#define DDMA_GP0_DES_CHAIN      (DBLK_DES_CHAIN1+DBLK_DES_CHAIN_LEN)

//FIXME: Can remoev TCSM1_DMB_MV_BASE? it seems not in using
#ifdef JZC_DBLKLI_OPT
//#define TCSM1_DMB_MV_BASE (DDMA_GP0_DES_CHAIN+DDMA_GP0_DES_CHAIN_LEN)
//#define TCSM1_DMB_MV_LEN ( sizeof(struct H264_MB_InterB_DecARGs) + 24 )
#else
#define DDMA_GP2_DES_CHAIN_LEN  ((4*6)<<2)
#define DDMA_GP2_DES_CHAIN0     (DDMA_GP0_DES_CHAIN+DDMA_GP0_DES_CHAIN_LEN)
#define DDMA_GP2_DES_CHAIN1     (DDMA_GP2_DES_CHAIN0+DDMA_GP2_DES_CHAIN_LEN)
//#define TCSM1_DMB_MV_BASE (TCSM1_MCC_CSN+4)
#define TCSM1_DMB_MV_BASE (DDMA_GP2_DES_CHAIN1+DDMA_GP2_DES_CHAIN_LEN)
#define TCSM1_DMB_MV_LEN ( sizeof(struct H264_MB_InterB_DecARGs) + 24 )
#endif

#define TCSM1_ACCUM_NUM                   (1)

#ifdef JZC_DBLKLI_OPT
#define TCSM1_DBLK_UPOUT_STRD_Y           ((TCSM1_ACCUM_NUM<<4))//ping-pong buffer
#define TCSM1_DBLK_UPOUT_STRD_C           ((TCSM1_ACCUM_NUM<<3)<<1)
#define TCSM1_DBLK_UPOUT_UV_OFFSET        (8)
//#define TCSM1_DBLK_UPOUT_Y0               (TCSM1_DMB_MV_BASE + TCSM1_DMB_MV_LEN)
#define TCSM1_DBLK_UPOUT_Y0               (DDMA_GP0_DES_CHAIN+DDMA_GP0_DES_CHAIN_LEN)
#define TCSM1_DBLK_UPOUT_Y1               (TCSM1_DBLK_UPOUT_Y0+64)
#define TCSM1_DBLK_UPOUT_U0               (TCSM1_DBLK_UPOUT_Y1+64)
#define TCSM1_DBLK_UPOUT_V0               (TCSM1_DBLK_UPOUT_U0+8)
#define TCSM1_DBLK_UPOUT_U1               (TCSM1_DBLK_UPOUT_U0+64)
#define TCSM1_DBLK_UPOUT_V1               (TCSM1_DBLK_UPOUT_U1+8)
#else
#define TCSM1_DBLK_UPOUT_STRD_Y           ((TCSM1_ACCUM_NUM<<4)<<1)//ping-pong buffer
#define TCSM1_DBLK_UPOUT_STRD_C           ((TCSM1_ACCUM_NUM<<3)<<1)
#define TCSM1_DBLK_UPOUT_UV_OFFSET        (TCSM1_DBLK_UPOUT_STRD_C<<2)
#define TCSM1_DBLK_UPOUT_Y0               (TCSM1_DMB_MV_BASE + TCSM1_DMB_MV_LEN + 0x300)
#define TCSM1_DBLK_UPOUT_Y1               (TCSM1_DBLK_UPOUT_Y0+(TCSM1_DBLK_UPOUT_STRD_Y>>1))
#define TCSM1_DBLK_UPOUT_U0               (TCSM1_DBLK_UPOUT_Y0+(TCSM1_DBLK_UPOUT_STRD_Y<<2))
#define TCSM1_DBLK_UPOUT_U1               (TCSM1_DBLK_UPOUT_U0+(TCSM1_DBLK_UPOUT_STRD_C>>1))
#define TCSM1_DBLK_UPOUT_V0               (TCSM1_DBLK_UPOUT_U0+(TCSM1_DBLK_UPOUT_STRD_C<<2))
#define TCSM1_DBLK_UPOUT_V1               (TCSM1_DBLK_UPOUT_V0+(TCSM1_DBLK_UPOUT_STRD_C>>1))
#endif

#define TCSM1_DBLK_MBOUT_STRD_Y           (4+(TCSM1_ACCUM_NUM<<4))
#define TCSM1_DBLK_MBOUT_STRD_C           (4+(TCSM1_ACCUM_NUM<<3))
#define TCSM1_DBLK_MBOUT_UV_OFFSET        (TCSM1_DBLK_MBOUT_STRD_C<<3)

#ifdef JZC_DBLKLI_OPT
#define TCSM1_DBLK_MBOUT_Y0               (TCSM1_DBLK_UPOUT_U1+64)
#else
#define TCSM1_DBLK_MBOUT_Y0               (TCSM1_DBLK_UPOUT_V0+(TCSM1_DBLK_UPOUT_STRD_C<<2))
#endif
#define TCSM1_DBLK_MBOUT_U0               (TCSM1_DBLK_MBOUT_Y0+(TCSM1_DBLK_MBOUT_STRD_Y<<4))
#define TCSM1_DBLK_MBOUT_V0               (TCSM1_DBLK_MBOUT_U0+(TCSM1_DBLK_MBOUT_STRD_C<<3))
#define TCSM1_DBLK_MBOUT_Y1               (TCSM1_DBLK_MBOUT_V0+(TCSM1_DBLK_MBOUT_STRD_C<<3))
#define TCSM1_DBLK_MBOUT_U1               (TCSM1_DBLK_MBOUT_Y1+(TCSM1_DBLK_MBOUT_STRD_Y<<4))
#define TCSM1_DBLK_MBOUT_V1               (TCSM1_DBLK_MBOUT_U1+(TCSM1_DBLK_MBOUT_STRD_C<<3))

#define TCSM1_ADD_ERROR_BUF0 (TCSM1_DBLK_MBOUT_V1 + (TCSM1_DBLK_MBOUT_STRD_C<<3))
#define ADD_ERROR_BUF_LEN (384 << 1)
//#define TCSM1_ADD_ERROR_BUF1 (TCSM1_ADD_ERROR_BUF0+ADD_ERROR_BUF_LEN)

//#define TCSM1_MC_DHA1 (TCSM1_ADD_ERROR_BUF1+ADD_ERROR_BUF_LEN)
#define TCSM1_MC_DHA1 (TCSM1_ADD_ERROR_BUF0+ADD_ERROR_BUF_LEN)

#define TCSM1_DBLK_MBOUT_DES_STRD_Y       (256)
#define TCSM1_DBLK_MBOUT_DES_STRD_C       (128)
#define TCSM1_DBLK_MBOUT_DES_OFFS_C       (8)
#define TCSM1_DBLK_UPOUT_DES_STRD_Y       (64)
#define TCSM1_DBLK_UPOUT_DES_STRD_C       (32)
#define TCSM1_DBLK_MBOUT_YDES             (TCSM1_MC_DHA1 + 0x110)
#define TCSM1_DBLK_MBOUT_UDES             (TCSM1_DBLK_MBOUT_YDES + TCSM1_DBLK_MBOUT_DES_STRD_Y)
#define TCSM1_DBLK_MBOUT_VDES             (TCSM1_DBLK_MBOUT_UDES + TCSM1_DBLK_MBOUT_DES_OFFS_C)

#define TCSM1_DBLK_MBOUT_YDES1             (TCSM1_DBLK_MBOUT_UDES + TCSM1_DBLK_MBOUT_DES_STRD_C)
#define TCSM1_DBLK_MBOUT_UDES1             (TCSM1_DBLK_MBOUT_YDES1 + TCSM1_DBLK_MBOUT_DES_STRD_Y)
#define TCSM1_DBLK_MBOUT_VDES1             (TCSM1_DBLK_MBOUT_UDES1 + TCSM1_DBLK_MBOUT_DES_OFFS_C)

#define TCSM1_DBLK_MBOUT_YDES2             (TCSM1_DBLK_MBOUT_UDES1 + TCSM1_DBLK_MBOUT_DES_STRD_C)
#define TCSM1_DBLK_MBOUT_UDES2             (TCSM1_DBLK_MBOUT_YDES2 + TCSM1_DBLK_MBOUT_DES_STRD_Y)
#define TCSM1_DBLK_MBOUT_VDES2             (TCSM1_DBLK_MBOUT_UDES2 + TCSM1_DBLK_MBOUT_DES_OFFS_C)

#define TCSM1_DDMA_GP1_DES_CHAIN1          (TCSM1_DBLK_MBOUT_UDES2 + TCSM1_DBLK_MBOUT_DES_STRD_C)
#define TCSM1_DDMA_GP1_DES_CHAIN2          (TCSM1_DDMA_GP1_DES_CHAIN1 + ((4*13)<<2))
#define TCSM1_DDMA_GP1_DES_CHAIN3          (TCSM1_DDMA_GP1_DES_CHAIN2 + ((4*13)<<2))

#define TCSM1_ROTA_MB_YBUF1                (TCSM1_DDMA_GP1_DES_CHAIN3 + ((4*3)<<2))
#define TCSM1_ROTA_MB_YBUF2                (TCSM1_ROTA_MB_YBUF1 + 256)
#define TCSM1_ROTA_MB_YBUF3                (TCSM1_ROTA_MB_YBUF2 + 256)

#define TCSM1_ROTA_MB_CBUF1                (TCSM1_ROTA_MB_YBUF3 + 256)
#define TCSM1_ROTA_MB_CBUF2                (TCSM1_ROTA_MB_CBUF1 + 128)
#define TCSM1_ROTA_MB_CBUF3                (TCSM1_ROTA_MB_CBUF2 + 128)

#define TCSM1_ROTA_UPMB_YBUF1              (TCSM1_ROTA_MB_CBUF3 + 128)
#define TCSM1_ROTA_UPMB_YBUF2              (TCSM1_ROTA_UPMB_YBUF1 + 64)

#define TCSM1_ROTA_UPMB_UBUF1              (TCSM1_ROTA_UPMB_YBUF2 + 64)
#define TCSM1_ROTA_UPMB_UBUF2              (TCSM1_ROTA_UPMB_UBUF1 + 32)

#define TCSM1_ROTA_UPMB_VBUF1              (TCSM1_ROTA_UPMB_UBUF2 + 32)
#define TCSM1_ROTA_UPMB_VBUF2              (TCSM1_ROTA_UPMB_VBUF1 + 32)

/*--------------------------------------------------
 * TCSM1_GP_END_FLAG; to indicate gp transmission finished; use to replace polling gp end
 --------------------------------------------------*/
#define TCSM1_GP0_END_FLAG_LEN             ( 1<< 2 )
#define TCSM1_GP0_END_FLAG                 (TCSM1_ROTA_UPMB_VBUF2 + 32)
#define TCSM1_GP1_END_FLAG_LEN             ( 1<< 2 )
#define TCSM1_GP1_END_FLAG                 ( TCSM1_GP0_END_FLAG + TCSM1_GP0_END_FLAG_LEN)
 
#define TCSM1_EDGE_YBUF1                 ( TCSM1_GP1_END_FLAG + TCSM1_GP1_END_FLAG_LEN)
#define TCSM1_EDGE_BAK_YBUF1             (TCSM1_EDGE_YBUF1 + 256)
#define TCSM1_EDGE_CBUF1                 (TCSM1_EDGE_BAK_YBUF1 + 24)
#define TCSM1_EDGE_BAK_CBUF1             (TCSM1_EDGE_CBUF1 + 128)

#define CV_FOR_SET_FIFO_ZERO             (TCSM1_EDGE_BAK_CBUF1 + 16)
#define CV_FOR_SET_FIFO_ZERO_LEN         (CV_MAX_TASK_LEN)
//#define CV_FOR_SET_FIFO_ZERO_LEN         (MAX_TASK_LEN) 

#define TCSM1_MC_BUG_SPACE               (CV_FOR_SET_FIFO_ZERO + CV_FOR_SET_FIFO_ZERO_LEN)
 
//#define P1_PMON_BUF             (TCSM1_MC_BUG_SPACE + 160 )
#define P1_PMON_BUF             (TCSM1_MC_BUG_SPACE)
#endif
