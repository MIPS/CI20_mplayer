/**
 * @file
 * VP5 and VP6 compatible video decoder (common features)
 *
 * Copyright (C) 2006  Aurelien Jacobs <aurel@gnuage.org>
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

#include "avcodec.h"
#include "bytestream.h"

#include "vp56.h"
#include "vp56data.h"

#define JZC_PMON_P0N
#define STA_CCLK
#include "../libjzcommon/jz4760e_pmon.h"
PMON_CREAT(test1);
PMON_CREAT(test2);
PMON_CREAT(test3);

#define JZC_CRC_VERN
#ifdef JZC_CRC_VER
short crc_code;
short mpFrame;
#include "../libjzcommon/crc.c"
#undef printf
#undef fprintf
#endif

#include "vp6_tcsm0.h"
#include "../libjzcommon/jzasm.h"
#include "../libjzcommon/jzmedia.h"
#include "../libjzcommon/jz4760e_2ddma_hw.h"
uint8_t idct_row[6];
short *blk_coeff;
#undef printf

void ff_vp56_init_dequant(VP56Context *s, int quantizer)
{
    s->quantizer = quantizer;
    s->dequant_dc = vp56_dc_dequant[quantizer] << 2;
    s->dequant_ac = vp56_ac_dequant[quantizer] << 2;
    memset(s->qscale_table, quantizer, s->mb_width);
}

static int vp56_get_vectors_predictors(VP56Context *s, int row, int col,
                                       VP56Frame ref_frame)
{
    int nb_pred = 0;
    VP56mv vect[2] = {{0,0}, {0,0}};
    int pos, offset;
    VP56mv mvp;

    for (pos=0; pos<12; pos++) {
        mvp.x = col + vp56_candidate_predictor_pos[pos][0];
        mvp.y = row + vp56_candidate_predictor_pos[pos][1];
        if (mvp.x < 0 || mvp.x >= s->mb_width ||
            mvp.y < 0 || mvp.y >= s->mb_height)
            continue;
        offset = mvp.x + s->mb_width*mvp.y;

        if (vp56_reference_frame[s->macroblocks[offset].type] != ref_frame)
            continue;
        if ((s->macroblocks[offset].mv.x == vect[0].x &&
             s->macroblocks[offset].mv.y == vect[0].y) ||
            (s->macroblocks[offset].mv.x == 0 &&
             s->macroblocks[offset].mv.y == 0))
            continue;

        vect[nb_pred++] = s->macroblocks[offset].mv;
        if (nb_pred > 1) {
            nb_pred = -1;
            break;
        }
        s->vector_candidate_pos = pos;
    }

    s->vector_candidate[0] = vect[0];
    s->vector_candidate[1] = vect[1];

    return nb_pred+1;
}

static void vp56_parse_mb_type_models(VP56Context *s)
{
    VP56RangeCoder *c = &s->c;
    VP56Model *model = s->modelp;
    int i, ctx, type;

    for (ctx=0; ctx<3; ctx++) {
        if (vp56_rac_get_prob(c, 174)) {
            int idx = vp56_rac_gets(c, 4);
            memcpy(model->mb_types_stats[ctx],
                   vp56_pre_def_mb_type_stats[idx][ctx],
                   sizeof(model->mb_types_stats[ctx]));
        }
        if (vp56_rac_get_prob(c, 254)) {
            for (type=0; type<10; type++) {
                for(i=0; i<2; i++) {
                    if (vp56_rac_get_prob(c, 205)) {
                        int delta, sign = vp56_rac_get(c);

                        delta = vp56_rac_get_tree(c, vp56_pmbtm_tree,
                                                  vp56_mb_type_model_model);
                        if (!delta)
                            delta = 4 * vp56_rac_gets(c, 7);
                        model->mb_types_stats[ctx][type][i] += (delta ^ -sign) + sign;
                    }
                }
            }
        }
    }

    /* compute MB type probability tables based on previous MB type */
    for (ctx=0; ctx<3; ctx++) {
        int p[10];

        for (type=0; type<10; type++)
            p[type] = 100 * model->mb_types_stats[ctx][type][1];

        for (type=0; type<10; type++) {
            int p02, p34, p0234, p17, p56, p89, p5689, p156789;

            /* conservative MB type probability */
            model->mb_type[ctx][type][0] = 255 - (255 * model->mb_types_stats[ctx][type][0]) / (1 + model->mb_types_stats[ctx][type][0] + model->mb_types_stats[ctx][type][1]);

            p[type] = 0;    /* same MB type => weight is null */

            /* binary tree parsing probabilities */
            p02 = p[0] + p[2];
            p34 = p[3] + p[4];
            p0234 = p02 + p34;
            p17 = p[1] + p[7];
            p56 = p[5] + p[6];
            p89 = p[8] + p[9];
            p5689 = p56 + p89;
            p156789 = p17 + p5689;

            model->mb_type[ctx][type][1] = 1 + 255 * p0234/(1+p0234+p156789);
            model->mb_type[ctx][type][2] = 1 + 255 * p02  / (1+p0234);
            model->mb_type[ctx][type][3] = 1 + 255 * p17  / (1+p156789);
            model->mb_type[ctx][type][4] = 1 + 255 * p[0] / (1+p02);
            model->mb_type[ctx][type][5] = 1 + 255 * p[3] / (1+p34);
            model->mb_type[ctx][type][6] = 1 + 255 * p[1] / (1+p17);
            model->mb_type[ctx][type][7] = 1 + 255 * p56  / (1+p5689);
            model->mb_type[ctx][type][8] = 1 + 255 * p[5] / (1+p56);
            model->mb_type[ctx][type][9] = 1 + 255 * p[8] / (1+p89);

            /* restore initial value */
            p[type] = 100 * model->mb_types_stats[ctx][type][1];
        }
    }
}

static VP56mb vp56_parse_mb_type(VP56Context *s,
                                 VP56mb prev_type, int ctx)
{
    uint8_t *mb_type_model = s->modelp->mb_type[ctx][prev_type];
    VP56RangeCoder *c = &s->c;

    if (vp56_rac_get_prob(c, mb_type_model[0]))
        return prev_type;
    else
        return vp56_rac_get_tree(c, vp56_pmbt_tree, mb_type_model);
}

static void vp56_decode_4mv(VP56Context *s, int row, int col)
{
    VP56mv mv = {0,0};
    int type[4];
    int b;

    /* parse each block type */
    for (b=0; b<4; b++) {
        type[b] = vp56_rac_gets(&s->c, 2);
        if (type[b])
            type[b]++;  /* only returns 0, 2, 3 or 4 (all INTER_PF) */
    }

    /* get vectors */
    for (b=0; b<4; b++) {
        switch (type[b]) {
            case VP56_MB_INTER_NOVEC_PF:
                s->mv[b] = (VP56mv) {0,0};
                break;
            case VP56_MB_INTER_DELTA_PF:
                s->parse_vector_adjustment(s, &s->mv[b]);
                break;
            case VP56_MB_INTER_V1_PF:
                s->mv[b] = s->vector_candidate[0];
                break;
            case VP56_MB_INTER_V2_PF:
                s->mv[b] = s->vector_candidate[1];
                break;
        }
        mv.x += s->mv[b].x;
        mv.y += s->mv[b].y;
    }

    /* this is the one selected for the whole MB for prediction */
    s->macroblocks[row * s->mb_width + col].mv = s->mv[3];

    /* chroma vectors are average luma vectors */
    if (s->avctx->codec->id == CODEC_ID_VP5) {
        s->mv[4].x = s->mv[5].x = RSHIFT(mv.x,2);
        s->mv[4].y = s->mv[5].y = RSHIFT(mv.y,2);
    } else {
        s->mv[4] = s->mv[5] = (VP56mv) {mv.x/4, mv.y/4};
    }
}

static VP56mb vp56_decode_mv(VP56Context *s, int row, int col)
{
    VP56mv *mv, vect = {0,0};
    int ctx, b;

    ctx = vp56_get_vectors_predictors(s, row, col, VP56_FRAME_PREVIOUS);
    s->mb_type = vp56_parse_mb_type(s, s->mb_type, ctx);
    s->macroblocks[row * s->mb_width + col].type = s->mb_type;

    switch (s->mb_type) {
        case VP56_MB_INTER_V1_PF:
            mv = &s->vector_candidate[0];
            break;

        case VP56_MB_INTER_V2_PF:
            mv = &s->vector_candidate[1];
            break;

        case VP56_MB_INTER_V1_GF:
            vp56_get_vectors_predictors(s, row, col, VP56_FRAME_GOLDEN);
            mv = &s->vector_candidate[0];
            break;

        case VP56_MB_INTER_V2_GF:
            vp56_get_vectors_predictors(s, row, col, VP56_FRAME_GOLDEN);
            mv = &s->vector_candidate[1];
            break;

        case VP56_MB_INTER_DELTA_PF:
            s->parse_vector_adjustment(s, &vect);
            mv = &vect;
            break;

        case VP56_MB_INTER_DELTA_GF:
            vp56_get_vectors_predictors(s, row, col, VP56_FRAME_GOLDEN);
            s->parse_vector_adjustment(s, &vect);
            mv = &vect;
            break;

        case VP56_MB_INTER_4V:
            vp56_decode_4mv(s, row, col);
            return s->mb_type;

        default:
            mv = &vect;
            break;
    }

    s->macroblocks[row*s->mb_width + col].mv = *mv;

    /* same vector for all blocks */
    for (b=0; b<6; b++)
        s->mv[b] = *mv;

    return s->mb_type;
}

