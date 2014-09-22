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

extern short mpFrame;
const int y_dc_table[16] = { 0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15 };

char scan4x4[24] = {0,1,4,5,2,3,6,7,8,9,12,13,10,11,14,15, 16,17,18,19,20,21,22,23};
char cache_to_bits[16] = {4,5,6,7,1,2,25,26,11,19,27,35,8,16,32,40};
char scan4_to_bits[16] = {4,5,6,7,1,2,25,26,11,19,27,35,8,16,32,40};

char right_pos[8] = {1*8+7,2*8+7,3*8+7,4*8+7, 1*8+2,2*8+2, 4*8+2,5*8+2};
char bottom_pos[8] = {4*8+4,4*8+5,4*8+6,4*8+7, 2*8+1,2*8+2, 5*8+1,5*8+2};

uint8_t scan4[16 + 2*4]={
  0, 1, 4, 5,
  2, 3, 6, 7,
  8, 9,12,13,
 10,11,14,15,
 16,17,18,19,
 20,21,22,23,
};

typedef struct MB_INFO_H264
{
  int nei_avail;          // neighbor mb(top,left,topleft,topright) available
  unsigned short mb_type; // 16 bit for hw_mb_type
  char intra_avail;       // top,left,topleft,topright available for vmau
  char decoder_error;     // error while hw decoding
  short cbp;
  char qscal;
  char direct;
  unsigned short skip_run;
  unsigned int nnz_dct;
  unsigned short coef_cnt;
  unsigned short nnz_br;

  unsigned int bit_count;
  unsigned int nnz_bottom;
  unsigned int nnz_right;
  short residual[408];

  unsigned int intra4x4_pred_mode0;
  unsigned int intra4x4_pred_mode1;
  unsigned char intra16x16_pred_mode;
  unsigned char chroma_pred_mode;
  unsigned char luma_neighbor;
  unsigned char chroma_neighbor;

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
};

struct MB_INFO_H264 mb_info_h264;
struct MB_INFO_H264 mb_info_sde;

int sde_ga_dqp;
unsigned char sde_luma_neighbor;
unsigned char sde_chroma_neighbor;
unsigned int sde_res_index;
unsigned int sde_res_di;
short sde_res_dct[408];
short sde_tmp_blk[64];
int sde_nnz_dct[27];
short sde_v_dc[4];

