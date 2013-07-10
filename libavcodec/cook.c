/*
 * COOK compatible decoder
 * Copyright (c) 2003 Sascha Sommer
 * Copyright (c) 2005 Benjamin Larsson
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
 * Cook compatible decoder. Bastardization of the G.722.1 standard.
 * This decoder handles RealNetworks, RealAudio G2 data.
 * Cook is identified by the codec name cook in RM files.
 *
 * To use this decoder, a calling application must supply the extradata
 * bytes provided from the RM container; 8+ bytes for mono streams and
 * 16+ for stereo streams (maybe more).
 *
 * Codec technicalities (all this assume a buffer length of 1024):
 * Cook works with several different techniques to achieve its compression.
 * In the timedomain the buffer is divided into 8 pieces and quantized. If
 * two neighboring pieces have different quantization index a smooth
 * quantization curve is used to get a smooth overlap between the different
 * pieces.
 * To get to the transformdomain Cook uses a modulated lapped transform.
 * The transform domain has 50 subbands with 20 elements each. This
 * means only a maximum of 50*20=1000 coefficients are used out of the 1024
 * available.
 */

#include <math.h>
#include <stddef.h>
#include <stdio.h>

#include "libavutil/lfg.h"
#include "libavutil/random_seed.h"
#include "avcodec.h"
#include "get_bits.h"
#include "dsputil.h"
#include "bytestream.h"
#include "fft.h"

#include "cookdata.h"

#include "../libjzcommon/jzmedia.h"
#include "../libjzcommon/jzasm.h"
#include "../libjzcommon/com_config.h"

#ifdef JZC_PMON_P0
#include "../libjzcommon/jz4760e_pmon.h"
PMON_CREAT(cook);
#endif

/* the different Cook versions */
#define MONO            0x1000001
#define STEREO          0x1000002
#define JOINT_STEREO    0x1000003
#define MC_COOK         0x2000000   //multichannel Cook, not supported

#define SUBBAND_SIZE    20
#define MAX_SUBPACKETS   5
//#define COOKDEBUG

typedef struct {
  int *now;
  int *previous;
} cook_gains;

typedef struct {
  int                 ch_idx;
  int                 size;
  int                 num_channels;
  int                 cookversion;
  int                 samples_per_frame;
  int                 subbands;
  int                 js_subband_start;
  int                 js_vlc_bits;
  int                 samples_per_channel;
  int                 log2_numvector_size;
  unsigned int        channel_mask;
  VLC                 ccpl;                 ///< channel coupling
  int                 joint_stereo;
  int                 bits_per_subpacket;
  int                 bits_per_subpdiv;
  int                 total_subbands;
  int                 numvector_size;       ///< 1 << log2_numvector_size;

  float               mono_previous_buffer1[1024];
  float               mono_previous_buffer2[1024];
  /** gain buffers */
  cook_gains          gains1;
  cook_gains          gains2;
  int                 gain_1[9];
  int                 gain_2[9];
  int                 gain_3[9];
  int                 gain_4[9];
} COOKSubpacket;

typedef struct cook {
  /*
   * The following 5 functions provide the lowlevel arithmetic on
   * the internal audio buffers.
   */
  void (* scalar_dequant)(struct cook *q, int index, int quant_index,
			  int* subband_coef_index, int* subband_coef_sign,
			  float* mlt_p, int* s32_mlt_p);

  void (* decouple) (struct cook *q,
		     COOKSubpacket *p,
		     int subband,
		     float f1, float f2,
		     float *decode_buffer,
		     float *mlt_buffer1, float *mlt_buffer2);

  void (* imlt_window) (struct cook *q, float *buffer1,
			cook_gains *gains_ptr, float *previous_buffer);

  void (* interpolate) (struct cook *q, float* buffer,
			int gain_index, int gain_index_next);

  void (* saturate_output) (struct cook *q, int chan, int16_t *out);

  AVCodecContext*     avctx;
  GetBitContext       gb;
  /* stream data */
  int                 nb_channels;
  int                 bit_rate;
  int                 sample_rate;
  int                 num_vectors;
  int                 samples_per_channel;
  /* states */
  AVLFG               random_state;

  /* transform data */
  FFTContext          mdct_ctx;
  float*              mlt_window;

  /* VLC data */
  VLC                 envelope_quant_index[13];
  VLC                 sqvh[7];          //scalar quantization

  /* generatable tables and related variables */
  int                 gain_size_factor;
  float               gain_table[23];

  /* data buffers */

  uint8_t*            decoded_bytes_buffer;
  DECLARE_ALIGNED(16, float,mono_mdct_output)[2048];
  DECLARE_ALIGNED(16, int,s32_mono_mdct_output)[2048];
  float               decode_buffer_1[1024];
  float               decode_buffer_2[1024];
  float               decode_buffer_0[1060]; /* static allocation for joint decode */

  const float         *cplscales[5];
  int                 num_subpackets;
  COOKSubpacket       subpacket[MAX_SUBPACKETS];
} COOKContext;

int mpFrame;

static float     pow2tab[127];
static float rootpow2tab[127];

static int s32_pow2tab[127];
static int s32_rootpow2tab[127];
static int *sp32_pow2tab;
static int *sp32_rootpow2tab;
//static int *s32_rootpow2tab_L14;
static int pow2tab_sft[127];
static int rootpow2tab_sft[127];
static int s32_gain_table_L20[23];

int s32_decode_buffer_0[1060];
int s32_decode_buffer_1[1024];
int s32_decode_buffer_2[1024];

int s32_mono_previous_buffer1[1024];
int s32_mono_previous_buffer2[1024];

int *s32_mlt_window = NULL;

#define ftoi_10(i,f)				\
  {\
    int t = 0x400;\
    i = f*t;	     \
  }

#define ftoi_14(i,f)				\
  {\
    int t = 0x4000;\
    i = f*t;	     \
  }

#define ftoi_20(i,f)				\
  {\
    int t = 0x100000;\
    i = f*t;	     \
  }

#define ftoi_30(i,f)				\
  {\
    int t = 0x40000000;\
    i = f*t;	     \
  }

#define COOK_MXU_OPT

static inline int S32MUL_R_MXU(a,b){
  S32MUL(xr1,xr2, (a), (b));
  return S32M2I(xr2);
}

#define S32MUL_Rx(a,b,x) (int)(((long long)(a)*(long long)(b))>>(x))

static inline int S32MUL_Rx_MXU(a,b,x){
  S32MUL(xr1,xr2, (a), (b));
  S32EXTRV(xr1,xr2,33-(x),31);
  D32SLL(xr1,xr1,xr3,xr3,1);
  D32SAR(xr1,xr1,xr3,xr3,1);
  return S32M2I(xr1);
}

#define S32MUL_R30(a,b) (int)(((long long)(a)*(long long)(b))>>30)

static inline int S32MUL_R30_MXU(int a, int b){
  S32MUL(xr1,xr2, (a), (b));
  S32EXTRV(xr1,xr2,3,31);
  D32SLL(xr1,xr1,xr3,xr3,1);
  D32SAR(xr1,xr1,xr3,xr3,1);
  return S32M2I(xr1);
}

#define S32MUL_R26(a,b) (int)(((long long)(a)*(long long)(b))>>26)
#define S32MUL_R24(a,b) (int)(((long long)(a)*(long long)(b))>>24)
#define S32MUL_R22(a,b) (int)(((long long)(a)*(long long)(b))>>22)
#define S32MUL_R21(a,b) (int)(((long long)(a)*(long long)(b))>>21)
#define S32MUL_R20(a,b) (int)(((long long)(a)*(long long)(b))>>20)
#define S32MUL_R18(a,b) (int)(((long long)(a)*(long long)(b))>>18)
#define S32MUL_R16(a,b) (int)(((long long)(a)*(long long)(b))>>16)
#define S32MUL_R15(a,b) (int)(((long long)(a)*(long long)(b))>>15)
#define S32MUL_R14(a,b) (int)(((long long)(a)*(long long)(b))>>14)
#define S32MUL_R6(a,b) (int)(((long long)(a)*(long long)(b))>>6)
//#define S32MUL(a,b) (int)((long long)(a)*(long long)(b))
/* debug functions */

#ifdef COOKDEBUG
static void dump_float_table(float* table, int size, int delimiter) {
  int i=0;
  av_log(NULL,AV_LOG_ERROR,"\n[%d]: ",i);
  for (i=0 ; i<size ; i++) {
    av_log(NULL, AV_LOG_ERROR, "%5.1f, ", table[i]);
    if ((i+1)%delimiter == 0) av_log(NULL,AV_LOG_ERROR,"\n[%d]: ",i+1);
  }
}

static void dump_int_table(int* table, int size, int delimiter) {
  int i=0;
  av_log(NULL,AV_LOG_ERROR,"\n[%d]: ",i);
  for (i=0 ; i<size ; i++) {
    av_log(NULL, AV_LOG_ERROR, "%d, ", table[i]);
    if ((i+1)%delimiter == 0) av_log(NULL,AV_LOG_ERROR,"\n[%d]: ",i+1);
  }
}

static void dump_short_table(short* table, int size, int delimiter) {
  int i=0;
  av_log(NULL,AV_LOG_ERROR,"\n[%d]: ",i);
  for (i=0 ; i<size ; i++) {
    av_log(NULL, AV_LOG_ERROR, "%d, ", table[i]);
    if ((i+1)%delimiter == 0) av_log(NULL,AV_LOG_ERROR,"\n[%d]: ",i+1);
  }
}

#endif

#undef printf

/*************** init functions ***************/

/* table generator */
static av_cold void init_pow2table(void){
  int i;
  for (i=-63 ; i<64 ; i++){
    pow2tab[63+i]=     pow(2, i);
    rootpow2tab[63+i]=sqrt(pow(2, i));
    //printf("%d %f %f\n", i, pow2tab[63+i], rootpow2tab[63+i]/*, pow(2,i), sqrt(pow(2, i))*/);
  }
#if 1
  for (i = -63; i < -41; i++){
    s32_pow2tab[63+i] = 0;
    s32_rootpow2tab[63+i] = 0;

    pow2tab_sft[63+i] = 0;
    rootpow2tab_sft[63+i] = 0;
  }

  for (i = -41; i < 0; i++){
    ftoi_20(s32_pow2tab[63+i], pow2tab[63+i]);
    ftoi_14(s32_rootpow2tab[63+i], rootpow2tab[63+i]);

    pow2tab_sft[63+i] = 20;
    rootpow2tab_sft[63+i] = 14;
  }

  for (i = 0; i < 34; i++){
    ftoi_14(s32_rootpow2tab[63+i], rootpow2tab[63+i]);
    rootpow2tab_sft[63+i] = 14;

    s32_pow2tab[63+i] = ((int)(pow2tab[63+i]));
    pow2tab_sft[63+i] = 0;
  }

  for (i = 34; i < 64; i++){
    s32_rootpow2tab[63+i] = (int)(rootpow2tab[63+i]);
    rootpow2tab_sft[63+i] = 0;

    s32_pow2tab[63+i] = ((int)(pow2tab[63+i]));
    pow2tab_sft[63+i] = 0;
  }

  sp32_pow2tab = &s32_pow2tab[63];
  sp32_rootpow2tab = &s32_rootpow2tab[63];
#else

  memset(s32_pow2tab, 0, sizeof(int)*127);
  memset(s32_rootpow2tab, 0, sizeof(int)*127);
  for (i = -26; i < 64; i++){
    ftoi_14(s32_pow2tab[i+63], pow2tab[i+63]);
    ftoi_14(s32_rootpow2tab[i+63], rootpow2tab[i+63]);
    //printf("%d %d %f\n", i, s32_rootpow2tab_L14[i], rootpow2tab[i+63]);
  }
  s32_rootpow2tab_L14 = &s32_rootpow2tab[63];
#endif
}