static void vp56_add_predictors_dc(VP56Context *s, VP56Frame ref_frame)
{
    int idx = s->scantable.permutated[0];
    int b;

    for (b=0; b<6; b++) {
        VP56RefDc *ab = &s->above_blocks[s->above_block_idx[b]];
        VP56RefDc *lb = &s->left_block[vp56_b6to4[b]];
        int count = 0;
        int dc = 0;
        int i;

        if (ref_frame == lb->ref_frame) {
            dc += lb->dc_coeff;
            count++;
        }
        if (ref_frame == ab->ref_frame) {
            dc += ab->dc_coeff;
            count++;
        }
        if (s->avctx->codec->id == CODEC_ID_VP5)
            for (i=0; i<2; i++)
                if (count < 2 && ref_frame == ab[-1+2*i].ref_frame) {
                    dc += ab[-1+2*i].dc_coeff;
                    count++;
                }
        if (count == 0)
            dc = s->prev_dc[vp56_b2p[b]][ref_frame];
        else if (count == 2)
            dc /= 2;
        *(blk_coeff + b*64 + idx) += dc;
        s->prev_dc[vp56_b2p[b]][ref_frame] = *(blk_coeff + b*64 + idx);
        ab->dc_coeff = *(blk_coeff + b*64 + idx);
        ab->ref_frame = ref_frame;
        lb->dc_coeff = *(blk_coeff + b*64 + idx);
        lb->ref_frame = ref_frame;
        *(blk_coeff + b*64 + idx) *= s->dequant_dc;
    }
}

static void vp56_deblock_filter(VP56Context *s, uint8_t *yuv,
                                int stride, int dx, int dy)
{
    int t = vp56_filter_threshold[s->quantizer];
    if (dx)  s->vp56dsp.edge_filter_hor(yuv +         10-dx , stride, t);
    if (dy)  s->vp56dsp.edge_filter_ver(yuv + stride*(10-dy), stride, t);
}

static void MC_put_o_8_c (uint8_t *dest, const uint8_t *ref, const int stride, int height)
{ 
    uint32_t  ref_aln, ref_rs;
    ref_aln = (uint32_t)(ref-stride) & 0xfffffffc;
    ref_rs  = 4 - ((uint32_t)(ref-stride) & 3);
    dest -= stride;
    do {
        S32LDIV(xr1,ref_aln,stride,0x0);
	S32LDD(xr2,ref_aln,0x4);
	S32LDD(xr4,ref_aln,0x8);
	S32ALN(xr3,xr2,xr1,ref_rs);
	S32ALN(xr5,xr4,xr2,ref_rs);
	
	S32SDIV(xr3,dest,stride,0x0);
	S32STD(xr5,dest,0x4);
    } while (--height);
}  

static void MC_put_o_12_c (uint8_t *dest, const uint8_t *ref, const int stride, int height)
{
    uint32_t  ref_aln, ref_rs;
    ref_aln = ((uint32_t)ref - stride) & 0xfffffffc;
    ref_rs  = 4 - (((uint32_t)ref) & 3);
    dest -= stride;
    do {
        S32LDIV(xr1,ref_aln,stride,0x0);
	S32LDD(xr2,ref_aln,0x4);
	S32LDD(xr4,ref_aln,0x8);
	S32LDD(xr6,ref_aln,0xc);
	
	S32ALN(xr3,xr2,xr1,ref_rs);
	S32ALN(xr5,xr4,xr2,ref_rs);
	S32ALN(xr7,xr6,xr4,ref_rs);
	
	S32SDIV(xr3,dest,stride,0x0);
	S32STD(xr5,dest,0x4);
	S32STD(xr7,dest,0x8);
    } while (--height); 
}

#if 1
static void vp56_mc(VP56Context *s, int b, int plane, uint8_t *src,
                    int stride, int x, int y)
{
    uint8_t *dst=s->framep[VP56_FRAME_CURRENT]->data[plane]+s->block_offset[b];
    uint8_t *src_block;
    int src_offset;
    int overlap_offset = 0;
    int mask = s->vp56_coord_div[b] - 1;
    int deblock_filtering = s->deblock_filtering;
    int dx;
    int dy;

    if (s->avctx->skip_loop_filter >= AVDISCARD_ALL ||
        (s->avctx->skip_loop_filter >= AVDISCARD_NONKEY
         && !s->framep[VP56_FRAME_CURRENT]->key_frame))
        deblock_filtering = 0;

    dx = s->mv[b].x / s->vp56_coord_div[b];
    dy = s->mv[b].y / s->vp56_coord_div[b];
    if (b >= 4) {
        x /= 2;
        y /= 2;
    }
    x += dx - 2;
    y += dy - 2;

    if (x<0 || x+12>=s->plane_width[plane] ||
        y<0 || y+12>=s->plane_height[plane]) {
        ff_emulated_edge_mc(s->edge_emu_buffer,
                            src + s->block_offset[b] + (dy-2)*stride + (dx-2),
                            stride, 12, 12, x, y,
                            s->plane_width[plane],
                            s->plane_height[plane]);
        src_block = s->edge_emu_buffer;
        src_offset = 2 + 2*stride;
    } else if (deblock_filtering) {
        /* only need a 12x12 block, but there is no such dsp function, */
        /* so copy a 16x12 block */
        MC_put_o_12_c(s->edge_emu_buffer,
		      src + s->block_offset[b] + (dy-2)*stride + (dx-2),
		      stride, 12);
        src_block = s->edge_emu_buffer;
        src_offset = 2 + 2*stride;
    } else {
        src_block = src;
        src_offset = s->block_offset[b] + dy*stride + dx;
    }

    if (s->mv[b].x & mask)
        overlap_offset += (s->mv[b].x > 0) ? 1 : -1;
    if (s->mv[b].y & mask)
        overlap_offset += (s->mv[b].y > 0) ? stride : -stride;
    
    if (overlap_offset) {
        if (s->filter)
            s->filter(s, dst, src_block, src_offset, src_offset+overlap_offset,
                      stride, s->mv[b], mask, s->filter_selection, b<4);
        else
            s->dsp.put_no_rnd_pixels_l2[1](dst, src_block+src_offset,
                                           src_block+src_offset+overlap_offset,
                                           stride, 8);
    } else {
      MC_put_o_8_c(dst, src_block+src_offset, stride, 8);
    }
}
#else
static void vp56_mc(VP56Context *s, int b, int plane, uint8_t *src,
                    int stride, int x, int y)
{
    uint8_t *dst=s->framep[VP56_FRAME_CURRENT]->data[plane]+s->block_offset[b];
    uint8_t *src_block;
    int src_offset;
    int overlap_offset = 0;
    int mask = s->vp56_coord_div[b] - 1;
    int deblock_filtering = s->deblock_filtering;
    int dx;
    int dy;

    if (s->avctx->skip_loop_filter >= AVDISCARD_ALL ||
        (s->avctx->skip_loop_filter >= AVDISCARD_NONKEY
         && !s->framep[VP56_FRAME_CURRENT]->key_frame))
        deblock_filtering = 0;

    dx = s->mv[b].x / s->vp56_coord_div[b];
    dy = s->mv[b].y / s->vp56_coord_div[b];

    if (b >= 4) {
        x /= 2;
        y /= 2;
    }
    x += dx - 2;
    y += dy - 2;

    if (x<0 || x+12>=s->plane_width[plane] ||
        y<0 || y+12>=s->plane_height[plane]) {
        ff_emulated_edge_mc(s->edge_emu_buffer,
                            src + s->block_offset[b] + (dy-2)*stride + (dx-2),
                            stride, 12, 12, x, y,
                            s->plane_width[plane],
                            s->plane_height[plane]);
        src_block = s->edge_emu_buffer;
        src_offset = 2 + 2*stride;
    } else if (deblock_filtering) {
        /* only need a 12x12 block, but there is no such dsp function, */
        /* so copy a 16x12 block */
        s->dsp.put_pixels_tab[0][0](s->edge_emu_buffer,
                                    src + s->block_offset[b] + (dy-2)*stride + (dx-2),
                                    stride, 12);
        src_block = s->edge_emu_buffer;
        src_offset = 2 + 2*stride;
    } else {
        src_block = src;
        src_offset = s->block_offset[b] + dy*stride + dx;
    }

    if (deblock_filtering)
        vp56_deblock_filter(s, src_block, stride, dx&7, dy&7);

    if (s->mv[b].x & mask)
        overlap_offset += (s->mv[b].x > 0) ? 1 : -1;
    if (s->mv[b].y & mask)
        overlap_offset += (s->mv[b].y > 0) ? stride : -stride;

    if (overlap_offset) {
        if (s->filter)
            s->filter(s, dst, src_block, src_offset, src_offset+overlap_offset,
                      stride, s->mv[b], mask, s->filter_selection, b<4);
        else
            s->dsp.put_no_rnd_pixels_l2[1](dst, src_block+src_offset,
                                           src_block+src_offset+overlap_offset,
                                           stride, 8);
    } else {
        s->dsp.put_pixels_tab[1][0](dst, src_block+src_offset, stride, 8);
    }
}
#endif

#define  wxr5   0x5A827642  //c4c2
#define  wxr6   0x5A8230FC  //c4c6
#define  wxr7   0x7D876A6E  //c1c3
#define  wxr8   0x18F9471D  //c7c5
#define  wxr9   0x6A6E18F9  //c3c7
#define  wxr10  0x471D7D87 //c5c1
static int32_t whirl_idct[6] = {wxr5, wxr6, wxr7, wxr8, wxr9, wxr10};

