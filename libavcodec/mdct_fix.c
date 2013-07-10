#include "mdct_fix.h"
/**
 * init MDCT or IMDCT computation.
 */
int mdct_init_fix(MDCTContext_fix *s, int nbits, int inverse)
{
    int n, n4, i;
    double alpha;

    memset(s, 0, sizeof(*s));
    n = 1 << nbits;//1<<6 = 64
    s->nbits = nbits;//6
    s->n = n;//64
    n4 = n >> 2;//16
    s->tcos = malloc(n4 * sizeof(FFTSample_fix));
    if (!s->tcos)
        goto fail;
    s->tsin = malloc(n4 * sizeof(FFTSample_fix));
    if (!s->tsin)
        goto fail;
   
    for(i=0;i<n4;i++) { //n4 = 16
        alpha = 2 * M_PI * (i + 1.0 / 8.0) / n;
        s->tcos[i] = -FFT_SAMPLE(cos(alpha));
        s->tsin[i] = -FFT_SAMPLE(sin(alpha));
    }
    if(inverse){
        s->mdct_calc = imdct_calc_fix;
        s->imdct_half = imdct_half_fix_c;
    }else{
        s->mdct_calc = mdct_calc_fix; //need mdct function
    }
    if (fft_init_fix(&s->fft, s->nbits - 2, inverse) < 0)
        goto fail;
    return 0;
 fail:
    free(s->tcos);
    free(s->tsin);
    return -1;
}

#define FFT_CMUL_fix(pre, pim, are, aim, bre, bim) \
  {\
    FFTSample_fix _are = (are);\
    FFTSample_fix _bre = (bre);\
    FFTSample_fix _aim = (aim);\
    FFTSample_fix _bim = (bim);\
    S32MUL(xr1, xr2, _are, _bre); \    
    S32MUL(xr5, xr6, _are, _bim); \
    S32MSUB(xr1, xr2, _aim, _bim); \
    S32MADD(xr5, xr6, _aim, _bre); \
    D32SLL(xr1, xr1, xr5, xr5, 1); \
    (pre) = S32M2I(xr1); \
    (pim) = S32M2I(xr5); \
  }
void imdct_half_fix_c(MDCTContext_fix *s, FFTSample_fix *output,
                   const FFTSample_fix *input)
{   
  //PMON_ON(qmf);
  int k, n8, n4, n2, n, j,j1;
    const FFTSample_fix *in1, *in2;
    const unsigned short *revtab = s->fft.revtab;
    const FFTSample_fix *tcos = s->tcos;
    const FFTSample_fix *tsin = s->tsin;
    FFTComplex_fix *z = (FFTComplex_fix *)output;

    n = 1 << s->nbits;//64
    n2 = n >> 1;//32
    n4 = n >> 2;//16
    n8 = n >> 3;//8
    /* pre rotation */
    in1 = input; //head
    in2 = input + n2 - 1;//tail
    for(k = 0; k < n8; k++) {
#if 0
        j=revtab[k];
        FFT_CMUL_fix(z[j].re, z[j].im, *in2, *in1, tcos[k], tsin[k]);
        in1 += 2;
        in2 -= 2;
#else
        FFTSample_fix _are,_bre,_aim,_bim,are,aim;
        _are = *in2;
        _bre = tcos[k];
	_aim = *in1;
        _bim = tsin[k];
        j=revtab[k];
        n=n4-k-1;
        j1=revtab[n];
        S32MUL(xr1,xr2, _are, _bre);        
        S32MUL(xr3, xr4, _are, _bim);
        in2--;
        are = *in2;
        S32MUL(xr7,xr8, are, _bre);
        S32MUL(xr9, xr10, are, _bim);
        S32MSUB(xr1, xr2, _aim, _bim);
        S32MADD(xr3, xr4, _aim, _bre);        ;
        in1++;
        aim = *in1;
        D32SLL(xr5,xr1,xr3,xr6,1);
        S32MSUB(xr7, xr8, aim, _bim);
        S32MADD(xr9, xr10, aim, _bre);        
        z[j].re=S32M2I(xr5);        
        D32SLL(xr11,xr7,xr9,xr12,1);
        z[j].im=S32M2I(xr6);
        in1++;
        in2--;
        z[j1].re=S32M2I(xr11);
        z[j1].im=S32M2I(xr12);
#endif
    }

    s->fft.fft_calc(&s->fft, z);

    /* post rotation + reordering */
    /* XXX: optimize */
    for(k = 0; k < n8; k++) {
        FFTSample_fix r0, i0, r1, i1;
        FFT_CMUL_fix(r0, i1, z[n8-k-1].im, z[n8-k-1].re, tsin[n8-k-1], tcos[n8-k-1]);
        FFT_CMUL_fix(r1, i0, z[n8+k  ].im, z[n8+k  ].re, tsin[n8+k  ], tcos[n8+k  ]);
        z[n8-k-1].re = r0;
        z[n8-k-1].im = i0;
        z[n8+k  ].re = r1;
        z[n8+k  ].im = i1;
    }
    //PMON_OFF(qmf);
}