/* table generator */
static av_cold void init_gain_table(COOKContext *q) {
  int i;
  q->gain_size_factor = q->samples_per_channel/8;
  for (i=0 ; i<23 ; i++) {
    q->gain_table[i] = pow(pow2tab[i+52] ,
			   (1.0/(double)q->gain_size_factor));
    ftoi_20(s32_gain_table_L20[i], q->gain_table[i]);
    //printf("%f %d\n", q->gain_table[i], s32_gain_table_L20[i]);
  }
}


static av_cold int init_cook_vlc_tables(COOKContext *q) {
  int i, result;

  result = 0;
  for (i=0 ; i<13 ; i++) {
    result |= init_vlc (&q->envelope_quant_index[i], 9, 24,
			envelope_quant_index_huffbits[i], 1, 1,
			envelope_quant_index_huffcodes[i], 2, 2, 0);
  }
  av_log(q->avctx,AV_LOG_DEBUG,"sqvh VLC init\n");
  for (i=0 ; i<7 ; i++) {
    result |= init_vlc (&q->sqvh[i], vhvlcsize_tab[i], vhsize_tab[i],
			cvh_huffbits[i], 1, 1,
			cvh_huffcodes[i], 2, 2, 0);
  }

  for(i=0;i<q->num_subpackets;i++){
    if (q->subpacket[i].joint_stereo==1){
      result |= init_vlc (&q->subpacket[i].ccpl, 6, (1<<q->subpacket[i].js_vlc_bits)-1,
			  ccpl_huffbits[q->subpacket[i].js_vlc_bits-2], 1, 1,
			  ccpl_huffcodes[q->subpacket[i].js_vlc_bits-2], 2, 2, 0);
      av_log(q->avctx,AV_LOG_DEBUG,"subpacket %i Joint-stereo VLC used.\n",i);
    }
  }

  av_log(q->avctx,AV_LOG_DEBUG,"VLC tables initialized.\n");
  return result;
}

static av_cold int init_cook_mlt(COOKContext *q) {
  int j;
  int mlt_size = q->samples_per_channel;

  if ((q->mlt_window = av_malloc(sizeof(float)*mlt_size)) == 0)
    return -1;

  if ((s32_mlt_window = av_malloc(sizeof(int)*mlt_size)) == 0){//opt
    av_free(q->mlt_window);
    return -1;
  }

  /* Initialize the MLT window: simple sine window. */
  ff_sine_window_init(q->mlt_window, mlt_size);
  for(j=0 ; j<mlt_size ; j++){
    q->mlt_window[j] *= sqrt(2.0 / q->samples_per_channel);
    ftoi_30(s32_mlt_window[j], q->mlt_window[j]);//opt
    //printf("win: %d %f\n", s32_mlt_window[j], q->mlt_window[j]);
  }

#if 1
  /* Initialize the MDCT. */
  if (cook_mdct_init(&q->mdct_ctx, av_log2(mlt_size)+1, 1, 1.0)) {
    av_free(q->mlt_window);
    av_free(s32_mlt_window);
    return -1;
  }
  //printf("ff_mdct_init successful\n");
#else
  if (aac_mdct_init(&q->mdct_ctx, av_log2(mlt_size)+1, 1, 1.0)){
    printf("aac_mdct_init failure\n");
    av_free(q->mlt_window);
    av_free(s32_mlt_window);
    return -1;
  }
  printf("aac_mdct_init successful\n");
#endif
  av_log(q->avctx,AV_LOG_DEBUG,"MDCT initialized, order = %d.\n",
	 av_log2(mlt_size)+1);

  return 0;
}

static av_cold void init_previous_buffer(float *f1, float *f2){
  int i;

  for (i = 0; i< 1024; i++){
    //printf("init_previous_buffer %d %f %f\n", i, f1[i], f2[2]);

    ftoi_10(s32_mono_previous_buffer1[i], f1[i]);
    ftoi_10(s32_mono_previous_buffer2[i], f2[i]);
  }

  return;
}

static const float *maybe_reformat_buffer32 (COOKContext *q, const float *ptr, int n)
{
  if (1)
    return ptr;
}

static av_cold void init_cplscales_table (COOKContext *q) {
  int i;
  for (i=0;i<5;i++)
    q->cplscales[i] = maybe_reformat_buffer32 (q, cplscales[i], (1<<(i+2))-1);

  for (i = 0; i < 3; i++){
    ftoi_30(s32_cplscale2[i], cplscale2[i]);
  }

  for (i = 0; i < 7; i++){
    ftoi_30(s32_cplscale3[i], cplscale3[i]);
  }

  for (i = 0; i < 15; i++){
    ftoi_30(s32_cplscale4[i], cplscale4[i]);
  }

  for (i = 0; i < 31; i++){
    ftoi_30(s32_cplscale5[i], cplscale5[i]);
  }

  for (i = 0; i < 63; i++){
    ftoi_30(s32_cplscale6[i], cplscale6[i]);
  }
}

/*************** init functions end ***********/

#define DECODE_BYTES_PAD1(bytes) (3 - ((bytes)+3) % 4)
#define DECODE_BYTES_PAD2(bytes) ((bytes) % 4 + DECODE_BYTES_PAD1(2 * (bytes)))

/**
 * Cook indata decoding, every 32 bits are XORed with 0x37c511f2.
 * Why? No idea, some checksum/error detection method maybe.
 *
 * Out buffer size: extra bytes are needed to cope with
 * padding/misalignment.
 * Subpackets passed to the decoder can contain two, consecutive
 * half-subpackets, of identical but arbitrary size.
 *          1234 1234 1234 1234  extraA extraB
 * Case 1:  AAAA BBBB              0      0
 * Case 2:  AAAA ABBB BB--         3      3
 * Case 3:  AAAA AABB BBBB         2      2
 * Case 4:  AAAA AAAB BBBB BB--    1      5
 *
 * Nice way to waste CPU cycles.
 *
 * @param inbuffer  pointer to byte array of indata
 * @param out       pointer to byte array of outdata
 * @param bytes     number of bytes
 */

static inline int decode_bytes(const uint8_t* inbuffer, uint8_t* out, int bytes){
  int i, off;
  uint32_t c;
  const uint32_t* buf;
  uint32_t* obuf = (uint32_t*) out;
  /* FIXME: 64 bit platforms would be able to do 64 bits at a time.
   * I'm too lazy though, should be something like
   * for(i=0 ; i<bitamount/64 ; i++)
   *     (int64_t)out[i] = 0x37c511f237c511f2^av_be2ne64(int64_t)in[i]);
   * Buffer alignment needs to be checked. */

  off = (intptr_t)inbuffer & 3;
  buf = (const uint32_t*) (inbuffer - off);
  c = av_be2ne32((0x37c511f2 >> (off*8)) | (0x37c511f2 << (32-(off*8))));
  bytes += 3 + off;
  for (i = 0; i < bytes/4; i++)
    obuf[i] = c ^ buf[i];

  return off;
}

/**
 * Cook uninit
 */

static av_cold int cook_decode_close(AVCodecContext *avctx)
{
  //printf("file:%s, func:%s, line:%d\n", __FILE__, __func__, __LINE__);
  int i;
  COOKContext *q = avctx->priv_data;
  //av_log(avctx,AV_LOG_DEBUG, "Deallocating memory.\n");

  /* Free allocated memory buffers. */
  av_free(q->mlt_window);
  av_free(s32_mlt_window);
  av_free(q->decoded_bytes_buffer);

  /* Free the transform. */
  ff_mdct_end(&q->mdct_ctx);

  /* Free the VLC tables. */
  for (i=0 ; i<13 ; i++) {
    free_vlc(&q->envelope_quant_index[i]);
  }
  for (i=0 ; i<7 ; i++) {
    free_vlc(&q->sqvh[i]);
  }
  for (i=0 ; i<q->num_subpackets ; i++) {
    free_vlc(&q->subpacket[i].ccpl);
  }

  av_log(avctx,AV_LOG_DEBUG,"Memory deallocated.\n");

  return 0;
}

/**
 * Fill the gain array for the timedomain quantization.
 *
 * @param gb          pointer to the GetBitContext
 * @param gaininfo[9] array of gain indexes
 */

static void decode_gain_info(GetBitContext *gb, int *gaininfo)
{
  int i, n;

  while (get_bits1(gb)) {}
  n = get_bits_count(gb) - 1;     //amount of elements*2 to update

  i = 0;
  while (n--) {
    int index = get_bits(gb, 3);
    int gain = get_bits1(gb) ? get_bits(gb, 4) - 7 : -1;

    while (i <= index) gaininfo[i++] = gain;
  }
  while (i <= 8) gaininfo[i++] = 0;
}

/**
 * Create the quant index table needed for the envelope.
 *
 * @param q                 pointer to the COOKContext
 * @param quant_index_table pointer to the array
 */

static void decode_envelope(COOKContext *q, COOKSubpacket *p, int* quant_index_table) {
  int i,j, vlc_index;

  quant_index_table[0]= get_bits(&q->gb,6) - 6;       //This is used later in categorize

  for (i=1 ; i < p->total_subbands ; i++){
    vlc_index=i;
    if (i >= p->js_subband_start * 2) {
      vlc_index-=p->js_subband_start;
    } else {
      vlc_index/=2;
      if(vlc_index < 1) vlc_index = 1;
    }
    if (vlc_index>13) vlc_index = 13;           //the VLC tables >13 are identical to No. 13

    j = get_vlc2(&q->gb, q->envelope_quant_index[vlc_index-1].table,
		 q->envelope_quant_index[vlc_index-1].bits,2);
    quant_index_table[i] = quant_index_table[i-1] + j - 12;    //differential encoding
  }
}

/**
 * Calculate the category and category_index vector.
 *
 * @param q                     pointer to the COOKContext
 * @param quant_index_table     pointer to the array
 * @param category              pointer to the category array
 * @param category_index        pointer to the category_index array
 */