static void ff_vp3_idct_put_mxu(uint8_t *dst, int stride, DCTELEM *input, uint8_t idct_row)
{ 
    int i;
    DCTELEM  *blk;
    int32_t wf = (int32_t)whirl_idct;
    S32LDD(xr5, wf, 0x0);         // xr5(w7, w3)
    S32LDD(xr6, wf, 0x4);         // xr6(w9, w8)
    S32LDD(xr7, wf, 0x8);         // xr7(w11,w10)
    S32LDD(xr8, wf, 0xc);         // xr8(w13,w12)
    S32LDD(xr9, wf, 0x10);        // xr9(w6, w0)
    S32LDD(xr10,wf, 0x14);
    blk = input - 8;
    /* Inverse DCT on the rows now */
    for (i=0; i<idct_row; i++) {
        S32LDI(xr1, blk, 0x10);       //  xr1 (x4, x0)
	S32LDD(xr2, blk, 0x4);        //  xr2 (x7, x3)
	S32LDD(xr3, blk, 0x8);        //  xr3 (x6, x1)
	S32LDD(xr4, blk, 0xc);        //  xr4 (x5, x2)
	S32OR(xr12, xr2,xr3);
	S32OR(xr11,xr12,xr4);
	S32OR(xr12,xr11,xr1);
	if (S32M2I(xr12) == 0) {
            continue;            //blk[0]= blk[1]=blk[2]=blk[3]=blk[4]=blk[5]=blk[6]=blk[7]=0
	}
	S32SFL(xr12,xr0,xr1,xr13,ptn3);
	S32OR(xr11,xr11,xr12);
	if (S32M2I(xr11) == 0 && S32M2I(xr13) != 0) {
	    D16MUL_HW(xr0,xr5,xr13,xr13);
	    D32SAR(xr0,xr0,xr13,xr13,15);
	    S32SFL(xr0,xr13,xr13,xr13,ptn3);
	    S32STD(xr13,blk, 0x0);
	    S32STD(xr13,blk, 0x4);
	    S32STD(xr13,blk, 0x8);
	    S32STD(xr13,blk, 0xc);         
	    continue;            //blk[0]!=0, and blk[1]=blk[2]=blk[3]=blk[4]=blk[5]=blk[6]=blk[7]=0
	}

	S32SFL(xr1,xr1,xr2,xr2, ptn3);  //xr1:s1, s3, xr2: s0, s2
	S32SFL(xr3,xr3,xr4,xr4, ptn3);  //xr3:s5, s7, xr4: s4, s6
	
	D16MUL_WW(xr11, xr2, xr5, xr12);//xr11: s0*c4, xr12: s2*c2
	D16MAC_AA_WW(xr11,xr4,xr6,xr12);//xr11: s0*c4+s4*c4, xr12: s2*c2+s6*c6
	
	D16MUL_WW(xr13, xr2, xr6, xr14);//xr13: s0*c4, xr14: s2*c6
	D16MAC_SS_WW(xr13,xr4,xr5,xr14);//xr13: s0*c4 - s4*c4, xr14: s2*c6-s6*c2
	
	D16MUL_HW(xr2, xr1, xr7, xr4);  //xr2: s1*c1, xr4: s1*c3 
	D16MAC_AS_LW(xr2,xr1,xr9,xr4);  //xr2: s1*c1+s3*c3, xr4: s1*c3-s3*c7
	D16MAC_AS_HW(xr2,xr3,xr10,xr4); //xr2: s1*c1+s3*c3+s5*c5,
	// xr4: s1*c3-s3*c7-s5*c1
	D16MAC_AS_LW(xr2,xr3,xr8,xr4);  //xr2: s1*c1+s3*c3+s5*c5+s7*c7,
                                      //xr4: s1*c3-s3*c7-s5*c1-s7*c5

	D32SAR(xr11, xr11,xr13,xr13,15);
	S32SFL(xr0, xr11,xr13,xr11,ptn3);
	D32SAR(xr2, xr2,xr4,xr4,15);
	S32SFL(xr0, xr2,xr4,xr2,ptn3);
	D32SAR(xr12,xr12,xr14,xr14,15);
	S32SFL(xr0, xr12,xr14,xr12,ptn3);
	
	
	D16MUL_HW(xr4, xr1, xr8, xr15);     //xr4: s1*c7, xr15:s1*c5
	D16MAC_SS_LW(xr4,xr1,xr10,xr15);    //xr4: s1*c7-s3*c5, xr15: s1*c5-s3*c1
	D16MAC_AA_HW(xr4,xr3,xr9,xr15);     //xr4: s1*c7-s3*c5+s5*c3, xr15: s1*c5-s3*c1+s5*c7
	D16MAC_SA_LW(xr4,xr3,xr7,xr15);     //xr4: s1*c7-s3*c5+s5*c3-s7*c1
	//xr15: s1*c5-s3*c1+s5*c7+s7*c3

	Q16ADD_AS_WW(xr11,xr11,xr12,xr12);  //xr11: rnd(s0*c4+s4*c4)>>15+rnd(s2*c2+s6*c6)>>15
                                          //      rnd(s0*c4-s4*c4)>>15+rnd(s2*c6-s6*c2)>>15
                                          //xr12: rnd(s0*c4+s4*c4)>>15-rnd(s2*c2+s6*c6)>>15
                                          //      rnd(s0*c4-s4*c4)>>15-rnd(s2*c6-s6*c2)>>15
	
	D32SAR(xr15,xr15,xr4,xr4,15);
	S32SFL(xr0,xr15,xr4,xr15,ptn3);
	Q16ADD_AS_WW(xr11, xr11, xr2, xr2);
              //xr11: rnd(s0*c4+s4*c4)>>15+rnd(s2*c2+s6*c6)>>15 + rnd(s1*c1+s3*c3+s5*c5+s7*c7)>>15
              //    : rnd(s0*c4-s4*c4)>>15+rnd(s2*c6-s6*c2)>>15 + rnd(s1*c3-s3*c7-s5*c1-s7*c5)>>15
              //xr2: rnd(s0*c4+s4*c4)>>15+rnd(s2*c2+s6*c6)>>15 - rnd(s1*c1+s3*c3+s5*c5+s7*c7)>>15
              //   : rnd(s0*c4-s4*c4)>>15+rnd(s2*c6-s6*c2)>>15 - rnd(s1*c3-s3*c7-s5*c1-s7*c5)>>15
	Q16ADD_AS_XW(xr12, xr12, xr15, xr15);
              //xr12: rnd(s0*c4+s4*c4)>>15-rnd(s2*c2+s6*c6)>>15+rnd(s1*c5-s3*c1+s5*c7+s7*c3)>>15
              //    : rnd(s0*c4-s4*c4)>>15+rnd(s2*c6-s6*c2)>>15+rnd(s1*c7-s3*c5+s5*c3-s7*c1)>>15
              //xr15: rnd(s0*c4+s4*c4)>>15-rnd(s2*c2+s6*c6)>>15-rnd(s1*c5-s3*c1+s5*c7+s7*c3)>>15
              //    : rnd(s0*c4-s4*c4)>>15+rnd(s2*c6-s6*c2)>>15-rnd(s1*c7-s3*c5+s5*c3-s7*c1)>>15

	S32SFL(xr11,xr11,xr12,xr12, ptn3);
              //xr11: rnd(s0*c4+s4*c4)>>15+rnd(s2*c2+s6*c6)>>15 + rnd(s1*c1+s3*c3+s5*c5+s7*c7)>>15
              //    : rnd(s0*c4+s4*c4)>>15-rnd(s2*c2+s6*c6)>>15+rnd(s1*c5-s3*c1+s5*c7+s7*c3)>>15
              //xr12: rnd(s0*c4-s4*c4)>>15+rnd(s2*c6-s6*c2)>>15 + rnd(s1*c3-s3*c7-s5*c1-s7*c5)>>15
              //    : rnd(s0*c4-s4*c4)>>15+rnd(s2*c6-s6*c2)>>15+rnd(s1*c7-s3*c5+s5*c3-s7*c1)>>15
	S32SFL(xr12,xr12,xr11,xr11, ptn3);
              //xr12: rnd(s0*c4-s4*c4)>>15+rnd(s2*c6-s6*c2)>>15 + rnd(s1*c3-s3*c7-s5*c1-s7*c5)>>15
              //    : rnd(s0*c4+s4*c4)>>15+rnd(s2*c2+s6*c6)>>15 + rnd(s1*c1+s3*c3+s5*c5+s7*c7)>>15
              //xr11: rnd(s0*c4-s4*c4)>>15+rnd(s2*c6-s6*c2)>>15+rnd(s1*c7-s3*c5+s5*c3-s7*c1)>>15
              //    : rnd(s0*c4+s4*c4)>>15-rnd(s2*c2+s6*c6)>>15+rnd(s1*c5-s3*c1+s5*c7+s7*c3)>>15
	S32STD(xr12, blk, 0x0);
	S32STD(xr11, blk, 0x4);
	S32STD(xr15, blk, 0x8);
	S32STD(xr2 , blk, 0xc);
    }

    blk = input - 2;
    for (i=0; i<4; i++)               /* idct columns */
    {
        S32I2M(xr5,wxr5);
	S32I2M(xr6,wxr6);        //xr5: c4 , c2
	S32LDI(xr1, blk, 0x4);   //xr1: ss0, s0
	S32LDD(xr3, blk, 0x20);  //xr3: ss2, s2
	S32LDD(xr11, blk, 0x40); //xr11: ss4, s4
	S32LDD(xr13, blk, 0x60); //xr13: ss6, s6
	
	D16MUL_HW(xr15, xr5, xr1, xr2);    //xr15: ss0*c4, xr9: s0*c4
	D16MAC_AA_HW(xr15,xr5,xr11,xr2);   //xr15: ss0*c4+ss4*c4, xr9: s0*c4+s4*c4
	D16MUL_LW(xr10, xr5, xr3, xr9);    //xr10: ss2*c2, xr9: s2*c2
	D16MAC_AA_LW(xr10,xr6,xr13,xr9);   //xr10: ss2*c2+ss6*c6, xr9: s2*c2+s6*c6
	D32SAR(xr15,xr15,xr2,xr2,15);
	S32SFL(xr0,xr15,xr2,xr15,ptn3);    //xr15: (ss0*c4+ss4*c4)>>15
	D32SAR(xr10,xr10,xr9,xr9,15);
	S32SFL(xr0,xr10,xr9,xr10,ptn3);
	
	S32LDD(xr2, blk, 0x10);            //xr2: ss1, s1
	S32LDD(xr4, blk, 0x30);            //xr4: ss3, s3
	Q16ADD_AS_WW(xr15,xr15,xr10,xr9);  //xr15: rnd(ss0*c4+ss4*c4)>>15+rnd(ss2*c2+ss6*c6)>>15
	                                   //    :rnd(s0*c4+s4*c4)>>15 + rnd(s2*c2 + s6*c6)>>15
                                         //xr9: rnd(ss0*c4+ss4*c4)>>15 - rnd(ss2*c2+ss6*c6)>>15
                                         //   : rnd(s0*c4+s4*c4)>>15 - rnd(s2*c2 + s6*c6)>>15

	D16MUL_HW(xr10, xr5, xr1, xr1);    //xr10: ss0*c4, xr1: s0*c4
	D16MAC_SS_HW(xr10,xr5,xr11,xr1);   //xr10: ss0*c4-ss4*c4, xr1: s0*c4 - s4*c4
	D16MUL_LW(xr11, xr6, xr3, xr12);    //xr11: ss2*c6, xr1: s2*c6
	D16MAC_SS_LW(xr11,xr5,xr13,xr12);   //xr11: ss2*c6-ss6*c2, xr1: s2*c6-s6*c2
	D32SAR(xr10,xr10,xr1,xr1,15);
	S32SFL(xr0,xr10,xr1,xr10,ptn3);
	D32SAR(xr11,xr11,xr12,xr12,15);
	S32SFL(xr0,xr11,xr12,xr11,ptn3);
	
	S32LDD(xr12, blk, 0x50);           //xr12: ss5, s5
	S32LDD(xr14, blk, 0x70);           //xr14: ss7, s7
	Q16ADD_AS_WW(xr10,xr10,xr11,xr1);  //xr10: rnd(ss0*c4-ss4*c4)>>15)+rnd(ss2*c6-ss6*c2)>>15
                                         //    : rnd(s0*c4 - s4*c4)>>15 +rnd(s2*c6 - s6*c2)>>15
                                         //xr1 : rnd(ss0*c4-ss4*c4)>>15-rnd(ss2*c6-ss6*c2)>>15
                                         //    : rnd(s0*c4 - s4*c4)>>15-rnd(s2*c6 - s6*c2)>>15

	D16MUL_HW(xr11, xr7, xr2, xr13);   //xr11: ss1*c1, xr13: s1*c1
	D16MAC_AA_LW(xr11,xr7,xr4,xr13);   //xr11: ss1*c1+ss3*c3, xr13: s1*c1+s3*c3
	D16MAC_AA_LW(xr11,xr8,xr12,xr13);  //xr11: ss1*c1+ss3*c3+ss5*c5 //xr13: s1*c1+s3*c3+s5*c5
	D16MAC_AA_HW(xr11,xr8,xr14,xr13);  //xr11: ss1*c1+ss3*c3+ss5*c5+ss7*c7
	D16MUL_LW(xr3, xr7, xr2, xr5);    //xr3: ss1*c3, xr13: s1*c3
	D16MAC_SS_HW(xr3,xr8,xr4,xr5);    //xr3: ss1*c3-ss3*c7, xr13: s1*c3-s3*c7
	D16MAC_SS_HW(xr3,xr7,xr12,xr5);   //xr3: ss1*c3-ss3*c7-ss5*c1
                                         //xr13: s1*c3-s3*c7-s5*c1
	D16MAC_SS_LW(xr3,xr8,xr14,xr5);   //xr3: ss1*c3-ss3*c7-ss5*c1-ss7*c5
                                         //xr13: s1*c3-s3*c7-s7*c5
	D32SAR(xr11,xr11,xr13,xr13,15);
	S32SFL(xr0,xr11,xr13,xr11,ptn3);
	D32SAR(xr3,xr3,xr5,xr5,15);
	S32SFL(xr0,xr3,xr5,xr3,ptn3);
	
	D16MUL_LW(xr5, xr8, xr2, xr13);    //xr5: ss1*c5, xr13:s1*c5
	D16MAC_SS_HW(xr5,xr7,xr4,xr13);    //xr5: ss1*c5-ss3*c1, xr13:s1*c5-s3*c1
	D16MAC_AA_HW(xr5,xr8,xr12,xr13);   //xr5: ss1*c5-ss3*c1+ss5*c7
                                         //   : s1*c5 - s3*c1+ s5*c7
	D16MAC_AA_LW(xr5,xr7,xr14,xr13);   //xr5: ss1*c5-ss3*c1+ss5*c7+ss7*c1
                                         //   : s1*c5 - s3*c1+ s5*c7+ s7*c1
	D16MUL_HW(xr2, xr8, xr2, xr6);    //xr2: ss1*c7, xr13: s1*c7
	D16MAC_SS_LW(xr2,xr8,xr4,xr6);    //xr2: ss1*c7-ss3*c5, xr13: s1*c7-s3*c5
	D16MAC_AA_LW(xr2,xr7,xr12,xr6);   //xr2: ss1*c7-ss3*c5+ss5*c1
                                         //xr13: s1*c7-s3*c5+s5*c1
	D16MAC_SS_HW(xr2,xr7,xr14,xr6);   //xr2: ss1*c7-ss3*c5+ss5*c1-ss7*c3
	D32SAR(xr5,xr5,xr13,xr13,15);
	S32SFL(xr0,xr5,xr13,xr5,ptn3);
                                         //xr13: s1*c7-s3*c5+s5*c1-s7*c3
	D32SAR(xr2,xr2,xr6,xr6,15);
	S32SFL(xr0,xr2,xr6,xr2,ptn3);
	
	S32I2M(xr4, 0x08080808);
	Q16ADD_AS_WW(xr15,xr15,xr11,xr11); //xr15:rnd(ss0*c4+ss4*c4)>>15+rnd(ss2*c2+ss6*c6)>>15+
                                         //     rnd(ss1*c1+ss3*c3+ss5*c5+ss7*c7)>>15
                                         //     rnd(s0*c4+s4*c4)>>15 + rnd(s2*c2 + s6*c6)>>15+
                                         //     rnd(s1*c1+s3*c3+s5*c5+s7*c7)>>15
                                         //xr11:rnd(ss0*c4+ss4*c4)>>15+rnd(ss2*c2+ss6*c6)>>15-
                                         //     rnd(ss1*c1+ss3*c3+ss5*c5+ss7*c7)>>16
                                         //     rnd(s0*c4+s4*c4)>>16 + rnd(s2*c2 + s6*c6)>>16-
                                         //     rnd(s1*c1+s3*c3+s5*c5+s7*c7)>>16
	Q16ADD_AS_WW(xr10,xr10,xr3,xr3);   //xr10:rnd(ss0*c4-ss4*c4)>>16)+rnd(ss2*c6-ss6*c2)>>16+
                                         //     rnd(ss1*c3-ss3*c7-ss5*c1-ss7*c5)>>16
                                         //     rnd(s0*c4 - s4*c4)>>16 +rnd(s2*c6 - s6*c2)>>16+
                                         //     rnd(s1*c3-s3*c7-s7*c5)>>16
                                         //xr10:rnd(ss0*c4-ss4*c4)>>16)+rnd(ss2*c6-ss6*c2)>>16-
                                         //     rnd(ss1*c3-ss3*c7-ss5*c1-ss7*c5)>>16
                                         //     rnd(s0*c4 - s4*c4)>>16 +rnd(s2*c6 - s6*c2)>>16-
                                         //     rnd(s1*c3-s3*c7-s7*c5)>>16
	Q16ADD_AS_WW(xr1,xr1,xr5,xr5);     //xr1: rnd(ss0*c4-ss4*c4)>>16-rnd(ss2*c6-ss6*c2)>>16+
                                         //     rnd(ss1*c5-ss3*c1+ss5*c7+ss7*c1)>>16
                                         //     rnd(s0*c4 - s4*c4)>>16 +rnd(s2*c6 - s6*c2)>>16+
                                         //     rnd(s1*c5 - s3*c1+ s5*c7+ s7*c1)>>16
                                         //xr1: rnd(ss0*c4-ss4*c4)>>16-rnd(ss2*c6-ss6*c2)>>16-
                                         //     rnd(ss1*c5-ss3*c1+ss5*c7+ss7*c1)>>16
                                         //     rnd(s0*c4 - s4*c4)>>16 +rnd(s2*c6 - s6*c2)>>16-
                                         //     rnd(s1*c5 - s3*c1+ s5*c7+ s7*c1)>>16
	Q16ADD_AS_WW(xr9,xr9,xr2,xr2);     //xr9: rnd(ss0*c4+ss4*c4)>>16 - rnd(ss2*c2+ss6*c6)>>16+
                                         //     rnd(ss1*c7-ss3*c5+ss5*c1-ss7*c3)>>16
                                         //     rnd(s0*c4+s4*c4)>>16 - rnd(s2*c2 + s6*c6)>>16+
                                         //     rnd(s1*c7-s3*c5+s5*c1-s7*c3)>>16
                                         //xr9: rnd(ss0*c4+ss4*c4)>>16 - rnd(ss2*c2+ss6*c6)>>16-
                                         //     rnd(ss1*c7-ss3*c5+ss5*c1-ss7*c3)>>16
                                         //     rnd(s0*c4+s4*c4)>>16 - rnd(s2*c2 + s6*c6)>>16-
                                         //     rnd(s1*c7-s3*c5+s5*c1-s7*c3)>>16
	Q16ACCM_AA(xr15,xr4,xr4,xr10);
	Q16ACCM_AA(xr11,xr4,xr4,xr1);
	Q16ACCM_AA(xr9,xr4,xr4,xr2);
	Q16ACCM_AA(xr5,xr4,xr4,xr3);
	Q16SAR(xr15,xr15,xr10,xr10,4);
	Q16SAR(xr11,xr11,xr1,xr1,4);
	Q16SAR(xr9,xr9,xr2,xr2,4);
	Q16SAR(xr5,xr5,xr3,xr3,4);

	S32STD(xr15, blk, 0x00);
	S32STD(xr10, blk, 0x10);
	S32STD(xr1, blk, 0x20);
	S32STD(xr9, blk, 0x30);
	S32STD(xr2, blk, 0x40);
	S32STD(xr5, blk, 0x50);
	S32STD(xr3, blk, 0x60);
	S32STD(xr11, blk, 0x70);
    }

    blk = input -8;
    dst -= stride;
    for (i=0; i<8; i++) {
        S32LDI(xr1, blk, 0x10);
	S32LDD(xr2, blk, 0x4);
	S32LDD(xr3, blk, 0x8);
	S32LDD(xr4, blk, 0xc);
	Q16SAT(xr5, xr2, xr1);
	Q16SAT(xr6, xr4, xr3);
	S32SDIV(xr5, dst, stride, 0x0);     
	S32STD(xr6, dst, 0x4);
    }
}

