
//#define JZC_P0_MC_PRE_CAL_OPT

#include "h264_bs_cavlc.c"

#ifdef HAVE_BUILTIN_EXPECT
#define likely(x) __builtin_expect ((x) != 0, 1)
#define unlikely(x) __builtin_expect ((x) != 0, 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

#define POLL_FIFO_IS_EMPTY() \
            do{}while ( !( ( ((unsigned int)task_fifo_wp&0x0000FFFF) > (*tcsm0_fifo_rp&0x0000FFFF)) || \
	     ( (((unsigned int)task_fifo_wp&0x0000FFFF) + MAX_TASK_LEN) <= (*tcsm0_fifo_rp&0x0000FFFF) ) ) );  

/*----------------- cavlc mxu opt note: -----------------------------
 * XR1 and XR2 are dedicated to store CAVLC BitStream during decode
 * a slice, should not be used elsewhere,
 --------------------------------------------------------------------*/
#undef printf
uint8_t nnz_left_bak[8];

H264_MB_Ctrl_DecARGs *dMB_Ctrl;
H264_MB_Intra_DecARGs *dMB_Intra;
H264_MB_Res *res;
int *ctrl_end;

__cache_text1__
int loop_cavlc_mb(H264Context *h, const int part_mask){
  MpegEncContext * const s = &h->s;
  GetBitContext *gb= &s->gb;

  for(;;){

#ifdef JZC_PMON_P0n
    PMON_ON(parse);
#endif
    int ret = decode_mb_cavlc_dcore(h);

    VLC_BS_CLOSE(gb);

#ifdef JZC_PMON_P0n
    PMON_OFF(parse);
#endif

    if(ret>=0){

#ifdef JZC_PMON_P0ed
      PMON_ON(dmb);
#endif
      hl_decode_cavlc_mb(h);
#ifdef JZC_PMON_P0ed
      PMON_OFF(dmb);
#endif

    }else{
      ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x, s->mb_y, (AC_ERROR|DC_ERROR|MV_ERROR)&part_mask);
      return -1;
    }

    if(++s->mb_x >= s->mb_width){
      s->mb_x=0;
      ff_draw_horiz_band(s, 16*s->mb_y, 16);
      ++s->mb_y;
      if(FIELD_OR_MBAFF_PICTURE) {
	++s->mb_y;
      }
      if(s->mb_y >= s->mb_height){
	if(get_bits_count(&s->gb) == s->gb.size_in_bits ) {
	  ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x-1, s->mb_y, (AC_END|DC_END|MV_END)&part_mask);
	  return 0;
	}else{
	  ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x, s->mb_y, (AC_END|DC_END|MV_END)&part_mask);
	  return -1;
	}
      }
    }

    if(get_bits_count(&s->gb) >= s->gb.size_in_bits && s->mb_skip_run<=0){
      if(get_bits_count(&s->gb) == s->gb.size_in_bits ){
	ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x-1, s->mb_y, (AC_END|DC_END|MV_END)&part_mask);
	return 0;
      }else{
	ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x, s->mb_y, (AC_ERROR|DC_ERROR|MV_ERROR)&part_mask);
	return -1;
      }
    }
  }
}

static av_always_inline
int fetch_diagonal_mv_cavlc(H264Context *h, uint32_t *C, int i, int list, int part_width){
  const int topright_ref= h->ref_cache[list][ i - 8 + part_width ];

  if(topright_ref != PART_NOT_AVAILABLE){
    *C= *(uint32_t*)h->mv_cache[list][ i - 8 + part_width ];
    return topright_ref;
  }else{
    *C= *(uint32_t*)h->mv_cache[list][ i - 8 - 1 ];
    return h->ref_cache[list][ i - 8 - 1 ];
  }
}

__cache_text1__
uint32_t pred_motion_cavlc(H264Context *h, int n, int part_width, int list, int ref){
  const int index8= scan8[n];
  const int top_ref=      h->ref_cache[list][ index8 - 8 ];
  const int left_ref=     h->ref_cache[list][ index8 - 1 ];
  const uint32_t A= *((uint32_t*)h->mv_cache[list][ index8 - 1 ]);
  const uint32_t B= *((uint32_t*)h->mv_cache[list][ index8 - 8 ]);
  uint32_t C;
  int diagonal_ref, match_count;

  diagonal_ref= fetch_diagonal_mv_cavlc(h, &C, index8, list, part_width);
  match_count= (diagonal_ref==ref) + (top_ref==ref) + (left_ref==ref);

  if(match_count > 1){ //most common
    S32I2M(xr7,A);
    S32I2M(xr8,B);
    S32I2M(xr3,C);
    D16MAX(xr4,xr7,xr8);//xr4=max of (A,B)
    D16MIN(xr5,xr7,xr8);//xr5=min of (A,B)
    D16MIN(xr4,xr4,xr3);//xr4:comp max(A,B) and C, get rid of the MAX of (A,B,C)
    D16MAX(xr4,xr4,xr5);//xr4:get rid of the MIN of (A,B,C)
    return S32M2I(xr4);
  }else if(match_count==1){
    if(left_ref==ref){
      return A;
    }else if(top_ref==ref){
      return B;
    }else{
      return C;
    }
  }else{
    if(top_ref == PART_NOT_AVAILABLE && diagonal_ref == PART_NOT_AVAILABLE && left_ref != PART_NOT_AVAILABLE){
      return A;
    }else{
      S32I2M(xr7,A);
      S32I2M(xr8,B);
      S32I2M(xr3,C);
      D16MAX(xr4,xr7,xr8);//xr4=max of (A,B)
      D16MIN(xr5,xr7,xr8);//xr5=min of (A,B)
      D16MIN(xr4,xr4,xr3);//xr4:comp max(A,B) and C, get rid of the MAX of (A,B,C)
      D16MAX(xr4,xr4,xr5);//xr4:get rid of the MIN of (A,B,C)
      return S32M2I(xr4);
    }
  }
}

static av_always_inline
uint32_t pred_pskip_motion_cavlc(H264Context *h){
  const int top_ref = h->ref_cache[0][ 12 - 8 ];
  const int left_ref= h->ref_cache[0][ 12 - 1 ];
  if(top_ref == PART_NOT_AVAILABLE || left_ref == PART_NOT_AVAILABLE
     || (top_ref == 0  && *(uint32_t*)h->mv_cache[0][ 12 - 8 ] == 0)
     || (left_ref == 0 && *(uint32_t*)h->mv_cache[0][ 12 - 1 ] == 0)){
    return 0;
  }
  return pred_motion_cavlc(h, 0, 4, 0, 0);
}

static av_always_inline
uint32_t pred_8x16_motion_cavlc(H264Context *h, int n, int list, int ref){
  if(n==0){
    const int left_ref=      h->ref_cache[list][ 12 - 1 ];
    const uint32_t * const A=  h->mv_cache[list][ 12 - 1 ];
    if(left_ref == ref){
      return A[0];
    }
  }else{
    uint32_t C;
    int diagonal_ref;
    diagonal_ref= fetch_diagonal_mv_cavlc(h, &C, 14, list, 2);
    if(diagonal_ref == ref){
      return C;
    }
  }
  return pred_motion_cavlc(h, n, 2, list, ref);
}

static av_always_inline
uint32_t pred_16x8_motion_cavlc(H264Context *h, int n, int list, int ref){
  if(n==0){
    const int top_ref=      h->ref_cache[list][ 12 - 8 ];
    const uint32_t * const B= h->mv_cache[list][ 12 - 8 ];
    if(top_ref == ref){
      return B[0];
    }
  }else{
    const int left_ref=     h->ref_cache[list][ 28 - 1 ];
    const uint32_t * const A= h->mv_cache[list][ 28 - 1 ];
    if(left_ref == ref){
      return A[0];
    }
  }

  return pred_motion_cavlc(h, n, 4, list, ref);
}

static av_always_inline int colZeroFlag_vlc(int * col_mv){
  S32LDD(xr7,col_mv,0);
  D16CPS(xr3,xr7,xr7);//xr7:abs_mvcol
  D16SLT(xr4,xr3,xr15);//FFABS(mvcol) < 2
  S32SFL(xr8,xr0,xr4,xr0,3);//xr8=xr7>>16
  S32AND(xr5,xr4,xr8);
  return S32M2I(xr5);
}