static void categorize(COOKContext *q, COOKSubpacket *p, int* quant_index_table,
                       int* category, int* category_index){
  int exp_idx, bias, tmpbias1, tmpbias2, bits_left, num_bits, index, v, i, j;
  int exp_index2[102];
  int exp_index1[102];

  int tmp_categorize_array[128*2];
  int tmp_categorize_array1_idx=p->numvector_size;
  int tmp_categorize_array2_idx=p->numvector_size;

  bits_left =  p->bits_per_subpacket - get_bits_count(&q->gb);

  if(bits_left > q->samples_per_channel) {
    bits_left = q->samples_per_channel +
      ((bits_left - q->samples_per_channel)*5)/8;
    //av_log(q->avctx, AV_LOG_ERROR, "bits_left = %d\n",bits_left);
  }

  memset(&exp_index1,0,102*sizeof(int));
  memset(&exp_index2,0,102*sizeof(int));
  memset(&tmp_categorize_array,0,128*2*sizeof(int));

#if 1
  bias=-32;
  /* Estimate bias. */
  for (i=32 ; i>0 ; i=i/2){
    num_bits = 0;
    index = 0;
    for (j=p->total_subbands ; j>0 ; j--){
      exp_idx = av_clip((i - quant_index_table[index] + bias) / 2, 0, 7);
      index++;
      num_bits+=expbits_tab[exp_idx];
    }
    if(num_bits >= bits_left - 32){
      bias+=i;
    }
  }
#endif

#if 0
  S32I2M(xr7, 7);
  bias=-32;
  /* Estimate bias. */
  for (i=32 ; i>0 ; i=i/2){
    num_bits = 0;
    index = 0;
    for (j=p->total_subbands ; j>0 ; j--){
      //exp_idx = av_clip((i - quant_index_table[index] + bias) / 2, 0, 7);
      exp_idx = (i - quant_index_table[index] + bias)/2;
      S32I2M(xr1, exp_idx);
      S32MAX(xr1,xr1,xr0);
      S32MIN(xr1,xr1,xr7);
      exp_idx = S32M2I(xr1);
      index++;
      num_bits+=expbits_tab[exp_idx];
    }
    if(num_bits >= bits_left - 32){
      bias+=i;
    }
  }
#endif

#if 0
  S32I2M(xr2, -32);
  S32I2M(xr7, 7);
  for (i = 32; i > 0; i=i/2){//xr1:num_bits    x2:bias     x5:exp_idx
    S32I2M(xr1, 0);//num_bits=0;
    index = 0;
    for (j = p->total_subbands; j > 0; j--){
      S32I2M(xr4, i);
      S32I2M(xr5, quant_index_table[index]);
      D32ADD_SS(xr4,xr5,xr4,xr5);
      D32ADD_AA(xr4,xr2,xr4,xr2);
      D32SAR(xr5,xr4,xr5,xr4,1);
      S32MAX(xr5,xr5,xr0);
      S32MIN(xr5,xr5,xr7);
      exp_idx = S32M2I(xr5);
      index++;
      S32I2M(xr3, expbits_tab[exp_idx]);
      D32ASUM_AA(xr1,xr3,xr1,xr3);
    }
    num_bits = S32M2I(xr1);
    if (num_bits > bits_left - 32){
      bias+=i;
    }
  }
#endif
  /* Calculate total number of bits. */
  num_bits=0;
  for (i=0 ; i<p->total_subbands ; i++) {
    exp_idx = av_clip((bias - quant_index_table[i]) / 2, 0, 7);
    num_bits += expbits_tab[exp_idx];
    exp_index1[i] = exp_idx;
    exp_index2[i] = exp_idx;
  }
  tmpbias1 = tmpbias2 = num_bits;

  for (j = 1 ; j < p->numvector_size ; j++) {
    if (tmpbias1 + tmpbias2 > 2*bits_left) {  /* ---> */
      int max = -999999;
      index=-1;
      for (i=0 ; i<p->total_subbands ; i++){
	if (exp_index1[i] < 7) {
	  v = (-2*exp_index1[i]) - quant_index_table[i] + bias;
	  if ( v >= max) {
	    max = v;
	    index = i;
	  }
	}
      }
      if(index==-1)break;
      tmp_categorize_array[tmp_categorize_array1_idx++] = index;
      tmpbias1 -= expbits_tab[exp_index1[index]] -
	expbits_tab[exp_index1[index]+1];
      ++exp_index1[index];
    } else {  /* <--- */
      int min = 999999;
      index=-1;
      for (i=0 ; i<p->total_subbands ; i++){
	if(exp_index2[i] > 0){
	  v = (-2*exp_index2[i])-quant_index_table[i]+bias;
	  if ( v < min) {
	    min = v;
	    index = i;
	  }
	}
      }
      if(index == -1)break;
      tmp_categorize_array[--tmp_categorize_array2_idx] = index;
      tmpbias2 -= expbits_tab[exp_index2[index]] -
	expbits_tab[exp_index2[index]-1];
      --exp_index2[index];
    }
  }

  for(i=0 ; i<p->total_subbands ; i++)
    category[i] = exp_index2[i];

  for(i=0 ; i<p->numvector_size-1 ; i++)
    category_index[i] = tmp_categorize_array[tmp_categorize_array2_idx++];

}


/**
 * Expand the category vector.
 *
 * @param q                     pointer to the COOKContext
 * @param category              pointer to the category array
 * @param category_index        pointer to the category_index array
 */

static inline void expand_category(COOKContext *q, int* category,
                                   int* category_index){
  int i;
  for(i=0 ; i<q->num_vectors ; i++){
    ++category[category_index[i]];
  }
}

/**
 * The real requantization of the mltcoefs
 *
 * @param q                     pointer to the COOKContext
 * @param index                 index
 * @param quant_index           quantisation index
 * @param subband_coef_index    array of indexes to quant_centroid_tab
 * @param subband_coef_sign     signs of coefficients
 * @param mlt_p                 pointer into the mlt buffer
 */
static void scalar_dequant_float(COOKContext *q, int index, int quant_index,
				 int* subband_coef_index, int* subband_coef_sign,
				 float* mlt_p, int* s32_mlt_p){
  int i;
  float f1;
  int i1;

  for(i=0 ; i<SUBBAND_SIZE ; i++) {
    if (subband_coef_index[i]) {
      f1 = quant_centroid_tab[index][subband_coef_index[i]];
      i1 = s32_quant_centroid_tab_L14[index][subband_coef_index[i]];
      if (subband_coef_sign[i]){
	f1 = -f1;
	//i1 = s32_quant_centroid_tab2_L14[index][subband_coef_index[i]];
	i1 = -i1;
	//if (i1 == -1)
	//s32_mlt_p[i] = -1;
	//else
#ifdef COOK_MXU_OPT
	//S32MUL_Rx(s32_mlt_p[i], i1, sp32_rootpow2tab[quant_index], rootpow2tab_sft[63+quant_index]+4);
	s32_mlt_p[i] = S32MUL_Rx(i1, sp32_rootpow2tab[quant_index], rootpow2tab_sft[63+quant_index]+7);
#else
	s32_mlt_p[i] = S32MUL_Rx(i1, sp32_rootpow2tab[quant_index], rootpow2tab_sft[63+quant_index]+4);
#endif
      }else{
	//if (i1 == 1)
	//s32_mlt_p[i] = 1;
	//else
#ifdef COOK_MXU_OPT
	//S32MUL_Rx(s32_mlt_p[i], i1, sp32_rootpow2tab[quant_index], rootpow2tab_sft[63+quant_index]+4);
	s32_mlt_p[i] = S32MUL_Rx(i1, sp32_rootpow2tab[quant_index], rootpow2tab_sft[63+quant_index]+7);
#else
	s32_mlt_p[i] = S32MUL_Rx(i1, sp32_rootpow2tab[quant_index], rootpow2tab_sft[63+quant_index]+4);
#endif
      }
    } else {
      /* noise coding if subband_coef_index[i] == 0 */
      f1 = dither_tab[index];
      i1 = s32_dither_tab_L20[index];
      if (av_lfg_get(&q->random_state) < 0x80000000){
	f1 = -f1;
	//i1 = s32_dither_tab2_L20[index];
	i1 = -i1;
	//if (i1 == -1)
	//s32_mlt_p[i] = -1;
	//else
#ifdef COOK_MXU_OPT
	//S32MUL_Rx(s32_mlt_p[i], i1, sp32_rootpow2tab[quant_index], rootpow2tab_sft[63+quant_index]+10);
	s32_mlt_p[i] = S32MUL_Rx(i1, sp32_rootpow2tab[quant_index], rootpow2tab_sft[63+quant_index]+13);
#else
	s32_mlt_p[i] = S32MUL_Rx(i1, sp32_rootpow2tab[quant_index], rootpow2tab_sft[63+quant_index]+10);
#endif
      }else{
	//if (i1 == 1)
	//s32_mlt_p[i] = 1;
	//else
#ifdef COOK_MXU_OPT
	//S32MUL_Rx(s32_mlt_p[i], i1, sp32_rootpow2tab[quant_index], rootpow2tab_sft[63+quant_index]+10);
	s32_mlt_p[i] = S32MUL_Rx(i1, sp32_rootpow2tab[quant_index], rootpow2tab_sft[63+quant_index]+13);
#else
	s32_mlt_p[i] = S32MUL_Rx(i1, sp32_rootpow2tab[quant_index], rootpow2tab_sft[63+quant_index]+10);
#endif
      }
    }
    mlt_p[i] = f1 * rootpow2tab[quant_index+63];
  }
}

static void scalar_dequant_int(COOKContext *q, int index, int quant_index,
				 int* subband_coef_index, int* subband_coef_sign,
				 int* s32_mlt_p){
  int i;
  int i1;

  int s1 = sp32_rootpow2tab[quant_index];
  int l1 = rootpow2tab_sft[63+quant_index]+7;
  int l2 = rootpow2tab_sft[63+quant_index]+13;

  for(i=0 ; i<SUBBAND_SIZE ; i++) {
    if (subband_coef_index[i]) {
      i1 = s32_quant_centroid_tab_L14[index][subband_coef_index[i]];
      if (subband_coef_sign[i]){
	i1 = -i1;
      }
#ifdef COOK_MXU_OPT
      //S32MUL_Rx(s32_mlt_p[i], i1, sp32_rootpow2tab[quant_index], rootpow2tab_sft[63+quant_index]+4);
      s32_mlt_p[i] = S32MUL_Rx_MXU(i1, s1, l1);
#else
      s32_mlt_p[i] = S32MUL_Rx(i1, s1, l1);
#endif
    } else {
      i1 = s32_dither_tab_L20[index];
      if (av_lfg_get(&q->random_state) < 0x80000000){
	i1 = -i1;
      }
#ifdef COOK_MXU_OPT
      //S32MUL_Rx(s32_mlt_p[i], i1, sp32_rootpow2tab[quant_index], rootpow2tab_sft[63+quant_index]+10);
      s32_mlt_p[i] = S32MUL_Rx_MXU(i1, s1, l2);
#else
      s32_mlt_p[i] = S32MUL_Rx(i1, s1, l2);
#endif
    }
  }

  return;
}

/**
 * Unpack the subband_coef_index and subband_coef_sign vectors.
 *
 * @param q                     pointer to the COOKContext
 * @param category              pointer to the category array
 * @param subband_coef_index    array of indexes to quant_centroid_tab
 * @param subband_coef_sign     signs of coefficients
 */
static int unpack_SQVH(COOKContext *q, COOKSubpacket *p, int category, int* subband_coef_index,
                       int* subband_coef_sign) {
  int i,j;
  int vlc, vd ,tmp, result;

  vd = vd_tab[category];
  result = 0;
  for(i=0 ; i<vpr_tab[category] ; i++){
    vlc = get_vlc2(&q->gb, q->sqvh[category].table, q->sqvh[category].bits, 3);
    if (p->bits_per_subpacket < get_bits_count(&q->gb)){
      vlc = 0;
      result = 1;
    }
    for(j=vd-1 ; j>=0 ; j--){
      tmp = (vlc * invradix_tab[category])/0x100000;
      subband_coef_index[vd*i+j] = vlc - tmp * (kmax_tab[category]+1);
      vlc = tmp;
    }
    for(j=0 ; j<vd ; j++){
      if (subband_coef_index[i*vd + j]) {
	if(get_bits_count(&q->gb) < p->bits_per_subpacket){
	  subband_coef_sign[i*vd+j] = get_bits1(&q->gb);
	} else {
	  result=1;
	  subband_coef_sign[i*vd+j]=0;
	}
      } else {
	subband_coef_sign[i*vd+j]=0;
      }
    }
  }
  return result;
}


/**
 * Fill the mlt_buffer with mlt coefficients.
 *
 * @param q                 pointer to the COOKContext
 * @param category          pointer to the category array
 * @param quant_index_table pointer to the array
 * @param mlt_buffer        pointer to mlt coefficients
 */

