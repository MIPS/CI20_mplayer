#include "fft_fix.h"
#include "mdct_fix.h"

int fft_init_fix(FFTContext_fix *s, int nbits, int inverse)
{
    int i, j, m, n;
    double alpha, c1, s1, s2;

    s->nbits = nbits;
    n = 1 << nbits;

    s->exptab = malloc((n / 2) * sizeof(FFTComplex_fix));
    if (!s->exptab)
        goto fail;
    s->revtab = malloc(n * sizeof(unsigned short));
    if (!s->revtab)
        goto fail;
    s->inverse = inverse;

    s2 = inverse ? 1.0 : -1.0;

    if(inverse)
      s->fft_calc = fft_calc_fix_inverse;
    else
      s->fft_calc = fft_calc_fix;

    s->imdct_calc = imdct_calc_fix;
    for(i=0;i<(n/2);i++) {
        alpha = 2 * M_PI * (double)i / (double)n;
        c1 = cos(alpha);
        s1 = sin(alpha) * s2;
        s->exptab[i].re = FFT_SAMPLE(c1);
        s->exptab[i].im = FFT_SAMPLE(s1);
    }
    /* compute bit reverse table */

    for(i=0;i<n;i++) {
        m=0;
        for(j=0;j<nbits;j++) {
            m |= ((i >> j) & 1) << (nbits-j-1);
        }
        s->revtab[i]=m;
    }
    return 0;
 fail:
    free(&s->revtab);
    free(&s->exptab);
    return -1;


}
/**
 * Do the permutation needed BEFORE calling ff_fft_calc()
 */
void fft_permute_fix(FFTContext_fix *s, FFTComplex_fix *z)
{
    int j, k, np;
    FFTComplex_fix tmp;
    const unsigned short*revtab = s->revtab;

    /* reverse */
    np = 1 << s->nbits;
    for(j=0;j<np;j++) {
        k = revtab[j];
        if (k < j) {
            tmp = z[k];
            z[k] = z[j];
            z[j] = tmp;
        }
    }
}

void fft_calc_fix_inverse(FFTContext_fix *s, FFTComplex_fix *z)
{
    int ln = s->nbits;
    int j, np, np2;
    int nblocks, nloops;
    register FFTComplex_fix *p, *q;
    FFTComplex_fix *exptab = s->exptab;
    int l;
    FFTSample_fix tmp_re, tmp_im;
    np = 1 << ln;
      /* function is :butterfly  all 4 step ,N=16 */
    /* pass 0 */
#if 0
    p=&z[0];
    j=(np >> 1);
    do {
      /*
	 X(k) = G(k)+H(k)*W  (= e j*0)
      */
        FFT_BF_fix(p[0].re, p[0].im, p[1].re, p[1].im,
           p[0].re, p[0].im, p[1].re, p[1].im);

        p+=2;
    } while (--j);
#endif

    /* pass 1 */
    p=&z[0];
    j=np >> 2;
    do {
#if 1
      S32LDD(xr1,p,0);
      S32LDD(xr2,p,4);
      S32LDD(xr3,p,8);
      S32LDD(xr4,p,12);
      S32LDD(xr5,p,16);
      S32LDD(xr6,p,20);
      S32LDD(xr7,p,24);
      S32LDD(xr8,p,28);
      D32ADD_AS(xr1,xr1,xr3,xr3);
      D32ADD_AS(xr2,xr2,xr4,xr4);
      D32ADD_AS(xr5,xr5,xr7,xr7);
      D32ADD_AS(xr6,xr6,xr8,xr8);
      D32ADD_AS(xr1,xr1,xr5,xr5);
      D32ADD_AS(xr2,xr2,xr6,xr6);
      D32ADD_SA(xr3,xr3,xr8,xr9);
      D32ADD_AS(xr4,xr4,xr7,xr8);
      S32STD(xr1,p,0);
      S32STD(xr2,p,4);
      S32STD(xr3,p,8);
      S32STD(xr4,p,12);
      S32STD(xr5,p,16);
      S32STD(xr6,p,20);
      S32STD(xr9,p,24);
      S32STD(xr8,p,28);
#else
      FFT_BF_fix(p[0].re, p[0].im, p[1].re, p[1].im,
		 p[0].re, p[0].im, p[1].re, p[1].im);
      FFT_BF_fix(p[2].re, p[2].im, p[3].re, p[3].im,
		 p[2].re, p[2].im, p[3].re, p[3].im);

      FFT_BF_fix(p[0].re, p[0].im, p[2].re, p[2].im,
		 p[0].re, p[0].im, p[2].re, p[2].im);
      FFT_BF_fix(p[1].re, p[1].im, p[3].re, p[3].im,
		 p[1].re, p[1].im, -p[3].im, p[3].re);
#endif
      p+=4;
    } while (--j);

    /* pass 2 .. ln-1 */
    nblocks = np >> 3;
    nloops = 1 << 2;
    np2 = np >> 1;
    do {
        p = z;
        q = z + nloops;
        for (j = 0; j < nblocks; ++j) {
#if 1
	  S32LDD(xr1,p,0);
	  S32LDD(xr2,p,4);
	  S32LDD(xr3,q,0);
	  S32LDD(xr4,q,4);
	  D32ADD_AS(xr1,xr1,xr3,xr3);
	  D32ADD_AS(xr2,xr2,xr4,xr4);
	  S32STD(xr1,p,0);
	  S32STD(xr2,p,4);
	  S32STD(xr3,q,0);
	  S32STD(xr4,q,4);
#else
	  FFT_BF_fix(p->re, p->im, q->re, q->im,
		     p->re, p->im, q->re, q->im);
#endif

	  p++;
	  q++;
	  for(l = nblocks; l < np2; l += nblocks) {
	    /* FFT_CMUL_fix( ) fuction is :
	       
	    (-j 2*PI/N *km)
	    H(i) * E
	    */
#if 1
	    FFTSample_fix _are = exptab[l].re;
	    FFTSample_fix _bre = q->re;
	    FFTSample_fix _aim = exptab[l].im;
	    FFTSample_fix _bim = q->im;

	    S32MUL(xr1, xr2, _are, _bre);	    
            S32MUL(xr5, xr6, _are, _bim);
	    S32LDD(xr7,p,0);	    
            S32MSUB(xr1, xr2, _aim, _bim);	
	    S32MADD(xr5, xr6, _aim, _bre);	
	    S32LDD(xr8,p,4);
	    D32SLL(xr1, xr1, xr5, xr5, 1);	

	    D32ADD_AS(xr7,xr7,xr1,xr1);
	    D32ADD_AS(xr8,xr8,xr5,xr5);
	    S32STD(xr7,p,0);
	    S32STD(xr8,p,4);
	    S32STD(xr1,q,0);
	    S32STD(xr5,q,4);

#else
	    FFT_CMUL_fix(tmp_re, tmp_im, exptab[l].re, exptab[l].im, q->re, q->im);
	    FFT_BF_fix(p->re, p->im, q->re, q->im,
		       p->re, p->im, tmp_re, tmp_im);
#endif
	    p++;
	    q++;
	  }
	  p += nloops;
	  q += nloops;
        }
        nblocks = nblocks >> 1;
        nloops = nloops << 1;
    } while (nblocks);
}

