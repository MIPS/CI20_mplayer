/*
 * MPEG4 decoder.
 * Copyright (c) 2000,2001 Fabrice Bellard
 * Copyright (c) 2002-2010 Michael Niedermayer <michaelni@gmx.at>
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

#include "mpegvideo.h"
#include "mpeg4video.h"
#include "h263.h"
#include "mpeg4.h"

#include "jzsoc/jzmedia.h"
#include "jzsoc/jzasm.h"
#include "jzsoc/mpeg4_dcore.h"

//#define get_bits get_bits_mxu
//#define get_bits1 get_bits1_mxu
#define mid_pred mid_pred_mxu

// The defines below define the number of bits that are read at once for
// reading vlc values. Changing these may improve speed and data cache needs
// be aware though that decreasing them may need the number of stages that is
// passed to get_vlc* to be increased.
#define SPRITE_TRAJ_VLC_BITS 6
#define DC_VLC_BITS 9
#define MB_TYPE_B_VLC_BITS 4

//uint8_t val[6];

#define MV_VLC_BITS 9
#define H263_MBTYPE_B_VLC_BITS 6
#define CBPC_B_VLC_BITS 3
#ifdef JZC_VLC_HW_OPT
#include "vlc_bs.c"
extern volatile char * mpeg4_hw_bs_buffer;
#endif

//#define JZC_PMON_P0
//#define STA_CCLK
#ifdef JZC_PMON_P01
#include "../libjzcommon/jz4760e_pmon.h"
PMON_CREAT(mpeg4vlc);
PMON_CREAT(dcp0);
int dcp0 = 0;
#endif
//extern short mpFrame;
#undef printf

static VLC dc_lum, dc_chrom;
static VLC sprite_trajectory;
static VLC mb_type_b_vlc;

extern MPEG4_MB_DecARGs *dMB;

const int8_t JZC_mpeg4_intra_level[103] = {
  1,  2,  3,  4,  5,  6,  7,  8,
  9, 10, 11, 12, 13, 14, 15, 16,
 17, 18, 19, 20, 21, 22, 23, 24,
 25, 26, 27,  1,  2,  3,  4,  5,
  6,  7,  8,  9, 10,  1,  2,  3,
  4,  5,  1,  2,  3,  4,  1,  2,
  3,  1,  2,  3,  1,  2,  3,  1,
  2,  3,  1,  2,  1,  2,  1,  1,
  1,  1,  1,  1,  2,  3,  4,  5,
  6,  7,  8,  1,  2,  3,  1,  2,
  1,  2,  1,  2,  1,  2,  1,  2,
  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  0,
};

const int32_t JZC_mpeg4_intra_run[103] = {
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  2,  2,  2,
  2,  2,  3,  3,  3,  3,  4,  4,
  4,  5,  5,  5,  6,  6,  6,  7,
  7,  7,  8,  8,  9,  9, 10, 11,//64
 12, 13, 14,  192,  192,  192,  192,  192,
  192,  192,  192,  193,  193,  193,  194,  194,//80
  195,  195,  196,  196,  197,  197,  198,  198,//88
  199,  200,  201, 202, 203, 204, 205, 206,//96
 207, 208, 209, 210, 211, 212, 65,
};

static const int mb_type_b_map[4]= {
  MB_TYPE_DIRECT2 | MB_TYPE_L0L1,
  MB_TYPE_L0L1 | MB_TYPE_16x16,
  MB_TYPE_L1 | MB_TYPE_16x16,
  MB_TYPE_L0 | MB_TYPE_16x16,
};

static inline int mid_pred_mxu(int a, int b, int c)
{
  S32I2M(xr1,a);
  S32I2M(xr2,b);
  S32I2M(xr3,c);

  S32MIN(xr4,xr1,xr2);
  S32MAX(xr5,xr1,xr2);
  S32MAX(xr6,xr4,xr3);
  S32MIN(xr7,xr6,xr5);

  return S32M2I(xr7);
}

#if 0 //3IV1 is quite rare and it slows things down a tiny bit
#define IS_3IV1 s->codec_tag == AV_RL32("3IV1")
#else
#define IS_3IV1 0
#endif

/**
 * predicts the dc.
 * encoding quantized level -> quantized diff
 * decoding quantized diff -> quantized level
 * @param n block index (0-3 are luma, 4-5 are chroma)
 * @param dir_ptr pointer to an integer where the prediction direction will be stored
 */
static inline int mpeg4_pred_dc(MpegEncContext * s, int n, int level, int *dir_ptr, int encoding)
{
    int a, b, c, wrap, pred, scale, ret;
    int16_t *dc_val;

    /* find prediction */
    if (n < 4) {
        scale = s->y_dc_scale;
    } else {
        scale = s->c_dc_scale;
    }
    if(IS_3IV1)
        scale= 8;

    wrap= s->block_wrap[n];
    dc_val = s->dc_val[0] + s->block_index[n];

    /* B C
     * A X
     */
    a = dc_val[ - 1];
    b = dc_val[ - 1 - wrap];
    c = dc_val[ - wrap];

    /* outside slice handling (we can't do that by memset as we need the dc for error resilience) */
    if(s->first_slice_line && n!=3){
        if(n!=2) b=c= 1024;
        if(n!=1 && s->mb_x == s->resync_mb_x) b=a= 1024;
    }
    if(s->mb_x == s->resync_mb_x && s->mb_y == s->resync_mb_y+1){
        if(n==0 || n==4 || n==5)
            b=1024;
    }

    if (abs(a - b) < abs(b - c)) {
        pred = c;
        *dir_ptr = 1; /* top */
    } else {
        pred = a;
        *dir_ptr = 0; /* left */
    }
    /* we assume pred is positive */
    pred = FASTDIV((pred + (scale >> 1)), scale);

    if(encoding){
        ret = level - pred;
    }else{
        level += pred;
        ret= level;
        if(s->error_recognition>=3){
            if(level<0){
                av_log(s->avctx, AV_LOG_ERROR, "dc<0 at %dx%d\n", s->mb_x, s->mb_y);
                return -1;
            }
            if(level*scale > 2048 + scale){
                av_log(s->avctx, AV_LOG_ERROR, "dc overflow at %dx%d\n", s->mb_x, s->mb_y);
                return -1;
            }
        }
    }
    level *=scale;
    if(level&(~2047)){
        if(level<0)
            level=0;
        else if(!(s->workaround_bugs&FF_BUG_DC_CLIP))
            level=2047;
    }
    dc_val[0]= level;

    return ret;
}

VLC ff_h263_intra_MCBPC_vlc_hw;
VLC ff_h263_inter_MCBPC_vlc_hw;
VLC ff_h263_cbpy_vlc_hw;
static VLC mv_vlc_hw;
static VLC h263_mbtype_b_vlc;
static VLC cbpc_b_vlc;
void mpeg4_decode_init_vlc(MpegEncContext *s)
{
    static int done = 0;

    if (!done) {
        done = 1;

        INIT_VLC_STATIC(&ff_h263_intra_MCBPC_vlc_hw, INTRA_MCBPC_VLC_BITS, 9,
                 ff_h263_intra_MCBPC_bits, 1, 1,
                 ff_h263_intra_MCBPC_code, 1, 1, 72);
        INIT_VLC_STATIC(&ff_h263_inter_MCBPC_vlc_hw, INTER_MCBPC_VLC_BITS, 28,
                 ff_h263_inter_MCBPC_bits, 1, 1,
                 ff_h263_inter_MCBPC_code, 1, 1, 198);
        INIT_VLC_STATIC(&ff_h263_cbpy_vlc_hw, CBPY_VLC_BITS, 16,
                 &ff_h263_cbpy_tab[0][1], 2, 1,
                 &ff_h263_cbpy_tab[0][0], 2, 1, 64);
        INIT_VLC_STATIC(&mv_vlc_hw, MV_VLC_BITS, 33,
                 &mvtab[0][1], 2, 1,
                 &mvtab[0][0], 2, 1, 538);

        init_rl(&ff_h263_rl_inter_hw, ff_h263_static_rl_table_store[0]);
        INIT_VLC_RL_HW(ff_h263_rl_inter_hw, 554);

        init_rl(&ff_h263_rl_inter, ff_h263_static_rl_table_store[0]);
        init_rl(&rl_intra_aic, ff_h263_static_rl_table_store[1]);
        INIT_VLC_RL(ff_h263_rl_inter, 554);
        INIT_VLC_RL(rl_intra_aic, 554);
        INIT_VLC_STATIC(&h263_mbtype_b_vlc, H263_MBTYPE_B_VLC_BITS, 15,
                 &h263_mbtype_b_tab[0][1], 2, 1,
                 &h263_mbtype_b_tab[0][0], 2, 1, 80);
        INIT_VLC_STATIC(&cbpc_b_vlc, CBPC_B_VLC_BITS, 4,
                 &cbpc_b_tab[0][1], 2, 1,
                 &cbpc_b_tab[0][0], 2, 1, 8);
    }
}

int mpeg4_decode_motion(MpegEncContext * s, int pred, int f_code)
{
  int code, val, sign, shift, l;
  code = get_vlc2(&s->gb, mv_vlc_hw.table, MV_VLC_BITS, 2);

  if (code == 0)
    return pred;
  if (code < 0)
    return 0xffff;

  sign = get_bits1(&s->gb);
  shift = f_code - 1;
  val = code;
  if (shift) {
    val = (val - 1) << shift;
    val |= get_bits(&s->gb, shift);
    val++;
  }
  if (sign)
    val = -val;
  val += pred;

  /* modulo decoding */
  if (!s->h263_long_vectors) {
    l = INT_BIT - 5 - f_code;
    val = (val<<l)>>l;
  } else {
    /* horrible h263 long vector mode */
    if (pred < -31 && val < -63)
      val += 64;
    if (pred > 32 && val > 63)
      val -= 64;

  }
  return val;
}


int16_t *mpeg4_pred_motion(MpegEncContext * s, int block, int dir,
			   int *px, int *py)
{
    int wrap;
    int16_t *A, *B, *C, (*mot_val)[2];
    static const int off[4]= {2, 1, 1, -1};

    wrap = s->b8_stride;
    mot_val = s->current_picture.motion_val[dir] + s->block_index[block];

    A = mot_val[ - 1];
    /* special case for first (slice) line */
    if (s->first_slice_line && block<3) {
        // we can't just change some MVs to simulate that as we need them for the B frames (and ME)
        // and if we ever support non rectangular objects than we need to do a few ifs here anyway :(
        if(block==0){ //most common case
            if(s->mb_x  == s->resync_mb_x){ //rare
                *px= *py = 0;
            }else if(s->mb_x + 1 == s->resync_mb_x && s->h263_pred){ //rare
                C = mot_val[off[block] - wrap];
                if(s->mb_x==0){
                    *px = C[0];
                    *py = C[1];
                }else{
                    *px = mid_pred(A[0], 0, C[0]);
                    *py = mid_pred(A[1], 0, C[1]);
                }
            }else{
                *px = A[0];
                *py = A[1];
            }
        }else if(block==1){
            if(s->mb_x + 1 == s->resync_mb_x && s->h263_pred){ //rare
                C = mot_val[off[block] - wrap];
                *px = mid_pred(A[0], 0, C[0]);
                *py = mid_pred(A[1], 0, C[1]);
            }else{
                *px = A[0];
                *py = A[1];
            }
        }else{ /* block==2*/
            B = mot_val[ - wrap];
            C = mot_val[off[block] - wrap];
            if(s->mb_x == s->resync_mb_x) //rare
                A[0]=A[1]=0;

            *px = mid_pred(A[0], B[0], C[0]);
            *py = mid_pred(A[1], B[1], C[1]);
        }
    } else {
        B = mot_val[ - wrap];
        C = mot_val[off[block] - wrap];
        *px = mid_pred(A[0], B[0], C[0]);
        *py = mid_pred(A[1], B[1], C[1]);
    }
    return *mot_val;
}

static inline void JZC_mpeg4_pred_ac(MpegEncContext * s, DCTELEM *block, int n,
		      int dir)
{
    int i;
    int16_t *ac_val, *ac_val1;
    int8_t * const qscale_table= s->current_picture.qscale_table;

    /* find prediction */
    ac_val = s->ac_val[0][0] + s->block_index[n] * 16;
    ac_val1 = ac_val;
    if (s->ac_pred) {
        if (dir == 0) {
            const int xy= s->mb_x-1 + s->mb_y*s->mb_stride;
            /* left prediction */
            ac_val -= 16;

            if(s->mb_x==0 || s->qscale == qscale_table[xy] || n==1 || n==3){
                /* same qscale */
                for(i=1;i<8;i++) {
                    block[s->dsp.idct_permutation[i<<3]] += ac_val[i];
                }
            }else{
                /* different qscale, we must rescale */
                for(i=1;i<8;i++) {
                    block[s->dsp.idct_permutation[i<<3]] += ROUNDED_DIV(ac_val[i]*qscale_table[xy], s->qscale);
                }
            }
        } else {
            const int xy= s->mb_x + s->mb_y*s->mb_stride - s->mb_stride;
            /* top prediction */
            ac_val -= 16 * s->block_wrap[n];

            if(s->mb_y==0 || s->qscale == qscale_table[xy] || n==2 || n==3){
                /* same qscale */
                for(i=1;i<8;i++) {
                    block[s->dsp.idct_permutation[i]] += ac_val[i + 8];
                }
            }else{
                /* different qscale, we must rescale */
                for(i=1;i<8;i++) {
                    block[s->dsp.idct_permutation[i]] += ROUNDED_DIV(ac_val[i + 8]*qscale_table[xy], s->qscale);
                }
            }
        }
    }
    /* left copy */
    for(i=1;i<8;i++)
        ac_val1[i    ] = block[s->dsp.idct_permutation[i<<3]];

    /* top copy */
    for(i=1;i<8;i++)
        ac_val1[8 + i] = block[s->dsp.idct_permutation[i   ]];

}
/**
 * check if the next stuff is a resync marker or the end.
 * @return 0 if not
 */