void pred_direct_motion_cavlc_L3dot0_below(H264Context * const h, int *mb_type){
  S32I2M(xr15,0x20002);
  const int mb_xy =   h->mb_xy;//s->mb_x +   s->mb_y*s->mb_stride;
  const int b8_xy = h->mb2b8_xy[mb_xy];//2*s->mb_x + 2*s->mb_y*h->b8_stride;
  const int b4_xy = h->mb2b_xy[mb_xy];//4*s->mb_x + 4*s->mb_y*h->b_stride;
  const int mb_type_col = h->ref_list[1][0].mb_type[mb_xy];
  const int16_t (*l1mv0)[2] = (const int16_t (*)[2]) &h->ref_list[1][0].motion_val[0][b4_xy];
  const int16_t (*l1mv1)[2] = (const int16_t (*)[2]) &h->ref_list[1][0].motion_val[1][b4_xy];
  const int8_t *l1ref0 = &h->ref_list[1][0].ref_index[0][b8_xy];
  const int8_t *l1ref1 = &h->ref_list[1][0].ref_index[1][b8_xy];
  const int is_b8x8 = IS_8X8(*mb_type);
  unsigned int sub_mb_type;
  int i8, i4;

#define MB_TYPE_16x16_OR_INTRA (MB_TYPE_16x16|MB_TYPE_INTRA4x4|MB_TYPE_INTRA16x16|MB_TYPE_INTRA_PCM)
  if(IS_8X8(mb_type_col) && !h->sps.direct_8x8_inference_flag){
    /* FIXME save sub mb types from previous frames (or derive from MVs)
     * so we know exactly what block size to use */
    sub_mb_type = MB_TYPE_8x8|MB_TYPE_P0L0|MB_TYPE_P0L1|MB_TYPE_DIRECT2; /* B_SUB_4x4 */
    *mb_type =    MB_TYPE_8x8|MB_TYPE_L0L1;
  }else if((!is_b8x8)&&(mb_type_col & MB_TYPE_16x16_OR_INTRA)){
    sub_mb_type = MB_TYPE_16x16|MB_TYPE_P0L0|MB_TYPE_P0L1|MB_TYPE_DIRECT2; /* B_SUB_8x8 */
    *mb_type =    MB_TYPE_16x16|MB_TYPE_P0L0|MB_TYPE_P0L1|MB_TYPE_DIRECT2; /* B_16x16 */
  }else{
    sub_mb_type = MB_TYPE_16x16|MB_TYPE_P0L0|MB_TYPE_P0L1|MB_TYPE_DIRECT2; /* B_SUB_8x8 */
    *mb_type =    MB_TYPE_8x8|MB_TYPE_L0L1;
  }
  if(!is_b8x8){
    ((short*)h->direct_slim_cache)[3]=((short*)h->direct_slim_cache)[5]=0x101;
    *mb_type |= MB_TYPE_DIRECT2;
  }
  if(MB_FIELD)
    *mb_type |= MB_TYPE_INTERLACED;

  int is_b16x16=IS_16X16(*mb_type);
  int is_col_intra=IS_INTRA(mb_type_col);
  if(h->direct_spatial_mv_pred){
    int ref[2];
    int32_t mv[2];
    int list;

    /* ref = min(neighbors) */
    for(list=0; list<2; list++){
      int refa = h->ref_cache[list][12 - 1];
      int refb = h->ref_cache[list][12 - 8];
      int refc = h->ref_cache[list][12 - 8 + 4];
      if(refc == -2)
	refc = h->ref_cache[list][12 - 8 - 1];
      ref[list] = refa;
      if(ref[list] < 0 || (refb < ref[list] && refb >= 0))
	ref[list] = refb;
      if(ref[list] < 0 || (refc < ref[list] && refc >= 0))
	ref[list] = refc;
      if(ref[list] < 0)
	ref[list] = -1;
    }

    if(ref[0] < 0 && ref[1] < 0){
      ref[0] = ref[1] = 0;
      mv[0] =  mv[1] = 0;
    }else{
      for(list=0; list<2; list++){
	if(ref[list] >= 0)
	  mv[list]=pred_motion_cavlc(h, 0, 4, list, ref[list]);
	else
	  mv[list]=0;
      }
    }

    if(ref[1] < 0){
      *mb_type &= ~MB_TYPE_P0L1;
      sub_mb_type &= ~MB_TYPE_P0L1;
    }else if(ref[0] < 0){
      *mb_type &= ~MB_TYPE_P0L0;
      sub_mb_type &= ~MB_TYPE_P0L0;
    }

    int x264_build_flag=(h->x264_build>33 || !h->x264_build);
    if(is_b16x16){
      int a=mv[0], b=mv[1];

      fill_rectangle(&h->ref_cache[0][12], 4, 4, 8, (uint8_t)ref[0], 1);
      fill_rectangle(&h->ref_cache[1][12], 4, 4, 8, (uint8_t)ref[1], 1);
      if(!is_col_intra
	 && (   (l1ref0[0] == 0 && colZeroFlag_vlc(l1mv0))
		|| (l1ref0[0]  < 0 && l1ref1[0] == 0 && colZeroFlag_vlc(l1mv1)
		    && x264_build_flag))){
	if(ref[0] <= 0)
	  a= 0;
	if(ref[1] <= 0)
	  b= 0;
      }

      fill_rectangle(&h->mv_cache[0][12], 4, 4, 8, a, 4);
      fill_rectangle(&h->mv_cache[1][12], 4, 4, 8, b, 4);
    }else{
      for(i8=0; i8<4; i8++){
	if(is_b8x8 && !IS_DIRECT(h->sub_mb_type[i8]))
	  continue;
	h->sub_mb_type[i8] = sub_mb_type;
	int ref_ofst=b8_offset[i8];
	int i4_base=i8*4;

	fill_rectangle(&h->mv_cache[0][scan8[i4_base]], 2, 2, 8, mv[0], 4);
	fill_rectangle(&h->mv_cache[1][scan8[i4_base]], 2, 2, 8, mv[1], 4);
	fill_rectangle(&h->ref_cache[0][scan8[i4_base]], 2, 2, 8, (uint8_t)ref[0], 1);
	fill_rectangle(&h->ref_cache[1][scan8[i4_base]], 2, 2, 8, (uint8_t)ref[1], 1);

	/* col_zero_flag */
	if(!is_col_intra && (   l1ref0[ref_ofst] == 0
				|| (l1ref0[ref_ofst] < 0 && l1ref1[ref_ofst] == 0
				    && x264_build_flag))){
	  const int16_t (*l1mv)[2]= l1ref0[ref_ofst] == 0 ? l1mv0 : l1mv1;
	  if(IS_SUB_8X8(sub_mb_type)){
	    const int16_t *mv_col = l1mv[b8x3_offset[i8]];
	    if(colZeroFlag_vlc(mv_col)){
	      if(ref[0] == 0)
		fill_rectangle(&h->mv_cache[0][scan8[i4_base]], 2, 2, 8, 0, 4);
	      if(ref[1] == 0)
		fill_rectangle(&h->mv_cache[1][scan8[i4_base]], 2, 2, 8, 0, 4);
	    }
	  }else
	    for(i4=0; i4<4; i4++){
	      int blk4=i4_base+i4;
	      const int16_t *mv_col = l1mv[b4_offset[blk4]];
	      if(colZeroFlag_vlc(mv_col)){
		if(ref[0] == 0)
		  *(uint32_t*)h->mv_cache[0][scan8[blk4]] = 0;
		if(ref[1] == 0)
		  *(uint32_t*)h->mv_cache[1][scan8[blk4]] = 0;
	      }
	    }
	}
      }
    }
  }else{ /* direct temporal mv pred */
    const int *map_col_to_list0[2] = {h->map_col_to_list0[0], h->map_col_to_list0[1]};
    const int *dist_scale_factor = h->dist_scale_factor;

    /* one-to-one mv scaling */

    if(is_b16x16){
      int ref, mv0, mv1;

      fill_rectangle(&h->ref_cache[1][12], 4, 4, 8, 0, 1);
      if(is_col_intra){
	ref=mv0=mv1=0;
      }else{
	const int ref0 = l1ref0[0] >= 0 ? map_col_to_list0[0][l1ref0[0]]
	  : map_col_to_list0[1][l1ref1[0]];
	S32LDDV(xr7,dist_scale_factor,ref0,2);
	const int16_t *mv_col = l1ref0[0] >= 0 ? l1mv0[0] : l1mv1[0];
	S32LDD(xr8,mv_col,0);//xr8=mv_col
	Q16SLL(xr3,xr8,xr0,xr0,3);//mv <<3, scale<<4, d16_mulf <<1,overall shift 8bit
	D16MULF_LW(xr4,xr7,xr3);//xr4=mvl0
	ref= ref0;
	Q16ADD_SS_WW(xr5,xr4,xr8,xr0);//xr5=mvl0-mv_col
	mv0=S32M2I(xr4);
	mv1=S32M2I(xr5);
      }
      fill_rectangle(&h->ref_cache[0][12], 4, 4, 8, ref, 1);
      int* mv_cc0=h->mv_cache[0][12];
      int* mv_cc1=h->mv_cache[1][12];
      int hh,ww;
      for(hh=0;hh<4;hh++){
	for(ww=0;ww<4;ww++){
	  *mv_cc0++=mv0;
	  *mv_cc1++=mv1;
	}
	mv_cc0+=4;
	mv_cc1+=4;
      }
    }else{
      for(i8=0; i8<4; i8++){
	int ref0;
	const int16_t (*l1mv)[2]= l1mv0;

	if(is_b8x8 && !IS_DIRECT(h->sub_mb_type[i8]))
	  continue;
	int i4_base=i8*4;
	h->sub_mb_type[i8] = sub_mb_type;
	fill_rectangle(&h->ref_cache[1][scan8[i4_base]], 2, 2, 8, 0, 1);
	if(is_col_intra){
	  fill_rectangle(&h->ref_cache[0][scan8[i4_base]], 2, 2, 8, 0, 1);
	  fill_rectangle(&h-> mv_cache[0][scan8[i4_base]], 2, 2, 8, 0, 4);
	  fill_rectangle(&h-> mv_cache[1][scan8[i4_base]], 2, 2, 8, 0, 4);
	  continue;
	}

	int ref_ofst=b8_offset[i8];
	ref0 = l1ref0[ref_ofst];
	if(ref0 >= 0)
	  ref0 = map_col_to_list0[0][ref0];
	else{
	  ref0 = map_col_to_list0[1][l1ref1[ref_ofst]];
	  l1mv= l1mv1;
	}
	S32LDDV(xr7,dist_scale_factor,ref0,2);

	fill_rectangle(&h->ref_cache[0][scan8[i4_base]], 2, 2, 8, ref0, 1);
	if(IS_SUB_8X8(sub_mb_type)){
	  const int16_t *mv_col = l1mv[b8x3_offset[i8]];
	  S32LDD(xr8,mv_col,0);//xr8=mv_col
	  Q16SLL(xr3,xr8,xr0,xr0,3);//mv <<3, scale<<4, d16_mulf <<1,overall shift 8bit
	  D16MULF_LW(xr4,xr7,xr3);//xr4=mvl0
	  Q16ADD_SS_WW(xr5,xr4,xr8,xr0);//xr5=mvl0-mv_col
	  int mv0=S32M2I(xr4);
	  int mv1=S32M2I(xr5);
	  fill_rectangle(&h->mv_cache[0][scan8[i4_base]], 2, 2, 8, mv0, 4);
	  fill_rectangle(&h->mv_cache[1][scan8[i4_base]], 2, 2, 8, mv1, 4);
	}else
	  for(i4=0; i4<4; i4++){
	    int blk4=i4_base+i4;
	    const int16_t *mv_col = l1mv[b4_offset[blk4]];
	    S32LDD(xr8,mv_col,0);//xr8=mv_col
	    Q16SLL(xr3,xr8,xr0,xr0,3);//mv <<3, scale<<4, d16_mulf <<1,overall shift 8bit
	    D16MULF_LW(xr4,xr7,xr3);//xr4=mvl0
	    Q16ADD_SS_WW(xr5,xr4,xr8,xr0);//xr5=mvl0-mv_col
	    *(uint32_t*)h->mv_cache[0][scan8[blk4]] = S32M2I(xr4);
	    *(uint32_t*)h->mv_cache[1][scan8[blk4]] = S32M2I(xr5);
	  }
      }
    }
  }
}

