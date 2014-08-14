/*
 * WMA compatible decoder
 * Copyright (c) 2002 The FFmpeg Project
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
 * WMA compatible decoder.
 * This decoder handles Microsoft Windows Media Audio data, versions 1 & 2.
 * WMA v1 is identified by audio format 0x160 in Microsoft media files
 * (ASF/AVI/WAV). WMA v2 is identified by audio format 0x161.
 *
 * To use this decoder, a calling application must supply the extra data
 * bytes provided with the WMA data. These are the extra, codec-specific
 * bytes at the end of a WAVEFORMATEX data structure. Transmit these bytes
 * to the decoder using the extradata[_size] fields in AVCodecContext. There
 * should be 4 extra bytes for v1 data and 6 extra bytes for v2 data.
 */

#include "avcodec.h"
#include "wma.h"

#include "fft.h"

#undef NDEBUG
#include <assert.h>

#include "../libjzcommon/jzmedia.h"
#include "aactab.h" 

#define EXPVLCBITS 8
#define EXPMAX ((19+EXPVLCBITS-1)/EXPVLCBITS)

#define HGAINVLCBITS 9
#define HGAINMAX ((13+HGAINVLCBITS-1)/HGAINVLCBITS)

//#define JZC_PMON_P0
//#define STA_CCLK
#ifdef JZC_PMON_P01
#include "libjzcommon/jz4760e_pmon.h"
PMON_CREAT(mpeg4vlc);
#undef printf
#endif

#define WMA_BITS 8
#define WMA_PRECISION (1 << WMA_BITS)
#define WMA_CONST(A) (((A) >= 0) ? ((int32_t)((A)*(WMA_PRECISION)+0.5)) : ((int32_t)((A)*(WMA_PRECISION)-0.5)))

#define WMA_BITS1 16
#define WMA_PRECISION1 (1 << WMA_BITS1)
#define WMA_CONST1(A) (((A) >= 0) ? ((int32_t)((A)*(WMA_PRECISION1)+0.5)) : ((int32_t)((A)*(WMA_PRECISION1)-0.5)))

#define MUL_X(A,B) (int)(((A)*(B)) >> WMA_BITS1)

#define WMA_DIV(a, b)   (int)((((int64_t)(a * WMA_PRECISION)) / (b)))

#define FIX_DIV(a, b)   ((int)((((long long)a)<<WMA_BITS) / (b)))

#define WMA_MULT(a, b)  (int)(((a) * (b)) >> WMA_BITS)

#define WMA_MULT3(a, b, c)  (int)(((int64_t)(((a) * (b)) >> WMA_BITS) * (c)) >> WMA_BITS)

#undef printf
static void wma_lsp_to_curve_init(WMACodecContext *s, int frame_len);

#ifdef TRACE
static void dump_shorts(WMACodecContext *s, const char *name, const short *tab, int n)
{
    int i;

    tprintf(s->avctx, "%s[%d]:\n", name, n);
    for(i=0;i<n;i++) {
        if ((i & 7) == 0)
            tprintf(s->avctx, "%4d: ", i);
        tprintf(s->avctx, " %5d.0", tab[i]);
        if ((i & 7) == 7)
            tprintf(s->avctx, "\n");
    }
}

static void dump_floats(WMACodecContext *s, const char *name, int prec, const float *tab, int n)
{
    int i;

    tprintf(s->avctx, "%s[%d]:\n", name, n);
    for(i=0;i<n;i++) {
        if ((i & 7) == 0)
            tprintf(s->avctx, "%4d: ", i);
        tprintf(s->avctx, " %8.*f", prec, tab[i]);
        if ((i & 7) == 7)
            tprintf(s->avctx, "\n");
    }
    if ((i & 7) != 0)
        tprintf(s->avctx, "\n");
}
#endif

static int wma_decode_init(AVCodecContext * avctx)
{
    S32I2M(xr16,0x7);
    WMACodecContext *s = avctx->priv_data;
    int i, flags2;
    uint8_t *extradata;

    s->avctx = avctx;

    /* extract flag infos */
    flags2 = 0;
    extradata = avctx->extradata;
    if (avctx->codec->id == CODEC_ID_WMAV1 && avctx->extradata_size >= 4) {
        flags2 = AV_RL16(extradata+2);
    } else if (avctx->codec->id == CODEC_ID_WMAV2 && avctx->extradata_size >= 6) {
        flags2 = AV_RL16(extradata+4);
    }
// for(i=0; i<avctx->extradata_size; i++)
//     av_log(NULL, AV_LOG_ERROR, "%02X ", extradata[i]);

    s->use_exp_vlc = flags2 & 0x0001;
    s->use_bit_reservoir = flags2 & 0x0002;
    s->use_variable_block_len = flags2 & 0x0004;

    if(ff_wma_init(avctx, flags2)<0)
        return -1;

    /* init MDCT */
    for(i = 0; i < s->nb_block_sizes; i++)
        aac_mdct_init(&s->mdct_ctx[i], s->frame_len_bits - i + 1, 1, 1.0);

    if (s->use_noise_coding) {
        init_vlc(&s->hgain_vlc, HGAINVLCBITS, sizeof(ff_wma_hgain_huffbits),
                 ff_wma_hgain_huffbits, 1, 1,
                 ff_wma_hgain_huffcodes, 2, 2, 0);
    }

    if (s->use_exp_vlc) {
        init_vlc(&s->exp_vlc, EXPVLCBITS, sizeof(ff_aac_scalefactor_bits), //FIXME move out of context
                 ff_aac_scalefactor_bits, 1, 1,
                 ff_aac_scalefactor_code, 4, 4, 0);
    } else {
        wma_lsp_to_curve_init(s, s->frame_len);
    }

    avctx->sample_fmt = SAMPLE_FMT_S16;
    return 0;
}