static inline int mpeg4_is_resync(MpegEncContext *s){
    int bits_count= get_bits_count(&s->gb);
    int v= show_bits(&s->gb, 16);

    if(s->workaround_bugs&FF_BUG_NO_PADDING){
        return 0;
    }

    while(v<=0xFF){
        if(s->pict_type==FF_B_TYPE || (v>>(8-s->pict_type)!=1) || s->partitioned_frame)
            break;
        skip_bits(&s->gb, 8+s->pict_type);
        bits_count+= 8+s->pict_type;
        v= show_bits(&s->gb, 16);
    }

    if(bits_count + 8 >= s->gb.size_in_bits){
        v>>=8;
        v|= 0x7F >> (7-(bits_count&7));

        if(v==0x7F)
            return 1;
    }else{
        if(v == ff_mpeg4_resync_prefix[bits_count&7]){
            int len;
            GetBitContext gb= s->gb;

            skip_bits(&s->gb, 1);
            align_get_bits(&s->gb);

            for(len=0; len<32; len++){
                if(get_bits1(&s->gb)) break;
            }

            s->gb= gb;

            if(len>=ff_mpeg4_get_video_packet_prefix_length(s))
                return 1;
        }
    }
    return 0;
}

static void mpeg4_decode_sprite_trajectory(MpegEncContext * s, GetBitContext *gb)
{
  int i;
  int a= 2<<s->sprite_warping_accuracy;
  int rho= 3-s->sprite_warping_accuracy;
  int r=16/a;
  const int vop_ref[4][2]= {{0,0}, {s->width,0}, {0, s->height}, {s->width, s->height}}; // only true for rectangle shapes
  int d[4][2]={{0,0}, {0,0}, {0,0}, {0,0}};
  int sprite_ref[4][2];
  int virtual_ref[2][2];
  int w2, h2, w3, h3;
  int alpha=0, beta=0;
  int w= s->width;
  int h= s->height;
  int min_ab;

  for(i=0; i<s->num_sprite_warping_points; i++){
    int length;
    int x=0, y=0;

    length= get_vlc2(gb, sprite_trajectory.table, SPRITE_TRAJ_VLC_BITS, 3);
    if(length){
      x= get_xbits(gb, length);
    }
    if(!(s->divx_version==500 && s->divx_build==413)) skip_bits1(gb); /* marker bit */

    length= get_vlc2(gb, sprite_trajectory.table, SPRITE_TRAJ_VLC_BITS, 3);
    if(length){
      y=get_xbits(gb, length);
    }
    skip_bits1(gb); /* marker bit */
    s->sprite_traj[i][0]= d[i][0]= x;
    s->sprite_traj[i][1]= d[i][1]= y;
  }
  for(; i<4; i++)
    s->sprite_traj[i][0]= s->sprite_traj[i][1]= 0;

  while((1<<alpha)<w) alpha++;
  while((1<<beta )<h) beta++; // there seems to be a typo in the mpeg4 std for the definition of w' and h'
  w2= 1<<alpha;
  h2= 1<<beta;

  // Note, the 4th point isn't used for GMC
  if(s->divx_version==500 && s->divx_build==413){
    sprite_ref[0][0]= a*vop_ref[0][0] + d[0][0];
    sprite_ref[0][1]= a*vop_ref[0][1] + d[0][1];
    sprite_ref[1][0]= a*vop_ref[1][0] + d[0][0] + d[1][0];
    sprite_ref[1][1]= a*vop_ref[1][1] + d[0][1] + d[1][1];
    sprite_ref[2][0]= a*vop_ref[2][0] + d[0][0] + d[2][0];
    sprite_ref[2][1]= a*vop_ref[2][1] + d[0][1] + d[2][1];
  } else {
    sprite_ref[0][0]= (a>>1)*(2*vop_ref[0][0] + d[0][0]);
    sprite_ref[0][1]= (a>>1)*(2*vop_ref[0][1] + d[0][1]);
    sprite_ref[1][0]= (a>>1)*(2*vop_ref[1][0] + d[0][0] + d[1][0]);
    sprite_ref[1][1]= (a>>1)*(2*vop_ref[1][1] + d[0][1] + d[1][1]);
    sprite_ref[2][0]= (a>>1)*(2*vop_ref[2][0] + d[0][0] + d[2][0]);
    sprite_ref[2][1]= (a>>1)*(2*vop_ref[2][1] + d[0][1] + d[2][1]);
  }
  /*    sprite_ref[3][0]= (a>>1)*(2*vop_ref[3][0] + d[0][0] + d[1][0] + d[2][0] + d[3][0]);
	sprite_ref[3][1]= (a>>1)*(2*vop_ref[3][1] + d[0][1] + d[1][1] + d[2][1] + d[3][1]); */

  // this is mostly identical to the mpeg4 std (and is totally unreadable because of that ...)
  // perhaps it should be reordered to be more readable ...
  // the idea behind this virtual_ref mess is to be able to use shifts later per pixel instead of divides
  // so the distance between points is converted from w&h based to w2&h2 based which are of the 2^x form
  virtual_ref[0][0]= 16*(vop_ref[0][0] + w2)
    + ROUNDED_DIV(((w - w2)*(r*sprite_ref[0][0] - 16*vop_ref[0][0]) + w2*(r*sprite_ref[1][0] - 16*vop_ref[1][0])),w);
  virtual_ref[0][1]= 16*vop_ref[0][1]
    + ROUNDED_DIV(((w - w2)*(r*sprite_ref[0][1] - 16*vop_ref[0][1]) + w2*(r*sprite_ref[1][1] - 16*vop_ref[1][1])),w);
  virtual_ref[1][0]= 16*vop_ref[0][0]
    + ROUNDED_DIV(((h - h2)*(r*sprite_ref[0][0] - 16*vop_ref[0][0]) + h2*(r*sprite_ref[2][0] - 16*vop_ref[2][0])),h);
  virtual_ref[1][1]= 16*(vop_ref[0][1] + h2)
    + ROUNDED_DIV(((h - h2)*(r*sprite_ref[0][1] - 16*vop_ref[0][1]) + h2*(r*sprite_ref[2][1] - 16*vop_ref[2][1])),h);

  switch(s->num_sprite_warping_points)
    {
    case 0:
      s->sprite_offset[0][0]= 0;
      s->sprite_offset[0][1]= 0;
      s->sprite_offset[1][0]= 0;
      s->sprite_offset[1][1]= 0;
      s->sprite_delta[0][0]= a;
      s->sprite_delta[0][1]= 0;
      s->sprite_delta[1][0]= 0;
      s->sprite_delta[1][1]= a;
      s->sprite_shift[0]= 0;
      s->sprite_shift[1]= 0;
      break;
    case 1: //GMC only
      s->sprite_offset[0][0]= sprite_ref[0][0] - a*vop_ref[0][0];
      s->sprite_offset[0][1]= sprite_ref[0][1] - a*vop_ref[0][1];
      s->sprite_offset[1][0]= ((sprite_ref[0][0]>>1)|(sprite_ref[0][0]&1)) - a*(vop_ref[0][0]/2);
      s->sprite_offset[1][1]= ((sprite_ref[0][1]>>1)|(sprite_ref[0][1]&1)) - a*(vop_ref[0][1]/2);
      s->sprite_delta[0][0]= a;
      s->sprite_delta[0][1]= 0;
      s->sprite_delta[1][0]= 0;
      s->sprite_delta[1][1]= a;
      s->sprite_shift[0]= 0;
      s->sprite_shift[1]= 0;
      break;
    case 2:
      s->sprite_offset[0][0]= (sprite_ref[0][0]<<(alpha+rho))
	+ (-r*sprite_ref[0][0] + virtual_ref[0][0])*(-vop_ref[0][0])
	+ ( r*sprite_ref[0][1] - virtual_ref[0][1])*(-vop_ref[0][1])
	+ (1<<(alpha+rho-1));
      s->sprite_offset[0][1]= (sprite_ref[0][1]<<(alpha+rho))
	+ (-r*sprite_ref[0][1] + virtual_ref[0][1])*(-vop_ref[0][0])
	+ (-r*sprite_ref[0][0] + virtual_ref[0][0])*(-vop_ref[0][1])
	+ (1<<(alpha+rho-1));
      s->sprite_offset[1][0]= ( (-r*sprite_ref[0][0] + virtual_ref[0][0])*(-2*vop_ref[0][0] + 1)
				+( r*sprite_ref[0][1] - virtual_ref[0][1])*(-2*vop_ref[0][1] + 1)
				+2*w2*r*sprite_ref[0][0]
				- 16*w2
				+ (1<<(alpha+rho+1)));
      s->sprite_offset[1][1]= ( (-r*sprite_ref[0][1] + virtual_ref[0][1])*(-2*vop_ref[0][0] + 1)
				+(-r*sprite_ref[0][0] + virtual_ref[0][0])*(-2*vop_ref[0][1] + 1)
				+2*w2*r*sprite_ref[0][1]
				- 16*w2
				+ (1<<(alpha+rho+1)));
      s->sprite_delta[0][0]=   (-r*sprite_ref[0][0] + virtual_ref[0][0]);
      s->sprite_delta[0][1]=   (+r*sprite_ref[0][1] - virtual_ref[0][1]);
      s->sprite_delta[1][0]=   (-r*sprite_ref[0][1] + virtual_ref[0][1]);
      s->sprite_delta[1][1]=   (-r*sprite_ref[0][0] + virtual_ref[0][0]);

      s->sprite_shift[0]= alpha+rho;
      s->sprite_shift[1]= alpha+rho+2;
      break;
    case 3:
      min_ab= FFMIN(alpha, beta);
      w3= w2>>min_ab;
      h3= h2>>min_ab;
      s->sprite_offset[0][0]=  (sprite_ref[0][0]<<(alpha+beta+rho-min_ab))
	+ (-r*sprite_ref[0][0] + virtual_ref[0][0])*h3*(-vop_ref[0][0])
	+ (-r*sprite_ref[0][0] + virtual_ref[1][0])*w3*(-vop_ref[0][1])
	+ (1<<(alpha+beta+rho-min_ab-1));
      s->sprite_offset[0][1]=  (sprite_ref[0][1]<<(alpha+beta+rho-min_ab))
	+ (-r*sprite_ref[0][1] + virtual_ref[0][1])*h3*(-vop_ref[0][0])
	+ (-r*sprite_ref[0][1] + virtual_ref[1][1])*w3*(-vop_ref[0][1])
	+ (1<<(alpha+beta+rho-min_ab-1));
      s->sprite_offset[1][0]=  (-r*sprite_ref[0][0] + virtual_ref[0][0])*h3*(-2*vop_ref[0][0] + 1)
	+ (-r*sprite_ref[0][0] + virtual_ref[1][0])*w3*(-2*vop_ref[0][1] + 1)
	+ 2*w2*h3*r*sprite_ref[0][0]
	- 16*w2*h3
	+ (1<<(alpha+beta+rho-min_ab+1));
      s->sprite_offset[1][1]=  (-r*sprite_ref[0][1] + virtual_ref[0][1])*h3*(-2*vop_ref[0][0] + 1)
	+ (-r*sprite_ref[0][1] + virtual_ref[1][1])*w3*(-2*vop_ref[0][1] + 1)
	+ 2*w2*h3*r*sprite_ref[0][1]
	- 16*w2*h3
	+ (1<<(alpha+beta+rho-min_ab+1));
      s->sprite_delta[0][0]=   (-r*sprite_ref[0][0] + virtual_ref[0][0])*h3;
      s->sprite_delta[0][1]=   (-r*sprite_ref[0][0] + virtual_ref[1][0])*w3;
      s->sprite_delta[1][0]=   (-r*sprite_ref[0][1] + virtual_ref[0][1])*h3;
      s->sprite_delta[1][1]=   (-r*sprite_ref[0][1] + virtual_ref[1][1])*w3;

      s->sprite_shift[0]= alpha + beta + rho - min_ab;
      s->sprite_shift[1]= alpha + beta + rho - min_ab + 2;
      break;
    }
  /* try to simplify the situation */
  if(   s->sprite_delta[0][0] == a<<s->sprite_shift[0]
	&& s->sprite_delta[0][1] == 0
	&& s->sprite_delta[1][0] == 0
	&& s->sprite_delta[1][1] == a<<s->sprite_shift[0])
    {
      s->sprite_offset[0][0]>>=s->sprite_shift[0];
      s->sprite_offset[0][1]>>=s->sprite_shift[0];
      s->sprite_offset[1][0]>>=s->sprite_shift[1];
      s->sprite_offset[1][1]>>=s->sprite_shift[1];
      s->sprite_delta[0][0]= a;
      s->sprite_delta[0][1]= 0;
      s->sprite_delta[1][0]= 0;
      s->sprite_delta[1][1]= a;
      s->sprite_shift[0]= 0;
      s->sprite_shift[1]= 0;
      s->real_sprite_warping_points=1;
    }
  else{
    int shift_y= 16 - s->sprite_shift[0];
    int shift_c= 16 - s->sprite_shift[1];
    for(i=0; i<2; i++){
      s->sprite_offset[0][i]<<= shift_y;
      s->sprite_offset[1][i]<<= shift_c;
      s->sprite_delta[0][i]<<= shift_y;
      s->sprite_delta[1][i]<<= shift_y;
      s->sprite_shift[i]= 16;
    }
    s->real_sprite_warping_points= s->num_sprite_warping_points;
  }
}