#undef printf
void gather_mb_info_h264(H264Context *h)
{
  MpegEncContext * const s = &h->s;
  const int mb_xy= s->mb_x + s->mb_y*s->mb_stride;
  int mb_num= s->mb_x + s->mb_y*s->mb_width;
  int i, j, k;


  unsigned int mb_type_new;
  unsigned int mb_type = s->current_picture.mb_type[mb_xy];
  mb_type_new =
    ((!!(mb_type & MB_TYPE_8x8DCT)) << 10) |  // 8x8dct -> gmc
    ((!!(mb_type & (MB_TYPE_INTRA4x4 | MB_TYPE_INTRA16x16))) << 2) | // is_intra -> intra_pcm
    mb_type;
  mb_type_new &= 0xFFFF;

  mb_info_h264.mb_type = mb_type_new;
  mb_info_h264.cbp = IS_SKIP(mb_type) ? 0 : (h->pps.cabac ? (short)h->cbp_table[mb_xy] : (short)h->cbp);
  mb_info_h264.qscal = (char)s->qscale;

  int top_xy, left_xy, topleft_xy, topright_xy;
  int top_type, left_type, topleft_type, topright_type;
  top_xy     = mb_xy  - s->mb_stride;
  left_xy    = mb_xy  - 1;
  topleft_xy = top_xy - 1;
  topright_xy= top_xy + 1;
  top_type     = h->hw_slice_table[top_xy     ] == h->slice_num ? s->current_picture.mb_type[top_xy]     : 0;
  left_type    = h->hw_slice_table[left_xy    ] == h->slice_num ? s->current_picture.mb_type[left_xy]    : 0;
  topleft_type = h->hw_slice_table[topleft_xy ] == h->slice_num ? s->current_picture.mb_type[topleft_xy] : 0;
  topright_type= h->hw_slice_table[topright_xy] == h->slice_num ? s->current_picture.mb_type[topright_xy]: 0;

  int nei_avail = 0;
  nei_avail |= !!left_type << 0;
  nei_avail |= !!top_type << 0;
  nei_avail |= !!topleft_type << 0;
  nei_avail |= !!topright_type << 0;
  mb_info_h264.nei_avail = nei_avail;

  int intra_avail = 0;
  if(!IS_INTRA(top_type) && (top_type==0 || h->pps.constrained_intra_pred))
    intra_avail |= 1 << 0;
  if(!IS_INTRA(left_type) && (left_type==0 || h->pps.constrained_intra_pred))
    intra_avail |= 1 << 1;
  if(!IS_INTRA(topleft_type) && (topleft_type==0 || h->pps.constrained_intra_pred))
    intra_avail |= 1 << 2;
  if(!IS_INTRA(topright_type) && (topright_type==0 || h->pps.constrained_intra_pred))
    intra_avail |= 1 << 3;
  mb_info_h264.intra_avail = intra_avail;

  unsigned int nnz_dct = 0;
  for(i=0; i<27; i++)
    nnz_dct |= (!!sde_nnz_dct[i]) << i;

/*    assign vmau_main_cbp = {nnz_dct_w[26],	// v dc   31 */
/* 			   nnz_dct_w[25],	// u dc   30 */
/* 			   1'b1,		// c error add  29 */
/* 			   nnz_dct_w[24],	// y dc   28 */
/* 			   mb_type_dout_val[`MB_TYPE_IS_INTRA],	// pred en   27 */
/* 			   1'b1,		// y error add  26 */
/* 			   matix_type[1:0],	// matix type: 00: luma 4x4, 10: luma 8x8, 11: luma 16x16   25,24 */
/* 			   nnz_dct_w[23:0]	// ac  23 ~ 0 */
/* 			   }; */
  unsigned int vmau_main_cbp = 0;
  vmau_main_cbp |= (nnz_dct & (0x3 << 25)) << 5;
  vmau_main_cbp |= 1 << 29;
  vmau_main_cbp |= (nnz_dct & (0x1 << 24)) << 4;
  vmau_main_cbp |= (!!(mb_type_new & (1 << 2))) << 27;
  vmau_main_cbp |= 1 << 26;
  int matix_type = 0;
  if (mb_type & MB_TYPE_8x8DCT) matix_type = 2;
  if (mb_type & MB_TYPE_INTRA16x16) matix_type = 3;
  vmau_main_cbp |= (matix_type & 0x3) << 24;
  vmau_main_cbp |= nnz_dct & ((1 << 24) - 1);

  mb_info_h264.nnz_dct = vmau_main_cbp;

  mb_info_h264.coef_cnt = sde_res_di << 1;

  for (i=0; i<sde_res_di; i++)
    mb_info_h264.residual[i] = (short)sde_res_dct[i];

  uint32_t nnz_bottom_right = 0;
  for(i=0; i<8; i++)
    nnz_bottom_right |= (!!(h->non_zero_count_cache[bottom_pos[i]])) << i;
  for(i=0; i<8; i++)
    nnz_bottom_right |= (!!(h->non_zero_count_cache[right_pos[i]])) << (i+8);
  mb_info_h264.nnz_br = nnz_bottom_right;

  uint32_t nnz_bottom = 0;
  uint32_t nnz_right = 0;
  for(i=0; i<8; i++)
    nnz_bottom |= (((h->non_zero_count_cache[bottom_pos[i]] == 16) ? 15 :
		    h->non_zero_count_cache[bottom_pos[i]]) & 0xF) << (i*4);
  for(i=0; i<8; i++)
    nnz_right |= (((h->non_zero_count_cache[right_pos[i]] == 16) ? 15 :
		   h->non_zero_count_cache[right_pos[i]]) & 0xF) << (i*4);
  mb_info_h264.nnz_bottom = nnz_bottom;
  mb_info_h264.nnz_right = nnz_right;

  mb_info_h264.bit_count = h->pps.cabac ? h->cabac.bytestream : get_bits_count(&s->gb);

  unsigned int intra4x4_pred_mode0 = 0;
  unsigned int intra4x4_pred_mode1 = 0;
  if IS_INTRA4x4(mb_type){
    intra4x4_pred_mode0 = (h->intra4x4_pred_mode_cache[scan8[0]] | 
			   (h->intra4x4_pred_mode_cache[scan8[1]] << 4)|
			   (h->intra4x4_pred_mode_cache[scan8[2]] << 8)|
			   (h->intra4x4_pred_mode_cache[scan8[3]] << 12)|
			   (h->intra4x4_pred_mode_cache[scan8[4]] << 16)|
			   (h->intra4x4_pred_mode_cache[scan8[5]] << 20)|
			   (h->intra4x4_pred_mode_cache[scan8[6]] << 24)|
			   (h->intra4x4_pred_mode_cache[scan8[7]] << 28)
			   );
    intra4x4_pred_mode1 = (h->intra4x4_pred_mode_cache[scan8[8]] | 
			   (h->intra4x4_pred_mode_cache[scan8[1+8]] << 4)|
			   (h->intra4x4_pred_mode_cache[scan8[2+8]] << 8)|
			   (h->intra4x4_pred_mode_cache[scan8[3+8]] << 12)|
			   (h->intra4x4_pred_mode_cache[scan8[4+8]] << 16)|
			   (h->intra4x4_pred_mode_cache[scan8[5+8]] << 20)|
			   (h->intra4x4_pred_mode_cache[scan8[6+8]] << 24)|
			   (h->intra4x4_pred_mode_cache[scan8[7+8]] << 28)
			   );
  }else{
    intra4x4_pred_mode0 = h->intra16x16_pred_mode;
  }
#undef printf

  mb_info_h264.intra4x4_pred_mode0 = intra4x4_pred_mode0;
  mb_info_h264.intra4x4_pred_mode1 = intra4x4_pred_mode1;
  
  mb_info_h264.intra16x16_pred_mode = (unsigned char)h->intra16x16_pred_mode;
  mb_info_h264.chroma_pred_mode = (unsigned char)h->chroma_pred_mode;
  mb_info_h264.luma_neighbor = sde_luma_neighbor;
  mb_info_h264.chroma_neighbor = !!sde_chroma_neighbor;

  if (mpFrame == -1 && s->mb_x == 0 && s->mb_y == 0) {
    printf("intra4x4_pred_mode0 = %x,%x, %d, mb_type_new = %d\n",intra4x4_pred_mode0,intra4x4_pred_mode1,IS_INTRA4x4(mb_type),IS_INTRA4x4(mb_type_new));
    printf("mb_info_h264.intra16x16_pred_mode = %x,mb_info_h264.chroma_pred_mode=%x\n",mb_info_h264.intra16x16_pred_mode,mb_info_h264.chroma_pred_mode);
  }

  unsigned char ref_cache_new[2][4];
  for (i=0; i<2; i++){
    for (j=0; j<4; j++){
      if (h->ref_cache[i][scan8[j<<2]] == LIST_NOT_USED)
	ref_cache_new[i][j] = 17;
      else if (h->ref_cache[i][scan8[j<<2]] == PART_NOT_AVAILABLE)
	ref_cache_new[i][j] = 18;
      else if (h->ref_cache[i][scan8[j<<2]] >= 0)
	ref_cache_new[i][j] = h->ref_cache[i][scan8[j<<2]];
    }
  }
  for (i=0; i<2; i++)
    for (j=0; j<4; j++)
      mb_info_h264.ref[i][j] = ref_cache_new[i][j];
  for (i=0; i<4; i++)
    mb_info_h264.sub_mb_type[i] = (short)h->sub_mb_type[i];

  // mv compress
  unsigned short mv_len;
  unsigned short mv_mode;
  short mv_comp[32][2];
  int list;
  mv_len = 0;
  mv_mode = 0;
  int mc_i = 0;
  for (list = 0; list < h->list_count; list++) {
    if (IS_16X16(mb_type)) {
      mv_len += 1;
      mv_mode |= 0;
      for (k=0; k<2; k++) 
	mv_comp[mc_i][k] = h->mv_cache[list][scan8[0]][k];
      mc_i++;
    } else if (IS_16X8(mb_type)) {
      mv_len += 2;
      mv_mode |= 1;
      for (k=0; k<2; k++) 
	mv_comp[mc_i][k] = h->mv_cache[list][scan8[0]][k];
      mc_i++;
      for (k=0; k<2; k++) 
	mv_comp[mc_i][k] = h->mv_cache[list][scan8[8]][k];
      mc_i++;
    } else if (IS_8X16(mb_type)) {
      mv_len += 2;
      mv_mode |= 2;
      for (k=0; k<2; k++) 
	mv_comp[mc_i][k] = h->mv_cache[list][scan8[0]][k];
      mc_i++;
      for (k=0; k<2; k++) 
	mv_comp[mc_i][k] = h->mv_cache[list][scan8[4]][k];
      mc_i++;
    } else if (IS_8X8(mb_type)) {
      mv_len += 0;
      mv_mode |= 3;
      for (i=0; i<4; i++) {
	if (IS_SUB_8X8(h->sub_mb_type[i])) {
	  mv_len += 1;
	  mv_mode |= 0 << (i*2 + 2);
	  for (k=0; k<2; k++) 
	    mv_comp[mc_i][k] = h->mv_cache[list][scan8[0 + i*4]][k];
	  mc_i++;
	} else if (IS_SUB_8X4(h->sub_mb_type[i])) {
	  mv_len += 2;
	  mv_mode |= 1 << (i*2 + 2);
	  for (k=0; k<2; k++) 
	    mv_comp[mc_i][k] = h->mv_cache[list][scan8[0 + i*4]][k];
	  mc_i++;
	  for (k=0; k<2; k++) 
	    mv_comp[mc_i][k] = h->mv_cache[list][scan8[2 + i*4]][k];
	  mc_i++;
	} else if (IS_SUB_4X8(h->sub_mb_type[i])) {
	  mv_len += 2;
	  mv_mode |= 2 << (i*2 + 2);
	  for (k=0; k<2; k++) 
	    mv_comp[mc_i][k] = h->mv_cache[list][scan8[0 + i*4]][k];
	  mc_i++;
	  for (k=0; k<2; k++) 
	    mv_comp[mc_i][k] = h->mv_cache[list][scan8[1 + i*4]][k];
	  mc_i++;
	} else {
	  mv_len += 4;
	  mv_mode |= 3 << (i*2 + 2);
	  for (k=0; k<2; k++) 
	    mv_comp[mc_i][k] = h->mv_cache[list][scan8[0 + i*4]][k];
	  mc_i++;
	  for (k=0; k<2; k++) 
	    mv_comp[mc_i][k] = h->mv_cache[list][scan8[1 + i*4]][k];
	  mc_i++;
	  for (k=0; k<2; k++) 
	    mv_comp[mc_i][k] = h->mv_cache[list][scan8[2 + i*4]][k];
	  mc_i++;
	  for (k=0; k<2; k++) 
	    mv_comp[mc_i][k] = h->mv_cache[list][scan8[3 + i*4]][k];
	  mc_i++;
	}
      }
    }
  }

  mb_info_h264.mv_len = mv_len;
  mb_info_h264.mv_mode = mv_mode;
  // mv
  for (i=0; i<mv_len; i++)
    for (j=0; j<2; j++)
      mb_info_h264.mv[i][j] = (short)mv_comp[i][j];

  list = 0;
  {
    // mv bottom
    for (j=0; j<4; j++)
      for (k=0; k<2; k++)
	mb_info_h264.mv_bottom_l0[j][k] = (short)h->mv_cache[list][bottom_pos[j]][k];
    // mv right
    for (j=0; j<4; j++)
      for (k=0; k<2; k++)
	mb_info_h264.mv_right_l0[j][k] = (short)h->mv_cache[list][right_pos[j]][k];
    // mvd bottom
    for (j=0; j<4; j++){
      for (k=0; k<2; k++){
	short mvd_br = (short)h->mvd_cache[list][bottom_pos[j]][k];
	unsigned short c_mvd_br = abs(mvd_br);
	unsigned short c_mvd_br_l = c_mvd_br & 0x3F;
	if (c_mvd_br & ~(0x3F))
	  c_mvd_br_l |= 1<<6;
	if(IS_SKIP(mb_type)) // skip mb's mvd all 0
	  c_mvd_br_l = 0;
	mb_info_h264.mvd_bottom_l0[j][k] = c_mvd_br_l;
      }
    }
    // mvd right
    for (j=0; j<4; j++){
      for (k=0; k<2; k++){
	short mvd_br = (short)h->mvd_cache[list][right_pos[j]][k];
	unsigned short c_mvd_br = abs(mvd_br);
	unsigned short c_mvd_br_l = c_mvd_br & 0x3F;
	if (c_mvd_br & ~(0x3F))
	  c_mvd_br_l |= 1<<6;
	if(IS_SKIP(mb_type)) // skip mb's mvd all 0
	  c_mvd_br_l = 0;
	mb_info_h264.mvd_right_l0[j][k] = c_mvd_br_l;
      }
    }
  }

  list = 1;
  if (h->slice_type == FF_B_TYPE) {
    // mv bottom
    for (j=0; j<4; j++)
      for (k=0; k<2; k++)
	mb_info_h264.mv_bottom_l1[j][k] = (short)h->mv_cache[list][bottom_pos[j]][k];
    // mv right
    for (j=0; j<4; j++)
      for (k=0; k<2; k++)
	mb_info_h264.mv_right_l1[j][k] = (short)h->mv_cache[list][right_pos[j]][k];
    // mvd bottom
    for (j=0; j<4; j++){
      for (k=0; k<2; k++){
	short mvd_br = (short)h->mvd_cache[list][bottom_pos[j]][k];
	unsigned short c_mvd_br = abs(mvd_br);
	unsigned short c_mvd_br_l = c_mvd_br & 0x3F;
	if (c_mvd_br & ~(0x3F))
	  c_mvd_br_l |= 1<<6;
	if(IS_SKIP(mb_type)) // skip mb's mvd all 0
	  c_mvd_br_l = 0;
	mb_info_h264.mvd_bottom_l1[j][k] = c_mvd_br_l;
      }
    }
    // mvd right
    for (j=0; j<4; j++){
      for (k=0; k<2; k++){
	short mvd_br = (short)h->mvd_cache[list][right_pos[j]][k];
	unsigned short c_mvd_br = abs(mvd_br);
	unsigned short c_mvd_br_l = c_mvd_br & 0x3F;
	if (c_mvd_br & ~(0x3F))
	  c_mvd_br_l |= 1<<6;
	if(IS_SKIP(mb_type)) // skip mb's mvd all 0
	  c_mvd_br_l = 0;
	mb_info_h264.mvd_right_l1[j][k] = c_mvd_br_l;
      }
    }
  }

#ifdef FPRINT_GATHER_OPT
  if ( (crc_frm == FPRINT_GATHER_FRM) && (h->slice_num == FPRINT_GATHER_SLICE) ) {
    unsigned int info0 = (mb_info_h264.mb_type & 0xFFFF) + ((mb_info_h264.intra_avail & 0xFF) << 16);
    unsigned char direct = (((!!(IS_DIRECT(mb_info_h264.sub_mb_type[3]))) << 3) +
			    ((!!(IS_DIRECT(mb_info_h264.sub_mb_type[2]))) << 2) +
			    ((!!(IS_DIRECT(mb_info_h264.sub_mb_type[1]))) << 1) +
			    ((!!(IS_DIRECT(mb_info_h264.sub_mb_type[0]))) << 0)
			    );
    unsigned int info1 = (mb_info_h264.cbp & 0xFFFF) + (sde_ga_dqp << 16) + ((direct & 0xFF) << 24);
    fprintf(sde_mb_p,"  { /* mb_x:%d, mb_y:%d */\n",s->mb_x,s->mb_y);
#if 0
    fprintf(sde_mb_p,"    0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, \n",
	    info0,info1,mb_info_h264.nnz_bottom,mb_info_h264.nnz_right,mb_info_h264.bit_count,
	    mb_info_h264.nnz_dct,mb_info_h264.coef_cnt,mb_info_h264.nnz_br);
#else
    fprintf(sde_mb_p,"    0x%x,\t/* mb_type:0x%x, intra_avail:0x%x*/\n",
	    info0,mb_info_h264.mb_type,mb_info_h264.intra_avail);
    fprintf(sde_mb_p,"    0x%x,\t/* cbp:0x%x, dqp:%d, direct:0x%x*/\n",
	    info1,mb_info_h264.cbp,sde_ga_dqp,direct);
    fprintf(sde_mb_p,"    0x%x,\t/* nnz_bottom:0x%x*/\n",mb_info_h264.nnz_bottom);
    fprintf(sde_mb_p,"    0x%x,\t/* nnz_right:0x%x*/\n",mb_info_h264.nnz_right);
    fprintf(sde_mb_p,"    0x%x,\t/* bit_count:0x%x*/\n",mb_info_h264.bit_count);
    fprintf(sde_mb_p,"    0x%x,\t/* nnz_dct:0x%x*/\n",mb_info_h264.nnz_dct);
    fprintf(sde_mb_p,"    0x%x,\t/* coef_cnt:0x%x*/\n",mb_info_h264.coef_cnt);
    fprintf(sde_mb_p,"    0x%x,\t/* nnz_br:0x%x*/\n",mb_info_h264.nnz_br);
#endif

    if (mb_info_h264.mb_type & 0x7) {
      unsigned int intra_2 = ((mb_info_h264.intra16x16_pred_mode & 0xFF) +
			      ((mb_info_h264.chroma_pred_mode & 0xFF) << 8) +
			      ((mb_info_h264.luma_neighbor & 0xFF) << 16) +
			      ((mb_info_h264.chroma_neighbor & 0xFF) << 24)
			      );
#if 0
      fprintf(sde_mb_p,"    0x%x, 0x%x, 0x%x, \n",
	      mb_info_h264.intra4x4_pred_mode0,mb_info_h264.intra4x4_pred_mode1,intra_2);
#else
      fprintf(sde_mb_p,"    0x%x,\t/* intra4x4_pred_mode0:0x%x*/\n",mb_info_h264.intra4x4_pred_mode0);
      fprintf(sde_mb_p,"    0x%x,\t/* intra4x4_pred_mode1:0x%x*/\n",mb_info_h264.intra4x4_pred_mode1);
      fprintf(sde_mb_p,"    0x%x,\t/* intra16_mode:%d, chroma_mode:%d, luma_neighbor:0x%x, chroma_neighbor:%d*/ \n",
	      intra_2,mb_info_h264.intra16x16_pred_mode,mb_info_h264.chroma_pred_mode,
	      mb_info_h264.luma_neighbor,mb_info_h264.chroma_neighbor);
#endif
    } else {

      fprintf(sde_mb_p, "    {"); // ref
      for (i=0; i<2; i++){
	fprintf(sde_mb_p, "{");
	for (j=0; j<4; j++)
	  fprintf(sde_mb_p, "%d,",mb_info_h264.ref[i][j]);
	fprintf(sde_mb_p, "},");
      }
      fprintf(sde_mb_p, "},\t/* ref */\n");
      fprintf(sde_mb_p, "    {"); // sub_type
      for (i=0; i<4; i++)
	fprintf(sde_mb_p, "%d,",mb_info_h264.sub_mb_type[i]);
      fprintf(sde_mb_p, "},\t/* sub_type */\n");
      fprintf(sde_mb_p, "    %d,\t/* mv_len */\n",mb_info_h264.mv_len);
      fprintf(sde_mb_p, "    %d,\t/* mv_mode */\n",mb_info_h264.mv_mode);

      // list 0
      fprintf(sde_mb_p, "    {"); // mv bottom
      for (j=0; j<4; j++)
	fprintf(sde_mb_p, "0x%x,",((int*)&mb_info_h264.mv_bottom_l0[0][0])[i]);
      fprintf(sde_mb_p, "},\t/* mv_bottom l0 */\n");
      fprintf(sde_mb_p, "    {"); // mv right
      for (j=0; j<4; j++)
	fprintf(sde_mb_p, "0x%x,",((int*)&mb_info_h264.mv_right_l0[0][0])[i]);
      fprintf(sde_mb_p, "},\t/* mv_right l0 */\n");
      fprintf(sde_mb_p, "    {"); // mvd bottom
      for (j=0; j<4; j++)
	fprintf(sde_mb_p, "0x%x,",((int*)&mb_info_h264.mvd_bottom_l0[0][0])[i]);
      fprintf(sde_mb_p, "},\t/* mvd_bottom l0 */\n");
      fprintf(sde_mb_p, "    {"); // mvd right
      for (j=0; j<4; j++)
	fprintf(sde_mb_p, "0x%x,",((int*)&mb_info_h264.mvd_right_l0[0][0])[i]);
      fprintf(sde_mb_p, "},\t/* mvd_right l0 */\n");

      if (h->slice_type == B_TYPE) {
	fprintf(sde_mb_p, "    {"); // mv bottom
	for (j=0; j<4; j++)
	  fprintf(sde_mb_p, "0x%x,",((int*)&mb_info_h264.mv_bottom_l1[0][0])[i]);
	fprintf(sde_mb_p, "},\t/* mv_bottom l1 */\n");
	fprintf(sde_mb_p, "    {"); // mv right
	for (j=0; j<4; j++)
	  fprintf(sde_mb_p, "0x%x,",((int*)&mb_info_h264.mv_right_l1[0][0])[i]);
	fprintf(sde_mb_p, "},\t/* mv_right l1 */\n");
	fprintf(sde_mb_p, "    {"); // mvd bottom
	for (j=0; j<4; j++)
	  fprintf(sde_mb_p, "0x%x,",((int*)&mb_info_h264.mvd_bottom_l1[0][0])[i]);
	fprintf(sde_mb_p, "},\t/* mvd_bottom l1 */\n");
	fprintf(sde_mb_p, "    {"); // mvd right
	for (j=0; j<4; j++)
	  fprintf(sde_mb_p, "0x%x,",((int*)&mb_info_h264.mvd_right_l1[0][0])[i]);
	fprintf(sde_mb_p, "},\t/* mvd_right l1 */\n");
      }

      fprintf(sde_mb_p, "    {"); // mv
      for (i=0; i<mb_info_h264.mv_len; i++)
	fprintf(sde_mb_p, "0x%x,",((int*)&mb_info_h264.mv[0][0])[i]);
      fprintf(sde_mb_p, "},\n");

    }

    fprintf(sde_mb_p,"  },\n");

  }
#endif // FPRINT_GATHER_OPT

}
