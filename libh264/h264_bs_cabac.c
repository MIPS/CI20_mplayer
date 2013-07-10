#define CABAC 1

#include "internal.h"
#include "dsputil.h"
#include "avcodec.h"
#include "h264.h"
#include "h264data.h"
#include "h264_mvpred.h"
#include "golomb.h"

#include "cabac.h"
#if ARCH_X86
#include "x86/h264_i386.h"
#endif

//#undef NDEBUG
#include <assert.h>
#undef printf

#ifdef JZC_GATHER_OPT
extern unsigned char sde_luma_neighbor;
extern unsigned char sde_chroma_neighbor;
extern unsigned int sde_res_index;
extern unsigned int sde_res_di;
extern short sde_res_dct[408];
extern short sde_tmp_blk[64];
extern int sde_nnz_dct[27];
extern short sde_v_dc[4];
extern const int y_dc_table[16];
#endif

static inline int get_chroma_qp_hw(H264Context *h, int t, int qscale){
    return h->pps.chroma_qp_table[t][qscale & 0xff];
}


static void predict_field_decoding_flag_hw(H264Context *h){
    MpegEncContext * const s = &h->s;
    const int mb_xy= s->mb_x + s->mb_y*s->mb_stride;
    int mb_type = (h->hw_slice_table[mb_xy-1] == h->slice_num)
                ? s->current_picture.mb_type[mb_xy-1]
                : (h->hw_slice_table[mb_xy-s->mb_stride] == h->slice_num)
                ? s->current_picture.mb_type[mb_xy-s->mb_stride]
                : 0;
    h->mb_mbaff = h->mb_field_decoding_flag = IS_INTERLACED(mb_type) ? 1 : 0;
}

static inline int get_cabac_cbf_ctx_hw( H264Context *h, int cat, int idx ) {
    int nza, nzb;
    int ctx = 0;

    if( cat == 0 ) {
        nza = h->left_cbp&0x100;
        nzb = h-> top_cbp&0x100;
    } else if( cat == 1 || cat == 2 ) {
        nza = h->non_zero_count_cache[scan8[idx] - 1];
        nzb = h->non_zero_count_cache[scan8[idx] - 8];
    } else if( cat == 3 ) {
        nza = (h->left_cbp>>(6+idx))&0x01;
        nzb = (h-> top_cbp>>(6+idx))&0x01;
    } else {
        assert(cat == 4);
        nza = h->non_zero_count_cache[scan8[16+idx] - 1];
        nzb = h->non_zero_count_cache[scan8[16+idx] - 8];
    }

    if( nza > 0 )
        ctx++;

    if( nzb > 0 )
        ctx += 2;

    return ctx + 4 * cat;
}