__cache_text1__
void pred_direct_motion_cavlc(H264Context * const h, int *mb_type){
  if(h->sps.direct_8x8_inference_flag){
    S32I2M(xr15,0x20002);
    const int mb_xy =   h->mb_xy;//s->mb_x +   s->mb_y*s->mb_stride;
    const int mb_type_col = h->ref_list[1][0].mb_type[mb_xy];
    const int * l1mv0_hl=h->ref_list[1][0].motion_3dot0_above[mb_xy][0];
    const int * l1mv1_hl=h->ref_list[1][0].motion_3dot0_above[mb_xy][1];
    const int8_t *l1ref0_hl = h->ref_list[1][0].ref_3dot0_above[mb_xy][0];
    const int8_t *l1ref1_hl = h->ref_list[1][0].ref_3dot0_above[mb_xy][1];

    const int is_b8x8 = IS_8X8(*mb_type);
    unsigned int sub_mb_type;
    int i8;

#define MB_TYPE_16x16_OR_INTRA (MB_TYPE_16x16|MB_TYPE_INTRA4x4|MB_TYPE_INTRA16x16|MB_TYPE_INTRA_PCM)
    if((!is_b8x8)&&(mb_type_col & MB_TYPE_16x16_OR_INTRA)){
      sub_mb_type = MB_TYPE_16x16|MB_TYPE_P0L0|MB_TYPE_P0L1|MB_TYPE_DIRECT2; /* B_SUB_8x8 */
      *mb_type =    MB_TYPE_16x16|MB_TYPE_P0L0|MB_TYPE_P0L1|MB_TYPE_DIRECT2; /* B_16x16 */
    }else{
      sub_mb_type = MB_TYPE_16x16|MB_TYPE_P0L0|MB_TYPE_P0L1|MB_TYPE_DIRECT2; /* B_SUB_8x8 */
      *mb_type =    MB_TYPE_8x8|MB_TYPE_L0L1;
    }
    if(!is_b8x8){
      ((short*)h->direct_slim_cache)[3]=((short*)h->direct_slim_cache)[5]=0x101;
      *mb_type |= MB_TYPE_DIRECT2;
    }
    if(MB_FIELD)
      *mb_type |= MB_TYPE_INTERLACED;

    int is_b16x16=IS_16X16(*mb_type);
    int is_col_intra=IS_INTRA(mb_type_col);
    if(h->direct_spatial_mv_pred){
      int ref[2];
      int32_t mv[2];
      int list;

      /* ref = min(neighbors) */
      for(list=0; list<2; list++){
	int refa = h->ref_cache[list][12 - 1];
	int refb = h->ref_cache[list][12 - 8];
	int refc = h->ref_cache[list][12 - 8 + 4];
	if(refc == -2)
	  refc = h->ref_cache[list][12 - 8 - 1];
	ref[list] = refa;
	if(ref[list] < 0 || (refb < ref[list] && refb >= 0))
	  ref[list] = refb;
	if(ref[list] < 0 || (refc < ref[list] && refc >= 0))
	  ref[list] = refc;
	if(ref[list] < 0)
	  ref[list] = -1;
      }

      if(ref[0] < 0 && ref[1] < 0){
	ref[0] = ref[1] = 0;
	mv[0] =  mv[1] = 0;
      }else{
	for(list=0; list<2; list++){
	  if(ref[list] >= 0)
	    mv[list]=pred_motion_cavlc(h, 0, 4, list, ref[list]);
	  else
	    mv[list]=0;
	}
      }

      if(ref[1] < 0){
	*mb_type &= ~MB_TYPE_P0L1;
	sub_mb_type &= ~MB_TYPE_P0L1;
      }else if(ref[0] < 0){
	*mb_type &= ~MB_TYPE_P0L0;
	sub_mb_type &= ~MB_TYPE_P0L0;
      }

      int x264_build_flag=(h->x264_build>33 || !h->x264_build);
      if(is_b16x16){
	int a=mv[0], b=mv[1];

	fill_rectangle(&h->ref_cache[0][12], 4, 4, 8, (uint8_t)ref[0], 1);
	fill_rectangle(&h->ref_cache[1][12], 4, 4, 8, (uint8_t)ref[1], 1);

	if(!is_col_intra && 
	   ((l1ref0_hl[0] == 0 && colZeroFlag_vlc(l1mv0_hl))
	    || (l1ref0_hl[0]  < 0 && l1ref1_hl[0] == 0 && colZeroFlag_vlc(l1mv1_hl) && x264_build_flag))){
	  if(ref[0] <= 0)
	    a= 0;
	  if(ref[1] <= 0)
	    b= 0;
	}

	fill_rectangle(&h->mv_cache[0][12], 4, 4, 8, a, 4);
	fill_rectangle(&h->mv_cache[1][12], 4, 4, 8, b, 4);
      }else{
	for(i8=0; i8<4; i8++){
	  if(is_b8x8 && !IS_DIRECT(h->sub_mb_type[i8]))
	    continue;
	  int a=mv[0], b=mv[1];
	  h->sub_mb_type[i8] = sub_mb_type;
	  int i4_base=i8*4;
	  fill_rectangle(&h->ref_cache[0][scan8[i4_base]], 2, 2, 8, (uint8_t)ref[0], 1);
	  fill_rectangle(&h->ref_cache[1][scan8[i4_base]], 2, 2, 8, (uint8_t)ref[1], 1);

	  /* col_zero_flag */
	  if(!is_col_intra && (l1ref0_hl[i8] == 0 || 
			       (l1ref0_hl[i8] < 0 && l1ref1_hl[i8] == 0 && x264_build_flag))){
	    int *l1mv_hl= l1ref0_hl[i8] == 0 ? l1mv0_hl : l1mv1_hl;
	    l1mv_hl+=i8;
	    if(colZeroFlag_vlc(l1mv_hl)){
	      if(ref[0] == 0)
		a=0;
	      if(ref[1] == 0)
		b=0;
	    }
	  }
	  fill_rectangle(&h->mv_cache[0][scan8[i4_base]], 2, 2, 8, a, 4);
	  fill_rectangle(&h->mv_cache[1][scan8[i4_base]], 2, 2, 8, b, 4);
	}
      }
    }else{ /* direct temporal mv pred */
      const int *map_col_to_list0[2] = {h->map_col_to_list0[0], h->map_col_to_list0[1]};
      const int *dist_scale_factor = h->dist_scale_factor;

      /* one-to-one mv scaling */

      if(is_b16x16){
	int ref, mv0, mv1;

	fill_rectangle(&h->ref_cache[1][12], 4, 4, 8, 0, 1);
	if(is_col_intra){
	  ref=mv0=mv1=0;
	}else{
	  const int ref0 = l1ref0_hl[0] >= 0 ? map_col_to_list0[0][l1ref0_hl[0]]
	    : map_col_to_list0[1][l1ref1_hl[0]];

	  S32LDDV(xr7,dist_scale_factor,ref0,2);
	  const int32_t *mv_col = l1ref0_hl[0] >= 0 ? l1mv0_hl : l1mv1_hl;
	  S32LDD(xr8,mv_col,0);//xr8=mv_col
	  Q16SLL(xr3,xr8,xr0,xr0,3);//mv <<3, scale<<4, d16_mulf <<1,overall shift 8bit
	  D16MULF_LW(xr4,xr7,xr3);//xr4=mvl0
	  ref= ref0;
	  Q16ADD_SS_WW(xr5,xr4,xr8,xr0);//xr5=mvl0-mv_col
	  mv0=S32M2I(xr4);
	  mv1=S32M2I(xr5);
	}
	fill_rectangle(&h->ref_cache[0][12], 4, 4, 8, ref, 1);
	int* mv_cc0=h->mv_cache[0][12];
	int* mv_cc1=h->mv_cache[1][12];
	int hh,ww;
	for(hh=0;hh<4;hh++){
	  for(ww=0;ww<4;ww++){
	    *mv_cc0++=mv0;
	    *mv_cc1++=mv1;
	  }
	  mv_cc0+=4;
	  mv_cc1+=4;
	}
      }else{
	for(i8=0; i8<4; i8++){
	  int ref0;
	  if(is_b8x8 && !IS_DIRECT(h->sub_mb_type[i8]))
	    continue;
	  int i4_base=i8*4;
	  h->sub_mb_type[i8] = sub_mb_type;
	  fill_rectangle(&h->ref_cache[1][scan8[i4_base]], 2, 2, 8, 0, 1);
	  if(is_col_intra){
	    fill_rectangle(&h->ref_cache[0][scan8[i4_base]], 2, 2, 8, 0, 1);
	    fill_rectangle(&h-> mv_cache[0][scan8[i4_base]], 2, 2, 8, 0, 4);
	    fill_rectangle(&h-> mv_cache[1][scan8[i4_base]], 2, 2, 8, 0, 4);
	    continue;
	  }
	    
	  int32_t *l1mv;
	  ref0 = l1ref0_hl[i8];
	  if(ref0 >= 0){
	    ref0 = map_col_to_list0[0][ref0];
	    l1mv= l1mv0_hl;
	  }
	  else{
	    ref0 = map_col_to_list0[1][l1ref1_hl[i8]];
	    l1mv= l1mv1_hl;
	  }
	  S32LDDV(xr7,dist_scale_factor,ref0,2);
	    
	  fill_rectangle(&h->ref_cache[0][scan8[i4_base]], 2, 2, 8, ref0, 1);
	  const int32_t *mv_col = l1mv+i8;
	  S32LDD(xr8,mv_col,0);//xr8=mv_col
	  Q16SLL(xr3,xr8,xr0,xr0,3);//mv <<3, scale<<4, d16_mulf <<1,overall shift 8bit
	  D16MULF_LW(xr4,xr7,xr3);//xr4=mvl0
	  Q16ADD_SS_WW(xr5,xr4,xr8,xr0);//xr5=mvl0-mv_col
	  int mv0=S32M2I(xr4);
	  int mv1=S32M2I(xr5);
	  fill_rectangle(&h->mv_cache[0][scan8[i4_base]], 2, 2, 8, mv0, 4);
	  fill_rectangle(&h->mv_cache[1][scan8[i4_base]], 2, 2, 8, mv1, 4);
	}
      }
    }
  }
  else{
    pred_direct_motion_cavlc_L3dot0_below(h,mb_type);
  }
}

void write_back_motion_cavlc_L3dot0_below(H264Context *h, int mb_type){
  MpegEncContext * const s = &h->s;
  int mb_x=s->mb_x;
  const int b_xy = 4*mb_x + 4*s->mb_y*h->b_stride;
  const int b8_xy= 2*mb_x + 2*s->mb_y*h->b8_stride;
  int list;

  if(!USES_LIST(mb_type, 0))
    fill_rectangle(&s->current_picture.ref_index[0][b8_xy], 2, 2, h->b8_stride, (uint8_t)LIST_NOT_USED, 1);

  int (*mv_ha)[4]=mv_hat_atm[mb_x];
  for(list=0; list<h->list_count; list++){
    int y;
    if(!USES_LIST(mb_type, list))
      continue;

    for(y=0; y<4; y++){
      *(uint64_t*)s->current_picture.motion_val[list][b_xy + 0 + y*h->b_stride]=*(uint64_t*)h->mv_cache[list][12+0 + 8*y];
      *(uint64_t*)s->current_picture.motion_val[list][b_xy + 2 + y*h->b_stride]= *(uint64_t*)h->mv_cache[list][12+2 + 8*y];
    }
    mv_ha[list][0]=*(uint32_t*)h->mv_cache[list][12+0 + 8*3];
    mv_ha[list][1]=*(uint32_t*)h->mv_cache[list][12+1 + 8*3];
    mv_ha[list][2]=*(uint32_t*)h->mv_cache[list][12+2 + 8*3];
    mv_ha[list][3]=*(uint32_t*)h->mv_cache[list][12+3 + 8*3];

    {
      int8_t *ref_index = &s->current_picture.ref_index[list][b8_xy];
      ref_index[0+0*h->b8_stride]= h->ref_cache[list][12];
      ref_index[1+0*h->b8_stride]= h->ref_cache[list][14];
      ref_index[0+1*h->b8_stride]= h->ref_cache[list][28];
      ref_index[1+1*h->b8_stride]= h->ref_cache[list][30];
      ref_hat_atm[mb_x][list]=((int*)(h->ref_cache[list]))[9];
    }
  }

}

__cache_text1__
void write_back_motion_cavlc(H264Context *h, int mb_type){
  if(h->sps.direct_8x8_inference_flag){
    MpegEncContext * const s = &h->s;
    int mb_x=s->mb_x;
    const int mb_xy=h->mb_xy;
    int list;

    int (*mv_ha)[4]=mv_hat_atm[mb_x];
    int (*mv_hl_index)[4]=s->current_picture.motion_3dot0_above[mb_xy];
    int8_t (*ref_hl_index)[4] = s->current_picture.ref_3dot0_above[mb_xy];

    if(!USES_LIST(mb_type, 0))
      *(int*)ref_hl_index[0]=((uint8_t)LIST_NOT_USED)*0x01010101;

    for(list=0; list<h->list_count; list++){
      //int y;
      if(!USES_LIST(mb_type, list))
	continue;

      mv_hl_index[list][0]=*(uint32_t*)h->mv_cache[list][12];
      mv_hl_index[list][1]=*(uint32_t*)h->mv_cache[list][15];
      mv_hl_index[list][2]=*(uint32_t*)h->mv_cache[list][36];
      mv_hl_index[list][3]=*(uint32_t*)h->mv_cache[list][39];

      mv_ha[list][0]=*(uint32_t*)h->mv_cache[list][12+0 + 8*3];
      mv_ha[list][1]=*(uint32_t*)h->mv_cache[list][12+1 + 8*3];
      mv_ha[list][2]=*(uint32_t*)h->mv_cache[list][12+2 + 8*3];
      mv_ha[list][3]=*(uint32_t*)h->mv_cache[list][12+3 + 8*3];

      ref_hl_index[list][0]= h->ref_cache[list][12];
      ref_hl_index[list][1]= h->ref_cache[list][14];
      ref_hl_index[list][2]= h->ref_cache[list][28];
      ref_hl_index[list][3]= h->ref_cache[list][30];
      ref_hat_atm[mb_x][list]=((int*)(h->ref_cache[list]))[9];
    }

  }
  else{
    write_back_motion_cavlc_L3dot0_below(h, mb_type);
  }
}