/**
 * gets the average motion vector for a GMC MB.
 * @param n either 0 for the x component or 1 for y
 * @return the average MV for a GMC MB
 */
static inline int get_amv(MpegEncContext *s, int n){
  int x, y, mb_v, sum, dx, dy, shift;
  int len = 1 << (s->f_code + 4);
  const int a= s->sprite_warping_accuracy;

  if(s->workaround_bugs & FF_BUG_AMV)
    len >>= s->quarter_sample;

  if(s->real_sprite_warping_points==1){
    if(s->divx_version==500 && s->divx_build==413)
      sum= s->sprite_offset[0][n] / (1<<(a - s->quarter_sample));
    else
      sum= RSHIFT(s->sprite_offset[0][n]<<s->quarter_sample, a);
  }else{
    dx= s->sprite_delta[n][0];
    dy= s->sprite_delta[n][1];
    shift= s->sprite_shift[0];
    if(n) dy -= 1<<(shift + a + 1);
    else  dx -= 1<<(shift + a + 1);
    mb_v= s->sprite_offset[0][n] + dx*s->mb_x*16 + dy*s->mb_y*16;

    sum=0;
    for(y=0; y<16; y++){
      int v;

      v= mb_v + dy*y;
      //XXX FIXME optimize
      for(x=0; x<16; x++){
	sum+= v>>shift;
	v+= dx;
      }
    }
    sum= RSHIFT(sum, a+8-s->quarter_sample);
  }

  if      (sum < -len) sum= -len;
  else if (sum >= len) sum= len-1;

  return sum;
}

/**
 * decodes the dc value.
 * @param n block index (0-3 are luma, 4-5 are chroma)
 * @param dir_ptr the prediction direction will be stored here
 * @return the quantized dc
 */
static inline int mpeg4_decode_dc(MpegEncContext * s, int n, int *dir_ptr)
{
  int level, code;

  if (n < 4)
    code = get_vlc2(&s->gb, dc_lum.table, DC_VLC_BITS, 1);
  else
    code = get_vlc2(&s->gb, dc_chrom.table, DC_VLC_BITS, 1);
  if (code < 0 || code > 9 /* && s->nbit<9 */){
    av_log(s->avctx, AV_LOG_ERROR, "illegal dc vlc\n");
    return -1;
  }
  if (code == 0) {
    level = 0;
  } else {
    if(IS_3IV1){
      if(code==1)
	level= 2*get_bits1(&s->gb)-1;
      else{
	if(get_bits1(&s->gb))
	  level = get_bits(&s->gb, code-1) + (1<<(code-1));
	else
	  level = -get_bits(&s->gb, code-1) - (1<<(code-1));
      }
    }else{
      level = get_xbits(&s->gb, code);
    }

    if (code > 8){
      if(get_bits1(&s->gb)==0){ /* marker */
	if(s->error_recognition>=2){
	  av_log(s->avctx, AV_LOG_ERROR, "dc marker bit missing\n");
	  return -1;
	}
      }
    }
  }

  return mpeg4_pred_dc(s, n, level, dir_ptr, 0);
}

/**
 * decodes first partition.
 * @return number of MBs decoded or <0 if an error occurred
 */
static int mpeg4_decode_partition_a(MpegEncContext *s){
  int mb_num;
  static const int8_t quant_tab[4] = { -1, -2, 1, 2 };

  /* decode first partition */
  mb_num=0;
  s->first_slice_line=1;
  for(; s->mb_y<s->mb_height; s->mb_y++){
    ff_init_block_index(s);
    for(; s->mb_x<s->mb_width; s->mb_x++){
      const int xy= s->mb_x + s->mb_y*s->mb_stride;
      int cbpc;
      int dir=0;

      mb_num++;
      ff_update_block_index(s);
      if(s->mb_x == s->resync_mb_x && s->mb_y == s->resync_mb_y+1)
	s->first_slice_line=0;

      if(s->pict_type==FF_I_TYPE){
	int i;

	do{
	  if(show_bits_long(&s->gb, 19)==DC_MARKER){
	    return mb_num-1;
	  }

	  cbpc = get_vlc2(&s->gb, ff_h263_intra_MCBPC_vlc_hw.table, INTRA_MCBPC_VLC_BITS, 2);
	  if (cbpc < 0){
	    av_log(s->avctx, AV_LOG_ERROR, "cbpc corrupted at %d %d\n", s->mb_x, s->mb_y);
	    return -1;
	  }
	}while(cbpc == 8);

	s->cbp_table[xy]= cbpc & 3;
	s->current_picture.mb_type[xy]= MB_TYPE_INTRA;
	s->mb_intra = 1;

	if(cbpc & 4) {
	  ff_set_qscale(s, s->qscale + quant_tab[get_bits(&s->gb, 2)]);
	}
	s->current_picture.qscale_table[xy]= s->qscale;

	s->mbintra_table[xy]= 1;
	for(i=0; i<6; i++){
	  int dc_pred_dir;
	  int dc= mpeg4_decode_dc(s, i, &dc_pred_dir);
	  if(dc < 0){
	    av_log(s->avctx, AV_LOG_ERROR, "DC corrupted at %d %d\n", s->mb_x, s->mb_y);
	    return -1;
	  }
	  dir<<=1;
	  if(dc_pred_dir) dir|=1;
	}
	s->pred_dir_table[xy]= dir;
      }else{ /* P/S_TYPE */
	int mx, my, pred_x, pred_y, bits;
	int16_t * const mot_val= s->current_picture.motion_val[0][s->block_index[0]];
	const int stride= s->b8_stride*2;

      try_again:
	bits= show_bits(&s->gb, 17);
	if(bits==MOTION_MARKER){
	  return mb_num-1;
	}
	skip_bits1(&s->gb);
	if(bits&0x10000){
	  /* skip mb */
	  if(s->pict_type==FF_S_TYPE && s->vol_sprite_usage==GMC_SPRITE){
	    s->current_picture.mb_type[xy]= MB_TYPE_SKIP | MB_TYPE_16x16 | MB_TYPE_GMC | MB_TYPE_L0;
	    mx= get_amv(s, 0);
	    my= get_amv(s, 1);
	  }else{
	    s->current_picture.mb_type[xy]= MB_TYPE_SKIP | MB_TYPE_16x16 | MB_TYPE_L0;
	    mx=my=0;
	  }
	  mot_val[0       ]= mot_val[2       ]=
	    mot_val[0+stride]= mot_val[2+stride]= mx;
	  mot_val[1       ]= mot_val[3       ]=
	    mot_val[1+stride]= mot_val[3+stride]= my;

	  if(s->mbintra_table[xy])
	    ff_clean_intra_table_entries(s);
	  continue;
	}

	cbpc = get_vlc2(&s->gb, ff_h263_inter_MCBPC_vlc_hw.table, INTER_MCBPC_VLC_BITS, 2);
	if (cbpc < 0){
	  av_log(s->avctx, AV_LOG_ERROR, "cbpc corrupted at %d %d\n", s->mb_x, s->mb_y);
	  return -1;
	}
	if(cbpc == 20)
	  goto try_again;

	s->cbp_table[xy]= cbpc&(8+3); //8 is dquant

	s->mb_intra = ((cbpc & 4) != 0);

	if(s->mb_intra){
	  s->current_picture.mb_type[xy]= MB_TYPE_INTRA;
	  s->mbintra_table[xy]= 1;
	  mot_val[0       ]= mot_val[2       ]=
	    mot_val[0+stride]= mot_val[2+stride]= 0;
	  mot_val[1       ]= mot_val[3       ]=
	    mot_val[1+stride]= mot_val[3+stride]= 0;
	}else{
	  if(s->mbintra_table[xy])
	    ff_clean_intra_table_entries(s);

	  if(s->pict_type==FF_S_TYPE && s->vol_sprite_usage==GMC_SPRITE && (cbpc & 16) == 0)
	    s->mcsel= get_bits1(&s->gb);
	  else s->mcsel= 0;

	  if ((cbpc & 16) == 0) {
	    /* 16x16 motion prediction */

	    h263_pred_motion(s, 0, 0, &pred_x, &pred_y);
	    if(!s->mcsel){
	      mx = h263_decode_motion(s, pred_x, s->f_code);
	      if (mx >= 0xffff)
		return -1;

	      my = h263_decode_motion(s, pred_y, s->f_code);
	      if (my >= 0xffff)
		return -1;
	      s->current_picture.mb_type[xy]= MB_TYPE_16x16 | MB_TYPE_L0;
	    } else {
	      mx = get_amv(s, 0);
	      my = get_amv(s, 1);
	      s->current_picture.mb_type[xy]= MB_TYPE_16x16 | MB_TYPE_GMC | MB_TYPE_L0;
	    }

	    mot_val[0       ]= mot_val[2       ] =
	      mot_val[0+stride]= mot_val[2+stride]= mx;
	    mot_val[1       ]= mot_val[3       ]=
	      mot_val[1+stride]= mot_val[3+stride]= my;
	  } else {
	    int i;
	    s->current_picture.mb_type[xy]= MB_TYPE_8x8 | MB_TYPE_L0;
	    for(i=0;i<4;i++) {
	      int16_t *mot_val= h263_pred_motion(s, i, 0, &pred_x, &pred_y);
	      mx = h263_decode_motion(s, pred_x, s->f_code);
	      if (mx >= 0xffff)
		return -1;

	      my = h263_decode_motion(s, pred_y, s->f_code);
	      if (my >= 0xffff)
		return -1;
	      mot_val[0] = mx;
	      mot_val[1] = my;
	    }
	  }
	}
      }
    }
    s->mb_x= 0;
  }

  return mb_num;
}

/**
 * decode second partition.
 * @return <0 if an error occurred
 */
static int mpeg4_decode_partition_b(MpegEncContext *s, int mb_count){
  int mb_num=0;
  static const int8_t quant_tab[4] = { -1, -2, 1, 2 };

  s->mb_x= s->resync_mb_x;
  s->first_slice_line=1;
  for(s->mb_y= s->resync_mb_y; mb_num < mb_count; s->mb_y++){
    ff_init_block_index(s);
    for(; mb_num < mb_count && s->mb_x<s->mb_width; s->mb_x++){
      const int xy= s->mb_x + s->mb_y*s->mb_stride;

      mb_num++;
      ff_update_block_index(s);
      if(s->mb_x == s->resync_mb_x && s->mb_y == s->resync_mb_y+1)
	s->first_slice_line=0;

      if(s->pict_type==FF_I_TYPE){
	int ac_pred= get_bits1(&s->gb);
	int cbpy = get_vlc2(&s->gb, ff_h263_cbpy_vlc_hw.table, CBPY_VLC_BITS, 1);
	if(cbpy<0){
	  av_log(s->avctx, AV_LOG_ERROR, "cbpy corrupted at %d %d\n", s->mb_x, s->mb_y);
	  return -1;
	}

	s->cbp_table[xy]|= cbpy<<2;
	s->current_picture.mb_type[xy] |= ac_pred*MB_TYPE_ACPRED;
      }else{ /* P || S_TYPE */
	if(IS_INTRA(s->current_picture.mb_type[xy])){
	  int dir=0,i;
	  int ac_pred = get_bits1(&s->gb);
	  int cbpy = get_vlc2(&s->gb, ff_h263_cbpy_vlc_hw.table, CBPY_VLC_BITS, 1);

	  if(cbpy<0){
	    av_log(s->avctx, AV_LOG_ERROR, "I cbpy corrupted at %d %d\n", s->mb_x, s->mb_y);
	    return -1;
	  }

	  if(s->cbp_table[xy] & 8) {
	    ff_set_qscale(s, s->qscale + quant_tab[get_bits(&s->gb, 2)]);
	  }
	  s->current_picture.qscale_table[xy]= s->qscale;

	  for(i=0; i<6; i++){
	    int dc_pred_dir;
	    int dc= mpeg4_decode_dc(s, i, &dc_pred_dir);
	    if(dc < 0){
	      av_log(s->avctx, AV_LOG_ERROR, "DC corrupted at %d %d\n", s->mb_x, s->mb_y);
	      return -1;
	    }
	    dir<<=1;
	    if(dc_pred_dir) dir|=1;
	  }
	  s->cbp_table[xy]&= 3; //remove dquant
	  s->cbp_table[xy]|= cbpy<<2;
	  s->current_picture.mb_type[xy] |= ac_pred*MB_TYPE_ACPRED;
	  s->pred_dir_table[xy]= dir;
	}else if(IS_SKIP(s->current_picture.mb_type[xy])){
	  s->current_picture.qscale_table[xy]= s->qscale;
	  s->cbp_table[xy]= 0;
	}else{
	  int cbpy = get_vlc2(&s->gb, ff_h263_cbpy_vlc_hw.table, CBPY_VLC_BITS, 1);

	  if(cbpy<0){
	    av_log(s->avctx, AV_LOG_ERROR, "P cbpy corrupted at %d %d\n", s->mb_x, s->mb_y);
	    return -1;
	  }

	  if(s->cbp_table[xy] & 8) {
	    ff_set_qscale(s, s->qscale + quant_tab[get_bits(&s->gb, 2)]);
	  }
	  s->current_picture.qscale_table[xy]= s->qscale;

	  s->cbp_table[xy]&= 3; //remove dquant
	  s->cbp_table[xy]|= (cbpy^0xf)<<2;
	}
      }
    }
    if(mb_num >= mb_count) return 0;
    s->mb_x= 0;
  }
  return 0;
}