void fill_caches_hw(H264Context *h, int mb_type, int for_deblock){
    MpegEncContext * const s = &h->s;
    const int mb_xy= s->mb_x + s->mb_y*s->mb_stride;
    int topleft_xy, top_xy, topright_xy, left_xy[2];
    int topleft_type, top_type, topright_type, left_type[2];
    int left_block[8];
    int i;

    //FIXME deblocking could skip the intra and nnz parts.
    if(for_deblock && (h->slice_num == 1 || h->hw_slice_table[mb_xy] == h->hw_slice_table[mb_xy-s->mb_stride]) && !FRAME_MBAFF)
        return;

    //wow what a mess, why didn't they simplify the interlacing&intra stuff, i can't imagine that these complex rules are worth it

    top_xy     = mb_xy  - (s->mb_stride << FIELD_PICTURE);
    topleft_xy = top_xy - 1;
    topright_xy= top_xy + 1;
    left_xy[1] = left_xy[0] = mb_xy-1;
    left_block[0]= 0;
    left_block[1]= 1;
    left_block[2]= 2;
    left_block[3]= 3;
    left_block[4]= 7;
    left_block[5]= 10;
    left_block[6]= 8;
    left_block[7]= 11;
    if(FRAME_MBAFF){
        const int pair_xy          = s->mb_x     + (s->mb_y & ~1)*s->mb_stride;
        const int top_pair_xy      = pair_xy     - s->mb_stride;
        const int topleft_pair_xy  = top_pair_xy - 1;
        const int topright_pair_xy = top_pair_xy + 1;
        const int topleft_mb_frame_flag  = !IS_INTERLACED(s->current_picture.mb_type[topleft_pair_xy]);
        const int top_mb_frame_flag      = !IS_INTERLACED(s->current_picture.mb_type[top_pair_xy]);
        const int topright_mb_frame_flag = !IS_INTERLACED(s->current_picture.mb_type[topright_pair_xy]);
        const int left_mb_frame_flag = !IS_INTERLACED(s->current_picture.mb_type[pair_xy-1]);
        const int curr_mb_frame_flag = !IS_INTERLACED(mb_type);
        const int bottom = (s->mb_y & 1);
        tprintf(s->avctx, "fill_caches: curr_mb_frame_flag:%d, left_mb_frame_flag:%d, topleft_mb_frame_flag:%d, top_mb_frame_flag:%d, topright_mb_frame_flag:%d\n", curr_mb_frame_flag, left_mb_frame_flag, topleft_mb_frame_flag, top_mb_frame_flag, topright_mb_frame_flag);
        if (bottom
                ? !curr_mb_frame_flag // bottom macroblock
                : (!curr_mb_frame_flag && !top_mb_frame_flag) // top macroblock
                ) {
            top_xy -= s->mb_stride;
        }
        if (bottom
                ? !curr_mb_frame_flag // bottom macroblock
                : (!curr_mb_frame_flag && !topleft_mb_frame_flag) // top macroblock
                ) {
            topleft_xy -= s->mb_stride;
        }
        if (bottom
                ? !curr_mb_frame_flag // bottom macroblock
                : (!curr_mb_frame_flag && !topright_mb_frame_flag) // top macroblock
                ) {
            topright_xy -= s->mb_stride;
        }
        if (left_mb_frame_flag != curr_mb_frame_flag) {
            left_xy[1] = left_xy[0] = pair_xy - 1;
            if (curr_mb_frame_flag) {
                if (bottom) {
                    left_block[0]= 2;
                    left_block[1]= 2;
                    left_block[2]= 3;
                    left_block[3]= 3;
                    left_block[4]= 8;
                    left_block[5]= 11;
                    left_block[6]= 8;
                    left_block[7]= 11;
                } else {
                    left_block[0]= 0;
                    left_block[1]= 0;
                    left_block[2]= 1;
                    left_block[3]= 1;
                    left_block[4]= 7;
                    left_block[5]= 10;
                    left_block[6]= 7;
                    left_block[7]= 10;
                }
            } else {
                left_xy[1] += s->mb_stride;
                //left_block[0]= 0;
                left_block[1]= 2;
                left_block[2]= 0;
                left_block[3]= 2;
                //left_block[4]= 7;
                left_block[5]= 10;
                left_block[6]= 7;
                left_block[7]= 10;
            }
        }
    }

    h->top_mb_xy = top_xy;
    h->left_mb_xy[0] = left_xy[0];
    h->left_mb_xy[1] = left_xy[1];

    if(for_deblock){
        topleft_type = 0;
        topright_type = 0;
        top_type     = h->hw_slice_table[top_xy     ] < 255 ? s->current_picture.mb_type[top_xy]     : 0;
        left_type[0] = h->hw_slice_table[left_xy[0] ] < 255 ? s->current_picture.mb_type[left_xy[0]] : 0;
        left_type[1] = h->hw_slice_table[left_xy[1] ] < 255 ? s->current_picture.mb_type[left_xy[1]] : 0;

        if(FRAME_MBAFF && !IS_INTRA(mb_type)){
            int list;
            int v = *(uint16_t*)&h->non_zero_count[mb_xy][14];
            for(i=0; i<16; i++)
                h->non_zero_count_cache[scan8[i]] = (v>>i)&1;
            for(list=0; list<h->list_count; list++){
                if(USES_LIST(mb_type,list)){
                    uint32_t *src = (uint32_t*)s->current_picture.motion_val[list][h->mb2b_xy[mb_xy]];
                    uint32_t *dst = (uint32_t*)h->mv_cache[list][scan8[0]];
                    int8_t *ref = &s->current_picture.ref_index[list][h->mb2b8_xy[mb_xy]];
                    for(i=0; i<4; i++, dst+=8, src+=h->b_stride){
                        dst[0] = src[0];
                        dst[1] = src[1];
                        dst[2] = src[2];
                        dst[3] = src[3];
                    }
                    *(uint32_t*)&h->ref_cache[list][scan8[ 0]] =
                    *(uint32_t*)&h->ref_cache[list][scan8[ 2]] = pack16to32(ref[0],ref[1])*0x0101;
                    ref += h->b8_stride;
                    *(uint32_t*)&h->ref_cache[list][scan8[ 8]] =
                    *(uint32_t*)&h->ref_cache[list][scan8[10]] = pack16to32(ref[0],ref[1])*0x0101;
                }else{
                    fill_rectangle_hw(&h-> mv_cache[list][scan8[ 0]], 4, 4, 8, 0, 4);
                    fill_rectangle_hw(&h->ref_cache[list][scan8[ 0]], 4, 4, 8, (uint8_t)LIST_NOT_USED, 1);
                }
            }
        }
    }else{
        topleft_type = h->hw_slice_table[topleft_xy ] == h->slice_num ? s->current_picture.mb_type[topleft_xy] : 0;
        top_type     = h->hw_slice_table[top_xy     ] == h->slice_num ? s->current_picture.mb_type[top_xy]     : 0;
        topright_type= h->hw_slice_table[topright_xy] == h->slice_num ? s->current_picture.mb_type[topright_xy]: 0;
        left_type[0] = h->hw_slice_table[left_xy[0] ] == h->slice_num ? s->current_picture.mb_type[left_xy[0]] : 0;
        left_type[1] = h->hw_slice_table[left_xy[1] ] == h->slice_num ? s->current_picture.mb_type[left_xy[1]] : 0;
    }

    if(IS_INTRA(mb_type)){
        h->topleft_samples_available=
        h->top_samples_available=
        h->left_samples_available= 0xFFFF;
        h->topright_samples_available= 0xEEEA;

        if(!IS_INTRA(top_type) && (top_type==0 || h->pps.constrained_intra_pred)){
            h->topleft_samples_available= 0xB3FF;
            h->top_samples_available= 0x33FF;
            h->topright_samples_available= 0x26EA;
        }
        for(i=0; i<2; i++){
            if(!IS_INTRA(left_type[i]) && (left_type[i]==0 || h->pps.constrained_intra_pred)){
                h->topleft_samples_available&= 0xDF5F;
                h->left_samples_available&= 0x5F5F;
            }
        }

        if(!IS_INTRA(topleft_type) && (topleft_type==0 || h->pps.constrained_intra_pred))
            h->topleft_samples_available&= 0x7FFF;

        if(!IS_INTRA(topright_type) && (topright_type==0 || h->pps.constrained_intra_pred))
            h->topright_samples_available&= 0xFBFF;

        if(IS_INTRA4x4(mb_type)){
            if(IS_INTRA4x4(top_type)){
                h->intra4x4_pred_mode_cache[4+8*0]= h->intra4x4_pred_mode_hw[top_xy][4];
                h->intra4x4_pred_mode_cache[5+8*0]= h->intra4x4_pred_mode_hw[top_xy][5];
                h->intra4x4_pred_mode_cache[6+8*0]= h->intra4x4_pred_mode_hw[top_xy][6];
                h->intra4x4_pred_mode_cache[7+8*0]= h->intra4x4_pred_mode_hw[top_xy][3];
            }else{
                int pred;
                if(!top_type || (IS_INTER(top_type) && h->pps.constrained_intra_pred))
                    pred= -1;
                else{
                    pred= 2;
                }
                h->intra4x4_pred_mode_cache[4+8*0]=
                h->intra4x4_pred_mode_cache[5+8*0]=
                h->intra4x4_pred_mode_cache[6+8*0]=
                h->intra4x4_pred_mode_cache[7+8*0]= pred;
            }
            for(i=0; i<2; i++){
                if(IS_INTRA4x4(left_type[i])){
                    h->intra4x4_pred_mode_cache[3+8*1 + 2*8*i]= h->intra4x4_pred_mode_hw[left_xy[i]][left_block[0+2*i]];
                    h->intra4x4_pred_mode_cache[3+8*2 + 2*8*i]= h->intra4x4_pred_mode_hw[left_xy[i]][left_block[1+2*i]];
                }else{
                    int pred;
                    if(!left_type[i] || (IS_INTER(left_type[i]) && h->pps.constrained_intra_pred))
                        pred= -1;
                    else{
                        pred= 2;
                    }
                    h->intra4x4_pred_mode_cache[3+8*1 + 2*8*i]=
                    h->intra4x4_pred_mode_cache[3+8*2 + 2*8*i]= pred;
                }
            }
        }
    }


/*
0 . T T. T T T T
1 L . .L . . . .
2 L . .L . . . .
3 . T TL . . . .
4 L . .L . . . .
5 L . .. . . . .
*/
//FIXME constraint_intra_pred & partitioning & nnz (lets hope this is just a typo in the spec)
    if(top_type){
        h->non_zero_count_cache[4+8*0]= h->non_zero_count[top_xy][4];
        h->non_zero_count_cache[5+8*0]= h->non_zero_count[top_xy][5];
        h->non_zero_count_cache[6+8*0]= h->non_zero_count[top_xy][6];
        h->non_zero_count_cache[7+8*0]= h->non_zero_count[top_xy][3];

        h->non_zero_count_cache[1+8*0]= h->non_zero_count[top_xy][9];
        h->non_zero_count_cache[2+8*0]= h->non_zero_count[top_xy][8];

        h->non_zero_count_cache[1+8*3]= h->non_zero_count[top_xy][12];
        h->non_zero_count_cache[2+8*3]= h->non_zero_count[top_xy][11];

    }else{
        h->non_zero_count_cache[4+8*0]=
        h->non_zero_count_cache[5+8*0]=
        h->non_zero_count_cache[6+8*0]=
        h->non_zero_count_cache[7+8*0]=

        h->non_zero_count_cache[1+8*0]=
        h->non_zero_count_cache[2+8*0]=

        h->non_zero_count_cache[1+8*3]=
        h->non_zero_count_cache[2+8*3]= h->pps.cabac && !IS_INTRA(mb_type) ? 0 : 64;

    }

    for (i=0; i<2; i++) {
        if(left_type[i]){
            h->non_zero_count_cache[3+8*1 + 2*8*i]= h->non_zero_count[left_xy[i]][left_block[0+2*i]];
            h->non_zero_count_cache[3+8*2 + 2*8*i]= h->non_zero_count[left_xy[i]][left_block[1+2*i]];
            h->non_zero_count_cache[0+8*1 +   8*i]= h->non_zero_count[left_xy[i]][left_block[4+2*i]];
            h->non_zero_count_cache[0+8*4 +   8*i]= h->non_zero_count[left_xy[i]][left_block[5+2*i]];
        }else{
            h->non_zero_count_cache[3+8*1 + 2*8*i]=
            h->non_zero_count_cache[3+8*2 + 2*8*i]=
            h->non_zero_count_cache[0+8*1 +   8*i]=
            h->non_zero_count_cache[0+8*4 +   8*i]= h->pps.cabac && !IS_INTRA(mb_type) ? 0 : 64;
        }
    }

    if( h->pps.cabac ) {
        // top_cbp
        if(top_type) {
            h->top_cbp = h->cbp_table[top_xy];
        } else if(IS_INTRA(mb_type)) {
            h->top_cbp = 0x1C0;
        } else {
            h->top_cbp = 0;
        }
        // left_cbp
        if (left_type[0]) {
            h->left_cbp = h->cbp_table[left_xy[0]] & 0x1f0;
        } else if(IS_INTRA(mb_type)) {
            h->left_cbp = 0x1C0;
        } else {
            h->left_cbp = 0;
        }
        if (left_type[0]) {
            h->left_cbp |= ((h->cbp_table[left_xy[0]]>>((left_block[0]&(~1))+1))&0x1) << 1;
        }
        if (left_type[1]) {
            h->left_cbp |= ((h->cbp_table[left_xy[1]]>>((left_block[2]&(~1))+1))&0x1) << 3;
        }
    }

#if 1
    if(IS_INTER(mb_type) || IS_DIRECT(mb_type)){
        int list;
        for(list=0; list<h->list_count; list++){
            if(!USES_LIST(mb_type, list) && !IS_DIRECT(mb_type) && !h->deblocking_filter){
                /*if(!h->mv_cache_clean[list]){
                    memset(h->mv_cache [list],  0, 8*5*2*sizeof(int16_t)); //FIXME clean only input? clean at all?
                    memset(h->ref_cache[list], PART_NOT_AVAILABLE, 8*5*sizeof(int8_t));
                    h->mv_cache_clean[list]= 1;
                }*/
                continue;
            }
            h->mv_cache_clean[list]= 0;

            if(USES_LIST(top_type, list)){
                const int b_xy= h->mb2b_xy[top_xy] + 3*h->b_stride;
                const int b8_xy= h->mb2b8_xy[top_xy] + h->b8_stride;
                *(uint32_t*)h->mv_cache[list][scan8[0] + 0 - 1*8]= *(uint32_t*)s->current_picture.motion_val[list][b_xy + 0];
                *(uint32_t*)h->mv_cache[list][scan8[0] + 1 - 1*8]= *(uint32_t*)s->current_picture.motion_val[list][b_xy + 1];
                *(uint32_t*)h->mv_cache[list][scan8[0] + 2 - 1*8]= *(uint32_t*)s->current_picture.motion_val[list][b_xy + 2];
                *(uint32_t*)h->mv_cache[list][scan8[0] + 3 - 1*8]= *(uint32_t*)s->current_picture.motion_val[list][b_xy + 3];
                h->ref_cache[list][scan8[0] + 0 - 1*8]=
                h->ref_cache[list][scan8[0] + 1 - 1*8]= s->current_picture.ref_index[list][b8_xy + 0];
                h->ref_cache[list][scan8[0] + 2 - 1*8]=
                h->ref_cache[list][scan8[0] + 3 - 1*8]= s->current_picture.ref_index[list][b8_xy + 1];
            }else{
                *(uint32_t*)h->mv_cache [list][scan8[0] + 0 - 1*8]=
                *(uint32_t*)h->mv_cache [list][scan8[0] + 1 - 1*8]=
                *(uint32_t*)h->mv_cache [list][scan8[0] + 2 - 1*8]=
                *(uint32_t*)h->mv_cache [list][scan8[0] + 3 - 1*8]= 0;
                *(uint32_t*)&h->ref_cache[list][scan8[0] + 0 - 1*8]= ((top_type ? LIST_NOT_USED : PART_NOT_AVAILABLE)&0xFF)*0x01010101;
            }
    
            for(i=0; i<2; i++){
                int cache_idx = scan8[0] - 1 + i*2*8;
                if(USES_LIST(left_type[i], list)){
                    const int b_xy= h->mb2b_xy[left_xy[i]] + 3;
                    const int b8_xy= h->mb2b8_xy[left_xy[i]] + 1;
                    *(uint32_t*)h->mv_cache[list][cache_idx  ]= *(uint32_t*)s->current_picture.motion_val[list][b_xy + h->b_stride*left_block[0+i*2]];
                    *(uint32_t*)h->mv_cache[list][cache_idx+8]= *(uint32_t*)s->current_picture.motion_val[list][b_xy + h->b_stride*left_block[1+i*2]];
                    h->ref_cache[list][cache_idx  ]= s->current_picture.ref_index[list][b8_xy + h->b8_stride*(left_block[0+i*2]>>1)];
                    h->ref_cache[list][cache_idx+8]= s->current_picture.ref_index[list][b8_xy + h->b8_stride*(left_block[1+i*2]>>1)];
                }else{
                    *(uint32_t*)h->mv_cache [list][cache_idx  ]=
                    *(uint32_t*)h->mv_cache [list][cache_idx+8]= 0;
                    h->ref_cache[list][cache_idx  ]=
                    h->ref_cache[list][cache_idx+8]= left_type[i] ? LIST_NOT_USED : PART_NOT_AVAILABLE;
                }
            }

            if((for_deblock || (IS_DIRECT(mb_type) && !h->direct_spatial_mv_pred)) && !FRAME_MBAFF)
                continue;

            if(USES_LIST(topleft_type, list)){
                const int b_xy = h->mb2b_xy[topleft_xy] + 3 + 3*h->b_stride;
                const int b8_xy= h->mb2b8_xy[topleft_xy] + 1 + h->b8_stride;
                *(uint32_t*)h->mv_cache[list][scan8[0] - 1 - 1*8]= *(uint32_t*)s->current_picture.motion_val[list][b_xy];
                h->ref_cache[list][scan8[0] - 1 - 1*8]= s->current_picture.ref_index[list][b8_xy];
            }else{
                *(uint32_t*)h->mv_cache[list][scan8[0] - 1 - 1*8]= 0;
                h->ref_cache[list][scan8[0] - 1 - 1*8]= topleft_type ? LIST_NOT_USED : PART_NOT_AVAILABLE;
            }


            if(USES_LIST(topright_type, list)){
                const int b_xy= h->mb2b_xy[topright_xy] + 3*h->b_stride;
                const int b8_xy= h->mb2b8_xy[topright_xy] + h->b8_stride;

                *(uint32_t*)h->mv_cache[list][scan8[0] + 4 - 1*8]= *(uint32_t*)s->current_picture.motion_val[list][b_xy];
                h->ref_cache[list][scan8[0] + 4 - 1*8]= s->current_picture.ref_index[list][b8_xy];
            }else{
                *(uint32_t*)h->mv_cache [list][scan8[0] + 4 - 1*8]= 0;
                h->ref_cache[list][scan8[0] + 4 - 1*8]= topright_type ? LIST_NOT_USED : PART_NOT_AVAILABLE;
            }

            if((IS_SKIP(mb_type) || IS_DIRECT(mb_type)) && !FRAME_MBAFF)
                continue;

            h->ref_cache[list][scan8[5 ]+1] =
            h->ref_cache[list][scan8[7 ]+1] =
            h->ref_cache[list][scan8[13]+1] =  //FIXME remove past 3 (init somewhere else)
            h->ref_cache[list][scan8[4 ]] =
            h->ref_cache[list][scan8[12]] = PART_NOT_AVAILABLE;
            *(uint32_t*)h->mv_cache [list][scan8[5 ]+1]=
            *(uint32_t*)h->mv_cache [list][scan8[7 ]+1]=
            *(uint32_t*)h->mv_cache [list][scan8[13]+1]= //FIXME remove past 3 (init somewhere else)
            *(uint32_t*)h->mv_cache [list][scan8[4 ]]=
            *(uint32_t*)h->mv_cache [list][scan8[12]]= 0;

            if( h->pps.cabac ) {

                /* XXX beurk, Load mvd */
                if(USES_LIST(top_type, list)){
                    const int b_xy= h->mb2b_xy[top_xy] + 3*h->b_stride;
                    *(uint32_t*)h->mvd_cache[list][scan8[0] + 0 - 1*8]= *(uint32_t*)h->mvd_table[list][b_xy + 0];
                    *(uint32_t*)h->mvd_cache[list][scan8[0] + 1 - 1*8]= *(uint32_t*)h->mvd_table[list][b_xy + 1];
                    *(uint32_t*)h->mvd_cache[list][scan8[0] + 2 - 1*8]= *(uint32_t*)h->mvd_table[list][b_xy + 2];
                    *(uint32_t*)h->mvd_cache[list][scan8[0] + 3 - 1*8]= *(uint32_t*)h->mvd_table[list][b_xy + 3];
                }else{
                    *(uint32_t*)h->mvd_cache [list][scan8[0] + 0 - 1*8]=
                    *(uint32_t*)h->mvd_cache [list][scan8[0] + 1 - 1*8]=
                    *(uint32_t*)h->mvd_cache [list][scan8[0] + 2 - 1*8]=
                    *(uint32_t*)h->mvd_cache [list][scan8[0] + 3 - 1*8]= 0;
                }

		
                if(USES_LIST(left_type[0], list)){
                    const int b_xy= h->mb2b_xy[left_xy[0]] + 3;
		    *(uint32_t*)h->mvd_cache[list][scan8[0] - 1 + 0*8]= *(uint32_t*)h->mvd_table[list][b_xy + h->b_stride*left_block[0]];
		    *(uint32_t*)h->mvd_cache[list][scan8[0] - 1 + 1*8]= *(uint32_t*)h->mvd_table[list][b_xy + h->b_stride*left_block[1]];
		    
                }else{
                    *(uint32_t*)h->mvd_cache [list][scan8[0] - 1 + 0*8]=
                    *(uint32_t*)h->mvd_cache [list][scan8[0] - 1 + 1*8]= 0;
                }

                if(USES_LIST(left_type[1], list)){
                    const int b_xy= h->mb2b_xy[left_xy[1]] + 3;
                    *(uint32_t*)h->mvd_cache[list][scan8[0] - 1 + 2*8]= *(uint32_t*)h->mvd_table[list][b_xy + h->b_stride*left_block[2]];
                    *(uint32_t*)h->mvd_cache[list][scan8[0] - 1 + 3*8]= *(uint32_t*)h->mvd_table[list][b_xy + h->b_stride*left_block[3]];
                }else{
                    *(uint32_t*)h->mvd_cache [list][scan8[0] - 1 + 2*8]=
                    *(uint32_t*)h->mvd_cache [list][scan8[0] - 1 + 3*8]= 0;
                }

                *(uint32_t*)h->mvd_cache [list][scan8[5 ]+1]=
                *(uint32_t*)h->mvd_cache [list][scan8[7 ]+1]=
                *(uint32_t*)h->mvd_cache [list][scan8[13]+1]= //FIXME remove past 3 (init somewhere else)
                *(uint32_t*)h->mvd_cache [list][scan8[4 ]]=
                *(uint32_t*)h->mvd_cache [list][scan8[12]]= 0;

                if(h->slice_type == FF_B_TYPE){
                    fill_rectangle_hw(&h->direct_cache[scan8[0]], 4, 4, 8, 0, 1);

                    if(IS_DIRECT(top_type)){
                        *(uint32_t*)&h->direct_cache[scan8[0] - 1*8]= 0x01010101;
                    }else if(IS_8X8(top_type)){
                        int b8_xy = h->mb2b8_xy[top_xy] + h->b8_stride;
                        h->direct_cache[scan8[0] + 0 - 1*8]= h->direct_table[b8_xy];
                        h->direct_cache[scan8[0] + 2 - 1*8]= h->direct_table[b8_xy + 1];
                    }else{
                        *(uint32_t*)&h->direct_cache[scan8[0] - 1*8]= 0;
                    }

                    if(IS_DIRECT(left_type[0]))
                        h->direct_cache[scan8[0] - 1 + 0*8]= 1;
                    else if(IS_8X8(left_type[0]))
                        h->direct_cache[scan8[0] - 1 + 0*8]= h->direct_table[h->mb2b8_xy[left_xy[0]] + 1 + h->b8_stride*(left_block[0]>>1)];
                    else
                        h->direct_cache[scan8[0] - 1 + 0*8]= 0;

                    if(IS_DIRECT(left_type[1]))
                        h->direct_cache[scan8[0] - 1 + 2*8]= 1;
                    else if(IS_8X8(left_type[1]))
                        h->direct_cache[scan8[0] - 1 + 2*8]= h->direct_table[h->mb2b8_xy[left_xy[1]] + 1 + h->b8_stride*(left_block[2]>>1)];
                    else
                        h->direct_cache[scan8[0] - 1 + 2*8]= 0;
                }
            }

            if(FRAME_MBAFF){
#define MAP_MVS\
                    MAP_F2F(scan8[0] - 1 - 1*8, topleft_type)\
                    MAP_F2F(scan8[0] + 0 - 1*8, top_type)\
                    MAP_F2F(scan8[0] + 1 - 1*8, top_type)\
                    MAP_F2F(scan8[0] + 2 - 1*8, top_type)\
                    MAP_F2F(scan8[0] + 3 - 1*8, top_type)\
                    MAP_F2F(scan8[0] + 4 - 1*8, topright_type)\
                    MAP_F2F(scan8[0] - 1 + 0*8, left_type[0])\
                    MAP_F2F(scan8[0] - 1 + 1*8, left_type[0])\
                    MAP_F2F(scan8[0] - 1 + 2*8, left_type[1])\
                    MAP_F2F(scan8[0] - 1 + 3*8, left_type[1])
                if(MB_FIELD){
#define MAP_F2F(idx, mb_type)\
                    if(!IS_INTERLACED(mb_type) && h->ref_cache[list][idx] >= 0){\
                        h->ref_cache[list][idx] <<= 1;\
                        h->mv_cache[list][idx][1] /= 2;\
                        h->mvd_cache[list][idx][1] /= 2;\
                    }
                    MAP_MVS
#undef MAP_F2F
                }else{
#define MAP_F2F(idx, mb_type)\
                    if(IS_INTERLACED(mb_type) && h->ref_cache[list][idx] >= 0){\
                        h->ref_cache[list][idx] >>= 1;\
                        h->mv_cache[list][idx][1] <<= 1;\
                        h->mvd_cache[list][idx][1] <<= 1;\
                    }
                    MAP_MVS
#undef MAP_F2F
                }
            }
        }
    }
#endif

    h->neighbor_transform_size= !!IS_8x8DCT(top_type) + !!IS_8x8DCT(left_type[0]);
}

