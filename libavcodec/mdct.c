/*
 * MDCT/IMDCT transforms
 * Copyright (c) 2002 Fabrice Bellard
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

#include <stdlib.h>
#include <string.h>
#include "libavutil/common.h"
#include "libavutil/mathematics.h"
#include "fft.h"

#include "../libjzcommon/jzmedia.h"
#include "aactab.h" 
/**
 * @file
 * MDCT/IMDCT transforms.
 */
#define WMA3_BITS 28 
#define WMA3_PRECISION (1 << WMA3_BITS)
#define WMA3_CONST(A) (((A) >= 0) ? ((int32_t)((A)*(WMA3_PRECISION)+0.5)) : ((int32_t)((A)*(WMA3_PRECISION)-0.5)))
#define WMA3_BITS1 14 
#define MUL_P(A,B) (int32_t)(((int64_t)(A)*(int64_t)(B)) >> WMA4_BITS1)

// Generate a Kaiser-Bessel Derived Window.
#define BESSEL_I0_ITER 50 // default: 50 iterations of Bessel I0 approximation

av_cold void ff_kbd_window_init(float *window, float alpha, int n)
{
   int i, j;
   double sum = 0.0, bessel, tmp;
   double local_window[FF_KBD_WINDOW_MAX];
   double alpha2 = (alpha * M_PI / n) * (alpha * M_PI / n);

   assert(n <= FF_KBD_WINDOW_MAX);

   for (i = 0; i < n; i++) {
       tmp = i * (n - i) * alpha2;
       bessel = 1.0;
       for (j = BESSEL_I0_ITER; j > 0; j--)
           bessel = bessel * tmp / (j * j) + 1;
       sum += bessel;
       local_window[i] = sum;
   }

   sum++;
   for (i = 0; i < n; i++)
       window[i] = sqrt(local_window[i] / sum);
}

av_cold void aac_kbd_window_init(int32_t *window, float alpha, int n)
{
   int i, j;
   double sum = 0.0, bessel, tmp;
   double local_window[FF_KBD_WINDOW_MAX];
   double alpha2 = (alpha * M_PI / n) * (alpha * M_PI / n);

   assert(n <= FF_KBD_WINDOW_MAX);

   for (i = 0; i < n; i++) {
       tmp = i * (n - i) * alpha2;
       bessel = 1.0;
       for (j = BESSEL_I0_ITER; j > 0; j--)
           bessel = bessel * tmp / (j * j) + 1;
       sum += bessel;
       local_window[i] = sum;
   }

   sum++;
   for (i = 0; i < n; i++)
     window[i] = FRAC_CONST (sqrt(local_window[i] / sum));
}

#include "mdct_tablegen.h"

av_cold int aac_mdct_init(FFTContext *s, int nbits, int inverse, double scale)
{

    int n, n4, i;
    double alpha, theta;
    int tstep;

    memset(s, 0, sizeof(*s));
    n = 1 << nbits;
    s->mdct_bits = nbits;
    s->mdct_size = n;
    n4 = n >> 2;
    s->permutation = FF_MDCT_PERM_NONE;
    if (aac_fft_init(s, s->mdct_bits - 2, inverse) < 0)
      {
        goto fail;
      }

    s->atcos = av_malloc(n/2 * sizeof(FFTSample));
    if (!s->atcos)
      {
        goto fail;
      }
    
    switch (s->permutation) {
    case FF_MDCT_PERM_NONE:
        s->atsin = s->atcos + n4;
        tstep = 1;
        break;
    case FF_MDCT_PERM_INTERLEAVE:
        s->atsin = s->atcos + 1;
        tstep = 2;
        break;
    default:
        goto fail;
    }

    theta = 1.0 / 8.0 + (scale < 0 ? n4 : 0);
    scale = sqrt(fabs(scale));
    for(i=0;i<n4;i++) {
        alpha = 2 * M_PI * (i + theta) / n;
        s->atcos[i*tstep] = -FRAC_CONST(cos(alpha) * scale);
        s->atsin[i*tstep] = -FRAC_CONST(sin(alpha) * scale);
    }
    return 0;
 fail:
    ff_mdct_end(s);
    return -1;
}

