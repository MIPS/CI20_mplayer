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

#include "mem_def_falcon.h"

#ifdef P1_USE_PADDR
#define SDE_VBASE 0x13290000
#else
#define SDE_VBASE sde_base
#endif
#define SDE_PBASE 0x13290000

#define SDE_CTRL_OFST		0x0
#define SDE_DESPC_OFST		0x4
#define SDE_SYNC_END_OFST	0x8
#define SDE_SLICE_INFO_0_OFST	0x10
#define SDE_SLICE_INFO_1_OFST	0x14
#define SDE_BS_ADDR_OFST	0x18
#define SDE_BS_OFST_OFST	0x1c
#define SDE_DIRECT_CIN_OFST	0x20
#define SDE_DIRECT_MIN_OFST	0x24
#define SDE_DIRECT_COUT_OFST	0x28
#define SDE_DIRECT_MOUT_OFST	0x2c
#define SDE_GLOBAL_CTRL_OFST	0x30
#define SDE_DBG_OFST		0x100
#define SDE_CTX_TBL_OFST	0x2000

#define SDE_MB_END_SHIFT	0
#define SDE_CTRL_DOUT_END_SHIFT	1
#define SDE_RES_DOUT_END_SHIFT	2
#define SDE_MB_ERROR_SHIFT	3

#define SDE_SLIEC_INIT_SHIFT	31
#define SDE_SLIEC_END_SHIFT	24


#define SDE_RESIDUAL_DOUT_ADDR  (SDE_SRAM_BASE)

#define SET_SDE_CTRL(val)	({write_reg((SDE_VBASE + SDE_CTRL_OFST), (val));})
#define SET_SDE_DESPC(val)	({write_reg((SDE_VBASE + SDE_DESPC_OFST), (val));})
#define SET_SDE_SYNC_END(val)	({write_reg((SDE_VBASE + SDE_SYNC_END_OFST), (val));})
#define POLL_MB_END()		({while( !(read_reg(SDE_VBASE, SDE_CTRL_OFST) & (1<<SDE_MB_END_SHIFT)) );})
#define POLL_CTRL_DOUT_END()	({while( !(read_reg(SDE_VBASE, SDE_CTRL_OFST) & (1<<SDE_CTRL_DOUT_END_SHIFT)) );})
#define POLL_RES_DOUT_END()	({while( !(read_reg(SDE_VBASE, SDE_CTRL_OFST) & (1<<SDE_RES_DOUT_END_SHIFT)) );})
#define GET_SDE_CTRL()		({read_reg((SDE_VBASE + SDE_CTRL_OFST), (0));})

#define SET_SLICE_INFO_0(val)	({write_reg((SDE_VBASE + SDE_SLICE_INFO_0_OFST), (val));})
#define SET_SLICE_INFO_1(val)	({write_reg((SDE_VBASE + SDE_SLICE_INFO_1_OFST), (val));})
#define POLL_SLICE_INIT()	({while(read_reg(SDE_VBASE, SDE_SLICE_INFO_0_OFST) & (1<<SDE_SLIEC_INIT_SHIFT));})
#define POLL_SLICE_END()	({while(read_reg(SDE_VBASE, SDE_SLICE_INFO_0_OFST) & (1<<SDE_SLIEC_END_SHIFT));})

#define SET_BS_ADDR(val) ({write_reg((SDE_VBASE + SDE_BS_ADDR_OFST), (val));})
#define SET_BS_OFST(val) ({write_reg((SDE_VBASE + SDE_BS_OFST_OFST), (val));})

#define SET_SDE_DIRECT_CIN(val)  ({write_reg((SDE_VBASE + SDE_DIRECT_CIN_OFST), (val));})
#define SET_SDE_DIRECT_MIN(val)  ({write_reg((SDE_VBASE + SDE_DIRECT_MIN_OFST), (val));})
#define SET_SDE_DIRECT_COUT(val) ({write_reg((SDE_VBASE + SDE_DIRECT_COUT_OFST), (val));})
#define SET_SDE_DIRECT_MOUT(val) ({write_reg((SDE_VBASE + SDE_DIRECT_MOUT_OFST), (val));})

#define SET_GLOBAL_CTRL(val)	({write_reg((SDE_VBASE + SDE_GLOBAL_CTRL_OFST), (val));})