static int decode_cabac_mb_skip_hw( H264Context *h, int mb_x, int mb_y ) {
    MpegEncContext * const s = &h->s;
    int mba_xy, mbb_xy;
    int ctx = 0;

    if(FRAME_MBAFF){ //FIXME merge with the stuff in fill_caches?
        int mb_xy = mb_x + (mb_y&~1)*s->mb_stride;
        mba_xy = mb_xy - 1;
        if( (mb_y&1)
            && h->hw_slice_table[mba_xy] == h->slice_num
            && MB_FIELD == !!IS_INTERLACED( s->current_picture.mb_type[mba_xy] ) )
            mba_xy += s->mb_stride;
        if( MB_FIELD ){
            mbb_xy = mb_xy - s->mb_stride;
            if( !(mb_y&1)
                && h->hw_slice_table[mbb_xy] == h->slice_num
                && IS_INTERLACED( s->current_picture.mb_type[mbb_xy] ) )
                mbb_xy -= s->mb_stride;
        }else
            mbb_xy = mb_x + (mb_y-1)*s->mb_stride;
    }else{
        int mb_xy = mb_x + mb_y*s->mb_stride;
        mba_xy = mb_xy - 1;
        mbb_xy = mb_xy - (s->mb_stride << FIELD_PICTURE);
    }

    if( h->hw_slice_table[mba_xy] == h->slice_num && !IS_SKIP( s->current_picture.mb_type[mba_xy] ))
        ctx++;
    if( h->hw_slice_table[mbb_xy] == h->slice_num && !IS_SKIP( s->current_picture.mb_type[mbb_xy] ))
        ctx++;

    if( h->slice_type == FF_B_TYPE )
        ctx += 13;
    return get_cabac_noinline( &h->cabac, &h->cabac_state[11+ctx] );
}

static int decode_cabac_field_decoding_flag_hw(H264Context *h) {
    MpegEncContext * const s = &h->s;
    const int mb_x = s->mb_x;
    const int mb_y = s->mb_y & ~1;
    const int mba_xy = mb_x - 1 +  mb_y   *s->mb_stride;
    const int mbb_xy = mb_x     + (mb_y-2)*s->mb_stride;

    unsigned int ctx = 0;

    if( h->hw_slice_table[mba_xy] == h->slice_num && IS_INTERLACED( s->current_picture.mb_type[mba_xy] ) ) {
        ctx += 1;
    }
    if( h->hw_slice_table[mbb_xy] == h->slice_num && IS_INTERLACED( s->current_picture.mb_type[mbb_xy] ) ) {
        ctx += 1;
    }

    return get_cabac_noinline( &h->cabac, &h->cabac_state[70 + ctx] );
}


static inline void write_back_motion_hw(H264Context *h, int mb_type){
    MpegEncContext * const s = &h->s;
    const int b_xy = 4*s->mb_x + 4*s->mb_y*h->b_stride;
    const int b8_xy= 2*s->mb_x + 2*s->mb_y*h->b8_stride;
    int list;

    if(!USES_LIST(mb_type, 0))
        fill_rectangle_hw(&s->current_picture.ref_index[0][b8_xy], 2, 2, h->b8_stride, (uint8_t)LIST_NOT_USED, 1);

    for(list=0; list<h->list_count; list++){
        int y;
        if(!USES_LIST(mb_type, list))
            continue;
        for(y=0; y<4; y++){
            *(uint64_t*)s->current_picture.motion_val[list][b_xy + 0 + y*h->b_stride]= *(uint64_t*)h->mv_cache[list][scan8[0]+0 + 8*y];
            *(uint64_t*)s->current_picture.motion_val[list][b_xy + 2 + y*h->b_stride]= *(uint64_t*)h->mv_cache[list][scan8[0]+2 + 8*y];
        }

        if( h->pps.cabac ) {
	  if(IS_SKIP(mb_type)){
                fill_rectangle_hw(h->mvd_table[list][b_xy], 4, 4, h->b_stride, 0, 4);
	  } else{
	    for(y=0; y<4; y++){
	      *(uint64_t*)h->mvd_table[list][b_xy + 0 + y*h->b_stride]= *(uint64_t*)h->mvd_cache[list][scan8[0]+0 + 8*y];
	      *(uint64_t*)h->mvd_table[list][b_xy + 2 + y*h->b_stride]= *(uint64_t*)h->mvd_cache[list][scan8[0]+2 + 8*y];
	    }
	  }
        }

        {
            int8_t *ref_index = &s->current_picture.ref_index[list][b8_xy];
            ref_index[0+0*h->b8_stride]= h->ref_cache[list][scan8[0]];
            ref_index[1+0*h->b8_stride]= h->ref_cache[list][scan8[4]];
            ref_index[0+1*h->b8_stride]= h->ref_cache[list][scan8[8]];
            ref_index[1+1*h->b8_stride]= h->ref_cache[list][scan8[12]];
        }
    }

    if(h->slice_type == FF_B_TYPE && h->pps.cabac){
        if(IS_8X8(mb_type)){
            uint8_t *direct_table = &h->direct_table[b8_xy];
            direct_table[1+0*h->b8_stride] = IS_DIRECT(h->sub_mb_type[1]) ? 1 : 0;
            direct_table[0+1*h->b8_stride] = IS_DIRECT(h->sub_mb_type[2]) ? 1 : 0;
            direct_table[1+1*h->b8_stride] = IS_DIRECT(h->sub_mb_type[3]) ? 1 : 0;
        }
    }
}


static inline void compute_mb_neighbors(H264Context *h)
{
    MpegEncContext * const s = &h->s;
    const int mb_xy  = s->mb_x + s->mb_y*s->mb_stride;
    h->top_mb_xy     = mb_xy - s->mb_stride;
    h->left_mb_xy[0] = mb_xy - 1;
    if(FRAME_MBAFF){
        const int pair_xy          = s->mb_x     + (s->mb_y & ~1)*s->mb_stride;
        const int top_pair_xy      = pair_xy     - s->mb_stride;
        const int top_mb_frame_flag      = !IS_INTERLACED(s->current_picture.mb_type[top_pair_xy]);
        const int left_mb_frame_flag = !IS_INTERLACED(s->current_picture.mb_type[pair_xy-1]);
        const int curr_mb_frame_flag = !MB_FIELD;
        const int bottom = (s->mb_y & 1);
        if (bottom
                ? !curr_mb_frame_flag // bottom macroblock
                : (!curr_mb_frame_flag && !top_mb_frame_flag) // top macroblock
                ) {
            h->top_mb_xy -= s->mb_stride;
        }
        if (left_mb_frame_flag != curr_mb_frame_flag) {
            h->left_mb_xy[0] = pair_xy - 1;
        }
    } else if (FIELD_PICTURE) {
        h->top_mb_xy -= s->mb_stride;
    }
    return;
}