av_cold int wma3_mdct_init(FFTContext *s, int nbits, int inverse, double scale)
{

    int n, n4, i;
    double alpha, theta;
    int tstep;

    memset(s, 0, sizeof(*s));
    n = 1 << nbits;
    s->mdct_bits = nbits;
    s->mdct_size = n;
    n4 = n >> 2;
    s->permutation = FF_MDCT_PERM_NONE;
    if (aac_fft_init(s, s->mdct_bits - 2, inverse) < 0)
      {
        goto fail;
      }

    s->atcos = av_malloc(n/2 * sizeof(FFTSample));
    if (!s->atcos)
      {
        goto fail;
      }
    
    switch (s->permutation) {
    case FF_MDCT_PERM_NONE:
        s->atsin = s->atcos + n4;
        tstep = 1;
        break;
    case FF_MDCT_PERM_INTERLEAVE:
        s->atsin = s->atcos + 1;
        tstep = 2;
        break;
    default:
        goto fail;
    }

    theta = 1.0 / 8.0 + (scale < 0 ? n4 : 0);
    scale = sqrt(fabs(scale));
    for(i=0;i<n4;i++) {
        alpha = 2 * M_PI * (i + theta) / n;
        s->atcos[i*tstep] = -WMA3_CONST(cos(alpha) * scale);
        s->atsin[i*tstep] = -WMA3_CONST(sin(alpha) * scale);
    }
    return 0;
 fail:
    ff_mdct_end(s);
    return -1;
}

/**
 * init MDCT or IMDCT computation.
 */
av_cold int ff_mdct_init(FFTContext *s, int nbits, int inverse, double scale)
{
    int n, n4, i;
    double alpha, theta;
    int tstep;

    memset(s, 0, sizeof(*s));
    n = 1 << nbits;
    s->mdct_bits = nbits;
    s->mdct_size = n;
    n4 = n >> 2;
    s->permutation = FF_MDCT_PERM_NONE;
    if (ff_fft_init(s, s->mdct_bits - 2, inverse) < 0)
      {
        goto fail;
      }

    s->tcos = av_malloc(n/2 * sizeof(FFTSample));
    if (!s->tcos)
      {
        goto fail;
      }
    
    switch (s->permutation) {
    case FF_MDCT_PERM_NONE:
        s->tsin = s->tcos + n4;
        tstep = 1;
        break;
    case FF_MDCT_PERM_INTERLEAVE:
        s->tsin = s->tcos + 1;
        tstep = 2;
        break;
    default:
        goto fail;
    }

    theta = 1.0 / 8.0 + (scale < 0 ? n4 : 0);
    scale = sqrt(fabs(scale));
    for(i=0;i<n4;i++) {
        alpha = 2 * M_PI * (i + theta) / n;
        s->tcos[i*tstep] = -cos(alpha) * scale;
        s->tsin[i*tstep] = -sin(alpha) * scale;
    }
    return 0;
 fail:
    ff_mdct_end(s);
    return -1;
}

av_cold int cook_mdct_init(FFTContext *s, int nbits, int inverse, double scale)
{
    int n, n4, i;
    double alpha, theta;
    int tstep;

    memset(s, 0, sizeof(*s));
    n = 1 << nbits;
    s->mdct_bits = nbits;
    s->mdct_size = n;
    n4 = n >> 2;
    s->permutation = FF_MDCT_PERM_NONE;
    if (ff_fft_init(s, s->mdct_bits - 2, inverse) < 0)
      {
        goto fail;
      }

    s->tcos = av_malloc(n/2 * sizeof(FFTSample));
    s->atcos = av_malloc(n/2 * sizeof(FFTSample));
    if (!s->tcos)
      {
        goto fail;
      }
    
    switch (s->permutation) {
    case FF_MDCT_PERM_NONE:
        s->tsin = s->tcos + n4;
	s->atsin = s->atcos + n4;
        tstep = 1;
        break;
    case FF_MDCT_PERM_INTERLEAVE:
        s->tsin = s->tcos + 1;
	s->atsin = s->atcos + 1;
        tstep = 2;
        break;
    default:
        goto fail;
    }

    theta = 1.0 / 8.0 + (scale < 0 ? n4 : 0);
    scale = sqrt(fabs(scale));
    for(i=0;i<n4;i++) {
        alpha = 2 * M_PI * (i + theta) / n;
        s->tcos[i*tstep] = -cos(alpha) * scale;
        s->tsin[i*tstep] = -sin(alpha) * scale;
	s->atcos[i*tstep] = -FRAC_CONST(cos(alpha) * scale);
        s->atsin[i*tstep] = -FRAC_CONST(sin(alpha) * scale);
    }
    return 0;
 fail:
    ff_mdct_end(s);
    return -1;
}

