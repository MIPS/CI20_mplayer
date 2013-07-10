#ifndef MDCT_FIXED_H
#define MDCT_FIXED_H
#include "fft_fix.h"


/***************************************************************************************/
/* MDCT computation */

typedef struct MDCTContext_fix {
    int n;  /* size of MDCT (i.e. number of input data * 2) */
    int nbits; /* n = 2^nbits */
    /* pre/post rotation tables */
    FFTSample_fix *tcos;
    FFTSample_fix *tsin;
    FFTContext_fix fft;
    void (*mdct_calc)(struct MDCTContext_fix *s, FFTSample_fix *output,
                   const FFTSample_fix *input, FFTSample_fix *tmp);
    void (*imdct_half)(struct MDCTContext_fix *s, FFTSample_fix *output,
                   const FFTSample_fix *input);
} MDCTContext_fix;

int mdct_init_fix(MDCTContext_fix *s, int nbits, int inverse);

void imdct_calc_fix(MDCTContext_fix *s, FFTSample_fix *output,
                   const FFTSample_fix *input, FFTSample_fix *tmp);
void imdct_half_fix_c(MDCTContext_fix *s, FFTSample_fix *output,
                   const FFTSample_fix *input);
void mdct_calc_fix(MDCTContext_fix *s, FFTSample_fix *out,
                  const FFTSample_fix *input, FFTSample_fix *tmp);

void mdct_end_fix(MDCTContext_fix *s);
static inline void ff_imdct_half_fix(MDCTContext_fix *s, FFTSample_fix *output, const FFTSample_fix *input)
{
    s->imdct_half(s, output, input);
}
#endif  //MDCT_FIXED_H
