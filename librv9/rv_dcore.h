/*****************************************************************************
 *
 * JZC RV Decoder Architecture
 *
 ****************************************************************************/

#ifndef __JZC_RV_ARCH_H__
#define __JZC_RV_ARCH_H__

typedef struct RV_Slice_Ptr{
  uint8_t *ptr[3];
}RV_Slice_Ptr;

typedef struct RV_Slice_GlbARGs{
 uint8_t deblocking_filter;
  uint8_t slice_type;
  uint8_t pict_type;
  uint8_t si_type;

  uint8_t *last_data[3];
  uint8_t *next_data[3];

  uint8_t *edge_emu_buffer;
  uint16_t intra_types_stride;
  uint16_t h_edge_pos;
  uint16_t v_edge_pos;
  uint8_t mb_width;
  uint8_t mb_height;
  uint8_t resync_mb_x;
  uint8_t resync_mb_y;

  uint8_t chroma_y_shift;
  uint8_t chroma_x_shift;
  uint8_t lowres;
  uint8_t draw_horiz_band;
  uint8_t picture_structure;

  int qp_thresh_aux;
  uint8_t mb_stride;
  uint16_t linesize;
  uint16_t uvlinesize;
  RV_Slice_Ptr current_picture;
}RV_Slice_GlbARGs;

typedef struct RV_MB_DecARGs{
  int cbp;
  int cbp2;
  int next_bt;
  uint8_t  qscale;
  uint32_t avail_cache[12];
  uint32_t mb_type;
  uint8_t  is16;
  uint8_t  block_type;
  
  uint8_t mb_x;     
  uint8_t mb_y; 
  int8_t  er_slice;
  uint8_t itype16;
  uint8_t itype8;
  uint8_t itype4[24];
  uint8_t rup[24];
  int8_t  status;
  uint8_t new_slice;
  //uint8_t first_slice_line;
  uint8_t dump[3];
  uint8_t *intra_types;
  DCTELEM block16[16];
  short rv_mv[8][2]; //
  short motion_val[2][4][2]; //
  DCTELEM block[6][64]; // 144+192=336 words
}RV_MB_DecARGs;

#endif