/**
 * Compute inverse MDCT of size N = 2^nbits
 * @param output N samples
 * @param input N/2 samples
 * @param tmp N/2 samples
 */
void imdct_calc_fix(MDCTContext_fix *s, FFTSample_fix *output,
                   const FFTSample_fix *input, FFTSample_fix *tmp)
{
   int k, n8, n4, n2, n, j;
    const unsigned short *revtab = s->fft.revtab;
    const FFTSample_fix *tcos = s->tcos;
    const FFTSample_fix *tsin = s->tsin;
    const FFTSample_fix *in1, *in2;
    FFTComplex_fix *z = (FFTComplex_fix *)tmp;

    n = 1 << s->nbits;
    n2 = n >> 1;
    n4 = n >> 2;
    n8 = n >> 3;

    /* pre rotation */
    in1 = input;
    in2 = input + n2 - 1;
    for(k = 0; k < n4; k++) {
        j=revtab[k];
        FFT_CMUL_fix(z[j].re, z[j].im, *in2, *in1, tcos[k], tsin[k]);
        in1 += 2;
        in2 -= 2;
    }
    s->fft.fft_calc(&s->fft, z);
    /* post rotation + reordering */
    /* XXX: optimize */
    for(k = 0; k < n4; k++) {
        FFT_CMUL_fix(z[k].re, z[k].im, z[k].re, z[k].im, tcos[k], tsin[k]);
    }
    for(k = 0; k < n8; k++) {
        output[2*k] = -z[n8 + k].im;
        output[n2-1-2*k] = z[n8 + k].im;

        output[2*k+1] = z[n8-1-k].re;
        output[n2-1-2*k-1] = -z[n8-1-k].re;

        output[n2 + 2*k]=-z[k+n8].re;
        output[n-1- 2*k]=-z[k+n8].re;

        output[n2 + 2*k+1]=z[n8-k-1].im;
        output[n-2 - 2 * k] = z[n8-k-1].im;
    }
}


/**
 * Compute MDCT of size N = 2^nbits
 * @param input N samples
 * @param out N/2 samples
 * @param tmp temporary storage of N/2 samples
 */
void mdct_calc_fix(MDCTContext_fix *s, FFTSample_fix *out,
                  const FFTSample_fix *input, FFTSample_fix *tmp)
{
    int i, j, n, n8, n4, n2, n3;
    FFTSample_fix re, im, re1, im1;
    const unsigned short *revtab = s->fft.revtab;
    const FFTSample_fix *tcos = s->tcos;
    const FFTSample_fix *tsin = s->tsin;
    FFTComplex_fix *x = (FFTComplex_fix *)tmp;

    n = 1 << s->nbits;
    n2 = n >> 1;
    n4 = n >> 2;
    n8 = n >> 3;
    n3 = 3 * n4;

    /* pre rotation */
    for(i=0;i<n8;i++) {
        re = -input[2*i+3*n4] - input[n3-1-2*i];
        im = -input[n4+2*i] + input[n4-1-2*i];
        j = revtab[i];
        FFT_CMUL_fix(x[j].re, x[j].im, re, im, -tcos[i], tsin[i]);

        re = input[2*i] - input[n2-1-2*i];
        im = -(input[n2+2*i] + input[n-1-2*i]);
        j = revtab[n8 + i];
        FFT_CMUL_fix(x[j].re, x[j].im, re, im, -tcos[n8 + i], tsin[n8 + i]);
    }

    s->fft.fft_calc(&s->fft, x);

    /* post rotation */
    for(i=0;i<n4;i++) {
        re = x[i].re;
        im = x[i].im;
        FFT_CMUL_fix(re1, im1, re, im, -tsin[i], -tcos[i]);
        out[2*i] = im1;
        out[n2-1-2*i] = re1;
    }
}
void mdct_end_fix(MDCTContext_fix *s)
{
    free(s->tcos);
    free(s->tsin);
    fft_end_fix(&s->fft);
}