/**
 * compute x^-0.25 with an exponent and mantissa table. We use linear
 * interpolation to reduce the mantissa table size at a small speed
 * expense (linear interpolation approximately doubles the number of
 * bits of precision).
 */
static inline float pow_m1_4(WMACodecContext *s, float x)
{
    union {
        float f;
        unsigned int v;
    } u, t;
    unsigned int e, m;
    float a, b;

    u.f = x;
    e = u.v >> 23;
    m = (u.v >> (23 - LSP_POW_BITS)) & ((1 << LSP_POW_BITS) - 1);
    /* build interpolation scale: 1 <= t < 2. */
    t.v = ((u.v << LSP_POW_BITS) & ((1 << 23) - 1)) | (127 << 23);
    a = s->lsp_pow_m_table1[m];
    b = s->lsp_pow_m_table2[m];
    return s->lsp_pow_e_table[e] * (a + b * t.f);
}

static void wma_lsp_to_curve_init(WMACodecContext *s, int frame_len)
{
    float wdel, a, b;
    int i, e, m;

    wdel = M_PI / frame_len;
    for(i=0;i<frame_len;i++)
        s->lsp_cos_table[i] = 2.0f * cos(wdel * i);

    /* tables for x^-0.25 computation */
    for(i=0;i<256;i++) {
        e = i - 126;
        s->lsp_pow_e_table[i] = pow(2.0, e * -0.25);
    }

    /* NOTE: these two tables are needed to avoid two operations in
       pow_m1_4 */
    b = 1.0;
    for(i=(1 << LSP_POW_BITS) - 1;i>=0;i--) {
        m = (1 << LSP_POW_BITS) + i;
        a = (float)m * (0.5 / (1 << LSP_POW_BITS));
        a = pow(a, -0.25);
        s->lsp_pow_m_table1[i] = 2 * a - b;
        s->lsp_pow_m_table2[i] = b - a;
        b = a;
    }
#if 0
    for(i=1;i<20;i++) {
        float v, r1, r2;
        v = 5.0 / i;
        r1 = pow_m1_4(s, v);
        r2 = pow(v,-0.25);
        printf("%f^-0.25=%f e=%f\n", v, r1, r2 - r1);
    }
#endif
}

/**
 * NOTE: We use the same code as Vorbis here
 * @todo optimize it further with SSE/3Dnow
 */
static void wma_lsp_to_curve(WMACodecContext *s,
                             uint32_t *out, int32_t *val_max_ptr,
                             int n, float *lsp)
{
    int i, j;
    float p, q, w, v, val_max;

    val_max = 0;
    for(i=0;i<n;i++) {
        p = 0.5f;
        q = 0.5f;
        w = s->lsp_cos_table[i];
        for(j=1;j<NB_LSP_COEFS;j+=2){
            q *= w - lsp[j - 1];
            p *= w - lsp[j];
        }
        p *= p * (2.0f - w);
        q *= q * (2.0f + w);
        v = p + q;
        v = pow_m1_4(s, v);
        if (v > val_max)
            val_max = v;
        out[i] = WMA_CONST1(v);
    }
    *val_max_ptr = WMA_CONST1(val_max);
}

/**
 * decode exponents coded with LSP coefficients (same idea as Vorbis)
 */
static void decode_exp_lsp(WMACodecContext *s, int ch)
{
    float lsp_coefs[NB_LSP_COEFS];
    int val, i;

    for(i = 0; i < NB_LSP_COEFS; i++) {
        if (i == 0 || i >= 8)
            val = get_bits(&s->gb, 3);
        else
            val = get_bits(&s->gb, 4);
        lsp_coefs[i] = ff_wma_lsp_codebook[i][val];
    }

    wma_lsp_to_curve(s, s->exponents[ch], &s->max_exponent[ch],
                     s->block_len, lsp_coefs);
}

static const int32_t pow_table[] =
  {
    256,287,322,361,405,455,510,573,
    643,721,809,908,1019,1143,1283,1439,
    1615,1812,2033,2281,2560,2872,3222,3616,
    4057,4552,5107,5731,6430,7215,8095,9083,
    10191,11435,12830,14395,16152,18123,20334,22816,
    25600,28723,32228,36160,40573,45523,51078,57311,
    64304,72150,80954,90832,101915,114350,128303,143959,
    161525,181234,203348,228160,256000,287236,322284,361609,
    405732,455239,510787,573112,643042,721506,809543,908322,
    1019154,1143509,1283039,1439593,1615250,1812341,2033480,2281602,
    2560000,2872367,3222849,3616096,4057326,4552395,5107871,5731126,
    6430429,7215060,8095430,9083222,10191543,11435099,12830393,14395937,
    16152508,18123412,20334802,22816024,25600000,28723672,32228490,36160961,
    40573265,45523952,51078715,57311261,64304292,72150603,80954308,90832227,
    101915435,114350999,128303931,143959379,161525080,181234120,203348028,228160240,
    256000000,287236724,322284905,361609611,405732657,455239528,510787152,573112611,
    643042926,721506030,809543081,908322276,1019154356,1143509995,1283039318,1439593792,
    1615250801,1812341208,2033480280
  };