static void ff_vp3_idct_add_mxu(uint8_t *src, int stride, DCTELEM *input, uint8_t idct_row)
{
    int i;
    DCTELEM *blk;
    int32_t wf = (int32_t)whirl_idct;

    S32LDD(xr5, wf, 0x0);         // xr5(w7, w3)
    S32LDD(xr6, wf, 0x4);         // xr6(w9, w8)
    S32LDD(xr7, wf, 0x8);         // xr7(w11,w10)
    S32LDD(xr8, wf, 0xc);         // xr8(w13,w12)
    S32LDD(xr9, wf, 0x10);        // xr9(w6, w0)
    S32LDD(xr10,wf, 0x14);
    blk = input - 8;
    /* Inverse DCT on the rows now */
    for (i=0; i<idct_row; i++) {
        S32LDI(xr1, blk, 0x10);       //  xr1 (x4, x0)
	S32LDD(xr2, blk, 0x4);        //  xr2 (x7, x3)
	S32LDD(xr3, blk, 0x8);        //  xr3 (x6, x1)
	S32LDD(xr4, blk, 0xc);        //  xr4 (x5, x2)
	S32OR(xr12, xr2,xr3);
	S32OR(xr11,xr12,xr4);
	S32OR(xr12,xr11,xr1);
	if (S32M2I(xr12) == 0) {
	    continue;            //blk[0]= blk[1]=blk[2]=blk[3]=blk[4]=blk[5]=blk[6]=blk[7]=0
	}
	S32SFL(xr12,xr0,xr1,xr13,ptn3);
	S32OR(xr11,xr11,xr12);
	if (S32M2I(xr11) == 0 && S32M2I(xr13) != 0) {
	    D16MUL_HW(xr0,xr5,xr13,xr13);
	    D32SAR(xr0,xr0,xr13,xr13,15);
	    S32SFL(xr0,xr13,xr13,xr13,ptn3);
	    S32STD(xr13,blk, 0x0);
	    S32STD(xr13,blk, 0x4);
	    S32STD(xr13,blk, 0x8);
	    S32STD(xr13,blk, 0xc);
	    continue;            //blk[0]!=0, and blk[1]=blk[2]=blk[3]=blk[4]=blk[5]=blk[6]=blk[7]=0
	}

	S32SFL(xr1,xr1,xr2,xr2, ptn3);  //xr1:s1, s3, xr2: s0, s2
	S32SFL(xr3,xr3,xr4,xr4, ptn3);  //xr3:s5, s7, xr4: s4, s6

	D16MUL_WW(xr11, xr2, xr5, xr12);//xr11: s0*c4, xr12: s2*c2
	D16MAC_AA_WW(xr11,xr4,xr6,xr12);//xr11: s0*c4+s4*c4, xr12: s2*c2+s6*c6

	D16MUL_WW(xr13, xr2, xr6, xr14);//xr13: s0*c4, xr14: s2*c6
	D16MAC_SS_WW(xr13,xr4,xr5,xr14);//xr13: s0*c4 - s4*c4, xr14: s2*c6-s6*c2

	D16MUL_HW(xr2, xr1, xr7, xr4);  //xr2: s1*c1, xr4: s1*c3 
	D16MAC_AS_LW(xr2,xr1,xr9,xr4);  //xr2: s1*c1+s3*c3, xr4: s1*c3-s3*c7
	D16MAC_AS_HW(xr2,xr3,xr10,xr4); //xr2: s1*c1+s3*c3+s5*c5,
                                      // xr4: s1*c3-s3*c7-s5*c1
	D16MAC_AS_LW(xr2,xr3,xr8,xr4);  //xr2: s1*c1+s3*c3+s5*c5+s7*c7,
                                      //xr4: s1*c3-s3*c7-s5*c1-s7*c5
	D32SAR(xr11, xr11,xr13,xr13,15);
	S32SFL(xr0, xr11,xr13,xr11,ptn3);
	D32SAR(xr12,xr12,xr14,xr14,15);
	S32SFL(xr0, xr12,xr14,xr12,ptn3);
	D32SAR(xr2, xr2,xr4,xr4,15);
	S32SFL(xr0, xr2,xr4,xr2,ptn3);
      
	D16MUL_HW(xr4, xr1, xr8, xr15);     //xr4: s1*c7, xr15:s1*c5
	D16MAC_SS_LW(xr4,xr1,xr10,xr15);    //xr4: s1*c7-s3*c5, xr15: s1*c5-s3*c1
	D16MAC_AA_HW(xr4,xr3,xr9,xr15);     //xr4: s1*c7-s3*c5+s5*c3, xr15: s1*c5-s3*c1+s5*c7
	D16MAC_SA_LW(xr4,xr3,xr7,xr15);     //xr4: s1*c7-s3*c5+s5*c3-s7*c1
	                                    //xr15: s1*c5-s3*c1+s5*c7+s7*c3
	Q16ADD_AS_WW(xr11,xr11,xr12,xr12);  //xr11: rnd(s0*c4+s4*c4)>>15+rnd(s2*c2+s6*c6)>>15
                                          //      rnd(s0*c4-s4*c4)>>15+rnd(s2*c6-s6*c2)>>15
                                          //xr12: rnd(s0*c4+s4*c4)>>15-rnd(s2*c2+s6*c6)>>15
                                          //      rnd(s0*c4-s4*c4)>>15-rnd(s2*c6-s6*c2)>>15
	D32SAR(xr15,xr15,xr4,xr4,15);
	S32SFL(xr0,xr15,xr4,xr15,ptn3);
	Q16ADD_AS_WW(xr11, xr11, xr2, xr2);
              //xr11: rnd(s0*c4+s4*c4)>>15+rnd(s2*c2+s6*c6)>>15 + rnd(s1*c1+s3*c3+s5*c5+s7*c7)>>15
              //    : rnd(s0*c4-s4*c4)>>15+rnd(s2*c6-s6*c2)>>15 + rnd(s1*c3-s3*c7-s5*c1-s7*c5)>>15
              //xr2: rnd(s0*c4+s4*c4)>>15+rnd(s2*c2+s6*c6)>>15 - rnd(s1*c1+s3*c3+s5*c5+s7*c7)>>15
              //   : rnd(s0*c4-s4*c4)>>15+rnd(s2*c6-s6*c2)>>15 - rnd(s1*c3-s3*c7-s5*c1-s7*c5)>>15

	Q16ADD_AS_XW(xr12, xr12, xr15, xr15);
              //xr12: rnd(s0*c4+s4*c4)>>15-rnd(s2*c2+s6*c6)>>15+rnd(s1*c5-s3*c1+s5*c7+s7*c3)>>15
              //    : rnd(s0*c4-s4*c4)>>15+rnd(s2*c6-s6*c2)>>15+rnd(s1*c7-s3*c5+s5*c3-s7*c1)>>15
              //xr15: rnd(s0*c4+s4*c4)>>15-rnd(s2*c2+s6*c6)>>15-rnd(s1*c5-s3*c1+s5*c7+s7*c3)>>15
              //    : rnd(s0*c4-s4*c4)>>15+rnd(s2*c6-s6*c2)>>15-rnd(s1*c7-s3*c5+s5*c3-s7*c1)>>15

	S32SFL(xr11,xr11,xr12,xr12, ptn3);
              //xr11: rnd(s0*c4+s4*c4)>>15+rnd(s2*c2+s6*c6)>>15 + rnd(s1*c1+s3*c3+s5*c5+s7*c7)>>15
              //    : rnd(s0*c4+s4*c4)>>15-rnd(s2*c2+s6*c6)>>15+rnd(s1*c5-s3*c1+s5*c7+s7*c3)>>15
              //xr12: rnd(s0*c4-s4*c4)>>15+rnd(s2*c6-s6*c2)>>15 + rnd(s1*c3-s3*c7-s5*c1-s7*c5)>>15
              //    : rnd(s0*c4-s4*c4)>>15+rnd(s2*c6-s6*c2)>>15+rnd(s1*c7-s3*c5+s5*c3-s7*c1)>>15
	S32SFL(xr12,xr12,xr11,xr11, ptn3);

              //xr12: rnd(s0*c4-s4*c4)>>16+rnd(s2*c6-s6*c2)>>16 + rnd(s1*c3-s3*c7-s5*c1-s7*c5)>>16
              //    : rnd(s0*c4+s4*c4)>>16+rnd(s2*c2+s6*c6)>>16 + rnd(s1*c1+s3*c3+s5*c5+s7*c7)>>16
              //xr11: rnd(s0*c4-s4*c4)>>16+rnd(s2*c6-s6*c2)>>16+rnd(s1*c7-s3*c5+s5*c3-s7*c1)>>16
              //    : rnd(s0*c4+s4*c4)>>16-rnd(s2*c2+s6*c6)>>16+rnd(s1*c5-s3*c1+s5*c7+s7*c3)>>16
	S32STD(xr12, blk, 0x0);
	S32STD(xr11, blk, 0x4);
	S32STD(xr15, blk, 0x8);
	S32STD(xr2, blk, 0xc);
    }
      
    blk = input - 2;
    for (i=0; i<4; i++)               /* idct columns */
    {
        S32I2M(xr5,wxr5);        //xr5: c4 , c2
	S32I2M(xr6,wxr6);        //xr5: c4 , c2
	S32LDI(xr1, blk, 0x4);   //xr1: ss0, s0
	S32LDD(xr3, blk, 0x20);  //xr3: ss2, s2
	S32LDD(xr11, blk, 0x40); //xr11: ss4, s4
	S32LDD(xr13, blk, 0x60); //xr13: ss6, s6

	D16MUL_HW(xr15, xr5, xr1, xr2);    //xr15: ss0*c4, xr9: s0*c4
	D16MAC_AA_HW(xr15,xr5,xr11,xr2);   //xr15: ss0*c4+ss4*c4, xr9: s0*c4+s4*c4
	D16MUL_LW(xr10, xr5, xr3, xr9);    //xr10: ss2*c2, xr9: s2*c2
	D16MAC_AA_LW(xr10,xr6,xr13,xr9);   //xr10: ss2*c2+ss6*c6, xr9: s2*c2+s6*c6
	D32SAR(xr15,xr15,xr2,xr2,15);      
	S32SFL(xr0,xr15,xr2,xr15,ptn3);    //xr15: (ss0*c4+ss4*c4)>>15
	D32SAR(xr10,xr10,xr9,xr9,15);      
	S32SFL(xr0,xr10,xr9,xr10,ptn3);    //xr10: (ss2*c2+ss6*c6)>>15

	S32LDD(xr2, blk, 0x10);            //xr2: ss1, s1
	S32LDD(xr4, blk, 0x30);            //xr4: ss3, s3
	Q16ADD_AS_WW(xr15,xr15,xr10,xr9);  //xr15: rnd(ss0*c4+ss4*c4)>>15+rnd(ss2*c2+ss6*c6)>>15
                                         //    :rnd(s0*c4+s4*c4)>>15 + rnd(s2*c2 + s6*c6)>>15
                                         //xr9: rnd(ss0*c4+ss4*c4)>>15 - rnd(ss2*c2+ss6*c6)>>15
                                         //   : rnd(s0*c4+s4*c4)>>15 - rnd(s2*c2 + s6*c6)>>15
	D16MUL_HW(xr10, xr5, xr1, xr1);    //xr10: ss0*c4, xr1: s0*c4
	D16MAC_SS_HW(xr10,xr5,xr11,xr1);   //xr10: ss0*c4-ss4*c4, xr1: s0*c4 - s4*c4
	D16MUL_LW(xr11, xr6, xr3, xr12);    //xr11: ss2*c6, xr1: s2*c6
	D16MAC_SS_LW(xr11,xr5,xr13,xr12);   //xr11: ss2*c6-ss6*c2, xr1: s2*c6-s6*c2
	D32SAR(xr10,xr10,xr1,xr1,15);
	S32SFL(xr0,xr10,xr1,xr10,ptn3);    //xr10: (ss0*c4-ss4*c4)>>15 //    : (s0*c4 - s4*c4)>>15
	D32SAR(xr11,xr11,xr12,xr12,15);      
	S32SFL(xr0,xr11,xr12,xr11,ptn3);    //xr11:(ss2*c6-ss6*c2)>>15
                                         //    :(s2*c6-s6*c2)>>15

	S32LDD(xr12, blk, 0x50);           //xr12: ss5, s5
	S32LDD(xr14, blk, 0x70);           //xr14: ss7, s7
	Q16ADD_AS_WW(xr10,xr10,xr11,xr1);  //xr10: rnd(ss0*c4-ss4*c4)>>15)+rnd(ss2*c6-ss6*c2)>>15
                                         //    : rnd(s0*c4 - s4*c4)>>15 +rnd(s2*c6 - s6*c2)>>15
                                         //xr1 : rnd(ss0*c4-ss4*c4)>>15-rnd(ss2*c6-ss6*c2)>>15
                                         //    : rnd(s0*c4 - s4*c4)>>15-rnd(s2*c6 - s6*c2)>>15

	D16MUL_HW(xr11, xr7, xr2, xr13);   //xr11: ss1*c1, xr13: s1*c1
	D16MAC_AA_LW(xr11,xr7,xr4,xr13);   //xr11: ss1*c1+ss3*c3, xr13: s1*c1+s3*c3
	D16MAC_AA_LW(xr11,xr8,xr12,xr13);  //xr11: ss1*c1+ss3*c3+ss5*c5 //xr13: s1*c1+s3*c3+s5*c5
	D16MAC_AA_HW(xr11,xr8,xr14,xr13);  //xr11: ss1*c1+ss3*c3+ss5*c5+ss7*c7
                                         //xr13: s1*c1+s3*c3+s5*c5+s7*c7
	D16MUL_LW(xr3, xr7, xr2, xr5);    //xr3: ss1*c3, xr13: s1*c3
	D16MAC_SS_HW(xr3,xr8,xr4,xr5);    //xr3: ss1*c3-ss3*c7, xr13: s1*c3-s3*c7
	D16MAC_SS_HW(xr3,xr7,xr12,xr5);   //xr3: ss1*c3-ss3*c7-ss5*c1
                                         //xr13: s1*c3-s3*c7-s5*c1
	D16MAC_SS_LW(xr3,xr8,xr14,xr5);   //xr3: ss1*c3-ss3*c7-ss5*c1-ss7*c5
                                         //xr13: s1*c3-s3*c7-s7*c5
	D32SAR(xr11,xr11,xr13,xr13,15); 
	S32SFL(xr0,xr11,xr13,xr11,ptn3);   //xr11: (ss1*c1+ss3*c3+ss5*c5+ss7*c7)>>15 //    : (s1*c1+s3*c3+s5*c5+s7*c7)>>15
	D32SAR(xr3,xr3,xr5,xr5,15);
	S32SFL(xr0,xr3,xr5,xr3,ptn3);     //xr3: (ss1*c3-ss3*c7-ss5*c1-ss7*c5)>>15
                                         //   : (s1*c3-s3*c7-s7*c5)>>15
	D16MUL_LW(xr5, xr8, xr2, xr13);    //xr5: ss1*c5, xr13:s1*c5
	D16MAC_SS_HW(xr5,xr7,xr4,xr13);    //xr5: ss1*c5-ss3*c1, xr13:s1*c5-s3*c1
	D16MAC_AA_HW(xr5,xr8,xr12,xr13);   //xr5: ss1*c5-ss3*c1+ss5*c7
                                         //   : s1*c5 - s3*c1+ s5*c7
	D16MAC_AA_LW(xr5,xr7,xr14,xr13);   //xr5: ss1*c5-ss3*c1+ss5*c7+ss7*c1
                                         //   : s1*c5 - s3*c1+ s5*c7+ s7*c1
	D16MUL_HW(xr2, xr8, xr2, xr6);    //xr2: ss1*c7, xr13: s1*c7
	D16MAC_SS_LW(xr2,xr8,xr4,xr6);    //xr2: ss1*c7-ss3*c5, xr13: s1*c7-s3*c5
	D16MAC_AA_LW(xr2,xr7,xr12,xr6);   //xr2: ss1*c7-ss3*c5+ss5*c1 //xr13: s1*c7-s3*c5+s5*c1
	D16MAC_SS_HW(xr2,xr7,xr14,xr6);   //xr2: ss1*c7-ss3*c5+ss5*c1-ss7*c3
                                         //xr13: s1*c7-s3*c5+s5*c1-s7*c3
	D32SAR(xr5,xr5,xr13,xr13,15);
	S32SFL(xr0,xr5,xr13,xr5,ptn3);     //xr5: (ss1*c5-ss3*c1+ss5*c7+ss7*c1)>>15 //  :(s1*c5 - s3*c1+ s5*c7+ s7*c1)>>15
	D32SAR(xr2,xr2,xr6,xr6,15);
	S32SFL(xr0,xr2,xr6,xr2,ptn3);     //xr2:(ss1*c7-ss3*c5+ss5*c1-ss7*c3)>>15
                                         //   :(s1*c7-s3*c5+s5*c1-s7*c3)>>15

	S32I2M(xr4, 0x00080008);//round value 8;
	Q16ADD_AS_WW(xr15,xr15,xr11,xr11); //xr15:rnd(ss0*c4+ss4*c4)>>16+rnd(ss2*c2+ss6*c6)>>16+
                                         //     rnd(ss1*c1+ss3*c3+ss5*c5+ss7*c7)>>16
                                         //     rnd(s0*c4+s4*c4)>>16 + rnd(s2*c2 + s6*c6)>>16+
                                         //     rnd(s1*c1+s3*c3+s5*c5+s7*c7)>>16

                                         //xr11:rnd(ss0*c4+ss4*c4)>>16+rnd(ss2*c2+ss6*c6)>>16-
                                         //     rnd(ss1*c1+ss3*c3+ss5*c5+ss7*c7)>>16
                                         //     rnd(s0*c4+s4*c4)>>16 + rnd(s2*c2 + s6*c6)>>16-
                                         //     rnd(s1*c1+s3*c3+s5*c5+s7*c7)>>16
	Q16ADD_AS_WW(xr10,xr10,xr3,xr3);   //xr10:rnd(ss0*c4-ss4*c4)>>16)+rnd(ss2*c6-ss6*c2)>>16+
                                         //     rnd(ss1*c3-ss3*c7-ss5*c1-ss7*c5)>>16
                                         //     rnd(s0*c4 - s4*c4)>>16 +rnd(s2*c6 - s6*c2)>>16+
                                         //     rnd(s1*c3-s3*c7-s7*c5)>>16
                                         //xr10:rnd(ss0*c4-ss4*c4)>>16)+rnd(ss2*c6-ss6*c2)>>16-
                                         //     rnd(ss1*c3-ss3*c7-ss5*c1-ss7*c5)>>16
                                         //     rnd(s0*c4 - s4*c4)>>16 +rnd(s2*c6 - s6*c2)>>16-
                                         //     rnd(s1*c3-s3*c7-s7*c5)>>16
	Q16ADD_AS_WW(xr1,xr1,xr5,xr5);     //xr1: rnd(ss0*c4-ss4*c4)>>16-rnd(ss2*c6-ss6*c2)>>16+
                                         //     rnd(ss1*c5-ss3*c1+ss5*c7+ss7*c1)>>16
                                         //     rnd(s0*c4 - s4*c4)>>16 +rnd(s2*c6 - s6*c2)>>16+
                                         //     rnd(s1*c5 - s3*c1+ s5*c7+ s7*c1)>>16
                                         //xr1: rnd(ss0*c4-ss4*c4)>>16-rnd(ss2*c6-ss6*c2)>>16-
                                         //     rnd(ss1*c5-ss3*c1+ss5*c7+ss7*c1)>>16
                                         //     rnd(s0*c4 - s4*c4)>>16 +rnd(s2*c6 - s6*c2)>>16-
                                         //     rnd(s1*c5 - s3*c1+ s5*c7+ s7*c1)>>16
	Q16ADD_AS_WW(xr9,xr9,xr2,xr2);     //xr9: rnd(ss0*c4+ss4*c4)>>16 - rnd(ss2*c2+ss6*c6)>>16+
                                         //     rnd(ss1*c7-ss3*c5+ss5*c1-ss7*c3)>>16
                                         //     rnd(s0*c4+s4*c4)>>16 - rnd(s2*c2 + s6*c6)>>16+
                                         //     rnd(s1*c7-s3*c5+s5*c1-s7*c3)>>16
                                         //xr9: rnd(ss0*c4+ss4*c4)>>16 - rnd(ss2*c2+ss6*c6)>>16-
                                         //     rnd(ss1*c7-ss3*c5+ss5*c1-ss7*c3)>>16
                                         //     rnd(s0*c4+s4*c4)>>16 - rnd(s2*c2 + s6*c6)>>16-
                                         //     rnd(s1*c7-s3*c5+s5*c1-s7*c3)>>16

	Q16ACCM_AA(xr15,xr4,xr4,xr10);
	Q16ACCM_AA(xr11,xr4,xr4,xr1);
	Q16ACCM_AA(xr9,xr4,xr4,xr2);
	Q16ACCM_AA(xr5,xr4,xr4,xr3);
	Q16SAR(xr15,xr15,xr10,xr10,4);
	Q16SAR(xr11,xr11,xr1,xr1,4);
	Q16SAR(xr9,xr9,xr2,xr2,4);
	Q16SAR(xr5,xr5,xr3,xr3,4);
	
	S32STD(xr15, blk, 0x00);
	S32STD(xr10, blk, 0x10);
	S32STD(xr1, blk, 0x20);
	S32STD(xr9, blk, 0x30);
	S32STD(xr2, blk, 0x40);
	S32STD(xr5, blk, 0x50);
	S32STD(xr3, blk, 0x60);
	S32STD(xr11, blk, 0x70);
    }

    blk = input - 8;
    src -= stride;
    for (i=0; i<8; i++) {
        S32LDIV(xr1, src, stride, 0x0);
	S32LDI(xr3, blk, 0x10);
	S32LDD(xr4, blk, 0x4);
	Q8ACCE_AA(xr4, xr1, xr0, xr3);
	S32LDD(xr2, src, 0x4);
	S32LDD(xr5, blk, 0x8);
	S32LDD(xr6, blk, 0xc);
	Q8ACCE_AA(xr6, xr2, xr0, xr5);
	Q16SAT(xr1, xr4, xr3);
	S32STD(xr1, src, 0x0);
	Q16SAT(xr2, xr6, xr5);
	S32STD(xr2, src, 0x4);
    }
}

