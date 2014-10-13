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

__cache_text0__
int loop_cabac_mb(H264Context *h, const int part_mask){
  MpegEncContext * const s = &h->s;
  for(;;){


#ifdef JZC_PMON_P0ed
    PMON_ON(parse);
#endif
    int ret = decode_mb_cabac_comp(h);

#ifdef JZC_PMON_P0ed
    PMON_OFF(parse);
#endif

#ifdef JZC_PMON_P0ed
    PMON_ON(dmb);
#endif
    if(ret>=0) hl_decode_cabac_mb(h);

    int eos;
    eos = get_cabac_terminate_hw();

    {
      if(ret < 0){
	//ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x, s->mb_y, (AC_ERROR|DC_ERROR|MV_ERROR)&part_mask);
	return -1;
      }
    }

#ifdef JZC_PMON_P0ed
    PMON_OFF(dmb);
#endif

    if( eos || s->mb_y >= s->mb_height ) {
      return 0;
    }
  }
}

int *task_fifo_res_wp;
int (* const mvd_hat_atm)[2][4]=(int*)MVD_HAT_PARA_ATM;
//__attribute__ ((aligned (32))) int mvd_hat_atm[H264_MAX_ROW_MBNUM][2][4];
int (* const mv_hat_atm)[2][4]=(int*)MV_HAT_PARA_ATM;
//int mv_hat_atm[80][2][4];
uint16_t * const direct_hat_atm=(uint16_t*)DIRECT_HAT_PARA_ATM;
uint32_t (* const ref_hat_atm)[2]=(int*)REF_HAT_PARA_ATM;
int mv_top_left_atm[2];
int8_t ref_top_left_atm[2];

//__cache_text0__ 
static av_always_inline int fetch_diagonal_mv_cabac(H264Context *h, uint32_t *C, int i, int list, int part_width){
    const int topright_ref= h->ref_cache[list][ i - 8 + part_width ];

    if(topright_ref != PART_NOT_AVAILABLE){
      *C= *(uint32_t*)h->mv_cache[list][ i - 8 + part_width ];
      return topright_ref;
    }else{
      *C= *(uint32_t*)h->mv_cache[list][ i - 8 - 1 ];
      return h->ref_cache[list][ i - 8 - 1 ];
    }
}

__cache_text0__ 
static uint32_t pred_motion_cabac(H264Context *h, int n, int part_width, int list, int ref){
    const int index8= scan8[n];
    const int top_ref=      h->ref_cache[list][ index8 - 8 ];
    const int left_ref=     h->ref_cache[list][ index8 - 1 ];
    const uint32_t A= *((uint32_t*)h->mv_cache[list][ index8 - 1 ]);
    const uint32_t B= *((uint32_t*)h->mv_cache[list][ index8 - 8 ]);
    uint32_t C;
    int diagonal_ref, match_count;

    diagonal_ref= fetch_diagonal_mv_cabac(h, &C, index8, list, part_width);
    match_count= (diagonal_ref==ref) + (top_ref==ref) + (left_ref==ref);
    if(match_count > 1){ //most common
      S32I2M(xr1,A);
      S32I2M(xr2,B);
      S32I2M(xr3,C);
      D16MAX(xr4,xr1,xr2);//xr4=max of (A,B)
      D16MIN(xr5,xr1,xr2);//xr5=min of (A,B)
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
	S32I2M(xr1,A);
	S32I2M(xr2,B);
	S32I2M(xr3,C);
	D16MAX(xr4,xr1,xr2);//xr4=max of (A,B)
	D16MIN(xr5,xr1,xr2);//xr5=min of (A,B)
	D16MIN(xr4,xr4,xr3);//xr4:comp max(A,B) and C, get rid of the MAX of (A,B,C)
	D16MAX(xr4,xr4,xr5);//xr4:get rid of the MIN of (A,B,C)
	return S32M2I(xr4);
      }
    }
}

//it only be used in pred_direct_motion_cabac 
//xr15=0x20002 is default, so S32I2M(xr15,0x20002) at the begin of pred_direct_motion_cabac
//xr15 can not be used during pred_direct_motion_cabac
static av_always_inline int colZeroFlag(int * col_mv){
  S32LDD(xr1,col_mv,0);
  D16CPS(xr3,xr1,xr1);//xr1:abs_mvcol
  D16SLT(xr4,xr3,xr15);//FFABS(mvcol) < 2
  S32SFL(xr2,xr0,xr4,xr0,3);//xr2=xr1>>16
  S32AND(xr5,xr4,xr2);
  return S32M2I(xr5);
}

__cache_text0__ 
static void pred_direct_motion_cabac(H264Context * const h, int *mb_type){
  if(h->sps.direct_8x8_inference_flag){
    S32I2M(xr15,0x20002);
    const int mb_xy =   h->mb_xy;//s->mb_x +   s->mb_y*s->mb_stride;
    const int mb_type_col = h->ref_list[1][0].mb_type[mb_xy];
    const int * l1mv0_hl=h->ref_list[1][0].motion_3dot0_above[mb_xy][0];
    i_pref(0x1,l1mv0_hl,0);//prefetch
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

    tprintf(s->avctx, "mb_type = %08x, sub_mb_type = %08x, is_b8x8 = %d, mb_type_col = %08x\n", *mb_type, sub_mb_type, is_b8x8, mb_type_col);

    int is_b16x16=IS_16X16(*mb_type);
    int is_col_intra=IS_INTRA(mb_type_col);
    if(h->direct_spatial_mv_pred){
        int ref[2];
        int32_t mv[2];
        int list;

        /* FIXME interlacing + spatial direct uses wrong colocated block positions */

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
		  mv[list]=pred_motion_cabac(h, 0, 4, list, ref[list]);
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
	       ((l1ref0_hl[0] == 0 && colZeroFlag(l1mv0_hl))
		|| (l1ref0_hl[0]  < 0 && l1ref1_hl[0] == 0 && colZeroFlag(l1mv1_hl) && x264_build_flag))){
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
		  if(colZeroFlag(l1mv_hl)){
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

	      S32LDDV(xr1,dist_scale_factor,ref0,2);
	      const int32_t *mv_col = l1ref0_hl[0] >= 0 ? l1mv0_hl : l1mv1_hl;
	      S32LDD(xr2,mv_col,0);//xr2=mv_col
	      Q16SLL(xr3,xr2,xr0,xr0,3);//mv <<3, scale<<4, d16_mulf <<1,overall shift 8bit
	      D16MULF_LW(xr4,xr1,xr3);//xr4=mvl0
	      ref= ref0;
	      Q16ADD_SS_WW(xr5,xr4,xr2,xr0);//xr5=mvl0-mv_col
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
	    S32LDDV(xr1,dist_scale_factor,ref0,2);
	    
	    fill_rectangle(&h->ref_cache[0][scan8[i4_base]], 2, 2, 8, ref0, 1);
	    const int32_t *mv_col = l1mv+i8;
	    S32LDD(xr2,mv_col,0);//xr2=mv_col
	    Q16SLL(xr3,xr2,xr0,xr0,3);//mv <<3, scale<<4, d16_mulf <<1,overall shift 8bit
	    D16MULF_LW(xr4,xr1,xr3);//xr4=mvl0
	    Q16ADD_SS_WW(xr5,xr4,xr2,xr0);//xr5=mvl0-mv_col
	    int mv0=S32M2I(xr4);
	    int mv1=S32M2I(xr5);
	    fill_rectangle(&h->mv_cache[0][scan8[i4_base]], 2, 2, 8, mv0, 4);
	    fill_rectangle(&h->mv_cache[1][scan8[i4_base]], 2, 2, 8, mv1, 4);
	  }
        }
    }
    i_cache(0x11,l1mv0_hl,0);//invalid
  }
  else{
    pred_direct_motion_cabac_L3dot0_below(h,mb_type);
  }
}

static av_always_inline uint32_t pred_16x8_motion_cabac(H264Context *h, int n, int list, int ref){
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
    return pred_motion_cabac(h, n, 4, list, ref);
}
static av_always_inline uint32_t pred_8x16_motion_cabac(H264Context *h, int n, int list, int ref){
    if(n==0){
        const int left_ref=      h->ref_cache[list][ 12 - 1 ];
        const uint32_t * const A=  h->mv_cache[list][ 12 - 1 ];
        if(left_ref == ref){
            return A[0];
        }
    }else{
        uint32_t C;
        int diagonal_ref;
        diagonal_ref= fetch_diagonal_mv_cabac(h, &C, 14, list, 2);
        if(diagonal_ref == ref){
            return C;
        }
    }
    return pred_motion_cabac(h, n, 2, list, ref);
}