static const int32_t pow_tab[] = {
0, 0, 
0, 0, 
0, 0, 
0, 0, 
0, 0, 
0, 0, 
0, 0, 
0, 0, 
0, 1, 
1, 1, 
1, 1, 
1, 1, 
1, 2, 
2, 2, 
3, 3, 
3, 4, 
5, 5, 
6, 7, 
8, 9, 
11, 12, 
14, 17, 
19, 22, 
26, 30, 
34, 39, 
46, 53, 
61, 70, 
81, 93, 
108, 125, 
144, 166, 
192, 222, 
256, 296, 
341, 394, 
455, 526, 
607, 701, 
810, 935, 
1080, 1247, 
1440, 1662, 
1920, 2217, 
2560, 2956, 
3414, 3942, 
4552, 5257, 
6071, 7010, 
8095, 9348, 
10795, 12466, 
14396, 16624, 
19197, 22169, 
25600, 29562, 
34138, 39422, 
45524, 52570, 
60707, 70104, 
80954, 93485, 
107954, 124664, 
143959, 166242, 
191973, 221687, 
256000, 295624, 
341382, 394221, 
455240, 525702, 
607072, 701035, 
809543, 934846, 
1079543, 1246637, 
1439594, 1662417, 
1919729, 2216869, 
2560000, 2956242, 
3413815, 3942212, 
4552396, 5257024, 
6070717, 7010355, 
8095431, 9348458, 
10795430, 12466369, 
14395938, 16624170, 
19197292, 22168686, 
25600000, 29562418, 
34138148, 39422120, 
45523952, 52570240, 
60707168, 70103544, 
80954312, 93484576, 
107954304, 124663688, 
143959376, 166241696, 
191972912, 221686864
};

/**
 * decode exponents coded with VLC codes
 */
static int decode_exp_vlc(WMACodecContext *s, int ch)
{
    int last_exp, n, code;
    const uint16_t *ptr;
    int32_t v, max_scale;
    int32_t *q, *q_end, iv;
    const uint32_t *ptab = pow_tab + 60;
    //const uint32_t *iptab = (const uint32_t*)ptab;

    ptr = s->exponent_bands[s->frame_len_bits - s->block_len_bits];
    q = s->exponents[ch];
    q_end = q + s->block_len;
    max_scale = 0;////
    if (s->version == 1) {
        last_exp = get_bits(&s->gb, 5) + 10;
        v = ptab[last_exp];
        iv = ptab[last_exp];
        max_scale = v;
        n = *ptr++;
        switch (n & 3) do {
        case 0: *q++ = iv;
        case 3: *q++ = iv;
        case 2: *q++ = iv;
        case 1: *q++ = iv;
        } while ((n -= 4) > 0);
    }else
        last_exp = 36;

    while (q < q_end) {
        code = get_vlc2(&s->gb, s->exp_vlc.table, EXPVLCBITS, EXPMAX);
        if (code < 0){
            av_log(s->avctx, AV_LOG_ERROR, "Exponent vlc invalid\n");
            return -1;
        }
        /* NOTE: this offset is the same as MPEG4 AAC ! */
        last_exp += code - 60;
        if ((unsigned)last_exp + 60 > FF_ARRAY_ELEMS(pow_tab)) {
            av_log(s->avctx, AV_LOG_ERROR, "Exponent out of range: %d\n",
                   last_exp);
            return -1;
        }
        v = ptab[last_exp];
        iv = ptab[last_exp];
        if (v > max_scale)
            max_scale = v;
        n = *ptr++;
        switch (n & 3) do {
        case 0: *q++ = iv;
        case 3: *q++ = iv;
        case 2: *q++ = iv;
        case 1: *q++ = iv;
        } while ((n -= 4) > 0);
    }
    s->max_exponent[ch] = max_scale;
    return 0;
}

static void wma_vector_fmul_add_c(int16_t *dst, const int32_t *src0, const int32_t *src1, const int16_t *src2, int len){
#if 0
    int i;
    for(i=0; i<len; i++)
      {
        dst[i] = (MUL_F(src0[i] , src1[i]) + src2[i]);
      }
#else
    int i;
    src2-=1;
    dst-=1; 
    for(i=0; i<len; i+=4)
      {
#if 0
        S32MUL(xr1,xr2,src0[i],src1[i]); 
        S32MUL(xr3,xr4,src0[i+1],src1[i+1]); 
        S32MUL(xr5,xr6,src0[i+2],src1[i+2]); 
        S32MUL(xr7,xr8,src0[i+3],src1[i+3]); 
        S32EXTRV(xr1,xr2,2,31);
        S32EXTRV(xr3,xr4,2,31);         
        S32EXTRV(xr5,xr6,2,31);       
        S32EXTRV(xr7,xr8,2,31);   
        D32SLL(xr9,xr1,xr3,xr10,1);
        D32SLL(xr11,xr5,xr7,xr12,1);
        S16LDI(xr1,src2,2,0);
        S16LDI(xr2,src2,2,0);            ;            
        S16LDI(xr3,src2,2,0);            
        S16LDI(xr4,src2,2,0);           
        D32ADD_AA(xr5,xr1,xr9,xr0); 
        D32ADD_AA(xr6,xr2,xr10,xr0);   
        D32ADD_AA(xr7,xr3,xr11,xr0);    
        D32ADD_AA(xr8,xr4,xr12,xr0);   
        S16SDI(xr5,dst,2,0);
        S16SDI(xr6,dst,2,0);
        S16SDI(xr7,dst,2,0);
        S16SDI(xr8,dst,2,0);       
#else
        S32MUL(xr1,xr2,src0[i],src1[i]); 
        S32MUL(xr3,xr4,src0[i+1],src1[i+1]); 
        S32MUL(xr5,xr6,src0[i+2],src1[i+2]); 
        S32MUL(xr7,xr8,src0[i+3],src1[i+3]); 
        S32EXTRV(xr1,xr2,18,16);
        S32EXTRV(xr3,xr4,18,16);         
        S32EXTRV(xr5,xr6,18,16);       
        S32EXTRV(xr7,xr8,18,16);   
        S16LDI(xr9,src2,2,0);
        S16LDI(xr10,src2,2,0);            ;            
        S16LDI(xr11,src2,2,0);            
        S16LDI(xr12,src2,2,0);           
        D32ADD_AA(xr2,xr1,xr9,xr0); 
        D32ADD_AA(xr4,xr3,xr10,xr0);   
        D32ADD_AA(xr6,xr5,xr11,xr0);    
        D32ADD_AA(xr8,xr7,xr12,xr0);   
        S16SDI(xr2,dst,2,0);
        S16SDI(xr4,dst,2,0);
        S16SDI(xr6,dst,2,0);
        S16SDI(xr8,dst,2,0);       
#endif
      }
#endif
}