// hw_mb_type use 16 bits,
#define SDE_MB_TYPE_INTRA4x4   0
#define SDE_MB_TYPE_INTRA16x16 1
#define SDE_MB_TYPE_INTRA_PCM  2
#define SDE_MB_TYPE_16x16      3
#define SDE_MB_TYPE_16x8       4
#define SDE_MB_TYPE_8x16       5
#define SDE_MB_TYPE_8x8        6
#define SDE_MB_TYPE_INTERLACED 7
#define SDE_MB_TYPE_DIRECT     8
#define SDE_MB_TYPE_ACPRED     9
#define SDE_MB_TYPE_GMC        10
#define SDE_MB_TYPE_SKIP       11
#define SDE_MB_TYPE_P0L0       12
#define SDE_MB_TYPE_P1L0       13
#define SDE_MB_TYPE_P0L1       14
#define SDE_MB_TYPE_P1L1       15
#define SDE_MB_TYPE_REF0       MB_TYPE_ACPRED
#define SDE_MB_TYPE_8x8DCT     MB_TYPE_GMC
#define SDE_MB_TYPE_IS_INTRA   MB_TYPE_INTRA_PCM

/*
  mv_len : mv number in word.
  mv_mode: use 10 bit.
           bit 0~1 : mb_type
           bit 2~3 : sub_mb_type[0]
           bit 4~5 : sub_mb_type[1]
           bit 6~7 : sub_mb_type[2]
           bit 8~9 : sub_mb_type[3]
           val 0: partation 16x16/8x8,
           val 1: partation 16x8 /8x4,
           val 2: partation 8x16 /4x8,
           val 3: partation 8x8  /4x4,
*/


#if 0
typedef struct SLICE_INFO
{
  int slice_type;
  int picture_structure;
  int qscale;
  int transform_8x8_mode;
  int constrained_intra_pred;
  int direct_8x8_inference_flag;
  int direct_spatial_mv_pred;
  int x264_build;
  unsigned int ref0_count;
  unsigned int ref1_count;
  unsigned int list_count;

  int start_mx;
  int start_my;
  int mb_width;
  int mb_height;

  int bs_ofst;
}slice_info, *slice_info_p;

typedef struct FRM_INFO_MV
{
  short mv[32][2];
};
#endif

typedef struct FRM_INFO_CTRL
{
  unsigned int frm_ctrl_0;
  unsigned int frm_ctrl_1;
};

typedef struct NEIGHBOR_INFO
{
  unsigned int mb_type;
  unsigned int nnz_cbp;
  unsigned int pred_mode_or_ref;
  short mv[2][4][2];
  short mvd[2][4][2];
}NEIGHBOR_INFO, *NEIGHBOR_INFO_p;

typedef struct MB_INTRA_BAC
{
  unsigned short mb_type; // 16 bit for hw_mb_type
  char intra_avail;       // top,left,topleft,topright available for vmau
  char decoder_error;     // error while hw decoding
  short cbp;
  char qscal_diff;
  unsigned char direct;
  unsigned int nnz_bottom;
  unsigned int nnz_right;
  unsigned int bit_count;
  unsigned int nnz_dct;   // vmau main cbp.
  unsigned short coef_cnt;
  unsigned short nnz_br;

  unsigned int intra4x4_pred_mode0;
  unsigned int intra4x4_pred_mode1;
  unsigned char chroma_pred_mode;
  unsigned char intra16x16_pred_mode;
  unsigned char luma_neighbor;
  unsigned char chroma_neighbor;
}mb_intra_bac, *mb_intra_bac_p;

typedef struct MB_INTER_P_BAC
{
  unsigned short mb_type; // 16 bit for hw_mb_type
  char intra_avail;       // top,left,topleft,topright available for vmau
  char decoder_error;     // error while hw decoding
  short cbp;
  char qscal_diff;
  unsigned char direct;
  unsigned int nnz_bottom;
  unsigned int nnz_right;
  unsigned int bit_count;
  unsigned int nnz_dct;
  unsigned short coef_cnt;
  unsigned short nnz_br;

  char ref[2][4];
  short sub_mb_type[4];
  unsigned short mv_len;
  unsigned short mv_mode;
  short mv_bottom_l0[4][2];
  short mv_right_l0[4][2];
  short mvd_bottom_l0[4][2];
  short mvd_right_l0[4][2];
  short mv[32][2];
}mb_inter_p_bac, *mb_inter_p_bac_p;