static void decode_vectors(COOKContext* q, COOKSubpacket* p, int* category,
                           int *quant_index_table, float* mlt_buffer){
  /* A zero in this table means that the subband coefficient is
     random noise coded. */
  int subband_coef_index[SUBBAND_SIZE];
  /* A zero in this table means that the subband coefficient is a
     positive multiplicator. */
  int subband_coef_sign[SUBBAND_SIZE];
  int band, j;
  int index=0;

  for(band=0 ; band<p->total_subbands ; band++){
    index = category[band];
    if(category[band] < 7){
      if(unpack_SQVH(q, p, category[band], subband_coef_index, subband_coef_sign)){
	index=7;
	for(j=0 ; j<p->total_subbands ; j++) category[band+j]=7;
      }
    }
    if(index>=7) {
      memset(subband_coef_index, 0, sizeof(subband_coef_index));
      memset(subband_coef_sign, 0, sizeof(subband_coef_sign));
    }
    //printf("decode_vectors %d\n", band*SUBBAND_SIZE);
    q->scalar_dequant(q, index, quant_index_table[band],//pmon:50000
		      subband_coef_index, subband_coef_sign,
		      &mlt_buffer[band * SUBBAND_SIZE], &s32_decode_buffer_0[band * SUBBAND_SIZE]);
#ifdef JZC_PMON_P0
    //PMON_ON(cook);
#endif
    //scalar_dequant_int(q, index, quant_index_table[band],//pmon:50000
      //subband_coef_index, subband_coef_sign,
    //&s32_decode_buffer_0[band * SUBBAND_SIZE]);
#ifdef JZC_PMON_P0
    //PMON_OFF(cook);
#endif
  }

  if (mpFrame == -1){
    for (j = 0; j < 1060; j++){
      printf("decode_vectors end %d %d %f\n", j, s32_decode_buffer_0[j], mlt_buffer[j]);
    }
  }

  if(p->total_subbands*SUBBAND_SIZE >= q->samples_per_channel){
    return;
  } /* FIXME: should this be removed, or moved into loop above? */
}

static void s32_decode_vectors(COOKContext* q, COOKSubpacket* p, int* category,
                           int *quant_index_table, int* mlt_buffer){
  /* A zero in this table means that the subband coefficient is
     random noise coded. */
  int subband_coef_index[SUBBAND_SIZE];
  /* A zero in this table means that the subband coefficient is a
     positive multiplicator. */
  int subband_coef_sign[SUBBAND_SIZE];
  int band, j;
  int index=0;

  for(band=0 ; band<p->total_subbands ; band++){
    index = category[band];
    if(category[band] < 7){
      if(unpack_SQVH(q, p, category[band], subband_coef_index, subband_coef_sign)){
	index=7;
	for(j=0 ; j<p->total_subbands ; j++) category[band+j]=7;
      }
    }
    if(index>=7) {
#ifdef JZC_PMON_P0
      //PMON_ON(cook);
#endif
      memset(subband_coef_index, 0, sizeof(subband_coef_index));
      memset(subband_coef_sign, 0, sizeof(subband_coef_sign));
#ifdef JZC_PMON_P0
      //PMON_OFF(cook);
#endif
    }
    //printf("decode_vectors %d\n", band*SUBBAND_SIZE);
#ifdef JZC_PMON_P0
    //PMON_ON(cook);
#endif
    scalar_dequant_int(q, index, quant_index_table[band],//pmon:50000
		       subband_coef_index, subband_coef_sign,
		       &mlt_buffer[band * SUBBAND_SIZE]);
#ifdef JZC_PMON_P0
    //PMON_OFF(cook);
#endif
  }

  if (mpFrame == -1){
    for (j = 0; j < 1060; j++){
      printf("decode_vectors end %d %d %f\n", j, s32_decode_buffer_0[j], mlt_buffer[j]);
    }
  }

  if(p->total_subbands*SUBBAND_SIZE >= q->samples_per_channel){
    return;
  } /* FIXME: should this be removed, or moved into loop above? */
}


/**
 * function for decoding mono data
 *
 * @param q                 pointer to the COOKContext
 * @param mlt_buffer        pointer to mlt coefficients
 */

static void mono_decode(COOKContext *q, COOKSubpacket *p, float* mlt_buffer) {

  int category_index[128];
  int quant_index_table[102];
  int category[128];

  memset(&category, 0, 128*sizeof(int));
  memset(&category_index, 0, 128*sizeof(int));

  decode_envelope(q, p, quant_index_table);//pmon:10000
  q->num_vectors = get_bits(&q->gb,p->log2_numvector_size);
#ifdef JZC_PMON_P0
  //PMON_ON(cook);
#endif
  categorize(q, p, quant_index_table, category, category_index);//pmon:130000
#ifdef JZC_PMON_P0
  //PMON_OFF(cook);
#endif
  expand_category(q, category, category_index);

  decode_vectors(q, p, category, quant_index_table, mlt_buffer);//float opt
}

static void s32_mono_decode(COOKContext *q, COOKSubpacket *p, int* mlt_buffer) {

  int category_index[128];
  int quant_index_table[102];
  int category[128];

  //S32STD(xr0, block, 0);
  memset(category, 0, 128*sizeof(int));
  memset(category_index, 0, 128*sizeof(int));

#ifdef JZC_PMON_P0
  //PMON_ON(cook);
#endif
  decode_envelope(q, p, quant_index_table);//pmon:10000
#ifdef JZC_PMON_P0
  //PMON_OFF(cook);
#endif
  q->num_vectors = get_bits(&q->gb,p->log2_numvector_size);
#ifdef JZC_PMON_P0
  //PMON_ON(cook);
#endif
  categorize(q, p, quant_index_table, category, category_index);//pmon:160000
#ifdef JZC_PMON_P0
  //PMON_OFF(cook);
#endif
  expand_category(q, category, category_index);

  s32_decode_vectors(q, p, category, quant_index_table, mlt_buffer);
}


/**
 * the actual requantization of the timedomain samples
 *
 * @param q                 pointer to the COOKContext
 * @param buffer            pointer to the timedomain buffer
 * @param gain_index        index for the block multiplier
 * @param gain_index_next   index for the next block multiplier
 */
static void interpolate_float(COOKContext *q, float* buffer,
			      int gain_index, int gain_index_next){
  int i;
  float fc1, fc2;
  fc1 = pow2tab[gain_index+63];

  if(gain_index == gain_index_next){              //static gain
    for(i=0 ; i<q->gain_size_factor ; i++){
      buffer[i]*=fc1;
    }
    return;
  } else {                                        //smooth gain
    fc2 = q->gain_table[11 + (gain_index_next-gain_index)];
    for(i=0 ; i<q->gain_size_factor ; i++){
      buffer[i]*=fc1;
      fc1*=fc2;
    }
    return;
  }
}

static void s32_interpolate_int(COOKContext *q, int* s32_buffer,
			      int gain_index, int gain_index_next){
  int i;
  int ic1, ic2, icsft;
  ic1 = s32_pow2tab[gain_index+63];
  icsft = pow2tab_sft[gain_index+63];

  if(gain_index == gain_index_next){              //static gain
    for(i=0 ; i<q->gain_size_factor ; i++){
#ifdef COOK_MXU_OPT
      //S32MUL_Rx(s32_buffer[i], s32_buffer[i], ic1, icsft);
      s32_buffer[i] = S32MUL_Rx(s32_buffer[i], ic1, icsft);
#else
      s32_buffer[i] = S32MUL_Rx(s32_buffer[i], ic1, icsft);
#endif
    }
    return;
  } else {                                        //smooth gain
    ic2 = s32_gain_table_L20[11 + (gain_index_next-gain_index)];
    for(i=0 ; i<q->gain_size_factor ; i++){
#ifdef COOK_MXU_OPT
      //S32MUL_Rx(s32_buffer[i], s32_buffer[i], ic1, icsft);
      s32_buffer[i] = S32MUL_Rx(s32_buffer[i], ic1, icsft);
#else
      s32_buffer[i] = S32MUL_Rx(s32_buffer[i], ic1, icsft);
#endif
      ic1 = S32MUL_R20(ic1, ic2);
    }
    return;
  }
}

/**
 * Apply transform window, overlap buffers.
 *
 * @param q                 pointer to the COOKContext
 * @param inbuffer          pointer to the mltcoefficients
 * @param gains_ptr         current and previous gains
 * @param previous_buffer   pointer to the previous buffer to be used for overlapping
 */

static void imlt_window_float (COOKContext *q, float *inbuffer,
                               cook_gains *gains_ptr, float *previous_buffer)
{
  const float fc = pow2tab[gains_ptr->previous[0] + 63];
  int i;
  /* The weird thing here, is that the two halves of the time domain
   * buffer are swapped. Also, the newest data, that we save away for
   * next frame, has the wrong sign. Hence the subtraction below.
   * Almost sounds like a complex conjugate/reverse data/FFT effect.
   */

  /* Apply window and overlap */
  for(i = 0; i < q->samples_per_channel; i++){
    inbuffer[i] = inbuffer[i] * fc * q->mlt_window[i] -
      previous_buffer[i] * q->mlt_window[q->samples_per_channel - 1 - i];
  }
}

