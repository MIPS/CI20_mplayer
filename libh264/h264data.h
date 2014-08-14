/*
 * H26L/H264/AVC/JVT/14496-10/... encoder/decoder
 * Copyright (c) 2003 Michael Niedermayer <michaelni@gmx.at>
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
 * @brief
 *     H264 / AVC / MPEG4 part10 codec data table
 * @author Michael Niedermayer <michaelni@gmx.at>
 */

#ifndef AVCODEC_H264DATA_H
#define AVCODEC_H264DATA_H

#include <stdint.h>
#include "libavutil/rational.h"
#include "mpegvideo.h"
#include "h264.h"

static const uint8_t chroma_qp[52]={
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,
   12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,
   28,29,29,30,31,32,32,33,34,34,35,35,36,36,37,37,
   37,38,38,38,39,39,39,39

};

static const uint8_t golomb_to_pict_type[5]=
{FF_P_TYPE, FF_B_TYPE, FF_I_TYPE, FF_SP_TYPE, FF_SI_TYPE};

static const uint8_t golomb_to_intra4x4_cbp[48]={
 47, 31, 15,  0, 23, 27, 29, 30,  7, 11, 13, 14, 39, 43, 45, 46,
 16,  3,  5, 10, 12, 19, 21, 26, 28, 35, 37, 42, 44,  1,  2,  4,
  8, 17, 18, 20, 24,  6,  9, 22, 25, 32, 33, 34, 36, 40, 38, 41
};

static const uint8_t golomb_to_inter_cbp[48]={
  0, 16,  1,  2,  4,  8, 32,  3,  5, 10, 12, 15, 47,  7, 11, 13,
 14,  6,  9, 31, 35, 37, 42, 44, 33, 34, 36, 40, 39, 43, 45, 46,
 17, 18, 20, 24, 19, 21, 26, 28, 23, 27, 29, 30, 22, 25, 38, 41
};

static const uint8_t zigzag_scan[16]={
 0+0*4, 1+0*4, 0+1*4, 0+2*4,
 1+1*4, 2+0*4, 3+0*4, 2+1*4,
 1+2*4, 0+3*4, 1+3*4, 2+2*4,
 3+1*4, 3+2*4, 2+3*4, 3+3*4,
};

static const uint8_t field_scan[16]={
 0+0*4, 0+1*4, 1+0*4, 0+2*4,
 0+3*4, 1+1*4, 1+2*4, 1+3*4,
 2+0*4, 2+1*4, 2+2*4, 2+3*4,
 3+0*4, 3+1*4, 3+2*4, 3+3*4,
};

static const uint8_t luma_dc_zigzag_scan[16]={
 0*16 + 0*64, 1*16 + 0*64, 2*16 + 0*64, 0*16 + 2*64,
 3*16 + 0*64, 0*16 + 1*64, 1*16 + 1*64, 2*16 + 1*64,
 1*16 + 2*64, 2*16 + 2*64, 3*16 + 2*64, 0*16 + 3*64,
 3*16 + 1*64, 1*16 + 3*64, 2*16 + 3*64, 3*16 + 3*64,
};

static const uint8_t luma_dc_field_scan[16]={
 0*16 + 0*64, 2*16 + 0*64, 1*16 + 0*64, 0*16 + 2*64,
 2*16 + 2*64, 3*16 + 0*64, 1*16 + 2*64, 3*16 + 2*64,
 0*16 + 1*64, 2*16 + 1*64, 0*16 + 3*64, 2*16 + 3*64,
 1*16 + 1*64, 3*16 + 1*64, 1*16 + 3*64, 3*16 + 3*64,
};

static const uint8_t chroma_dc_scan[4]={
 (0+0*2)*16, (1+0*2)*16,
 (0+1*2)*16, (1+1*2)*16,  //FIXME
};