//__cache_text0__ 
static av_always_inline 
void hl_decode_8x8_mb(H264Context *h, int *mb_type, int *dct8x8_allowed){
  int i, j, sub_partition_count[4], list, ref[2][4];
  if( h->slice_type == FF_B_TYPE ) {
    for( i = 0; i < 4; i++ ) {
      int sub_mb_type_tmp = decode_cabac_b_mb_sub_type_hw();
      sub_partition_count[i]= b_sub_mb_type_info[ sub_mb_type_tmp ].partition_count;
      h->sub_mb_type[i]=      b_sub_mb_type_info[ sub_mb_type_tmp ].type;
    }
    if( IS_DIRECT(h->sub_mb_type[0] | h->sub_mb_type[1] |
		  h->sub_mb_type[2] | h->sub_mb_type[3]) ) {
      pred_direct_motion_cabac(h, mb_type);
      h->ref_cache[0][6+1*8] =
	h->ref_cache[1][6+1*8] =
	h->ref_cache[0][6+3*8] =
	h->ref_cache[1][6+3*8] = PART_NOT_AVAILABLE;
      if( h->ref_count[0] > 1 || h->ref_count[1] > 1 ) {
	for( i = 0; i < 4; i++ )
	  if( IS_DIRECT(h->sub_mb_type[i]) ){
	    h->direct_slim_cache[6+i+(i&0x2)]=1;
	  }
      }
    }
  } else {
    for( i = 0; i < 4; i++ ) {
      int sub_mb_type_tmp = decode_cabac_p_mb_sub_type_hw();
      sub_partition_count[i]= p_sub_mb_type_info[ sub_mb_type_tmp ].partition_count;
      h->sub_mb_type[i]=      p_sub_mb_type_info[ sub_mb_type_tmp ].type;
    }
  }

  for( list = 0; list < h->list_count; list++ ) {
    for( i = 0; i < 4; i++ ) {
      if(IS_DIRECT(h->sub_mb_type[i])) continue;
      if(IS_DIR(h->sub_mb_type[i], 0, list)){
	if( h->ref_count[list] > 1 )
	  ref[list][i] = decode_cabac_mb_ref_hw( h, list, 4*i );
	else
	  ref[list][i] = 0;
      } else {
	ref[list][i] = -1;
      }
      h->ref_cache[list][ scan8[4*i]+1 ]=
	h->ref_cache[list][ scan8[4*i]+8 ]=h->ref_cache[list][ scan8[4*i]+9 ]= ref[list][i];
    }
  }

  if(*dct8x8_allowed)
    *dct8x8_allowed = get_dct8x8_allowed(h);

  for(list=0; list<h->list_count; list++){
    for(i=0; i<4; i++){
      h->ref_cache[list][ scan8[4*i]   ]=h->ref_cache[list][ scan8[4*i]+1 ];
      if(IS_DIRECT(h->sub_mb_type[i])){
	fill_rectangle(h->mvd_cache[list][scan8[4*i]], 2, 2, 8, 0, 4);
	continue;
      }

      if(IS_DIR(h->sub_mb_type[i], 0, list)){
	const int sub_mb_type= h->sub_mb_type[i];
	const int block_width= (sub_mb_type & (MB_TYPE_16x16|MB_TYPE_16x8)) ? 2 : 1;
	for(j=0; j<sub_partition_count[i]; j++){
	  int mpxy;
	  int mvd;
	  //short *mvd_p=&mvd;
	  const int index= 4*i + block_width*j;
	  int16_t (* mv_cache)[2]= &h->mv_cache[list][ scan8[index] ];
	  int16_t (* mvd_cache)[2]= &h->mvd_cache[list][ scan8[index] ];
	  mpxy=pred_motion_cabac(h, index, block_width, list, h->ref_cache[list][ scan8[index] ]);
	  mvd=decode_cabac_mb_mvd_hw( mvd_cache );

	  S32I2M(xr1,mpxy);
	  S32I2M(xr2,mvd);
	  Q16ADD_AA_WW(xr3,xr1,xr2,xr4);

	  if(IS_SUB_8X8(sub_mb_type)){
	    S32STD(xr3,mv_cache,4); S32STD(xr3,mv_cache,32); S32STD(xr3,mv_cache,36);
	    S32STD(xr2,mvd_cache,4); S32STD(xr2,mvd_cache,32); S32STD(xr2,mvd_cache,36);
	  }else if(IS_SUB_8X4(sub_mb_type)){
	    S32STD(xr3,mv_cache,4);
	    S32STD(xr2,mvd_cache,4);
	  }else if(IS_SUB_4X8(sub_mb_type)){
	    S32STD(xr3,mv_cache,32);
	    S32STD(xr2,mvd_cache,32);
	  }
	  S32STD(xr3,mv_cache,0);
	  S32STD(xr2,mvd_cache,0);
	}
      }else{
	uint32_t *p= (uint32_t *)&h->mv_cache[list][ scan8[4*i] ][0];
	uint32_t *pd= (uint32_t *)&h->mvd_cache[list][ scan8[4*i] ][0];
	p[0] = p[1] = p[8] = p[9] = 0;
	pd[0]= pd[1]= pd[8]= pd[9]= 0;
      }
    }
  }
}

static av_always_inline 
void hl_decode_16x16_mb(H264Context *h, int mb_type){
  int list;
  int mpxy;
  for(list=0; list<h->list_count; list++){
    int ref;
    if(IS_DIR(mb_type, 0, list))
      ref = h->ref_count[list] > 1 ? decode_cabac_mb_ref_hw( h, list, 0 ) : 0;
    else
      ref = (uint8_t)LIST_NOT_USED;
    fill_rectangle(&h->ref_cache[list][ 12 ], 4, 4, 8, ref, 1);
  }
  for(list=0; list<h->list_count; list++){
    if(IS_DIR(mb_type, 0, list)){
      mpxy=pred_motion_cabac(h, 0, 4, list, h->ref_cache[list][ 12 ]);
      int mvd;
      //short *mvd_p=&mvd;
      int *mvd_cc=h->mvd_cache[list][ 12 ];
      mvd=decode_cabac_mb_mvd_hw( mvd_cc );

      S32I2M(xr1,mpxy);
      S32I2M(xr2,mvd);
      Q16ADD_AA_WW(xr3,xr1,xr2,xr4);
      //      fill_rectangle(mvd_cc, 4, 4, 8, mvd, 4);
      mvd_cc[3]=mvd_cc[11]=mvd_cc[19]=mvd_cc[27]=mvd_cc[26]=mvd_cc[25]=mvd_cc[24]=mvd;
    }else
      S32AND(xr3,xr3,xr0);

    //xr3=mv,  fill_rectangle(h->mv_cache[list][ 12 ], 4, 4, 8, mv, 4);
    int *mv=h->mv_cache[list][ 12 ];
    S32STD(xr3,mv,0); S32STD(xr3,mv,4); S32STD(xr3,mv,8); S32STD(xr3,mv,12);
    S32SDI(xr3,mv,32); S32STD(xr3,mv,4); S32STD(xr3,mv,8); S32STD(xr3,mv,12);
    S32SDI(xr3,mv,32); S32STD(xr3,mv,4); S32STD(xr3,mv,8); S32STD(xr3,mv,12);
    S32SDI(xr3,mv,32); S32STD(xr3,mv,4); S32STD(xr3,mv,8); S32STD(xr3,mv,12);
  }
}

//__cache_text0__ 
static av_always_inline 
void hl_decode_8x16_mb(H264Context *h, int mb_type){
  int list, i;
  int mpxy;

  for(list=0; list<h->list_count; list++){
    for(i=0; i<2; i++){
      int ref;
      if(IS_DIR(mb_type, i, list)){ //FIXME optimize
	ref= h->ref_count[list] > 1 ? decode_cabac_mb_ref_hw( h, list, 4*i ) : 0;
      }else
	ref=LIST_NOT_USED&0xFF;
      fill_rectangle(&h->ref_cache[list][ 12 + 2*i ], 2, 4, 8, ref, 1);
    }
  }

  for(list=0; list<h->list_count; list++){
    int *mvd_cc;
    for(i=0; i<2; i++){
      int mvd;
      int *mv_cc=h->mv_cache[list][ 12 + 2*i ];
      mvd_cc=h->mvd_cache[list][ 12 + 2*i ];
      if(IS_DIR(mb_type, i, list)){
	mpxy=pred_8x16_motion_cabac(h, i*4, list, h->ref_cache[list][ 12 + 2*i ]);
	//short *mvd_p=&mvd;
	mvd=decode_cabac_mb_mvd_hw( mvd_cc );

	S32I2M(xr1,mpxy);
	S32I2M(xr2,mvd);
	Q16ADD_AA_WW(xr3,xr1,xr2,xr4);
      }else{
	S32AND(xr2,xr2,xr0);
	S32AND(xr3,xr3,xr0);
      }
      //xr2=mvd,  fill_rectangle(h->mvd_cache[list][ 12 ], 4, 4, 8, mvd, 4);
      //xr3=mv,  fill_rectangle(h->mv_cache[list][ 12 ], 4, 4, 8, mv, 4);
      S32STD(xr3,mv_cc,0); S32STD(xr3,mv_cc,4); S32SDI(xr3,mv_cc,32); S32STD(xr3,mv_cc,4);
      S32SDI(xr3,mv_cc,32); S32STD(xr3,mv_cc,4); S32SDI(xr3,mv_cc,32); S32STD(xr3,mv_cc,4);
      //      S32STD(xr2,mvd_cc,0); S32STD(xr2,mvd_cc,4); S32SDI(xr2,mvd_cc,32); S32STD(xr2,mvd_cc,4);
      //      S32SDI(xr2,mvd_cc,32); S32STD(xr2,mvd_cc,4); S32SDI(xr2,mvd_cc,32); S32STD(xr2,mvd_cc,4);
      S32STD(xr2,mvd_cc,4); S32STD(xr2,mvd_cc,96); S32STD(xr2,mvd_cc,100);
    }
    S32STD(xr2,mvd_cc,36); S32STD(xr2,mvd_cc,68);
  }
}