typedef struct MB_INTER_B_BAC
{
  unsigned short mb_type; // 16 bit for hw_mb_type
  char intra_avail;       // top,left,topleft,topright available for vmau
  char decoder_error;     // error while hw decoding
  short cbp;
  char qscal_diff;
  unsigned char direct;
  unsigned int nnz_bottom;
  unsigned int nnz_right;
  unsigned int bit_count;
  unsigned int nnz_dct;
  unsigned short coef_cnt;
  unsigned short nnz_br;

  char ref[2][4];
  short sub_mb_type[4];
  unsigned short mv_len;
  unsigned short mv_mode;
  short mv_bottom_l0[4][2];
  short mv_right_l0[4][2];
  short mvd_bottom_l0[4][2];
  short mvd_right_l0[4][2];
  short mv_bottom_l1[4][2];
  short mv_right_l1[4][2];
  short mvd_bottom_l1[4][2];
  short mvd_right_l1[4][2];
  short mv[32][2];
}mb_inter_b_bac, *mb_inter_b_bac_p;

typedef struct MB_INTRA_VLC
{
  unsigned short mb_type; // 16 bit for hw_mb_type
  char intra_avail;       // top,left,topleft,topright available for vmau
  char decoder_error;     // error while hw decoding
  char cbp;
  char qscal_diff;
  unsigned short skip_run;
  unsigned int nnz_bottom;
  unsigned int nnz_right;
  unsigned int bit_count;
  unsigned int nnz_dct;
  unsigned short coef_cnt;
  unsigned short nnz_br;

  unsigned int intra4x4_pred_mode0;
  unsigned int intra4x4_pred_mode1;
  unsigned char chroma_pred_mode;
  unsigned char intra16x16_pred_mode;
  unsigned char luma_neighbor;
  unsigned char chroma_neighbor;
}mb_intra_vlc, *mb_intra_vlc_p;

typedef struct MB_INTER_P_VLC
{
  unsigned short mb_type; // 16 bit for hw_mb_type
  char intra_avail;       // top,left,topleft,topright available for vmau
  char decoder_error;     // error while hw decoding
  char cbp;
  char qscal_diff;
  unsigned short skip_run;
  unsigned int nnz_bottom;
  unsigned int nnz_right;
  unsigned int bit_count;
  unsigned int nnz_dct;
  unsigned short coef_cnt;
  unsigned short nnz_br;

  char ref[2][4];
  short sub_mb_type[4];
  unsigned short mv_len;
  unsigned short mv_mode;
  short mv_bottom_l0[4][2];
  short mv_right_l0[4][2];
  short mv[32][2];
}mb_inter_p_vlc, *mb_inter_p_vlc_p;

typedef struct MB_INTER_B_VLC
{
  unsigned short mb_type; // 16 bit for hw_mb_type
  char intra_avail;       // top,left,topleft,topright available for vmau
  char decoder_error;     // error while hw decoding
  char cbp;
  char qscal_diff;
  unsigned short skip_run;
  unsigned int nnz_bottom;
  unsigned int nnz_right;
  unsigned int bit_count;
  unsigned int nnz_dct;
  unsigned short coef_cnt;
  unsigned short nnz_br;

  char ref[2][4];
  short sub_mb_type[4];
  unsigned short mv_len;
  unsigned short mv_mode;
  short mv_bottom_l0[4][2];
  short mv_right_l0[4][2];
  short mv_bottom_l1[4][2];
  short mv_right_l1[4][2];
  short mv[32][2];
}mb_inter_b_vlc, *mb_inter_b_vlc_p;


#define MB_W_720P 80
#define MB_H_720P 45
#define MB_NUM_720P (MB_W_720P*MB_H_720P)
#define MB_W_1080P 120
#define MB_H_1080P 68
#define MB_NUM_1080P (MB_W_1080P*MB_H_1080P)


#if 0
typedef struct SDE_VLC_STA
{
  int ram_ofst;
  int size;
  int lvl0_len;
};


struct SDE_VLC_STA sde_vlc2_sta[7] = {
  {0  , 128, 6}, // vlc_tables_coeff_token_table_0
  {128, 116, 5}, // vlc_tables_coeff_token_table_1
  {256, 104, 6}, // vlc_tables_coeff_token_table_2
  {384,  64, 6}, // vlc_tables_coeff_token_table_3
  {512,  70, 6}, // vlc_tables_chroma_dc_coeff_token_table
  {640,  74, 6}, // vlc_tables_total_zeros_table_0
  {768,  96, 6}, // vlc_tables_run7_table
};