static const uint8_t zigzag_scan8x8[64]={
 0+0*8, 1+0*8, 0+1*8, 0+2*8,
 1+1*8, 2+0*8, 3+0*8, 2+1*8,
 1+2*8, 0+3*8, 0+4*8, 1+3*8,
 2+2*8, 3+1*8, 4+0*8, 5+0*8,
 4+1*8, 3+2*8, 2+3*8, 1+4*8,
 0+5*8, 0+6*8, 1+5*8, 2+4*8,
 3+3*8, 4+2*8, 5+1*8, 6+0*8,
 7+0*8, 6+1*8, 5+2*8, 4+3*8,
 3+4*8, 2+5*8, 1+6*8, 0+7*8,
 1+7*8, 2+6*8, 3+5*8, 4+4*8,
 5+3*8, 6+2*8, 7+1*8, 7+2*8,
 6+3*8, 5+4*8, 4+5*8, 3+6*8,
 2+7*8, 3+7*8, 4+6*8, 5+5*8,
 6+4*8, 7+3*8, 7+4*8, 6+5*8,
 5+6*8, 4+7*8, 5+7*8, 6+6*8,
 7+5*8, 7+6*8, 6+7*8, 7+7*8,
};


// zigzag_scan8x8_cavlc[i] = zigzag_scan8x8[(i/4) + 16*(i%4)]
static const uint8_t zigzag_scan8x8_cavlc[64]={
 0+0*8, 1+1*8, 1+2*8, 2+2*8,
 4+1*8, 0+5*8, 3+3*8, 7+0*8,
 3+4*8, 1+7*8, 5+3*8, 6+3*8,
 2+7*8, 6+4*8, 5+6*8, 7+5*8,
 1+0*8, 2+0*8, 0+3*8, 3+1*8,
 3+2*8, 0+6*8, 4+2*8, 6+1*8,
 2+5*8, 2+6*8, 6+2*8, 5+4*8,
 3+7*8, 7+3*8, 4+7*8, 7+6*8,
 0+1*8, 3+0*8, 0+4*8, 4+0*8,
 2+3*8, 1+5*8, 5+1*8, 5+2*8,
 1+6*8, 3+5*8, 7+1*8, 4+5*8,
 4+6*8, 7+4*8, 5+7*8, 6+7*8,
 0+2*8, 2+1*8, 1+3*8, 5+0*8,
 1+4*8, 2+4*8, 6+0*8, 4+3*8,
 0+7*8, 4+4*8, 7+2*8, 3+6*8,
 5+5*8, 6+5*8, 6+6*8, 7+7*8,
};

static const uint8_t field_scan8x8[64]={
 0+0*8, 0+1*8, 0+2*8, 1+0*8,
 1+1*8, 0+3*8, 0+4*8, 1+2*8,
 2+0*8, 1+3*8, 0+5*8, 0+6*8,
 0+7*8, 1+4*8, 2+1*8, 3+0*8,
 2+2*8, 1+5*8, 1+6*8, 1+7*8,
 2+3*8, 3+1*8, 4+0*8, 3+2*8,
 2+4*8, 2+5*8, 2+6*8, 2+7*8,
 3+3*8, 4+1*8, 5+0*8, 4+2*8,
 3+4*8, 3+5*8, 3+6*8, 3+7*8,
 4+3*8, 5+1*8, 6+0*8, 5+2*8,
 4+4*8, 4+5*8, 4+6*8, 4+7*8,
 5+3*8, 6+1*8, 6+2*8, 5+4*8,
 5+5*8, 5+6*8, 5+7*8, 6+3*8,
 7+0*8, 7+1*8, 6+4*8, 6+5*8,
 6+6*8, 6+7*8, 7+2*8, 7+3*8,
 7+4*8, 7+5*8, 7+6*8, 7+7*8,
};