//__cache_text0__ 
static av_always_inline 
void hl_decode_16x8_mb(H264Context *h, int mb_type){
  int list, i;
  int mpxy;

  for(list=0; list<h->list_count; list++){
    for(i=0; i<2; i++){
      int ref;
      if(IS_DIR(mb_type, i, list)){
	ref= h->ref_count[list] > 1 ? decode_cabac_mb_ref_hw( h, list, 8*i ) : 0;
      }else
	ref=LIST_NOT_USED&0xFF;
      fill_rectangle(&h->ref_cache[list][ 12 + 16*i], 4, 2, 8, ref, 1);
    }
  }
  for(list=0; list<h->list_count; list++){
    int *mvd_cc;
    for(i=0; i<2; i++){
      int mvd;
      int *mv_cc=h->mv_cache[list][ 12 + 16*i ];
      mvd_cc=h->mvd_cache[list][ 12 + 16*i ];
      if(IS_DIR(mb_type, i, list)){
	mpxy=pred_16x8_motion_cabac(h, 8*i, list, h->ref_cache[list][12 + 16*i]);
	//short *mvd_p=&mvd;
	mvd=decode_cabac_mb_mvd_hw( mvd_cc );

	S32I2M(xr1,mpxy);
	S32I2M(xr2,mvd);
	Q16ADD_AA_WW(xr3,xr1,xr2,xr4);
      }else{
	S32AND(xr2,xr2,xr0);
	S32AND(xr3,xr3,xr0);
      }
      S32STD(xr3,mv_cc,0); S32STD(xr3,mv_cc,4); S32STD(xr3,mv_cc,8); S32STD(xr3,mv_cc,12);
      S32SDI(xr3,mv_cc,32); S32STD(xr3,mv_cc,4); S32STD(xr3,mv_cc,8); S32STD(xr3,mv_cc,12);
      //      S32STD(xr2,mvd_cc,0); S32STD(xr2,mvd_cc,4); S32STD(xr2,mvd_cc,8); S32STD(xr2,mvd_cc,12);
      //      S32SDI(xr2,mvd_cc,32); S32STD(xr2,mvd_cc,4); S32STD(xr2,mvd_cc,8); S32STD(xr2,mvd_cc,12);
      S32STD(xr2,mvd_cc,12); S32STD(xr2,mvd_cc,32); S32STD(xr2,mvd_cc,44);
    }
    S32STD(xr2,mvd_cc,36); S32STD(xr2,mvd_cc,40);
  }
}

__cache_text0__ 
static void fill_cabac_curr_caches2(H264Context *h, int mb_type){
  MpegEncContext * const s = &h->s;

  int top_type  = h->top_type_normal;
  int left_type = h->left_type_normal;
  int mb_x=s->mb_x;

  /*------------------------ fill curr MB nnz and cbp ----------------------------------*/
  int nnz_top_left, top_cbp, left_cbp;
  if(top_type){
    nnz_top_left=h->nnz_bottom_right_table[mb_x]&0xff;
    top_cbp = h->cbp_table[mb_x];
  }else if(IS_INTRA(mb_type)) {
    nnz_top_left=0xff;
    top_cbp = 0x1C0;
  }else{
    nnz_top_left=0;
    top_cbp = 0;
  }

  if(left_type){
    nnz_top_left|=h->nnz_bottom_right_table[mb_x-1]&0xff00;
    left_cbp = h->cbp & 0x1fa;
  }else if(IS_INTRA(mb_type)) {
    nnz_top_left|=0xff00;
    left_cbp = 0x1C0;
  }else{
    left_cbp = 0;
  }
  h->nnz_top_left = nnz_top_left;
  h->top_cbp = top_cbp;
  h->left_cbp = left_cbp;
}

