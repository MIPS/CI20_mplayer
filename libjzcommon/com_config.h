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

#ifndef _COM_CONFIG_H_
#define _COM_CONFIG_H_

#undef  EDGE_WIDTH
#define EDGE_WIDTH 32

#define JZ4765_OPT
#define JZ4760_OPT
#define JZ4750_OPT
#define JZ47_OPT
#define JZC_SYS

#define USE_JZ_IPU
/* You should select a SOC type for IPU or Software render */
#ifdef USE_JZ_IPU
//#define USE_SW_COLOR_CONVERT_RENDER
//#define JZ4740_IPU
//#define JZ4765_IPU
//#define JZ4760_IPU
//#define JZ4770_IPU
#define JZ4780_IPU
//#define JZ4780_IPU_LCDC_OVERLAY
//#define JZ4780_IPU_USE_HDMI_OVERLAY
#define USE_JZ4780_DMMU		/* support virtual address */
//#define JZ4780_X2D
#endif

#ifdef JZ4780_IPU_LCDC_OVERLAY
#define USE_IPU_THROUGH_MODE
#endif

//#define JZC_CRC_VER
#define JZC_HW_MEDIA
#define JZ_LINUX_OS
#define PROFILE_3DOT0_ABOVE_OPT
#define JZC_H264_CAVLC_OPT
#ifdef USE_IPU_THROUGH_MODE
#define USE_FBUF_NUM 6
#endif

#ifdef JZ4780_IPU
#define  RECOVE_FB_ORG
#endif

//#define JZC_PMON_P0
//#define STA_CCLK
//#define STA_DCC
//#define STA_ICC
//#define STA_UINSN
//#define STA_INSN
//#define STA_UINSN
//#define STA_TLB

#ifdef USE_JZ_IPU
/* if use h264 cabac hw on linux, this macro must be open. */
#define JZC_BS_HW_OPT
#endif

#define JZC_ROTA90_OPT
#define JZC_LINE_OPT
#define JZC_TLB_OPT

#endif