static const uint8_t field_scan8x8_cavlc[64]={
 0+0*8, 1+1*8, 2+0*8, 0+7*8,
 2+2*8, 2+3*8, 2+4*8, 3+3*8,
 3+4*8, 4+3*8, 4+4*8, 5+3*8,
 5+5*8, 7+0*8, 6+6*8, 7+4*8,
 0+1*8, 0+3*8, 1+3*8, 1+4*8,
 1+5*8, 3+1*8, 2+5*8, 4+1*8,
 3+5*8, 5+1*8, 4+5*8, 6+1*8,
 5+6*8, 7+1*8, 6+7*8, 7+5*8,
 0+2*8, 0+4*8, 0+5*8, 2+1*8,
 1+6*8, 4+0*8, 2+6*8, 5+0*8,
 3+6*8, 6+0*8, 4+6*8, 6+2*8,
 5+7*8, 6+4*8, 7+2*8, 7+6*8,
 1+0*8, 1+2*8, 0+6*8, 3+0*8,
 1+7*8, 3+2*8, 2+7*8, 4+2*8,
 3+7*8, 5+2*8, 4+7*8, 5+4*8,
 6+3*8, 6+5*8, 7+3*8, 7+7*8,
};

typedef struct IMbInfo{
    uint16_t type;
    uint8_t pred_mode;
    uint8_t cbp;
} IMbInfo;

static const IMbInfo i_mb_type_info[26]={
{MB_TYPE_INTRA4x4  , -1, -1},
{MB_TYPE_INTRA16x16,  2,  0},
{MB_TYPE_INTRA16x16,  1,  0},
{MB_TYPE_INTRA16x16,  0,  0},
{MB_TYPE_INTRA16x16,  3,  0},
{MB_TYPE_INTRA16x16,  2,  16},
{MB_TYPE_INTRA16x16,  1,  16},
{MB_TYPE_INTRA16x16,  0,  16},
{MB_TYPE_INTRA16x16,  3,  16},
{MB_TYPE_INTRA16x16,  2,  32},
{MB_TYPE_INTRA16x16,  1,  32},
{MB_TYPE_INTRA16x16,  0,  32},
{MB_TYPE_INTRA16x16,  3,  32},
{MB_TYPE_INTRA16x16,  2,  15+0},
{MB_TYPE_INTRA16x16,  1,  15+0},
{MB_TYPE_INTRA16x16,  0,  15+0},
{MB_TYPE_INTRA16x16,  3,  15+0},
{MB_TYPE_INTRA16x16,  2,  15+16},
{MB_TYPE_INTRA16x16,  1,  15+16},
{MB_TYPE_INTRA16x16,  0,  15+16},
{MB_TYPE_INTRA16x16,  3,  15+16},
{MB_TYPE_INTRA16x16,  2,  15+32},
{MB_TYPE_INTRA16x16,  1,  15+32},
{MB_TYPE_INTRA16x16,  0,  15+32},
{MB_TYPE_INTRA16x16,  3,  15+32},
{MB_TYPE_INTRA_PCM , -1, -1},
};

typedef struct PMbInfo{
    uint16_t type;
    uint8_t partition_count;
} PMbInfo;

static const PMbInfo p_mb_type_info[5]={
{MB_TYPE_16x16|MB_TYPE_P0L0             , 1},
{MB_TYPE_16x8 |MB_TYPE_P0L0|MB_TYPE_P1L0, 2},
{MB_TYPE_8x16 |MB_TYPE_P0L0|MB_TYPE_P1L0, 2},
{MB_TYPE_8x8  |MB_TYPE_P0L0|MB_TYPE_P1L0, 4},
{MB_TYPE_8x8  |MB_TYPE_P0L0|MB_TYPE_P1L0|MB_TYPE_REF0, 4},
};

static const PMbInfo p_sub_mb_type_info[4]={
{MB_TYPE_16x16|MB_TYPE_P0L0             , 1},
{MB_TYPE_16x8 |MB_TYPE_P0L0             , 2},
{MB_TYPE_8x16 |MB_TYPE_P0L0             , 2},
{MB_TYPE_8x8  |MB_TYPE_P0L0             , 4},
};