static inline void s32_imlt_window_int (COOKContext *q, int *inbuffer,
                               cook_gains *gains_ptr, int *previous_buffer)
{
  const int ic = s32_pow2tab[gains_ptr->previous[0] + 63];
  const int ic_sft = pow2tab_sft[gains_ptr->previous[0] + 63];
  int i;

  int r1,r2;
  int tmp;
#if 0
  /* Apply window and overlap */
  for(i = 0; i < q->samples_per_channel; i++){
    inbuffer[i] = inbuffer[i] * ic * q->mlt_window[i] -
      previous_buffer[i] * q->mlt_window[q->samples_per_channel - 1 - i];
  }
#endif

#if 0
  if (ic_sft != 0){
    for (i = 0; i < q->samples_per_channel; i++){
#ifdef COOK_MXU_OPT
      //S32MUL_Rx(r1, inbuffer[i], ic, ic_sft);
      r1 = S32MUL_Rx_MXU(inbuffer[i], ic, ic_sft);
      if (mpFrame == 50)
	printf("1%d\n", r1);
      r1 = S32MUL_R30_MXU(r1, s32_mlt_window[i]);
      r2 = S32MUL_R30_MXU(previous_buffer[i], s32_mlt_window[q->samples_per_channel - 1 - i]);
#else
      r1 = S32MUL_Rx(inbuffer[i], ic, ic_sft);
      r1 = S32MUL_R30(r1, s32_mlt_window[i]);
      r2 = S32MUL_R30(previous_buffer[i], s32_mlt_window[q->samples_per_channel - 1 - i]);
#endif
      //tmp = S32MUL_R30NEW(r1, s32_mlt_window[i]);

      inbuffer[i] = r1 - r2;
    }
  }else{
    for (i = 0; i < q->samples_per_channel; i++){
      r1 = S32MUL_R_MXU(inbuffer[i], ic);
      //if (mpFrame == 50)
      //printf("%d\n", r1);
      r1 = S32MUL_R30_MXU(r1, s32_mlt_window[i]);
      r2 = S32MUL_R30_MXU(previous_buffer[i], s32_mlt_window[q->samples_per_channel - 1 - i]);
      inbuffer[i] = r1 - r2;
      if (mpFrame == 50)
	printf("%d\n", r2);
      //if (mpFrame == 50)
      //printf("%d\n", inbuffer[i]);
    }
  }
#endif

#if 1
  if (ic_sft != 0){
    int r1,r2,r3,r4;
    for (i = 0; i < q->samples_per_channel-3; i+=4){
      S32MUL(xr1,xr2,inbuffer[i], ic);
      S32MUL(xr3,xr4,inbuffer[i+1], ic);
      S32MUL(xr5,xr6,inbuffer[i+2], ic);
      S32MUL(xr7,xr8,inbuffer[i+3], ic);

      S32EXTRV(xr1,xr2,13,31);
      S32EXTRV(xr3,xr4,13,31);
      S32EXTRV(xr5,xr6,13,31);
      S32EXTRV(xr7,xr8,13,31);

      D32SLL(xr1,xr1,xr3,xr3,1);
      D32SLL(xr5,xr5,xr7,xr7,1);
      D32SAR(xr1,xr1,xr3,xr3,1);
      D32SAR(xr5,xr5,xr7,xr7,1);
      
      r1 = S32M2I(xr1);
      r2 = S32M2I(xr3);
      r3 = S32M2I(xr5);
      r4 = S32M2I(xr7);

      S32MUL(xr1,xr2,s32_mlt_window[i], r1);
      S32MUL(xr3,xr4,s32_mlt_window[i+1], r2);
      S32MUL(xr5,xr6,s32_mlt_window[i+2], r3);
      S32MUL(xr7,xr8,s32_mlt_window[i+3], r4);
      
      S32MSUB(xr1,xr2,previous_buffer[i],   s32_mlt_window[q->samples_per_channel - 1 - i]);
      S32MSUB(xr3,xr4,previous_buffer[i+1], s32_mlt_window[q->samples_per_channel - 2 - i]);
      S32MSUB(xr5,xr6,previous_buffer[i+2], s32_mlt_window[q->samples_per_channel - 3 - i]);
      S32MSUB(xr7,xr8,previous_buffer[i+3], s32_mlt_window[q->samples_per_channel - 4 - i]);

      S32EXTRV(xr1,xr2,3,31);
      S32EXTRV(xr3,xr4,3,31);
      S32EXTRV(xr5,xr6,3,31);
      S32EXTRV(xr7,xr8,3,31);

      D32SLL(xr1,xr1,xr3,xr3,1);
      D32SLL(xr5,xr5,xr7,xr7,1);
      D32SAR(xr1,xr1,xr3,xr3,1);
      D32SAR(xr5,xr5,xr7,xr7,1);

      inbuffer[i] =   S32M2I(xr1);
      inbuffer[i+1] = S32M2I(xr3);
      inbuffer[i+2] = S32M2I(xr5);
      inbuffer[i+3] = S32M2I(xr7);
    }

    for (; i < q->samples_per_channel; i++){
      r1 = S32MUL_Rx_MXU(inbuffer[i], ic, ic_sft);
      r1 = S32MUL_R30_MXU(r1, s32_mlt_window[i]);
      r2 = S32MUL_R30_MXU(previous_buffer[i], s32_mlt_window[q->samples_per_channel - 1 - i]);
      inbuffer[i] = r1 - r2;
    }
  }else{
    int r1,r2,r3,r4;
    for (i = 0; i < q->samples_per_channel-3; i+=4){
      S32MUL(xr1,xr2,inbuffer[i], ic);
      S32MUL(xr3,xr4,inbuffer[i+1], ic);
      S32MUL(xr5,xr6,inbuffer[i+2], ic);
      S32MUL(xr7,xr8,inbuffer[i+3], ic);

      r1 = S32M2I(xr2);
      r2 = S32M2I(xr4);
      r3 = S32M2I(xr6);
      r4 = S32M2I(xr8);

      S32MUL(xr1,xr2,s32_mlt_window[i], r1);
      S32MUL(xr3,xr4,s32_mlt_window[i+1], r2);
      S32MUL(xr5,xr6,s32_mlt_window[i+2], r3);
      S32MUL(xr7,xr8,s32_mlt_window[i+3], r4);      

      S32MSUB(xr1,xr2,previous_buffer[i],   s32_mlt_window[q->samples_per_channel - 1 - i]);
      S32MSUB(xr3,xr4,previous_buffer[i+1], s32_mlt_window[q->samples_per_channel - 2 - i]);
      S32MSUB(xr5,xr6,previous_buffer[i+2], s32_mlt_window[q->samples_per_channel - 3 - i]);
      S32MSUB(xr7,xr8,previous_buffer[i+3], s32_mlt_window[q->samples_per_channel - 4 - i]);

      S32EXTRV(xr1,xr2,3,31);
      S32EXTRV(xr3,xr4,3,31);
      S32EXTRV(xr5,xr6,3,31);
      S32EXTRV(xr7,xr8,3,31);

      D32SLL(xr1,xr1,xr3,xr3,1);
      D32SLL(xr5,xr5,xr7,xr7,1);
      D32SAR(xr1,xr1,xr3,xr3,1);
      D32SAR(xr5,xr5,xr7,xr7,1);

      inbuffer[i] =   S32M2I(xr1);
      inbuffer[i+1] = S32M2I(xr3);
      inbuffer[i+2] = S32M2I(xr5);
      inbuffer[i+3] = S32M2I(xr7);
      //if (mpFrame == 50){
      //printf("%d\n", inbuffer[i]);
      //printf("%d\n", inbuffer[i+1]);
      //printf("%d\n", inbuffer[i+2]);
      //printf("%d\n", inbuffer[i+3]);
      //}
    }

    for (; i < q->samples_per_channel; i++){
      r1 = S32MUL_R_MXU(inbuffer[i], ic);
      r1 = S32MUL_R30_MXU(r1, s32_mlt_window[i]);
      r2 = S32MUL_R30_MXU(previous_buffer[i], s32_mlt_window[q->samples_per_channel - 1 - i]);
      inbuffer[i] = r1 - r2;
    }
  }
#endif
}

/**
 * The modulated lapped transform, this takes transform coefficients
 * and transforms them into timedomain samples.
 * Apply transform window, overlap buffers, apply gain profile
 * and buffer management.
 *
 * @param q                 pointer to the COOKContext
 * @param inbuffer          pointer to the mltcoefficients
 * @param gains_ptr         current and previous gains
 * @param previous_buffer   pointer to the previous buffer to be used for overlapping
 */

static void imlt_gain(COOKContext *q, float *inbuffer,
                      cook_gains *gains_ptr, float* previous_buffer)
{
  float *buffer0 = q->mono_mdct_output;
  float *buffer1 = q->mono_mdct_output + q->samples_per_channel;
  int i;
  int tmpbuf[2048];
  float tmpbuf2[2048];

  memset(tmpbuf, 0, sizeof(int)*2048);
  //memset(tmpbuf2, 0.0, sizeof(float)*2048);
  /* Inverse modified discrete cosine transform */
  //ff_imdct_calc(&q->mdct_ctx, q->mono_mdct_output, inbuffer);
  for (i = 0; i < 1024; i++){
    if (1){
      int i1 = s32_decode_buffer_1[i];
      float f1 = inbuffer[i];
      if ((i1 > 0 && f1 < 0.0001) || (i1 < 0 && f1 > 0.0001))
	printf("%d %d %f\n", i, s32_decode_buffer_1[i], inbuffer[i]);
    }
  }

  ff_imdct_calc_c(&q->mdct_ctx, q->mono_mdct_output, inbuffer);//opt

  aac_imdct_calc_c(&q->mdct_ctx, tmpbuf, s32_decode_buffer_1);

  for (i = 0; i < 2048; i++){
    tmpbuf2[i] = (float)(tmpbuf[i]) / 128.0;
    //printf("gain:%d %f %d %f\n", s32_decode_buffer_1[i], inbuffer[i], tmpbuf[i], q->mono_mdct_output[i]);
    q->mono_mdct_output[i] = (float)(tmpbuf[i]) / 128.0;
    //
    //if (mpFrame == 50)
    //printf("%d %f %f\n", i, tmpbuf2[i], q->mono_mdct_output[i]);
  }

  q->imlt_window (q, buffer1, gains_ptr, previous_buffer);

  /* Apply gain profile */
  for (i = 0; i < 8; i++) {
    if (gains_ptr->now[i] || gains_ptr->now[i + 1])
      q->interpolate(q, &buffer1[q->gain_size_factor * i],
		     gains_ptr->now[i], gains_ptr->now[i + 1]);
  }

  /* Save away the current to be previous block. */
  memcpy(previous_buffer, buffer0, sizeof(float)*q->samples_per_channel);
}

static void imlt_gain2(COOKContext *q, float *inbuffer,
                      cook_gains *gains_ptr, float* previous_buffer)
{
  float *buffer0 = q->mono_mdct_output;
  float *buffer1 = q->mono_mdct_output + q->samples_per_channel;
  int i;
  int tmpbuf[2048];

  memset(tmpbuf, 0, sizeof(int)*2048);
  /* Inverse modified discrete cosine transform */
  //ff_imdct_calc(&q->mdct_ctx, q->mono_mdct_output, inbuffer);
  ff_imdct_calc_c(&q->mdct_ctx, q->mono_mdct_output, inbuffer);//opt

  aac_imdct_calc_c(&q->mdct_ctx, tmpbuf, s32_decode_buffer_2);

  for (i = 0; i < 2048; i++){
    //printf("gain:%d %f %d %f\n", s32_decode_buffer_2[i], inbuffer[i], tmpbuf[i], q->mono_mdct_output[i]);
    //q->mono_mdct_output[i] = (float)(tmpbuf[i]) / 1024.0;
    //if (tmpbuf[i])
    //printf("%d %f %f\n", i, q->mono_mdct_output[i]);
  }
			     

  q->imlt_window (q, buffer1, gains_ptr, previous_buffer);

  /* Apply gain profile */
  for (i = 0; i < 8; i++) {
    if (gains_ptr->now[i] || gains_ptr->now[i + 1])
      q->interpolate(q, &buffer1[q->gain_size_factor * i],
		     gains_ptr->now[i], gains_ptr->now[i + 1]);
  }

  /* Save away the current to be previous block. */
  memcpy(previous_buffer, buffer0, sizeof(float)*q->samples_per_channel);
}

static void s32_imlt_gain(COOKContext *q, int *inbuffer,
			  cook_gains *gains_ptr, float* previous_buffer, int *s32_previous_buffer)
{
  float *buffer0 = q->mono_mdct_output;
  float *buffer1 = q->mono_mdct_output + q->samples_per_channel;

  int *s32_buffer0 = q->s32_mono_mdct_output;
  int *s32_buffer1 = q->s32_mono_mdct_output + q->samples_per_channel;

  int i;
  //memset(tmpbuf2, 0.0, sizeof(float)*2048);
  /* Inverse modified discrete cosine transform */
  //ff_imdct_calc(&q->mdct_ctx, q->mono_mdct_output, inbuffer);
  //ff_imdct_calc_c(&q->mdct_ctx, q->mono_mdct_output, inbuffer);//opt

#ifdef JZC_PMON_P0
  //PMON_ON(cook);
#endif
  aac_imdct_calc_c(&q->mdct_ctx, q->s32_mono_mdct_output, inbuffer);
#ifdef JZC_PMON_P0
  //PMON_OFF(cook);
#endif

  //q->imlt_window (q, buffer1, gains_ptr, previous_buffer);
  s32_imlt_window_int(q, s32_buffer1, gains_ptr, s32_previous_buffer);

  /* Apply gain profile */
  for (i = 0; i < 8; i++) {
    if (gains_ptr->now[i] || gains_ptr->now[i + 1]){
      //q->interpolate(q, &buffer1[q->gain_size_factor * i],
      //gains_ptr->now[i], gains_ptr->now[i + 1]);
      s32_interpolate_int(q, &s32_buffer1[q->gain_size_factor * i],
		      gains_ptr->now[i], gains_ptr->now[i + 1]);
    }
  }

#if 0
  for (i = 0; i < 2048; i++){
    //tmpbuf2[i] = (float)(tmpbuf[i]) / 1024.0;
    //printf("gain:%d %f %d %f\n", s32_decode_buffer_1[i], inbuffer[i], tmpbuf[i], q->mono_mdct_output[i]);
    q->mono_mdct_output[i] = (float)(q->s32_mono_mdct_output[i]) / 1024.0;
    //printf("%d %f\n", i, q->mono_mdct_output[i]);
  }
#endif

  /* Save away the current to be previous block. */
  memcpy(s32_previous_buffer, s32_buffer0, sizeof(int)*q->samples_per_channel);
  //memcpy(previous_buffer, buffer0, sizeof(float)*q->samples_per_channel);
}