/* complex multiplication: p = a * b */
#define CMUL(pre, pim, are, aim, bre, bim) \
{\
    FFTSample _are = (are);\
    FFTSample _aim = (aim);\
    FFTSample _bre = (bre);\
    FFTSample _bim = (bim);\
    (pre) = _are * _bre - _aim * _bim;\
    (pim) = _are * _bim + _aim * _bre;\
}

#if 1
#if 0
#define CMUL_FIX(pre, pim, are, aim, bre, bim) \
{\
    S32MUL(xr1,xr2, are, bre);\
    S32MUL(xr3,xr4, are, bim);\
    S32MSUB(xr1,xr2,aim,bim);\
    S32MADD(xr3,xr4,aim,bre);\
    S32EXTRV(xr1,xr2,2,31);\
    S32EXTRV(xr3,xr4,2,31);\
    D32SLL(xr5,xr1,xr3,xr6,1);\
    S32STD(xr5,&pre,0);  \
    S32STD(xr6,&pim,0);  \
}
#else
#define CMUL_FIX(pre, pim, are, aim, bre, bim) \
{\
    S32MUL(xr1,xr2, are, bre);\
    S32MUL(xr3,xr4, aim, bim);\
    S32MUL(xr5,xr6, are, bim);\
    S32MUL(xr7,xr8, aim, bre);\
    S32EXTRV(xr1,xr2,3,31);\
    S32EXTRV(xr3,xr4,3,31);\
    S32EXTRV(xr5,xr6,3,31);\
    S32EXTRV(xr7,xr8,3,31);\
    D32SLL(xr1,xr1,xr3,xr3,1);\
    D32SLL(xr5,xr5,xr7,xr7,1);\
    D32ADD_SS(xr9,xr1,xr3,xr0);\
    D32ADD_AA(xr10,xr5,xr7,xr0);\
    D32SAR(xr11,xr9,xr10,xr12,1);\
    pre = S32M2I(xr11);\
    pim = S32M2I(xr12);\
}
#endif
#else
#define CMUL_FIX(pre, pim, are, aim, bre, bim) \
{\
    (pre) = MUL_F(are , bre) - MUL_F(aim , bim);	\
    (pim) = MUL_F(are , bim) + MUL_F(aim , bre);	\
}
#endif


#define WMA3_CMUL_FIX(pre, pim, are, aim, bre, bim) \
{\
  (pre) = MUL_P(are , bre) - MUL_P(aim , bim);	\
  (pim) = MUL_P(are , bim) + MUL_P(aim , bre);	\
}

/**
 * Compute the middle half of the inverse MDCT of size N = 2^nbits,
 * thus excluding the parts that can be derived by symmetry
 * @param output N/2 samples
 * @param input N/2 samples
 */

