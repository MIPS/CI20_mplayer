#ifndef FFT_FIXED_H
#define FFT_FIXED_H

#include <math.h>

#if 0
 #ifndef __LINUX__
  #include "uclib.h"
 #undef memcpy
  #define memcpy uc_memcpy
 #endif
#endif

#include "jzmedia.h"

#define M_PI 3.14159265358979323846

// fixed fft definite
#if 0
typedef double FFTSample;
typedef struct FFTComplex {
    FFTSample re, im;
} FFTComplex;
#endif

typedef int   FFTSample_fix;
typedef struct FFTComplex_fix {
    FFTSample_fix re, im;
} FFTComplex_fix;

struct MDCTContext_fix;

typedef struct FFTContext_fix {
    int nbits;
    int inverse;
    unsigned short *revtab;
    FFTComplex_fix *exptab;
    void (*fft_calc)(struct FFTContext_fix *s, FFTComplex_fix *z);
    void (*imdct_calc)(struct MDCTContext_fix *s, FFTSample_fix *output,
                       const FFTSample_fix *input, FFTSample_fix *tmp);

} FFTContext_fix;
/* butter fly op */

#define FFT_BF_fix(pre, pim, qre, qim, pre1, pim1, qre1, qim1) \
{\
  FFTSample_fix ax, ay, bx, by;\
  bx=pre1;\
  by=pim1;\
  ax=qre1;\
  ay=qim1;\
  pre = (bx + ax);\
  pim = (by + ay);\
  qre = (bx - ax);\
  qim = (by - ay);\
}

#define FFT_FRICTIONS 31
#define FFT_SAMPLE(x)  ((FFTSample_fix)((x)*((1<<FFT_FRICTIONS) -1)))
#ifdef ORI_MUL
#define FFT_MUL_fix(a, b)  (((long long)(a) * (b) + (1<<(FFT_FRICTIONS - 1))) >> FFT_FRICTIONS) 
#define FFT_CMUL_fix(pre, pim, are, aim, bre, bim) \
{\
    FFTSample_fix _are = (are);\
    FFTSample_fix _aim = (aim);\
    FFTSample_fix _bre = (bre);\
    FFTSample_fix _bim = (bim);\
    (pre) = FFT_MUL_fix(_are, _bre) - FFT_MUL_fix(_aim, _bim);\
    (pim) = FFT_MUL_fix(_are, _bim) + FFT_MUL_fix(_aim, _bre);\
}
#else
#if 1
#define FFT_CMUL_fix(pre, pim, are, aim, bre, bim) \
  {\
    FFTSample_fix _are = (are);\
    FFTSample_fix _bre = (bre);\
    FFTSample_fix _aim = (aim);\
    FFTSample_fix _bim = (bim);\
    asm(".set noreorder\n");   \
    S32MUL(xr1, xr2, _are, _bre); \
    S32MSUB(xr1, xr2, _aim, _bim); \
    S32MUL(xr5, xr6, _are, _bim); \
    S32MADD(xr5, xr6, _aim, _bre); \
    D32SLL(xr1, xr1, xr5, xr5, 1); \
    (pre) = S32M2I(xr1); \
    (pim) = S32M2I(xr5); \
    asm(".set reorder\n");   \
  }
#else

#define FFT_MUL_fix(a, b)  \
({                         \
   long hi;                \
   unsigned long lo;            \
   asm __volatile("mult %2,%3"      \
             :"=h"(hi),"=l"(lo)      \
             :"%r"(a), "r"(b));     \
   ((hi<<(32-FFT_FRICTIONS))|(lo >>FFT_FRICTIONS)); \
})
#define FFT_CMUL_fix(pre, pim, are, aim, bre, bim) \
{\
    long hi;                                       \
    unsigned long lo;                              \
    FFTSample_fix _are = (are);\
    FFTSample_fix _aim = (aim);\
    FFTSample_fix _bre = (bre);\
    FFTSample_fix _bim = (bim);\
    asm ("mult %2,%3\n"                            \
         "msub %4,%5\n"                            \
         :"=h"(hi),"=l"(lo)                        \
         :"r"(_are),"r"(_bre),"r"(_aim), "r"(_bim));   \
    (pre) = ((hi<<(32-FFT_FRICTIONS))|(lo >>FFT_FRICTIONS)); \
    asm ("mult %2,%3\n"                            \
         "madd %4,%5\n"                            \
         :"=h"(hi),"=l"(lo)                        \
         :"r"(_are),"r"(_bim),"r"(_aim), "r"(_bre));   \
    (pim) = ((hi<<(32-FFT_FRICTIONS))|(lo >>FFT_FRICTIONS)); \
}
#endif
#endif

int  fft_init_fix(FFTContext_fix *s, int nbits, int inverse);
void fft_permute_fix(FFTContext_fix *s, FFTComplex_fix *z);
void fft_calc_fix_inverse(FFTContext_fix *s, FFTComplex_fix *z);
void fft_calc_fix(FFTContext_fix *s, FFTComplex_fix *z);
void fft_end_fix(FFTContext_fix *s);
#endif //FFT_FIXED_H