unsigned short sde_vlc2_table[7][128]={
  { // 0
    0xa020, 0x887c, 0x806c, 0x200f, 0x100a, 0x100a, 0x100a, 0x100a, 
    0x0805, 0x0805, 0x0805, 0x0805, 0x0805, 0x0805, 0x0805, 0x0805, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, // 32
    0xa040, 0x9070, 0x8078, 0x807a, 0x2023, 0x201a, 0x2015, 0x2010, 
    0x181f, 0x181f, 0x1816, 0x1816, 0x1811, 0x1811, 0x180c, 0x180c, 
    0x101b, 0x101b, 0x101b, 0x101b, 0x1012, 0x1012, 0x1012, 0x1012, 
    0x100d, 0x100d, 0x100d, 0x100d, 0x1008, 0x1008, 0x1008, 0x1008, // 64
    0x4001, 0x2035, 0x8060, 0x8062, 0x8064, 0x8066, 0x8068, 0x806a, 
    0x203b, 0x2036, 0x2031, 0x2030, 0x2037, 0x2032, 0x202d, 0x202c, 
    0x1833, 0x1833, 0x182e, 0x182e, 0x1829, 0x1829, 0x1828, 0x1828, 
    0x182f, 0x182f, 0x182a, 0x182a, 0x1825, 0x1825, 0x1824, 0x1824, // 96
    0x0040, 0x0042, 0x0041, 0x003c, 0x0043, 0x003e, 0x003d, 0x0038, // 104
    0x003f, 0x003a, 0x0039, 0x0034, 0x0009, 0x0004, 0x4000, 0x4000, // 112
    0x1020, 0x1026, 0x1021, 0x101c, 0x102b, 0x1022, 0x101d, 0x1018, // 120
    0x0027, 0x001e, 0x0019, 0x0014, 0x0817, 0x080e, 0x0013, 0x0013, // 128
  },
  { // 1
    0xa020, 0x8868, 0x806c, 0x806e, 0x8070, 0x8072, 0x2017, 0x2009, 
    0x1813, 0x1813, 0x180f, 0x180f, 0x100a, 0x100a, 0x100a, 0x100a, 
    0x0805, 0x0805, 0x0805, 0x0805, 0x0805, 0x0805, 0x0805, 0x0805, 
    0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 
    0x9840, 0x9050, 0x8858, 0x885c, 0x8060, 0x8062, 0x8064, 0x8066, 
    0x1827, 0x1827, 0x181e, 0x181e, 0x181d, 0x181d, 0x1818, 0x1818, 
    0x1014, 0x1014, 0x1014, 0x1014, 0x101a, 0x101a, 0x101a, 0x101a, 
    0x1019, 0x1019, 0x1019, 0x1019, 0x1010, 0x1010, 0x1010, 0x1010, 
    0x4001, 0x4001, 0x103f, 0x103f, 0x1843, 0x1842, 0x1841, 0x1840, 
    0x183d, 0x183c, 0x183e, 0x1839, 0x103a, 0x103a, 0x1038, 0x1038, 
    0x103b, 0x1036, 0x1035, 0x1034, 0x1037, 0x1032, 0x1031, 0x1030, 
    0x082c, 0x082e, 0x082d, 0x0828, 0x0833, 0x082a, 0x0829, 0x0824, 
    0x002f, 0x0026, 0x0025, 0x0020, 0x002b, 0x0022, 0x0021, 0x001c, 
    0x0823, 0x0816, 0x0815, 0x080c, 0x001f, 0x0012, 0x0011, 0x0008, 
    0x001b, 0x000e, 0x000d, 0x0004, 
  },
  { // 2
    0x9840, 0x9050, 0x8858, 0x885c, 0x8060, 0x8062, 0x8064, 0x8066, 
    0x280c, 0x281e, 0x281d, 0x2808, 0x2827, 0x281a, 0x2819, 0x2804, 
    0x2015, 0x2015, 0x2016, 0x2016, 0x2011, 0x2011, 0x2012, 0x2012, 
    0x200d, 0x200d, 0x2023, 0x2023, 0x200e, 0x200e, 0x2009, 0x2009, 
    0x181f, 0x181f, 0x181f, 0x181f, 0x181b, 0x181b, 0x181b, 0x181b, 
    0x1817, 0x1817, 0x1817, 0x1817, 0x1813, 0x1813, 0x1813, 0x1813, 
    0x180f, 0x180f, 0x180f, 0x180f, 0x180a, 0x180a, 0x180a, 0x180a, 
    0x1805, 0x1805, 0x1805, 0x1805, 0x1800, 0x1800, 0x1800, 0x1800, 
    0x4001, 0x1840, 0x1843, 0x1842, 0x1841, 0x183c, 0x183f, 0x183e, 
    0x183d, 0x1838, 0x183b, 0x183a, 0x1839, 0x1834, 0x1035, 0x1035, 
    0x1030, 0x1036, 0x1031, 0x102c, 0x1037, 0x1032, 0x102d, 0x1028, 
    0x0833, 0x082e, 0x0829, 0x0824, 0x082f, 0x082a, 0x0825, 0x0820, 
    0x001c, 0x0018, 0x0026, 0x0014, 0x002b, 0x0022, 0x0021, 0x0010, 
  },
  { // 3
    0x2804, 0x2805, 0x4001, 0x2800, 0x2808, 0x2809, 0x280a, 0x4001, 
    0x280c, 0x280d, 0x280e, 0x280f, 0x2810, 0x2811, 0x2812, 0x2813, 
    0x2814, 0x2815, 0x2816, 0x2817, 0x2818, 0x2819, 0x281a, 0x281b, 
    0x281c, 0x281d, 0x281e, 0x281f, 0x2820, 0x2821, 0x2822, 0x2823, 
    0x2824, 0x2825, 0x2826, 0x2827, 0x2828, 0x2829, 0x282a, 0x282b, 
    0x282c, 0x282d, 0x282e, 0x282f, 0x2830, 0x2831, 0x2832, 0x2833, 
    0x2834, 0x2835, 0x2836, 0x2837, 0x2838, 0x2839, 0x283a, 0x283b, 
    0x283c, 0x283d, 0x283e, 0x283f, 0x2840, 0x2841, 0x2842, 0x2843, 
  },
  { // 4
    0x8840, 0x8044, 0x2810, 0x280c, 0x2808, 0x280f, 0x2809, 0x2804, 
    0x100a, 0x100a, 0x100a, 0x100a, 0x100a, 0x100a, 0x100a, 0x100a, 
    0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 
    0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 
    0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 
    0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 
    0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 
    0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 
    0x0013, 0x0013, 0x0812, 0x0811, 0x000e, 0x000d, 
  },
  {
    0x9040, 0x8048, 0x2808, 0x2807, 0x2006, 0x2006, 0x2005, 0x2005, 
    0x1804, 0x1804, 0x1804, 0x1804, 0x1803, 0x1803, 0x1803, 0x1803, 
    0x1002, 0x1002, 0x1002, 0x1002, 0x1002, 0x1002, 0x1002, 0x1002, 
    0x1001, 0x1001, 0x1001, 0x1001, 0x1001, 0x1001, 0x1001, 0x1001, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
    0x4001, 0x100f, 0x100e, 0x100d, 0x080c, 0x080c, 0x080b, 0x080b, 
    0x000a, 0x0009, 
  },
  {
    0xa040, 0x2809, 0x2008, 0x2008, 0x1807, 0x1807, 0x1807, 0x1807, 
    0x1006, 0x1006, 0x1006, 0x1006, 0x1006, 0x1006, 0x1006, 0x1006, 
    0x1005, 0x1005, 0x1005, 0x1005, 0x1005, 0x1005, 0x1005, 0x1005, 
    0x1004, 0x1004, 0x1004, 0x1004, 0x1004, 0x1004, 0x1004, 0x1004, 
    0x1003, 0x1003, 0x1003, 0x1003, 0x1003, 0x1003, 0x1003, 0x1003, 
    0x1002, 0x1002, 0x1002, 0x1002, 0x1002, 0x1002, 0x1002, 0x1002, 
    0x1001, 0x1001, 0x1001, 0x1001, 0x1001, 0x1001, 0x1001, 0x1001, 
    0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 
    0x4001, 0x200e, 0x180d, 0x180d, 0x100c, 0x100c, 0x100c, 0x100c, 
    0x080b, 0x080b, 0x080b, 0x080b, 0x080b, 0x080b, 0x080b, 0x080b, 
    0x000a, 0x000a, 0x000a, 0x000a, 0x000a, 0x000a, 0x000a, 0x000a, 
    0x000a, 0x000a, 0x000a, 0x000a, 0x000a, 0x000a, 0x000a, 0x000a, 
  },
};
#endif