void aac_imdct_half_c(FFTContext *s, int32_t *output, const int32_t *input)
{
    int k, n8, n4, n2, n, j;
    const uint16_t *revtab = s->revtab;
    const int32_t *tcos = s->atcos;
    const int32_t *tsin = s->atsin;
    const int32_t *in1, *in2;
    AAC_FFTComplex_fix *z = (AAC_FFTComplex_fix *)output;

    n = 1 << s->mdct_bits;
    n2 = n >> 1;
    n4 = n >> 2;
    n8 = n >> 3;

    /* pre rotation */
    in1 = input;
    in2 = input + n2 - 1;
    for(k = 0; k < n4; k++) {
        j=revtab[k];
        CMUL_FIX(z[j].re, z[j].im, *in2, *in1, tcos[k], tsin[k]);
        in1 += 2;
        in2 -= 2;
    }
    aac_fft_calc_c(s, z);

    /* post rotation + reordering */
    for(k = 0; k < n8; k++) {
        int32_t r0, i0, r1, i1;
        CMUL_FIX(r0, i1, z[n8-k-1].im, z[n8-k-1].re, tsin[n8-k-1], tcos[n8-k-1]);
        CMUL_FIX(r1, i0, z[n8+k  ].im, z[n8+k  ].re, tsin[n8+k  ], tcos[n8+k  ]);
        z[n8-k-1].re = r0;
        z[n8-k-1].im = i0;
        z[n8+k  ].re = r1;
        z[n8+k  ].im = i1;
    }
}

void ff_imdct_half_c(FFTContext *s, FFTSample *output, const FFTSample *input)
{
    int k, n8, n4, n2, n, j;
    const uint16_t *revtab = s->revtab;
    const FFTSample *tcos = s->tcos;
    const FFTSample *tsin = s->tsin;
    const FFTSample *in1, *in2;
    FFTComplex *z = (FFTComplex *)output;

    n = 1 << s->mdct_bits;
    n2 = n >> 1;
    n4 = n >> 2;
    n8 = n >> 3;

    /* pre rotation */
    in1 = input;
    in2 = input + n2 - 1;
    for(k = 0; k < n4; k++) {
        j=revtab[k];
        CMUL(z[j].re, z[j].im, *in2, *in1, tcos[k], tsin[k]);
        in1 += 2;
        in2 -= 2;
    }

    ff_fft_calc(s, z);

    /* post rotation + reordering */
    for(k = 0; k < n8; k++) {
        FFTSample r0, i0, r1, i1;

        CMUL(r0, i1, z[n8-k-1].im, z[n8-k-1].re, tsin[n8-k-1], tcos[n8-k-1]);
        CMUL(r1, i0, z[n8+k  ].im, z[n8+k  ].re, tsin[n8+k  ], tcos[n8+k  ]);
        z[n8-k-1].re = r0;
        z[n8-k-1].im = i0;
        z[n8+k  ].re = r1;
        z[n8+k  ].im = i1;
    }
}