__cache_text0__
static  void fill_cabac_next_caches(H264Context *h, int init){
  MpegEncContext * const s = &h->s;
  if(s->mb_x == (s->mb_width-1)){
    s->mb_x  = 0;
    s->mb_y += 1;
  }else{
    s->mb_x += 1;
  }

  int mb_x = s->mb_x;
  int mb_y = s->mb_y;

  int (*mvd_ha)[4]=mvd_hat_atm[mb_x];
  //  i_pref(0x1,mvd_ha,0);//prefetch
  const int mb_xy= mb_x + mb_y*s->mb_stride;
  h->mb_xy=mb_xy;
  const int top_xy = mb_xy  - s->mb_stride;
  const int left_xy = mb_xy - 1;

  int topleft_xy = top_xy - 1;
  int topright_xy= top_xy + 1;


  int slice_num = h->slice_num;
  uint8_t *p_slice_table = h->slice_table;
  uint32_t *p_mb_type = s->current_picture.mb_type;
  int top_type     = p_slice_table[top_xy     ] == slice_num ? p_mb_type[top_xy     ] : 0;
  int left_type    = p_slice_table[left_xy    ] == slice_num ? p_mb_type[left_xy    ] : 0;
  int topleft_type = p_slice_table[topleft_xy ] == slice_num ? p_mb_type[topleft_xy ] : 0;
  int topright_type= p_slice_table[topright_xy] == slice_num ? p_mb_type[topright_xy] : 0;
  int constrained_intra_pred = h->pps.constrained_intra_pred;
  
  //printf("top_type = %d, left_type = %d, topleft_type = %d, topright_type = %d, constrained_intra_pred = %d\n", \
    // top_type, left_type, topleft_type, topright_type, constrained_intra_pred);

  /*------------------------ fill next MB intra pred stuff ----------------------------------*/
  h->top_samples_available= h->left_samples_available= 0xFFFF;
  h->topleft_samples_available = 0xf0;
  h->topright_samples_available= 0x5777;

  if(!IS_INTRA(top_type) && (top_type==0 || constrained_intra_pred)){
    h->topleft_samples_available = 0xD0;
    h->top_samples_available= 0x33FF;
    h->topright_samples_available= 0x5764;
    //printf("1 -------------------------\n");
  }

  if(!IS_INTRA(left_type) && (left_type==0 || constrained_intra_pred)){
    h->topleft_samples_available&= 0xB0;
    h->left_samples_available&= 0x5F5F;
    //printf("2 -------------------------\n");
  }

  if(!IS_INTRA(topleft_type) && (topleft_type==0 || constrained_intra_pred)){
    h->topleft_samples_available&= 0xE0;
    //printf("3 -------------------------\n");
  }

  if(!IS_INTRA(topright_type) && (topright_type==0 || constrained_intra_pred)){
    h->topright_samples_available&= 0xFFDF;
    //printf("4 -------------------------\n");
  }

  int8_t * h_intra4x4_pred_mode_cache = &h->intra4x4_pred_mode_cache[0];
  if(IS_INTRA4x4(top_type)){
    ((int*)h_intra4x4_pred_mode_cache)[1]=h->top_intra4x4_pred_mode[mb_x];
    //printf("IS_INTRA4x4(top_type), ((int*)h_intra4x4_pred_mode_cache)[1] = %d\n", ((int*)h_intra4x4_pred_mode_cache)[1]);
  }else{
    int pred;
    if(!top_type || (IS_INTER(top_type) && constrained_intra_pred))
      pred= -1;
    else{
      pred= 2;
    }
    ((int*)h_intra4x4_pred_mode_cache)[1]=(uint8_t)pred*0x01010101;
    //printf("top_type is NOT intra4x4, (int*)h_intra4x4_pred_mode_cache)[1] = %d\n", ((int*)h_intra4x4_pred_mode_cache)[1]);
  }

#if 0
  if(IS_INTRA4x4(left_type)){
    h_intra4x4_pred_mode_cache[3+8*1]= h_intra4x4_pred_mode_cache[7+8*1];
    h_intra4x4_pred_mode_cache[3+8*2]= h_intra4x4_pred_mode_cache[7+8*2];
    h_intra4x4_pred_mode_cache[3+8*3]= h_intra4x4_pred_mode_cache[7+8*3];
    h_intra4x4_pred_mode_cache[3+8*4]= h_intra4x4_pred_mode_cache[7+8*4];
  }else{
    int pred;
    if(!left_type || (IS_INTER(left_type) && constrained_intra_pred))
      pred= -1;
    else{
      pred= 2;
    }
    h_intra4x4_pred_mode_cache[3+8*1]=
      h_intra4x4_pred_mode_cache[3+8*2]=
      h_intra4x4_pred_mode_cache[3+8*3]=
      h_intra4x4_pred_mode_cache[3+8*4]= pred;
  }
#else
  if(!IS_INTRA4x4(left_type)){
    int pred;
    if(!left_type || (IS_INTER(left_type) && constrained_intra_pred))
      pred= -1;
    else{
      pred= 2;
    }
    h_intra4x4_pred_mode_cache[3+8*1]=
      h_intra4x4_pred_mode_cache[3+8*2]=
      h_intra4x4_pred_mode_cache[3+8*3]=
      h_intra4x4_pred_mode_cache[3+8*4]= pred;
    //printf("left_type is NOT intra4x4, pred = %d\n", pred);
  }
#endif
  /*-------------------------- fill next MB inter pred stuff ---------------------------------*/
  int i;
  int (*mv_ha)[4]=mv_hat_atm[mb_x];
  int *ref_ha=ref_hat_atm[mb_x];
  for(i=0;i<h->slice_type-1;i++){
    uint32_t * h_mv_cache = &h->mv_cache[i][0][0];
    uint32_t * h_mvd_cache = &h->mvd_cache[i][0][0];
    int8_t * h_ref_cache = &h->ref_cache[i][0];

    if(USES_LIST(top_type, i)){
      h_mv_cache[4]= mv_ha[i][0];
      h_mv_cache[5]= mv_ha[i][1];
      h_mv_cache[6]= mv_ha[i][2];
      h_mv_cache[7]= mv_ha[i][3];

      h_mvd_cache[4]= mvd_ha[i][0];
      h_mvd_cache[5]= mvd_ha[i][1];
      h_mvd_cache[6]= mvd_ha[i][2];
      h_mvd_cache[7]= mvd_ha[i][3];

      ((uint32_t*)h_ref_cache)[1]=ref_ha[i];
    }else{
      h_mv_cache[4]= h_mv_cache[5]= h_mv_cache[6]= h_mv_cache[7]= 0;
      h_mvd_cache[4]= h_mvd_cache[5]= h_mvd_cache[6]= h_mvd_cache[7]= 0;
      *(uint32_t*)&h_ref_cache[4]=
	((top_type ? LIST_NOT_USED : PART_NOT_AVAILABLE)&0xFF)*0x01010101;
    }

    if(USES_LIST(left_type, i)){
      h_mv_cache[11]= h_mv_cache[15];
      h_mv_cache[19]= h_mv_cache[23];
      h_mv_cache[27]= h_mv_cache[31];
      h_mv_cache[35]= h_mv_cache[39];

      h_mvd_cache[11]= h_mvd_cache[15];
      h_mvd_cache[19]= h_mvd_cache[23];
      h_mvd_cache[27]= h_mvd_cache[31];
      h_mvd_cache[35]= h_mvd_cache[39];

      h_ref_cache[11]= h_ref_cache[15];
      h_ref_cache[19]= h_ref_cache[23];
      h_ref_cache[27]= h_ref_cache[31];
      h_ref_cache[35]= h_ref_cache[39];
    }else{
      h_mv_cache[11]= h_mv_cache[19]= h_mv_cache[27]= h_mv_cache[35]= 0;
      h_mvd_cache[11]= h_mvd_cache[19]= h_mvd_cache[27]= h_mvd_cache[35]= 0;
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

  if(h->slice_type == FF_B_TYPE){
    h->direct_slim_cache[5]=h->direct_slim_cache[7];
    h->direct_slim_cache[9]=h->direct_slim_cache[11];
    ((short*)h->direct_slim_cache)[1]=direct_hat_atm[mb_x];
    ((short*)h->direct_slim_cache)[3]=((short*)h->direct_slim_cache)[5]=0;
  }
}

static av_always_inline uint32_t pred_pskip_motion_cabac(H264Context *h){
    const int top_ref = h->ref_cache[0][ 12 - 8 ];
    const int left_ref= h->ref_cache[0][ 12 - 1 ];
    if(top_ref == PART_NOT_AVAILABLE || left_ref == PART_NOT_AVAILABLE
       || (top_ref == 0  && *(uint32_t*)h->mv_cache[0][ 12 - 8 ] == 0)
       || (left_ref == 0 && *(uint32_t*)h->mv_cache[0][ 12 - 1 ] == 0)){
        return 0;
    }
    return pred_motion_cabac(h, 0, 4, 0, 0);
}

__cache_text0__ 
void write_back_motion_cabac(H264Context *h, int mb_type){
  if(h->sps.direct_8x8_inference_flag){
    MpegEncContext * const s = &h->s;
    int mb_x=s->mb_x;
    const int mb_xy=h->mb_xy;
    int list;

    int (*mv_hl_index)[4]=s->current_picture.motion_3dot0_above[mb_xy];
    i_pref(30,mv_hl_index,0);
    int8_t (*ref_hl_index)[4] = s->current_picture.ref_3dot0_above[mb_xy];
    int (*mvd_ha)[4]=mvd_hat_atm[mb_x];
    //    i_pref(30,mvd_ha,0);
    int (*mv_ha)[4]=mv_hat_atm[mb_x];

    if(!USES_LIST(mb_type, 0))
      *(int*)ref_hl_index[0]=((uint8_t)LIST_NOT_USED)*0x01010101;

    for(list=0; list<h->list_count; list++){
      //int y;
        if(!USES_LIST(mb_type, list))
            continue;

#define MB_TYPE_SKIP_OR_DIRECT (MB_TYPE_SKIP|MB_TYPE_DIRECT2)
	if(mb_type&MB_TYPE_SKIP_OR_DIRECT){
	  *(uint32_t*)h->mvd_cache[list][15]=*(uint32_t*)h->mvd_cache[list][23]=
	    *(uint32_t*)h->mvd_cache[list][31]=*(uint32_t*)h->mvd_cache[list][39]=0;
	  mvd_ha[list][0]=mvd_ha[list][1]=
	    mvd_ha[list][2]=mvd_ha[list][3]=0;
	}
	else{
	  mvd_ha[list][0]=*(uint32_t*)h->mvd_cache[list][12+0 + 8*3];
	  mvd_ha[list][1]=*(uint32_t*)h->mvd_cache[list][12+1 + 8*3];
	  mvd_ha[list][2]=*(uint32_t*)h->mvd_cache[list][12+2 + 8*3];
	  mvd_ha[list][3]=*(uint32_t*)h->mvd_cache[list][12+3 + 8*3];
	}

	mv_ha[list][0]=*(uint32_t*)h->mv_cache[list][12+0 + 8*3];
	mv_ha[list][1]=*(uint32_t*)h->mv_cache[list][12+1 + 8*3];
	mv_ha[list][2]=*(uint32_t*)h->mv_cache[list][12+2 + 8*3];
	mv_ha[list][3]=*(uint32_t*)h->mv_cache[list][12+3 + 8*3];

	mv_hl_index[list][0]=*(uint32_t*)h->mv_cache[list][12];
	mv_hl_index[list][1]=*(uint32_t*)h->mv_cache[list][15];
	mv_hl_index[list][2]=*(uint32_t*)h->mv_cache[list][36];
	mv_hl_index[list][3]=*(uint32_t*)h->mv_cache[list][39];

	ref_hl_index[list][0]= h->ref_cache[list][12];
	ref_hl_index[list][1]= h->ref_cache[list][14];
	ref_hl_index[list][2]= h->ref_cache[list][28];
	ref_hl_index[list][3]= h->ref_cache[list][30];
	ref_hat_atm[mb_x][list]=((int*)(h->ref_cache[list]))[9];
    }

    direct_hat_atm[mb_x]=((short*)h->direct_slim_cache)[5];
    i_cache(0x15,mv_hl_index,0);//write back and invalid
  }
  else{
    write_back_motion_cabac_L3dot0_below(h, mb_type);
  }
}

__cache_text0__ 
int decode_mb_cabac_skip(H264Context *h){
  int mb_type=0;

  if( h->slice_type == FF_B_TYPE ) {
    mb_type|= MB_TYPE_16x16|MB_TYPE_P0L0|MB_TYPE_P0L1|MB_TYPE_DIRECT2|MB_TYPE_SKIP;

    pred_direct_motion_cabac(h, &mb_type);
    mb_type|= MB_TYPE_SKIP;
  } else {
    mb_type|= MB_TYPE_16x16|MB_TYPE_P0L0|MB_TYPE_P1L0|MB_TYPE_SKIP;

    int mv=pred_pskip_motion_cabac(h);
    fill_rectangle(&h->ref_cache[0][12], 4, 4, 8, 0, 1);
    fill_rectangle(  h->mv_cache[0][12], 4, 4, 8, mv, 4);
  }

  fill_cabac_curr_caches2(h,mb_type);

  return mb_type;
}

static av_always_inline int decode_cabac_mb_type_hw( H264Context *h ) {
  if( h->slice_type == FF_I_TYPE ) {
    int mb_type;
    int ctx=0;
    if( h->left_mb_avail && !IS_INTRA4x4( h->left_type_normal ))
      ctx++;
    if( h->top_mb_avail && !IS_INTRA4x4( h->top_type_normal ))
      ctx++;

    if( get_cabac_hw2(ctx + 3) == 0 )
      return 0;   /* I4x4 */

    if( get_cabac_terminate_hw() )
      return 25;  /* PCM */

    mb_type = 1; /* I16x16 */
    mb_type += 12 * get_cabac_hw2(1 + 5); /* cbp_luma != 0 */

    if( get_cabac_hw2(2 + 5) ) /* cbp_chroma */
      mb_type += 4 + 4 * get_cabac_hw2(2+1 + 5);

    mb_type += 2 * get_cabac_hw2(3+1 +5);
    mb_type += 1 * get_cabac_hw2(3+2 + 5);

    return mb_type;
  } else if( h->slice_type == FF_P_TYPE ) {
    return decode_cabac_p_mb_type_hw();
  } else if( h->slice_type == FF_B_TYPE ) {
    int ctx = 0;
    if( h->left_mb_avail && !IS_DIRECT( h->left_type_normal ))
      ctx++;
    if( h->top_mb_avail && !IS_DIRECT( h->top_type_normal ))
      ctx++;

    return decode_cabac_b_mb_type_hw(ctx);
  } else {
    /* TODO SI/SP frames? */
    return -1;
  }
}

static av_always_inline int decode_cabac_mb_transform_size_hw( H264Context *h ) {
  //printf("h->neighbor_transform_size = %d\n", h->neighbor_transform_size);
  return get_cabac_hw2(399 + h->neighbor_transform_size);
}

__cache_text0__ 
static int decode_cabac_mb_chroma_pre_mode_hw( H264Context *h) {
  const int mb_x=h->s.mb_x;

  int ctx = 0;

  /* No need to test for IS_INTRA4x4 and IS_INTRA16x16, as we set chroma_pred_mode_table to 0 */
  if( h->left_mb_avail && h->chroma_pred_mode_table[mb_x-1] != 0 )
    ctx++;

  if( h->top_mb_avail && h->chroma_pred_mode_table[mb_x] != 0 )
    ctx++;

  if( get_cabac_hw2(64+ctx) == 0 )
    return 0;

  if( get_cabac_hw2(64+3) == 0 )
    return 1;
  if( get_cabac_hw2(64+3) == 0 )
    return 2;
  else
    return 3;
}

__cache_text0__ 
static int decode_cabac_mb_dqp_hw( H264Context *h) {
  int   ctx = 0;
  int   val = 0;

  if( h->last_qscale_diff != 0 )
    ctx++;

  while( get_cabac_hw2(60 + ctx) ) {
    if( ctx < 2 )
      ctx = 2;
    else
      ctx = 3;
    val++;
    if(val > 102) //prevent infinite loop
      return INT_MIN;
  }

  if( val&0x01 )
    return (val + 1)/2;
  else
    return -(val + 1)/2;
}

/* hw_qscale[0]:luma8x8; hw_qscale[1]:luma4x4; hw_qscale[2]:chroma */
char hw_qscale[2][3]={{99,99,99},{99,99,99}};
int dblk_left_mv[2][4];
int8_t dblk_left_ref[2][4];
int for_dblk_c;

__cache_text0__ 
int decode_mb_cabac_comp(H264Context *h) {
#ifdef JZC_DBG_WAIT
  int cnt_i = 0;
#endif // JZC_DBG_WAIT

  //#define JZ_DTS_OPT
#ifdef JZ_DTS_OPT
  /* following vars be define in mplayer.c */
  extern g_h264_dts_mode;
  extern g_Dec_flag;
  extern dec_audio_times;
  extern g_audio_ds_packets;
  extern Demux_packet_flag;
  int flag = 0;
#endif
  unsigned int tcsm0_fifo_rp_lh, task_fifo_wp_lh, task_fifo_wp_lh_overlap;
#ifdef JZC_PMON_P0ed
  PMON_ON(wait);
#endif

	  do{
#ifdef JZ_DTS_OPT
	    if(flag && g_h264_dts_mode){
	      if(g_audio_ds_packets){ //have audio can be decode
		      if(g_Dec_flag){
			g_Dec_flag = 1;
			Demux_packet_flag = 0;
			//PMON_ON(parse);
			 int ret = fill_audio_out_buffers();
			 if(g_Dec_flag == 2){ //header be decoded sucessfully
			     g_Dec_flag = 0;
			     dec_audio_times++;
			 }
			 //PMON_OFF(parse);
		      }else{
			//PMON_ON(dmb);
			fill_audio_out_buffers_for_DTS_qmf();
			 g_Dec_flag = 1;
			 //PMON_OFF(dmb);
		      }
	      }
		
	   }
	    flag ^= 0x1;
#endif
	    tcsm0_fifo_rp_lh = (unsigned int)*tcsm0_fifo_rp & 0x0000FFFF;
	    task_fifo_wp_lh = (unsigned int)task_fifo_wp & 0x0000FFFF;
	    //task_fifo_wp_lh_overlap = ((unsigned int)task_fifo_wp + MAX_TASK_LEN) & 0x0000FFFF;
	    task_fifo_wp_lh_overlap = task_fifo_wp_lh + MAX_TASK_LEN;
	#ifdef JZC_DBG_WAIT
	    MpegEncContext * const s = &h->s;
	    cnt_i++;
	    if( (cnt_i%10000000) == 9000000 )
	      mp_msg(NULL,NULL,"p0 waiting p1 at mb_x:%d, mb_y:%d ....\n",s->mb_x,s->mb_y);
	#endif // JZC_DBG_WAIT
	  } while ( !( (task_fifo_wp_lh_overlap <= tcsm0_fifo_rp_lh) || (task_fifo_wp_lh > tcsm0_fifo_rp_lh) ) );

#ifdef JZC_PMON_P0ed
  PMON_OFF(wait);
#endif

#ifdef JZC_PMON_P0ed
  PMON_ON(parse);
#endif

  MpegEncContext * const s = &h->s;
  //  const int mb_xy= s->mb_x + s->mb_y*s->mb_stride;
  //  h->mb_xy=mb_xy;
  const int mb_xy=h->mb_xy;
  const int mb_x=s->mb_x;

  int mb_type, partition_count, cbp = 0;
  int dct8x8_allowed= h->pps.transform_8x8_mode;
  int nnz_16bits_hw_use;

  cabac_hw_started = 0;

  int top_xy  = h->top_mb_xy     = mb_xy  - s->mb_stride;
  int left_xy = h->left_mb_xy = mb_xy - 1;
  
  int slice_num = h->slice_num;
  uint32_t *ptr_mb_type = s->current_picture.mb_type;
  h->top_mb_avail  = h->slice_table[top_xy ] == slice_num;
  h->left_mb_avail = h->slice_table[left_xy] == slice_num;
  h->top_type_normal  = h->top_mb_avail ? ptr_mb_type[top_xy ] : 0;
  h->left_type_normal = h->left_mb_avail ? ptr_mb_type[left_xy] : 0;

  h->slice_table[ mb_xy ]= h->slice_num;

  for_dblk_c = !(slice_num == 1 || h->top_mb_avail);
  //for_dblk_c = !(h->slice_num == 1 || h->slice_table[mb_xy] == h->slice_table[mb_xy-s->mb_stride]);
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
  }

  if( h->slice_type != FF_I_TYPE && h->slice_type != FF_SI_TYPE ) {
    int skip;
    int ctx = 0;
    if( h->left_mb_avail && !IS_SKIP( h->left_type_normal ))
      ctx++;
    if( h->top_mb_avail && !IS_SKIP( h->top_type_normal ))
      ctx++;
    if( h->slice_type == FF_B_TYPE )
      ctx += 13;
    skip = get_cabac_hw2(11+ctx);
    if( skip ) {
      mb_type = decode_mb_cabac_skip(h);
      h->cbp = 0;

      goto fill_task_mv;
      //      return 0;
    }
  }

  if( ( mb_type = decode_cabac_mb_type_hw( h ) ) < 0 ) {
    return -1;
  }

  if( h->slice_type == FF_B_TYPE ) {
    if( mb_type < 23 ){
      partition_count= b_mb_type_info[mb_type].partition_count;
      mb_type=         b_mb_type_info[mb_type].type;
    }else{
      mb_type -= 23;
      goto decode_intra_mb;
    }
  } else if( h->slice_type == FF_P_TYPE ) {
    if( mb_type < 5) {
      partition_count= p_mb_type_info[mb_type].partition_count;
      mb_type=         p_mb_type_info[mb_type].type;
    } else {
      mb_type -= 5;
      goto decode_intra_mb;
    }
  } else {
  decode_intra_mb:
    partition_count = 0;
    cbp= i_mb_type_info[mb_type].cbp;
    h->intra16x16_pred_mode= i_mb_type_info[mb_type].pred_mode;
    mb_type= i_mb_type_info[mb_type].type;
  }


  if(IS_INTRA_PCM(mb_type)) {
    return -1;//hl_decode_intra_pcm(h,mb_type);
  }

  h->neighbor_transform_size= !!IS_8x8DCT(h->top_type_normal) + !!IS_8x8DCT(h->left_type_normal);

  if( IS_INTRA( mb_type ) ) {
#ifdef JZC_PMON_P0ed
  PMON_ON(parse);
#endif

    int i, pred_mode;
    if( IS_INTRA4x4( mb_type ) ) {
      if( dct8x8_allowed && decode_cabac_mb_transform_size_hw( h ) ) {
	mb_type |= MB_TYPE_8x8DCT;
	for( i = 0; i < 16; i+=4 ) {
	  int pred = pred_intra_mode( h, i );
	  int mode = decode_cabac_mb_intra4x4_pred_mode_hw( pred );
	  fill_rectangle( &h->intra4x4_pred_mode_cache[ scan8[i] ], 2, 2, 8, mode, 1 );
	}
      } else {
	for( i = 0; i < 16; i++ ) {
	  int pred = pred_intra_mode( h, i );
	  h->intra4x4_pred_mode_cache[scan8[i]] = decode_cabac_mb_intra4x4_pred_mode_hw( pred );
	}
      }
      //      write_back_intra_pred_mode(h);
#if 1
      h->top_intra4x4_pred_mode[mb_x]=((int*)h->intra4x4_pred_mode_cache)[9];
      h->intra4x4_pred_mode_cache[11]=h->intra4x4_pred_mode_cache[15];
      h->intra4x4_pred_mode_cache[19]=h->intra4x4_pred_mode_cache[23];
      h->intra4x4_pred_mode_cache[27]=h->intra4x4_pred_mode_cache[31];
      h->intra4x4_pred_mode_cache[35]=h->intra4x4_pred_mode_cache[39];
#endif
#if 0
      if( check_intra4x4_pred_mode(h) < 0 ) return -1;
#else
      if( ff_h264_check_intra4x4_pred_mode(h) < 0 ){ 
      	return -1;
      }
#endif 

    } else {
      //h->intra16x16_pred_mode= check_intra_pred_mode( h, h->intra16x16_pred_mode );
      h->intra16x16_pred_mode= ff_h264_check_intra_pred_mode( h, h->intra16x16_pred_mode );
      if( h->intra16x16_pred_mode < 0 ) return -1;
    }

    h->chroma_pred_mode_table[mb_x] =
      pred_mode                        = decode_cabac_mb_chroma_pre_mode_hw( h );

    //pred_mode= check_intra_pred_mode( h, pred_mode );
    pred_mode= ff_h264_check_intra_pred_mode( h, pred_mode );
    if( pred_mode < 0 ) return -1;
    h->chroma_pred_mode= pred_mode;
#ifdef JZC_PMON_P0ed
    PMON_OFF(parse);
#endif

  } else if( partition_count == 4 ) {
    hl_decode_8x8_mb(h, &mb_type, &dct8x8_allowed);
  } else if( IS_DIRECT(mb_type) ) {
    pred_direct_motion_cabac(h, &mb_type);
    dct8x8_allowed &= h->sps.direct_8x8_inference_flag;
  } else {
    //int list, mx, my, i, mpx, mpy;
    if(IS_16X16(mb_type)){
      hl_decode_16x16_mb(h,mb_type);
    }
    else if(IS_16X8(mb_type)){
      hl_decode_16x8_mb(h,mb_type);
    }else{
      hl_decode_8x16_mb(h,mb_type);
    }
  }

  fill_cabac_curr_caches2(h,mb_type);

  if( !IS_INTRA16x16( mb_type ) ) {
    cbp = decode_cabac_mb_cbp_hw(h);
  }

  h->cbp = cbp;

  if( dct8x8_allowed && (cbp&15) && !IS_INTRA( mb_type ) ) {
    if( decode_cabac_mb_transform_size_hw( h ) )
      mb_type |= MB_TYPE_8x8DCT;
  }

fill_task_mv:
  //h->nnz_top_left will be changed during fill_cache_for_dblk, so back up for cabac_hw
  nnz_16bits_hw_use = h->nnz_top_left;
  s->current_picture.mb_type[mb_xy]= mb_type;
  if(for_dblk_c){
    fill_cache_for_dblk(h);
  }

  if(IS_INTRA(mb_type)) {
    task_fifo_res_wp = task_fifo_wp + ((sizeof(H264_MB_Ctrl_DecARGs) + 3) >> 2)
                                    + ((sizeof(H264_MB_Intra_DecARGs) + 3) >> 2);
  } else{
    int *dMB_mvnum = task_fifo_wp + ((sizeof(H264_MB_Ctrl_DecARGs) + 3) >> 2);
    char *dMB_ref = dMB_mvnum+1;
    int *dMB_mvtype = dMB_ref + (8<<(h->slice_type-2));
    int *dMB_mvneib = dMB_mvtype+1;
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
    task_fifo_res_wp = dMB_mvcurr+pos;
    *dMB_mvnum = pos >> (h->slice_type==3);
  }


  if( cbp || IS_INTRA16x16( mb_type ) ) {
    int dqp;
    h->last_qscale_diff = dqp = decode_cabac_mb_dqp_hw( h );
    if( dqp == INT_MIN ){
      return -1;
    }
    s->qscale += dqp;
    if(((unsigned)s->qscale) > 51){
      if(s->qscale<0) s->qscale+= 52;
      else            s->qscale-= 52;
    }
    h->chroma_qp[0] = get_chroma_qp(h, 0, s->qscale);
    h->chroma_qp[1] = get_chroma_qp(h, 1, s->qscale);

    {
      int i;
      int * dq_table_base= (int *)TCSM0_DQ_TABLE_BASE;
      uint8_t * res_out_base = (uint8_t *)(task_fifo_res_wp);

      unsigned int res_dq_addr =
	((((unsigned int)(res_out_base))&0xffff)<<16) |
	(((unsigned int)(dq_table_base))&0xffff);

      CPU_WR_CABAC(6,res_dq_addr);//res_dq_addr

      unsigned int final_cbp = ((h->top_cbp&0x1c0)<<6) | ((h->left_cbp&0x1c0)<<3) | (h->cbp&0x3f);

      CPU_WR_CABAC(5,(nnz_16bits_hw_use<<16)|final_cbp);//nnz and cbp

#ifdef JZC_PMON_P0ed
      PMON_ON(wait);
#endif
      int is_intra = (mb_type & 0x0003)? 1 : 0;
      int is_8x8dct = (mb_type & 0x01000000)>>22;

      int hw_mbtype = (mb_type&0xff)|is_8x8dct;
      CPU_WR_CABAC(4,hw_mbtype);//mb_type

      int dq_update=0;
      int dq40 = (is_intra)?0:3;
      if(is_8x8dct && (hw_qscale[is_intra][0]!=s->qscale)){
	  dq_update|=(is_intra) ? 0x1 : 0x8;
	  hw_qscale[is_intra][0]=s->qscale;
	  int *src=h->dequant8_coeff[!is_intra][s->qscale]-1;
          int *dst = dq_table_base - 1;
          for(i=0; i<16; i++){
	    S32LDI(xr3,src,0x4);
	    S32LDI(xr4,src,0x4);
	    S32LDI(xr5,src,0x4);
	    S32LDI(xr6,src,0x4);
	    S32SDI(xr3,dst,0x4);
	    S32SDI(xr4,dst,0x4);
	    S32SDI(xr5,dst,0x4);
	    S32SDI(xr6,dst,0x4);
          }
	}
	else if(hw_qscale[is_intra][1]!=s->qscale){
	  dq_update|=(is_intra) ? 0x2 : 0x10;
	  hw_qscale[is_intra][1]=s->qscale;
	  int *src=h->dequant4_coeff[dq40][s->qscale]-1;
          int *dst = dq_table_base - 1; 
	  for(i=0;i<4;i++){
	    S32LDI(xr3,src,0x4);
	    S32LDI(xr4,src,0x4);
	    S32LDI(xr5,src,0x4);
	    S32LDI(xr6,src,0x4);
	    S32SDI(xr3,dst,0x4);
	    S32SDI(xr4,dst,0x4);
	    S32SDI(xr5,dst,0x4);
	    S32SDI(xr6,dst,0x4);
          }
	}

	if(hw_qscale[is_intra][2]!=s->qscale){
	  dq_update|=(is_intra) ? 0x4 : 0x20;
	  hw_qscale[is_intra][2]=s->qscale;
	  int *src1=h->dequant4_coeff[dq40+1][h->chroma_qp[0]]-1;
	  int *src2=h->dequant4_coeff[dq40+2][h->chroma_qp[1]]-1;
          int *dst1 = dq_table_base + 63;
          int *dst2 = dst1 + 16;
          for(i=0;i<8;i++){
	    S32LDI(xr3,src1,0x4);
	    S32LDI(xr4,src1,0x4);
	    S32LDI(xr5,src2,0x4);
	    S32LDI(xr6,src2,0x4);
	    S32SDI(xr3,dst1,0x4);
	    S32SDI(xr4,dst1,0x4);
	    S32SDI(xr5,dst2,0x4);
	    S32SDI(xr6,dst2,0x4);
	  }
	}

#ifdef JZC_PMON_P0ed
      PMON_OFF(wait);
#endif

      CPU_SET_CTRL(0, dq_update, 0, 0, 0, 1, 0, 1, 0, 0);//residual_en

      cabac_hw_started = 1;
    }

  } else {
    h->last_qscale_diff = 0;
  }
  s->current_picture.qscale_table[mb_xy]= s->qscale;

#ifdef JZC_PMON_P0ed
  PMON_OFF(parse);
#endif
  return 0;
}

__cache_text0__ 
void hl_decode_cabac_mb(H264Context *h) {
  MpegEncContext * const s = &h->s;
  const int mb_x= s->mb_x;
  const int mb_y= s->mb_y;
  const int mb_xy= h->mb_xy;
  const int top_xy = h->top_mb_xy;

  const int mb_type= s->current_picture.mb_type[mb_xy];
  int i;

  mv_top_left_atm[0]=mv_hat_atm[mb_x][0][3];
  mv_top_left_atm[1]=mv_hat_atm[mb_x][1][3];
  ref_top_left_atm[0]=ref_hat_atm[mb_x][0]>>24;
  ref_top_left_atm[1]=ref_hat_atm[mb_x][1]>>24;
  if( IS_INTER( mb_type ) ) {
    h->chroma_pred_mode_table[mb_x] = 0;
    write_back_motion_cabac( h, mb_type );
  }else if (IS_INTRA4x4( mb_type )){
    //h->top_intra4x4_pred_mode[mb_x]=((int*)h->intra4x4_pred_mode_cache)[9];
  }

  {
    H264_MB_Ctrl_DecARGs * dMB_Ctrl = task_fifo_wp;
    H264_MB_Intra_DecARGs * dMB_Intra;
    //H264_MB_InterP_DecARGs * dMB_InterP;
    //H264_MB_InterB_DecARGs * dMB_InterB;

    if(h->new_slice_p0){
      dMB_Ctrl->new_slice = 1;
      h->new_slice_p0 = 0;
    }else{
      dMB_Ctrl->new_slice = 0;
    }

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

    /*---- dMB control part constant length ----*/
    if(IS_INTRA(mb_type)) {
      dMB_Intra = task_fifo_wp + ((sizeof(H264_MB_Ctrl_DecARGs) + 3) >> 2);

      dMB_Intra->chroma_pred_mode = (uint8_t)h->chroma_pred_mode;
      dMB_Intra->intra16x16_pred_mode = (uint8_t)h->intra16x16_pred_mode;
      dMB_Intra->topleft_samples_available = h->topleft_samples_available;
      dMB_Intra->topright_samples_available = h->topright_samples_available;

#if 1
      uint32_t intra4x4_pred_mode = 0;
      for(i=0; i<8; i++)
	intra4x4_pred_mode |= h->intra4x4_pred_mode_cache[scan8[i]] << 4*i;
      dMB_Intra->intra4x4_pred_mode0 = intra4x4_pred_mode;
      intra4x4_pred_mode = 0;
      for(i=8; i<16; i++)
	intra4x4_pred_mode |= h->intra4x4_pred_mode_cache[scan8[i]] << 4*(i-8);
      dMB_Intra->intra4x4_pred_mode1 = intra4x4_pred_mode;
#else
      dMB_Intra->intra4x4_pred_mode1 = (h->intra4x4_pred_mode_cache[scan8[8]] | 
					(h->intra4x4_pred_mode_cache[scan8[1+8]] << 4)|
					(h->intra4x4_pred_mode_cache[scan8[2+8]] << 8)|
					(h->intra4x4_pred_mode_cache[scan8[3+8]] << 12)|
					(h->intra4x4_pred_mode_cache[scan8[4+8]] << 16)|
					(h->intra4x4_pred_mode_cache[scan8[5+8]] << 20)|
					(h->intra4x4_pred_mode_cache[scan8[6+8]] << 24)|
					(h->intra4x4_pred_mode_cache[scan8[7+8]] << 28)
					);
      dMB_Intra->intra4x4_pred_mode0 = (h->intra4x4_pred_mode_cache[scan8[0]] | 
				       (h->intra4x4_pred_mode_cache[scan8[1]] << 4)|
				       (h->intra4x4_pred_mode_cache[scan8[2]] << 8)|
				       (h->intra4x4_pred_mode_cache[scan8[3]] << 12)|
				       (h->intra4x4_pred_mode_cache[scan8[4]] << 16)|
				       (h->intra4x4_pred_mode_cache[scan8[5]] << 20)|
				       (h->intra4x4_pred_mode_cache[scan8[6]] << 24)|
				       (h->intra4x4_pred_mode_cache[scan8[7]] << 28)
				       );
#endif

    } else if (IS_8X8(mb_type)){
      dMB_Ctrl->sub_mb_type[0] = h->sub_mb_type[0];
      dMB_Ctrl->sub_mb_type[1] = h->sub_mb_type[1];
      dMB_Ctrl->sub_mb_type[2] = h->sub_mb_type[2];
      dMB_Ctrl->sub_mb_type[3] = h->sub_mb_type[3];
    }

    task_fifo_wp = task_fifo_res_wp;
    dMB_Ctrl->ctrl_len = (unsigned int)(task_fifo_wp - task_fifo_wp_d1) << 2;

    dMB_Ctrl->nnz_tl_8bits = (h->nnz_top_left&0xf)|((h->nnz_top_left&0xf00)>>4);

    fill_cabac_next_caches(h,0);

    /* cabac residual veriable length part*/    
    if (cabac_hw_started) {
      CPU_POLL_RES_DEC_DONE();//poll residual_done
      CPU_POLL_RES_OUT_DONE();//poll dout_done

      uint32_t * res_out_base = task_fifo_wp;//(uint32_t *)(TCSM0_RES_OUT_BASE);
      int hw_total_coeff = (int)(*(uint16_t *)(((uint32_t)res_out_base)+30));
      int cabac_total_len = 8 + ((hw_total_coeff + 3) >> 2) + ((hw_total_coeff + 1) >> 1);

#if 0
      if(for_dblk_c) {//FIXME: is it a bug?
	uint8_t *hw_nnz = (uint8_t *)res_out_base;
	hw_nnz[27] = (h->nnz_top_left&0xf)|((h->nnz_top_left&0xf00)>>4);
      }
#endif      

      /*---- cabac residual veriable length ----*/
      task_fifo_wp += cabac_total_len;

      //write_back_non_zero_count;
      h->nnz_bottom_right_table[mb_x]=((short*)res_out_base)[14];
      h->cbp= (CPU_RD_CABAC(CABAC_CPU_IF_NNZCBP)&0x1ff);
    } else {
      h->nnz_bottom_right_table[mb_x]=0;

      //*task_fifo_wp = (h->nnz_top_left&0xf)|((h->nnz_top_left&0xf00)>>4);
      //task_fifo_wp += 1;
      //dMB_Ctrl->nnz_tl_8bits = (h->nnz_top_left&0xf)|((h->nnz_top_left&0xf00)>>4);
    }
    h->cbp_table[mb_x] = h->cbp;

    //int current_mb_len = (unsigned int)(task_fifo_wp - task_fifo_wp_d1) << 2;
    //*(uint16_t *)task_fifo_wp_d2 = current_mb_len;
    ((H264_MB_Ctrl_DecARGs *)task_fifo_wp_d2)->next_mb_len = (unsigned int)(task_fifo_wp-task_fifo_wp_d1) << 2;

    if ( mb_xy == 0)
      *(int *)(TCSM1_VUCADDR(TCSM1_FIRST_MBLEN)) = ((H264_MB_Ctrl_DecARGs *)task_fifo_wp_d2)->next_mb_len;
    //else
    //(*tcsm1_fifo_wp)=mb_xy-mb_y;//because mb_stride=mb_width+1
    (*tcsm1_fifo_wp)++;

    //int reach_tcsm0_end = ((unsigned int)task_fifo_wp + MAX_TASK_LEN) > 0xF4004000;
    //if (reach_tcsm0_end)
    if(((unsigned int)task_fifo_wp + MAX_TASK_LEN) > 0xF4004000){//reach_tcsm0_end
      task_fifo_wp = (int *)TCSM0_TASK_FIFO;
    }

    task_fifo_wp_d2 = task_fifo_wp_d1;
    task_fifo_wp_d1 = task_fifo_wp;
  }
}

#if 0
int hl_decode_intra_pcm(H264Context * const h,int mb_type)
{
  MpegEncContext * const s = &h->s;
  const int mb_xy =   s->mb_x +   s->mb_y*s->mb_stride;
  const uint8_t *ptr;
  unsigned int x, y;
  
  // We assume these blocks are very rare so we do not optimize it.
  // FIXME The two following lines get the bitstream position in the cabac
  // decode, I think it should be done by a function in cabac.h (or cabac.c).
  ptr= h->cabac.bytestream;
  if(h->cabac.low&0x1) ptr--;
  if(CABAC_BITS==16){
    if(h->cabac.low&0x1FF) ptr--;
  }

  // The pixels are stored in the same order as levels in h->mb array.
  for(y=0; y<16; y++){
    const int index= 4*(y&3) + 32*((y>>2)&1) + 128*(y>>3);
    for(x=0; x<16; x++){
      tprintf(s->avctx, "LUMA ICPM LEVEL (%3d)\n", *ptr);
      h->mb[index + (x&3) + 16*((x>>2)&1) + 64*(x>>3)]= *ptr++;
    }
  }
  for(y=0; y<8; y++){
    const int index= 256 + 4*(y&3) + 32*(y>>2);
    for(x=0; x<8; x++){
      tprintf(s->avctx, "CHROMA U ICPM LEVEL (%3d)\n", *ptr);
      h->mb[index + (x&3) + 16*(x>>2)]= *ptr++;
    }
  }
  for(y=0; y<8; y++){
    const int index= 256 + 64 + 4*(y&3) + 32*(y>>2);
    for(x=0; x<8; x++){
      tprintf(s->avctx, "CHROMA V ICPM LEVEL (%3d)\n", *ptr);
      h->mb[index + (x&3) + 16*(x>>2)]= *ptr++;
    }
  }

  ff_init_cabac_decoder(&h->cabac, ptr, h->cabac.bytestream_end - ptr);

  // All blocks are present
  h->cbp_table[s->mb_x] = 0x1ef;
  h->chroma_pred_mode_table[s->mb_x] = 0;
  // In deblocking, the quantizer is 0
  s->current_picture.qscale_table[mb_xy]= 0;
  h->chroma_qp[0] = get_chroma_qp(h, 0, 0);
  h->chroma_qp[1] = get_chroma_qp(h, 1, 0);
  // All coeffs are present
  //memset(h->non_zero_count[mb_xy], 16, 16);
  s->current_picture.mb_type[mb_xy]= mb_type;
  return 0;
}
#endif

av_noinline void fill_cache_for_dblk(H264Context * const h){
  MpegEncContext * const s = &h->s;
  const int mb_xy=h->mb_xy;
  {
    int top_xy, left_xy;
    int topleft_type, top_type, topright_type, left_type;

    top_xy     = mb_xy  - (s->mb_stride << FIELD_PICTURE);
    left_xy = mb_xy-1;

    topleft_type = 0;
    topright_type = 0;
    top_type     = h->slice_table[top_xy     ] < 255 ? s->current_picture.mb_type[top_xy]     : 0;
    left_type = h->slice_table[left_xy ] < 255 ? s->current_picture.mb_type[left_xy] : 0;

    if(top_type){
      h->nnz_top_left=(h->nnz_top_left&0xff00)|(h->nnz_bottom_right_table[s->mb_x]&0xff);
    }

    if(left_type){
      h->nnz_top_left=(h->nnz_top_left&0xff)|(h->nnz_bottom_right_table[s->mb_x-1]&0xff00);
    }

    int list;
    for(list=0; list<h->list_count; list++){
      if(USES_LIST(top_type, list)){
	//const int b8_xy= h->mb2b8_xy[top_xy] + h->b8_stride;
	*(uint32_t*)h->mv_cache[list][12+0-8]= mv_hat_atm[s->mb_x][list][0];
	*(uint32_t*)h->mv_cache[list][12+1-8]= mv_hat_atm[s->mb_x][list][1];
	*(uint32_t*)h->mv_cache[list][12+2-8]= mv_hat_atm[s->mb_x][list][2];
	*(uint32_t*)h->mv_cache[list][12+3-8]= mv_hat_atm[s->mb_x][list][3];

	((int*)h->ref_cache[list])[1]=ref_hat_atm[s->mb_x][list];
      }
      if(USES_LIST(left_type, list)){
	//const int b8_xy= h->mb2b8_xy[left_xy] + 1;
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

void pred_direct_motion_cabac_L3dot0_below(H264Context * const h, int *mb_type){
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

    tprintf(s->avctx, "mb_type = %08x, sub_mb_type = %08x, is_b8x8 = %d, mb_type_col = %08x\n", *mb_type, sub_mb_type, is_b8x8, mb_type_col);

    int is_b16x16=IS_16X16(*mb_type);
    int is_col_intra=IS_INTRA(mb_type_col);
    if(h->direct_spatial_mv_pred){
        int ref[2];
        int32_t mv[2];
        int list;

        /* FIXME interlacing + spatial direct uses wrong colocated block positions */

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
		  mv[list]=pred_motion_cabac(h, 0, 4, list, ref[list]);
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
               && (   (l1ref0[0] == 0 && colZeroFlag(l1mv0))
		      || (l1ref0[0]  < 0 && l1ref1[0] == 0 && colZeroFlag(l1mv1)
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
		      if(colZeroFlag(mv_col)){
                            if(ref[0] == 0)
                                fill_rectangle(&h->mv_cache[0][scan8[i4_base]], 2, 2, 8, 0, 4);
                            if(ref[1] == 0)
                                fill_rectangle(&h->mv_cache[1][scan8[i4_base]], 2, 2, 8, 0, 4);
                        }
                    }else
                    for(i4=0; i4<4; i4++){
		      int blk4=i4_base+i4;
                        const int16_t *mv_col = l1mv[b4_offset[blk4]];
                        if(colZeroFlag(mv_col)){
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
		S32LDDV(xr1,dist_scale_factor,ref0,2);
		const int16_t *mv_col = l1ref0[0] >= 0 ? l1mv0[0] : l1mv1[0];
		S32LDD(xr2,mv_col,0);//xr2=mv_col
		Q16SLL(xr3,xr2,xr0,xr0,3);//mv <<3, scale<<4, d16_mulf <<1,overall shift 8bit
		D16MULF_LW(xr4,xr1,xr3);//xr4=mvl0
                ref= ref0;
		Q16ADD_SS_WW(xr5,xr4,xr2,xr0);//xr5=mvl0-mv_col
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
		S32LDDV(xr1,dist_scale_factor,ref0,2);

                fill_rectangle(&h->ref_cache[0][scan8[i4_base]], 2, 2, 8, ref0, 1);
                if(IS_SUB_8X8(sub_mb_type)){
		    const int16_t *mv_col = l1mv[b8x3_offset[i8]];
		    S32LDD(xr2,mv_col,0);//xr2=mv_col
		    Q16SLL(xr3,xr2,xr0,xr0,3);//mv <<3, scale<<4, d16_mulf <<1,overall shift 8bit
		    D16MULF_LW(xr4,xr1,xr3);//xr4=mvl0
		    Q16ADD_SS_WW(xr5,xr4,xr2,xr0);//xr5=mvl0-mv_col
		    int mv0=S32M2I(xr4);
		    int mv1=S32M2I(xr5);
                    fill_rectangle(&h->mv_cache[0][scan8[i4_base]], 2, 2, 8, mv0, 4);
                    fill_rectangle(&h->mv_cache[1][scan8[i4_base]], 2, 2, 8, mv1, 4);
                }else
                for(i4=0; i4<4; i4++){
		  int blk4=i4_base+i4;
		    const int16_t *mv_col = l1mv[b4_offset[blk4]];
		    S32LDD(xr2,mv_col,0);//xr2=mv_col
		    Q16SLL(xr3,xr2,xr0,xr0,3);//mv <<3, scale<<4, d16_mulf <<1,overall shift 8bit
		    D16MULF_LW(xr4,xr1,xr3);//xr4=mvl0
		    Q16ADD_SS_WW(xr5,xr4,xr2,xr0);//xr5=mvl0-mv_col
		    *(uint32_t*)h->mv_cache[0][scan8[blk4]] = S32M2I(xr4);
		    *(uint32_t*)h->mv_cache[1][scan8[blk4]] = S32M2I(xr5);
                }
            }
        }
    }
}

void write_back_motion_cabac_L3dot0_below(H264Context *h, int mb_type){
    MpegEncContext * const s = &h->s;
    int mb_x=s->mb_x;
    const int b_xy = 4*mb_x + 4*s->mb_y*h->b_stride;
    const int b8_xy= 2*mb_x + 2*s->mb_y*h->b8_stride;
    int list;

    if(!USES_LIST(mb_type, 0))
        fill_rectangle(&s->current_picture.ref_index[0][b8_xy], 2, 2, h->b8_stride, (uint8_t)LIST_NOT_USED, 1);

    int (*mvd_ha)[4]=mvd_hat_atm[mb_x];
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

#define MB_TYPE_SKIP_OR_DIRECT (MB_TYPE_SKIP|MB_TYPE_DIRECT2)
	if(mb_type&MB_TYPE_SKIP_OR_DIRECT){
	  *(uint32_t*)h->mvd_cache[list][15]=*(uint32_t*)h->mvd_cache[list][23]=
	    *(uint32_t*)h->mvd_cache[list][31]=*(uint32_t*)h->mvd_cache[list][39]=0;
	  mvd_ha[list][0]=mvd_ha[list][1]=
	    mvd_ha[list][2]=mvd_ha[list][3]=0;
	}
	else{
	  mvd_ha[list][0]=*(uint32_t*)h->mvd_cache[list][12+0 + 8*3];
	  mvd_ha[list][1]=*(uint32_t*)h->mvd_cache[list][12+1 + 8*3];
	  mvd_ha[list][2]=*(uint32_t*)h->mvd_cache[list][12+2 + 8*3];
	  mvd_ha[list][3]=*(uint32_t*)h->mvd_cache[list][12+3 + 8*3];
	}

        {
            int8_t *ref_index = &s->current_picture.ref_index[list][b8_xy];
            ref_index[0+0*h->b8_stride]= h->ref_cache[list][12];
            ref_index[1+0*h->b8_stride]= h->ref_cache[list][14];
            ref_index[0+1*h->b8_stride]= h->ref_cache[list][28];
            ref_index[1+1*h->b8_stride]= h->ref_cache[list][30];
	    ref_hat_atm[mb_x][list]=((int*)(h->ref_cache[list]))[9];
        }
    }

    direct_hat_atm[mb_x]=((short*)h->direct_slim_cache)[5];
}