static void wma_vector_fmul_reverse_c(int16_t *dst, const int32_t *src0, const int32_t *src1, int len){
#if 0
    int i;
    src1 += len-1;
    for(i=0; i<len; i++)
      {
        dst[i] = MUL_F(src0[i] , src1[-i]);
      }
#else
    int i;
    src1 += len-1;
    dst-=1; 
    for(i=0; i<len; i+=4)
      {
#if 0
        S32MUL(xr1,xr2,src0[i],src1[-i]);
        S32MUL(xr3,xr4,src0[i+1],src1[-i-1]);
        S32MUL(xr5,xr6,src0[i+2],src1[-i-2]);
        S32MUL(xr7,xr8,src0[i+3],src1[-i-3]);
        S32EXTRV(xr1,xr2,2,31);
        S32EXTRV(xr3,xr4,2,31);
        S32EXTRV(xr5,xr6,2,31);
        S32EXTRV(xr7,xr8,2,31);
        D32SLL(xr9,xr1,xr3,xr10,1);
        D32SLL(xr11,xr5,xr7,xr12,1);
        S16SDI(xr9,dst,2,0);
        S16SDI(xr10,dst,2,0);  
        S16SDI(xr11,dst,2,0);
        S16SDI(xr12,dst,2,0);     
#else
        S32MUL(xr1,xr2,src0[i],src1[-i]);
        S32MUL(xr3,xr4,src0[i+1],src1[-i-1]);
        S32MUL(xr5,xr6,src0[i+2],src1[-i-2]);
        S32MUL(xr7,xr8,src0[i+3],src1[-i-3]);
        S32EXTRV(xr1,xr2,18,16);
        S32EXTRV(xr3,xr4,18,16);
        S32EXTRV(xr5,xr6,18,16);
        S32EXTRV(xr7,xr8,18,16);
        S16SDI(xr1,dst,2,0);
        S16SDI(xr3,dst,2,0);  
        S16SDI(xr5,dst,2,0);
        S16SDI(xr7,dst,2,0);     
#endif
      } 
#endif
}

#define MXU_MEMSET(addr)			\
    do {					\
      int32_t mxu_i;				\
      int32_t local = (int32_t)(addr)-4;	\
      for (mxu_i=0; mxu_i < (n>>4); mxu_i++)	\
	{					\
	  S32SDI(xr0,local,4);			\
	  S32SDI(xr0,local,4);			\
	  S32SDI(xr0,local,4);			\
	  S32SDI(xr0,local,4);			\
	  S32SDI(xr0,local,4);			\
	  S32SDI(xr0,local,4);			\
	  S32SDI(xr0,local,4);                \    
          S32SDI(xr0,local,4);		     \
}                                     \
}while(0)

/**
 * Apply MDCT window and add into output.
 *
 * We ensure that when the windows overlap their squared sum
 * is always 1 (MDCT reconstruction rule).
 */
static void wma_window(WMACodecContext *s, int16_t *out)
{
    int32_t *in = s->output;
    int block_len, bsize, n;

    /* left part */
    if (s->block_len_bits <= s->prev_block_len_bits) {
        block_len = s->block_len;
        bsize = s->frame_len_bits - s->block_len_bits;

        wma_vector_fmul_add_c(out, in, s->windows[bsize],
                               out, block_len);

    } else {
        block_len = 1 << s->prev_block_len_bits;
        n = (s->block_len - block_len) / 2;
        bsize = s->frame_len_bits - s->prev_block_len_bits;

        wma_vector_fmul_add_c(out+n, in+n, s->windows[bsize],
                               out+n, block_len);
#if 0
        memcpy(out+n+block_len, in+n+block_len, n*sizeof(int32_t));
#else
        int k; 
        in+=(n+block_len);
        out+=(n+block_len);
        for(k=0;k<n;k++){
            out[k]=in[k];  
	}
        in-=(n+block_len);
        out-=(n+block_len);
#endif
    }

    out += s->block_len;
    in += s->block_len;
    /* right part */
    if (s->block_len_bits <= s->next_block_len_bits) {
        block_len = s->block_len;
        bsize = s->frame_len_bits - s->block_len_bits;

        wma_vector_fmul_reverse_c(out, in, s->windows[bsize], block_len);

    } else {
        block_len = 1 << s->next_block_len_bits;
        n = (s->block_len - block_len) / 2;
        bsize = s->frame_len_bits - s->next_block_len_bits;
#if 0
        memcpy(out, in, n*sizeof(int32_t));
#else
       int kk; 
       for(kk=0;kk<n;kk++){
           out[kk]=in[kk];
       }
#endif
        wma_vector_fmul_reverse_c(out+n, in+n, s->windows[bsize], block_len);
        //memset(out+n+block_len, 0, n*sizeof(int16_t));
        MXU_MEMSET(out+n+block_len);
    }
}


static void wma_butterflies_float_c(int32_t *restrict v1, int32_t *restrict v2,
                                int len)
{
#if 0
    int i;
    for (i = 0; i < len; i++) {
        int32_t t = v1[i] - v2[i];
        v1[i] += v2[i];
        v2[i] = t;
    }
#else
    int i;
    v1-=1;
    v2-=1;
    for (i = 0; i < len/2; i++) {
      S32LDI(xr1,v1,4); 
      S32LDI(xr2,v2,4);
      S32LDI(xr3,v1,4);
      S32LDI(xr4,v2,4); 
      D32ADD_SA(xr5,xr1,xr2,xr6);
      D32ADD_SA(xr7,xr3,xr4,xr8);
      S32STD(xr6,v1,-4);  
      S32STD(xr5,v2,-4); 
      S32STD(xr8,v1,0);  
      S32STD(xr7,v2,0);  
    }
#endif
}