void wma3_imdct_half_c(FFTContext *s, int32_t *output, const int32_t *input)
{
    int k, n8, n4, n2, n, j,j1;
    const uint16_t *revtab = s->revtab;
    const int32_t *tcos = s->atcos;
    const int32_t *tsin = s->atsin;
    const int32_t *in1, *in2;
    AAC_FFTComplex_fix *z = (AAC_FFTComplex_fix *)output;

    n = 1 << s->mdct_bits;
    n2 = n >> 1;
    n4 = n >> 2;
    n8 = n >> 3;

    /* pre rotation */
    in1 = input;
    in2 = input + n2 - 1;
#if 0
    for(k = 0; k < n4; k++) {
        j=revtab[k];
        WMA3_CMUL_FIX(z[j].re, z[j].im, *in2, *in1, tcos[k], tsin[k]);
        in1 += 2;
        in2 -= 2;
    }
#else
    for(k = 0; k < n8; k++) {
     j=revtab[k];
     n=n4-k-1;
     j1=revtab[n];
     S32MUL(xr1,xr2, *in2, tcos[k]);
     S32MUL(xr3,xr4, *in1, tsin[k]);
     S32MUL(xr5,xr6, *in2, tsin[k]);
     S32MUL(xr7,xr8, *in1, tcos[k]);
     S32EXTRV(xr1,xr2,18,31);
     S32EXTRV(xr3,xr4,18,31);
     S32EXTRV(xr5,xr6,18,31);
     S32EXTRV(xr7,xr8,18,31);
     D32SLL(xr1,xr1,xr3,xr3,1);
     D32SLL(xr5,xr5,xr7,xr7,1);
     in1++;
     D32ADD_SS(xr9,xr1,xr3,xr0);
     D32ADD_AA(xr10,xr5,xr7,xr0);
     in2--;
     D32SAR(xr11,xr9,xr10,xr12,1);                
     S32MUL(xr1,xr2, *in1, tcos[n]);
     S32MUL(xr3,xr4, *in2, tsin[n]);
     S32MUL(xr5,xr6, *in1, tsin[n]);
     S32MUL(xr7,xr8, *in2, tcos[n]);
     S32EXTRV(xr1,xr2,18,31);
     S32EXTRV(xr3,xr4,18,31);
     S32EXTRV(xr5,xr6,18,31);
     S32EXTRV(xr7,xr8,18,31);
     D32SLL(xr1,xr1,xr3,xr3,1);
     D32SLL(xr5,xr5,xr7,xr7,1);
     z[j].re=S32M2I(xr11);
     D32ADD_SS(xr9,xr1,xr3,xr0);
     D32ADD_AA(xr10,xr5,xr7,xr0);
     z[j].im=S32M2I(xr12);
     D32SAR(xr13,xr9,xr10,xr14,1);          
     in1++;
     in2--;
     z[j1].re=S32M2I(xr13);
     z[j1].im=S32M2I(xr14);
    }
#endif
    aac_fft_calc_c(s, z);

    /* post rotation + reordering */

    for(k = 0; k < n8; k++) {
#if 0
        int32_t r0, i0, r1, i1;
        WMA3_CMUL_FIX(r0, i1, z[n8-k-1].im, z[n8-k-1].re, tsin[n8-k-1], tcos[n8-k-1]);
        WMA3_CMUL_FIX(r1, i0, z[n8+k  ].im, z[n8+k  ].re, tsin[n8+k  ], tcos[n8+k  ]);
        z[n8-k-1].re = r0;
        z[n8-k-1].im = i0;
        z[n8+k  ].re = r1;
        z[n8+k  ].im = i1;
#else
       S32MUL(xr1,xr2, z[n8-k-1].im, tsin[n8-k-1]);
       S32MUL(xr3,xr4, z[n8-k-1].im, tcos[n8-k-1]);
       S32MUL(xr7,xr8, z[n8+k  ].im, tsin[n8+k  ]);
       S32MUL(xr9,xr10, z[n8+k  ].im, tcos[n8+k  ]);
       S32MSUB(xr1,xr2,z[n8-k-1].re,tcos[n8-k-1]);       
       S32MADD(xr3,xr4, z[n8-k-1].re, tsin[n8-k-1]);
       S32MSUB(xr7,xr8, z[n8+k  ].re, tcos[n8+k  ]);
       S32MADD(xr9,xr10, z[n8+k  ].re, tsin[n8+k  ]);
       S32EXTRV(xr1,xr2,18,31);
       S32EXTRV(xr3,xr4,18,31);
       S32EXTRV(xr7,xr8,18,31);
       S32EXTRV(xr9,xr10,18,31);
       D32SLL(xr5,xr1,xr3,xr6,1);
       D32SLL(xr11,xr7,xr9,xr12,1);
       z[n8-k-1].re = S32M2I(xr5);
       z[n8+k  ].im = S32M2I(xr6);
       z[n8+k  ].re = S32M2I(xr11);
       z[n8-k-1].im = S32M2I(xr12);
#endif
    }
}