/**
 * function for getting the jointstereo coupling information
 *
 * @param q                 pointer to the COOKContext
 * @param decouple_tab      decoupling array
 *
 */

static void decouple_info(COOKContext *q, COOKSubpacket *p, int* decouple_tab){
  int length, i;

  if(get_bits1(&q->gb)) {
    if(cplband[p->js_subband_start] > cplband[p->subbands-1]) return;

    length = cplband[p->subbands-1] - cplband[p->js_subband_start] + 1;
    for (i=0 ; i<length ; i++) {
      decouple_tab[cplband[p->js_subband_start] + i] = get_vlc2(&q->gb, p->ccpl.table, p->ccpl.bits, 2);
    }
    return;
  }

  if(cplband[p->js_subband_start] > cplband[p->subbands-1]) return;

  length = cplband[p->subbands-1] - cplband[p->js_subband_start] + 1;
  for (i=0 ; i<length ; i++) {
    decouple_tab[cplband[p->js_subband_start] + i] = get_bits(&q->gb, p->js_vlc_bits);
  }
  return;
}

/*
 * function decouples a pair of signals from a single signal via multiplication.
 *
 * @param q                 pointer to the COOKContext
 * @param subband           index of the current subband
 * @param f1                multiplier for channel 1 extraction
 * @param f2                multiplier for channel 2 extraction
 * @param decode_buffer     input buffer
 * @param mlt_buffer1       pointer to left channel mlt coefficients
 * @param mlt_buffer2       pointer to right channel mlt coefficients
 */
static void decouple_float (COOKContext *q,
                            COOKSubpacket *p,
                            int subband,
                            float f1, float f2,
                            float *decode_buffer,
                            float *mlt_buffer1, float *mlt_buffer2)
{
  int j, tmp_idx;
  for (j=0 ; j<SUBBAND_SIZE ; j++) {
    tmp_idx = ((p->js_subband_start + subband)*SUBBAND_SIZE)+j;
    mlt_buffer1[SUBBAND_SIZE*subband + j] = f1 * decode_buffer[tmp_idx];
    mlt_buffer2[SUBBAND_SIZE*subband + j] = f2 * decode_buffer[tmp_idx];
    //if (mpFrame == 5)
    //printf("decouple float:%f %f\n", mlt_buffer1[SUBBAND_SIZE*subband + j], mlt_buffer2[SUBBAND_SIZE*subband + j]);
  }
}

static void decouple_int(COOKContext *q,
                            COOKSubpacket *p,
                            int subband,
                            int i1, int i2,
                            int *decode_buffer,
                            int *mlt_buffer1, int *mlt_buffer2)
{
  int j, tmp_idx, tmp_idx2;
  //int tmp;
#if 0
  for (j=0 ; j<SUBBAND_SIZE ; j++){
    tmp_idx = ((p->js_subband_start + subband)*SUBBAND_SIZE)+j;
    tmp_idx2 = SUBBAND_SIZE*subband + j;

#ifdef COOK_MXU_OPT
    mlt_buffer1[tmp_idx2] = S32MUL_R30_MXU(i1, decode_buffer[tmp_idx]);
    mlt_buffer2[tmp_idx2] = S32MUL_R30_MXU(i2, decode_buffer[tmp_idx]);    
#else
    mlt_buffer1[tmp_idx2] = S32MUL_R30(i1, decode_buffer[tmp_idx]);
    mlt_buffer2[tmp_idx2] = S32MUL_R30(i2, decode_buffer[tmp_idx]);
#endif
    //if (mpFrame == 230)
    //printf("decouple:%d %d\n", mlt_buffer1[SUBBAND_SIZE*subband + j], mlt_buffer2[SUBBAND_SIZE*subband + j]);
    //tmp = S32MUL_R30NEW(i2, decode_buffer[tmp_idx]);
    //if (tmp != mlt_buffer2[SUBBAND_SIZE*subband + j])
    //printf("decouple:%d %d %d\n", mpFrame, tmp, mlt_buffer2[SUBBAND_SIZE*subband + j]);
  }
#else
  for (j=0 ; j<SUBBAND_SIZE ; j+=4){
    tmp_idx = ((p->js_subband_start + subband)*SUBBAND_SIZE)+j;
    tmp_idx2 = SUBBAND_SIZE*subband + j;

#ifdef JZC_PMON_P0
    //PMON_ON(cook);
#endif
    S32MUL(xr1, xr2, i1, decode_buffer[tmp_idx]);
    S32MUL(xr3, xr4, i1, decode_buffer[tmp_idx+1]);
    S32MUL(xr5, xr6, i1, decode_buffer[tmp_idx+2]);
    S32MUL(xr7, xr8, i1, decode_buffer[tmp_idx+3]);

    S32EXTRV(xr1,xr2,3,31);
    S32EXTRV(xr3,xr4,3,31);
    S32EXTRV(xr5,xr6,3,31);
    S32EXTRV(xr7,xr8,3,31);

    D32SLL(xr1,xr1,xr3,xr3,1);
    D32SAR(xr1,xr1,xr3,xr3,1);
    D32SLL(xr5,xr5,xr7,xr7,1);
    D32SAR(xr5,xr5,xr7,xr7,1);

    mlt_buffer1[tmp_idx2]   = S32M2I(xr1);
    mlt_buffer1[tmp_idx2+1] = S32M2I(xr3);
    mlt_buffer1[tmp_idx2+2] = S32M2I(xr5);
    mlt_buffer1[tmp_idx2+3] = S32M2I(xr7);


    S32MUL(xr1, xr2, i2, decode_buffer[tmp_idx]);
    S32MUL(xr3, xr4, i2, decode_buffer[tmp_idx+1]);
    S32MUL(xr5, xr6, i2, decode_buffer[tmp_idx+2]);
    S32MUL(xr7, xr8, i2, decode_buffer[tmp_idx+3]);

    S32EXTRV(xr1,xr2,3,31);
    S32EXTRV(xr3,xr4,3,31);
    S32EXTRV(xr5,xr6,3,31);
    S32EXTRV(xr7,xr8,3,31);

    D32SLL(xr1,xr1,xr3,xr3,1);
    D32SAR(xr1,xr1,xr3,xr3,1);
    D32SLL(xr5,xr5,xr7,xr7,1);
    D32SAR(xr5,xr5,xr7,xr7,1);

    mlt_buffer2[tmp_idx2]   = S32M2I(xr1);
    mlt_buffer2[tmp_idx2+1] = S32M2I(xr3);
    mlt_buffer2[tmp_idx2+2] = S32M2I(xr5);
    mlt_buffer2[tmp_idx2+3] = S32M2I(xr7);
#ifdef JZC_PMON_P0
    //PMON_OFF(cook);
#endif
    //if (mpFrame == 230){
    //printf("decouple:%d %d\n", mlt_buffer1[tmp_idx], mlt_buffer2[tmp_idx]);
    //printf("decouple:%d %d\n", mlt_buffer1[tmp_idx+1], mlt_buffer2[tmp_idx+1]);
    //printf("decouple:%d %d\n", mlt_buffer1[tmp_idx+2], mlt_buffer2[tmp_idx+2]);
    //printf("decouple:%d %d\n", mlt_buffer1[tmp_idx+3], mlt_buffer2[tmp_idx+3]);
    //}
  }
#endif
}

/**
 * function for decoding joint stereo data
 *
 * @param q                 pointer to the COOKContext
 * @param mlt_buffer1       pointer to left channel mlt coefficients
 * @param mlt_buffer2       pointer to right channel mlt coefficients
 */
static void joint_decode(COOKContext *q, COOKSubpacket *p, float* mlt_buffer1,
                         float* mlt_buffer2) {
  int i,j;
  int decouple_tab[SUBBAND_SIZE];
  float *decode_buffer = q->decode_buffer_0;
  int idx, cpl_tmp;
  float f1,f2;
  const float* cplscale;
  int i1,i2;
  int* s32_cplscale;

  memset(decouple_tab, 0, sizeof(decouple_tab));
  memset(decode_buffer, 0, sizeof(decode_buffer));

  //opt
  memset(s32_decode_buffer_0, 0, sizeof(int));

  /* Make sure the buffers are zeroed out. */
  memset(mlt_buffer1,0, 1024*sizeof(float));
  memset(mlt_buffer2,0, 1024*sizeof(float));
  //opt
  memset(s32_decode_buffer_1, 0, 1024*sizeof(int));
  memset(s32_decode_buffer_2, 0, 1024*sizeof(int));

  decouple_info(q, p, decouple_tab);//s32 no opt

#ifdef JZC_PMON_P0
  //PMON_ON(cook);
#endif
  mono_decode(q, p, decode_buffer);//float opt
#ifdef JZC_PMON_P0
  //PMON_OFF(cook);
#endif

  /* The two channels are stored interleaved in decode_buffer. */
  for (i=0 ; i<p->js_subband_start ; i++) {
    for (j=0 ; j<SUBBAND_SIZE ; j++) {
      mlt_buffer1[i*20+j] = decode_buffer[i*40+j];
      mlt_buffer2[i*20+j] = decode_buffer[i*40+20+j];

      s32_decode_buffer_1[i*20+j] = s32_decode_buffer_0[i*40+j];
      s32_decode_buffer_2[i*20+j] = s32_decode_buffer_0[i*40+20+j];
      //printf("joint:%d %f %d %f\n", s32_decode_buffer_1[i*20+j], mlt_buffer1[i*20+j], s32_decode_buffer_2[i*20+j], mlt_buffer2[i*20+j]);
    }
  }

  /* When we reach js_subband_start (the higher frequencies)
     the coefficients are stored in a coupling scheme. */
  idx = (1 << p->js_vlc_bits) - 1;
  for (i=p->js_subband_start ; i<p->subbands ; i++) {
    cpl_tmp = cplband[i];
    idx -=decouple_tab[cpl_tmp];
    cplscale = q->cplscales[p->js_vlc_bits-2];  //choose decoupler table
    f1 = cplscale[decouple_tab[cpl_tmp]];
    f2 = cplscale[idx-1];
    q->decouple (q, p, i, f1, f2, decode_buffer, mlt_buffer1, mlt_buffer2);
    idx = (1 << p->js_vlc_bits) - 1;
  }

  idx = (1 << p->js_vlc_bits) - 1;
  for (i=p->js_subband_start ; i<p->subbands ; i++) {
    cpl_tmp = cplband[i];
    idx -=decouple_tab[cpl_tmp];
    s32_cplscale = s32_cplscales[p->js_vlc_bits-2];//cplscales is <<30
    i1 = s32_cplscale[decouple_tab[cpl_tmp]];
    i2 = s32_cplscale[idx-1];
    //printf("decouple %d %d %d\n", i, i1, i2);
    decouple_int(q, p, i, i1, i2, s32_decode_buffer_0, s32_decode_buffer_1, s32_decode_buffer_2);
    idx = (1 << p->js_vlc_bits) - 1;
  }

}