/**
 * @return 0 if OK. 1 if last block of frame. return -1 if
 * unrecorrable error.
 */
static int wma_decode_block(WMACodecContext *s)
{
    int n, v, a, ch, bsize;
    int coef_nb_bits, total_gain;
    int nb_coefs[MAX_CHANNELS];
    float mdct_norm;
    int32_t mdct_norm_fix; 
    //mp_msg(NULL, NULL, "decode_start\n");
#ifdef TRACE
    tprintf(s->avctx, "***decode_block: %d:%d\n", s->frame_count - 1, s->block_num);
#endif

    /* compute current block length */
    if (s->use_variable_block_len) {
        n = av_log2(s->nb_block_sizes - 1) + 1;

        if (s->reset_block_lengths) {
            s->reset_block_lengths = 0;
            v = get_bits(&s->gb, n);
            if (v >= s->nb_block_sizes){
                av_log(s->avctx, AV_LOG_ERROR, "prev_block_len_bits %d out of range\n", s->frame_len_bits - v);
                return -1;
            }
            s->prev_block_len_bits = s->frame_len_bits - v;
            v = get_bits(&s->gb, n);
            if (v >= s->nb_block_sizes){
                av_log(s->avctx, AV_LOG_ERROR, "block_len_bits %d out of range\n", s->frame_len_bits - v);
                return -1;
            }
            s->block_len_bits = s->frame_len_bits - v;
        } else {
            /* update block lengths */
            s->prev_block_len_bits = s->block_len_bits;
            s->block_len_bits = s->next_block_len_bits;
        }
        v = get_bits(&s->gb, n);
        if (v >= s->nb_block_sizes){
            av_log(s->avctx, AV_LOG_ERROR, "next_block_len_bits %d out of range\n", s->frame_len_bits - v);
            return -1;
        }
        s->next_block_len_bits = s->frame_len_bits - v;
    } else {
        /* fixed block len */
        s->next_block_len_bits = s->frame_len_bits;
        s->prev_block_len_bits = s->frame_len_bits;
        s->block_len_bits = s->frame_len_bits;
    }

    /* now check if the block length is coherent with the frame length */
    s->block_len = 1 << s->block_len_bits;
    if ((s->block_pos + s->block_len) > s->frame_len){
        av_log(s->avctx, AV_LOG_ERROR, "frame_len overflow\n");
        return -1;
    }

    if (s->nb_channels == 2) {
        s->ms_stereo = get_bits1(&s->gb);
    }
    v = 0;
    for(ch = 0; ch < s->nb_channels; ch++) {
        a = get_bits1(&s->gb);
        s->channel_coded[ch] = a;
        v |= a;
    }

    bsize = s->frame_len_bits - s->block_len_bits;

    /* if no channel coded, no need to go further */
    /* XXX: fix potential framing problems */
    if (!v)
        goto next;

    /* read total gain and extract corresponding number of bits for
       coef escape coding */
    total_gain = 1;
    for(;;) {
        a = get_bits(&s->gb, 7);
        total_gain += a;
        if (a != 127)
            break;
    }

    coef_nb_bits= ff_wma_total_gain_to_bits(total_gain);

    /* compute number of coefficients */
    n = s->coefs_end[bsize] - s->coefs_start;
    for(ch = 0; ch < s->nb_channels; ch++)
        nb_coefs[ch] = n;

    /* complex coding */
    if (s->use_noise_coding) {
        for(ch = 0; ch < s->nb_channels; ch++) {
            if (s->channel_coded[ch]) {
                int i, n, a;
                n = s->exponent_high_sizes[bsize];
                for(i=0;i<n;i++) {
                    a = get_bits1(&s->gb);
                    s->high_band_coded[ch][i] = a;
                    /* if noise coding, the coefficients are not transmitted */
                    if (a)
                        nb_coefs[ch] -= s->exponent_high_bands[bsize][i];
                }
            }
        }
        for(ch = 0; ch < s->nb_channels; ch++) {
            if (s->channel_coded[ch]) {
                int i, n, val, code;

                n = s->exponent_high_sizes[bsize];
                val = (int)0x80000000;
                for(i=0;i<n;i++) {
                    if (s->high_band_coded[ch][i]) {
                        if (val == (int)0x80000000) {
                            val = get_bits(&s->gb, 7) - 19;
                        } else {
                            code = get_vlc2(&s->gb, s->hgain_vlc.table, HGAINVLCBITS, HGAINMAX);
                            if (code < 0){
                                av_log(s->avctx, AV_LOG_ERROR, "hgain vlc invalid\n");
                                return -1;
                            }
                            val += code - 18;
                        }
                        s->high_band_values[ch][i] = val;
                    }
                }
            }
        }
    }

    /* exponents can be reused in short blocks. */
    if ((s->block_len_bits == s->frame_len_bits) ||
        get_bits1(&s->gb)) {
        for(ch = 0; ch < s->nb_channels; ch++) {
            if (s->channel_coded[ch]) {
                if (s->use_exp_vlc) {
                    if (decode_exp_vlc(s, ch) < 0)
                        return -1;
                } else {
                    decode_exp_lsp(s, ch);
                }
                s->exponents_bsize[ch] = bsize;
            }
        }
    }

    /* parse spectral coefficients : just RLE encoding */
    for(ch = 0; ch < s->nb_channels; ch++) {
        if (s->channel_coded[ch]) {
            int tindex;
            WMACoef* ptr = &s->coefs1[ch][0];

            /* special VLC tables are used for ms stereo because
               there is potentially less energy there */
            tindex = (ch == 1 && s->ms_stereo);
            memset(ptr, 0, s->block_len * sizeof(WMACoef));
            ff_wma_run_level_decode(s->avctx, &s->gb, &s->coef_vlc[tindex],
                  s->level_table[tindex], s->run_table[tindex],
                  0, ptr, 0, nb_coefs[ch],
                  s->block_len, s->frame_len_bits, coef_nb_bits);            
        }
        if (s->version == 1 && s->nb_channels >= 2) {
            align_get_bits(&s->gb);
        }
    }
    /* normalize */
    {
        int n4 = s->block_len / 2;
        mdct_norm = 1.0 / (float)n4;
        if (s->version == 1) {
            mdct_norm *= sqrt(n4);
        }
    }
    mdct_norm_fix = WMA_CONST1(mdct_norm);  
    /* finally compute the MDCT coefficients */
    for(ch = 0; ch < s->nb_channels; ch++) {
        if (s->channel_coded[ch]) {
            WMACoef *coefs1;
            int32_t *coefs, *exponents, mult, mult1, noise;
            int i, j, n, n1, last_high_band, esize;
            int32_t exp_power[HIGH_BAND_MAX_SIZE];
               
            coefs1 = s->coefs1[ch];
            exponents = s->exponents[ch];
            esize = s->exponents_bsize[ch];
            if(s->max_exponent[ch] == 0){
	         mult = 0;
	    }
            else{
	         mult=FIX_DIV(pow_table[total_gain], s->max_exponent[ch]);
	    }
            mult = MUL_X(mult, mdct_norm_fix);
            coefs = s->coefs[ch];
            if (s->use_noise_coding) {
                mult1 = mult;
                /* very low freqs : noise */
#if 0
                for(i = 0;i < s->coefs_start; i++) {
                    *coefs++ = WMA_MULT3(s->noise_table[s->noise_index],exponents[i<<bsize>>esize],mult1);
                    s->noise_index = (s->noise_index + 1) & (NOISE_TAB_SIZE - 1);
                }
#endif
                n1 = s->exponent_high_sizes[bsize];

                /* compute power of high bands */
                exponents = s->exponents[ch] +
                    (s->high_band_start[bsize]<<bsize>>esize);
                //last_high_band = 0; /* avoid warning */
                for(j=0;j<n1;j++) {
                    n = s->exponent_high_bands[s->frame_len_bits -
                                              s->block_len_bits][j];
#if 0
                    if (s->high_band_coded[ch][j]) {
                        uint32_t e2, v;
                        e2 = 0;
                        for(i = 0;i < n; i++) {
                            v = exponents[i<<bsize>>esize];
                            e2 += WMA_MULT(v , v);
                        }
                        exp_power[j] = e2 / n;
                        last_high_band = j;
                        tprintf(s->avctx, "%d: power=%f (%d)\n", j, exp_power[j], n);
                    }
#endif
                    exponents += n<<bsize>>esize;
                }

                /* main freqs and high freqs */
                exponents = s->exponents[ch] + (s->coefs_start<<bsize>>esize);
                for(j=-1;j<n1;j++) {
                    if (j < 0) {
                        n = s->high_band_start[bsize] -
                            s->coefs_start;
                    } else {
                        n = s->exponent_high_bands[s->frame_len_bits -
                                                  s->block_len_bits][j];
                    }
                    if (j >= 0 && s->high_band_coded[ch][j]) {
                        /* use noise with specified power */
                        //mult1 = sqrt(exp_power[j] / exp_power[last_high_band]);
		        //mult1 = sqrt_int(WMA_DIV (exp_power[j],exp_power[last_high_band])); 
                        /* XXX: use a table */
                        //mult1 = mult1 * (int)pow(10, s->high_band_values[ch][j] * 0.05);
#if 0
                        mult1 = WMA_DIV(mult1,WMA_MULT(s->max_exponent[ch],s->noise_mult));
#else
                        mult1 = WMA_DIV(mult1,s->max_exponent[ch]);
#endif
                        mult1 = MUL_X(mult1,mdct_norm_fix); 
                        for(i = 0;i < n; i++) {
#if 1
			  //noise = s->noise_table[s->noise_index];
			  //s->noise_index = (s->noise_index + 1) & (NOISE_TAB_SIZE - 1);
                            *coefs++ = WMA_MULT(exponents[i<<bsize>>esize] , mult1)>>9; 
#else
			 //*coefs++ = WMA_MULT(exponents[i<<bsize>>esize] , mult1)>>9; 
                         coefs--; 
                         for(i = 0;i < n; i++) {
	                    S32MUL(xr2,xr1,exponents[i<<bsize>>esize],mult1);
                            S32ALNI(xr9,xr2,xr1,2);
	                    S32SDI(xr9,coefs,4);
                         }
#endif
                        }
                        exponents += n<<bsize>>esize;
                    } else { 
                        /* coded values + small noise */
                        for(i = 0;i < n; i++) {
#if 0
                            noise = s->noise_table[s->noise_index];
                            s->noise_index = (s->noise_index + 1) & (NOISE_TAB_SIZE - 1);
                           *coefs++ = WMA_MULT3(((*coefs1++) * WMA_PRECISION),exponents[i<<bsize>>esize],mult)>>9;
#else
                           *coefs++ = (*coefs1++) * (WMA_MULT(exponents[i<<bsize>>esize],mult))>>9;
#endif                      
                        }
                        exponents += n<<bsize>>esize;
                    }
                }
#if 0
                /* very high freqs : noise */
                n = s->block_len - s->coefs_end[bsize];
                mult1 = WMA_MULT(mult , exponents[((-1<<bsize))>>esize]);
                for(i = 0; i < n; i++) {
                    *coefs++ = WMA_MULT(s->noise_table[s->noise_index],mult1)>>8;
                    s->noise_index = (s->noise_index + 1) & (NOISE_TAB_SIZE - 1);
                }
#endif
            } else {
                /* XXX: optimize more */
                for(i = 0;i < s->coefs_start; i++)
                    *coefs++ = 0;
                n = nb_coefs[ch];
#if 1
                for(i = 0;i < n; i++) {
		  *coefs++ = (coefs1[i] * WMA_MULT(exponents[i<<bsize>>esize] , mult))>>9;  
                }
#else 
                coefs--; 
                for(i = 0;i < n; i++) {
	          S32MUL(xr2,xr1,coefs1[i]*exponents[i<<bsize>>esize],mult);
                  S32ALNI(xr9,xr2,xr1,2);
	          S32SDI(xr9,coefs,4);
                }
#endif
                n = s->block_len - s->coefs_end[bsize];
                for(i = 0;i < n; i++)
                    *coefs++ = 0;
            }
        }
    }

#ifdef TRACE
    for(ch = 0; ch < s->nb_channels; ch++) {
        if (s->channel_coded[ch]) {
            dump_floats(s, "exponents", 3, s->exponents[ch], s->block_len);
            dump_floats(s, "coefs", 1, s->coefs[ch], s->block_len);
        }
    }
#endif
    if (s->ms_stereo && s->channel_coded[1]) {
        /* nominal case for ms stereo: we do it before mdct */
        /* no need to optimize this case because it should almost
           never happen */
        if (!s->channel_coded[0]) {
            tprintf(s->avctx, "rare ms-stereo case happened\n");
            memset(s->coefs[0], 0, sizeof(int32_t) * s->block_len);
            s->channel_coded[0] = 1;
        }
        wma_butterflies_float_c(s->coefs[0], s->coefs[1], s->block_len);
    }
  next:
    for(ch = 0; ch < s->nb_channels; ch++) {
        int n4, index;
        n4 = s->block_len / 2;
        if(s->channel_coded[ch]){
            wma_imdct_calc_c(&s->mdct_ctx[bsize], s->output, s->coefs[ch]);
        }else if(!(s->ms_stereo && ch==1))
            memset(s->output, 0, sizeof(s->output));

        /* multiply by the window and add in the frame */
        index = (s->frame_len / 2) + s->block_pos - n4;
        wma_window(s, &s->frame_out[ch][index]);
    }
    /* update block number */
    s->block_num++;
    s->block_pos += s->block_len;
    if (s->block_pos >= s->frame_len)
        return 1;
    else
        return 0;
}