__cache_text1__
void fill_caches_cavlc(H264Context *h, int mb_type){
  MpegEncContext * const s = &h->s;
  const int mb_xy= h->mb_xy;
  int topleft_xy, top_xy, topright_xy, left_xy;
  int i;

  top_xy  = mb_xy - (s->mb_stride << FIELD_PICTURE);
  left_xy = mb_xy - 1;
  topleft_xy = top_xy - 1;
  topright_xy= top_xy + 1;

  int slice_num = h->slice_num;
  uint8_t *p_slice_table = h->slice_table;
  uint32_t *p_mb_type = s->current_picture.mb_type;
  int topleft_type = p_slice_table[topleft_xy ] == slice_num ? p_mb_type[topleft_xy ] : 0;
  int top_type     = p_slice_table[top_xy     ] == slice_num ? p_mb_type[top_xy     ] : 0;
  int topright_type= p_slice_table[topright_xy] == slice_num ? p_mb_type[topright_xy] : 0;
  int left_type    = p_slice_table[left_xy    ] == slice_num ? p_mb_type[left_xy    ] : 0;

  if(IS_INTRA(mb_type)){
    int constrained_intra_pred = h->pps.constrained_intra_pred;
    h->top_samples_available= h->left_samples_available= 0xFFFF;
    h->topleft_samples_available = 0xf0;
    h->topright_samples_available= 0x5777;

    if(!IS_INTRA(top_type) && (top_type==0 || constrained_intra_pred)){
      h->topleft_samples_available = 0xD0;
      h->top_samples_available= 0x33FF;
      h->topright_samples_available= 0x5764;
    }

    if(!IS_INTRA(left_type) && (left_type==0 || constrained_intra_pred)){
      h->topleft_samples_available&= 0xB0;
      h->left_samples_available&= 0x5F5F;
    }

    if(!IS_INTRA(topleft_type) && (topleft_type==0 || constrained_intra_pred))
      h->topleft_samples_available&= 0xE0;

    if(!IS_INTRA(topright_type) && (topright_type==0 || constrained_intra_pred))
      h->topright_samples_available&= 0xFFDF;

    int8_t * h_intra4x4_pred_mode_cache = &h->intra4x4_pred_mode_cache[0];
    if(IS_INTRA4x4(top_type)){
#if 0
      int8_t * h_intra4x4_pred_mode_top = &h->intra4x4_pred_mode[top_xy][0];
      h_intra4x4_pred_mode_cache[4+8*0]= h_intra4x4_pred_mode_top[4];
      h_intra4x4_pred_mode_cache[5+8*0]= h_intra4x4_pred_mode_top[5];
      h_intra4x4_pred_mode_cache[6+8*0]= h_intra4x4_pred_mode_top[6];
      h_intra4x4_pred_mode_cache[7+8*0]= h_intra4x4_pred_mode_top[3];
#else
      *(uint32_t *)&h_intra4x4_pred_mode_cache[4+8*0] = *(uint32_t *)&h->intra4x4_pred_mode[top_xy][4];
#endif
    }else{
      //int pred;
      if(!top_type || (IS_INTER(top_type) && constrained_intra_pred))
	//pred= -1;
        *(uint32_t *)&h_intra4x4_pred_mode_cache[4+8*0] = 0x01010101 * -1;
      else{
	//pred= 2;
	*(uint32_t *)&h_intra4x4_pred_mode_cache[4+8*0] = 0x01010101 * 2;
      }
      //h_intra4x4_pred_mode_cache[4+8*0]=
      //h_intra4x4_pred_mode_cache[5+8*0]=
      //h_intra4x4_pred_mode_cache[6+8*0]=
      //h_intra4x4_pred_mode_cache[7+8*0]= pred;
    }

    if(IS_INTRA4x4(left_type)){
      int8_t * h_intra4x4_pred_mode_left = &h->intra4x4_pred_mode[left_xy][0];
      h_intra4x4_pred_mode_cache[3+8*1 + 2*8*0]= h_intra4x4_pred_mode_left[0];
      h_intra4x4_pred_mode_cache[3+8*2 + 2*8*0]= h_intra4x4_pred_mode_left[1];
      h_intra4x4_pred_mode_cache[3+8*1 + 2*8*1]= h_intra4x4_pred_mode_left[2];
      h_intra4x4_pred_mode_cache[3+8*2 + 2*8*1]= h_intra4x4_pred_mode_left[3];
    }else{
      int pred;
      if(!left_type || (IS_INTER(left_type) && constrained_intra_pred))
	pred= -1;
      else{
	pred= 2;
      }
      h_intra4x4_pred_mode_cache[3+8*1 + 2*8*0]=
	h_intra4x4_pred_mode_cache[3+8*2 + 2*8*0]=
	h_intra4x4_pred_mode_cache[3+8*1 + 2*8*1]=
	h_intra4x4_pred_mode_cache[3+8*2 + 2*8*1]= pred;
    }

  }

  uint8_t * h_non_zero_count_cache = &h->non_zero_count_cache[0];
  if(top_type){
    uint8_t * h_non_zero_count_topxy = &h->non_zero_count[top_xy][0];
    *(uint32_t *)&h_non_zero_count_cache[4+8*0]= *(uint32_t *)&h_non_zero_count_topxy[0];

    h_non_zero_count_cache[1+8*0]= h_non_zero_count_topxy[4];
    h_non_zero_count_cache[2+8*0]= h_non_zero_count_topxy[5];
    h_non_zero_count_cache[1+8*3]= h_non_zero_count_topxy[6];
    h_non_zero_count_cache[2+8*3]= h_non_zero_count_topxy[7];    
  }else{
    *(uint32_t *)&h_non_zero_count_cache[4+8*0]=
      *(uint32_t *)&h_non_zero_count_cache[0+8*0]=
      *(uint32_t *)&h_non_zero_count_cache[0+8*3]= 64 * 0x01010101;
    /* fill_nnz_left must be after fill_nnz_top, because here chroma top fills 4 bytes and covers left*/
  }

  if(left_type){
    h_non_zero_count_cache[3 + 8*1 + 8*0]=h_non_zero_count_cache[7 + 8*1 + 8*0];
    h_non_zero_count_cache[3 + 8*1 + 8*1]=h_non_zero_count_cache[7 + 8*1 + 8*1];
    h_non_zero_count_cache[3 + 8*1 + 8*2]=h_non_zero_count_cache[7 + 8*1 + 8*2];
    h_non_zero_count_cache[3 + 8*1 + 8*3]=h_non_zero_count_cache[7 + 8*1 + 8*3];
    h_non_zero_count_cache[0 + 8*1 + 8*0]=h_non_zero_count_cache[2 + 8*1 + 8*0];
    h_non_zero_count_cache[0 + 8*1 + 8*1]=h_non_zero_count_cache[2 + 8*1 + 8*1];
    h_non_zero_count_cache[0 + 8*4 + 8*0]=h_non_zero_count_cache[2 + 8*4 + 8*0];
    h_non_zero_count_cache[0 + 8*4 + 8*1]=h_non_zero_count_cache[2 + 8*4 + 8*1];
  }else{
    h_non_zero_count_cache[3 + 8*1 + 8*0]=
    h_non_zero_count_cache[3 + 8*1 + 8*1]=
    h_non_zero_count_cache[3 + 8*1 + 8*2]=
    h_non_zero_count_cache[3 + 8*1 + 8*3]=
    h_non_zero_count_cache[0 + 8*1 + 8*0]=
    h_non_zero_count_cache[0 + 8*1 + 8*1]=
    h_non_zero_count_cache[0 + 8*4 + 8*0]=
    h_non_zero_count_cache[0 + 8*4 + 8*1]= 64;
  }

  if(IS_INTER(mb_type) || IS_DIRECT(mb_type)) {
    int (*mv_ha)[4]=mv_hat_atm[s->mb_x];
    int *ref_ha=ref_hat_atm[s->mb_x];
    for(i=0;i<h->slice_type-1;i++){

      /* */
      //if(!USES_LIST(mb_type,i) && !IS_DIRECT(mb_type) && !h->deblocking_filter) continue;

      uint32_t * h_mv_cache = &h->mv_cache[i][0][0];
      //uint32_t * s_motion_val = &s->current_picture.motion_val[i][0][0];
      int8_t * h_ref_cache = &h->ref_cache[i][0];
      //int8_t * s_ref_index = &s->current_picture.ref_index[i][0];

      if(USES_LIST(top_type, i)){
	h_mv_cache[4]= mv_ha[i][0];
	h_mv_cache[5]= mv_ha[i][1];
	h_mv_cache[6]= mv_ha[i][2];
	h_mv_cache[7]= mv_ha[i][3];
	((uint32_t*)h_ref_cache)[1]=ref_ha[i];
      }else{
	h_mv_cache[4]= h_mv_cache[5]= h_mv_cache[6]= h_mv_cache[7]= 0;
	*(uint32_t*)&h_ref_cache[4]=
	  ((top_type ? LIST_NOT_USED : PART_NOT_AVAILABLE)&0xFF)*0x01010101;
      }

      if(USES_LIST(left_type, i)){
	h_mv_cache[11]= h_mv_cache[15];
	h_mv_cache[19]= h_mv_cache[23];
	h_mv_cache[27]= h_mv_cache[31];
	h_mv_cache[35]= h_mv_cache[39];

	h_ref_cache[11]= h_ref_cache[15];
	h_ref_cache[19]= h_ref_cache[23];
	h_ref_cache[27]= h_ref_cache[31];
	h_ref_cache[35]= h_ref_cache[39];
      }else{
	h_mv_cache[11]= h_mv_cache[19]= h_mv_cache[27]= h_mv_cache[35]= 0;
	h_ref_cache[11]= h_ref_cache[19]= h_ref_cache[27]= h_ref_cache[35]=
	  left_type ? LIST_NOT_USED : PART_NOT_AVAILABLE;
      }

      if(USES_LIST(topleft_type, i)){
	h_mv_cache[3]=mv_top_left_atm[i];
	h_ref_cache[3]=ref_top_left_atm[i];
      }else{
	h_mv_cache[3]= 0;
	h_ref_cache[3]=topleft_type ? LIST_NOT_USED : PART_NOT_AVAILABLE;
      }

      if(USES_LIST(topright_type, i)){
	h_mv_cache[8]= mv_ha[i][8];
	h_ref_cache[8]= ref_ha[i+2];
      }else{
	h_mv_cache[8]= 0;
	h_ref_cache[8]= topright_type ? LIST_NOT_USED : PART_NOT_AVAILABLE;
      }

      h_ref_cache[16] = h_ref_cache[24] = h_ref_cache[32] =
	h_ref_cache[14] = h_ref_cache[30] = PART_NOT_AVAILABLE;
      h_mv_cache[16]= h_mv_cache[24]= h_mv_cache[32]= h_mv_cache[14]= h_mv_cache[30]= 0;
    }
  }
}