static void vp56_decode_mb(VP56Context *s, int row, int col, int is_alpha)
{
    AVFrame *frame_current, *frame_ref;
    VP56mb mb_type;
    VP56Frame ref_frame;
    int b, ab, b_max, plane, off;
    blk_coeff = TCSM0_BANK1;

    if (s->framep[VP56_FRAME_CURRENT]->key_frame)
        mb_type = VP56_MB_INTRA;
    else
        mb_type = vp56_decode_mv(s, row, col);
    ref_frame = vp56_reference_frame[mb_type];

    if (row != 0 && col != 0){
      poll_gp0_end();
    }

    s->parse_coeff(s);
    vp56_add_predictors_dc(s, ref_frame);

    frame_current = s->framep[VP56_FRAME_CURRENT];
    frame_ref = s->framep[ref_frame];

    ab = 6*is_alpha;
    b_max = 6 - 2*is_alpha;

    switch (mb_type) {
        case VP56_MB_INTRA:
            for (b=0; b<b_max; b++) {
                plane = vp56_b2p[b+ab];
                ff_vp3_idct_put_mxu(frame_current->data[plane] + s->block_offset[b],s->stride[plane],
				    blk_coeff + b*64,idct_row[b]);
            }
            break;

        case VP56_MB_INTER_NOVEC_PF:
        case VP56_MB_INTER_NOVEC_GF:
            for (b=0; b<b_max; b++) {
                plane = vp56_b2p[b+ab];
                off = s->block_offset[b];
                MC_put_o_8_c(frame_current->data[plane] + off,
		                          frame_ref->data[plane] + off,
		                          s->stride[plane], 8);
                ff_vp3_idct_add_mxu(frame_current->data[plane] + off,
				    s->stride[plane], blk_coeff+b*64,idct_row[b]);
            }
            break;

        case VP56_MB_INTER_DELTA_PF:
        case VP56_MB_INTER_V1_PF:
        case VP56_MB_INTER_V2_PF:
        case VP56_MB_INTER_DELTA_GF:
        case VP56_MB_INTER_4V:
        case VP56_MB_INTER_V1_GF:
        case VP56_MB_INTER_V2_GF:
            for (b=0; b<b_max; b++) {
                int x_off = b==1 || b==3 ? 8 : 0;
                int y_off = b==2 || b==3 ? 8 : 0;
                plane = vp56_b2p[b+ab];
                vp56_mc(s, b, plane, frame_ref->data[plane], s->stride[plane],
                        16*col+x_off, 16*row+y_off);
                ff_vp3_idct_add_mxu(frame_current->data[plane] + s->block_offset[b],
			    s->stride[plane], blk_coeff+b*64,idct_row[b]);
            }
            break;
    }
    set_gp0_dcs();       

}