/* decode a frame of frame_len samples */
static int wma_decode_frame(WMACodecContext *s, int16_t *samples)
{
#ifdef JZC_PMON_P01
          PMON_ON(mpeg4vlc);
#endif
    int ret, i, n, ch, incr;
    int16_t *ptr;
    int16_t *iptr,*ch0,*ch1;

#ifdef TRACE
    tprintf(s->avctx, "***decode_frame: %d size=%d\n", s->frame_count++, s->frame_len);
#endif

    /* read each block */
    s->block_num = 0;
    s->block_pos = 0;
    for(;;) {
        ret = wma_decode_block(s);
        if (ret < 0)
            return -1;
        if (ret)
            break;
    }
    /* convert frame to integer */
    n = s->frame_len;
    incr = s->nb_channels;

#if 0
    for(ch = 0; ch < s->nb_channels; ch++) {
        ptr = samples + ch;
        iptr = s->frame_out[ch];

        for(i=0;i<n;i++) {
           *ptr = *iptr++;
           ptr += incr;
        }
        /* prepare for next block */
        memmove(&s->frame_out[ch][0], &s->frame_out[ch][s->frame_len],
                    s->frame_len * sizeof(int16_t));
    }
#else
  if(incr==2){
    n>>=1;
    ch0=s->frame_out[0]-2;
    ch1=s->frame_out[1]-2;
    ptr =samples-2;
    if (!s->channel_coded[1])
      for(i=0;i<n;i++)
	{
	  S32LDI(xr1,ch0,4);
	  S32MOVZ(xr2,xr0,xr1);
	  S32SFL(xr2,xr2,xr1,xr1,ptn3);
	  S32SDI(xr1,ptr,4);
	  S32SDI(xr2,ptr,4);
	}
    else
      for(i=0;i<n;i++)
	{
	  S32LDI(xr1,ch0,4);
	  S32LDI(xr2,ch1,4);
	  S32SFL(xr2,xr2,xr1,xr1,ptn3);
	  S32SDI(xr1,ptr,4);
	  S32SDI(xr2,ptr,4);
	}
  }else{
    memcpy(samples,s->frame_out[0], s->frame_len*sizeof(int16_t));
  }
  for(ch=0;ch<incr;ch++){
    ch0=&s->frame_out[ch][0]-2;
    ch1=&s->frame_out[ch][s->frame_len]-2;
  for(i=0;i<n;i++)
    {
      S32LDI(xr5,ch1,4);
      S32SDI(xr5,ch0,4);
    }
  }
#endif
#ifdef JZC_PMON_P01
          PMON_OFF(mpeg4vlc);
#endif
#ifdef JZC_PMON_P01
    {
      printf("PMON VLC  -D: %d; I:%d\n",
	      mpeg4vlc_pmon_val, mpeg4vlc_pmon_val_ex);     
      mpeg4vlc_pmon_val=0; mpeg4vlc_pmon_val_ex=0;

    }
#endif //JZC_PMON_P0

#ifdef TRACE
    dump_shorts(s, "samples", samples, n * s->nb_channels);
#endif
    return 0;
}