void fft_calc_fix(FFTContext_fix *s, FFTComplex_fix *z)
{
    int ln = s->nbits;
    int j, np, np2;
    int nblocks, nloops;
    register FFTComplex_fix *p, *q;
    FFTComplex_fix *exptab = s->exptab;
    int l;
    FFTSample_fix tmp_re, tmp_im;
    np = 1 << ln;
      /* function is :butterfly  all 4 step ,N=16 */
    /* pass 0 */

    p=&z[0];
    j=(np >> 1);
    do {
      /*
	 X(k) = G(k)+H(k)*W  (= e j*0)
      */
        FFT_BF_fix(p[0].re, p[0].im, p[1].re, p[1].im,
           p[0].re, p[0].im, p[1].re, p[1].im);

        p+=2;
    } while (--j);


    /* pass 1 */
    p=&z[0];
    j=np >> 2;
    do {
      FFT_BF_fix(p[0].re, p[0].im, p[2].re, p[2].im,
		 p[0].re, p[0].im, p[2].re, p[2].im);
      FFT_BF_fix(p[1].re, p[1].im, p[3].re, p[3].im,
		 p[1].re, p[1].im, p[3].im, -p[3].re);
      p+=4;
    } while (--j);

    /* pass 2 .. ln-1 */
    nblocks = np >> 3;
    nloops = 1 << 2;
    np2 = np >> 1;
    do {
        p = z;
        q = z + nloops;
        for (j = 0; j < nblocks; ++j) {
            FFT_BF_fix(p->re, p->im, q->re, q->im,
               p->re, p->im, q->re, q->im);

            p++;
            q++;
            for(l = nblocks; l < np2; l += nblocks) {
	      /* FFT_CMUL_fix( ) fuction is :
		 
	      (-j 2*PI/N *km)
	      H(i) * E
	      */
	      
	      FFT_CMUL_fix(tmp_re, tmp_im, exptab[l].re, exptab[l].im, q->re, q->im);
	      FFT_BF_fix(p->re, p->im, q->re, q->im,
			 p->re, p->im, tmp_re, tmp_im);
	      p++;
	      q++;
            }
            p += nloops;
            q += nloops;
        }
        nblocks = nblocks >> 1;
        nloops = nloops << 1;
    } while (nblocks);
}

void fft_end_fix(FFTContext_fix *s)
{
    free(s->revtab);
    free(s->exptab);
}