#if 0
static const PMbInfo b_mb_type_info[23]={
{MB_TYPE_DIRECT2|MB_TYPE_L0L1                                      , 1, },
{MB_TYPE_16x16|MB_TYPE_P0L0                                       , 1, },
{MB_TYPE_16x16             |MB_TYPE_P0L1                          , 1, },
{MB_TYPE_16x16|MB_TYPE_P0L0|MB_TYPE_P0L1                          , 1, },
{MB_TYPE_16x8 |MB_TYPE_P0L0             |MB_TYPE_P1L0             , 2, },
{MB_TYPE_8x16 |MB_TYPE_P0L0             |MB_TYPE_P1L0             , 2, },
{MB_TYPE_16x8              |MB_TYPE_P0L1             |MB_TYPE_P1L1, 2, },
{MB_TYPE_8x16              |MB_TYPE_P0L1             |MB_TYPE_P1L1, 2, },
{MB_TYPE_16x8 |MB_TYPE_P0L0                          |MB_TYPE_P1L1, 2, },
{MB_TYPE_8x16 |MB_TYPE_P0L0                          |MB_TYPE_P1L1, 2, },
{MB_TYPE_16x8              |MB_TYPE_P0L1|MB_TYPE_P1L0             , 2, },
{MB_TYPE_8x16              |MB_TYPE_P0L1|MB_TYPE_P1L0             , 2, },
{MB_TYPE_16x8 |MB_TYPE_P0L0             |MB_TYPE_P1L0|MB_TYPE_P1L1, 2, },
{MB_TYPE_8x16 |MB_TYPE_P0L0             |MB_TYPE_P1L0|MB_TYPE_P1L1, 2, },
{MB_TYPE_16x8              |MB_TYPE_P0L1|MB_TYPE_P1L0|MB_TYPE_P1L1, 2, },
{MB_TYPE_8x16              |MB_TYPE_P0L1|MB_TYPE_P1L0|MB_TYPE_P1L1, 2, },
{MB_TYPE_16x8 |MB_TYPE_P0L0|MB_TYPE_P0L1|MB_TYPE_P1L0             , 2, },
{MB_TYPE_8x16 |MB_TYPE_P0L0|MB_TYPE_P0L1|MB_TYPE_P1L0             , 2, },
{MB_TYPE_16x8 |MB_TYPE_P0L0|MB_TYPE_P0L1             |MB_TYPE_P1L1, 2, },
{MB_TYPE_8x16 |MB_TYPE_P0L0|MB_TYPE_P0L1             |MB_TYPE_P1L1, 2, },
{MB_TYPE_16x8 |MB_TYPE_P0L0|MB_TYPE_P0L1|MB_TYPE_P1L0|MB_TYPE_P1L1, 2, },
{MB_TYPE_8x16 |MB_TYPE_P0L0|MB_TYPE_P0L1|MB_TYPE_P1L0|MB_TYPE_P1L1, 2, },
{MB_TYPE_8x8  |MB_TYPE_P0L0|MB_TYPE_P0L1|MB_TYPE_P1L0|MB_TYPE_P1L1, 4, },
};
#else
static const PMbInfo b_mb_type_info[23]={
{MB_TYPE_DIRECT2                                                   , 1, },
{MB_TYPE_16x16|MB_TYPE_P0L0                                       , 1, },
{MB_TYPE_16x16             |MB_TYPE_P0L1                          , 1, },
{MB_TYPE_16x16|MB_TYPE_P0L0|MB_TYPE_P0L1                          , 1, },
{MB_TYPE_16x8 |MB_TYPE_P0L0             |MB_TYPE_P1L0             , 2, },
{MB_TYPE_8x16 |MB_TYPE_P0L0             |MB_TYPE_P1L0             , 2, },
{MB_TYPE_16x8              |MB_TYPE_P0L1             |MB_TYPE_P1L1, 2, },
{MB_TYPE_8x16              |MB_TYPE_P0L1             |MB_TYPE_P1L1, 2, },
{MB_TYPE_16x8 |MB_TYPE_P0L0                          |MB_TYPE_P1L1, 2, },
{MB_TYPE_8x16 |MB_TYPE_P0L0                          |MB_TYPE_P1L1, 2, },
{MB_TYPE_16x8              |MB_TYPE_P0L1|MB_TYPE_P1L0             , 2, },
{MB_TYPE_8x16              |MB_TYPE_P0L1|MB_TYPE_P1L0             , 2, },
{MB_TYPE_16x8 |MB_TYPE_P0L0             |MB_TYPE_P1L0|MB_TYPE_P1L1, 2, },
{MB_TYPE_8x16 |MB_TYPE_P0L0             |MB_TYPE_P1L0|MB_TYPE_P1L1, 2, },
{MB_TYPE_16x8              |MB_TYPE_P0L1|MB_TYPE_P1L0|MB_TYPE_P1L1, 2, },
{MB_TYPE_8x16              |MB_TYPE_P0L1|MB_TYPE_P1L0|MB_TYPE_P1L1, 2, },
{MB_TYPE_16x8 |MB_TYPE_P0L0|MB_TYPE_P0L1|MB_TYPE_P1L0             , 2, },
{MB_TYPE_8x16 |MB_TYPE_P0L0|MB_TYPE_P0L1|MB_TYPE_P1L0             , 2, },
{MB_TYPE_16x8 |MB_TYPE_P0L0|MB_TYPE_P0L1             |MB_TYPE_P1L1, 2, },
{MB_TYPE_8x16 |MB_TYPE_P0L0|MB_TYPE_P0L1             |MB_TYPE_P1L1, 2, },
{MB_TYPE_16x8 |MB_TYPE_P0L0|MB_TYPE_P0L1|MB_TYPE_P1L0|MB_TYPE_P1L1, 2, },
{MB_TYPE_8x16 |MB_TYPE_P0L0|MB_TYPE_P0L1|MB_TYPE_P1L0|MB_TYPE_P1L1, 2, },
{MB_TYPE_8x8  |MB_TYPE_P0L0|MB_TYPE_P0L1|MB_TYPE_P1L0|MB_TYPE_P1L1, 4, },
};
#endif