static void s32_joint_decode(COOKContext *q, COOKSubpacket *p, int* mlt_buffer1,
                         int* mlt_buffer2) {
  int i,j;
  int decouple_tab[SUBBAND_SIZE];
  int *decode_buffer = s32_decode_buffer_0;
  int idx, cpl_tmp;
  float f1,f2;
  const float* cplscale;
  int i1,i2;
  int* s32_cplscale;

  memset(decouple_tab, 0, sizeof(decouple_tab));
  memset(decode_buffer, 0, sizeof(decode_buffer));

  memset(mlt_buffer1,0, 1024*sizeof(int));
  memset(mlt_buffer2,0, 1024*sizeof(int));

  decouple_info(q, p, decouple_tab);//s32 no opt

#ifdef JZC_PMON_P0
  //PMON_ON(cook);
#endif
  s32_mono_decode(q, p, decode_buffer);//float opt
#ifdef JZC_PMON_P0
  //PMON_OFF(cook);
#endif

  /* The two channels are stored interleaved in decode_buffer. */
  for (i=0 ; i<p->js_subband_start ; i++) {
    for (j=0 ; j<SUBBAND_SIZE ; j++) {
      mlt_buffer1[i*20+j] = decode_buffer[i*40+j];
      mlt_buffer2[i*20+j] = decode_buffer[i*40+20+j];
    }
  }

  idx = (1 << p->js_vlc_bits) - 1;
  for (i=p->js_subband_start ; i<p->subbands ; i++) {
    cpl_tmp = cplband[i];
    idx -=decouple_tab[cpl_tmp];
    s32_cplscale = s32_cplscales[p->js_vlc_bits-2];//cplscales is <<30
    i1 = s32_cplscale[decouple_tab[cpl_tmp]];
    i2 = s32_cplscale[idx-1];
    //printf("decouple %d %d %d\n", i, i1, i2);
#ifdef JZC_PMON_P0
    //PMON_ON(cook);
#endif
    decouple_int(q, p, i, i1, i2, decode_buffer, mlt_buffer1, mlt_buffer2);
#ifdef JZC_PMON_P0
    //PMON_OFF(cook);
#endif
    idx = (1 << p->js_vlc_bits) - 1;
  }

  return;
}

/**
 * First part of subpacket decoding:
 *  decode raw stream bytes and read gain info.
 *
 * @param q                 pointer to the COOKContext
 * @param inbuffer          pointer to raw stream data
 * @param gains_ptr         array of current/prev gain pointers
 */

static inline void
decode_bytes_and_gain(COOKContext *q, COOKSubpacket *p, const uint8_t *inbuffer,
                      cook_gains *gains_ptr)
{
  int offset;

  offset = decode_bytes(inbuffer, q->decoded_bytes_buffer,
			p->bits_per_subpacket/8);
  init_get_bits(&q->gb, q->decoded_bytes_buffer + offset,
		p->bits_per_subpacket);
  decode_gain_info(&q->gb, gains_ptr->now);

  /* Swap current and previous gains */
  FFSWAP(int *, gains_ptr->now, gains_ptr->previous);
}

/**
 * Saturate the output signal to signed 16bit integers.
 *
 * @param q                 pointer to the COOKContext
 * @param chan              channel to saturate
 * @param out               pointer to the output vector
 */
static void
saturate_output_float (COOKContext *q, int chan, int16_t *out)
{
  int j;
  float *output = q->mono_mdct_output + q->samples_per_channel;
  /* Clip and convert floats to 16 bits.
   */
  for (j = 0; j < q->samples_per_channel; j++) {
    out[chan + q->nb_channels * j] =
      av_clip_int16(lrintf(output[j]));
    //if (mpFrame == 500)
    //printf("saturate %f %d\n", output[j], out[chan + q->nb_channels * j]);
  }
}

static void
s32_saturate_output_int(COOKContext *q, int chan, int16_t *out)
{
  int j;
  int *output = q->s32_mono_mdct_output + q->samples_per_channel;
  /* Clip and convert floats to 16 bits.
   */
  for (j = 0; j < q->samples_per_channel; j++) {
    out[chan + q->nb_channels * j] = av_clip_int16((output[j]+512)/128);
    //if (mpFrame == 500)
    //printf("saturate %d %d\n", output[j], out[chan + q->nb_channels * j]);
  }
#if 0
  S32I2M(xr1,512);
  S32I2M(xr2,128);
  for (j = 0; j < q->samples_per_channel-1; j+=2){
    S32I2M(xr7,output[j]);
    S32I2M(xr8,output[j+1]);
    D32ASUM(xr7,xr1,xr1,xr8);
    D32SAR(xr7,xr7,xr8,xr8,7);
  }
#endif
}

/**
 * Final part of subpacket decoding:
 *  Apply modulated lapped transform, gain compensation,
 *  clip and convert to integer.
 *
 * @param q                 pointer to the COOKContext
 * @param decode_buffer     pointer to the mlt coefficients
 * @param gains_ptr         array of current/prev gain pointers
 * @param previous_buffer   pointer to the previous buffer to be used for overlapping
 * @param out               pointer to the output buffer
 * @param chan              0: left or single channel, 1: right channel
 */

static inline void
mlt_compensate_output(COOKContext *q, float *decode_buffer,
                      cook_gains *gains_ptr, float *previous_buffer,
                      int16_t *out, int chan)
{
  imlt_gain(q, decode_buffer, gains_ptr, previous_buffer);
  q->saturate_output (q, chan, out);
}

static inline void
mlt_compensate_output2(COOKContext *q, float *decode_buffer,
                      cook_gains *gains_ptr, float *previous_buffer,
                      int16_t *out, int chan)
{
  imlt_gain2(q, decode_buffer, gains_ptr, previous_buffer);
  q->saturate_output (q, chan, out);
}

static inline void
s32_mlt_compensate_output(COOKContext *q, int *decode_buffer,
			  cook_gains *gains_ptr, float *previous_buffer, int *s32_previous_buffer,
                      int16_t *out, int chan)
{
  s32_imlt_gain(q, decode_buffer, gains_ptr, previous_buffer, s32_previous_buffer);
  //q->saturate_output (q, chan, out);
#ifdef JZC_PMON_P0
  //PMON_ON(cook);
#endif
  s32_saturate_output_int(q, chan, out);
#ifdef JZC_PMON_P0
  //PMON_OFF(cook);
#endif
}

/**
 * Cook subpacket decoding. This function returns one decoded subpacket,
 * usually 1024 samples per channel.
 *
 * @param q                 pointer to the COOKContext
 * @param inbuffer          pointer to the inbuffer
 * @param outbuffer         pointer to the outbuffer
 */
static void decode_subpacket(COOKContext *q, COOKSubpacket* p, const uint8_t *inbuffer, int16_t *outbuffer) {
  int sub_packet_size = p->size;

  memset(q->decode_buffer_1,0,sizeof(q->decode_buffer_1));
  decode_bytes_and_gain(q, p, inbuffer, &p->gains1);

  if (p->joint_stereo) {
#ifdef JZC_PMON_P0
    //PMON_ON(cook);
#endif
#if 0
    joint_decode(q, p, q->decode_buffer_1, q->decode_buffer_2);
#else
    s32_joint_decode(q, p, s32_decode_buffer_1, s32_decode_buffer_2);
#endif
#ifdef JZC_PMON_P0
    //PMON_OFF(cook);
#endif
  } else {
    //printf("no joint_decode\n");
#if 0
    mono_decode(q, p, q->decode_buffer_1);

    if (p->num_channels == 2) {
      decode_bytes_and_gain(q, p, inbuffer + sub_packet_size/2, &p->gains2);
      mono_decode(q, p, q->decode_buffer_2);
    }
#else
    s32_mono_decode(q, p, s32_decode_buffer_1);

    if (p->num_channels == 2) {
      decode_bytes_and_gain(q, p, inbuffer + sub_packet_size/2, &p->gains2);
      s32_mono_decode(q, p, s32_decode_buffer_2);
    }
#endif
  }
#if 0
  mlt_compensate_output(q, q->decode_buffer_1, &p->gains1,
			p->mono_previous_buffer1, outbuffer, p->ch_idx);

  if (p->num_channels == 2) {
    if (p->joint_stereo) {
      mlt_compensate_output2(q, q->decode_buffer_2, &p->gains1,
			    p->mono_previous_buffer2, outbuffer, p->ch_idx + 1);
    } else {
      mlt_compensate_output2(q, q->decode_buffer_2, &p->gains2,
			    p->mono_previous_buffer2, outbuffer, p->ch_idx + 1);
    }
  }
#else
#ifdef JZC_PMON_P0
  //PMON_ON(cook);
#endif
  s32_mlt_compensate_output(q, s32_decode_buffer_1, &p->gains1,
			    p->mono_previous_buffer1, s32_mono_previous_buffer1, outbuffer, p->ch_idx);
#ifdef JZC_PMON_P0
  //PMON_OFF(cook);
#endif

  if (p->num_channels == 2) {
#ifdef JZC_PMON_P0
    //PMON_ON(cook);
#endif
    if (p->joint_stereo) {
      s32_mlt_compensate_output(q, s32_decode_buffer_2, &p->gains1,
				p->mono_previous_buffer2, s32_mono_previous_buffer2, outbuffer, p->ch_idx + 1);
    } else {
      s32_mlt_compensate_output(q, s32_decode_buffer_2, &p->gains2,
				p->mono_previous_buffer2, s32_mono_previous_buffer2, outbuffer, p->ch_idx + 1);
    }
#ifdef JZC_PMON_P0
    //PMON_OFF(cook);
#endif
  }
#endif
}


/**
 * Cook frame decoding
 *
 * @param avctx     pointer to the AVCodecContext
 */

static int cook_decode_frame(AVCodecContext *avctx,
			     void *data, int *data_size,
			     AVPacket *avpkt) {
  const uint8_t *buf = avpkt->data;
  int buf_size = avpkt->size;
  COOKContext *q = avctx->priv_data;
  int i;
  int offset = 0;
  int chidx = 0;

  //mpFrame++;
  //printf("cook_decode_frame start %d\n", mpFrame);
  if (buf_size < avctx->block_align)
    return buf_size;

  /* estimate subpacket sizes */
  q->subpacket[0].size = avctx->block_align;

  for(i=1;i<q->num_subpackets;i++){
    q->subpacket[i].size = 2 * buf[avctx->block_align - q->num_subpackets + i];
    q->subpacket[0].size -= q->subpacket[i].size + 1;
    if (q->subpacket[0].size < 0) {
      av_log(avctx,AV_LOG_DEBUG,"frame subpacket size total > avctx->block_align!\n");
      return -1;
    }
  }

  /* decode supbackets */
  *data_size = 0;
  for(i=0;i<q->num_subpackets;i++){
    q->subpacket[i].bits_per_subpacket = (q->subpacket[i].size*8)>>q->subpacket[i].bits_per_subpdiv;
    q->subpacket[i].ch_idx = chidx;
    av_log(avctx,AV_LOG_DEBUG,"subpacket[%i] size %i js %i %i block_align %i\n",i,q->subpacket[i].size,q->subpacket[i].joint_stereo,offset,avctx->block_align);
#ifdef JZC_PMON_P0
    //PMON_ON(cook);
#endif
    //init_previous_buffer(q->subpacket[i].mono_previous_buffer1, q->subpacket[i].mono_previous_buffer2);
    decode_subpacket(q, &q->subpacket[i], buf + offset, (int16_t*)data);
#ifdef JZC_PMON_P0
    //PMON_OFF(cook);
#endif
    offset += q->subpacket[i].size;
    chidx += q->subpacket[i].num_channels;
    av_log(avctx,AV_LOG_DEBUG,"subpacket[%i] %i %i\n",i,q->subpacket[i].size * 8,get_bits_count(&q->gb));
  }
  *data_size = sizeof(int16_t) * q->nb_channels * q->samples_per_channel;

  /* Discard the first two frames: no valid audio. */
  if (avctx->frame_number < 2) *data_size = 0;

#ifdef JZC_PMON_P0
  //PMON_OFF(cook);
  //printf("PMON: cook:%d\n", cook_pmon_val);
  //PMON_CLEAR(cook);
#endif
  //printf("cook_decode_frame end\n");

  return avctx->block_align;
}