static int decode_cabac_intra_mb_type_hw(H264Context *h, int ctx_base, int intra_slice) {
    uint8_t *state= &h->cabac_state[ctx_base];
    int mb_type;

    if(intra_slice){
        MpegEncContext * const s = &h->s;
        const int mba_xy = h->left_mb_xy[0];
        const int mbb_xy = h->top_mb_xy;
        int ctx=0;
        if( h->hw_slice_table[mba_xy] == h->slice_num && !IS_INTRA4x4( s->current_picture.mb_type[mba_xy] ) )
            ctx++;
        if( h->hw_slice_table[mbb_xy] == h->slice_num && !IS_INTRA4x4( s->current_picture.mb_type[mbb_xy] ) )
            ctx++;
        if( get_cabac_noinline( &h->cabac, &state[ctx] ) == 0 )
            return 0;   /* I4x4 */
        state += 2;
    }else{
        if( get_cabac_noinline( &h->cabac, &state[0] ) == 0 )
            return 0;   /* I4x4 */
    }

    if( get_cabac_terminate( &h->cabac ) )
        return 25;  /* PCM */

    mb_type = 1; /* I16x16 */
    mb_type += 12 * get_cabac_noinline( &h->cabac, &state[1] ); /* cbp_luma != 0 */
    if( get_cabac_noinline( &h->cabac, &state[2] ) ) /* cbp_chroma */
        mb_type += 4 + 4 * get_cabac_noinline( &h->cabac, &state[2+intra_slice] );
    mb_type += 2 * get_cabac_noinline( &h->cabac, &state[3+intra_slice] );
    mb_type += 1 * get_cabac_noinline( &h->cabac, &state[3+2*intra_slice] );
    return mb_type;
}


static int decode_cabac_mb_type_hw( H264Context *h ) {
    MpegEncContext * const s = &h->s;

    if( h->slice_type == FF_I_TYPE ) {
        return decode_cabac_intra_mb_type_hw(h, 3, 1);
    } else if( h->slice_type == FF_P_TYPE ) {
        if( get_cabac_noinline( &h->cabac, &h->cabac_state[14] ) == 0 ) {
            /* P-type */
            if( get_cabac_noinline( &h->cabac, &h->cabac_state[15] ) == 0 ) {
                /* P_L0_D16x16, P_8x8 */
                return 3 * get_cabac_noinline( &h->cabac, &h->cabac_state[16] );
            } else {
                /* P_L0_D8x16, P_L0_D16x8 */
                return 2 - get_cabac_noinline( &h->cabac, &h->cabac_state[17] );
            }
        } else {
            return decode_cabac_intra_mb_type_hw(h, 17, 0) + 5;
        }
    } else if( h->slice_type == FF_B_TYPE ) {
        const int mba_xy = h->left_mb_xy[0];
        const int mbb_xy = h->top_mb_xy;
        int ctx = 0;
        int bits;

        if( h->hw_slice_table[mba_xy] == h->slice_num && !IS_DIRECT( s->current_picture.mb_type[mba_xy] ) )
            ctx++;
        if( h->hw_slice_table[mbb_xy] == h->slice_num && !IS_DIRECT( s->current_picture.mb_type[mbb_xy] ) )
            ctx++;

        if( !get_cabac_noinline( &h->cabac, &h->cabac_state[27+ctx] ) )
            return 0; /* B_Direct_16x16 */

        if( !get_cabac_noinline( &h->cabac, &h->cabac_state[27+3] ) ) {
            return 1 + get_cabac_noinline( &h->cabac, &h->cabac_state[27+5] ); /* B_L[01]_16x16 */
        }

        bits = get_cabac_noinline( &h->cabac, &h->cabac_state[27+4] ) << 3;
        bits|= get_cabac_noinline( &h->cabac, &h->cabac_state[27+5] ) << 2;
        bits|= get_cabac_noinline( &h->cabac, &h->cabac_state[27+5] ) << 1;
        bits|= get_cabac_noinline( &h->cabac, &h->cabac_state[27+5] );
        if( bits < 8 )
            return bits + 3; /* B_Bi_16x16 through B_L1_L0_16x8 */
        else if( bits == 13 ) {
            return decode_cabac_intra_mb_type_hw(h, 32, 0) + 23;
        } else if( bits == 14 )
            return 11; /* B_L1_L0_8x16 */
        else if( bits == 15 )
            return 22; /* B_8x8 */

        bits= ( bits<<1 ) | get_cabac_noinline( &h->cabac, &h->cabac_state[27+5] );
        return bits - 4; /* B_L0_Bi_* through B_Bi_Bi_* */
    } else {
        /* TODO SI/SP frames? */
        return -1;
    }
}


static inline int decode_cabac_mb_transform_size_hw( H264Context *h ) {
    return get_cabac_noinline( &h->cabac, &h->cabac_state[399 + h->neighbor_transform_size] );
}

static int decode_cabac_mb_intra4x4_pred_mode_hw( H264Context *h, int pred_mode ) {
    int mode = 0;

    if( get_cabac( &h->cabac, &h->cabac_state[68] ) )
        return pred_mode;

    mode += 1 * get_cabac( &h->cabac, &h->cabac_state[69] );
    mode += 2 * get_cabac( &h->cabac, &h->cabac_state[69] );
    mode += 4 * get_cabac( &h->cabac, &h->cabac_state[69] );

    if( mode >= pred_mode )
        return mode + 1;
    else
        return mode;
}

static inline void write_back_intra_pred_mode_hw(H264Context *h){
    MpegEncContext * const s = &h->s;
    const int mb_xy= s->mb_x + s->mb_y*s->mb_stride;
    h->intra4x4_pred_mode_hw[mb_xy][0]= h->intra4x4_pred_mode_cache[7+8*1];
    h->intra4x4_pred_mode_hw[mb_xy][1]= h->intra4x4_pred_mode_cache[7+8*2];
    h->intra4x4_pred_mode_hw[mb_xy][2]= h->intra4x4_pred_mode_cache[7+8*3];
    h->intra4x4_pred_mode_hw[mb_xy][3]= h->intra4x4_pred_mode_cache[7+8*4];
    h->intra4x4_pred_mode_hw[mb_xy][4]= h->intra4x4_pred_mode_cache[4+8*4];
    h->intra4x4_pred_mode_hw[mb_xy][5]= h->intra4x4_pred_mode_cache[5+8*4];
    h->intra4x4_pred_mode_hw[mb_xy][6]= h->intra4x4_pred_mode_cache[6+8*4];
}

static inline int check_intra4x4_pred_mode_hw(H264Context *h){
    MpegEncContext * const s = &h->s;
    static const int8_t top [12]= {-1, 0,LEFT_DC_PRED,-1,-1,-1,-1,-1, 0};
    static const int8_t left[12]= { 0,-1, TOP_DC_PRED, 0,-1,-1,-1, 0,-1,DC_128_PRED};
    int i;

    if(!(h->top_samples_available&0x8000)){
        for(i=0; i<4; i++){
            int status= top[ h->intra4x4_pred_mode_cache[scan8[0] + i] ];
            if(status<0){
                av_log(h->s.avctx, AV_LOG_ERROR, "top block unavailable for requested intra4x4 mode %d at %d %d\n", status, s->mb_x, s->mb_y);
                return -1;
            } else if(status){
                h->intra4x4_pred_mode_cache[scan8[0] + i]= status;
            }
        }
    }

    if(!(h->left_samples_available&0x8000)){
        for(i=0; i<4; i++){
            int status= left[ h->intra4x4_pred_mode_cache[scan8[0] + 8*i] ];
            if(status<0){
                av_log(h->s.avctx, AV_LOG_ERROR, "left block unavailable for requested intra4x4 mode %d at %d %d\n", status, s->mb_x, s->mb_y);
                return -1;
            } else if(status){
                h->intra4x4_pred_mode_cache[scan8[0] + 8*i]= status;
            }
        }
    }

    return 0;
} //FIXME cleanup like next


static inline int check_intra_pred_mode_hw(H264Context *h, int mode){
    MpegEncContext * const s = &h->s;
    static const int8_t top [7]= {LEFT_DC_PRED8x8, 1,-1,-1};
    static const int8_t left[7]= { TOP_DC_PRED8x8,-1, 2,-1,DC_128_PRED8x8};

    if(mode > 6U) {
        av_log(h->s.avctx, AV_LOG_ERROR, "out of range intra chroma pred mode at %d %d\n", s->mb_x, s->mb_y);
        return -1;
    }

    if(!(h->top_samples_available&0x8000)){
        mode= top[ mode ];
        if(mode<0){
            av_log(h->s.avctx, AV_LOG_ERROR, "top block unavailable for requested intra mode at %d %d\n", s->mb_x, s->mb_y);
            return -1;
        }
    }

    if(!(h->left_samples_available&0x8000)){
        mode= left[ mode ];
        if(mode<0){
            av_log(h->s.avctx, AV_LOG_ERROR, "left block unavailable for requested intra mode at %d %d\n", s->mb_x, s->mb_y);
            return -1;
        }
    }

    return mode;
}

static int decode_cabac_mb_chroma_pre_mode_hw( H264Context *h) {
    const int mba_xy = h->left_mb_xy[0];
    const int mbb_xy = h->top_mb_xy;

    int ctx = 0;

    /* No need to test for IS_INTRA4x4 and IS_INTRA16x16, as we set chroma_pred_mode_table to 0 */
    if( h->hw_slice_table[mba_xy] == h->slice_num && h->chroma_pred_mode_table[mba_xy] != 0 )
        ctx++;

    if( h->hw_slice_table[mbb_xy] == h->slice_num && h->chroma_pred_mode_table[mbb_xy] != 0 )
        ctx++;

    if( get_cabac_noinline( &h->cabac, &h->cabac_state[64+ctx] ) == 0 )
        return 0;

    if( get_cabac_noinline( &h->cabac, &h->cabac_state[64+3] ) == 0 )
        return 1;
    if( get_cabac_noinline( &h->cabac, &h->cabac_state[64+3] ) == 0 )
        return 2;
    else
        return 3;
}

static int decode_cabac_p_mb_sub_type_hw( H264Context *h ) {
    if( get_cabac( &h->cabac, &h->cabac_state[21] ) )
        return 0;   /* 8x8 */
    if( !get_cabac( &h->cabac, &h->cabac_state[22] ) )
        return 1;   /* 8x4 */
    if( get_cabac( &h->cabac, &h->cabac_state[23] ) )
        return 2;   /* 4x8 */
    return 3;       /* 4x4 */
}


static int decode_cabac_b_mb_sub_type_hw( H264Context *h ) {
    int type;
    if( !get_cabac( &h->cabac, &h->cabac_state[36] ) )
        return 0;   /* B_Direct_8x8 */
    if( !get_cabac( &h->cabac, &h->cabac_state[37] ) )
        return 1 + get_cabac( &h->cabac, &h->cabac_state[39] ); /* B_L0_8x8, B_L1_8x8 */
    type = 3;
    if( get_cabac( &h->cabac, &h->cabac_state[38] ) ) {
        if( get_cabac( &h->cabac, &h->cabac_state[39] ) )
            return 11 + get_cabac( &h->cabac, &h->cabac_state[39] ); /* B_L1_4x4, B_Bi_4x4 */
        type += 4;
    }
    type += 2*get_cabac( &h->cabac, &h->cabac_state[39] );
    type +=   get_cabac( &h->cabac, &h->cabac_state[39] );
    return type;
}