//__cache_text1__
void fill_caches_cavlc_for_dblk(H264Context *h, int mb_type){
  MpegEncContext * const s = &h->s;
  const int mb_xy=h->mb_xy;

  int top_xy, left_xy;
  int top_type, left_type;

  top_xy  = mb_xy - (s->mb_stride << FIELD_PICTURE);
  left_xy = mb_xy - 1;

  top_type  = h->slice_table[top_xy ] < 255 ? s->current_picture.mb_type[top_xy ] : 0;
  left_type = h->slice_table[left_xy] < 255 ? s->current_picture.mb_type[left_xy] : 0;

  uint8_t * h_non_zero_count_cache = &h->non_zero_count_cache[0];
  if(top_type){
    uint8_t * h_non_zero_count_topxy = &h->non_zero_count[top_xy][0];
    *(uint32_t *)&h_non_zero_count_cache[4+8*0]= *(uint32_t *)&h_non_zero_count_topxy[0];
    h_non_zero_count_cache[1+8*0]= h_non_zero_count_topxy[4];
    h_non_zero_count_cache[2+8*0]= h_non_zero_count_topxy[5];
    h_non_zero_count_cache[1+8*3]= h_non_zero_count_topxy[6];
    h_non_zero_count_cache[2+8*3]= h_non_zero_count_topxy[7];
  }else{
    *(uint32_t *)&h_non_zero_count_cache[4+8*0]=
      *(uint32_t *)&h_non_zero_count_cache[0+8*0]=
      *(uint32_t *)&h_non_zero_count_cache[0+8*3]= 64 * 0x01010101;
    /* fill_nnz_left must be after fill_nnz_top, because here chroma top fills 4 bytes and covers left*/
  }

  if(left_type){
    h_non_zero_count_cache[3 + 8*1 + 8*0]=nnz_left_bak[0];
    h_non_zero_count_cache[3 + 8*1 + 8*1]=nnz_left_bak[1];
    h_non_zero_count_cache[3 + 8*1 + 8*2]=nnz_left_bak[2];
    h_non_zero_count_cache[3 + 8*1 + 8*3]=nnz_left_bak[3];
    h_non_zero_count_cache[0 + 8*1 + 8*0]=nnz_left_bak[4];
    h_non_zero_count_cache[0 + 8*1 + 8*1]=nnz_left_bak[5];
    h_non_zero_count_cache[0 + 8*4 + 8*0]=nnz_left_bak[6];
    h_non_zero_count_cache[0 + 8*4 + 8*1]=nnz_left_bak[7];
  }else{
    h_non_zero_count_cache[3 + 8*1 + 8*0]=
      h_non_zero_count_cache[3 + 8*1 + 8*1]=
      h_non_zero_count_cache[3 + 8*1 + 8*2]=
      h_non_zero_count_cache[3 + 8*1 + 8*3]=
      h_non_zero_count_cache[0 + 8*1 + 8*0]=
      h_non_zero_count_cache[0 + 8*1 + 8*1]=
      h_non_zero_count_cache[0 + 8*4 + 8*0]=
      h_non_zero_count_cache[0 + 8*4 + 8*1]= 64;
  }
}

//__cache_text1__
int decode_mb_skip_cavlc(H264Context *h){
  int mb_type;
  h->cbp = 0;

  mb_type = MB_TYPE_16x16|MB_TYPE_P0L0|MB_TYPE_P1L0|MB_TYPE_SKIP;
  if(MB_FIELD)
    mb_type|= MB_TYPE_INTERLACED;

  fill_caches_cavlc(h, mb_type);

  if( h->slice_type == FF_B_TYPE ) {
    pred_direct_motion_cavlc(h, &mb_type);
    mb_type|= MB_TYPE_SKIP;
  } else {
    int mpxy = pred_pskip_motion_cavlc(h);
    fill_rectangle(&h->ref_cache[0][(4+1*8)], 4, 4, 8, 0, 1);
    fill_rectangle(  h->mv_cache[0][(4+1*8)], 4, 4, 8, mpxy, 4);
  }

  return mb_type;
}

void hl_decode_cavlc_intra_pcm(H264Context * const h,int mb_type) {
  MpegEncContext * const s = &h->s;
  const int mb_xy = s->mb_x +   s->mb_y*s->mb_stride;
  unsigned int x, y;

  GetBitContext *gb= &s->gb;
  VLC_BS_CLOSE(gb);

  // We assume these blocks are very rare so we do not optimize it.
  align_get_bits(&s->gb);

  // The pixels are stored in the same order as levels in h->mb array.
  for(y=0; y<16; y++){
    const int index= 4*(y&3) + 32*((y>>2)&1) + 128*(y>>3);
    for(x=0; x<16; x++){
      h->mb[index + (x&3) + 16*((x>>2)&1) + 64*(x>>3)]= get_bits(&s->gb, 8);
    }
  }
  for(y=0; y<8; y++){
    const int index= 256 + 4*(y&3) + 32*(y>>2);
    for(x=0; x<8; x++){
      h->mb[index + (x&3) + 16*(x>>2)]= get_bits(&s->gb, 8);
    }
  }
  for(y=0; y<8; y++){
    const int index= 256 + 64 + 4*(y&3) + 32*(y>>2);
    for(x=0; x<8; x++){
      h->mb[index + (x&3) + 16*(x>>2)]= get_bits(&s->gb, 8);
    }
  }

  VLC_BS_OPEN(gb);

  // In deblocking, the quantizer is 0
  s->current_picture.qscale_table[mb_xy]= 0;
  h->chroma_qp[0] = get_chroma_qp(h, 0, 0);
  h->chroma_qp[1] = get_chroma_qp(h, 1, 0);
  // All coeffs are present
  memset(h->non_zero_count[mb_xy], 16, 8);
  s->current_picture.mb_type[mb_xy]= mb_type;
}


static av_always_inline
void write_back_non_zero_count_cavlc(H264Context *h){
  //MpegEncContext * const s = &h->s;
    const int mb_xy= h->mb_xy;
    uint8_t * h_non_zero_count_mbxy = &h->non_zero_count[mb_xy][0];
    *(uint32_t *)&h_non_zero_count_mbxy[0]= *(uint32_t *)&h->non_zero_count_cache[4+8*4];
    h_non_zero_count_mbxy[4]= h->non_zero_count_cache[1+8*2];
    h_non_zero_count_mbxy[5]= h->non_zero_count_cache[2+8*2];
    h_non_zero_count_mbxy[6]= h->non_zero_count_cache[1+8*5];
    h_non_zero_count_mbxy[7]= h->non_zero_count_cache[2+8*5];
}

static av_always_inline
int pred_intra_mode_cavlc(H264Context *h, int index8){
  //const int index8= scan8[n];
    const int left= h->intra4x4_pred_mode_cache[index8 - 1];
    const int top = h->intra4x4_pred_mode_cache[index8 - 8];
    const int min= FFMIN(left, top);
    if(min<0) return DC_PRED;
    else      return min;
}

static av_always_inline
void write_back_intra_pred_mode_cavlc(H264Context *h){
  //MpegEncContext * const s = &h->s;
    const int mb_xy= h->mb_xy;
    int8_t * h_intra4x4_pred_mode_mbxy = &h->intra4x4_pred_mode[mb_xy][0];
    h_intra4x4_pred_mode_mbxy[0]= h->intra4x4_pred_mode_cache[7+8*1];
    h_intra4x4_pred_mode_mbxy[1]= h->intra4x4_pred_mode_cache[7+8*2];
    h_intra4x4_pred_mode_mbxy[2]= h->intra4x4_pred_mode_cache[7+8*3];
    h_intra4x4_pred_mode_mbxy[3]= h->intra4x4_pred_mode_cache[7+8*4];
#if 0
    h_intra4x4_pred_mode_mbxy[4]= h->intra4x4_pred_mode_cache[4+8*4];
    h_intra4x4_pred_mode_mbxy[5]= h->intra4x4_pred_mode_cache[5+8*4];
    h_intra4x4_pred_mode_mbxy[6]= h->intra4x4_pred_mode_cache[6+8*4];
#else
    *(uint32_t *)&h_intra4x4_pred_mode_mbxy[4] = *(uint32_t *)&h->intra4x4_pred_mode_cache[4+8*4];
#endif
}

static av_always_inline
int check_intra4x4_pred_mode_cavlc(H264Context *h){
  //MpegEncContext * const s = &h->s;
  static const int8_t top [12]= {-1, 0,LEFT_DC_PRED,-1,-1,-1,-1,-1, 0};
  static const int8_t left[12]= { 0,-1, TOP_DC_PRED, 0,-1,-1,-1, 0,-1,DC_128_PRED};
  int i;
  if(!(h->top_samples_available&0x8000)){
    for(i=0; i<4; i++){
      int status= top[ h->intra4x4_pred_mode_cache[(4+1*8) + i] ];
      if(status<0){
	return -1;
      } else if(status){
	h->intra4x4_pred_mode_cache[(4+1*8) + i]= status;
      }
    }
  }
  if(!(h->left_samples_available&0x8000)){
    for(i=0; i<4; i++){
      int status= left[ h->intra4x4_pred_mode_cache[(4+1*8) + 8*i] ];
      if(status<0){
	return -1;
      } else if(status){
	h->intra4x4_pred_mode_cache[(4+1*8) + 8*i]= status;
      }
    }
  }
  return 0;
}

static av_always_inline
int check_intra_pred_mode_cavlc(H264Context *h, int mode){
  //MpegEncContext * const s = &h->s;
  static const int8_t top [7]= {LEFT_DC_PRED8x8, 1,-1,-1};
  static const int8_t left[7]= { TOP_DC_PRED8x8,-1, 2,-1,DC_128_PRED8x8};
  if(mode > 6U) {
    return -1;
  }
  if(!(h->top_samples_available&0x8000)){
    mode= top[ mode ];
    if(mode<0){
      return -1;
    }
  }
  if(!(h->left_samples_available&0x8000)){
    mode= left[ mode ];
    if(mode<0){
      return -1;
    }
  }
  return mode;
}