static inline int mpeg4_decode_block(MpegEncContext * s, DCTELEM * block,
                              int n, int coded, int intra, int rvlc)
{
    int level, i, last, run;
    int dc_pred_dir;
    RLTable * rl;
    RL_VLC_ELEM * rl_vlc;
    const uint8_t * scan_table;
    int qmul, qadd;

    //Note intra & rvlc should be optimized away if this is inlined

    if(intra) {
      if(s->use_intra_dc_vlc){
        /* DC coef */
        if(s->partitioned_frame){
            level = s->dc_val[0][ s->block_index[n] ];
            if(n<4) level= FASTDIV((level + (s->y_dc_scale>>1)), s->y_dc_scale);
            else    level= FASTDIV((level + (s->c_dc_scale>>1)), s->c_dc_scale);
            dc_pred_dir= (s->pred_dir_table[s->mb_x + s->mb_y*s->mb_stride]<<n)&32;
        }else{
            level = mpeg4_decode_dc(s, n, &dc_pred_dir);
            if (level < 0)
                return -1;
        }
        block[0] = level;
        i = 0;
      }else{
            i = -1;
            mpeg4_pred_dc(s, n, 0, &dc_pred_dir, 0);
      }
      if (!coded)
          goto not_coded;

      if(rvlc){
          rl = &rvlc_rl_intra;
          rl_vlc = rvlc_rl_intra.rl_vlc[0];
      }else{
          rl = &ff_mpeg4_rl_intra;
          rl_vlc = ff_mpeg4_rl_intra.rl_vlc[0];
      }
      if (s->ac_pred) {
          if (dc_pred_dir == 0)
              scan_table = s->intra_v_scantable.permutated; /* left */
          else
              scan_table = s->intra_h_scantable.permutated; /* top */
      } else {
            scan_table = s->intra_scantable.permutated;
      }
      qmul=1;
      qadd=0;
    } else {
        i = -1;
        if (!coded) {
            s->block_last_index[n] = i;
            return 0;
        }
        if(rvlc) rl = &rvlc_rl_inter;
        else     rl = &ff_h263_rl_inter;

        scan_table = s->intra_scantable.permutated;

        if(s->mpeg_quant){
            qmul=1;
            qadd=0;
            if(rvlc){
                rl_vlc = rvlc_rl_inter.rl_vlc[0];
            }else{
                rl_vlc = ff_h263_rl_inter.rl_vlc[0];
            }
        }else{
	  //qmul = s->qscale << 1;
	  //qadd = (s->qscale - 1) | 1;
	  qmul = 1;
	  qadd = 0;
            if(rvlc){
                rl_vlc = rvlc_rl_inter.rl_vlc[0];
            }else{
                rl_vlc = ff_h263_rl_inter.rl_vlc[0];
            }
        }
    }
  {
    OPEN_READER(re, &s->gb);
    for(;;) {
        UPDATE_CACHE(re, &s->gb);
        GET_RL_VLC(level, run, re, &s->gb, rl_vlc, TEX_VLC_BITS, 2, 0);
        if (level==0) {
          /* escape */
          if(rvlc){
                if(SHOW_UBITS(re, &s->gb, 1)==0){
                    av_log(s->avctx, AV_LOG_ERROR, "1. marker bit missing in rvlc esc\n");
                    return -1;
                }; SKIP_CACHE(re, &s->gb, 1);

                last=  SHOW_UBITS(re, &s->gb, 1); SKIP_CACHE(re, &s->gb, 1);
                run=   SHOW_UBITS(re, &s->gb, 6); LAST_SKIP_CACHE(re, &s->gb, 6);
                SKIP_COUNTER(re, &s->gb, 1+1+6);
                UPDATE_CACHE(re, &s->gb);

                if(SHOW_UBITS(re, &s->gb, 1)==0){
                    av_log(s->avctx, AV_LOG_ERROR, "2. marker bit missing in rvlc esc\n");
                    return -1;
                }; SKIP_CACHE(re, &s->gb, 1);

                level= SHOW_UBITS(re, &s->gb, 11); SKIP_CACHE(re, &s->gb, 11);

                if(SHOW_UBITS(re, &s->gb, 5)!=0x10){
                    av_log(s->avctx, AV_LOG_ERROR, "reverse esc missing\n");
                    return -1;
                }; SKIP_CACHE(re, &s->gb, 5);

                level=  level * qmul + qadd;
                level = (level ^ SHOW_SBITS(re, &s->gb, 1)) - SHOW_SBITS(re, &s->gb, 1); LAST_SKIP_CACHE(re, &s->gb, 1);
                SKIP_COUNTER(re, &s->gb, 1+11+5+1);

                i+= run + 1;
                if(last) i+=192;
          }else{
            int cache;
            cache= GET_CACHE(re, &s->gb);

            if(IS_3IV1)
                cache ^= 0xC0000000;

            if (cache&0x80000000) {
                if (cache&0x40000000) {
                    /* third escape */
                    SKIP_CACHE(re, &s->gb, 2);
                    last=  SHOW_UBITS(re, &s->gb, 1); SKIP_CACHE(re, &s->gb, 1);
                    run=   SHOW_UBITS(re, &s->gb, 6); LAST_SKIP_CACHE(re, &s->gb, 6);
                    SKIP_COUNTER(re, &s->gb, 2+1+6);
                    UPDATE_CACHE(re, &s->gb);

                    if(IS_3IV1){
                        level= SHOW_SBITS(re, &s->gb, 12); LAST_SKIP_BITS(re, &s->gb, 12);
                    }else{
                        if(SHOW_UBITS(re, &s->gb, 1)==0){
                            av_log(s->avctx, AV_LOG_ERROR, "1. marker bit missing in 3. esc\n");
                            return -1;
                        }; SKIP_CACHE(re, &s->gb, 1);

                        level= SHOW_SBITS(re, &s->gb, 12); SKIP_CACHE(re, &s->gb, 12);

                        if(SHOW_UBITS(re, &s->gb, 1)==0){
                            av_log(s->avctx, AV_LOG_ERROR, "2. marker bit missing in 3. esc\n");
                            return -1;
                        }; LAST_SKIP_CACHE(re, &s->gb, 1);

                        SKIP_COUNTER(re, &s->gb, 1+12+1);
                    }

#if 0
                    if(s->error_recognition >= FF_ER_COMPLIANT){
                        const int abs_level= FFABS(level);
                        if(abs_level<=MAX_LEVEL && run<=MAX_RUN){
                            const int run1= run - rl->max_run[last][abs_level] - 1;
                            if(abs_level <= rl->max_level[last][run]){
                                av_log(s->avctx, AV_LOG_ERROR, "illegal 3. esc, vlc encoding possible\n");
                                return -1;
                            }
                            if(s->error_recognition > FF_ER_COMPLIANT){
                                if(abs_level <= rl->max_level[last][run]*2){
                                    av_log(s->avctx, AV_LOG_ERROR, "illegal 3. esc, esc 1 encoding possible\n");
                                    return -1;
                                }
                                if(run1 >= 0 && abs_level <= rl->max_level[last][run1]){
                                    av_log(s->avctx, AV_LOG_ERROR, "illegal 3. esc, esc 2 encoding possible\n");
                                    return -1;
                                }
                            }
                        }
                    }
#endif
                    if (level>0) level= level * qmul + qadd;
                    else         level= level * qmul - qadd;

                    if((unsigned)(level + 2048) > 4095){
                        if(s->error_recognition > FF_ER_COMPLIANT){
                            if(level > 2560 || level<-2560){
                                av_log(s->avctx, AV_LOG_ERROR, "|level| overflow in 3. esc, qp=%d\n", s->qscale);
                                return -1;
                            }
                        }
                        level= level<0 ? -2048 : 2047;
                    }

                    i+= run + 1;
                    if(last) i+=192;
                } else {
                    /* second escape */
#if MIN_CACHE_BITS < 20
                    LAST_SKIP_BITS(re, &s->gb, 2);
                    UPDATE_CACHE(re, &s->gb);
#else
                    SKIP_BITS(re, &s->gb, 2);
#endif
                    GET_RL_VLC(level, run, re, &s->gb, rl_vlc, TEX_VLC_BITS, 2, 1);
                    i+= run + rl->max_run[run>>7][level/qmul] +1; //FIXME opt indexing
                    level = (level ^ SHOW_SBITS(re, &s->gb, 1)) - SHOW_SBITS(re, &s->gb, 1);
                    LAST_SKIP_BITS(re, &s->gb, 1);
                }
            } else {
                /* first escape */
#if MIN_CACHE_BITS < 19
                LAST_SKIP_BITS(re, &s->gb, 1);
                UPDATE_CACHE(re, &s->gb);
#else
                SKIP_BITS(re, &s->gb, 1);
#endif
                GET_RL_VLC(level, run, re, &s->gb, rl_vlc, TEX_VLC_BITS, 2, 1);
                i+= run;
                level = level + rl->max_level[run>>7][(run-1)&63] * qmul;//FIXME opt indexing
                level = (level ^ SHOW_SBITS(re, &s->gb, 1)) - SHOW_SBITS(re, &s->gb, 1);
                LAST_SKIP_BITS(re, &s->gb, 1);
            }
          }
        } else {
            i+= run;
            level = (level ^ SHOW_SBITS(re, &s->gb, 1)) - SHOW_SBITS(re, &s->gb, 1);
            LAST_SKIP_BITS(re, &s->gb, 1);
        }
        if (i > 62){
            i-= 192;
            if(i&(~63)){
                av_log(s->avctx, AV_LOG_ERROR, "ac-tex damaged at %d %d\n", s->mb_x, s->mb_y);
                return -1;
            }

            block[scan_table[i]] = level;
            break;
        }

        block[scan_table[i]] = level;
    }
    CLOSE_READER(re, &s->gb);
  }
 not_coded:
    if (intra) {
        if(!s->use_intra_dc_vlc){
            block[0] = mpeg4_pred_dc(s, n, block[0], &dc_pred_dir, 0);

            i -= i>>31; //if(i == -1) i=0;
        }

        JZC_mpeg4_pred_ac(s, block, n, dc_pred_dir);
        if (s->ac_pred) {
            i = 63; /* XXX: not optimal */
        }
    }
    s->block_last_index[n] = i;
    return 0;
}

/**
 * decode partition C of one MB.
 * @return <0 if an error occurred
 */
static int mpeg4_decode_partitioned_mb(MpegEncContext *s, DCTELEM block[6][64])
{
  int cbp, mb_type;
  const int xy= s->mb_x + s->mb_y*s->mb_stride;

  mb_type= s->current_picture.mb_type[xy];
  cbp = s->cbp_table[xy];

  s->use_intra_dc_vlc= s->qscale < s->intra_dc_threshold;

  if(s->current_picture.qscale_table[xy] != s->qscale){
    ff_set_qscale(s, s->current_picture.qscale_table[xy] );
  }

  if (s->pict_type == FF_P_TYPE || s->pict_type==FF_S_TYPE) {
    int i;
    for(i=0; i<4; i++){
      s->mv[0][i][0] = s->current_picture.motion_val[0][ s->block_index[i] ][0];
      s->mv[0][i][1] = s->current_picture.motion_val[0][ s->block_index[i] ][1];
    }
    s->mb_intra = IS_INTRA(mb_type);

    if (IS_SKIP(mb_type)) {
      /* skip mb */
      for(i=0;i<6;i++)
	s->block_last_index[i] = -1;
      s->mv_dir = MV_DIR_FORWARD;
      s->mv_type = MV_TYPE_16X16;
      if(s->pict_type==FF_S_TYPE && s->vol_sprite_usage==GMC_SPRITE){
	s->mcsel=1;
	s->mb_skipped = 0;
      }else{
	s->mcsel=0;
	s->mb_skipped = 1;
      }
    }else if(s->mb_intra){
      s->ac_pred = IS_ACPRED(s->current_picture.mb_type[xy]);
    }else if(!s->mb_intra){
      //            s->mcsel= 0; //FIXME do we need to init that

      s->mv_dir = MV_DIR_FORWARD;
      if (IS_8X8(mb_type)) {
	s->mv_type = MV_TYPE_8X8;
      } else {
	s->mv_type = MV_TYPE_16X16;
      }
    }
  } else { /* I-Frame */
    s->mb_intra = 1;
    s->ac_pred = IS_ACPRED(s->current_picture.mb_type[xy]);
  }

  if (!IS_SKIP(mb_type)) {
    int i;
    s->dsp.clear_blocks(s->block[0]);
    /* decode each block */
    for (i = 0; i < 6; i++) {
      if(mpeg4_decode_block(s, block[i], i, cbp&32, s->mb_intra, s->rvlc) < 0){
	av_log(s->avctx, AV_LOG_ERROR, "texture corrupted at %d %d %d\n", s->mb_x, s->mb_y, s->mb_intra);
	return -1;
      }
      cbp+=cbp;
    }
  }

  /* per-MB end of slice check */

  if(--s->mb_num_left <= 0){
    if(mpeg4_is_resync(s))
      return SLICE_END;
    else
      return SLICE_NOEND;
  }else{
    if(mpeg4_is_resync(s)){
      const int delta= s->mb_x + 1 == s->mb_width ? 2 : 1;
      if(s->cbp_table[xy+delta])
	return SLICE_END;
    }
    return SLICE_OK;
  }
}