static const PMbInfo b_sub_mb_type_info[13]={
{MB_TYPE_DIRECT2                                                   , 1, },
{MB_TYPE_16x16|MB_TYPE_P0L0                                       , 1, },
{MB_TYPE_16x16             |MB_TYPE_P0L1                          , 1, },
{MB_TYPE_16x16|MB_TYPE_P0L0|MB_TYPE_P0L1                          , 1, },
{MB_TYPE_16x8 |MB_TYPE_P0L0             |MB_TYPE_P1L0             , 2, },
{MB_TYPE_8x16 |MB_TYPE_P0L0             |MB_TYPE_P1L0             , 2, },
{MB_TYPE_16x8              |MB_TYPE_P0L1             |MB_TYPE_P1L1, 2, },
{MB_TYPE_8x16              |MB_TYPE_P0L1             |MB_TYPE_P1L1, 2, },
{MB_TYPE_16x8 |MB_TYPE_P0L0|MB_TYPE_P0L1|MB_TYPE_P1L0|MB_TYPE_P1L1, 2, },
{MB_TYPE_8x16 |MB_TYPE_P0L0|MB_TYPE_P0L1|MB_TYPE_P1L0|MB_TYPE_P1L1, 2, },
{MB_TYPE_8x8  |MB_TYPE_P0L0             |MB_TYPE_P1L0             , 4, },
{MB_TYPE_8x8               |MB_TYPE_P0L1             |MB_TYPE_P1L1, 4, },
{MB_TYPE_8x8  |MB_TYPE_P0L0|MB_TYPE_P0L1|MB_TYPE_P1L0|MB_TYPE_P1L1, 4, },
};

