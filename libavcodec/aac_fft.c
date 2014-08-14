/*
 * FFT/IFFT transforms
 * Copyright (c) 2008 Loren Merritt
 * Copyright (c) 2002 Fabrice Bellard
 * Partly based on libdjbfft by D. J. Bernstein
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
 * FFT/IFFT transforms.
 */

#include <stdlib.h>
#include <string.h>
#include "libavutil/mathematics.h"
#include "fft.h"
#include "aactab.h" 
#include "../libjzcommon/jzmedia.h"

/* cos(2*pi*x/n) for 0<=x<=n/4, followed by its reverse */

static int split_radix_permutation(int i, int n, int inverse)
{
    int m;
    if(n <= 2) return i&1;
    m = n >> 1;
    if(!(i&m))            return split_radix_permutation(i, m, inverse)*2;
    m >>= 1;
    if(inverse == !(i&m)) return split_radix_permutation(i, m, inverse)*4 + 1;
    else                  return split_radix_permutation(i, m, inverse)*4 - 1;
}

av_cold int aac_fft_init(FFTContext *s, int nbits, int inverse)
{
    int i, j, n;

    if (nbits < 2 || nbits > 16)
        goto fail;
    s->nbits = nbits;
    n = 1 << nbits;

    s->revtab = av_malloc(n * sizeof(uint16_t));
    if (!s->revtab)
        goto fail;
    s->tmp_buf = av_malloc(n * sizeof(FFTComplex));
    if (!s->tmp_buf)
        goto fail;
    s->inverse = inverse;

    s->fft_permute = ff_fft_permute_c;
    s->fft_calc    = ff_fft_calc_c;
#if CONFIG_MDCT
    s->imdct_calc  = ff_imdct_calc_c;
    s->imdct_half  = ff_imdct_half_c;
    s->mdct_calc   = ff_mdct_calc_c;
#endif

    if (ARCH_ARM)     ff_fft_init_arm(s);
    if (HAVE_ALTIVEC) ff_fft_init_altivec(s);
    if (HAVE_MMX)     ff_fft_init_mmx(s);

    for(j=4; j<=nbits; j++) {
        ff_init_ff_cos_tabs(j);
    }
    for(i=0; i<n; i++)
        s->revtab[-split_radix_permutation(i, n, s->inverse) & (n-1)] = i;

    return 0;
 fail:
    av_freep(&s->revtab);
    av_freep(&s->tmp_buf);
    return -1;
}

#define sqrthalf (float)M_SQRT1_2

#define BF(x,y,a,b) {\
    x = a - b;\
    y = a + b;\
}

#define BUTTERFLIES(a0,a1,a2,a3) {\
    BF(t3, t5, t5, t1);\
    BF(a2.re, a0.re, a0.re, t5);\
    BF(a3.im, a1.im, a1.im, t3);\
    BF(t4, t6, t2, t6);\
    BF(a3.re, a1.re, a1.re, t4);\
    BF(a2.im, a0.im, a0.im, t6);\
}

// force loading all the inputs before storing any.
// this is slightly slower for small data, but avoids store->load aliasing
// for addresses separated by large powers of 2.
#define BUTTERFLIES_BIG(a0,a1,a2,a3) {\
    int32_t r0=a0.re, i0=a0.im, r1=a1.re, i1=a1.im;\
    BF(t3, t5, t5, t1);\
    BF(a2.re, a0.re, r0, t5);\
    BF(a3.im, a1.im, i1, t3);\
    BF(t4, t6, t2, t6);\
    BF(a3.re, a1.re, r1, t4);\
    BF(a2.im, a0.im, i0, t6);\
}

#if 0
#define TRANSFORM_FIX(a0,a1,a2,a3,wre,wim) {\
    t1 = MUL_Z(a2.re , wre) + MUL_Z(a2.im , wim);	\
    t2 = MUL_Z(a2.im , wre) - MUL_Z(a2.re , wim);	\
    t5 = MUL_Z(a3.re , wre) - MUL_Z(a3.im , wim);	\
    t6 = MUL_Z(a3.im , wre) + MUL_Z(a3.re , wim);	\
    BUTTERFLIES(a0,a1,a2,a3)\
}
#else
#define TRANSFORM_FIX(a0,a1,a2,a3,wre,wim) {\
    S32MUL(xr1,xr2, a2.re, wre);\
    S32MUL(xr3,xr4, a2.im, wre);\
    S32MUL(xr7,xr8, a3.re, wre);\
    S32MUL(xr9,xr10, a3.im, wre);\
    S32MADD(xr1,xr2,a2.im,wim);\
    S32MSUB(xr3,xr4,a2.re,wim);\
    S32MSUB(xr7,xr8,a3.im,wim);\
    S32MADD(xr9,xr10,a3.re,wim);\
    S32EXTRV(xr1,xr2,2,31);\
    S32EXTRV(xr3,xr4,2,31);\  
    S32EXTRV(xr7,xr8,2,31);\
    S32EXTRV(xr9,xr10,2,31);\  
    D32SLL(xr5,xr1,xr3,xr6,1);\
    D32SLL(xr11,xr7,xr9,xr12,1);\
    D32SAR(xr5,xr5,xr6,xr6,1);\
    D32SAR(xr11,xr11,xr12,xr12,1);\
    t1 = S32M2I(xr5);   \
    t2 = S32M2I(xr6);   \
    t5 = S32M2I(xr11);  \
    t6 = S32M2I(xr12);  \
    BUTTERFLIES(a0,a1,a2,a3)\
}
#endif