#define tab_size ((signed)FF_ARRAY_ELEMS(s->direct_scale_mv[0]))
#define tab_bias (tab_size/2)
static inline void JZC_mpeg4_set_one_direct_mv(MpegEncContext *s, int mx, int my, int i){
  int xy= s->block_index[i];
  uint16_t time_pp= s->pp_time;
  uint16_t time_pb= s->pb_time;
  int p_mx, p_my;

  p_mx= s->next_picture.motion_val[0][xy][0];
  if((unsigned)(p_mx + tab_bias) < tab_size){
    s->mv[0][i][0] = s->direct_scale_mv[0][p_mx + tab_bias] + mx;
    s->mv[1][i][0] = mx ? s->mv[0][i][0] - p_mx
      : s->direct_scale_mv[1][p_mx + tab_bias];
  }else{
    s->mv[0][i][0] = p_mx*time_pb/time_pp + mx;
    s->mv[1][i][0] = mx ? s->mv[0][i][0] - p_mx
      : p_mx*(time_pb - time_pp)/time_pp;
  }
  dMB->mv[i*2] = s->mv[0][i][0];
  dMB->mv[8+i*2]=s->mv[1][i][0];

  p_my= s->next_picture.motion_val[0][xy][1];
  if((unsigned)(p_my + tab_bias) < tab_size){
    s->mv[0][i][1] = s->direct_scale_mv[0][p_my + tab_bias] + my;
    s->mv[1][i][1] = my ? s->mv[0][i][1] - p_my
      : s->direct_scale_mv[1][p_my + tab_bias];
  }else{
    s->mv[0][i][1] = p_my*time_pb/time_pp + my;
    s->mv[1][i][1] = my ? s->mv[0][i][1] - p_my
      : p_my*(time_pb - time_pp)/time_pp;
  }
  dMB->mv[i*2+1] = s->mv[0][i][1];
  dMB->mv[8+i*2+1]=s->mv[1][i][1];
}
#undef tab_size
#undef tab_bias

static inline int JZC_mpeg4_set_direct_mv(MpegEncContext *s, int mx, int my){
  const int mb_index= s->mb_x + s->mb_y*s->mb_stride;
  const int colocated_mb_type= s->next_picture.mb_type[mb_index];
  uint16_t time_pp;
  uint16_t time_pb;
  int i;

  if(IS_8X8(colocated_mb_type)){
    s->mv_type = MV_TYPE_8X8;
    for(i=0; i<4; i++){
      JZC_mpeg4_set_one_direct_mv(s, mx, my, i);
    }
    return MB_TYPE_DIRECT2 | MB_TYPE_8x8 | MB_TYPE_L0L1;
  }else{
    JZC_mpeg4_set_one_direct_mv(s, mx, my, 0);
    s->mv[0][1][0] = s->mv[0][2][0] = s->mv[0][3][0] = s->mv[0][0][0];
    s->mv[0][1][1] = s->mv[0][2][1] = s->mv[0][3][1] = s->mv[0][0][1];
    s->mv[1][1][0] = s->mv[1][2][0] = s->mv[1][3][0] = s->mv[1][0][0];
    s->mv[1][1][1] = s->mv[1][2][1] = s->mv[1][3][1] = s->mv[1][0][1];
    memcpy(dMB->mv, s->mv, sizeof(int)*16);
    if((s->avctx->workaround_bugs & FF_BUG_DIRECT_BLOCKSIZE) || !s->quarter_sample)
      s->mv_type= MV_TYPE_16X16;
    else
      s->mv_type= MV_TYPE_8X8;
    return MB_TYPE_DIRECT2 | MB_TYPE_16x16 | MB_TYPE_L0L1; //Note see prev line
  }
}

static inline void clear_blocks_c_mxu(DCTELEM *blocks,DCTELEM n)
{
  uint32_t i;
  uint32_t block = (uint32_t)blocks;
  for(i=0;i<4*n;i++){
    S32STD(xr0, block, 0);
    S32STD(xr0, block, 4);
    S32STD(xr0, block, 8);

    S32STD(xr0, block, 12);
    S32STD(xr0, block, 16);
    S32STD(xr0, block, 20);
    S32STD(xr0, block, 24);
    S32STD(xr0, block, 28);
#if 0
    S32STD(xr0, block, 32);
    S32STD(xr0, block, 36);
    S32STD(xr0, block, 40);

    S32STD(xr0, block, 44);
    S32STD(xr0, block, 48);
    S32STD(xr0, block, 52);
    S32STD(xr0, block, 56);
    S32STD(xr0, block, 60);
#endif
    block += 32;
  }
}