static inline int fetch_diagonal_mv_hw(H264Context *h, const int16_t **C, int i, int list, int part_width){
    const int topright_ref= h->ref_cache[list][ i - 8 + part_width ];
    MpegEncContext *s = &h->s;

    /* there is no consistent mapping of mvs to neighboring locations that will
     * make mbaff happy, so we can't move all this logic to fill_caches */
    if(FRAME_MBAFF){
        const uint32_t *mb_types = s->current_picture_ptr->mb_type;
        const int16_t *mv;
        *(uint32_t*)h->mv_cache[list][scan8[0]-2] = 0;
        *C = h->mv_cache[list][scan8[0]-2];

        if(!MB_FIELD
           && (s->mb_y&1) && i < scan8[0]+8 && topright_ref != PART_NOT_AVAILABLE){
            int topright_xy = s->mb_x + (s->mb_y-1)*s->mb_stride + (i == scan8[0]+3);
            if(IS_INTERLACED(mb_types[topright_xy])){
#define SET_DIAG_MV(MV_OP, REF_OP, X4, Y4)\
                const int x4 = X4, y4 = Y4;\
                const int mb_type = mb_types[(x4>>2)+(y4>>2)*s->mb_stride];\
                if(!USES_LIST(mb_type,list) && !IS_8X8(mb_type))\
                    return LIST_NOT_USED;\
                mv = s->current_picture_ptr->motion_val[list][x4 + y4*h->b_stride];\
                h->mv_cache[list][scan8[0]-2][0] = mv[0];\
                h->mv_cache[list][scan8[0]-2][1] = mv[1] MV_OP;\
                return s->current_picture_ptr->ref_index[list][(x4>>1) + (y4>>1)*h->b8_stride] REF_OP;

                SET_DIAG_MV(*2, >>1, s->mb_x*4+(i&7)-4+part_width, s->mb_y*4-1);
            }
        }
        if(topright_ref == PART_NOT_AVAILABLE
           && ((s->mb_y&1) || i >= scan8[0]+8) && (i&7)==4
           && h->ref_cache[list][scan8[0]-1] != PART_NOT_AVAILABLE){
            if(!MB_FIELD
               && IS_INTERLACED(mb_types[h->left_mb_xy[0]])){
                SET_DIAG_MV(*2, >>1, s->mb_x*4-1, (s->mb_y|1)*4+(s->mb_y&1)*2+(i>>4)-1);
            }
            if(MB_FIELD
               && !IS_INTERLACED(mb_types[h->left_mb_xy[0]])
               && i >= scan8[0]+8){
                // leftshift will turn LIST_NOT_USED into PART_NOT_AVAILABLE, but that's ok.
                SET_DIAG_MV(>>1, <<1, s->mb_x*4-1, (s->mb_y&~1)*4 - 1 + ((i-scan8[0])>>3)*2);
            }
        }
#undef SET_DIAG_MV
    }

    if(topright_ref != PART_NOT_AVAILABLE){
        *C= h->mv_cache[list][ i - 8 + part_width ];
        return topright_ref;
    }else{
        tprintf(s->avctx, "topright MV not available\n");

        *C= h->mv_cache[list][ i - 8 - 1 ];
        return h->ref_cache[list][ i - 8 - 1 ];
    }
}


static inline void pred_motion_hw(H264Context * const h, int n, int part_width, int list, int ref, int * const mx, int * const my){
    const int index8= scan8[n];
    const int top_ref=      h->ref_cache[list][ index8 - 8 ];
    const int left_ref=     h->ref_cache[list][ index8 - 1 ];
    const int16_t * const A= h->mv_cache[list][ index8 - 1 ];
    const int16_t * const B= h->mv_cache[list][ index8 - 8 ];
    const int16_t * C;
    int diagonal_ref, match_count;

    assert(part_width==1 || part_width==2 || part_width==4);

/* mv_cache
  B . . A T T T T
  U . . L . . , .
  U . . L . . . .
  U . . L . . , .
  . . . L . . . .
*/

    diagonal_ref= fetch_diagonal_mv_hw(h, &C, index8, list, part_width);
    match_count= (diagonal_ref==ref) + (top_ref==ref) + (left_ref==ref);
    tprintf(h->s.avctx, "pred_motion match_count=%d\n", match_count);
    if(match_count > 1){ //most common
        *mx= mid_pred(A[0], B[0], C[0]);
        *my= mid_pred(A[1], B[1], C[1]);
    }else if(match_count==1){
        if(left_ref==ref){
            *mx= A[0];
            *my= A[1];
        }else if(top_ref==ref){
            *mx= B[0];
            *my= B[1];
        }else{
            *mx= C[0];
            *my= C[1];
        }
    }else{
        if(top_ref == PART_NOT_AVAILABLE && diagonal_ref == PART_NOT_AVAILABLE && left_ref != PART_NOT_AVAILABLE){
            *mx= A[0];
            *my= A[1];
        }else{
            *mx= mid_pred(A[0], B[0], C[0]);
            *my= mid_pred(A[1], B[1], C[1]);
        }
    }

    tprintf(h->s.avctx, "pred_motion (%2d %2d %2d) (%2d %2d %2d) (%2d %2d %2d) -> (%2d %2d %2d) at %2d %2d %d list %d\n", top_ref, B[0], B[1],                    diagonal_ref, C[0], C[1], left_ref, A[0], A[1], ref, *mx, *my, h->s.mb_x, h->s.mb_y, n, list);
}

static inline void pred_pskip_motion_hw(H264Context * const h, int * const mx, int * const my){
    const int top_ref = h->ref_cache[0][ scan8[0] - 8 ];
    const int left_ref= h->ref_cache[0][ scan8[0] - 1 ];

    tprintf(h->s.avctx, "pred_pskip: (%d) (%d) at %2d %2d\n", top_ref, left_ref, h->s.mb_x, h->s.mb_y);

    if(top_ref == PART_NOT_AVAILABLE || left_ref == PART_NOT_AVAILABLE
       || (top_ref == 0  && *(uint32_t*)h->mv_cache[0][ scan8[0] - 8 ] == 0)
       || (left_ref == 0 && *(uint32_t*)h->mv_cache[0][ scan8[0] - 1 ] == 0)){

        *mx = *my = 0;
        return;
    }

    pred_motion(h, 0, 4, 0, 0, mx, my);

    return;
}


static void decode_mb_skip_hw(H264Context *h){
    MpegEncContext * const s = &h->s;
    const int mb_xy= s->mb_x + s->mb_y*s->mb_stride;
    int mb_type=0;

    memset(h->non_zero_count[mb_xy], 0, 16);
    memset(h->non_zero_count_cache + 8, 0, 8*5); //FIXME ugly, remove pfui

    if(MB_FIELD)
        mb_type|= MB_TYPE_INTERLACED;

    if( h->slice_type == FF_B_TYPE )
    {
        // just for fill_caches. pred_direct_motion will set the real mb_type
        mb_type|= MB_TYPE_16x16|MB_TYPE_P0L0|MB_TYPE_P0L1|MB_TYPE_DIRECT2|MB_TYPE_SKIP;
        fill_caches_hw(h, mb_type, 0); //FIXME check what is needed and what not ...
        pred_direct_motion_hw(h, &mb_type);
        mb_type|= MB_TYPE_SKIP;
    }
    else
    {
        int mx, my;
        mb_type|= MB_TYPE_16x16|MB_TYPE_P0L0|MB_TYPE_P1L0|MB_TYPE_SKIP;
        fill_caches_hw(h, mb_type, 0); //FIXME check what is needed and what not ...
        pred_pskip_motion_hw(h, &mx, &my);
        fill_rectangle_hw(&h->ref_cache[0][scan8[0]], 4, 4, 8, 0, 1);
        fill_rectangle_hw(h->mv_cache[0][scan8[0]], 4, 4, 8, pack16to32(mx,my), 4);
    }

    write_back_motion_hw(h, mb_type);
    s->current_picture.mb_type[mb_xy]= mb_type;
    s->current_picture.qscale_table[mb_xy]= s->qscale;
    h->hw_slice_table[ mb_xy ]= h->slice_num;
    h->prev_mb_skipped= 1;
}


static int decode_cabac_mb_ref_hw( H264Context *h, int list, int n ) {
    int refa = h->ref_cache[list][scan8[n] - 1];
    int refb = h->ref_cache[list][scan8[n] - 8];
    int ref  = 0;
    int ctx  = 0;

    if( h->slice_type == FF_B_TYPE) {
        if( refa > 0 && !h->direct_cache[scan8[n] - 1] )
            ctx++;
        if( refb > 0 && !h->direct_cache[scan8[n] - 8] )
            ctx += 2;
    } else {
        if( refa > 0 )
            ctx++;
        if( refb > 0 )
            ctx += 2;
    }

    while( get_cabac( &h->cabac, &h->cabac_state[54+ctx] ) ) {
        ref++;
        if( ctx < 4 )
            ctx = 4;
        else
            ctx = 5;
        if(ref >= 32 /*h->ref_list[list]*/){
            av_log(h->s.avctx, AV_LOG_ERROR, "overflow in decode_cabac_mb_ref\n");
            return 0; //FIXME we should return -1 and check the return everywhere
        }
    }
    return ref;
}

static int decode_cabac_mb_mvd_hw( H264Context *h, int list, int n, int l ) {
    int amvd = abs( h->mvd_cache[list][scan8[n] - 1][l] ) +
               abs( h->mvd_cache[list][scan8[n] - 8][l] );
    int ctxbase = (l == 0) ? 40 : 47;
    int ctx, mvd;

    if( amvd < 3 )
        ctx = 0;
    else if( amvd > 32 )
        ctx = 2;
    else
        ctx = 1;

    if(!get_cabac(&h->cabac, &h->cabac_state[ctxbase+ctx]))
        return 0;

    mvd= 1;
    ctx= 3;
    while( mvd < 9 && get_cabac( &h->cabac, &h->cabac_state[ctxbase+ctx] ) ) {
        mvd++;
        if( ctx < 6 )
            ctx++;
    }

    if( mvd >= 9 ) {
        int k = 3;
        while( get_cabac_bypass( &h->cabac ) ) {
            mvd += 1 << k;
            k++;
            if(k>24){
                av_log(h->s.avctx, AV_LOG_ERROR, "overflow in decode_cabac_mb_mvd\n");
                return INT_MIN;
            }
        }
        while( k-- ) {
            if( get_cabac_bypass( &h->cabac ) )
                mvd += 1 << k;
        }
    }
    return get_cabac_bypass_sign( &h->cabac, -mvd );
}

static inline void pred_16x8_motion_hw(H264Context * const h, int n, int list, int ref, int * const mx, int * const my){
    if(n==0){
        const int top_ref=      h->ref_cache[list][ scan8[0] - 8 ];
        const int16_t * const B= h->mv_cache[list][ scan8[0] - 8 ];

        tprintf(h->s.avctx, "pred_16x8: (%2d %2d %2d) at %2d %2d %d list %d\n", top_ref, B[0], B[1], h->s.mb_x, h->s.mb_y, n, list);

        if(top_ref == ref){
            *mx= B[0];
            *my= B[1];
            return;
        }
    }else{
        const int left_ref=     h->ref_cache[list][ scan8[8] - 1 ];
        const int16_t * const A= h->mv_cache[list][ scan8[8] - 1 ];

        tprintf(h->s.avctx, "pred_16x8: (%2d %2d %2d) at %2d %2d %d list %d\n", left_ref, A[0], A[1], h->s.mb_x, h->s.mb_y, n, list);

        if(left_ref == ref){
            *mx= A[0];
            *my= A[1];
            return;
        }
    }

    //RARE
    pred_motion_hw(h, n, 4, list, ref, mx, my);
}

static inline void pred_8x16_motion_hw(H264Context * const h, int n, int list, int ref, int * const mx, int * const my){
    if(n==0){
        const int left_ref=      h->ref_cache[list][ scan8[0] - 1 ];
        const int16_t * const A=  h->mv_cache[list][ scan8[0] - 1 ];

        tprintf(h->s.avctx, "pred_8x16: (%2d %2d %2d) at %2d %2d %d list %d\n", left_ref, A[0], A[1], h->s.mb_x, h->s.mb_y, n, list);

        if(left_ref == ref){
            *mx= A[0];
            *my= A[1];
            return;
        }
    }else{
        const int16_t * C;
        int diagonal_ref;

        diagonal_ref= fetch_diagonal_mv_hw(h, &C, scan8[4], list, 2);

        tprintf(h->s.avctx, "pred_8x16: (%2d %2d %2d) at %2d %2d %d list %d\n", diagonal_ref, C[0], C[1], h->s.mb_x, h->s.mb_y, n, list);

        if(diagonal_ref == ref){
            *mx= C[0];
            *my= C[1];
            return;
        }
    }

    //RARE
    pred_motion_hw(h, n, 2, list, ref, mx, my);
}