#ifdef COOKDEBUG
static void dump_cook_context(COOKContext *q)
{
  //int i=0;
#define PRINT(a,b) av_log(q->avctx,AV_LOG_ERROR," %s = %d\n", a, b);
  av_log(q->avctx,AV_LOG_ERROR,"COOKextradata\n");
  av_log(q->avctx,AV_LOG_ERROR,"cookversion=%x\n",q->subpacket[0].cookversion);
  if (q->subpacket[0].cookversion > STEREO) {
    PRINT("js_subband_start",q->subpacket[0].js_subband_start);
    PRINT("js_vlc_bits",q->subpacket[0].js_vlc_bits);
  }
  av_log(q->avctx,AV_LOG_ERROR,"COOKContext\n");
  PRINT("nb_channels",q->nb_channels);
  PRINT("bit_rate",q->bit_rate);
  PRINT("sample_rate",q->sample_rate);
  PRINT("samples_per_channel",q->subpacket[0].samples_per_channel);
  PRINT("samples_per_frame",q->subpacket[0].samples_per_frame);
  PRINT("subbands",q->subpacket[0].subbands);
  PRINT("random_state",q->random_state);
  PRINT("js_subband_start",q->subpacket[0].js_subband_start);
  PRINT("log2_numvector_size",q->subpacket[0].log2_numvector_size);
  PRINT("numvector_size",q->subpacket[0].numvector_size);
  PRINT("total_subbands",q->subpacket[0].total_subbands);
}
#endif

static av_cold int cook_count_channels(unsigned int mask){
  int i;
  int channels = 0;
  for(i = 0;i<32;i++){
    if(mask & (1<<i))
      ++channels;
  }
  return channels;
}

/**
 * Cook initialization
 *
 * @param avctx     pointer to the AVCodecContext
 */

static av_cold int cook_decode_init(AVCodecContext *avctx)
{
  COOKContext *q = avctx->priv_data;
  const uint8_t *edata_ptr = avctx->extradata;
  const uint8_t *edata_ptr_end = edata_ptr + avctx->extradata_size;
  int extradata_size = avctx->extradata_size;
  int s = 0;
  unsigned int channel_mask = 0;
  q->avctx = avctx;

  S32I2M(xr16, 0x7);
  mpFrame = 0;
  /* Take care of the codec specific extradata. */
  if (extradata_size <= 0) {
    av_log(avctx,AV_LOG_ERROR,"Necessary extradata missing!\n");
    return -1;
  }
  av_log(avctx,AV_LOG_DEBUG,"codecdata_length=%d\n",avctx->extradata_size);

  /* Take data from the AVCodecContext (RM container). */
  q->sample_rate = avctx->sample_rate;
  q->nb_channels = avctx->channels;
  q->bit_rate = avctx->bit_rate;

  /* Initialize RNG. */
  av_lfg_init(&q->random_state, 0);

  while(edata_ptr < edata_ptr_end){
    /* 8 for mono, 16 for stereo, ? for multichannel
       Swap to right endianness so we don't need to care later on. */
    if (extradata_size >= 8){
      q->subpacket[s].cookversion = bytestream_get_be32(&edata_ptr);
      q->subpacket[s].samples_per_frame =  bytestream_get_be16(&edata_ptr);
      q->subpacket[s].subbands = bytestream_get_be16(&edata_ptr);
      extradata_size -= 8;
    }
    if (avctx->extradata_size >= 8){
      bytestream_get_be32(&edata_ptr);    //Unknown unused
      q->subpacket[s].js_subband_start = bytestream_get_be16(&edata_ptr);
      q->subpacket[s].js_vlc_bits = bytestream_get_be16(&edata_ptr);
      extradata_size -= 8;
    }

    /* Initialize extradata related variables. */
    q->subpacket[s].samples_per_channel = q->subpacket[s].samples_per_frame / q->nb_channels;
    q->subpacket[s].bits_per_subpacket = avctx->block_align * 8;

    /* Initialize default data states. */
    q->subpacket[s].log2_numvector_size = 5;
    q->subpacket[s].total_subbands = q->subpacket[s].subbands;
    q->subpacket[s].num_channels = 1;

    /* Initialize version-dependent variables */

    av_log(avctx,AV_LOG_DEBUG,"subpacket[%i].cookversion=%x\n",s,q->subpacket[s].cookversion);
    q->subpacket[s].joint_stereo = 0;
    switch (q->subpacket[s].cookversion) {
    case MONO:
      if (q->nb_channels != 1) {
	av_log(avctx,AV_LOG_ERROR,"Container channels != 1, report sample!\n");
	return -1;
      }
      av_log(avctx,AV_LOG_DEBUG,"MONO\n");
      break;
    case STEREO:
      if (q->nb_channels != 1) {
	q->subpacket[s].bits_per_subpdiv = 1;
	q->subpacket[s].num_channels = 2;
      }
      av_log(avctx,AV_LOG_DEBUG,"STEREO\n");
      break;
    case JOINT_STEREO:
      if (q->nb_channels != 2) {
	av_log(avctx,AV_LOG_ERROR,"Container channels != 2, report sample!\n");
	return -1;
      }
      av_log(avctx,AV_LOG_DEBUG,"JOINT_STEREO\n");
      if (avctx->extradata_size >= 16){
	q->subpacket[s].total_subbands = q->subpacket[s].subbands + q->subpacket[s].js_subband_start;
	q->subpacket[s].joint_stereo = 1;
	q->subpacket[s].num_channels = 2;
      }
      if (q->subpacket[s].samples_per_channel > 256) {
	q->subpacket[s].log2_numvector_size  = 6;
      }
      if (q->subpacket[s].samples_per_channel > 512) {
	q->subpacket[s].log2_numvector_size  = 7;
      }
      break;
    case MC_COOK:
      av_log(avctx,AV_LOG_DEBUG,"MULTI_CHANNEL\n");
      if(extradata_size >= 4)
	channel_mask |= q->subpacket[s].channel_mask = bytestream_get_be32(&edata_ptr);

      if(cook_count_channels(q->subpacket[s].channel_mask) > 1){
	q->subpacket[s].total_subbands = q->subpacket[s].subbands + q->subpacket[s].js_subband_start;
	q->subpacket[s].joint_stereo = 1;
	q->subpacket[s].num_channels = 2;
	q->subpacket[s].samples_per_channel = q->subpacket[s].samples_per_frame >> 1;

	if (q->subpacket[s].samples_per_channel > 256) {
	  q->subpacket[s].log2_numvector_size  = 6;
	}
	if (q->subpacket[s].samples_per_channel > 512) {
	  q->subpacket[s].log2_numvector_size  = 7;
	}
      }else
	q->subpacket[s].samples_per_channel = q->subpacket[s].samples_per_frame;

      break;
    default:
      av_log(avctx,AV_LOG_ERROR,"Unknown Cook version, report sample!\n");
      return -1;
      break;
    }

    if(s > 1 && q->subpacket[s].samples_per_channel != q->samples_per_channel) {
      av_log(avctx,AV_LOG_ERROR,"different number of samples per channel!\n");
      return -1;
    } else
      q->samples_per_channel = q->subpacket[0].samples_per_channel;


    /* Initialize variable relations */
    q->subpacket[s].numvector_size = (1 << q->subpacket[s].log2_numvector_size);

    /* Try to catch some obviously faulty streams, othervise it might be exploitable */
    if (q->subpacket[s].total_subbands > 53) {
      av_log(avctx,AV_LOG_ERROR,"total_subbands > 53, report sample!\n");
      return -1;
    }

    if ((q->subpacket[s].js_vlc_bits > 6) || (q->subpacket[s].js_vlc_bits < 0)) {
      av_log(avctx,AV_LOG_ERROR,"js_vlc_bits = %d, only >= 0 and <= 6 allowed!\n",q->subpacket[s].js_vlc_bits);
      return -1;
    }

    if (q->subpacket[s].subbands > 50) {
      av_log(avctx,AV_LOG_ERROR,"subbands > 50, report sample!\n");
      return -1;
    }
    q->subpacket[s].gains1.now      = q->subpacket[s].gain_1;
    q->subpacket[s].gains1.previous = q->subpacket[s].gain_2;
    q->subpacket[s].gains2.now      = q->subpacket[s].gain_3;
    q->subpacket[s].gains2.previous = q->subpacket[s].gain_4;

    q->num_subpackets++;
    s++;
    if (s > MAX_SUBPACKETS) {
      av_log(avctx,AV_LOG_ERROR,"Too many subpackets > 5, report file!\n");
      return -1;
    }
  }
  /* Generate tables */
  init_pow2table();
  init_gain_table(q);
  init_cplscales_table(q);

  if (init_cook_vlc_tables(q) != 0)
    return -1;


  if(avctx->block_align >= UINT_MAX/2)
    return -1;

  /* Pad the databuffer with:
     DECODE_BYTES_PAD1 or DECODE_BYTES_PAD2 for decode_bytes(),
     FF_INPUT_BUFFER_PADDING_SIZE, for the bitstreamreader. */
  q->decoded_bytes_buffer =
    av_mallocz(avctx->block_align
	       + DECODE_BYTES_PAD1(avctx->block_align)
	       + FF_INPUT_BUFFER_PADDING_SIZE);
  if (q->decoded_bytes_buffer == NULL)
    return -1;

  /* Initialize transform. */
  if ( init_cook_mlt(q) != 0 )
    return -1;

  /* Initialize COOK signal arithmetic handling */
  if (1) {
    q->scalar_dequant  = scalar_dequant_float;
    q->decouple        = decouple_float;
    q->imlt_window     = imlt_window_float;
    q->interpolate     = interpolate_float;
    q->saturate_output = saturate_output_float;
  }

  /* Try to catch some obviously faulty streams, othervise it might be exploitable */
  if ((q->samples_per_channel == 256) || (q->samples_per_channel == 512) || (q->samples_per_channel == 1024)) {
  } else {
    av_log(avctx,AV_LOG_ERROR,"unknown amount of samples_per_channel = %d, report sample!\n",q->samples_per_channel);
    return -1;
  }

  avctx->sample_fmt = SAMPLE_FMT_S16;
  if (channel_mask)
    avctx->channel_layout = channel_mask;
  else
    avctx->channel_layout = (avctx->channels==2) ? CH_LAYOUT_STEREO : CH_LAYOUT_MONO;

#ifdef COOKDEBUG
  dump_cook_context(q);
#endif
  return 0;
}


AVCodec cook_decoder =
  {
    .name = "cook",
    .type = AVMEDIA_TYPE_AUDIO,
    .id = CODEC_ID_COOK,
    .priv_data_size = sizeof(COOKContext),
    .init = cook_decode_init,
    .close = cook_decode_close,
    .decode = cook_decode_frame,
    .long_name = NULL_IF_CONFIG_SMALL("COOK"),
  };