static int mpeg4_decode_mb(MpegEncContext *s,
			   DCTELEM block[6][64])
{
  int cbpc, cbpy, i, cbp, pred_x, pred_y, mx, my, dquant;
  int16_t *mot_val;
  static int8_t quant_tab[4] = { -1, -2, 1, 2 };
  const int xy= s->mb_x + s->mb_y * s->mb_stride;

  assert(s->h263_pred);

  if (s->pict_type == FF_P_TYPE || s->pict_type==FF_S_TYPE) {
    do{
      if (get_bits1(&s->gb)) {
	/* skip mb */
	s->mb_intra = 0;
	for(i=0;i<6;i++)
	  s->block_last_index[i] = -1;
	s->mv_dir = MV_DIR_FORWARD;
	s->mv_type = MV_TYPE_16X16;
	if(s->pict_type==FF_S_TYPE && s->vol_sprite_usage==GMC_SPRITE){
	  s->current_picture.mb_type[xy]= MB_TYPE_SKIP | MB_TYPE_GMC | MB_TYPE_16x16 | MB_TYPE_L0;
	  s->mcsel=1;
	  s->mv[0][0][0]= dMB->mv[0] = get_amv(s, 0);
	  s->mv[0][0][1]= dMB->mv[1] = get_amv(s, 1);

	  s->mb_skipped = 0;
	}else{
	  s->current_picture.mb_type[xy]= MB_TYPE_SKIP | MB_TYPE_16x16 | MB_TYPE_L0;
	  s->mcsel=0;
	  s->mv[0][0][0] = dMB->mv[0] = 0;
	  s->mv[0][0][1] = dMB->mv[1] = 0;
	  s->mb_skipped = 1;
	}
	dMB->code_cbp = 0;
	goto end;
      }
      cbpc = get_vlc2(&s->gb, ff_h263_inter_MCBPC_vlc_hw.table, INTER_MCBPC_VLC_BITS, 2);
      if (cbpc < 0){
	av_log(s->avctx, AV_LOG_ERROR, "cbpc damaged at %d %d\n", s->mb_x, s->mb_y);
	return -1;
      }
    }while(cbpc == 20);

    //s->dsp.clear_blocks(s->block[0]);
    clear_blocks_c_mxu(block[0],6);
    dquant = cbpc & 8;
    s->mb_intra = ((cbpc & 4) != 0);
    if (s->mb_intra) goto intra;

    if(s->pict_type==FF_S_TYPE && s->vol_sprite_usage==GMC_SPRITE && (cbpc & 16) == 0)
      s->mcsel= get_bits1(&s->gb);
    else s->mcsel= 0;
    cbpy = get_vlc2(&s->gb, ff_h263_cbpy_vlc_hw.table, CBPY_VLC_BITS, 1) ^ 0x0F;

    cbp = (cbpc & 3) | (cbpy << 2);
    if (dquant) {
      ff_set_qscale(s, s->qscale + quant_tab[get_bits(&s->gb, 2)]);
    }
    if((!s->progressive_sequence) && (cbp || (s->workaround_bugs&FF_BUG_XVID_ILACE)))
      s->interlaced_dct= get_bits1(&s->gb);

    s->mv_dir = MV_DIR_FORWARD;
    if ((cbpc & 16) == 0) {
      if(s->mcsel){
	s->current_picture.mb_type[xy]= MB_TYPE_GMC | MB_TYPE_16x16 | MB_TYPE_L0;
	/* 16x16 global motion prediction */
	s->mv_type = MV_TYPE_16X16;
	mx= get_amv(s, 0);
	my= get_amv(s, 1);
	s->mv[0][0][0] = dMB->mv[0] = mx;
	s->mv[0][0][1] = dMB->mv[1] = my;
      }else if((!s->progressive_sequence) && get_bits1(&s->gb)){
	s->current_picture.mb_type[xy]= MB_TYPE_16x8 | MB_TYPE_L0 | MB_TYPE_INTERLACED;
	/* 16x8 field motion prediction */
	s->mv_type= MV_TYPE_FIELD;

	s->field_select[0][0]= get_bits1(&s->gb);
	s->field_select[0][1]= get_bits1(&s->gb);

	mpeg4_pred_motion(s, 0, 0, &pred_x, &pred_y);

	for(i=0; i<2; i++){
	  mx = mpeg4_decode_motion(s, pred_x, s->f_code);
	  if (mx >= 0xffff)
	    return -1;

	  my = mpeg4_decode_motion(s, pred_y/2, s->f_code);
	  if (my >= 0xffff)
	    return -1;

	  s->mv[0][i][0] = dMB->mv[i*2] = mx;
	  s->mv[0][i][1] = dMB->mv[i*2+1] = my;
	}
      }else{
	s->current_picture.mb_type[xy]= MB_TYPE_16x16 | MB_TYPE_L0;
	/* 16x16 motion prediction */
	s->mv_type = MV_TYPE_16X16;
	mpeg4_pred_motion(s, 0, 0, &pred_x, &pred_y);
	mx = mpeg4_decode_motion(s, pred_x, s->f_code);

	if (mx >= 0xffff)
	  return -1;

	my = mpeg4_decode_motion(s, pred_y, s->f_code);

	if (my >= 0xffff)
	  return -1;
	s->mv[0][0][0] = dMB->mv[0] = mx;
	s->mv[0][0][1] = dMB->mv[1] = my;
      }
    } else {
      s->current_picture.mb_type[xy]= MB_TYPE_8x8 | MB_TYPE_L0;
      s->mv_type = MV_TYPE_8X8;
      for(i=0;i<4;i++) {
	mot_val = mpeg4_pred_motion(s, i, 0, &pred_x, &pred_y);
	mx = mpeg4_decode_motion(s, pred_x, s->f_code);
	if (mx >= 0xffff)
	  return -1;

	my = mpeg4_decode_motion(s, pred_y, s->f_code);
	if (my >= 0xffff)
	  return -1;
	s->mv[0][i][0] = dMB->mv[i*2] = mx;
	s->mv[0][i][1] = dMB->mv[i*2+1] = my;
	mot_val[0] = mx;
	mot_val[1] = my;
      }
    }
  } else if(s->pict_type==FF_B_TYPE) {
    int modb1; // first bit of modb
    int modb2; // second bit of modb
    int mb_type;

    s->mb_intra = 0; //B-frames never contain intra blocks
    s->mcsel=0;      //     ...               true gmc blocks

    if(s->mb_x==0){
      for(i=0; i<2; i++){
	s->last_mv[i][0][0]=
	  s->last_mv[i][0][1]=
	  s->last_mv[i][1][0]=
	  s->last_mv[i][1][1]= 0;
      }
    }

    /* if we skipped it in the future P Frame than skip it now too */
    s->mb_skipped= s->next_picture.mbskip_table[s->mb_y * s->mb_stride + s->mb_x]; // Note, skiptab=0 if last was GMC

    if(s->mb_skipped){
      /* skip mb */
      for(i=0;i<6;i++)
	s->block_last_index[i] = -1;

      s->mv_dir = MV_DIR_FORWARD;
      s->mv_type = MV_TYPE_16X16;
      s->mv[0][0][0] = dMB->mv[0] = 0;
      s->mv[0][0][1] = dMB->mv[1] = 0;
      s->mv[1][0][0] = dMB->mv[8] = 0;
      s->mv[1][0][1] = dMB->mv[9] = 0;
      s->current_picture.mb_type[xy]= MB_TYPE_SKIP | MB_TYPE_16x16 | MB_TYPE_L0;
      dMB->code_cbp = 0;
      goto end;
    }

    modb1= get_bits1(&s->gb);
    if(modb1){
      mb_type= MB_TYPE_DIRECT2 | MB_TYPE_SKIP | MB_TYPE_L0L1; //like MB_TYPE_B_DIRECT but no vectors coded
      cbp=0;
    }else{
      modb2= get_bits1(&s->gb);
      mb_type= get_vlc2(&s->gb, mb_type_b_vlc.table, MB_TYPE_B_VLC_BITS, 1);
      if(mb_type<0){
	av_log(s->avctx, AV_LOG_ERROR, "illegal MB_type\n");
	return -1;
      }
      mb_type= mb_type_b_map[ mb_type ];
      if(modb2) cbp= 0;
      else{
	//s->dsp.clear_blocks(s->block[0]);
	clear_blocks_c_mxu(block[0],6);
	cbp= get_bits(&s->gb, 6);
      }

      if ((!IS_DIRECT(mb_type)) && cbp) {
	if(get_bits1(&s->gb)){
	  ff_set_qscale(s, s->qscale + get_bits1(&s->gb)*4 - 2);
	}
      }

      if(!s->progressive_sequence){
	if(cbp)
	  s->interlaced_dct= get_bits1(&s->gb);

	if(!IS_DIRECT(mb_type) && get_bits1(&s->gb)){
	  mb_type |= MB_TYPE_16x8 | MB_TYPE_INTERLACED;
	  mb_type &= ~MB_TYPE_16x16;

	  if(USES_LIST(mb_type, 0)){
	    s->field_select[0][0]= get_bits1(&s->gb);
	    s->field_select[0][1]= get_bits1(&s->gb);
	  }
	  if(USES_LIST(mb_type, 1)){
	    s->field_select[1][0]= get_bits1(&s->gb);
	    s->field_select[1][1]= get_bits1(&s->gb);
	  }
	}
      }

      s->mv_dir = 0;
      if((mb_type & (MB_TYPE_DIRECT2|MB_TYPE_INTERLACED)) == 0){
	s->mv_type= MV_TYPE_16X16;

	if(USES_LIST(mb_type, 0)){
	  s->mv_dir = MV_DIR_FORWARD;

	  mx = mpeg4_decode_motion(s, s->last_mv[0][0][0], s->f_code);
	  my = mpeg4_decode_motion(s, s->last_mv[0][0][1], s->f_code);
	  s->last_mv[0][1][0]= s->last_mv[0][0][0]= s->mv[0][0][0] = dMB->mv[0] = mx;
	  s->last_mv[0][1][1]= s->last_mv[0][0][1]= s->mv[0][0][1] = dMB->mv[1] = my;
	}

	if(USES_LIST(mb_type, 1)){
	  s->mv_dir |= MV_DIR_BACKWARD;

	  mx = mpeg4_decode_motion(s, s->last_mv[1][0][0], s->b_code);
	  my = mpeg4_decode_motion(s, s->last_mv[1][0][1], s->b_code);
	  s->last_mv[1][1][0]= s->last_mv[1][0][0]= s->mv[1][0][0] = dMB->mv[8] = mx;
	  s->last_mv[1][1][1]= s->last_mv[1][0][1]= s->mv[1][0][1] = dMB->mv[9] = my;
	}
      }else if(!IS_DIRECT(mb_type)){
	s->mv_type= MV_TYPE_FIELD;

	if(USES_LIST(mb_type, 0)){
	  s->mv_dir = MV_DIR_FORWARD;

	  for(i=0; i<2; i++){
	    mx = mpeg4_decode_motion(s, s->last_mv[0][i][0]  , s->f_code);
	    my = mpeg4_decode_motion(s, s->last_mv[0][i][1]/2, s->f_code);
	    s->last_mv[0][i][0]=  s->mv[0][i][0] = dMB->mv[i*2] = mx;
	    s->last_mv[0][i][1]= (s->mv[0][i][1] = dMB->mv[i*2+1] = my)*2;
	  }
	}

	if(USES_LIST(mb_type, 1)){
	  s->mv_dir |= MV_DIR_BACKWARD;

	  for(i=0; i<2; i++){
	    mx = mpeg4_decode_motion(s, s->last_mv[1][i][0]  , s->b_code);
	    my = mpeg4_decode_motion(s, s->last_mv[1][i][1]/2, s->b_code);
	    s->last_mv[1][i][0]=  s->mv[1][i][0] = dMB->mv[8+i*2] = mx;
	    s->last_mv[1][i][1]= (s->mv[1][i][1] = dMB->mv[8+i*2+1] = my)*2;
	  }
	}
      }
    }

    if(IS_DIRECT(mb_type)){
      if(IS_SKIP(mb_type))
	mx=my=0;
      else{
	mx = mpeg4_decode_motion(s, 0, 1);
	my = mpeg4_decode_motion(s, 0, 1);
      }

      s->mv_dir = MV_DIR_FORWARD | MV_DIR_BACKWARD | MV_DIRECT;
      mb_type |= JZC_mpeg4_set_direct_mv(s, mx, my);
    }
    s->current_picture.mb_type[xy]= mb_type;
  } else { /* I-Frame */
    do{
      cbpc = get_vlc2(&s->gb, ff_h263_intra_MCBPC_vlc_hw.table, INTRA_MCBPC_VLC_BITS, 2);
      if (cbpc < 0){
	av_log(s->avctx, AV_LOG_ERROR, "I cbpc damaged at %d %d\n", s->mb_x, s->mb_y);
	return -1;
      }
    }while(cbpc == 8);

    dquant = cbpc & 4;
    s->mb_intra = 1;
  intra:
    s->ac_pred = get_bits1(&s->gb);
    if(s->ac_pred)
      s->current_picture.mb_type[xy]= MB_TYPE_INTRA | MB_TYPE_ACPRED;
    else
      s->current_picture.mb_type[xy]= MB_TYPE_INTRA;

    cbpy = get_vlc2(&s->gb, ff_h263_cbpy_vlc_hw.table, CBPY_VLC_BITS, 1);
    if(cbpy<0){
      av_log(s->avctx, AV_LOG_ERROR, "I cbpy damaged at %d %d\n", s->mb_x, s->mb_y);
      return -1;
    }
    cbp = (cbpc & 3) | (cbpy << 2);

    s->use_intra_dc_vlc= s->qscale < s->intra_dc_threshold;

    if (dquant) {
      ff_set_qscale(s, s->qscale + quant_tab[get_bits(&s->gb, 2)]);
    }

    if(!s->progressive_sequence)
      s->interlaced_dct= get_bits1(&s->gb);

    //s->dsp.clear_blocks(s->block[0]);
    clear_blocks_c_mxu(block[0],6);
    /* decode each block */
    for (i = 0; i < 6; i++) {
      if (mpeg4_decode_block(s, block[i], i, cbp&32, 1, 0) < 0)
	return -1;
      cbp+=cbp;
    }
    goto end;
  }

  dMB->code_cbp = ((cbp&1)<<20)|((cbp&2)<<15)|((cbp&4)<<10)|((cbp&8)<<5)|(cbp&16)|((cbp&32)>>5);
  /* decode each block */
  for (i = 0; i < 6; i++) {
    if (mpeg4_decode_block(s, block[i], i, cbp&32, 0, 0) < 0)
      return -1;
    cbp+=cbp;
  }
 end:

  /* per-MB end of slice check */
  if(s->codec_id==CODEC_ID_MPEG4){
    if(mpeg4_is_resync(s)){
      const int delta= s->mb_x + 1 == s->mb_width ? 2 : 1;
      if(s->pict_type==FF_B_TYPE && s->next_picture.mbskip_table[xy + delta])
	return SLICE_OK;
      return SLICE_END;
    }
  }

  return SLICE_OK;
}

static int mpeg4_decode_gop_header(MpegEncContext * s, GetBitContext *gb){
  int hours, minutes, seconds;

  hours= get_bits(gb, 5);
  minutes= get_bits(gb, 6);
  skip_bits1(gb);
  seconds= get_bits(gb, 6);

  s->time_base= seconds + 60*(minutes + 60*hours);

  skip_bits1(gb);
  skip_bits1(gb);

  return 0;
}