void wma_imdct_half_c(FFTContext *s, int32_t *output, const int32_t *input)
{
    int k, n8, n4, n2, n, j,j1;
    const uint16_t *revtab = s->revtab;
    const int32_t *tcos = s->atcos;
    const int32_t *tsin = s->atsin;
    const int32_t *in1, *in2;
    AAC_FFTComplex_fix *z = (AAC_FFTComplex_fix *)output;

    n = 1 << s->mdct_bits;
    n2 = n >> 1;
    n4 = n >> 2;
    n8 = n >> 3;

    /* pre rotation */
    in1 = input;
    in2 = input + n2 - 1;
#if 0
    for(k = 0; k < n4; k++) {
        j=revtab[k];
        CMUL_FIX(z[j].re, z[j].im, *in2, *in1, tcos[k], tsin[k]);
        in1 += 2;
        in2 -= 2;
    }
#else
    for(k = 0; k < n8; k++) {
     j=revtab[k];
     n=n4-k-1;
     j1=revtab[n];
     S32MUL(xr1,xr2, *in2, tcos[k]);
     S32MUL(xr3,xr4, *in1, tsin[k]);
     S32MUL(xr5,xr6, *in2, tsin[k]);
     S32MUL(xr7,xr8, *in1, tcos[k]);
     S32EXTRV(xr1,xr2,3,31);
     S32EXTRV(xr3,xr4,3,31);
     S32EXTRV(xr5,xr6,3,31);
     S32EXTRV(xr7,xr8,3,31);
     D32SLL(xr1,xr1,xr3,xr3,1);
     D32SLL(xr5,xr5,xr7,xr7,1);
     in1++;
     D32ADD_SS(xr9,xr1,xr3,xr0);
     D32ADD_AA(xr10,xr5,xr7,xr0);
     in2--;
     D32SAR(xr11,xr9,xr10,xr12,1);                
     S32MUL(xr1,xr2, *in1, tcos[n]);
     S32MUL(xr3,xr4, *in2, tsin[n]);
     S32MUL(xr5,xr6, *in1, tsin[n]);
     S32MUL(xr7,xr8, *in2, tcos[n]);
     S32EXTRV(xr1,xr2,3,31);
     S32EXTRV(xr3,xr4,3,31);
     S32EXTRV(xr5,xr6,3,31);
     S32EXTRV(xr7,xr8,3,31);
     D32SLL(xr1,xr1,xr3,xr3,1);
     D32SLL(xr5,xr5,xr7,xr7,1);
     z[j].re=S32M2I(xr11);
     D32ADD_SS(xr9,xr1,xr3,xr0);
     D32ADD_AA(xr10,xr5,xr7,xr0);
     z[j].im=S32M2I(xr12);
     D32SAR(xr13,xr9,xr10,xr14,1);          
     in1++;
     in2--;
     z[j1].re=S32M2I(xr13);
     z[j1].im=S32M2I(xr14);
    }
#endif
    aac_fft_calc_c(s, z);

    /* post rotation + reordering */

    for(k = 0; k < n8; k++) {
#if 0
        int32_t r0, i0, r1, i1;
        CMUL_FIX(r0, i1, z[n8-k-1].im, z[n8-k-1].re, tsin[n8-k-1], tcos[n8-k-1]);
        CMUL_FIX(r1, i0, z[n8+k  ].im, z[n8+k  ].re, tsin[n8+k  ], tcos[n8+k  ]);
        z[n8-k-1].re = r0;
        z[n8-k-1].im = i0;
        z[n8+k  ].re = r1;
        z[n8+k  ].im = i1;
#else
       S32MUL(xr1,xr2, z[n8-k-1].im, tsin[n8-k-1]);
       S32MUL(xr3,xr4, z[n8-k-1].im, tcos[n8-k-1]);
       S32MUL(xr7,xr8, z[n8+k  ].im, tsin[n8+k  ]);
       S32MUL(xr9,xr10, z[n8+k  ].im, tcos[n8+k  ]);
       S32MSUB(xr1,xr2,z[n8-k-1].re,tcos[n8-k-1]);       
       S32MADD(xr3,xr4, z[n8-k-1].re, tsin[n8-k-1]);
       S32MSUB(xr7,xr8, z[n8+k  ].re, tcos[n8+k  ]);
       S32MADD(xr9,xr10, z[n8+k  ].re, tsin[n8+k  ]);
       S32EXTRV(xr1,xr2,2,31);
       S32EXTRV(xr3,xr4,2,31);
       S32EXTRV(xr7,xr8,2,31);
       S32EXTRV(xr9,xr10,2,31);
       D32SLL(xr5,xr1,xr3,xr6,1);
       D32SLL(xr11,xr7,xr9,xr12,1);
       z[n8-k-1].re = S32M2I(xr5);
       z[n8+k  ].im = S32M2I(xr6);
       z[n8+k  ].re = S32M2I(xr11);
       z[n8-k-1].im = S32M2I(xr12);
#endif
    }
}