static int decode_cabac_mb_cbp_luma_hw( H264Context *h) {
    int cbp_b, cbp_a, ctx, cbp = 0;

    cbp_a = h->hw_slice_table[h->left_mb_xy[0]] == h->slice_num ? h->left_cbp : -1;
    cbp_b = h->hw_slice_table[h->top_mb_xy]     == h->slice_num ? h->top_cbp  : -1;

    ctx = !(cbp_a & 0x02) + 2 * !(cbp_b & 0x04);
    cbp |= get_cabac_noinline(&h->cabac, &h->cabac_state[73 + ctx]);
    ctx = !(cbp   & 0x01) + 2 * !(cbp_b & 0x08);
    cbp |= get_cabac_noinline(&h->cabac, &h->cabac_state[73 + ctx]) << 1;
    ctx = !(cbp_a & 0x08) + 2 * !(cbp   & 0x01);
    cbp |= get_cabac_noinline(&h->cabac, &h->cabac_state[73 + ctx]) << 2;
    ctx = !(cbp   & 0x04) + 2 * !(cbp   & 0x02);
    cbp |= get_cabac_noinline(&h->cabac, &h->cabac_state[73 + ctx]) << 3;
    return cbp;
}
static int decode_cabac_mb_cbp_chroma_hw( H264Context *h) {
    int ctx;
    int cbp_a, cbp_b;

    cbp_a = (h->left_cbp>>4)&0x03;
    cbp_b = (h-> top_cbp>>4)&0x03;

    ctx = 0;
    if( cbp_a > 0 ) ctx++;
    if( cbp_b > 0 ) ctx += 2;
    if( get_cabac_noinline( &h->cabac, &h->cabac_state[77 + ctx] ) == 0 )
        return 0;

    ctx = 4;
    if( cbp_a == 2 ) ctx++;
    if( cbp_b == 2 ) ctx += 2;
    return 1 + get_cabac_noinline( &h->cabac, &h->cabac_state[77 + ctx] );
}