static const uint8_t dequant4_coeff_init[6][3]={
  {10,13,16},
  {11,14,18},
  {13,16,20},
  {14,18,23},
  {16,20,25},
  {18,23,29},
};

static const uint8_t dequant8_coeff_init_scan[16] = {
  0,3,4,3, 3,1,5,1, 4,5,2,5, 3,1,5,1
};
static const uint8_t dequant8_coeff_init[6][6]={
  {20,18,32,19,25,24},
  {22,19,35,21,28,26},
  {26,23,42,24,33,31},
  {28,25,45,26,35,33},
  {32,28,51,30,40,38},
  {36,32,58,34,46,43},
};

/* Deblocking filter (p153) */
static const uint8_t alpha_table_hw[52*3] = {
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  4,  4,  5,  6,
     7,  8,  9, 10, 12, 13, 15, 17, 20, 22,
    25, 28, 32, 36, 40, 45, 50, 56, 63, 71,
    80, 90,101,113,127,144,162,182,203,226,
   255,255,
   255,255,255,255,255,255,255,255,255,255,255,255,255,
   255,255,255,255,255,255,255,255,255,255,255,255,255,
   255,255,255,255,255,255,255,255,255,255,255,255,255,
   255,255,255,255,255,255,255,255,255,255,255,255,255,
};
static const uint8_t beta_table_hw[52*3] = {
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  2,  2,  2,  3,
     3,  3,  3,  4,  4,  4,  6,  6,  7,  7,
     8,  8,  9,  9, 10, 10, 11, 11, 12, 12,
    13, 13, 14, 14, 15, 15, 16, 16, 17, 17,
    18, 18,
    18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
    18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
    18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
    18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
};
static const uint8_t tc0_table_hw[52*3][3] = {
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 1 },
    { 0, 0, 1 }, { 0, 0, 1 }, { 0, 0, 1 }, { 0, 1, 1 }, { 0, 1, 1 }, { 1, 1, 1 },
    { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 2 }, { 1, 1, 2 }, { 1, 1, 2 },
    { 1, 1, 2 }, { 1, 2, 3 }, { 1, 2, 3 }, { 2, 2, 3 }, { 2, 2, 4 }, { 2, 3, 4 },
    { 2, 3, 4 }, { 3, 3, 5 }, { 3, 4, 6 }, { 3, 4, 6 }, { 4, 5, 7 }, { 4, 5, 8 },
    { 4, 6, 9 }, { 5, 7,10 }, { 6, 8,11 }, { 6, 8,13 }, { 7,10,14 }, { 8,11,16 },
    { 9,12,18 }, {10,13,20 }, {11,15,23 }, {13,17,25 },
    {13,17,25 }, {13,17,25 }, {13,17,25 }, {13,17,25 }, {13,17,25 }, {13,17,25 },
    {13,17,25 }, {13,17,25 }, {13,17,25 }, {13,17,25 }, {13,17,25 }, {13,17,25 },
    {13,17,25 }, {13,17,25 }, {13,17,25 }, {13,17,25 }, {13,17,25 }, {13,17,25 },
    {13,17,25 }, {13,17,25 }, {13,17,25 }, {13,17,25 }, {13,17,25 }, {13,17,25 },
    {13,17,25 }, {13,17,25 }, {13,17,25 }, {13,17,25 }, {13,17,25 }, {13,17,25 },
    {13,17,25 }, {13,17,25 }, {13,17,25 }, {13,17,25 }, {13,17,25 }, {13,17,25 },
    {13,17,25 }, {13,17,25 }, {13,17,25 }, {13,17,25 }, {13,17,25 }, {13,17,25 },
    {13,17,25 }, {13,17,25 }, {13,17,25 }, {13,17,25 }, {13,17,25 }, {13,17,25 },
    {13,17,25 }, {13,17,25 }, {13,17,25 }, {13,17,25 },
};


#endif /* AVCODEC_H264DATA_H */