#define TRANSFORM_ZERO(a0,a1,a2,a3) {\
    t1 = a2.re;\
    t2 = a2.im;\
    t5 = a3.re;\
    t6 = a3.im;\
    BUTTERFLIES(a0,a1,a2,a3)\
}

/* z[0...8n-1], w[1...2n-1] */
#define PASS(name)\
static void name(AAC_FFTComplex_fix *z, const int32_t *wre, unsigned int n)\
{\
    int32_t t1, t2, t3, t4, t5, t6;\
    int o1 = 2*n;\
    int o2 = 4*n;\
    int o3 = 6*n;\
    const int32_t *wim = wre+o1;\
    n--;\
\
    TRANSFORM_ZERO(z[0],z[o1],z[o2],z[o3]);\
    TRANSFORM_FIX(z[1],z[o1+1],z[o2+1],z[o3+1],wre[1],wim[-1]);\
    do {\
        z += 2;\
        wre += 2;\
        wim -= 2;\
        TRANSFORM_FIX(z[0],z[o1],z[o2],z[o3],wre[0],wim[0]);\
        TRANSFORM_FIX(z[1],z[o1+1],z[o2+1],z[o3+1],wre[1],wim[-1]);\
    } while(--n);\
}

PASS(pass)
#undef BUTTERFLIES
#define BUTTERFLIES BUTTERFLIES_BIG
PASS(pass_big)