__cache_text1__
int decode_mb_cavlc_dcore(H264Context *h){
  MpegEncContext * const s = &h->s;
  const int mb_xy= s->mb_x + s->mb_y*s->mb_stride;
  h->mb_xy = mb_xy;
  int partition_count;
  unsigned int mb_type, cbp = 0;
  int dct8x8_allowed= h->pps.transform_8x8_mode;

#ifdef JZC_PMON_P0ed
    PMON_ON(wait);
#endif

    POLL_FIFO_IS_EMPTY();

#ifdef JZC_PMON_P0ed
    PMON_OFF(wait);
#endif

    dMB_Ctrl = (H264_MB_Ctrl_DecARGs *)task_fifo_wp;
    ctrl_end = task_fifo_wp + ((sizeof(H264_MB_Ctrl_DecARGs) + 3) >> 2);
    task_fifo_wp = ctrl_end;


#ifdef JZC_PMON_P0ed
    PMON_ON(parse);
#endif

  h->slice_table[ mb_xy ]= h->slice_num;

  for_dblk_c = !(h->slice_num == 1 || h->slice_num == h->slice_table[mb_xy-s->mb_stride]);
  if(for_dblk_c){
    dblk_left_mv[0][0]=*(uint32_t*)h->mv_cache[0][15];
    dblk_left_mv[0][1]=*(uint32_t*)h->mv_cache[0][23];
    dblk_left_mv[0][2]=*(uint32_t*)h->mv_cache[0][31];
    dblk_left_mv[0][3]=*(uint32_t*)h->mv_cache[0][39];
    dblk_left_mv[1][0]=*(uint32_t*)h->mv_cache[1][15];
    dblk_left_mv[1][1]=*(uint32_t*)h->mv_cache[1][23];
    dblk_left_mv[1][2]=*(uint32_t*)h->mv_cache[1][31];
    dblk_left_mv[1][3]=*(uint32_t*)h->mv_cache[1][39];

    dblk_left_ref[0][0]=h->ref_cache[0][15];
    dblk_left_ref[0][1]=h->ref_cache[0][23];
    dblk_left_ref[0][2]=h->ref_cache[0][31];
    dblk_left_ref[0][3]=h->ref_cache[0][39];
    dblk_left_ref[1][0]=h->ref_cache[1][15];
    dblk_left_ref[1][1]=h->ref_cache[1][23];
    dblk_left_ref[1][2]=h->ref_cache[1][31];
    dblk_left_ref[1][3]=h->ref_cache[1][39];

    nnz_left_bak[0] = h->non_zero_count_cache[7 + 8*1 + 8*0];
    nnz_left_bak[1] = h->non_zero_count_cache[7 + 8*1 + 8*1];
    nnz_left_bak[2] = h->non_zero_count_cache[7 + 8*1 + 8*2];
    nnz_left_bak[3] = h->non_zero_count_cache[7 + 8*1 + 8*3];
    nnz_left_bak[4] = h->non_zero_count_cache[2 + 8*1 + 8*0];
    nnz_left_bak[5] = h->non_zero_count_cache[2 + 8*1 + 8*1];
    nnz_left_bak[6] = h->non_zero_count_cache[2 + 8*4 + 8*0];
    nnz_left_bak[7] = h->non_zero_count_cache[2 + 8*4 + 8*1];
  }

  //GetBitContext *gb= &s->gb;
  //VLC_BS_OPEN(gb);
  //VLC_BS_CLOSE(gb);

  if(h->slice_type != FF_I_TYPE && h->slice_type != FF_SI_TYPE){
    if(s->mb_skip_run==-1)
      s->mb_skip_run= get_ue_golomb_bs();

    if (s->mb_skip_run--) {
      mb_type = decode_mb_skip_cavlc(h);
      goto skip_mb_cavlc;
    }
  }

  h->mb_field_decoding_flag= (s->picture_structure!=PICT_FRAME);

  mb_type= get_ue_golomb_bs();
  if(h->slice_type == FF_B_TYPE){
    if(mb_type < 23){
      partition_count= b_mb_type_info[mb_type].partition_count;
      mb_type=         b_mb_type_info[mb_type].type;
    }else{
      mb_type -= 23;
      goto decode_intra_mb;
    }
  }else if(h->slice_type == FF_P_TYPE /*|| h->slice_type == SP_TYPE */){
    if(mb_type < 5){
      partition_count= p_mb_type_info[mb_type].partition_count;
      mb_type=         p_mb_type_info[mb_type].type;
    }else{
      mb_type -= 5;
      goto decode_intra_mb;
    }
  }else{
    assert(h->slice_type == FF_I_TYPE);
  decode_intra_mb:
    if(mb_type > 25){
      return -1;
    }
    partition_count=0;
    cbp= i_mb_type_info[mb_type].cbp;
    h->intra16x16_pred_mode= i_mb_type_info[mb_type].pred_mode;
    mb_type= i_mb_type_info[mb_type].type;
  }

  if(MB_FIELD)
    mb_type |= MB_TYPE_INTERLACED;

  if(IS_INTRA_PCM(mb_type)){
    hl_decode_cavlc_intra_pcm(h,mb_type);
    return 0;
  }

  fill_caches_cavlc(h, mb_type);

  if(IS_INTRA(mb_type)){
    int pred_mode;
    if(IS_INTRA4x4(mb_type)){
      int i;
      int di = 1;
      if(dct8x8_allowed && get_bits1_bs()){
	mb_type |= MB_TYPE_8x8DCT;
	di = 4;
      }

      for(i=0; i<16; i+=di){
	const int index8= scan8[i];
	int mode= pred_intra_mode_cavlc(h, index8);

	if(!get_bits1_bs()){
	  const int rem_mode= get_bits_bs(3);
	  mode = rem_mode + (rem_mode >= mode);
	}
	if(di==4)
	  fill_rectangle( &h->intra4x4_pred_mode_cache[ index8 ], 2, 2, 8, mode, 1 );
	else
	  h->intra4x4_pred_mode_cache[ index8 ] = mode;
      }

      write_back_intra_pred_mode_cavlc(h);
      if( check_intra4x4_pred_mode_cavlc(h) < 0)
	return -1;
    }else{
      h->intra16x16_pred_mode= check_intra_pred_mode_cavlc(h, h->intra16x16_pred_mode);
      if(h->intra16x16_pred_mode < 0)
	return -1;
    }

    pred_mode= check_intra_pred_mode_cavlc(h, get_ue_golomb_bs());
    if(pred_mode < 0)
      return -1;
    h->chroma_pred_mode= pred_mode;
  }else if(partition_count==4){
    int i, j, sub_partition_count[4], list, ref[2][4];
    if(h->slice_type == FF_B_TYPE){
      for(i=0; i<4; i++){
	int sub_mb_type_tmp = get_ue_golomb_bs();
	if(sub_mb_type_tmp >=13){
	  return -1;
	}
	sub_partition_count[i]= b_sub_mb_type_info[ sub_mb_type_tmp ].partition_count;
	h->sub_mb_type[i]=      b_sub_mb_type_info[ sub_mb_type_tmp ].type;
      }
      if(IS_DIRECT(h->sub_mb_type[0] | h->sub_mb_type[1]
		   | h->sub_mb_type[2] | h->sub_mb_type[3])) {
	pred_direct_motion_cavlc(h, &mb_type);
	h->ref_cache[0][scan8[4]] =
	  h->ref_cache[1][scan8[4]] =
	  h->ref_cache[0][scan8[12]] =
	  h->ref_cache[1][scan8[12]] = PART_NOT_AVAILABLE;
      }
    }else{
      for(i=0; i<4; i++){
	int sub_mb_type_tmp = get_ue_golomb_bs();
	if(sub_mb_type_tmp >=4){
	  return -1;
	}
	sub_partition_count[i]= p_sub_mb_type_info[ sub_mb_type_tmp ].partition_count;
	h->sub_mb_type[i]=      p_sub_mb_type_info[ sub_mb_type_tmp ].type;
      }
    }

    for(list=0; list<h->list_count; list++){
      int ref_count= IS_REF0(mb_type) ? 1 : h->ref_count[list];
      for(i=0; i<4; i++){
	if(IS_DIRECT(h->sub_mb_type[i])) continue;
	if(IS_DIR(h->sub_mb_type[i], 0, list)){
	  unsigned int tmp = get_te0_golomb_bs(ref_count);
	  if(tmp>=ref_count){
	    return -1;
	  }
	  ref[list][i]= tmp;
	}else{
	  ref[list][i] = -1;
	}
      }
    }

    if(dct8x8_allowed)
      dct8x8_allowed = get_dct8x8_allowed(h);

    for(list=0; list<h->list_count; list++){
      for(i=0; i<4; i++){
	if(IS_DIRECT(h->sub_mb_type[i])) {
	  h->ref_cache[list][ scan8[4*i] ] = h->ref_cache[list][ scan8[4*i]+1 ];
	  continue;
	}
	h->ref_cache[list][ scan8[4*i] ] = h->ref_cache[list][ scan8[4*i]+1 ]=
	  h->ref_cache[list][ scan8[4*i]+8 ]=h->ref_cache[list][ scan8[4*i]+9 ]= ref[list][i];

	if(IS_DIR(h->sub_mb_type[i], 0, list)){
	  const int sub_mb_type= h->sub_mb_type[i];
	  const int block_width= (sub_mb_type & (MB_TYPE_16x16|MB_TYPE_16x8)) ? 2 : 1;
	  for(j=0; j<sub_partition_count[i]; j++){
	    const int index= 4*i + block_width*j;
	    int16_t (* mv_cache)[2]= &h->mv_cache[list][ scan8[index] ];
	    int mpxy = pred_motion_cavlc(h, index, block_width, list, h->ref_cache[list][ scan8[index] ]);
	    get_se_golomb_mvd();
	    S32I2M(xr7,mpxy);
	    Q16ADD_AA_WW(xr3,xr7,xr8,xr4);

	    if(IS_SUB_8X8(sub_mb_type)){
	      S32STD(xr3,mv_cache,4); S32STD(xr3,mv_cache,32); S32STD(xr3,mv_cache,36);
	    }else if(IS_SUB_8X4(sub_mb_type)){
	      S32STD(xr3,mv_cache,4);
	    }else if(IS_SUB_4X8(sub_mb_type)){
	      S32STD(xr3,mv_cache,32);
	    }
	    S32STD(xr3,mv_cache,0);
	  }
	}else{
	  uint32_t *p= (uint32_t *)&h->mv_cache[list][ scan8[4*i] ][0];
	  p[0] = p[1]= p[8] = p[9]= 0;
	}
      }
    }
  }else if(IS_DIRECT(mb_type)){
    pred_direct_motion_cavlc(h, &mb_type);
    dct8x8_allowed &= h->sps.direct_8x8_inference_flag;
  }else{
    int list, i;
    if(IS_16X16(mb_type)){
      for(list=0; list<h->list_count; list++){
	unsigned int val;
	if(IS_DIR(mb_type, 0, list)){
	  val= get_te0_golomb_bs(h->ref_count[list]);
	  if(val >= h->ref_count[list]){
	    return -1;
	  }
	}else
	  val= LIST_NOT_USED&0xFF;
	fill_rectangle(&h->ref_cache[list][ (4+1*8) ], 4, 4, 8, val, 1);
      }

      for(list=0; list<h->list_count; list++){
	unsigned int val;
	if(IS_DIR(mb_type, 0, list)){
	  int mpxy = pred_motion_cavlc(h, 0, 4, list, h->ref_cache[list][ (4+1*8) ]);
	  get_se_golomb_mvd();
	  S32I2M(xr7,mpxy);
	  Q16ADD_AA_WW(xr3,xr7,xr8,xr4);
	  val= S32M2I(xr3);
	}else
	  val=0;
	fill_rectangle(h->mv_cache[list][ (4+1*8) ], 4, 4, 8, val, 4);
      }
    }else if(IS_16X8(mb_type)){
      for(list=0; list<h->list_count; list++){
	for(i=0; i<2; i++){
	  unsigned int val;
	  if(IS_DIR(mb_type, i, list)){
	    val= get_te0_golomb_bs(h->ref_count[list]);
	    if(val >= h->ref_count[list]){
	      return -1;
	    }
	  }else
	    val= LIST_NOT_USED&0xFF;
	  fill_rectangle(&h->ref_cache[list][ (4+1*8) + 16*i ], 4, 2, 8, val, 1);
	}
      }

      for(list=0; list<h->list_count; list++){
	for(i=0; i<2; i++){
	  unsigned int val;
	  if(IS_DIR(mb_type, i, list)){
	    int mpxy = pred_16x8_motion_cavlc(h, 8*i, list, h->ref_cache[list][(4+1*8) + 16*i]);
	    get_se_golomb_mvd();
	    S32I2M(xr7,mpxy);
	    Q16ADD_AA_WW(xr3,xr7,xr8,xr4);
	    val= S32M2I(xr3);
	  }else
	    val=0;
	  fill_rectangle(h->mv_cache[list][ (4+1*8) + 16*i ], 4, 2, 8, val, 4);
	}
      }
    }else{
      assert(IS_8X16(mb_type));
      for(list=0; list<h->list_count; list++){
	for(i=0; i<2; i++){
	  unsigned int val;
	  if(IS_DIR(mb_type, i, list)){ //FIXME optimize
	    val= get_te0_golomb_bs(h->ref_count[list]);
	    if(val >= h->ref_count[list]){
	      return -1;
	    }
	  }else
	    val= LIST_NOT_USED&0xFF;
	  fill_rectangle(&h->ref_cache[list][ (4+1*8) + 2*i ], 2, 4, 8, val, 1);
	}
      }
      for(list=0; list<h->list_count; list++){
	for(i=0; i<2; i++){
	  unsigned int val;
	  if(IS_DIR(mb_type, i, list)){
	    int mpxy = pred_8x16_motion_cavlc(h, i*4, list, h->ref_cache[list][ (4+1*8) + 2*i ]);
	    get_se_golomb_mvd();
	    S32I2M(xr7,mpxy);
	    Q16ADD_AA_WW(xr3,xr7,xr8,xr4);
	    val= S32M2I(xr3);
	  }else
	    val=0;
	  fill_rectangle(h->mv_cache[list][ (4+1*8) + 2*i ], 2, 4, 8, val, 4);
	}
      }
    }
  }

  if(!IS_INTRA16x16(mb_type)){
    cbp= get_ue_golomb_bs();
    if(cbp > 47){
      return -1;
    }

    if(IS_INTRA4x4(mb_type))
      cbp= golomb_to_intra4x4_cbp[cbp];
    else
      cbp= golomb_to_inter_cbp[cbp];
  }
  h->cbp = cbp;

  if(dct8x8_allowed && (cbp&15) && !IS_INTRA(mb_type)){
    if(get_bits1_bs())
      mb_type |= MB_TYPE_8x8DCT;
  }

skip_mb_cavlc:
#if 1
  //int mb_x = s->mb_x;
  mv_top_left_atm[0]=mv_hat_atm[s->mb_x][0][3];
  mv_top_left_atm[1]=mv_hat_atm[s->mb_x][1][3];
  ref_top_left_atm[0]=ref_hat_atm[s->mb_x][0]>>24;
  ref_top_left_atm[1]=ref_hat_atm[s->mb_x][1]>>24;

  if(for_dblk_c){
    int top_xy, left_xy;
    int top_type, left_type;

    top_xy  = mb_xy - (s->mb_stride << FIELD_PICTURE);
    left_xy = mb_xy - 1;

    top_type  = h->slice_table[top_xy ] < 255 ? s->current_picture.mb_type[top_xy ] : 0;
    left_type = h->slice_table[left_xy] < 255 ? s->current_picture.mb_type[left_xy] : 0;

    if(IS_INTER(mb_type) || IS_DIRECT(mb_type)) {
      int list;
      for(list=0; list<h->list_count; list++){
	if(USES_LIST(top_type, list)){
	  *(uint32_t*)h->mv_cache[list][12+0-8]= mv_hat_atm[s->mb_x][list][0];
	  *(uint32_t*)h->mv_cache[list][12+1-8]= mv_hat_atm[s->mb_x][list][1];
	  *(uint32_t*)h->mv_cache[list][12+2-8]= mv_hat_atm[s->mb_x][list][2];
	  *(uint32_t*)h->mv_cache[list][12+3-8]= mv_hat_atm[s->mb_x][list][3];

	  ((int*)h->ref_cache[list])[1]=ref_hat_atm[s->mb_x][list];
	}
	if(USES_LIST(left_type, list)){
	  *(uint32_t*)h->mv_cache[list][11]=dblk_left_mv[list][0];
	  *(uint32_t*)h->mv_cache[list][19]=dblk_left_mv[list][1];
	  *(uint32_t*)h->mv_cache[list][27]=dblk_left_mv[list][2];
	  *(uint32_t*)h->mv_cache[list][35]=dblk_left_mv[list][3];

	  h->ref_cache[list][11]=dblk_left_ref[list][0];
	  h->ref_cache[list][19]=dblk_left_ref[list][1];
	  h->ref_cache[list][27]=dblk_left_ref[list][2];
	  h->ref_cache[list][35]=dblk_left_ref[list][3];
	}
      }
    }
  }

  if(IS_INTER(mb_type)) write_back_motion_cavlc(h, mb_type);
#endif

#if 1
  {
    int i;
    if(IS_INTRA(mb_type)) {
      /*---- dMB intra_mb_info part; constant length ----*/
      dMB_Intra = (H264_MB_Intra_DecARGs * )ctrl_end;
      dMB_Intra->chroma_pred_mode = (uint8_t)h->chroma_pred_mode;
      dMB_Intra->intra16x16_pred_mode = (uint8_t)h->intra16x16_pred_mode;
      dMB_Intra->topleft_samples_available = h->topleft_samples_available;
      dMB_Intra->topright_samples_available = h->topright_samples_available;
      uint32_t intra4x4_pred_mode = 0;
      for(i=0; i<8; i++)
	intra4x4_pred_mode |= h->intra4x4_pred_mode_cache[scan8[i]] << 4*i;
      dMB_Intra->intra4x4_pred_mode0 = intra4x4_pred_mode;
      intra4x4_pred_mode = 0;
      for(i=8; i<16; i++)
	intra4x4_pred_mode |= h->intra4x4_pred_mode_cache[scan8[i]] << 4*(i-8);
      dMB_Intra->intra4x4_pred_mode1 = intra4x4_pred_mode;

      task_fifo_wp += (sizeof(H264_MB_Intra_DecARGs) + 3) >> 2;
    } else {
      /*---- dMB inter_mb_info part; veriable length ----*/
      if (IS_8X8(mb_type)){
	dMB_Ctrl->sub_mb_type[0] = h->sub_mb_type[0];
	dMB_Ctrl->sub_mb_type[1] = h->sub_mb_type[1];
	dMB_Ctrl->sub_mb_type[2] = h->sub_mb_type[2];
	dMB_Ctrl->sub_mb_type[3] = h->sub_mb_type[3];
      }


      int *dMB_mvnum = ctrl_end;
      char *dMB_ref = dMB_mvnum+1;
      int *dMB_mvtype = dMB_ref + (8<<(h->slice_type-2));
      int *dMB_mvneib = dMB_mvtype + 1;
      int *dMB_mvcurr = dMB_mvneib + (8<<(h->slice_type-2));

      int pos = 0;
      uint32_t * dMB_mv = dMB_mvcurr;

      int l;

      for(l=0; l<h->slice_type-1; l++){
	*dMB_ref++ = h->ref_cache[l][12];
	*dMB_ref++ = h->ref_cache[l][14];
	*dMB_ref++ = h->ref_cache[l][28];
	*dMB_ref++ = h->ref_cache[l][30];
	*dMB_ref++ = h->ref_cache[l][4];
	*dMB_ref++ = h->ref_cache[l][6];
	*dMB_ref++ = h->ref_cache[l][11];
	*dMB_ref++ = h->ref_cache[l][27];

	*dMB_mvneib++ = *(uint32_t*)h->mv_cache[l][4];
	*dMB_mvneib++ = *(uint32_t*)h->mv_cache[l][5];
	*dMB_mvneib++ = *(uint32_t*)h->mv_cache[l][6];
	*dMB_mvneib++ = *(uint32_t*)h->mv_cache[l][7];
	*dMB_mvneib++ = *(uint32_t*)h->mv_cache[l][11];
	*dMB_mvneib++ = *(uint32_t*)h->mv_cache[l][19];
	*dMB_mvneib++ = *(uint32_t*)h->mv_cache[l][27];
	*dMB_mvneib++ = *(uint32_t*)h->mv_cache[l][35];

	if(IS_16X16(mb_type)){
	  *dMB_mvtype = 0;
	  dMB_mv[ pos++ ] = *(uint32_t*)h->mv_cache[l][12];
	}else if(IS_16X8(mb_type)){
	  *dMB_mvtype = 1;
	  dMB_mv[ pos++ ] = *(uint32_t*)h->mv_cache[l][12];
	  dMB_mv[ pos++ ] = *(uint32_t*)h->mv_cache[l][28];
	}else if(IS_8X16(mb_type)){
	  *dMB_mvtype = 2;
	  dMB_mv[ pos++ ] = *(uint32_t*)h->mv_cache[l][12];
	  dMB_mv[ pos++ ] = *(uint32_t*)h->mv_cache[l][14];
	}else{
	  int i;
	  int mb_type_info = 0;
	  for(i=0; i<4; i++){
	    int n = i*4;
	    const int sub_mb_type= h->sub_mb_type[i];
	    if(IS_SUB_8X8(sub_mb_type)){
	      dMB_mv[ pos++ ] = *(uint32_t*)h->mv_cache[l][scan8[n]];
	    }else if(IS_SUB_8X4(sub_mb_type)){
	      mb_type_info |= 1 << (i*8);
	      dMB_mv[ pos++ ] = *(uint32_t*)h->mv_cache[l][scan8[n]];
	      dMB_mv[ pos++ ] = *(uint32_t*)h->mv_cache[l][scan8[n+2]];
	    }else if(IS_SUB_4X8(sub_mb_type)){
	      mb_type_info |= 2 << (i*8);
	      dMB_mv[ pos++ ] = *(uint32_t*)h->mv_cache[l][scan8[n]];
	      dMB_mv[ pos++ ] = *(uint32_t*)h->mv_cache[l][scan8[n+1]];
	    }else{
	      mb_type_info |= 3 << (i*8);
	      int j;
	      for(j=0; j<4; j++)
		dMB_mv[ pos++ ] = *(uint32_t*)h->mv_cache[l][scan8[n+j]];
	    }
	  }
	  *dMB_mvtype = mb_type_info | ((mb_type_info & 0xFF) << 4) | 3;
	}
      }
      task_fifo_wp = dMB_mvcurr+pos;
      *dMB_mvnum = pos >> (h->slice_type==3);
    }
    dMB_Ctrl->ctrl_len = (unsigned int)(task_fifo_wp - task_fifo_wp_d1) << 2;
  }  
#endif

#if 0
  if(h->cbp || IS_INTRA(mb_type)){
    s->dsp.clear_blocks(mb);//memset(blocks, 0, sizeof(DCTELEM)*6*64);
  }
#endif

  if(cbp || IS_INTRA16x16(mb_type)){
    int i8x8, i4x4, chroma_idx;
    int dquant;
    const uint8_t *scan, *scan8x8, *dc_scan;

    res = (H264_MB_Res *)task_fifo_wp;
    task_fifo_wp += (sizeof(H264_MB_Res) + 3) >> 2;

    DCTELEM *mb = &res->mb;

    if(IS_INTERLACED(mb_type)){
      scan8x8= s->qscale ? h->field_scan8x8_cavlc : h->field_scan8x8_cavlc_q0;
      scan= s->qscale ? h->field_scan : h->field_scan_q0;
      dc_scan= luma_dc_field_scan;
    }else{
      scan8x8= s->qscale ? h->zigzag_scan8x8_cavlc : h->zigzag_scan8x8_cavlc_q0;
      scan= s->qscale ? h->zigzag_scan : h->zigzag_scan_q0;
      dc_scan= luma_dc_zigzag_scan;
    }

    dquant= get_se_golomb_bs();

    if( dquant > 25 || dquant < -26 ){
      return -1;
    }

    s->qscale += dquant;
    if(((unsigned)s->qscale) > 51){
      if(s->qscale<0) s->qscale+= 52;
      else            s->qscale-= 52;
    }

    h->chroma_qp[0]= get_chroma_qp(h, 0, s->qscale);
    h->chroma_qp[1]= get_chroma_qp(h, 1, s->qscale);

    if(IS_INTRA16x16(mb_type)){
      //if( decode_residual_bs(h, h->mb, LUMA_DC_BLOCK_INDEX, dc_scan, NULL, 16) < 0){
      if( decode_residual_bs(h, mb, LUMA_DC_BLOCK_INDEX, dc_scan, NULL, 16) < 0){	
	return -1;
      }

      assert((cbp&15) == 0 || (cbp&15) == 15);

      if(cbp&15){
	const uint32_t *qmul = h->dequant4_coeff[0][s->qscale];
	for(i8x8=0; i8x8<4; i8x8++){
	  for(i4x4=0; i4x4<4; i4x4++){
	    const int index= i4x4 + 4*i8x8;
	    //if( decode_residual_bs(h, h->mb + 16*index, index, scan + 1, qmul, 15) < 0 ){
	    if( decode_residual_bs(h, mb + 16*index, index, scan + 1, qmul, 15) < 0 ){
	      return -1;
	    }
	  }
	}
      }else{
	fill_rectangle(&h->non_zero_count_cache[(4+1*8)], 4, 4, 8, 0, 1);
      }

    }else{
      if(IS_8x8DCT(mb_type)){
	const uint32_t *qmul = h->dequant8_coeff[IS_INTRA( mb_type ) ? 0:1][s->qscale];	
	for(i8x8=0; i8x8<4; i8x8++){
	  uint8_t * const nnz= &h->non_zero_count_cache[ scan8[4*i8x8] ];	  
	  if(cbp & (1<<i8x8)){
	    //DCTELEM *buf = &h->mb[64*i8x8];
	    DCTELEM *buf = &mb[64*i8x8];
	    for(i4x4=0; i4x4<4; i4x4++){
	      if( decode_residual_bs(h, buf, i4x4+4*i8x8, scan8x8+16*i4x4, qmul, 16) < 0 )
		return -1;
	    }
	    nnz[0] += nnz[1] + nnz[8] + nnz[9];
	  } else {
	    nnz[0] = nnz[1] = nnz[8] = nnz[9] = 0;
	  }
	}
      } else {
	const uint32_t *qmul = h->dequant4_coeff[IS_INTRA( mb_type ) ? 0:3][s->qscale];
	for(i8x8=0; i8x8<4; i8x8++){
	  if(cbp & (1<<i8x8)){
	    for(i4x4=0; i4x4<4; i4x4++){
	      const int index= i4x4 + 4*i8x8;
	      //if( decode_residual_bs(h, h->mb + 16*index, index, scan, qmul, 16) <0 )
	      if( decode_residual_bs(h, mb + 16*index, index, scan, qmul, 16) <0 )
		return -1;
	    }
	  } else {
	    uint8_t * const nnz= &h->non_zero_count_cache[ scan8[4*i8x8] ];
	    nnz[0] = nnz[1] = nnz[8] = nnz[9] = 0;
	  }
	}
      }

    }

    if(cbp&0x30){
      for(chroma_idx=0; chroma_idx<2; chroma_idx++) {
	//if( decode_residual_bs(h, h->mb + 256 + 16*4*chroma_idx, CHROMA_DC_BLOCK_INDEX + chroma_idx, chroma_dc_scan, NULL, 4) < 0){
	if( decode_residual_bs(h, mb + 256 + 16*4*chroma_idx, CHROMA_DC_BLOCK_INDEX + chroma_idx, chroma_dc_scan, NULL, 4) < 0){
	  return -1;
	}
      }
    }

    if(cbp&0x20){
      for(chroma_idx=0; chroma_idx<2; chroma_idx++){
	const uint32_t *qmul = h->dequant4_coeff[chroma_idx+1+(IS_INTRA( mb_type ) ? 0:3)][h->chroma_qp[chroma_idx]];
	for(i4x4=0; i4x4<4; i4x4++){
	  const int index= 16 + 4*chroma_idx + i4x4;
	  //if( decode_residual_bs(h, h->mb + 16*index, index, scan + 1, qmul, 15) < 0){
	  if( decode_residual_bs(h, mb + 16*index, index, scan + 1, qmul, 15) < 0){
	    return -1;
	  }
	}
      }
    }else{
      uint8_t * const nnz= &h->non_zero_count_cache[0];
      nnz[ (1+1*8)+0 ] = nnz[ (1+1*8)+1 ] =nnz[ (1+1*8)+8 ] =nnz[ (1+1*8)+9 ] =
	nnz[ (1+4*8)+0 ] = nnz[ (1+4*8)+1 ] =nnz[ (1+4*8)+8 ] =nnz[ (1+4*8)+9 ] = 0;
    }

  }else{
    uint8_t * const nnz= &h->non_zero_count_cache[0];

    fill_rectangle(&nnz[(4+1*8)], 4, 4, 8, 0, 1);
    nnz[ (1+1*8)+0 ] = nnz[ (1+1*8)+1 ] =nnz[ (1+1*8)+8 ] =nnz[ (1+1*8)+9 ] =
    nnz[ (1+4*8)+0 ] = nnz[ (1+4*8)+1 ] =nnz[ (1+4*8)+8 ] =nnz[ (1+4*8)+9 ] = 0;
  }

  s->current_picture.qscale_table[mb_xy]= s->qscale;
  s->current_picture.mb_type[mb_xy]= mb_type;
  write_back_non_zero_count_cavlc(h);
#ifdef JZC_PMON_P0ed
    PMON_OFF(parse);
#endif

  return 0;
}


