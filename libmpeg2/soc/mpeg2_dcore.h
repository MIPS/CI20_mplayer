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

#ifndef __MPEG2_DCORE_H__
#define __MPEG2_DCORE_H__

typedef unsigned int paddr_t;
typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

typedef struct _M2D_BitStream {
  uint32_t buffer;    /*BS buffer physical address*/
  uint8_t  bit_ofst;  /*bit offset for first word*/
}_M2D_BitStream;


typedef struct _M2D_SliceInfo {
  uint8_t mb_width; /*frame width(mb layer)*/
  uint8_t mb_height; /*frame height(mb layer)*/
  uint8_t mb_pos_x; /*slice_head position*/
  uint8_t mb_pos_y;

  uint32_t coef_qt[4][16]; /*qt table for vmau module*/
  uint32_t scan[16]; /*scan table*/
  
  uint8_t coding_type;  /*I_TYPE(1), P_TYPE(2), B_TYPE(3), D_TYPE(4) */
  uint8_t fr_pred_fr_dct; /*frame_pred_frame_dct*/
  uint8_t pic_type; /*TOP_FIELD(1), BOTTOM_FIELD(2), FRAME_PICTURE(3)*/
  uint8_t conceal_mv; /*concealment_motion_vectors*/
  uint8_t intra_dc_pre;  /*intra_dc_precision*/
  uint8_t intra_vlc_format; /*1, B15; 0, B14*/
  uint8_t mpeg1; /**/
  uint8_t top_fi_first; /*top field first*/
  uint8_t q_scale_type; 
  uint8_t qs_code; /*quantizer_scale_code*/
  uint8_t dmv_ofst; /*1, +1; 0, -1*/
  uint8_t sec_fld; /*current picture is second field*/

  uint8_t f_code[2][2]; /*{backward motion, forward motion}{1, 0}*/
  
  /*--------------------------------------------------------------------------------
   * pic_ref[a][b].
   * a | 0 | 1 |
   * --|---|---|---
   *   | C | Y |
   * ---------------------
   * b |  0  |  1   |    2    |    3    |    4    |    5    |    6    |   7     |
   *---|-----|------|---------|---------|---------|---------|---------|---------|-
   *   |f_frm| b_frm|f_top_fld|b_top_fld|f_bot_fld|b_bot_fld|c_top_fld|c_bot_fld|
   --------------------------------------------------------------------------------*/
  /* f->forward , b->backward , c->current */
  paddr_t pic_ref[2][8]; /*The phy_addr of frame reference picture*/
  paddr_t pic_cur[2]; /*the phy_addr of current picture. {C, Y}*/

  _M2D_BitStream  bs_buf;

  /*descriptor address*/
  paddr_t *des_va, des_pa;
}_M2D_SliceInfo;

#endif