static int decode_cabac_mb_dqp_hw( H264Context *h) {
    int   ctx = 0;
    int   val = 0;

    if( h->last_qscale_diff != 0 )
        ctx++;

    while( get_cabac_noinline( &h->cabac, &h->cabac_state[60 + ctx] ) ) {
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


static void decode_cabac_residual_hw( H264Context *h, DCTELEM *block, int cat, int n, const uint8_t *scantable, const uint32_t *qmul, int max_coeff) {
    MpegEncContext * const s = &h->s;
    const int mb_xy  = h->s.mb_x + h->s.mb_y*h->s.mb_stride;
    static const int significant_coeff_flag_offset[2][6] = {
      { 105+0, 105+15, 105+29, 105+44, 105+47, 402 },
      { 277+0, 277+15, 277+29, 277+44, 277+47, 436 }
    };
    static const int last_coeff_flag_offset[2][6] = {
      { 166+0, 166+15, 166+29, 166+44, 166+47, 417 },
      { 338+0, 338+15, 338+29, 338+44, 338+47, 451 }
    };
    static const int coeff_abs_level_m1_offset[6] = {
        227+0, 227+10, 227+20, 227+30, 227+39, 426
    };
    static const uint8_t significant_coeff_flag_offset_8x8[2][63] = {
      { 0, 1, 2, 3, 4, 5, 5, 4, 4, 3, 3, 4, 4, 4, 5, 5,
        4, 4, 4, 4, 3, 3, 6, 7, 7, 7, 8, 9,10, 9, 8, 7,
        7, 6,11,12,13,11, 6, 7, 8, 9,14,10, 9, 8, 6,11,
       12,13,11, 6, 9,14,10, 9,11,12,13,11,14,10,12 },
      { 0, 1, 1, 2, 2, 3, 3, 4, 5, 6, 7, 7, 7, 8, 4, 5,
        6, 9,10,10, 8,11,12,11, 9, 9,10,10, 8,11,12,11,
        9, 9,10,10, 8,11,12,11, 9, 9,10,10, 8,13,13, 9,
        9,10,10, 8,13,13, 9, 9,10,10,14,14,14,14,14 }
    };

    static const uint8_t last_coeff_flag_offset_8x8_hw[63] = {
      0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
      5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8
    };

    int index[64];

    int av_unused last;
    int coeff_count = 0;

    int abslevel1 = 1;
    int abslevelgt1 = 0;

    uint8_t *significant_coeff_ctx_base;
    uint8_t *last_coeff_ctx_base;
    uint8_t *abs_level_m1_ctx_base;

#ifndef ARCH_X86
#define CABAC_ON_STACK
#endif
#ifdef CABAC_ON_STACK
#define CC &cc
    CABACContext cc;
    cc.range     = h->cabac.range;
    cc.low       = h->cabac.low;
    cc.bytestream= h->cabac.bytestream;
#else
#define CC &h->cabac
#endif


    /* cat: 0-> DC 16x16  n = 0
     *      1-> AC 16x16  n = luma4x4idx
     *      2-> Luma4x4   n = luma4x4idx
     *      3-> DC Chroma n = iCbCr
     *      4-> AC Chroma n = 4 * iCbCr + chroma4x4idx
     *      5-> Luma8x8   n = 4 * luma8x8idx
     */

    /* read coded block flag */
    if( cat != 5 ) {
        if( get_cabac( CC, &h->cabac_state[85 + get_cabac_cbf_ctx_hw( h, cat, n ) ] ) == 0 ) {
            if( cat == 1 || cat == 2 )
                h->non_zero_count_cache[scan8[n]] = 0;
            else if( cat == 4 )
                h->non_zero_count_cache[scan8[16+n]] = 0;
#ifdef CABAC_ON_STACK
            h->cabac.range     = cc.range     ;
            h->cabac.low       = cc.low       ;
            h->cabac.bytestream= cc.bytestream;
#endif
            return;
        }
    }

    significant_coeff_ctx_base = h->cabac_state
        + significant_coeff_flag_offset[MB_FIELD][cat];
    last_coeff_ctx_base = h->cabac_state
        + last_coeff_flag_offset[MB_FIELD][cat];
    abs_level_m1_ctx_base = h->cabac_state
        + coeff_abs_level_m1_offset[cat];

    if( cat == 5 ) {
#define DECODE_SIGNIFICANCE( coefs, sig_off, last_off ) \
        for(last= 0; last < coefs; last++) { \
            uint8_t *sig_ctx = significant_coeff_ctx_base + sig_off; \
            if( get_cabac( CC, sig_ctx )) { \
                uint8_t *last_ctx = last_coeff_ctx_base + last_off; \
                index[coeff_count++] = last; \
                if( get_cabac( CC, last_ctx ) ) { \
                    last= max_coeff; \
                    break; \
                } \
            } \
        }\
        if( last == max_coeff -1 ) {\
            index[coeff_count++] = last;\
        }
        const uint8_t *sig_off = significant_coeff_flag_offset_8x8[MB_FIELD];
#if defined(ARCH_X86N) && defined(HAVE_7REGS) && defined(HAVE_EBX_AVAILABLE) && !defined(BROKEN_RELOCATIONS)
        coeff_count= decode_significance_8x8_x86(CC, significant_coeff_ctx_base, index, sig_off);
    } else {
      coeff_count= decode_significance_x86(CC, max_coeff, significant_coeff_ctx_base, index);
#else

       DECODE_SIGNIFICANCE( 63, sig_off[last], last_coeff_flag_offset_8x8_hw[last]);
    } else {
       DECODE_SIGNIFICANCE( max_coeff - 1, last, last );
#endif
    }

    assert(coeff_count > 0);

    if( cat == 0 )
        h->cbp_table[mb_xy] |= 0x100;
    else if( cat == 1 || cat == 2 )
        h->non_zero_count_cache[scan8[n]] = coeff_count;
    else if( cat == 3 )
        h->cbp_table[mb_xy] |= 0x40 << n;
    else if( cat == 4 )
        h->non_zero_count_cache[scan8[16+n]] = coeff_count;
    else {
        assert( cat == 5 );
        fill_rectangle_hw(&h->non_zero_count_cache[scan8[n]], 2, 2, 8, coeff_count, 1);
    }

#ifdef JZC_GATHER_OPT
    int di;
    for (di=0; di<64; di++)
      sde_tmp_blk[di] = 0;
    if( cat == 0 )
      sde_nnz_dct[24] = coeff_count;
    else if( cat == 1 || cat == 2 )
      sde_nnz_dct[n] = coeff_count;
    else if( cat == 3 )
      sde_nnz_dct[25+n] = coeff_count;
    else if( cat == 4 )
      sde_nnz_dct[16+n] = coeff_count;
    else
      sde_nnz_dct[n] = sde_nnz_dct[n+1] = sde_nnz_dct[n+2] = sde_nnz_dct[n+3] = coeff_count;
#endif

    for( coeff_count--; coeff_count >= 0; coeff_count-- ) {
        uint8_t *ctx = (abslevelgt1 != 0 ? 0 : FFMIN( 4, abslevel1 )) + abs_level_m1_ctx_base;
        int j= scantable[index[coeff_count]];
#ifdef JZC_GATHER_OPT
	int sj;
	if ( cat == 0 ) sj = y_dc_table[j >> 4];
	else if ( cat == 3 ) sj = j >> 4;
	else sj = j;
#endif

        if( get_cabac( CC, ctx ) == 0 ) {
            if( !qmul ) {
                block[j] = get_cabac_bypass_sign( CC, -1);
#ifdef JZC_GATHER_OPT
		sde_tmp_blk[sj] = block[j];
#endif
            }else{
#ifdef JZC_GATHER_OPT
	       block[j] = get_cabac_bypass_sign( CC, -1);
	       sde_tmp_blk[sj] = block[j];
	       block[j] = (block[j] * qmul[j] + 32)>>6;
#else
	       block[j] = (get_cabac_bypass_sign( CC, -qmul[j]) + 32) >> 6;;
#endif
            }

            abslevel1++;
        } else {
            int coeff_abs = 2;
            ctx = 5 + FFMIN( 4, abslevelgt1 ) + abs_level_m1_ctx_base;
            while( coeff_abs < 15 && get_cabac( CC, ctx ) ) {
                coeff_abs++;
            }

            if( coeff_abs >= 15 ) {
                int j = 0;
                while( get_cabac_bypass( CC ) ) {
                    j++;
                }

                coeff_abs=1;
                while( j-- ) {
                    coeff_abs += coeff_abs + get_cabac_bypass( CC );
                }
                coeff_abs+= 14;
            }
#ifdef JZC_GATHER_OPT
	    if( get_cabac_bypass( CC ) ) block[j] = -coeff_abs;
	    else                                block[j] =  coeff_abs;
	    sde_tmp_blk[sj]=block[j];
	    if (qmul)
	      block[j] = (block[j] * qmul[j] + 32) >>6;
#else
            if( !qmul ) {
                if( get_cabac_bypass( CC ) ) block[j] = -coeff_abs;
                else                                block[j] =  coeff_abs;
            }else{
                if( get_cabac_bypass( CC ) ) block[j] = (-coeff_abs * qmul[j] + 32) >> 6;
                else                                block[j] = ( coeff_abs * qmul[j] + 32) >> 6;
            }
#endif
            abslevelgt1++;
        }
    }

#ifdef JZC_GATHER_OPT
    if ( cat == 3 ) {
      if ( n == 1 ) {
	for (di=0; di<4; di++)
	  sde_v_dc[di] = sde_tmp_blk[di];
      } else {
	for (di=0; di<4; di++)
	  sde_res_dct[sde_res_di + di] = sde_tmp_blk[di];
	sde_res_di += 4;
      }
    } else {
      if ( cat == 5 ) {
	for (di=0; di<64; di++)
	  sde_res_dct[sde_res_di + di] = sde_tmp_blk[di];
	sde_res_di += 64;
      } else {
	  for (di=0; di<16; di++)
	    sde_res_dct[sde_res_di + di] = sde_tmp_blk[di];
	  sde_res_di += 16;
      }
    }
#endif

#ifdef CABAC_ON_STACK
            h->cabac.range     = cc.range     ;
            h->cabac.low       = cc.low       ;
            h->cabac.bytestream= cc.bytestream;
#endif

}

static inline void write_back_non_zero_count_hw(H264Context *h){
    MpegEncContext * const s = &h->s;
    const int mb_xy= s->mb_x + s->mb_y*s->mb_stride;

    h->non_zero_count[mb_xy][0]= h->non_zero_count_cache[7+8*1];
    h->non_zero_count[mb_xy][1]= h->non_zero_count_cache[7+8*2];
    h->non_zero_count[mb_xy][2]= h->non_zero_count_cache[7+8*3];
    h->non_zero_count[mb_xy][3]= h->non_zero_count_cache[7+8*4];
    h->non_zero_count[mb_xy][4]= h->non_zero_count_cache[4+8*4];
    h->non_zero_count[mb_xy][5]= h->non_zero_count_cache[5+8*4];
    h->non_zero_count[mb_xy][6]= h->non_zero_count_cache[6+8*4];

    h->non_zero_count[mb_xy][9]= h->non_zero_count_cache[1+8*2];
    h->non_zero_count[mb_xy][8]= h->non_zero_count_cache[2+8*2];
    h->non_zero_count[mb_xy][7]= h->non_zero_count_cache[2+8*1];

    h->non_zero_count[mb_xy][12]=h->non_zero_count_cache[1+8*5];
    h->non_zero_count[mb_xy][11]=h->non_zero_count_cache[2+8*5];
    h->non_zero_count[mb_xy][10]=h->non_zero_count_cache[2+8*4];

    if(FRAME_MBAFF){
        // store all luma nnzs, for deblocking
        int v = 0, i;
        for(i=0; i<16; i++)
            v += (!!h->non_zero_count_cache[scan8[i]]) << i;
        *(uint16_t*)&h->non_zero_count[mb_xy][14] = v;
    }
}


int ff_h264_decode_mb_cabac_hw(H264Context *h) {
    MpegEncContext * const s = &h->s;
    const int mb_xy= s->mb_x + s->mb_y*s->mb_stride;
    int mb_type, partition_count, cbp = 0;
    int dct8x8_allowed= h->pps.transform_8x8_mode;

#ifdef JZC_GATHER_OPT
    {
      sde_res_di = 0;
      int di;
      for (di=0; di<408; di++)
	sde_res_dct[di] = 0;
      for (di=0; di<27; di++)
	sde_nnz_dct[di] = 0;
      for (di=0; di<4; di++)
	sde_v_dc[di] = 0;
    }
#endif

    s->dsp.clear_blocks(h->mb); //FIXME avoid if already clear (move after skip handlong?)

    tprintf(s->avctx, "pic:%d mb:%d/%d\n", h->frame_num, s->mb_x, s->mb_y);
    if( h->slice_type != FF_I_TYPE && h->slice_type != FF_SI_TYPE ) {
        int skip;
        /* a skipped mb needs the aff flag from the following mb */
        if( FRAME_MBAFF && s->mb_x==0 && (s->mb_y&1)==0 )
            predict_field_decoding_flag_hw(h);
        if( FRAME_MBAFF && (s->mb_y&1)==1 && h->prev_mb_skipped )
            skip = h->next_mb_skipped;
        else
            skip = decode_cabac_mb_skip_hw( h, s->mb_x, s->mb_y );

        /* read skip flags */
        if( skip ) {
            if( FRAME_MBAFF && (s->mb_y&1)==0 ){
                s->current_picture.mb_type[mb_xy] = MB_TYPE_SKIP;
                h->next_mb_skipped = decode_cabac_mb_skip_hw( h, s->mb_x, s->mb_y+1 );
                if(h->next_mb_skipped)
                    predict_field_decoding_flag_hw(h);
                else
                    h->mb_mbaff = h->mb_field_decoding_flag = decode_cabac_field_decoding_flag_hw(h);
            }

            decode_mb_skip_hw(h);
            h->cbp_table[mb_xy] = 0;
            h->chroma_pred_mode_table[mb_xy] = 0;
            h->last_qscale_diff = 0;
            return 0;

        }
    }

    if(FRAME_MBAFF){
        if( (s->mb_y&1) == 0 )
            h->mb_mbaff =
            h->mb_field_decoding_flag = decode_cabac_field_decoding_flag_hw(h);
    }else
        h->mb_field_decoding_flag= (s->picture_structure!=PICT_FRAME);

    h->prev_mb_skipped = 0;
    compute_mb_neighbors(h);
    if( ( mb_type = decode_cabac_mb_type_hw( h ) ) < 0 ) {
        av_log( h->s.avctx, AV_LOG_ERROR, "decode_cabac_mb_type failed\n" );
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
       assert(h->slice_type == FF_I_TYPE);
decode_intra_mb:
        partition_count = 0;
        cbp= i_mb_type_info[mb_type].cbp;
        h->intra16x16_pred_mode= i_mb_type_info[mb_type].pred_mode;
        mb_type= i_mb_type_info[mb_type].type;
    }
    if(MB_FIELD)
        mb_type |= MB_TYPE_INTERLACED;

    h->hw_slice_table[ mb_xy ]= h->slice_num;
    if(IS_INTRA_PCM(mb_type)) {
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
        h->cbp_table[mb_xy] = 0x1ef;
        h->chroma_pred_mode_table[mb_xy] = 0;
        // In deblocking, the quantizer is 0
        s->current_picture.qscale_table[mb_xy]= 0;
        h->chroma_qp[0] = get_chroma_qp_hw(h, 0, 0);
        h->chroma_qp[1] = get_chroma_qp_hw(h, 1, 0);
        // All coeffs are present
        memset(h->non_zero_count[mb_xy], 16, 16);
        s->current_picture.mb_type[mb_xy]= mb_type;
        return 0;
    }

    if(MB_MBAFF){
        h->ref_count[0] <<= 1;
        h->ref_count[1] <<= 1;
    }

    fill_caches_hw(h, mb_type, 0);
    if( IS_INTRA( mb_type ) ) {
        int i, pred_mode;
        if( IS_INTRA4x4( mb_type ) ) {
            if( dct8x8_allowed && decode_cabac_mb_transform_size_hw( h ) ) {
                mb_type |= MB_TYPE_8x8DCT;
                for( i = 0; i < 16; i+=4 ) {
                    int pred = pred_intra_mode( h, i );
                    int mode = decode_cabac_mb_intra4x4_pred_mode_hw( h, pred );
                    fill_rectangle_hw( &h->intra4x4_pred_mode_cache[ scan8[i] ], 2, 2, 8, mode, 1 );
                }
            } else {
                for( i = 0; i < 16; i++ ) {
                    int pred = pred_intra_mode( h, i );
                    h->intra4x4_pred_mode_cache[ scan8[i] ] = decode_cabac_mb_intra4x4_pred_mode_hw( h, pred );

                //av_log( s->avctx, AV_LOG_ERROR, "i4x4 pred=%d mode=%d\n", pred, h->intra4x4_pred_mode_cache[ scan8[i] ] );
                }
            }
#ifdef JZC_GATHER_OPT
	    sde_luma_neighbor = ((((unsigned int)(h->intra4x4_pred_mode_cache[ scan8[10] ]) & 0xFF) << 4) +
				 ( (unsigned int)(h->intra4x4_pred_mode_cache[ scan8[ 5] ]) & 0xFF));
#endif
            write_back_intra_pred_mode_hw(h);
            if( check_intra4x4_pred_mode_hw(h) < 0 ) return -1;
        } else {
            h->intra16x16_pred_mode= check_intra_pred_mode_hw( h, h->intra16x16_pred_mode );
            if( h->intra16x16_pred_mode < 0 ) return -1;
        }
#ifdef JZC_GATHER_OPT
	sde_chroma_neighbor              =
#endif
        h->chroma_pred_mode_table[mb_xy] =
        pred_mode                        = decode_cabac_mb_chroma_pre_mode_hw( h );

        pred_mode= check_intra_pred_mode_hw( h, pred_mode );
        if( pred_mode < 0 ) return -1;
        h->chroma_pred_mode= pred_mode;
    } else if( partition_count == 4 ) {
        int i, j, sub_partition_count[4], list, ref[2][4];
        if( h->slice_type == FF_B_TYPE ) {
            for( i = 0; i < 4; i++ ) {
                h->sub_mb_type[i] = decode_cabac_b_mb_sub_type_hw( h );
                sub_partition_count[i]= b_sub_mb_type_info[ h->sub_mb_type[i] ].partition_count;
                h->sub_mb_type[i]=      b_sub_mb_type_info[ h->sub_mb_type[i] ].type;
            }
            if( IS_DIRECT(h->sub_mb_type[0] | h->sub_mb_type[1] |
                          h->sub_mb_type[2] | h->sub_mb_type[3]) ) {
                pred_direct_motion_hw(h, &mb_type);
                h->ref_cache[0][scan8[4]] =
                h->ref_cache[1][scan8[4]] =
                h->ref_cache[0][scan8[12]] =
                h->ref_cache[1][scan8[12]] = PART_NOT_AVAILABLE;
                if( h->ref_count[0] > 1 || h->ref_count[1] > 1 ) {
                    for( i = 0; i < 4; i++ )
                        if( IS_DIRECT(h->sub_mb_type[i]) )
                            fill_rectangle_hw( &h->direct_cache[scan8[4*i]], 2, 2, 8, 1, 1 );
                }
            }
        } else {
            for( i = 0; i < 4; i++ ) {
                h->sub_mb_type[i] = decode_cabac_p_mb_sub_type_hw( h );
                sub_partition_count[i]= p_sub_mb_type_info[ h->sub_mb_type[i] ].partition_count;
                h->sub_mb_type[i]=      p_sub_mb_type_info[ h->sub_mb_type[i] ].type;
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

        if(dct8x8_allowed)
            dct8x8_allowed = get_dct8x8_allowed(h);

        for(list=0; list<h->list_count; list++){
            for(i=0; i<4; i++){
                h->ref_cache[list][ scan8[4*i]   ]=h->ref_cache[list][ scan8[4*i]+1 ];
                if(IS_DIRECT(h->sub_mb_type[i])){
                    fill_rectangle_hw(h->mvd_cache[list][scan8[4*i]], 2, 2, 8, 0, 4);
                    continue;
                }

                if(IS_DIR(h->sub_mb_type[i], 0, list) && !IS_DIRECT(h->sub_mb_type[i])){
                    const int sub_mb_type= h->sub_mb_type[i];
                    const int block_width= (sub_mb_type & (MB_TYPE_16x16|MB_TYPE_16x8)) ? 2 : 1;
                    for(j=0; j<sub_partition_count[i]; j++){
                        int mpx, mpy;
                        int mx, my;
                        const int index= 4*i + block_width*j;
                        int16_t (* mv_cache)[2]= &h->mv_cache[list][ scan8[index] ];
                        int16_t (* mvd_cache)[2]= &h->mvd_cache[list][ scan8[index] ];
                        pred_motion_hw(h, index, block_width, list, h->ref_cache[list][ scan8[index] ], &mpx, &mpy);

                        mx = mpx + decode_cabac_mb_mvd_hw( h, list, index, 0 );
                        my = mpy + decode_cabac_mb_mvd_hw( h, list, index, 1 );
                        tprintf(s->avctx, "final mv:%d %d\n", mx, my);

                        if(IS_SUB_8X8(sub_mb_type)){
                            mv_cache[ 1 ][0]=
                            mv_cache[ 8 ][0]= mv_cache[ 9 ][0]= mx;
                            mv_cache[ 1 ][1]=
                            mv_cache[ 8 ][1]= mv_cache[ 9 ][1]= my;

                            mvd_cache[ 1 ][0]=
                            mvd_cache[ 8 ][0]= mvd_cache[ 9 ][0]= mx - mpx;
                            mvd_cache[ 1 ][1]=
                            mvd_cache[ 8 ][1]= mvd_cache[ 9 ][1]= my - mpy;
                        }else if(IS_SUB_8X4(sub_mb_type)){
                            mv_cache[ 1 ][0]= mx;
                            mv_cache[ 1 ][1]= my;

                            mvd_cache[ 1 ][0]= mx - mpx;
                            mvd_cache[ 1 ][1]= my - mpy;
                        }else if(IS_SUB_4X8(sub_mb_type)){
                            mv_cache[ 8 ][0]= mx;
                            mv_cache[ 8 ][1]= my;

                            mvd_cache[ 8 ][0]= mx - mpx;
                            mvd_cache[ 8 ][1]= my - mpy;
                        }
                        mv_cache[ 0 ][0]= mx;
                        mv_cache[ 0 ][1]= my;

                        mvd_cache[ 0 ][0]= mx - mpx;
                        mvd_cache[ 0 ][1]= my - mpy;
                    }
                }else{
                    uint32_t *p= (uint32_t *)&h->mv_cache[list][ scan8[4*i] ][0];
                    uint32_t *pd= (uint32_t *)&h->mvd_cache[list][ scan8[4*i] ][0];
                    p[0] = p[1] = p[8] = p[9] = 0;
                    pd[0]= pd[1]= pd[8]= pd[9]= 0;
                }
            }
        }
    } else if( IS_DIRECT(mb_type) ) {
        pred_direct_motion_hw(h, &mb_type);
        fill_rectangle_hw(h->mvd_cache[0][scan8[0]], 4, 4, 8, 0, 4);
        fill_rectangle_hw(h->mvd_cache[1][scan8[0]], 4, 4, 8, 0, 4);
        dct8x8_allowed &= h->sps.direct_8x8_inference_flag;
    } else {
        int list, mx, my, i, mpx, mpy;
        if(IS_16X16(mb_type)){
            for(list=0; list<h->list_count; list++){
                if(IS_DIR(mb_type, 0, list)){
                    const int ref = h->ref_count[list] > 1 ? decode_cabac_mb_ref_hw( h, list, 0 ) : 0;
                    fill_rectangle_hw(&h->ref_cache[list][ scan8[0] ], 4, 4, 8, ref, 1);
                }else
		    fill_rectangle_hw(&h->ref_cache[list][ scan8[0] ], 4, 4, 8, (uint8_t)LIST_NOT_USED, 1); //FIXME factorize and the other fill_rect below too
            }
            for(list=0; list<h->list_count; list++){
                if(IS_DIR(mb_type, 0, list)){
                    pred_motion_hw(h, 0, 4, list, h->ref_cache[list][ scan8[0] ], &mpx, &mpy);

                    mx = mpx + decode_cabac_mb_mvd_hw( h, list, 0, 0 );
                    my = mpy + decode_cabac_mb_mvd_hw( h, list, 0, 1 );
                    tprintf(s->avctx, "final mv:%d %d\n", mx, my);
 
                    fill_rectangle_hw(h->mvd_cache[list][ scan8[0] ], 4, 4, 8, pack16to32(mx-mpx,my-mpy), 4);
                    fill_rectangle_hw(h->mv_cache[list][ scan8[0] ], 4, 4, 8, pack16to32(mx,my), 4);
                }else{
                    fill_rectangle_hw(h->mv_cache[list][ scan8[0] ], 4, 4, 8, 0, 4);
		    fill_rectangle_hw(h->mvd_cache[list][ scan8[0] ], 4, 4, 8, 0, 4);
                }
            }
        }
        else if(IS_16X8(mb_type)){
            for(list=0; list<h->list_count; list++){
                    for(i=0; i<2; i++){
                        if(IS_DIR(mb_type, i, list)){
                            const int ref= h->ref_count[list] > 1 ? decode_cabac_mb_ref_hw( h, list, 8*i ) : 0;
                            fill_rectangle_hw(&h->ref_cache[list][ scan8[0] + 16*i ], 4, 2, 8, ref, 1);
                        }else
                            fill_rectangle_hw(&h->ref_cache[list][ scan8[0] + 16*i ], 4, 2, 8, (LIST_NOT_USED&0xFF), 1);
                    }
            }
            for(list=0; list<h->list_count; list++){
                for(i=0; i<2; i++){
                    if(IS_DIR(mb_type, i, list)){
                        pred_16x8_motion_hw(h, 8*i, list, h->ref_cache[list][scan8[0] + 16*i], &mpx, &mpy);
                        mx = mpx + decode_cabac_mb_mvd_hw( h, list, 8*i, 0 );
                        my = mpy + decode_cabac_mb_mvd_hw( h, list, 8*i, 1 );
                        tprintf(s->avctx, "final mv:%d %d\n", mx, my);

                        fill_rectangle_hw(h->mvd_cache[list][ scan8[0] + 16*i ], 4, 2, 8, pack16to32(mx-mpx,my-mpy), 4);
                        fill_rectangle_hw(h->mv_cache[list][ scan8[0] + 16*i ], 4, 2, 8, pack16to32(mx,my), 4);
                    }else{
                        fill_rectangle_hw(h->mvd_cache[list][ scan8[0] + 16*i ], 4, 2, 8, 0, 4);
                        fill_rectangle_hw(h-> mv_cache[list][ scan8[0] + 16*i ], 4, 2, 8, 0, 4);
                    }
                }
            }
        }else{
            assert(IS_8X16(mb_type));
            for(list=0; list<h->list_count; list++){
                    for(i=0; i<2; i++){
                        if(IS_DIR(mb_type, i, list)){ //FIXME optimize
                            const int ref= h->ref_count[list] > 1 ? decode_cabac_mb_ref_hw( h, list, 4*i ) : 0;
                            fill_rectangle_hw(&h->ref_cache[list][ scan8[0] + 2*i ], 2, 4, 8, ref, 1);
                        }else
                            fill_rectangle_hw(&h->ref_cache[list][ scan8[0] + 2*i ], 2, 4, 8, (LIST_NOT_USED&0xFF), 1);
                    }
            }
            for(list=0; list<h->list_count; list++){
                for(i=0; i<2; i++){
                    if(IS_DIR(mb_type, i, list)){
                        pred_8x16_motion_hw(h, i*4, list, h->ref_cache[list][ scan8[0] + 2*i ], &mpx, &mpy);
                        mx = mpx + decode_cabac_mb_mvd_hw( h, list, 4*i, 0 );
                        my = mpy + decode_cabac_mb_mvd_hw( h, list, 4*i, 1 );

                        tprintf(s->avctx, "final mv:%d %d\n", mx, my);
                        fill_rectangle_hw(h->mvd_cache[list][ scan8[0] + 2*i ], 2, 4, 8, pack16to32(mx-mpx,my-mpy), 4);
                        fill_rectangle_hw(h->mv_cache[list][ scan8[0] + 2*i ], 2, 4, 8, pack16to32(mx,my), 4);
                    }else{
                        fill_rectangle_hw(h->mvd_cache[list][ scan8[0] + 2*i ], 2, 4, 8, 0, 4);
                        fill_rectangle_hw(h-> mv_cache[list][ scan8[0] + 2*i ], 2, 4, 8, 0, 4);
                    }
                }
            }
        }
    }
    
    if( IS_INTER( mb_type ) ) {
        h->chroma_pred_mode_table[mb_xy] = 0;
        write_back_motion_hw( h, mb_type );
    }


    if( !IS_INTRA16x16( mb_type ) ) {
        cbp  = decode_cabac_mb_cbp_luma_hw( h );
        cbp |= decode_cabac_mb_cbp_chroma_hw( h ) << 4;
    }

    h->cbp_table[mb_xy] = h->cbp = cbp;

    if( dct8x8_allowed && (cbp&15) && !IS_INTRA( mb_type ) ) {
        if( decode_cabac_mb_transform_size_hw( h ) )
            mb_type |= MB_TYPE_8x8DCT;
    }
    s->current_picture.mb_type[mb_xy]= mb_type;

    if( cbp || IS_INTRA16x16( mb_type ) ) {
        const uint8_t *scan, *scan8x8, *dc_scan;
        const uint32_t *qmul;
        int dqp;

        if(IS_INTERLACED(mb_type)){
            scan8x8= s->qscale ? h->field_scan8x8 : h->field_scan8x8_q0;
            scan= s->qscale ? h->field_scan : h->field_scan_q0;
            dc_scan= luma_dc_field_scan;
        }else{
            scan8x8= s->qscale ? h->zigzag_scan8x8 : h->zigzag_scan8x8_q0;
            scan= s->qscale ? h->zigzag_scan : h->zigzag_scan_q0;
            dc_scan= luma_dc_zigzag_scan;
        }

        h->last_qscale_diff = dqp = decode_cabac_mb_dqp_hw( h );
        if( dqp == INT_MIN ){
            av_log(h->s.avctx, AV_LOG_ERROR, "cabac decode of qscale diff failed at %d %d\n", s->mb_x, s->mb_y);
            return -1;
        }
        s->qscale += dqp;
        if(((unsigned)s->qscale) > 51){
            if(s->qscale<0) s->qscale+= 52;
            else            s->qscale-= 52;
        }
        h->chroma_qp[0] = get_chroma_qp_hw(h, 0, s->qscale);
        h->chroma_qp[1] = get_chroma_qp_hw(h, 1, s->qscale);

        if( IS_INTRA16x16( mb_type ) ) {
            int i;
            //av_log( s->avctx, AV_LOG_ERROR, "INTRA16x16 DC\n" );
            decode_cabac_residual_hw( h, h->mb, 0, 0, dc_scan, NULL, 16);

            if( cbp&15 ) {
                qmul = h->dequant4_coeff[0][s->qscale];
                for( i = 0; i < 16; i++ ) {
                    //av_log( s->avctx, AV_LOG_ERROR, "INTRA16x16 AC:%d\n", i );
                    decode_cabac_residual_hw(h, h->mb + 16*i, 1, i, scan + 1, qmul, 15);
                }
            } else {
                fill_rectangle_hw(&h->non_zero_count_cache[scan8[0]], 4, 4, 8, 0, 1);
            }
        } else {
            int i8x8, i4x4;
            for( i8x8 = 0; i8x8 < 4; i8x8++ ) {
                if( cbp & (1<<i8x8) ) {
                    if( IS_8x8DCT(mb_type) ) {
                        decode_cabac_residual_hw(h, h->mb + 64*i8x8, 5, 4*i8x8,
                            scan8x8, h->dequant8_coeff[IS_INTRA( mb_type ) ? 0:1][s->qscale], 64);
                    } else {
                        qmul = h->dequant4_coeff[IS_INTRA( mb_type ) ? 0:3][s->qscale];
                        for( i4x4 = 0; i4x4 < 4; i4x4++ ) {
                            const int index = 4*i8x8 + i4x4;
                            //av_log( s->avctx, AV_LOG_ERROR, "Luma4x4: %d\n", index );
//START_TIMER
                            decode_cabac_residual_hw(h, h->mb + 16*index, 2, index, scan, qmul, 16);
//STOP_TIMER("decode_residual")
                        }
                    }
                } else {
                    uint8_t * const nnz= &h->non_zero_count_cache[ scan8[4*i8x8] ];
                    nnz[0] = nnz[1] = nnz[8] = nnz[9] = 0;
                }
            }
        }

        if( cbp&0x30 ){
            int c;
            for( c = 0; c < 2; c++ ) {
                //av_log( s->avctx, AV_LOG_ERROR, "INTRA C%d-DC\n",c );
                decode_cabac_residual_hw(h, h->mb + 256 + 16*4*c, 3, c, chroma_dc_scan, NULL, 4);
            }
        }

        if( cbp&0x20 ) {
            int c, i;
            for( c = 0; c < 2; c++ ) {
                qmul = h->dequant4_coeff[c+1+(IS_INTRA( mb_type ) ? 0:3)][h->chroma_qp[c]];
                for( i = 0; i < 4; i++ ) {
                    const int index = 16 + 4 * c + i;
                    //av_log( s->avctx, AV_LOG_ERROR, "INTRA C%d-AC %d\n",c, index - 16 );
                    decode_cabac_residual_hw(h, h->mb + 16*index, 4, index - 16, scan + 1, qmul, 15);
                }
#ifdef JZC_GATHER_OPT
		if (c == 0){
		  int di;
		  if (sde_nnz_dct[26]){
		    for (di=0; di<4; di++)
		      sde_res_dct[sde_res_di + di] = sde_v_dc[di];
		    sde_res_di += 4;
		  }
		}
#endif
            }
        } else {
#ifdef JZC_GATHER_OPT
	  {
	    int di;
	    if (sde_nnz_dct[26]){
	      for (di=0; di<4; di++)
		sde_res_dct[sde_res_di + di] = sde_v_dc[di];
	      sde_res_di += 4;
	    }
	  }
#endif
	  uint8_t * const nnz= &h->non_zero_count_cache[0];
	  nnz[ scan8[16]+0 ] = nnz[ scan8[16]+1 ] =nnz[ scan8[16]+8 ] =nnz[ scan8[16]+9 ] =
            nnz[ scan8[20]+0 ] = nnz[ scan8[20]+1 ] =nnz[ scan8[20]+8 ] =nnz[ scan8[20]+9 ] = 0;
        }
    } else {
        uint8_t * const nnz= &h->non_zero_count_cache[0];
        fill_rectangle_hw(&nnz[scan8[0]], 4, 4, 8, 0, 1);
        nnz[ scan8[16]+0 ] = nnz[ scan8[16]+1 ] =nnz[ scan8[16]+8 ] =nnz[ scan8[16]+9 ] =
        nnz[ scan8[20]+0 ] = nnz[ scan8[20]+1 ] =nnz[ scan8[20]+8 ] =nnz[ scan8[20]+9 ] = 0;
        h->last_qscale_diff = 0;
    }

    s->current_picture.qscale_table[mb_xy]= s->qscale;
    write_back_non_zero_count_hw(h);
    if(MB_MBAFF){
        h->ref_count[0] >>= 1;
        h->ref_count[1] >>= 1;
    }
    return 0;
}