__cache_text1__
void hl_decode_cavlc_mb(H264Context *h) {
  MpegEncContext * const s = &h->s;
  int i;
  int mb_x= s->mb_x;
  int mb_y= s->mb_y;
  int mb_xy= h->mb_xy;

  int top_xy  = mb_xy  - (s->mb_stride << FIELD_PICTURE);

  int mb_type= s->current_picture.mb_type[mb_xy];

  if (for_dblk_c) {
    fill_caches_cavlc_for_dblk(h, mb_type);
  }

  {
    /*---- dMB control part; constant length ----*/
    dMB_Ctrl->new_slice = h->new_slice_p0;   
    h->new_slice_p0 = 0;

    dMB_Ctrl->mb_x = (uint8_t)mb_x;
    dMB_Ctrl->mb_y = (uint8_t)mb_y;

    dMB_Ctrl->cbp = h->cbp;

    dMB_Ctrl->mb_type = mb_type;
    dMB_Ctrl->left_mb_type = (uint8_t)s->current_picture.mb_type[mb_xy -1];
    dMB_Ctrl->top_mb_type = (uint8_t)s->current_picture.mb_type[top_xy];

    dMB_Ctrl->qscale = s->qscale;
    dMB_Ctrl->qp_left= s->current_picture.qscale_table[mb_xy - 1];
    dMB_Ctrl->qp_top = s->current_picture.qscale_table[top_xy];

    dMB_Ctrl->chroma_qp[0] = h->chroma_qp[0];
    dMB_Ctrl->chroma_qp_left[0] = h->pps.chroma_qp_table[0][dMB_Ctrl->qp_left & 0xff];
    dMB_Ctrl->chroma_qp_top[0] = h->pps.chroma_qp_table[0][dMB_Ctrl->qp_top & 0xff];

    dMB_Ctrl->chroma_qp[1] = h->chroma_qp[1];
    dMB_Ctrl->chroma_qp_left[1] = h->pps.chroma_qp_table[1][dMB_Ctrl->qp_left & 0xff];
    dMB_Ctrl->chroma_qp_top[1] = h->pps.chroma_qp_table[1][dMB_Ctrl->qp_top & 0xff];

    dMB_Ctrl->dblk_start  = ( (h->deblocking_filter==2 &&
			       h->slice_table[mb_xy -1] != h->slice_table[mb_xy])
			      || (h->slice_table[mb_xy -1] == 255) ) ? 1 : 0;
    dMB_Ctrl->dblk_start |= ( (h->deblocking_filter==2 &&
			       h->slice_table[top_xy] != h->slice_table[mb_xy])
			      || (h->slice_table[top_xy] == 255) ) ? 0x10 : 0;

    int is_intra = IS_INTRA(mb_type);
    dMB_Ctrl->dc_dequant4_coeff_Y = h->dequant4_coeff[0][s->qscale][0];
    dMB_Ctrl->dc_dequant4_coeff_U = h->dequant4_coeff[is_intra ? 1:4][h->chroma_qp[0]][0];
    dMB_Ctrl->dc_dequant4_coeff_V = h->dequant4_coeff[is_intra ? 2:5][h->chroma_qp[1]][0];

    static const cache_to_tl_8bits[8] = {4,5,6,7,11,19,27,35};
    unsigned int nnz_tl_8bits = 0;
    for(i=0;i<8;i++){
      if(h->non_zero_count_cache[cache_to_tl_8bits[i]]){
	nnz_tl_8bits |= (1<<i);  
      }
    }

    dMB_Ctrl->nnz_tl_8bits = nnz_tl_8bits;
    
    /* cavlc residual part; constant length*/
    if(h->cbp || IS_INTRA16x16(mb_type)){
      uint8_t * dmb_nnz = &res->dmb_nnz;

      for(i=0 ; i<16; i++){
	  dmb_nnz[ scan4x4[i] ] = h->non_zero_count_cache[ scan8[i] ];
      }

      for(i=16; i<24; i++)
        dmb_nnz[i] = h->non_zero_count_cache[ scan8[i] ];
    }

    ((H264_MB_Ctrl_DecARGs *)task_fifo_wp_d2)->next_mb_len = (unsigned int)(task_fifo_wp-task_fifo_wp_d1) << 2;

    if ( !mb_xy ){
      *(int *)(TCSM1_VUCADDR(TCSM1_FIRST_MBLEN)) = ((H264_MB_Ctrl_DecARGs *)task_fifo_wp_d2)->next_mb_len;
    }

    (*tcsm1_fifo_wp)++;

    if(((unsigned int)task_fifo_wp + MAX_TASK_LEN) > 0xF4004000){//reach_tcsm0_end
      task_fifo_wp = (int *)TCSM0_TASK_FIFO;
    }

    task_fifo_wp_d2 = task_fifo_wp_d1;
    task_fifo_wp_d1 = task_fifo_wp;
  }
}