static int wma_decode_superframe(AVCodecContext *avctx,
                                 void *data, int *data_size,
                                 AVPacket *avpkt)
{
    const uint8_t *buf = avpkt->data;
    int buf_size = avpkt->size;
    WMACodecContext *s = avctx->priv_data;
    int nb_frames, bit_offset, i, pos, len;
    uint8_t *q;
    int16_t *samples;

    tprintf(avctx, "***decode_superframe:\n");

    if(buf_size==0){
        s->last_superframe_len = 0;
        return 0;
    }
    if (buf_size < s->block_align)
        return 0;
    buf_size = s->block_align;

    samples = data;

    init_get_bits(&s->gb, buf, buf_size*8);

    if (s->use_bit_reservoir) {
        /* read super frame header */
        skip_bits(&s->gb, 4); /* super frame index */
        nb_frames = get_bits(&s->gb, 4) - 1;

        if((nb_frames+1) * s->nb_channels * s->frame_len * sizeof(int16_t) > *data_size){
            av_log(s->avctx, AV_LOG_ERROR, "Insufficient output space\n");
            goto fail;
        }

        bit_offset = get_bits(&s->gb, s->byte_offset_bits + 3);

        if (s->last_superframe_len > 0) {
            //        printf("skip=%d\n", s->last_bitoffset);
            /* add bit_offset bits to last frame */
            if ((s->last_superframe_len + ((bit_offset + 7) >> 3)) >
                MAX_CODED_SUPERFRAME_SIZE)
                goto fail;
            q = s->last_superframe + s->last_superframe_len;
            len = bit_offset;
            while (len > 7) {
                *q++ = (get_bits)(&s->gb, 8);
                len -= 8;
            }
            if (len > 0) {
                *q++ = (get_bits)(&s->gb, len) << (8 - len);
            }

            /* XXX: bit_offset bits into last frame */
            init_get_bits(&s->gb, s->last_superframe, MAX_CODED_SUPERFRAME_SIZE*8);
            /* skip unused bits */
            if (s->last_bitoffset > 0)
                skip_bits(&s->gb, s->last_bitoffset);
            /* this frame is stored in the last superframe and in the
               current one */
            if (wma_decode_frame(s, samples) < 0) {
		mp_msg(NULL, NULL, "file:%s, line:%d, wma_decode_frame failed\n", __FILE__, __LINE__);
                goto fail;
	    }
            samples += s->nb_channels * s->frame_len;
        }

        /* read each frame starting from bit_offset */
        pos = bit_offset + 4 + 4 + s->byte_offset_bits + 3;
        init_get_bits(&s->gb, buf + (pos >> 3), (MAX_CODED_SUPERFRAME_SIZE - (pos >> 3))*8);
        len = pos & 7;
        if (len > 0)
            skip_bits(&s->gb, len);

        s->reset_block_lengths = 1;
        for(i=0;i<nb_frames;i++) {
            if (wma_decode_frame(s, samples) < 0) {
		mp_msg(NULL, NULL, "file:%s, line:%d, wma_decode_frame failed\n", __FILE__, __LINE__);
                goto fail;
	    }
            samples += s->nb_channels * s->frame_len;
        }

        /* we copy the end of the frame in the last frame buffer */
        pos = get_bits_count(&s->gb) + ((bit_offset + 4 + 4 + s->byte_offset_bits + 3) & ~7);
        s->last_bitoffset = pos & 7;
        pos >>= 3;
        len = buf_size - pos;
        if (len > MAX_CODED_SUPERFRAME_SIZE || len < 0) {
            av_log(s->avctx, AV_LOG_ERROR, "len %d invalid\n", len);
            goto fail;
        }
        s->last_superframe_len = len;
        memcpy(s->last_superframe, buf + pos, len);
    } else {
        if(s->nb_channels * s->frame_len * sizeof(int16_t) > *data_size){
            av_log(s->avctx, AV_LOG_ERROR, "Insufficient output space\n");
            goto fail;
        }
        /* single frame decode */
        if (wma_decode_frame(s, samples) < 0) {
	    mp_msg(NULL, NULL, "file:%s, line:%d, wma_decode_frame failed\n", __FILE__, __LINE__);
            goto fail;
	}
        samples += s->nb_channels * s->frame_len;
    }

//av_log(NULL, AV_LOG_ERROR, "%d %d %d %d outbytes:%d eaten:%d\n", s->frame_len_bits, s->block_len_bits, s->frame_len, s->block_len,        (int8_t *)samples - (int8_t *)data, s->block_align);

    *data_size = (int8_t *)samples - (int8_t *)data;
    return s->block_align;
 fail:
    /* when error, we reset the bit reservoir */
    s->last_superframe_len = 0;
    return -1;
}

static av_cold void flush(AVCodecContext *avctx)
{
    WMACodecContext *s = avctx->priv_data;

    s->last_bitoffset=
    s->last_superframe_len= 0;
}

AVCodec wmav1_decoder =
{
    "wmav1",
    AVMEDIA_TYPE_AUDIO,
    CODEC_ID_WMAV1,
    sizeof(WMACodecContext),
    wma_decode_init,
    NULL,
    ff_wma_end,
    wma_decode_superframe,
    .flush=flush,
    .long_name = NULL_IF_CONFIG_SMALL("Windows Media Audio 1"),
};

AVCodec wmav2_decoder =
{
    "wmav2",
    AVMEDIA_TYPE_AUDIO,
    CODEC_ID_WMAV2,
    sizeof(WMACodecContext),
    wma_decode_init,
    NULL,
    ff_wma_end,
    wma_decode_superframe,
    .flush=flush,
    .long_name = NULL_IF_CONFIG_SMALL("Windows Media Audio 2"),
};