static int decode_vol_header(MpegEncContext *s, GetBitContext *gb){
  int width, height, vo_ver_id;

  /* vol header */
  skip_bits(gb, 1); /* random access */
  s->vo_type= get_bits(gb, 8);
  if (get_bits1(gb) != 0) { /* is_ol_id */
    vo_ver_id = get_bits(gb, 4); /* vo_ver_id */
    skip_bits(gb, 3); /* vo_priority */
  } else {
    vo_ver_id = 1;
  }
  s->aspect_ratio_info= get_bits(gb, 4);
  if(s->aspect_ratio_info == FF_ASPECT_EXTENDED){
    s->avctx->sample_aspect_ratio.num= get_bits(gb, 8); // par_width
    s->avctx->sample_aspect_ratio.den= get_bits(gb, 8); // par_height
  }else{
    s->avctx->sample_aspect_ratio= ff_h263_pixel_aspect[s->aspect_ratio_info];
  }

  if ((s->vol_control_parameters=get_bits1(gb))) { /* vol control parameter */
    int chroma_format= get_bits(gb, 2);
    if(chroma_format!=CHROMA_420){
      av_log(s->avctx, AV_LOG_ERROR, "illegal chroma format\n");
    }
    s->low_delay= get_bits1(gb);
    if(get_bits1(gb)){ /* vbv parameters */
      get_bits(gb, 15);   /* first_half_bitrate */
      skip_bits1(gb);     /* marker */
      get_bits(gb, 15);   /* latter_half_bitrate */
      skip_bits1(gb);     /* marker */
      get_bits(gb, 15);   /* first_half_vbv_buffer_size */
      skip_bits1(gb);     /* marker */
      get_bits(gb, 3);    /* latter_half_vbv_buffer_size */
      get_bits(gb, 11);   /* first_half_vbv_occupancy */
      skip_bits1(gb);     /* marker */
      get_bits(gb, 15);   /* latter_half_vbv_occupancy */
      skip_bits1(gb);     /* marker */
    }
  }else{
    // set low delay flag only once the smartest? low delay detection won't be overriden
    if(s->picture_number==0)
      s->low_delay=0;
  }

  s->shape = get_bits(gb, 2); /* vol shape */
  if(s->shape != RECT_SHAPE) av_log(s->avctx, AV_LOG_ERROR, "only rectangular vol supported\n");
  if(s->shape == GRAY_SHAPE && vo_ver_id != 1){
    av_log(s->avctx, AV_LOG_ERROR, "Gray shape not supported\n");
    skip_bits(gb, 4);  //video_object_layer_shape_extension
  }

  check_marker(gb, "before time_increment_resolution");

  s->avctx->time_base.den = get_bits(gb, 16);
  if(!s->avctx->time_base.den){
    av_log(s->avctx, AV_LOG_ERROR, "time_base.den==0\n");
    return -1;
  }

  s->time_increment_bits = av_log2(s->avctx->time_base.den - 1) + 1;
  if (s->time_increment_bits < 1)
    s->time_increment_bits = 1;

  check_marker(gb, "before fixed_vop_rate");

  if (get_bits1(gb) != 0) {   /* fixed_vop_rate  */
    s->avctx->time_base.num = get_bits(gb, s->time_increment_bits);
  }else
    s->avctx->time_base.num = 1;

  s->t_frame=0;

  if (s->shape != BIN_ONLY_SHAPE) {
    if (s->shape == RECT_SHAPE) {
      skip_bits1(gb);   /* marker */
      width = get_bits(gb, 13);
      skip_bits1(gb);   /* marker */
      height = get_bits(gb, 13);
      skip_bits1(gb);   /* marker */
      if(width && height && !(s->width && s->codec_tag == AV_RL32("MP4S"))){ /* they should be non zero but who knows ... */
	s->width = width;
	s->height = height;
      }
    }

    s->progressive_sequence=
      s->progressive_frame= get_bits1(gb)^1;
    s->interlaced_dct=0;
    if(!get_bits1(gb) && (s->avctx->debug & FF_DEBUG_PICT_INFO))
      av_log(s->avctx, AV_LOG_INFO, "MPEG4 OBMC not supported (very likely buggy encoder)\n");   /* OBMC Disable */
    if (vo_ver_id == 1) {
      s->vol_sprite_usage = get_bits1(gb); /* vol_sprite_usage */
    } else {
      s->vol_sprite_usage = get_bits(gb, 2); /* vol_sprite_usage */
    }
    if(s->vol_sprite_usage==STATIC_SPRITE) av_log(s->avctx, AV_LOG_ERROR, "Static Sprites not supported\n");
    if(s->vol_sprite_usage==STATIC_SPRITE || s->vol_sprite_usage==GMC_SPRITE){
      if(s->vol_sprite_usage==STATIC_SPRITE){
	s->sprite_width = get_bits(gb, 13);
	skip_bits1(gb); /* marker */
	s->sprite_height= get_bits(gb, 13);
	skip_bits1(gb); /* marker */
	s->sprite_left  = get_bits(gb, 13);
	skip_bits1(gb); /* marker */
	s->sprite_top   = get_bits(gb, 13);
	skip_bits1(gb); /* marker */
      }
      s->num_sprite_warping_points= get_bits(gb, 6);
      if(s->num_sprite_warping_points > 3){
	av_log(s->avctx, AV_LOG_ERROR, "%d sprite_warping_points\n", s->num_sprite_warping_points);
	s->num_sprite_warping_points= 0;
	return -1;
      }
      s->sprite_warping_accuracy = get_bits(gb, 2);
      s->sprite_brightness_change= get_bits1(gb);
      if(s->vol_sprite_usage==STATIC_SPRITE)
	s->low_latency_sprite= get_bits1(gb);
    }
    // FIXME sadct disable bit if verid!=1 && shape not rect

    if (get_bits1(gb) == 1) {   /* not_8_bit */
      s->quant_precision = get_bits(gb, 4); /* quant_precision */
      if(get_bits(gb, 4)!=8) av_log(s->avctx, AV_LOG_ERROR, "N-bit not supported\n"); /* bits_per_pixel */
      if(s->quant_precision!=5) av_log(s->avctx, AV_LOG_ERROR, "quant precision %d\n", s->quant_precision);
    } else {
      s->quant_precision = 5;
    }

    // FIXME a bunch of grayscale shape things

    if((s->mpeg_quant=get_bits1(gb))){ /* vol_quant_type */
      int i, v;

      /* load default matrixes */
      for(i=0; i<64; i++){
	int j= s->dsp.idct_permutation[i];
	v= ff_mpeg4_default_intra_matrix[i];
	s->intra_matrix[j]= v;
	s->chroma_intra_matrix[j]= v;

	v= ff_mpeg4_default_non_intra_matrix[i];
	s->inter_matrix[j]= v;
	s->chroma_inter_matrix[j]= v;
      }

      /* load custom intra matrix */
      if(get_bits1(gb)){
	int last=0;
	for(i=0; i<64; i++){
	  int j;
	  v= get_bits(gb, 8);
	  if(v==0) break;

	  last= v;
	  j= s->dsp.idct_permutation[ ff_zigzag_direct[i] ];
	  s->intra_matrix[j]= v;
	  s->chroma_intra_matrix[j]= v;
	}

	/* replicate last value */
	for(; i<64; i++){
	  int j= s->dsp.idct_permutation[ ff_zigzag_direct[i] ];
	  s->intra_matrix[j]= last;
	  s->chroma_intra_matrix[j]= last;
	}
      }

      /* load custom non intra matrix */
      if(get_bits1(gb)){
	int last=0;
	for(i=0; i<64; i++){
	  int j;
	  v= get_bits(gb, 8);
	  if(v==0) break;

	  last= v;
	  j= s->dsp.idct_permutation[ ff_zigzag_direct[i] ];
	  s->inter_matrix[j]= v;
	  s->chroma_inter_matrix[j]= v;
	}

	/* replicate last value */
	for(; i<64; i++){
	  int j= s->dsp.idct_permutation[ ff_zigzag_direct[i] ];
	  s->inter_matrix[j]= last;
	  s->chroma_inter_matrix[j]= last;
	}
      }

      // FIXME a bunch of grayscale shape things
    }

    if(vo_ver_id != 1)
      s->quarter_sample= get_bits1(gb);
    else s->quarter_sample=0;

    if(!get_bits1(gb)){
      int pos= get_bits_count(gb);
      int estimation_method= get_bits(gb, 2);
      if(estimation_method<2){
	if(!get_bits1(gb)){
	  s->cplx_estimation_trash_i += 8*get_bits1(gb); //opaque
	  s->cplx_estimation_trash_i += 8*get_bits1(gb); //transparent
	  s->cplx_estimation_trash_i += 8*get_bits1(gb); //intra_cae
	  s->cplx_estimation_trash_i += 8*get_bits1(gb); //inter_cae
	  s->cplx_estimation_trash_i += 8*get_bits1(gb); //no_update
	  s->cplx_estimation_trash_i += 8*get_bits1(gb); //upampling
	}
	if(!get_bits1(gb)){
	  s->cplx_estimation_trash_i += 8*get_bits1(gb); //intra_blocks
	  s->cplx_estimation_trash_p += 8*get_bits1(gb); //inter_blocks
	  s->cplx_estimation_trash_p += 8*get_bits1(gb); //inter4v_blocks
	  s->cplx_estimation_trash_i += 8*get_bits1(gb); //not coded blocks
	}
	if(!check_marker(gb, "in complexity estimation part 1")){
	  skip_bits_long(gb, pos - get_bits_count(gb));
	  goto no_cplx_est;
	}
	if(!get_bits1(gb)){
	  s->cplx_estimation_trash_i += 8*get_bits1(gb); //dct_coeffs
	  s->cplx_estimation_trash_i += 8*get_bits1(gb); //dct_lines
	  s->cplx_estimation_trash_i += 8*get_bits1(gb); //vlc_syms
	  s->cplx_estimation_trash_i += 4*get_bits1(gb); //vlc_bits
	}
	if(!get_bits1(gb)){
	  s->cplx_estimation_trash_p += 8*get_bits1(gb); //apm
	  s->cplx_estimation_trash_p += 8*get_bits1(gb); //npm
	  s->cplx_estimation_trash_b += 8*get_bits1(gb); //interpolate_mc_q
	  s->cplx_estimation_trash_p += 8*get_bits1(gb); //forwback_mc_q
	  s->cplx_estimation_trash_p += 8*get_bits1(gb); //halfpel2
	  s->cplx_estimation_trash_p += 8*get_bits1(gb); //halfpel4
	}
	if(!check_marker(gb, "in complexity estimation part 2")){
	  skip_bits_long(gb, pos - get_bits_count(gb));
	  goto no_cplx_est;
	}
	if(estimation_method==1){
	  s->cplx_estimation_trash_i += 8*get_bits1(gb); //sadct
	  s->cplx_estimation_trash_p += 8*get_bits1(gb); //qpel
	}
      }else
	av_log(s->avctx, AV_LOG_ERROR, "Invalid Complexity estimation method %d\n", estimation_method);
    }else{
    no_cplx_est:
      s->cplx_estimation_trash_i=
	s->cplx_estimation_trash_p=
	s->cplx_estimation_trash_b= 0;
    }

    s->resync_marker= !get_bits1(gb); /* resync_marker_disabled */

    s->data_partitioning= get_bits1(gb);
    if(s->data_partitioning){
      s->rvlc= get_bits1(gb);
    }

    if(vo_ver_id != 1) {
      s->new_pred= get_bits1(gb);
      if(s->new_pred){
	av_log(s->avctx, AV_LOG_ERROR, "new pred not supported\n");
	skip_bits(gb, 2); /* requested upstream message type */
	skip_bits1(gb); /* newpred segment type */
      }
      s->reduced_res_vop= get_bits1(gb);
      if(s->reduced_res_vop) av_log(s->avctx, AV_LOG_ERROR, "reduced resolution VOP not supported\n");
    }
    else{
      s->new_pred=0;
      s->reduced_res_vop= 0;
    }

    s->scalability= get_bits1(gb);

    if (s->scalability) {
      GetBitContext bak= *gb;
      int ref_layer_id;
      int ref_layer_sampling_dir;
      int h_sampling_factor_n;
      int h_sampling_factor_m;
      int v_sampling_factor_n;
      int v_sampling_factor_m;

      s->hierachy_type= get_bits1(gb);
      ref_layer_id= get_bits(gb, 4);
      ref_layer_sampling_dir= get_bits1(gb);
      h_sampling_factor_n= get_bits(gb, 5);
      h_sampling_factor_m= get_bits(gb, 5);
      v_sampling_factor_n= get_bits(gb, 5);
      v_sampling_factor_m= get_bits(gb, 5);
      s->enhancement_type= get_bits1(gb);

      if(   h_sampling_factor_n==0 || h_sampling_factor_m==0
	    || v_sampling_factor_n==0 || v_sampling_factor_m==0){
	/* illegal scalability header (VERY broken encoder),
	 * trying to workaround */
	s->scalability=0;
	*gb= bak;
      }else
	av_log(s->avctx, AV_LOG_ERROR, "scalability not supported\n");

      // bin shape stuff FIXME
    }
  }
  return 0;
}

/**
 * decodes the user data stuff in the header.
 * Also initializes divx/xvid/lavc_version/build.
 */
static int decode_user_data(MpegEncContext *s, GetBitContext *gb){
  char buf[256];
  int i;
  int e;
  int ver = 0, build = 0, ver2 = 0, ver3 = 0;
  char last;

  for(i=0; i<255 && get_bits_count(gb) < gb->size_in_bits; i++){
    if(show_bits(gb, 23) == 0) break;
    buf[i]= get_bits(gb, 8);
  }
  buf[i]=0;

  /* divx detection */
  e=sscanf(buf, "DivX%dBuild%d%c", &ver, &build, &last);
  if(e<2)
    e=sscanf(buf, "DivX%db%d%c", &ver, &build, &last);
  if(e>=2){
    s->divx_version= ver;
    s->divx_build= build;
    s->divx_packed= e==3 && last=='p';
    if(s->divx_packed && !s->showed_packed_warning) {
      av_log(s->avctx, AV_LOG_WARNING, "Invalid and inefficient vfw-avi packed B frames detected\n");
      s->showed_packed_warning=1;
    }
  }

  /* ffmpeg detection */
  e=sscanf(buf, "FFmpe%*[^b]b%d", &build)+3;
  if(e!=4)
    e=sscanf(buf, "FFmpeg v%d.%d.%d / libavcodec build: %d", &ver, &ver2, &ver3, &build);
  if(e!=4){
    e=sscanf(buf, "Lavc%d.%d.%d", &ver, &ver2, &ver3)+1;
    if (e>1)
      build= (ver<<16) + (ver2<<8) + ver3;
  }
  if(e!=4){
    if(strcmp(buf, "ffmpeg")==0){
      s->lavc_build= 4600;
    }
  }
  if(e==4){
    s->lavc_build= build;
  }

  /* Xvid detection */
  e=sscanf(buf, "XviD%d", &build);
  if(e==1){
    s->xvid_build= build;
  }

  return 0;
}