static int vp56_size_changed(AVCodecContext *avctx)
{
    VP56Context *s = avctx->priv_data;
    int stride = s->framep[VP56_FRAME_CURRENT]->linesize[0];
    int i;

    s->plane_width[0]  = s->plane_width[3]  = avctx->coded_width;
    s->plane_width[1]  = s->plane_width[2]  = avctx->coded_width/2;
    s->plane_height[0] = s->plane_height[3] = avctx->coded_height;
    s->plane_height[1] = s->plane_height[2] = avctx->coded_height/2;

    for (i=0; i<4; i++)
        s->stride[i] = s->flip * s->framep[VP56_FRAME_CURRENT]->linesize[i];

    s->mb_width  = (avctx->coded_width +15) / 16;
    s->mb_height = (avctx->coded_height+15) / 16;

    if (s->mb_width > 1000 || s->mb_height > 1000) {
        av_log(avctx, AV_LOG_ERROR, "picture too big\n");
        return -1;
    }
#ifdef JZ_LINUX_OS
    s->qscale_table = jz4740_alloc_frame_k0(16,s->mb_width);
    s->above_blocks = jz4740_alloc_frame_k0(16,(4*s->mb_width+6) * sizeof(*s->above_blocks));
    s->macroblocks = jz4740_alloc_frame_k0(16,s->mb_width*s->mb_height*sizeof(*s->macroblocks));
    av_free(s->edge_emu_buffer_alloc);
    s->edge_emu_buffer_alloc = jz4740_alloc_frame_k0(16,16*stride);
#else
    s->qscale_table = av_realloc(s->qscale_table, s->mb_width);
    s->above_blocks = av_realloc(s->above_blocks,
                                 (4*s->mb_width+6) * sizeof(*s->above_blocks));
    s->macroblocks = av_realloc(s->macroblocks,
                                s->mb_width*s->mb_height*sizeof(*s->macroblocks));
    av_free(s->edge_emu_buffer_alloc);
    s->edge_emu_buffer_alloc = av_malloc(16*stride);
#endif
    s->edge_emu_buffer = s->edge_emu_buffer_alloc;
    if (s->flip < 0)
        s->edge_emu_buffer += 15 * stride;

    return 0;
}