#define DECL_FFT_AAC(n,n2,n4)\
static void aac_fft##n(AAC_FFTComplex_fix *z)\
{\
    aac_fft##n2(z);\
    aac_fft##n4(z+n4*2);\
    aac_fft##n4(z+n4*3);\
    pass(z,aac_cos_##n,n4/2);\
}

static void aac_fft4(AAC_FFTComplex_fix *z)
{
#if 0
    int32_t t1, t2, t3, t4, t5, t6, t7, t8;

    BF(t3, t1, z[0].re, z[1].re);
    BF(t8, t6, z[3].re, z[2].re);
    BF(z[2].re, z[0].re, t1, t6);
    BF(t4, t2, z[0].im, z[1].im);
    BF(t7, t5, z[2].im, z[3].im);
    BF(z[3].im, z[1].im, t4, t8);
    BF(z[3].re, z[1].re, t3, t7);
    BF(z[2].im, z[0].im, t2, t5);
#else
   uint32_t tm4=(uint32_t)(z);
   S32LDI(xr1, tm4, 0);        
   S32LDI(xr2, tm4, 8);       
   S32LDI(xr3, tm4, 8);      
   S32LDI(xr4, tm4, 8);

   D32ADD_SA(xr5, xr1, xr2, xr6);   
   D32ADD_SA(xr7, xr4, xr3, xr8);  
   D32ADD_SA(xr9, xr6, xr8, xr10);

   S32SDI(xr9,tm4,-8);         
   S32SDI(xr10,tm4,-16);

   S32LDI(xr1, tm4, 4);      
   S32LDI(xr2, tm4, 8);     
   S32LDI(xr3, tm4, 8);    
   S32LDI(xr4, tm4, 8);  

   D32ADD_SA(xr6, xr1, xr2, xr9);   
   D32ADD_SA(xr8, xr3, xr4, xr10);  

   D32ADD_SA(xr1,xr6,xr7,xr2); 
   D32ADD_SA(xr11,xr5,xr8,xr12);   
   D32ADD_SA(xr13,xr9,xr10,xr14);  

   S32SDI(xr1,tm4,0);              
   S32SDI(xr11,tm4,-4);             
   S32SDI(xr13, tm4,-4);             
   S32SDI(xr2,tm4,-8);             
   S32SDI(xr12,tm4,-4);             
   S32SDI(xr14, tm4,-4);
#endif
}

static void aac_fft8(AAC_FFTComplex_fix *z)
{
    int32_t t1, t2, t3, t4, t5, t6, t7, t8;

    aac_fft4(z);
#if 0
    BF(t1, z[5].re, z[4].re, -z[5].re);

    BF(t2, z[5].im, z[4].im, -z[5].im);

    BF(t3, z[7].re, z[6].re, -z[7].re);
    BF(t4, z[7].im, z[6].im, -z[7].im);
    BF(t8, t1, t3, t1);

    BF(t7, t2, t2, t4);

    BF(z[4].re, z[0].re, z[0].re, t1);
    BF(z[4].im, z[0].im, z[0].im, t2);
    BF(z[6].re, z[2].re, z[2].re, t7);
    BF(z[6].im, z[2].im, z[2].im, t8);
#else
   uint32_t tm8=(uint32_t)(z);
   S32LDI(xr1, tm8, 32);        
   S32LDI(xr2, tm8, 4);       
   S32LDI(xr3, tm8, 4);      
   S32LDI(xr4, tm8, 4);

   D32ADD_AS(xr5, xr1, xr3, xr6);   
   D32ADD_AS(xr7, xr2, xr4, xr8);  
    
   S32SDI(xr6,tm8,-4);         
   S32SDI(xr8,tm8,4);
   t3 = S32M2I(xr5);
   t4 = S32M2I(xr7);
   S32LDI(xr1, tm8, 4);      
   S32LDI(xr2, tm8, 4);     
   S32LDI(xr3, tm8, 4);    
   S32LDI(xr4, tm8, 4);  

   D32ADD_AS(xr6, xr1, xr3, xr9);   
   D32ADD_AS(xr8, xr2, xr4, xr10);  
   S32SDI(xr9,tm8,-4);         
   S32SDI(xr10,tm8,4);

   D32ADD_SA(xr11, xr6, xr5, xr12);
   D32ADD_SA(xr13, xr7, xr8, xr14);
   t8 =  S32M2I(xr11);
   t1 =  S32M2I(xr12);
   t7 =  S32M2I(xr13);
   t2 =  S32M2I(xr14);
   S32LDI(xr1, tm8, -60);      
   S32LDI(xr2, tm8, 4);     
   S32LDI(xr3, tm8, 12);    
   S32LDI(xr4, tm8, 4);  

   D32ADD_SA(xr5, xr1, xr12, xr6);   
   D32ADD_SA(xr7, xr2, xr14, xr8);  
   
   D32ADD_SA(xr9,xr3,xr13,xr10); 
   D32ADD_SA(xr1,xr4,xr11,xr2);   

   S32SDI(xr2,tm8,0);              
   S32SDI(xr10,tm8,-4);             
   S32SDI(xr8, tm8,-12);             
   S32SDI(xr6,tm8,-4);             
   S32SDI(xr5,tm8,32);             
   S32SDI(xr7, tm8,4);   
   S32SDI(xr9, tm8,12);   
   S32SDI(xr1, tm8,4);   
#endif

   TRANSFORM_FIX(z[1],z[3],z[5],z[7],1518500250,1518500250);
}
#if !CONFIG_SMALL
static void aac_fft16(AAC_FFTComplex_fix *z)
{
    int32_t t1, t2, t3, t4, t5, t6;

    aac_fft8(z);
    aac_fft4(z+8);
    aac_fft4(z+12);
    TRANSFORM_ZERO(z[0],z[4],z[8],z[12]);
    TRANSFORM_FIX(z[2],z[6],z[10],z[14],1518500250,1518500250);
    TRANSFORM_FIX(z[1],z[5],z[9],z[13],aac_cos_16[1],aac_cos_16[3]);
    TRANSFORM_FIX(z[3],z[7],z[11],z[15],aac_cos_16[3],aac_cos_16[1]);
}
#else
DECL_FFT_AAC(16,8,4)
#endif
DECL_FFT_AAC(32,16,8)
DECL_FFT_AAC(64,32,16)
DECL_FFT_AAC(128,64,32)
DECL_FFT_AAC(256,128,64)
DECL_FFT_AAC(512,256,128)
#if !CONFIG_SMALL
#define pass pass_big
#endif
DECL_FFT_AAC(1024,512,256)

static void (* const aac_fft_dispatch[])(AAC_FFTComplex_fix*) = {
  aac_fft4, aac_fft8, aac_fft16, aac_fft32, aac_fft64, aac_fft128, aac_fft256, aac_fft512, aac_fft1024,
    
};
void aac_fft_calc_c(FFTContext *s, AAC_FFTComplex_fix *z)
{
  aac_fft_dispatch[s->nbits-2](z);
#if 0
  if ((s->nbits-2)==7)
    fft512(z);
  else if((s->nbits-2)==4)
    fft64(z); 
#endif
}

