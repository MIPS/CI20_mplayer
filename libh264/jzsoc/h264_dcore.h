/*****************************************************************************
 *
 * JZC 4760 H264 Decoder Architecture
 *
 ****************************************************************************/

#ifndef __JZC_H264_DCORE_H__
#define __JZC_H264_DCORE_H__


typedef struct H264_Frame_Ptr{
  uint8_t * y_ptr;
  uint8_t * u_ptr;
  uint8_t * v_ptr;
}H264_Frame_Ptr;

typedef struct H264_Frame_LPtr{
  uint8_t * y_ptr;
  uint8_t * uv_ptr;
  uint8_t * rota_y_ptr;
  uint8_t * rota_uv_ptr;
}H264_Frame_LPtr;

typedef struct H264_Slice_GlbARGs{
  uint8_t deblocking_filter;
  uint8_t slice_type;
  uint8_t use_weight;
  uint8_t use_weight_chroma;
  int8_t luma_log2_weight_denom;
  int8_t chroma_log2_weight_denom;

  uint8_t mb_width;
  uint8_t mb_height;
  int qp_thresh_aux;
  
  uint16_t linesize;
  uint16_t uvlinesize;
  H264_Frame_Ptr current_picture;
  H264_Frame_LPtr line_current_picture;

  short luma_weight[32][2][2];
  short chroma_weight[32][2][2][2];
}H264_Slice_GlbARGs;

#define SLICE_T_CC_LINE ((sizeof(struct H264_Slice_GlbARGs)+31)/32)//=58

typedef struct H264_MB_Ctrl_DecARGs{
  uint16_t next_mb_len;
  uint16_t ctrl_len;

  uint8_t top_mb_type;
  uint8_t qscale;
  uint8_t qp_left;
  uint8_t qp_top;//

  uint16_t cbp;
  uint8_t chroma_qp[2];//

  uint8_t chroma_qp_left[2];
  uint8_t chroma_qp_top[2];//

  uint8_t dblk_start;
  unsigned int dc_dequant4_coeff_Y;
  unsigned int dc_dequant4_coeff_U;
  unsigned int dc_dequant4_coeff_V;
  int8_t new_slice;
  uint8_t mb_x;
  uint8_t mb_y;
  uint8_t left_mb_type;
  unsigned int mb_type;
  uint16_t sub_mb_type[4];
  uint8_t nnz_tl_8bits;
}H264_MB_Ctrl_DecARGs;

typedef struct{
  uint8_t dmb_nnz[16 + 8];//FIXME: filter_mb_hw assume dmb_nnz as just after H264_MB_Ctrl_DecARGs
  DCTELEM mb[16*24];
  //unsigned int error_add_cbp;
}H264_MB_Res;

typedef struct H264_MB_Intra_DecARGs{
  uint8_t chroma_pred_mode;
  uint8_t intra16x16_pred_mode;
  uint8_t dummy[2];
  uint32_t intra4x4_pred_mode0;
  uint32_t intra4x4_pred_mode1;
  unsigned int topleft_samples_available;
  unsigned int topright_samples_available;
}H264_MB_Intra_DecARGs;

typedef struct H264_MB_InterP_DecARGs{
  int mv_num;
  int8_t ref_cache[8];
  int mv_reduce_type;
  int16_t mv_top_left_cache[8][2];
  int16_t mv_body_cache[16][2];
}H264_MB_InterP_DecARGs;

#define PF_MV_OFFSET_BY_WORDS (1+2+1+8)

typedef struct H264_MB_InterB_DecARGs{
  int mv_num;
  int8_t ref_cache[2][8];
  int mv_reduce_type;
  int16_t mv_top_left_cache[2][8][2];
  int16_t mv_body_cache[2][16][2];
}H264_MB_InterB_DecARGs;
#define BF_MV_OFFSET_BY_WORDS (1+4+1+16)

#if 0 /* Not actually used, just for understanding. */
/*--------------------------------------------
 * intra and inter pred infos;
 *
 * ref_cache[8]:     |-------|
 *                   | 4 | 5 |
 *               |---|-------|
 *               | 6 | 0 | 1 |
 *               |---|-------|
 *               | 7 | 2 | 3 |
 *               |---|-------|
 *
 * mv_top_left_cache[8]:     |---------------|
 *                           | 0 | 1 | 2 | 3 |
 *                       |---|---------------|
 *                       | 4 | 
 *                       |---|
 *                       | 5 |
 *                       |---|
 *                       | 6 |
 *                       |---|
 *                       | 7 |
 *                       |---|
 ---------------------------------------------*/
#ifdef INTRA_MB
typedef struct H264_MB_Pred_DecARGs{
  uint8_t chroma_pred_mode;
  uint8_t intra16x16_pred_mode;
  uint8_t dummy[2];
  uint8_t intra4x4_pred_mode_cache[16];
  unsigned int topleft_samples_available;
  unsigned int topright_samples_available;
}H264_MB_Pred_DecARGs;
#elif defined(INTER_MB)
#ifdef P_SLICE
typedef struct H264_MB_Pred_DecARGs{
  int8_t ref_cache[8];
  int16_t mv_top_left_cache[8][2];
  int mv_reduce_type;
  int16_t mv_body_cache[1~16][2];
}H264_MB_Pred_DecARGs;
#elif defined(B_SLICE)
typedef struct H264_MB_Pred_DecARGs{
  int8_t ref_cache[2][8];
  int16_t mv_top_left_cache[2][8][2];
  int mv_reduce_type;
  int16_t mv_body_cache[2][1~16][2];
}H264_MB_Pred_DecARGs;
#endif
#endif

/*--------------------------------------------
 * nnz and residuals;
 *
 * non_zero_count_cache[32]:
 *  0  ~ 15 : nnz luma;
 *  16 ~ 23 : nnz chroma;
 *       24 : nnz luma dc;
 *  25 ~ 26 : nnz chroma dc;
 *       27 : nnz top_left reducde for dblk;
 *  28 ~ 29 : reserved;
 *  30 ~ 31 : total_coeff_count;
 ---------------------------------------------*/
typedef struct H264_MB_RES_DecARGs{
  uint8_t non_zero_count_cache[32];
  uint8_t index[0~384];
  int16_t residual[0~384];
}H264_MB_RES_DecARGs;
#endif


/*--------------------------------------------
 * internal variables.
 ---------------------------------------------*/
typedef struct H264_DCORE_T{
  uint8_t *dblk_up_addr_y;
  uint8_t *dblk_up_addr_u;
  uint8_t *dblk_up_addr_v;
}H264_DCORE_T;


typedef struct H264_XCH2_T{
  uint8_t * dblk_des_ptr;
  uint8_t * dblk_mv_ptr;
#ifndef JZC_DBLKLI_OPT
  uint8_t * dblk_upout_addr;
#endif
  uint8_t * dblk_out_addr;
  uint8_t * dblk_out_addr_c;
#ifndef JZC_DBLKLI_OPT
  uint8_t * dblk_upout_addr_c;
#endif
  short   * add_error_buf;
  uint8_t * future_ptr;
#ifdef JZC_DBLKLI_OPT
#ifdef JZC_ROTA90_OPT
  uint8_t *rota_upmb_ybuf;
  uint8_t *rota_upmb_ubuf;
  uint8_t *rota_upmb_vbuf;
#endif
  int     * gp1_chain_ptr;
#endif
  //int     * gp2_chain_ptr;
}H264_XCH2_T;


#endif //__JZC_H264_DCORE_H__