int ff_vp56_decode_frame(AVCodecContext *avctx, void *data, int *data_size,
                         AVPacket *avpkt)
{
    const uint8_t *buf = avpkt->data;
    VP56Context *s = avctx->priv_data;
    AVFrame *const p = s->framep[VP56_FRAME_CURRENT];
    int remaining_buf_size = avpkt->size;
    int is_alpha, av_uninit(alpha_offset);

    if (s->has_alpha) {
        if (remaining_buf_size < 3)
            return -1;
        alpha_offset = bytestream_get_be24(&buf);
        remaining_buf_size -= 3;
        if (remaining_buf_size < alpha_offset)
            return -1;
    }

    int *gp0_chain_ptr = DDMA_GP0_DES_CHAIN;
    gp0_chain_ptr[0] = TCSM1_PADDR(0xF4001000);
    gp0_chain_ptr[1] = TCSM0_PADDR(TCSM0_BANK1);
    gp0_chain_ptr[2] = GP_STRD(768,GP_FRM_NML,768);
    gp0_chain_ptr[3] = GP_UNIT(GP_TAG_UL,768,768);
    set_gp0_dha(TCSM0_PADDR(DDMA_GP0_DES_CHAIN));   

    for (is_alpha=0; is_alpha < 1+s->has_alpha; is_alpha++) {
        int mb_row, mb_col, mb_row_flip, mb_offset = 0;
        int block, y, uv, stride_y, stride_uv;
        int golden_frame = 0;
        int res;

        s->modelp = &s->models[is_alpha];

        res = s->parse_header(s, buf, remaining_buf_size, &golden_frame);
        if (!res)
            return -1;

        if (!is_alpha) {
            p->reference = 1;
            if (avctx->get_buffer(avctx, p) < 0) {
                av_log(avctx, AV_LOG_ERROR, "get_buffer() failed\n");
                return -1;
            }

            if (res == 2)
                if (vp56_size_changed(avctx)) {
                    avctx->release_buffer(avctx, p);
                    return -1;
                }
        }

        if (p->key_frame) {
            p->pict_type = FF_I_TYPE;
            s->default_models_init(s);
            for (block=0; block<s->mb_height*s->mb_width; block++)
                s->macroblocks[block].type = VP56_MB_INTRA;
        } else {
            p->pict_type = FF_P_TYPE;
            vp56_parse_mb_type_models(s);
            s->parse_vector_models(s);
            s->mb_type = VP56_MB_INTER_NOVEC_PF;
        }

        s->parse_coeff_models(s);

        memset(s->prev_dc, 0, sizeof(s->prev_dc));
        s->prev_dc[1][VP56_FRAME_CURRENT] = 128;
        s->prev_dc[2][VP56_FRAME_CURRENT] = 128;

        for (block=0; block < 4*s->mb_width+6; block++) {
            s->above_blocks[block].ref_frame = VP56_FRAME_NONE;
            s->above_blocks[block].dc_coeff = 0;
            s->above_blocks[block].not_null_dc = 0;
        }
        s->above_blocks[2*s->mb_width + 2].ref_frame = VP56_FRAME_CURRENT;
        s->above_blocks[3*s->mb_width + 4].ref_frame = VP56_FRAME_CURRENT;

        stride_y  = p->linesize[0];
        stride_uv = p->linesize[1];

        if (s->flip < 0)
            mb_offset = 7;
        /* main macroblocks loop */
#ifdef JZC_PMON_P0
	PMON_ON(test1);
#endif
        for (mb_row=0; mb_row<s->mb_height; mb_row++) {
            if (s->flip < 0)
                mb_row_flip = s->mb_height - mb_row - 1;
            else
                mb_row_flip = mb_row;

            for (block=0; block<4; block++) {
                s->left_block[block].ref_frame = VP56_FRAME_NONE;
                s->left_block[block].dc_coeff = 0;
                s->left_block[block].not_null_dc = 0;
            }
	    if (s->avctx->codec->id == CODEC_ID_VP5) { 
	      memset(s->coeff_ctx, 0, sizeof(s->coeff_ctx));
	      memset(s->coeff_ctx_last, 24, sizeof(s->coeff_ctx_last));
            }
            s->above_block_idx[0] = 1;
            s->above_block_idx[1] = 2;
            s->above_block_idx[2] = 1;
            s->above_block_idx[3] = 2;
            s->above_block_idx[4] = 2*s->mb_width + 2 + 1;
            s->above_block_idx[5] = 3*s->mb_width + 4 + 1;

            s->block_offset[s->frbi] = (mb_row_flip*16 + mb_offset) * stride_y;
            s->block_offset[s->srbi] = s->block_offset[s->frbi] + 8*stride_y;
            s->block_offset[1] = s->block_offset[0] + 8;
            s->block_offset[3] = s->block_offset[2] + 8;
            s->block_offset[4] = (mb_row_flip*8 + mb_offset) * stride_uv;
            s->block_offset[5] = s->block_offset[4];

            for (mb_col=0; mb_col<s->mb_width; mb_col++) {
                vp56_decode_mb(s, mb_row, mb_col, is_alpha);

                for (y=0; y<4; y++) {
                    s->above_block_idx[y] += 2;
                    s->block_offset[y] += 16;
                }

                for (uv=4; uv<6; uv++) {
                    s->above_block_idx[uv] += 1;
                    s->block_offset[uv] += 8;
                }
            }
        }

	poll_gp0_end();

#ifdef JZC_PMON_P0
	PMON_OFF(test1);
#endif


        if (p->key_frame || golden_frame) {
            if (s->framep[VP56_FRAME_GOLDEN]->data[0] &&
                s->framep[VP56_FRAME_GOLDEN] != s->framep[VP56_FRAME_GOLDEN2])
                avctx->release_buffer(avctx, s->framep[VP56_FRAME_GOLDEN]);
            s->framep[VP56_FRAME_GOLDEN] = p;
        }

        if (s->has_alpha) {
            FFSWAP(AVFrame *, s->framep[VP56_FRAME_GOLDEN],
                              s->framep[VP56_FRAME_GOLDEN2]);
            buf += alpha_offset;
            remaining_buf_size -= alpha_offset;
        }
    }

    if (s->framep[VP56_FRAME_PREVIOUS] == s->framep[VP56_FRAME_GOLDEN] ||
        s->framep[VP56_FRAME_PREVIOUS] == s->framep[VP56_FRAME_GOLDEN2]) {
        if (s->framep[VP56_FRAME_UNUSED] != s->framep[VP56_FRAME_GOLDEN] &&
            s->framep[VP56_FRAME_UNUSED] != s->framep[VP56_FRAME_GOLDEN2])
            FFSWAP(AVFrame *, s->framep[VP56_FRAME_PREVIOUS],
                              s->framep[VP56_FRAME_UNUSED]);
        else
            FFSWAP(AVFrame *, s->framep[VP56_FRAME_PREVIOUS],
                              s->framep[VP56_FRAME_UNUSED2]);
    } else if (s->framep[VP56_FRAME_PREVIOUS]->data[0])
        avctx->release_buffer(avctx, s->framep[VP56_FRAME_PREVIOUS]);
    FFSWAP(AVFrame *, s->framep[VP56_FRAME_CURRENT],
                      s->framep[VP56_FRAME_PREVIOUS]);

    p->qstride = 0;
    p->qscale_table = s->qscale_table;
    p->qscale_type = FF_QSCALE_TYPE_VP56;
    *(AVFrame*)data = *p;
    *data_size = sizeof(AVFrame);

#ifdef JZC_CRC_VER
    {
      int crc_i;
      for(crc_i=0;crc_i<avctx->height;crc_i++)
	crc_code = crc(p->data[0]+crc_i*s->stride[0], 
		       avctx->width, crc_code);
      for(crc_i=0;crc_i<(avctx->height>>1);crc_i++){
	crc_code = crc(p->data[1]+crc_i*s->stride[1], 
		       (avctx->width>>1), crc_code);
	crc_code = crc(p->data[2]+crc_i*s->stride[2], 
		       (avctx->width>>1), crc_code);
      }
      mp_msg(NULL,NULL,"frame: %d, crc_code: 0x%x\n", mpFrame, crc_code);
      mpFrame ++;  
    }
#endif //JZC_CRC_VER

#ifdef JZC_PMON_P0
    {
      int mb_num = s->mb_width * s->mb_height;
      printf("PMON frame num: %d\n",mpFrame-1);
      printf("PMON PARSE -D: %d; I:%d\n",
	     test1_pmon_val/mb_num, test1_pmon_val_ex/mb_num);
      printf("PMON DMB -D: %d; I:%d\n",
	     test2_pmon_val/mb_num, test2_pmon_val_ex/mb_num);
      printf("PMON WAIT -D: %d; I:%d\n",
	     test3_pmon_val/mb_num, test3_pmon_val_ex/mb_num);

      test1_pmon_val=0; test1_pmon_val_ex=0;
      test2_pmon_val=0; test2_pmon_val_ex=0;
      test3_pmon_val=0; test3_pmon_val_ex=0;
    }
#endif

    return avpkt->size;
}