void wma_imdct_calc_c(FFTContext *s, int32_t *output, const int32_t *input)
{
    int k;
    int n = 1 << s->mdct_bits;
    int n2 = n >> 1;
    int n4 = n >> 2;

    wma_imdct_half_c(s, output+n4, input);

    for(k = 0; k < n4; k+=2) {
        output[k] = -output[n2-k-1];
        output[k+1] = -output[n2-k-2];
        output[n-k-1] = output[n2+k];
        output[n-k-2] = output[n2+k+1];
    }
}

/**
 * Compute inverse MDCT of size N = 2^nbits
 * @param output N samples
 * @param input N/2 samples
 */
void ff_imdct_calc_c(FFTContext *s, FFTSample *output, const FFTSample *input)
{
    int k;
    int n = 1 << s->mdct_bits;
    int n2 = n >> 1;
    int n4 = n >> 2;

    ff_imdct_half_c(s, output+n4, input);

    for(k = 0; k < n4; k++) {
        output[k] = -output[n2-k-1];
        output[n-k-1] = output[n2+k];
    }
}

void aac_imdct_calc_c(FFTContext *s, int32_t *output, const int32_t *input)
{
    int k;
    int n = 1 << s->mdct_bits;
    int n2 = n >> 1;
    int n4 = n >> 2;

    aac_imdct_half_c(s, output+n4, input);

    for(k = 0; k < n4; k++) {
        output[k] = -output[n2-k-1];
        output[n-k-1] = output[n2+k];
    }
}

/**
 * Compute MDCT of size N = 2^nbits
 * @param input N samples
 * @param out N/2 samples
 */
void ff_mdct_calc_c(FFTContext *s, FFTSample *out, const FFTSample *input)
{
    int i, j, n, n8, n4, n2, n3;
    FFTSample re, im;
    const uint16_t *revtab = s->revtab;
    const FFTSample *tcos = s->tcos;
    const FFTSample *tsin = s->tsin;
    FFTComplex *x = (FFTComplex *)out;

    n = 1 << s->mdct_bits;
    n2 = n >> 1;
    n4 = n >> 2;
    n8 = n >> 3;
    n3 = 3 * n4;

    /* pre rotation */
    for(i=0;i<n8;i++) {
        re = -input[2*i+3*n4] - input[n3-1-2*i];
        im = -input[n4+2*i] + input[n4-1-2*i];
        j = revtab[i];
        CMUL(x[j].re, x[j].im, re, im, -tcos[i], tsin[i]);

        re = input[2*i] - input[n2-1-2*i];
        im = -(input[n2+2*i] + input[n-1-2*i]);
        j = revtab[n8 + i];
        CMUL(x[j].re, x[j].im, re, im, -tcos[n8 + i], tsin[n8 + i]);
    }

    ff_fft_calc(s, x);

    /* post rotation */
    for(i=0;i<n8;i++) {
        FFTSample r0, i0, r1, i1;
        CMUL(i1, r0, x[n8-i-1].re, x[n8-i-1].im, -tsin[n8-i-1], -tcos[n8-i-1]);
        CMUL(i0, r1, x[n8+i  ].re, x[n8+i  ].im, -tsin[n8+i  ], -tcos[n8+i  ]);
        x[n8-i-1].re = r0;
        x[n8-i-1].im = i0;
        x[n8+i  ].re = r1;
        x[n8+i  ].im = i1;
    }
}

av_cold void ff_mdct_end(FFTContext *s)
{
    av_freep(&s->tcos);
    av_freep(&s->atcos);
    ff_fft_end(s);
}