static int decode_vop_header(MpegEncContext *s, GetBitContext *gb){
  int time_incr, time_increment;

#ifdef JZC_PMON_P0MB
  PMON_CLEAR(dcp0);
#endif

  s->pict_type = get_bits(gb, 2) + FF_I_TYPE;        /* pict type: I = 0 , P = 1 */
  if(s->pict_type==FF_B_TYPE && s->low_delay && s->vol_control_parameters==0 && !(s->flags & CODEC_FLAG_LOW_DELAY)){
    av_log(s->avctx, AV_LOG_ERROR, "low_delay flag incorrectly, clearing it\n");
    s->low_delay=0;
  }

  s->partitioned_frame= s->data_partitioning && s->pict_type!=FF_B_TYPE;
  if(s->partitioned_frame)
    s->decode_mb= mpeg4_decode_partitioned_mb;
  else
    s->decode_mb= mpeg4_decode_mb;

  time_incr=0;
  while (get_bits1(gb) != 0)
    time_incr++;

  check_marker(gb, "before time_increment");

  if(s->time_increment_bits==0 || !(show_bits(gb, s->time_increment_bits+1)&1)){
    av_log(s->avctx, AV_LOG_ERROR, "hmm, seems the headers are not complete, trying to guess time_increment_bits\n");

    for(s->time_increment_bits=1 ;s->time_increment_bits<16; s->time_increment_bits++){
      if (    s->pict_type == FF_P_TYPE
	      || (s->pict_type == FF_S_TYPE && s->vol_sprite_usage==GMC_SPRITE)) {
	if((show_bits(gb, s->time_increment_bits+6)&0x37) == 0x30) break;
      }else
	if((show_bits(gb, s->time_increment_bits+5)&0x1F) == 0x18) break;
    }

    av_log(s->avctx, AV_LOG_ERROR, "my guess is %d bits ;)\n",s->time_increment_bits);
  }

  if(IS_3IV1) time_increment= get_bits1(gb); //FIXME investigate further
  else time_increment= get_bits(gb, s->time_increment_bits);

  if(s->pict_type!=FF_B_TYPE){
    s->last_time_base= s->time_base;
    s->time_base+= time_incr;
    s->time= s->time_base*s->avctx->time_base.den + time_increment;
    if(s->workaround_bugs&FF_BUG_UMP4){
      if(s->time < s->last_non_b_time){
	/* header is not mpeg-4-compatible, broken encoder,
	 * trying to workaround */
	s->time_base++;
	s->time+= s->avctx->time_base.den;
      }
    }
    s->pp_time= s->time - s->last_non_b_time;
    s->last_non_b_time= s->time;
  }else{
    s->time= (s->last_time_base + time_incr)*s->avctx->time_base.den + time_increment;
    s->pb_time= s->pp_time - (s->last_non_b_time - s->time);
    if(s->pp_time <=s->pb_time || s->pp_time <= s->pp_time - s->pb_time || s->pp_time<=0){
      /* messed up order, maybe after seeking? skipping current b-frame */
      return FRAME_SKIPPED;
    }
    ff_mpeg4_init_direct_mv(s);

    if(s->t_frame==0) s->t_frame= s->pb_time;
    if(s->t_frame==0) s->t_frame=1; // 1/0 protection
    s->pp_field_time= (  ROUNDED_DIV(s->last_non_b_time, s->t_frame)
			 - ROUNDED_DIV(s->last_non_b_time - s->pp_time, s->t_frame))*2;
    s->pb_field_time= (  ROUNDED_DIV(s->time, s->t_frame)
			 - ROUNDED_DIV(s->last_non_b_time - s->pp_time, s->t_frame))*2;
    if(!s->progressive_sequence){
      if(s->pp_field_time <= s->pb_field_time || s->pb_field_time <= 1)
	return FRAME_SKIPPED;
    }
  }

  if(s->avctx->time_base.num)
    s->current_picture_ptr->pts= (s->time + s->avctx->time_base.num/2) / s->avctx->time_base.num;
  else
    s->current_picture_ptr->pts= AV_NOPTS_VALUE;
  if(s->avctx->debug&FF_DEBUG_PTS)
    av_log(s->avctx, AV_LOG_DEBUG, "MPEG4 PTS: %"PRId64"\n", s->current_picture_ptr->pts);

  check_marker(gb, "before vop_coded");

  /* vop coded */
  if (get_bits1(gb) != 1){
    if(s->avctx->debug&FF_DEBUG_PICT_INFO)
      av_log(s->avctx, AV_LOG_ERROR, "vop not coded\n");
    return FRAME_SKIPPED;
  }
  if (s->shape != BIN_ONLY_SHAPE && ( s->pict_type == FF_P_TYPE
				      || (s->pict_type == FF_S_TYPE && s->vol_sprite_usage==GMC_SPRITE))) {
    /* rounding type for motion estimation */
    s->no_rounding = get_bits1(gb);
  } else {
    s->no_rounding = 0;
  }
  //FIXME reduced res stuff

  if (s->shape != RECT_SHAPE) {
    if (s->vol_sprite_usage != 1 || s->pict_type != FF_I_TYPE) {
      int width, height, hor_spat_ref, ver_spat_ref;

      width = get_bits(gb, 13);
      skip_bits1(gb);   /* marker */
      height = get_bits(gb, 13);
      skip_bits1(gb);   /* marker */
      hor_spat_ref = get_bits(gb, 13); /* hor_spat_ref */
      skip_bits1(gb);   /* marker */
      ver_spat_ref = get_bits(gb, 13); /* ver_spat_ref */
    }
    skip_bits1(gb); /* change_CR_disable */

    if (get_bits1(gb) != 0) {
      skip_bits(gb, 8); /* constant_alpha_value */
    }
  }
  //FIXME complexity estimation stuff

  if (s->shape != BIN_ONLY_SHAPE) {
    skip_bits_long(gb, s->cplx_estimation_trash_i);
    if(s->pict_type != FF_I_TYPE)
      skip_bits_long(gb, s->cplx_estimation_trash_p);
    if(s->pict_type == FF_B_TYPE)
      skip_bits_long(gb, s->cplx_estimation_trash_b);

    s->intra_dc_threshold= mpeg4_dc_threshold[ get_bits(gb, 3) ];
    if(!s->progressive_sequence){
      s->top_field_first= get_bits1(gb);
      s->alternate_scan= get_bits1(gb);
    }else
      s->alternate_scan= 0;
  }

  if(s->alternate_scan){
    ff_init_scantable(s->dsp.idct_permutation, &s->inter_scantable  , ff_alternate_vertical_scan);
    ff_init_scantable(s->dsp.idct_permutation, &s->intra_scantable  , ff_alternate_vertical_scan);
    ff_init_scantable(s->dsp.idct_permutation, &s->intra_h_scantable, ff_alternate_vertical_scan);
    ff_init_scantable(s->dsp.idct_permutation, &s->intra_v_scantable, ff_alternate_vertical_scan);
  } else{
    ff_init_scantable(s->dsp.idct_permutation, &s->inter_scantable  , ff_zigzag_direct);
    ff_init_scantable(s->dsp.idct_permutation, &s->intra_scantable  , ff_zigzag_direct);
    ff_init_scantable(s->dsp.idct_permutation, &s->intra_h_scantable, ff_alternate_horizontal_scan);
    ff_init_scantable(s->dsp.idct_permutation, &s->intra_v_scantable, ff_alternate_vertical_scan);
  }

  if(s->pict_type == FF_S_TYPE && (s->vol_sprite_usage==STATIC_SPRITE || s->vol_sprite_usage==GMC_SPRITE)){
    mpeg4_decode_sprite_trajectory(s, gb);
    if(s->sprite_brightness_change) av_log(s->avctx, AV_LOG_ERROR, "sprite_brightness_change not supported\n");
    if(s->vol_sprite_usage==STATIC_SPRITE) av_log(s->avctx, AV_LOG_ERROR, "static sprite not supported\n");
  }

  if (s->shape != BIN_ONLY_SHAPE) {
    s->chroma_qscale= s->qscale = get_bits(gb, s->quant_precision);
    if(s->qscale==0){
      av_log(s->avctx, AV_LOG_ERROR, "Error, header damaged or not MPEG4 header (qscale=0)\n");
      return -1; // makes no sense to continue, as there is nothing left from the image then
    }

    if (s->pict_type != FF_I_TYPE) {
      s->f_code = get_bits(gb, 3);       /* fcode_for */
      if(s->f_code==0){
	av_log(s->avctx, AV_LOG_ERROR, "Error, header damaged or not MPEG4 header (f_code=0)\n");
	return -1; // makes no sense to continue, as the MV decoding will break very quickly
      }
    }else
      s->f_code=1;

    if (s->pict_type == FF_B_TYPE) {
      s->b_code = get_bits(gb, 3);
    }else
      s->b_code=1;

    if(s->avctx->debug&FF_DEBUG_PICT_INFO){
      av_log(s->avctx, AV_LOG_DEBUG, "qp:%d fc:%d,%d %s size:%d pro:%d alt:%d top:%d %spel part:%d resync:%d w:%d a:%d rnd:%d vot:%d%s dc:%d ce:%d/%d/%d\n",
	     s->qscale, s->f_code, s->b_code,
	     s->pict_type == FF_I_TYPE ? "I" : (s->pict_type == FF_P_TYPE ? "P" : (s->pict_type == FF_B_TYPE ? "B" : "S")),
	     gb->size_in_bits,s->progressive_sequence, s->alternate_scan, s->top_field_first,
	     s->quarter_sample ? "q" : "h", s->data_partitioning, s->resync_marker, s->num_sprite_warping_points,
	     s->sprite_warping_accuracy, 1-s->no_rounding, s->vo_type, s->vol_control_parameters ? " VOLC" : " ", s->intra_dc_threshold, s->cplx_estimation_trash_i, s->cplx_estimation_trash_p, s->cplx_estimation_trash_b);
    }

    if(!s->scalability){
      if (s->shape!=RECT_SHAPE && s->pict_type!=FF_I_TYPE) {
	skip_bits1(gb); // vop shape coding type
      }
    }else{
      if(s->enhancement_type){
	int load_backward_shape= get_bits1(gb);
	if(load_backward_shape){
	  av_log(s->avctx, AV_LOG_ERROR, "load backward shape isn't supported\n");
	}
      }
      skip_bits(gb, 2); //ref_select_code
    }
  }
  /* detect buggy encoders which don't set the low_delay flag (divx4/xvid/opendivx)*/
  // note we cannot detect divx5 without b-frames easily (although it's buggy too)
  if(s->vo_type==0 && s->vol_control_parameters==0 && s->divx_version==-1 && s->picture_number==0){
    av_log(s->avctx, AV_LOG_ERROR, "looks like this file was encoded with (divx4/(old)xvid/opendivx) -> forcing low_delay flag\n");
    s->low_delay=1;
  }

  s->picture_number++; // better than pic number==0 always ;)

  s->y_dc_scale_table= ff_mpeg4_y_dc_scale_table; //FIXME add short header support
  s->c_dc_scale_table= ff_mpeg4_c_dc_scale_table;

  if(s->workaround_bugs&FF_BUG_EDGE){
    s->h_edge_pos= s->width;
    s->v_edge_pos= s->height;
  }
  return 0;
}

static av_cold int decode_init(AVCodecContext *avctx)
{
  MpegEncContext *s = avctx->priv_data;
  int ret;
  static int done = 0;

  //printf("decode_init p0\n");
  s->divx_version=
    s->divx_build=
    s->xvid_build=
    s->lavc_build= -1;

  if((ret=mpeg4_decode_init(avctx)) < 0)
    return ret;

  //printf("decode_init, mpeg4_decode_init end\n");

  if (!done) {
    done = 1;

    init_rl(&ff_mpeg4_rl_intra_hw, ff_mpeg4_static_rl_table_store[0]);
    INIT_VLC_RL_HW(ff_mpeg4_rl_intra_hw, 554);

    init_rl(&ff_mpeg4_rl_intra, ff_mpeg4_static_rl_table_store[0]);
    init_rl(&rvlc_rl_inter, ff_mpeg4_static_rl_table_store[1]);
    init_rl(&rvlc_rl_intra, ff_mpeg4_static_rl_table_store[2]);
    INIT_VLC_RL(ff_mpeg4_rl_intra, 554);
    INIT_VLC_RL(rvlc_rl_inter, 1072);
    INIT_VLC_RL(rvlc_rl_intra, 1072);
    INIT_VLC_STATIC(&dc_lum, DC_VLC_BITS, 10 /* 13 */,
		    &ff_mpeg4_DCtab_lum[0][1], 2, 1,
		    &ff_mpeg4_DCtab_lum[0][0], 2, 1, 512);
    INIT_VLC_STATIC(&dc_chrom, DC_VLC_BITS, 10 /* 13 */,
		    &ff_mpeg4_DCtab_chrom[0][1], 2, 1,
		    &ff_mpeg4_DCtab_chrom[0][0], 2, 1, 512);
    INIT_VLC_STATIC(&sprite_trajectory, SPRITE_TRAJ_VLC_BITS, 15,
		    &sprite_trajectory_tab[0][1], 4, 2,
		    &sprite_trajectory_tab[0][0], 4, 2, 128);
    INIT_VLC_STATIC(&mb_type_b_vlc, MB_TYPE_B_VLC_BITS, 4,
		    &mb_type_b_tab[0][1], 2, 1,
		    &mb_type_b_tab[0][0], 2, 1, 16);
  }

  s->h263_pred = 1;
  s->low_delay = 0; //default, might be overriden in the vol header during header parsing
  s->decode_mb= mpeg4_decode_mb;
  s->time_increment_bits = 4; /* default value for broken headers */
  avctx->chroma_sample_location = AVCHROMA_LOC_LEFT;

  //mpeg4_MDarg_init();

  return 0;
}

int JZC_mpeg4_decode_picture_header(MpegEncContext * s, GetBitContext *gb){
  int startcode, v;

  //printf("JZC_mpeg4_decode_picture_header start\n");
  /* search next start code */
  align_get_bits(gb);

  if(s->codec_tag == AV_RL32("WV1F") && show_bits(gb, 24) == 0x575630){
    skip_bits(gb, 24);
    if(get_bits(gb, 8) == 0xF0)
      goto end;
  }

  startcode = 0xff;
  for(;;) {
    if(get_bits_count(gb) >= gb->size_in_bits){
      if(gb->size_in_bits==8 && (s->divx_version>=0 || s->xvid_build>=0)){
	av_log(s->avctx, AV_LOG_ERROR, "frame skip %d\n", gb->size_in_bits);
	return FRAME_SKIPPED; //divx bug
      }else
	return -1; //end of stream
    }

    /* use the bits after the test */
    v = get_bits(gb, 8);
    startcode = ((startcode << 8) | v) & 0xffffffff;

    if((startcode&0xFFFFFF00) != 0x100)
      continue; //no startcode

    if(startcode >= 0x120 && startcode <= 0x12F){
      //printf("decode_vol_header start\n");
      if(decode_vol_header(s, gb) < 0)
	return -1;
      //printf("decode_vol_header end\n");
    }
    else if(startcode == USER_DATA_STARTCODE){
      decode_user_data(s, gb);
    }
    else if(startcode == GOP_STARTCODE){
      mpeg4_decode_gop_header(s, gb);
    }
    else if(startcode == VOP_STARTCODE){
      break;
    }

    align_get_bits(gb);
    startcode = 0xff;
  }

 end:
  if(s->flags & CODEC_FLAG_LOW_DELAY)
    s->low_delay=1;
  s->avctx->has_b_frames= !s->low_delay;
  //printf("decode_vop_header start\n");

  return decode_vop_header(s, gb);
}

AVCodec mpeg4_decoder = {
  "mpeg4",
  AVMEDIA_TYPE_VIDEO,
  CODEC_ID_MPEG4,
  sizeof(MpegEncContext),
  decode_init,
  NULL,
  mpeg4_decode_end,
  mpeg4_decode_frame,
  CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1 | CODEC_CAP_TRUNCATED | CODEC_CAP_DELAY,
  .flush= ff_mpeg_flush,
  .max_lowres= 3,
  .long_name= NULL_IF_CONFIG_SMALL("MPEG-4 part 2"),
  .pix_fmts= ff_hwaccel_pixfmt_list_420,
};