av_cold void ff_vp56_init(AVCodecContext *avctx, int flip, int has_alpha)
{
    VP56Context *s = avctx->priv_data;
    int i;

    S32I2M(xr16,0x3);
#ifdef JZC_CRC_VER
    mpFrame = 0;
    crc_code = 0;
#endif    
   
    unsigned int *tcsm1_zero = (unsigned int *)TCSM1_VUCADDR(0xF4001000);
    tcsm1_zero -= 1;
    for(i=0; i<48; i++){
      S32SDI(xr0,tcsm1_zero,0x4);
      S32SDI(xr0,tcsm1_zero,0x4);
      S32SDI(xr0,tcsm1_zero,0x4);
      S32SDI(xr0,tcsm1_zero,0x4);
    }

    unsigned int *tcsm0_zero = TCSM0_BANK1;
    tcsm0_zero -= 1;
    for(i=0; i<48; i++){
      S32SDI(xr0,tcsm0_zero,0x4);
      S32SDI(xr0,tcsm0_zero,0x4);
      S32SDI(xr0,tcsm0_zero,0x4);
      S32SDI(xr0,tcsm0_zero,0x4);
    }

    s->avctx = avctx;
    avctx->pix_fmt = has_alpha ? PIX_FMT_YUVA420P : PIX_FMT_YUV420P;

    if (avctx->idct_algo == FF_IDCT_AUTO)
        avctx->idct_algo = FF_IDCT_VP3;
    dsputil_init(&s->dsp, avctx);
    ff_vp56dsp_init(&s->vp56dsp, avctx->codec->id);
    ff_init_scantable(s->dsp.idct_permutation, &s->scantable,ff_zigzag_direct);

    for (i=0; i<4; i++)
        s->framep[i] = &s->frames[i];
    s->framep[VP56_FRAME_UNUSED] = s->framep[VP56_FRAME_GOLDEN];
    s->framep[VP56_FRAME_UNUSED2] = s->framep[VP56_FRAME_GOLDEN2];
    s->edge_emu_buffer_alloc = NULL;

    s->above_blocks = NULL;
    s->macroblocks = NULL;
    s->quantizer = -1;
    s->deblock_filtering = 1;

    s->filter = NULL;

    s->has_alpha = has_alpha;
    if (flip) {
        s->flip = -1;
        s->frbi = 2;
        s->srbi = 0;
    } else {
        s->flip = 1;
        s->frbi = 0;
        s->srbi = 2;
    }
}

av_cold int ff_vp56_free(AVCodecContext *avctx)
{
    VP56Context *s = avctx->priv_data;
#ifdef JZC_PMON_P0
    printf("PMON ======================\n");
#endif

#ifdef JZ_LINUX_OS
#else
    av_freep(&s->qscale_table);
    av_freep(&s->above_blocks);
    av_freep(&s->macroblocks);
    av_freep(&s->edge_emu_buffer_alloc);
#endif
    if (s->framep[VP56_FRAME_GOLDEN]->data[0])
        avctx->release_buffer(avctx, s->framep[VP56_FRAME_GOLDEN]);
    if (s->framep[VP56_FRAME_GOLDEN2]->data[0])
        avctx->release_buffer(avctx, s->framep[VP56_FRAME_GOLDEN2]);
    if (s->framep[VP56_FRAME_PREVIOUS]->data[0])
        avctx->release_buffer(avctx, s->framep[VP56_FRAME_PREVIOUS]);
    return 0;
}
