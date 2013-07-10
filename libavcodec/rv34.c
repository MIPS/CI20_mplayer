/*
 * RV30/40 decoder common data
 * Copyright (c) 2007 Mike Melanson, Konstantin Shishkov
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
 * RV30/40 decoder common data
 */

#include "avcodec.h"
#include "dsputil.h"
#include "mpegvideo.h"
#include "golomb.h"
#include "mathops.h"
#include "rectangle.h"

#include "rv34vlc.h"
#include "rv34data.h"
#include "rv34.h"
#define JZC_MXU_OPT
#define JZC_VLC_HW_OPT
#ifdef JZC_MXU_OPT
#include "../libjzcommon/jzmedia.h"
#endif
#include "../libjzcommon/jz4760e_idct_hw.h"

//#define JZC_CRC_VER
#ifdef JZC_CRC_VER
#include "../libjzcommon/crc.c"
short crc_code = 0;
int rvFrame = 0;
#endif//JZC_CRC_VER

//#define JZC_PMON_P0
//#define STA_CCLK
//#define STA_DCC
#ifdef JZC_PMON_P0
#include "../libjzcommon/jz4760e_pmon.h"
FILE *pmon_p0_fp;
extern char* filename;
int rv8_pmon_p0_frmcnt = 0;
PMON_CREAT(rv8vlc);
PMON_CREAT(rv8wait);
PMON_CREAT(rv8while);
long long all_pmon_val = 0;
#endif//JZC_PMON_P0
int frame_num = 0;
#include "rv30_tcsm0.h"
int *tcsm_blk=COEF_BUF1;
int *tcsm_blk2=COEF_BUF2;
int *idct_out=IDCT_OUT1;
int *idct_out2=IDCT_OUT2;
uint32_t *idct_chain_tab=TCSM0_VCADDR(IDCT_DES_CHAIN);
//#define DEBUG
#define XCHG(a,b)   {uint32_t t=a;a=b;b=t;}
#undef printf
#undef fprintf

//#define MC_USE_TCSM

static void ptr_square(void * start_ptr,int size,int h,int w, int stride){
  unsigned int* start_int=(int*)start_ptr;
  unsigned short* start_short=(short*)start_ptr;
  unsigned char* start_byte=(char*)start_ptr;
  int i, j;
  if(size==4){
    for(i=0;i<h;i++){
      for(j=0;j<w;j++){
	printf("0x%08x,",start_int[i*stride+j]);
      }
      printf("\n");
    }
  }
  if(size==2){
    for(i=0;i<h;i++){
      for(j=0;j<w;j++){
	printf("0x%04x,",start_short[i*stride+j]);
      }
      printf("\n");
    }
  }
  if(size==1){
    for(i=0;i<h;i++){
      for(j=0;j<w;j++){
	printf("0x%02x,",start_byte[i*stride+j]);
      }
      printf("\n");
    }
  }
}

static inline void ZERO8x2(void* dst, int stride)
{
  fill_rectangle(dst,                 1, 2, stride, 0, 4);
  fill_rectangle(((uint8_t*)(dst))+4, 1, 2, stride, 0, 4);
}

/** translation of RV30/40 macroblock types to lavc ones */
static const int rv34_mb_type_to_lavc[12] = {
    MB_TYPE_INTRA,
    MB_TYPE_INTRA16x16              | MB_TYPE_SEPARATE_DC,
    MB_TYPE_16x16   | MB_TYPE_L0,
    MB_TYPE_8x8     | MB_TYPE_L0,
    MB_TYPE_16x16   | MB_TYPE_L0,
    MB_TYPE_16x16   | MB_TYPE_L1,
    MB_TYPE_SKIP,
    MB_TYPE_DIRECT2 | MB_TYPE_16x16,
    MB_TYPE_16x8    | MB_TYPE_L0,
    MB_TYPE_8x16    | MB_TYPE_L0,
    MB_TYPE_16x16   | MB_TYPE_L0L1,
    MB_TYPE_16x16   | MB_TYPE_L0    | MB_TYPE_SEPARATE_DC
};


static RV34VLC intra_vlcs[NUM_INTRA_TABLES], inter_vlcs[NUM_INTER_TABLES];

/**
 * @defgroup vlc RV30/40 VLC generating functions
 * @{
 */

static const int table_offs[] = {
      0,   1818,   3622,   4144,   4698,   5234,   5804,   5868,   5900,   5932,
   5996,   6252,   6316,   6348,   6380,   7674,   8944,  10274,  11668,  12250,
  14060,  15846,  16372,  16962,  17512,  18148,  18180,  18212,  18244,  18308,
  18564,  18628,  18660,  18692,  20036,  21314,  22648,  23968,  24614,  26384,
  28190,  28736,  29366,  29938,  30608,  30640,  30672,  30704,  30768,  31024,
  31088,  31120,  31184,  32570,  33898,  35236,  36644,  37286,  39020,  40802,
  41368,  42052,  42692,  43348,  43380,  43412,  43444,  43476,  43604,  43668,
  43700,  43732,  45100,  46430,  47778,  49160,  49802,  51550,  53340,  53972,
  54648,  55348,  55994,  56122,  56154,  56186,  56218,  56346,  56410,  56442,
  56474,  57878,  59290,  60636,  62036,  62682,  64460,  64524,  64588,  64716,
  64844,  66076,  67466,  67978,  68542,  69064,  69648,  70296,  72010,  72074,
  72138,  72202,  72330,  73572,  74936,  75454,  76030,  76566,  77176,  77822,
  79582,  79646,  79678,  79742,  79870,  81180,  82536,  83064,  83672,  84242,
  84934,  85576,  87384,  87448,  87480,  87544,  87672,  88982,  90340,  90902,
  91598,  92182,  92846,  93488,  95246,  95278,  95310,  95374,  95502,  96878,
  98266,  98848,  99542, 100234, 100884, 101524, 103320, 103352, 103384, 103416,
 103480, 104874, 106222, 106910, 107584, 108258, 108902, 109544, 111366, 111398,
 111430, 111462, 111494, 112878, 114320, 114988, 115660, 116310, 116950, 117592
};

static VLC_TYPE table_data[117592][2];

/**
 * Generate VLC from codeword lengths.
 * @param bits   codeword lengths (zeroes are accepted)
 * @param size   length of input data
 * @param vlc    output VLC
 * @param insyms symbols for input codes (NULL for default ones)
 * @param num    VLC table number (for static initialization)
 */
static void rv34_gen_vlc(const uint8_t *bits, int size, VLC *vlc, const uint8_t *insyms,
                         const int num)
{
    int i;
    int counts[17] = {0}, codes[17];
    uint16_t cw[MAX_VLC_SIZE], syms[MAX_VLC_SIZE];
    uint8_t bits2[MAX_VLC_SIZE];
    int maxbits = 0, realsize = 0;

    for(i = 0; i < size; i++){
        if(bits[i]){
            bits2[realsize] = bits[i];
            syms[realsize] = insyms ? insyms[i] : i;
            realsize++;
            maxbits = FFMAX(maxbits, bits[i]);
            counts[bits[i]]++;
        }
    }

    codes[0] = 0;
    for(i = 0; i < 16; i++)
        codes[i+1] = (codes[i] + counts[i]) << 1;
    for(i = 0; i < realsize; i++)
        cw[i] = codes[bits2[i]]++;

    vlc->table = &table_data[table_offs[num]];
    vlc->table_allocated = table_offs[num + 1] - table_offs[num];
    init_vlc_sparse(vlc, FFMIN(maxbits, 9), realsize,
                    bits2, 1, 1,
                    cw,    2, 2,
                    syms,  2, 2, INIT_VLC_USE_NEW_STATIC);
}

/**
 * Initialize all tables.
 */
static av_cold void rv34_init_tables(void)
{
    int i, j, k;

    for(i = 0; i < NUM_INTRA_TABLES; i++){
        for(j = 0; j < 2; j++){
            rv34_gen_vlc(rv34_table_intra_cbppat   [i][j], CBPPAT_VLC_SIZE,   &intra_vlcs[i].cbppattern[j],     NULL, 19*i + 0 + j);
            rv34_gen_vlc(rv34_table_intra_secondpat[i][j], OTHERBLK_VLC_SIZE, &intra_vlcs[i].second_pattern[j], NULL, 19*i + 2 + j);
            rv34_gen_vlc(rv34_table_intra_thirdpat [i][j], OTHERBLK_VLC_SIZE, &intra_vlcs[i].third_pattern[j],  NULL, 19*i + 4 + j);
            for(k = 0; k < 4; k++){
                rv34_gen_vlc(rv34_table_intra_cbp[i][j+k*2],  CBP_VLC_SIZE,   &intra_vlcs[i].cbp[j][k],         rv34_cbp_code, 19*i + 6 + j*4 + k);
            }
        }
        for(j = 0; j < 4; j++){
            rv34_gen_vlc(rv34_table_intra_firstpat[i][j], FIRSTBLK_VLC_SIZE, &intra_vlcs[i].first_pattern[j], NULL, 19*i + 14 + j);
        }
        rv34_gen_vlc(rv34_intra_coeff[i], COEFF_VLC_SIZE, &intra_vlcs[i].coefficient, NULL, 19*i + 18);
    }

    for(i = 0; i < NUM_INTER_TABLES; i++){
        rv34_gen_vlc(rv34_inter_cbppat[i], CBPPAT_VLC_SIZE, &inter_vlcs[i].cbppattern[0], NULL, i*12 + 95);
        for(j = 0; j < 4; j++){
            rv34_gen_vlc(rv34_inter_cbp[i][j], CBP_VLC_SIZE, &inter_vlcs[i].cbp[0][j], rv34_cbp_code, i*12 + 96 + j);
        }
        for(j = 0; j < 2; j++){
            rv34_gen_vlc(rv34_table_inter_firstpat [i][j], FIRSTBLK_VLC_SIZE, &inter_vlcs[i].first_pattern[j],  NULL, i*12 + 100 + j);
            rv34_gen_vlc(rv34_table_inter_secondpat[i][j], OTHERBLK_VLC_SIZE, &inter_vlcs[i].second_pattern[j], NULL, i*12 + 102 + j);
            rv34_gen_vlc(rv34_table_inter_thirdpat [i][j], OTHERBLK_VLC_SIZE, &inter_vlcs[i].third_pattern[j],  NULL, i*12 + 104 + j);
        }
        rv34_gen_vlc(rv34_inter_coeff[i], COEFF_VLC_SIZE, &inter_vlcs[i].coefficient, NULL, i*12 + 106);
    }
}

/** @} */ // vlc group
#define op_avg(a, b) a = (((a)+(((b) + 32)>>6)+1)>>1)
#define op_put(a, b) a = (((b) + 32)>>6)

static void put_rv30_chroma_mc2_c(uint8_t *dst/*align 8*/, uint8_t *src/*align 1*/, int stride, int h, int x, int y){
#ifdef JZC_PMON_P0
  //PMON_ON(rv8vlc);
#endif
    const int A=(8-x)*(8-y);
    const int B=(  x)*(8-y);
    const int C=(8-x)*(  y);
    const int D=(  x)*(  y);
    int i;
    //assert(x<8 && y<8 && x>=0 && y>=0);

    if(D){
        for(i=0; i<h; i++){
            op_put(dst[0], (A*src[0] + B*src[1] + C*src[stride+0] + D*src[stride+1]));
            op_put(dst[1], (A*src[1] + B*src[2] + C*src[stride+1] + D*src[stride+2]));
#ifdef MC_USE_TCSM
	    dst+= 16;
#else
            dst+= stride;
#endif
            src+= stride;
        }
    }else{
        const int E= B+C;
        const int step= C ? stride : 1;
        for(i=0; i<h; i++){
            op_put(dst[0], (A*src[0] + E*src[step+0]));
            op_put(dst[1], (A*src[1] + E*src[step+1]));
#ifdef MC_USE_TCSM
	    dst+= 16;
#else
            dst+= stride;
#endif
            src+= stride;
        }
    }
#ifdef JZC_PMON_P0
    //PMON_OFF(rv8vlc);
#endif
}

static void put_rv30_chroma_mc4_c(uint8_t *dst/*align 8*/, uint8_t *src/*align 1*/, int stride, int h, int x, int y){
#ifdef JZC_PMON_P0
  //PMON_ON(rv8vlc);
#endif
  const int A=(8-x)*(8-y);
  const int B=(  x)*(8-y);
  const int C=(8-x)*(  y);
  const int D=(  x)*(  y);
  int i;
  //assert(x<8 && y<8 && x>=0 && y>=0);
  uint8_t qbuf[5];
  qbuf[0] = A;
  qbuf[1] = B;
  qbuf[2] = C;
  qbuf[3] = D;
  qbuf[4] = B+C;
    
  S32I2M(xr11,0x00200020);
  S8LDD(xr12,qbuf,0,7);
  S8LDD(xr13,qbuf,1,7);
  S8LDD(xr14,qbuf,2,7);
  S8LDD(xr15,qbuf,3,7);
  S8LDD(xr10,qbuf,4,7);

  if(D){
    for(i=0; i<h; i++){
#if 1
      S8LDD(xr1,src,0,3);S8LDD(xr3,&src[stride],0,3);
      S8LDD(xr1,src,1,2);S8LDD(xr3,&src[stride],1,2);
      S8LDD(xr1,src,2,1);S8LDD(xr3,&src[stride],2,1);
      S8LDD(xr1,src,3,0);S8LDD(xr3,&src[stride],3,0);
      D32SLL(xr2,xr1,xr3,xr4,8);
      S8LDD(xr2,src,4,0);S8LDD(xr4,&src[stride],4,0);

      Q8MUL(xr5,xr1,xr12,xr6);Q8MUL(xr7,xr2,xr13,xr8);
      Q8MUL(xr9,xr3,xr14,xr10);Q8MUL(xr1,xr4,xr15,xr2);

      Q16ACCM_AA(xr5,xr7,xr8,xr6);
      Q16ACCM_AA(xr9,xr1,xr2,xr10);
      Q16ACCM_AA(xr5,xr11,xr11,xr6);
      Q16ACCM_AA(xr5,xr9,xr10,xr6);
      Q16SAR(xr5,xr5,xr6,xr6,6);
      Q16SAT(xr9,xr5,xr6);
      S32STDR(xr9,dst,0);
#else
      op_put(dst[0], (A*src[0] + B*src[1] + C*src[stride+0] + D*src[stride+1]));
      op_put(dst[1], (A*src[1] + B*src[2] + C*src[stride+1] + D*src[stride+2]));
      op_put(dst[2], (A*src[2] + B*src[3] + C*src[stride+2] + D*src[stride+3]));
      op_put(dst[3], (A*src[3] + B*src[4] + C*src[stride+3] + D*src[stride+4]));
      //printf("0x%08x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\n", S32M2I(xr9), dst[3], dst[2], dst[1], dst[0]);
#endif
#ifdef MC_USE_TCSM
      dst+= 16;
#else
      dst+= stride;
#endif
      src+= stride;
    }
  }else{
    const int E= B+C;
    const int step= C ? stride : 1;
    //printf("A is %d, E is %d\n", A, E);
    //printf("step is %d\n", step);
    //printf("qbuf[3] is %d\n", qbuf[3]);
    //printf("qbuf[1] is %d, xr14:%d\n", qbuf[1], S32M2I(xr14));
    //printf("E is %d xr15 0x%08x\n", E, S32M2I(xr15));
    for(i=0; i<h; i++){
#if 1
      //printf("E is %d xr10 0x%08x\n", E, S32M2I(xr10));
      S8LDD(xr1,src,0,3);S8LDD(xr2,&src[step+0],0,3);
      S8LDD(xr1,src,1,2);S8LDD(xr2,&src[step+1],0,2);
      S8LDD(xr1,src,2,1);S8LDD(xr2,&src[step+2],0,1);
      S8LDD(xr1,src,3,0);S8LDD(xr2,&src[step+3],0,0);
      //printf("11 0x%08x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\n", S32M2I(xr2), src[step+0], src[step+1], src[step+2], src[step+3]);
      //printf("22 0x%08x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\n", S32M2I(xr1), src[0], src[1], src[2], src[3]);
      //printf("A is %d xr12 0x%08x\n", A, S32M2I(xr12));
      Q8MUL(xr5,xr1,xr12,xr6);
      //printf("xr5 0x%08x, xr6 0x%08x\n", S32M2I(xr5), S32M2I(xr6));
      
      //printf("qbuf[3] is %d\n", qbuf[3]);
      Q8MUL(xr7,xr2,xr10,xr8);
      Q16ACCM_AA(xr5,xr11,xr11,xr6);
      Q16ACCM_AA(xr5,xr7,xr8,xr6);
      Q16SAR(xr5,xr5,xr6,xr6,6);
      Q16SAT(xr9,xr5,xr6);
      S32STDR(xr9,dst,0);
#else
      op_put(dst[0], (A*src[0] + E*src[step+0]));
      op_put(dst[1], (A*src[1] + E*src[step+1]));
      op_put(dst[2], (A*src[2] + E*src[step+2]));
      op_put(dst[3], (A*src[3] + E*src[step+3]));
      //printf("0x%08x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\n", S32M2I(xr9), dst[3], dst[2], dst[1], dst[0]);
#endif
#ifdef MC_USE_TCSM
      dst+= 16;
#else
      dst+= stride;
#endif
      src+= stride;
    }
  }
#ifdef JZC_PMON_P0
  //PMON_OFF(rv8vlc);
#endif
}

static void put_rv30_chroma_mc8_c(uint8_t *dst/*align 8*/, uint8_t *src/*align 1*/, int stride, int h, int x, int y){
#ifdef JZC_PMON_P0
  //PMON_ON(rv8vlc);
#endif
  //const int A=(8-x)*(8-y);
  //const int B=(  x)*(8-y);
  //const int C=(8-x)*(  y);
  //const int D=(  x)*(  y);
  int i;
  //assert(x<8 && y<8 && x>=0 && y>=0);
  uint8_t qbuf[6];
  qbuf[0] = (8-x)*(8-y);
  qbuf[1] = (  x)*(8-y);
  qbuf[2] = (8-x)*(  y);
  qbuf[3] = (  x)*(  y);
  qbuf[4] = qbuf[1]+qbuf[2];

  S32I2M(xr11,0x00200020);
  S8LDD(xr12,qbuf,0,7);
  S8LDD(xr13,qbuf,1,7);
  S8LDD(xr14,qbuf,2,7);
  S8LDD(xr15,qbuf,3,7);
  S8LDD(xr10,qbuf,4,7);

  if(qbuf[3]){
    for(i=0; i<h; i++){
#if 1
      S8LDD(xr1,src,0,3);S8LDD(xr3,&src[stride],0,3);
      S8LDD(xr1,src,1,2);S8LDD(xr3,&src[stride],1,2);
      S8LDD(xr1,src,2,1);S8LDD(xr3,&src[stride],2,1);
      S8LDD(xr1,src,3,0);S8LDD(xr3,&src[stride],3,0);
      D32SLL(xr2,xr1,xr3,xr4,8);
      S8LDD(xr2,src,4,0);S8LDD(xr4,&src[stride],4,0);

      Q8MUL(xr5,xr1,xr12,xr6);Q8MUL(xr7,xr2,xr13,xr8);
      Q8MUL(xr9,xr3,xr14,xr10);Q8MUL(xr1,xr4,xr15,xr2);

      Q16ACCM_AA(xr5,xr7,xr8,xr6);
      S8LDD(xr3,&src[stride],4,3);//
      Q16ACCM_AA(xr9,xr1,xr2,xr10);
      S8LDD(xr1,src,4,3);//
      Q16ACCM_AA(xr5,xr11,xr11,xr6);
      S8LDD(xr3,&src[stride],5,2);//
      Q16ACCM_AA(xr5,xr9,xr10,xr6);
      S8LDD(xr1,src,5,2);//
      Q16SAR(xr5,xr5,xr6,xr6,6);
      S8LDD(xr3,&src[stride],6,1);//
      Q16SAT(xr9,xr5,xr6);
      //printf("xr9:0x%08x\n", S32M2I(xr9));
      S8LDD(xr1,src,6,1);//
      S32STDR(xr9,dst,0);
      S8LDD(xr3,&src[stride],7,0);//
      S8LDD(xr1,src,7,0);//

      D32SLL(xr2,xr1,xr3,xr4,8);
      S8LDD(xr2,src,8,0);S8LDD(xr4,&src[stride],8,0);

      Q8MUL(xr5,xr1,xr12,xr6);Q8MUL(xr7,xr2,xr13,xr8);
      Q8MUL(xr9,xr3,xr14,xr10);Q8MUL(xr1,xr4,xr15,xr2);

      Q16ACCM_AA(xr5,xr7,xr8,xr6);
      Q16ACCM_AA(xr9,xr1,xr2,xr10);
      Q16ACCM_AA(xr5,xr11,xr11,xr6);
      Q16ACCM_AA(xr5,xr9,xr10,xr6);
      Q16SAR(xr5,xr5,xr6,xr6,6);
      Q16SAT(xr9,xr5,xr6);
      S32STDR(xr9,dst,4);
      //op_put(qbuf[5], (src[7] + src[8] + C*src[stride+7] + D*src[stride+8]));
      //op_put(qbuf[5], (A*src[6] + B*src[7] + C*src[stride+6] + D*src[stride+7]));
#else
      op_put(dst[0], (A*src[0] + B*src[1] + C*src[stride+0] + D*src[stride+1]));
      op_put(dst[1], (A*src[1] + B*src[2] + C*src[stride+1] + D*src[stride+2]));
      op_put(dst[2], (A*src[2] + B*src[3] + C*src[stride+2] + D*src[stride+3]));
      op_put(dst[3], (A*src[3] + B*src[4] + C*src[stride+3] + D*src[stride+4]));
      op_put(dst[4], (A*src[4] + B*src[5] + C*src[stride+4] + D*src[stride+5]));
      op_put(dst[5], (A*src[5] + B*src[6] + C*src[stride+5] + D*src[stride+6]));
      op_put(dst[6], (A*src[6] + B*src[7] + C*src[stride+6] + D*src[stride+7]));
      op_put(dst[7], (A*src[7] + B*src[8] + C*src[stride+7] + D*src[stride+8]));
#endif
#ifdef MC_USE_TCSM
      dst+= 16;
#else
      dst+= stride;
#endif
      src+= stride;
    }
  }else{
    //const int E= B+C;
    const int step= qbuf[2] ? stride : 1;
    for(i=0; i<h; i++){
#if 1
      S8LDD(xr1,src,0,3);S8LDD(xr2,&src[step+0],0,3);
      S8LDD(xr1,src,1,2);S8LDD(xr2,&src[step+1],0,2);
      S8LDD(xr1,src,2,1);S8LDD(xr2,&src[step+2],0,1);
      S8LDD(xr1,src,3,0);S8LDD(xr2,&src[step+3],0,0);

      Q8MUL(xr5,xr1,xr12,xr6);
      Q8MUL(xr7,xr2,xr10,xr8);

      S8LDD(xr1,src,4,3);//
      Q16ACCM_AA(xr5,xr11,xr11,xr6);
      S8LDD(xr2,&src[step],4,3);//
      Q16ACCM_AA(xr5,xr7,xr8,xr6);
      S8LDD(xr1,src,5,2);//
      Q16SAR(xr5,xr5,xr6,xr6,6);
      S8LDD(xr2,&src[step],5,2);//
      Q16SAT(xr9,xr5,xr6);
      S8LDD(xr1,src,6,1);//
      S32STDR(xr9,dst,0);
      S8LDD(xr2,&src[step],6,1);//

      S8LDD(xr1,src,7,0);S8LDD(xr2,&src[step],7,0);

      Q8MUL(xr5,xr1,xr12,xr6);
      Q8MUL(xr7,xr2,xr10,xr8);

      Q16ACCM_AA(xr5,xr11,xr11,xr6);
      Q16ACCM_AA(xr5,xr7,xr8,xr6);
      Q16SAR(xr5,xr5,xr6,xr6,6);
      Q16SAT(xr9,xr5,xr6);
      S32STDR(xr9,dst,4);
      //op_put(qbuf[5], (src[7] + src[step+7]));
#else
      op_put(dst[0], (A*src[0] + E*src[step+0]));
      op_put(dst[1], (A*src[1] + E*src[step+1]));
      op_put(dst[2], (A*src[2] + E*src[step+2]));
      op_put(dst[3], (A*src[3] + E*src[step+3]));
      op_put(dst[4], (A*src[4] + E*src[step+4]));
      op_put(dst[5], (A*src[5] + E*src[step+5]));
      op_put(dst[6], (A*src[6] + E*src[step+6]));
      op_put(dst[7], (A*src[7] + E*src[step+7]));
#endif
#ifdef MC_USE_TCSM
      dst+= 16;
#else
      dst+= stride;
#endif
      src+= stride;
    }
  }
#ifdef JZC_PMON_P0
  //PMON_OFF(rv8vlc);
#endif
}

static void avg_rv30_chroma_mc2_c(uint8_t *dst/*align 8*/, uint8_t *src/*align 1*/, int stride, int h, int x, int y){
  const int A=(8-x)*(8-y);
  const int B=(  x)*(8-y);
  const int C=(8-x)*(  y);
  const int D=(  x)*(  y);
  int i;
  //assert(x<8 && y<8 && x>=0 && y>=0);

  if(D){
    for(i=0; i<h; i++){
      op_avg(dst[0], (A*src[0] + B*src[1] + C*src[stride+0] + D*src[stride+1]));
      op_avg(dst[1], (A*src[1] + B*src[2] + C*src[stride+1] + D*src[stride+2]));
#ifdef MC_USE_TCSM
      dst+= 16;
#else
      dst+= stride;
#endif
      src+= stride;
    }
  }else{
    const int E= B+C;
    const int step= C ? stride : 1;
    for(i=0; i<h; i++){
      op_avg(dst[0], (A*src[0] + E*src[step+0]));
      op_avg(dst[1], (A*src[1] + E*src[step+1]));
#ifdef MC_USE_TCSM
      dst+= 16;
#else
      dst+= stride;
#endif
      src+= stride;
    }
  }
}

static void avg_rv30_chroma_mc4_c(uint8_t *dst/*align 8*/, uint8_t *src/*align 1*/, int stride, int h, int x, int y){
  //const int A=(8-x)*(8-y);
  //const int B=(  x)*(8-y);
  const int C=(8-x)*(  y);
  const int D=(  x)*(  y);
  int i;
  //assert(x<8 && y<8 && x>=0 && y>=0);
  uint8_t qbuf[5];
  qbuf[0] = (8-x)*(8-y);
  qbuf[1] = (  x)*(8-y);
  qbuf[2] = C;
  qbuf[3] = D;
  qbuf[4] = qbuf[1]+qbuf[2];

  S32I2M(xr11,0x00200020);
  S8LDD(xr12,qbuf,0,7);
  S8LDD(xr13,qbuf,1,7);
  S8LDD(xr14,qbuf,2,7);
  S8LDD(xr15,qbuf,3,7);
  S8LDD(xr10,qbuf,4,7);

  if(D){
    for(i=0; i<h; i++){
#if 1
      S8LDD(xr1,src,0,3);S8LDD(xr3,&src[stride],0,3);
      S8LDD(xr1,src,1,2);S8LDD(xr3,&src[stride],1,2);
      S8LDD(xr1,src,2,1);S8LDD(xr3,&src[stride],2,1);
      S8LDD(xr1,src,3,0);S8LDD(xr3,&src[stride],3,0);

      D32SLL(xr2,xr1,xr3,xr4,8);
      S8LDD(xr2,src,4,0);S8LDD(xr4,&src[stride],4,0);

      Q8MUL(xr5,xr1,xr12,xr6);Q8MUL(xr7,xr2,xr13,xr8);
      Q8MUL(xr9,xr3,xr14,xr10);Q8MUL(xr1,xr4,xr15,xr2);

      Q16ACCM_AA(xr5,xr7,xr8,xr6);
      Q16ACCM_AA(xr9,xr1,xr2,xr10);
      Q16ACCM_AA(xr5,xr11,xr11,xr6);
      Q16ACCM_AA(xr5,xr9,xr10,xr6);
      Q16SAR(xr5,xr5,xr6,xr6,6);
      Q16SAT(xr9,xr5,xr6);
      S32LDDR(xr8,dst,0);
      Q8AVGR(xr9,xr8,xr9);
      S32STDR(xr9,dst,0);
#else
      op_avg(dst[0], (A*src[0] + B*src[1] + C*src[stride+0] + D*src[stride+1]));
      op_avg(dst[1], (A*src[1] + B*src[2] + C*src[stride+1] + D*src[stride+2]));
      op_avg(dst[2], (A*src[2] + B*src[3] + C*src[stride+2] + D*src[stride+3]));
      op_avg(dst[3], (A*src[3] + B*src[4] + C*src[stride+3] + D*src[stride+4]));
#endif
#ifdef MC_USE_TCSM
      dst+= 16;
#else
      dst+= stride;
#endif
      src+= stride;
    }
  }else{
    //const int E= B+C;
    const int step= C ? stride : 1;
    for(i=0; i<h; i++){
#if 1
      S8LDD(xr1,src,0,3);S8LDD(xr2,&src[step],0,3);
      S8LDD(xr1,src,1,2);S8LDD(xr2,&src[step],1,2);
      S8LDD(xr1,src,2,1);S8LDD(xr2,&src[step],2,1);
      S8LDD(xr1,src,3,0);S8LDD(xr2,&src[step],3,0);

      Q8MUL(xr5,xr1,xr12,xr6);
      Q8MUL(xr7,xr2,xr10,xr8);

      Q16ACCM_AA(xr5,xr11,xr11,xr6);
      Q16ACCM_AA(xr5,xr7,xr8,xr6);
      Q16SAR(xr5,xr5,xr6,xr6,6);
      Q16SAT(xr9,xr5,xr6);
      S32LDDR(xr8,dst,0);
      Q8AVGR(xr9,xr8,xr9);
      S32STDR(xr9,dst,0);
#else
      op_avg(dst[0], (A*src[0] + E*src[step+0]));
      op_avg(dst[1], (A*src[1] + E*src[step+1]));
      op_avg(dst[2], (A*src[2] + E*src[step+2]));
      op_avg(dst[3], (A*src[3] + E*src[step+3]));
#endif
#ifdef MC_USE_TCSM
      dst+= 16;
#else
      dst+= stride;
#endif
      src+= stride;
    }
  }
}

//a = (((a)+(((b) + 32)>>6)+1)>>1)
static void avg_rv30_chroma_mc8_c(uint8_t *dst/*align 8*/, uint8_t *src/*align 1*/, int stride, int h, int x, int y){
  //const int A=(8-x)*(8-y);
  //const int B=(  x)*(8-y);
  const int C=(8-x)*(  y);
  const int D=(  x)*(  y);
  int i;
  //assert(x<8 && y<8 && x>=0 && y>=0);
  uint8_t qbuf[5];
  qbuf[0] = (8-x)*(8-y);
  qbuf[1] = (  x)*(8-y);
  qbuf[2] = C;
  qbuf[3] = D;
  qbuf[4] = qbuf[1] + qbuf[2];

  S32I2M(xr11,0x00200020);
  S8LDD(xr12,qbuf,0,7);
  S8LDD(xr13,qbuf,1,7);
  S8LDD(xr14,qbuf,2,7);
  S8LDD(xr15,qbuf,3,7);
  S8LDD(xr10,qbuf,4,7);

  if(D){
    for(i=0; i<h; i++){
#if 1
      S8LDD(xr1,src,0,3);S8LDD(xr3,&src[stride],0,3);
      S8LDD(xr1,src,1,2);S8LDD(xr3,&src[stride],1,2);
      S8LDD(xr1,src,2,1);S8LDD(xr3,&src[stride],2,1);
      S8LDD(xr1,src,3,0);S8LDD(xr3,&src[stride],3,0);
      D32SLL(xr2,xr1,xr3,xr4,8);
      S8LDD(xr2,src,4,0);S8LDD(xr4,&src[stride],4,0);

      Q8MUL(xr5,xr1,xr12,xr6);Q8MUL(xr7,xr2,xr13,xr8);
      Q8MUL(xr9,xr3,xr14,xr10);Q8MUL(xr1,xr4,xr15,xr2);

      Q16ACCM_AA(xr5,xr7,xr8,xr6);
      S8LDD(xr3,&src[stride],4,3);//
      Q16ACCM_AA(xr9,xr1,xr2,xr10);
      S8LDD(xr1,src,4,3);//
      Q16ACCM_AA(xr5,xr11,xr11,xr6);
      S8LDD(xr3,&src[stride],5,2);//
      Q16ACCM_AA(xr5,xr9,xr10,xr6);
      S8LDD(xr1,src,5,2);//
      Q16SAR(xr5,xr5,xr6,xr6,6);
      S8LDD(xr3,&src[stride],6,1);//
      Q16SAT(xr9,xr5,xr6);
      S32LDDR(xr8,dst,0);
      S8LDD(xr1,src,6,1);//
      Q8AVGR(xr9,xr8,xr9);
      S8LDD(xr3,&src[stride],7,0);//
      S32STDR(xr9,dst,0);
      S8LDD(xr1,src,7,0);//

      D32SLL(xr2,xr1,xr3,xr4,8);
      S8LDD(xr2,src,8,0);S8LDD(xr4,&src[stride],8,0);

      Q8MUL(xr5,xr1,xr12,xr6);Q8MUL(xr7,xr2,xr13,xr8);
      Q8MUL(xr9,xr3,xr14,xr10);Q8MUL(xr1,xr4,xr15,xr2);

      Q16ACCM_AA(xr5,xr7,xr8,xr6);
      Q16ACCM_AA(xr9,xr1,xr2,xr10);
      Q16ACCM_AA(xr5,xr11,xr11,xr6);
      Q16ACCM_AA(xr5,xr9,xr10,xr6);
      Q16SAR(xr5,xr5,xr6,xr6,6);
      Q16SAT(xr9,xr5,xr6);
      S32LDDR(xr8,dst,4);
      Q8AVGR(xr9,xr8,xr9);
      S32STDR(xr9,dst,4);
#else
      op_avg(dst[0], (A*src[0] + B*src[1] + C*src[stride+0] + D*src[stride+1]));
      op_avg(dst[1], (A*src[1] + B*src[2] + C*src[stride+1] + D*src[stride+2]));
      op_avg(dst[2], (A*src[2] + B*src[3] + C*src[stride+2] + D*src[stride+3]));
      op_avg(dst[3], (A*src[3] + B*src[4] + C*src[stride+3] + D*src[stride+4]));
      op_avg(dst[4], (A*src[4] + B*src[5] + C*src[stride+4] + D*src[stride+5]));
      op_avg(dst[5], (A*src[5] + B*src[6] + C*src[stride+5] + D*src[stride+6]));
      op_avg(dst[6], (A*src[6] + B*src[7] + C*src[stride+6] + D*src[stride+7]));
      op_avg(dst[7], (A*src[7] + B*src[8] + C*src[stride+7] + D*src[stride+8]));
#endif
#ifdef MC_USE_TCSM
      dst+= 16;
#else
      dst+= stride;
#endif
      src+= stride;
    }
  }else{
    //const int E= B+C;
    const int step= C ? stride : 1;
    for(i=0; i<h; i++){
#if 1
      S8LDD(xr1,src,0,3);S8LDD(xr2,&src[step],0,3);
      S8LDD(xr1,src,1,2);S8LDD(xr2,&src[step],1,2);
      S8LDD(xr1,src,2,1);S8LDD(xr2,&src[step],2,1);
      S8LDD(xr1,src,3,0);S8LDD(xr2,&src[step],3,0);

      Q8MUL(xr5,xr1,xr12,xr6);
      Q8MUL(xr7,xr2,xr10,xr8);

      S8LDD(xr1,src,4,3);//
      Q16ACCM_AA(xr5,xr11,xr11,xr6);
      S8LDD(xr2,&src[step],4,3);//
      Q16ACCM_AA(xr5,xr7,xr8,xr6);
      S8LDD(xr1,src,5,2);//
      Q16SAR(xr5,xr5,xr6,xr6,6);
      S8LDD(xr2,&src[step],5,2);//
      Q16SAT(xr9,xr5,xr6);
      S8LDD(xr1,src,6,1);//
      S32LDDR(xr8,dst,0);
      S8LDD(xr2,&src[step],6,1);//
      Q8AVGR(xr9,xr8,xr9);
      S8LDD(xr1,src,7,0);//
      S32STDR(xr9,dst,0);
      S8LDD(xr2,&src[step],7,0);//

      Q8MUL(xr5,xr1,xr12,xr6);
      Q8MUL(xr7,xr2,xr10,xr8);

      Q16ACCM_AA(xr5,xr11,xr11,xr6);
      Q16ACCM_AA(xr5,xr7,xr8,xr6);
      Q16SAR(xr5,xr5,xr6,xr6,6);
      Q16SAT(xr9,xr5,xr6);
      S32LDDR(xr8,dst,4);
      Q8AVGR(xr9,xr8,xr9);
      S32STDR(xr9,dst,4);
#else
      op_avg(dst[0], (A*src[0] + E*src[step+0]));
      op_avg(dst[1], (A*src[1] + E*src[step+1]));
      op_avg(dst[2], (A*src[2] + E*src[step+2]));
      op_avg(dst[3], (A*src[3] + E*src[step+3]));
      op_avg(dst[4], (A*src[4] + E*src[step+4]));
      op_avg(dst[5], (A*src[5] + E*src[step+5]));
      op_avg(dst[6], (A*src[6] + E*src[step+6]));
      op_avg(dst[7], (A*src[7] + E*src[step+7]));
      //printf("xr9:0x%08x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\n", S32M2I(xr9), dst[0], dst[1], dst[2], dst[3]);
#endif
#ifdef MC_USE_TCSM
      dst+= 16;
#else
      dst+= stride;
#endif
      src+= stride;
    }
  }
}
#undef op_avg
#undef op_put

/**
 * @defgroup transform RV30/40 inverse transform functions
 * @{
 */

static av_always_inline void rv34_row_transform(int temp[16], DCTELEM *block)
{
    int i;

    for(i=0; i<4; i++){
        const int z0= 13*(block[i+8*0] +    block[i+8*2]);
        const int z1= 13*(block[i+8*0] -    block[i+8*2]);
        const int z2=  7* block[i+8*1] - 17*block[i+8*3];
        const int z3= 17* block[i+8*1] +  7*block[i+8*3];

        temp[4*i+0]= z0+z3;
        temp[4*i+1]= z1+z2;
        temp[4*i+2]= z1-z2;
        temp[4*i+3]= z0-z3;
    }
}

/**
 * Real Video 3.0/4.0 inverse transform
 * Code is almost the same as in SVQ3, only scaling is different.
 */
static void rv34_inv_transform(int *src,DCTELEM *block){
    int temp[16];
    int i;

    for(i=0; i<4; i++){
        const int z0= 13*(src[i+4*0] +    src[i+4*2]);
        const int z1= 13*(src[i+4*0] -    src[i+4*2]);
        const int z2=  7* src[i+4*1] - 17*src[i+4*3];
        const int z3= 17* src[i+4*1] +  7*src[i+4*3];

        temp[4*i+0]= z0+z3;
        temp[4*i+1]= z1+z2;
        temp[4*i+2]= z1-z2;
        temp[4*i+3]= z0-z3;
    }

    for(i=0; i<4; i++){
        const int z0= 13*(temp[4*0+i] +    temp[4*2+i]) + 0x200;
        const int z1= 13*(temp[4*0+i] -    temp[4*2+i]) + 0x200;
        const int z2=  7* temp[4*1+i] - 17*temp[4*3+i];
        const int z3= 17* temp[4*1+i] +  7*temp[4*3+i];

        block[i*8+0]= (z0 + z3)>>10;
        block[i*8+1]= (z1 + z2)>>10;
        block[i*8+2]= (z1 - z2)>>10;
        block[i*8+3]= (z0 - z3)>>10;
    }

}

/**
 * RealVideo 3.0/4.0 inverse transform for DC block
 *
 * Code is almost the same as rv34_inv_transform()
 * but final coefficients are multiplied by 1.5 and have no rounding.
 */
static av_always_inline void rv34_inv_transform_noround(DCTELEM *block){
  int temp[16];
  int i;

  S32I2M(xr11,13);
  S32I2M(xr12,((7<<16)|17));
  uint32_t src=block-1;
  uint32_t dst=temp-4;
  for(i=0; i<4; i++){
#if 1
    S16LDI(xr1,src,0x2,2);
    S16LDD(xr2,src,0x10,2);
    S16LDD(xr3,src,0x8,3);
    D32ADD_AS(xr1,xr1,xr2,xr2);
    S32SFL(xr0,xr1,xr2,xr2,3);
    D16MUL_LW(xr1,xr11,xr2,xr2);
    S16LDD(xr5,src,0x18,3);

    D16MUL_WW(xr3,xr3,xr12,xr4);
    D16MUL_WW(xr5,xr5,xr12,xr6);
    D32ASUM_SA(xr3,xr6,xr5,xr4);

    D32ADD_AS(xr1,xr1,xr4,xr4);
    D32ADD_AS(xr2,xr2,xr3,xr3);
    S32SDI(xr1,dst,0x10);
    S32STD(xr2,dst,0x4);
    S32STD(xr3,dst,0x8);
    S32STD(xr4,dst,0xc);
#else
    const int z0= 13*(block[i+4*0] +    block[i+4*2]);
    const int z1= 13*(block[i+4*0] -    block[i+4*2]);
    const int z2=  7* block[i+4*1] - 17*block[i+4*3];
    const int z3= 17* block[i+4*1] +  7*block[i+4*3];

    temp[4*i+0]= z0+z3;
    temp[4*i+1]= z1+z2;
    temp[4*i+2]= z1-z2;
    temp[4*i+3]= z0-z3;
#endif
  }
  for(i=0; i<4; i++){
    const int z0= 13*(temp[4*0+i] +    temp[4*2+i]);
    const int z1= 13*(temp[4*0+i] -    temp[4*2+i]);
    const int z2=  7* temp[4*1+i] - 17*temp[4*3+i];
    const int z3= 17* temp[4*1+i] +  7*temp[4*3+i];
    block[i*4+0]= ((z0 + z3)*3)>>11;
    block[i*4+1]= ((z1 + z2)*3)>>11;
    block[i*4+2]= ((z1 - z2)*3)>>11;
    block[i*4+3]= ((z0 - z3)*3)>>11;
  }

}

/** @} */ // transform


/**
 * @defgroup block RV30/40 4x4 block decoding functions
 * @{
 */

/**
 * Decode coded block pattern.
 */
static int rv34_decode_cbp(GetBitContext *gb, RV34VLC *vlc, int table)
{
    int pattern, code, cbp=0;
    int ones;
    static const int cbp_masks[3] = {0x100000, 0x010000, 0x110000};
    static const int shifts[4] = { 0, 2, 8, 10 };
    const int *curshift = shifts;
    int i, t, mask;

    code = get_vlc2(gb, vlc->cbppattern[table].table, 9, 2);
    pattern = code & 0xF;
    code >>= 4;

    ones = rv34_count_ones[pattern];

    for(mask = 8; mask; mask >>= 1, curshift++){
        if(pattern & mask)
            cbp |= get_vlc2(gb, vlc->cbp[table][ones].table, vlc->cbp[table][ones].bits, 1) << curshift[0];
    }

    for(i = 0; i < 4; i++){
        t = modulo_three_table[code][i];
        if(t == 1)
            cbp |= cbp_masks[get_bits1(gb)] << i;
        if(t == 2)
            cbp |= cbp_masks[2] << i;
    }
    return cbp;
}

/**
 * Get one coefficient value from the bistream and store it.
 */
static inline void decode_coeff(DCTELEM *dst, int coef, int esc, GetBitContext *gb, VLC* vlc)
{
    if(coef){
        if(coef == esc){
            coef = get_vlc2(gb, vlc->table, 9, 2);
            if(coef > 23){
                coef -= 23;
                coef = 22 + ((1 << coef) | get_bits(gb, coef));
            }
            coef += esc;
        }
        if(get_bits1(gb))
            coef = -coef;
        *dst = coef;
    }
}

static inline void decode_qcoeff(int *dst, int coef, int esc, GetBitContext *gb, VLC* vlc, int Q)
{
    if(coef){
        if(coef == esc){
            coef = get_vlc2(gb, vlc->table, 9, 2);
            if(coef > 23){
                coef -= 23;
                coef = 22 + ((1 << coef) | get_bits(gb, coef));
            }
            coef += esc;
        }
        if(get_bits1(gb))
            coef = -coef;
        *dst = (coef*Q+8)>>4;
    }
}

/**
 * Decode 2x2 subblock of coefficients.
 */
static inline void decode_subblock(int *dst, int code, const int is_block2, GetBitContext *gb, VLC *vlc, int Q)
{
    int coeffs[4];

    coeffs[0] = modulo_three_table[code][0];
    coeffs[1] = modulo_three_table[code][1];
    coeffs[2] = modulo_three_table[code][2];
    coeffs[3] = modulo_three_table[code][3];
    decode_qcoeff(dst  , coeffs[0], 3, gb, vlc,Q);
    if(is_block2){
      decode_qcoeff(dst+4, coeffs[1], 2, gb, vlc,Q);
      decode_qcoeff(dst+1, coeffs[2], 2, gb, vlc,Q);
    }else{
      decode_qcoeff(dst+1, coeffs[1], 2, gb, vlc,Q);
      decode_qcoeff(dst+4, coeffs[2], 2, gb, vlc,Q);
    }
    decode_qcoeff(dst+5, coeffs[3], 2, gb, vlc,Q);
}

static av_always_inline void decode_subblock0(int *dst, int code, const int is_block2, GetBitContext *gb, VLC *vlc,int Qdc,int Q)
{
    int coeffs[4];

    coeffs[0] = modulo_three_table[code][0];
    coeffs[1] = modulo_three_table[code][1];
    coeffs[2] = modulo_three_table[code][2];
    coeffs[3] = modulo_three_table[code][3];
    decode_qcoeff(dst  , coeffs[0], 3, gb, vlc,Qdc);
    if(is_block2){
      decode_qcoeff(dst+4, coeffs[1], 2, gb, vlc,Q);
      decode_qcoeff(dst+1, coeffs[2], 2, gb, vlc,Q);
    }else{
      decode_qcoeff(dst+1, coeffs[1], 2, gb, vlc,Q);
      decode_qcoeff(dst+4, coeffs[2], 2, gb, vlc,Q);
    }
    decode_qcoeff(dst+5, coeffs[3], 2, gb, vlc,Q);
}

static inline void decode_subblock16(DCTELEM *dst, int code, const int is_block2, GetBitContext *gb, VLC *vlc)
{
    int coeffs[4];

    coeffs[0] = modulo_three_table[code][0];
    coeffs[1] = modulo_three_table[code][1];
    coeffs[2] = modulo_three_table[code][2];
    coeffs[3] = modulo_three_table[code][3];
    decode_coeff(dst  , coeffs[0], 3, gb, vlc);
    if(is_block2){
        decode_coeff(dst+4, coeffs[1], 2, gb, vlc);
        decode_coeff(dst+1, coeffs[2], 2, gb, vlc);
    }else{
        decode_coeff(dst+1, coeffs[1], 2, gb, vlc);
        decode_coeff(dst+4, coeffs[2], 2, gb, vlc);
    }
    decode_coeff(dst+5, coeffs[3], 2, gb, vlc);
}

/**
 * Decode coefficients for 4x4 block.
 *
 * This is done by filling 2x2 subblocks with decoded coefficients
 * in this order (the same for subblocks and subblock coefficients):
 *  o--o
 *    /
 *   /
 *  o--o
 */
static inline void rv34_decode_block(int *dst, GetBitContext *gb, RV34VLC *rvlc, int fc, int sc,int Qdc,int Q)
{
    int code, pattern;

    code = get_vlc2(gb, rvlc->first_pattern[fc].table, 9, 2);

    pattern = code & 0x7;

    code >>= 3;
    decode_subblock0(dst, code, 0, gb, &rvlc->coefficient,Qdc,Q);

    if(pattern & 4){
        code = get_vlc2(gb, rvlc->second_pattern[sc].table, 9, 2);
        decode_subblock(dst + 2, code, 0, gb, &rvlc->coefficient,Q);
    }
    if(pattern & 2){ // Looks like coefficients 1 and 2 are swapped for this block
        code = get_vlc2(gb, rvlc->second_pattern[sc].table, 9, 2);
        decode_subblock(dst + 8, code, 1, gb, &rvlc->coefficient,Q);
    }
    if(pattern & 1){
        code = get_vlc2(gb, rvlc->third_pattern[sc].table, 9, 2);
        decode_subblock(dst + 10, code, 0, gb, &rvlc->coefficient,Q);
    }

}

static inline void rv34_decode_block16(DCTELEM *dst, GetBitContext *gb, RV34VLC *rvlc, int fc, int sc)
{
    int code, pattern;

    code = get_vlc2(gb, rvlc->first_pattern[fc].table, 9, 2);

    pattern = code & 0x7;

    code >>= 3;
    decode_subblock16(dst, code, 0, gb, &rvlc->coefficient);

    if(pattern & 4){
        code = get_vlc2(gb, rvlc->second_pattern[sc].table, 9, 2);
        decode_subblock16(dst + 2, code, 0, gb, &rvlc->coefficient);
    }
    if(pattern & 2){ // Looks like coefficients 1 and 2 are swapped for this block
        code = get_vlc2(gb, rvlc->second_pattern[sc].table, 9, 2);
        decode_subblock16(dst + 8, code, 1, gb, &rvlc->coefficient);
    }
    if(pattern & 1){
        code = get_vlc2(gb, rvlc->third_pattern[sc].table, 9, 2);
        decode_subblock16(dst + 10, code, 0, gb, &rvlc->coefficient);
    }

}

/**
 * Dequantize ordinary 4x4 block.
 * @todo optimize
 */
static inline void rv34_dequant4x4(DCTELEM *block, int Qdc, int Q)
{
    int i, j;

    block[0] = (block[0] * Qdc + 8) >> 4;
    for(i = 0; i < 4; i++)
        for(j = !i; j < 4; j++)
            block[j + i*8] = (block[j + i*8] * Q + 8) >> 4;
}

/**
 * Dequantize 4x4 block of DC values for 16x16 macroblock.
 * @todo optimize
 */
static av_always_inline void rv34_dequant4x4_16x16(DCTELEM *block, int Qdc, int Q)
{
  int i;
#if 0
  const uint8_t rv40_dezigzag[16] = {
    0,  1,  4,  8,
    5,  2,  3,  6,
    9, 12, 13, 10,
    7, 11, 14, 15
  };
  for(i = 0; i < 3; i++)
    block[rv40_dezigzag[i]] = (block[rv40_dezigzag[i]] * Qdc + 8) >> 4;
  for(; i < 16; i++)
    block[rv40_dezigzag[i]] = (block[rv40_dezigzag[i]] * Q + 8) >> 4;
#else
  S16LDD(xr11,&Q,0,0);
  S16LDD(xr11,&Qdc,0,1);
  S32I2M(xr12,0x8);
  S32LDD(xr1,block,0x0);
  D16MUL_HW(xr3,xr11,xr1,xr4);
  S32LDD(xr2,block,0x4);
  D32ASUM_AA(xr3,xr12,xr12,xr4);
  D16MUL_LW(xr5,xr11,xr2,xr6);
  D32SARL(xr3,xr3,xr4,4);
  S32LDD(xr1,block,0x8);
  D32ASUM_AA(xr5,xr12,xr12,xr6);
  S32STD(xr3,block,0x0);

  D16MUL_XW(xr3,xr11,xr1,xr4);
  S32LDD(xr2,block,0xc);
  D32SARL(xr5,xr5,xr6,4);
  S32STD(xr5,block,0x4);
  D32ASUM_AA(xr3,xr12,xr12,xr4);

  D16MUL_LW(xr5,xr11,xr2,xr6);
  S32LDD(xr1,block,0x10);
  D32SARL(xr3,xr3,xr4,4);
  D32ASUM_AA(xr5,xr12,xr12,xr6);
  S32STD(xr3,block,0x8);

  D16MUL_LW(xr3,xr11,xr1,xr4);
  S32LDD(xr2,block,0x14);
  D32SARL(xr5,xr5,xr6,4);
  D32ASUM_AA(xr3,xr12,xr12,xr4);
  S32STD(xr5,block,0x0c);

  D16MUL_LW(xr5,xr11,xr2,xr6);
  S32LDD(xr1,block,0x18);
  D32SARL(xr3,xr3,xr4,4);
  D32ASUM_AA(xr5,xr12,xr12,xr6);
  S32STD(xr3,block,0x10);

  D16MUL_LW(xr3,xr11,xr1,xr4);
  S32LDD(xr2,block,0x1c);
  D32SARL(xr5,xr5,xr6,4);
  D32ASUM_AA(xr3,xr12,xr12,xr4);
  S32STD(xr5,block,0x14);

  D16MUL_LW(xr5,xr11,xr2,xr6);
  D32SARL(xr3,xr3,xr4,4);
  D32ASUM_AA(xr5,xr12,xr12,xr6);
  S32STD(xr3,block,0x18);

  D32SARL(xr5,xr5,xr6,4);
  S32STD(xr5,block,0x1c);
#endif
}
/** @} */ //block functions


/**
 * @defgroup rv3040_bitstream RV30/40 bitstream parsing
 * @{
 */

/**
 * Decode starting slice position.
 * @todo Maybe replace with ff_h263_decode_mba() ?
 */
int ff_rv34_get_start_offset(GetBitContext *gb, int mb_size)
{
    int i;
    for(i = 0; i < 5; i++)
        if(rv34_mb_max_sizes[i] >= mb_size - 1)
            break;
    return rv34_mb_bits_sizes[i];
}

/**
 * Select VLC set for decoding from current quantizer, modifier and frame type.
 */
static inline RV34VLC* choose_vlc_set(int quant, int mod, int type)
{
    if(mod == 2 && quant < 19) quant += 10;
    else if(mod && quant < 26) quant += 5;
    return type ? &inter_vlcs[rv34_quant_to_vlc_set[1][av_clip(quant, 0, 30)]]
                : &intra_vlcs[rv34_quant_to_vlc_set[0][av_clip(quant, 0, 30)]];
}

/**
 * Decode quantizer difference and return modified quantizer.
 */
static inline int rv34_decode_dquant(GetBitContext *gb, int quant)
{
    if(get_bits1(gb))
        return rv34_dquant_tab[get_bits1(gb)][quant];
    else
        return get_bits(gb, 5);
}

/** @} */ //bitstream functions

/**
 * @defgroup mv motion vector related code (prediction, reconstruction, motion compensation)
 * @{
 */

/** macroblock partition width in 8x8 blocks */
static const uint8_t part_sizes_w[RV34_MB_TYPES] = { 2, 2, 2, 1, 2, 2, 2, 2, 2, 1, 2, 2 };

/** macroblock partition height in 8x8 blocks */
static const uint8_t part_sizes_h[RV34_MB_TYPES] = { 2, 2, 2, 1, 2, 2, 2, 2, 1, 2, 2, 2 };

/** availability index for subblocks */
static const uint8_t avail_indexes[4] = { 6, 7, 10, 11 };

/**
 * motion vector prediction
 *
 * Motion prediction performed for the block by using median prediction of
 * motion vectors from the left, top and right top blocks but in corner cases
 * some other vectors may be used instead.
 */
static void rv34_pred_mv(RV34DecContext *r, int block_type, int subblock_no, int dmv_no)
{
#ifdef JZC_PMON_P0
  //PMON_ON(rv8vlc);
#endif
    MpegEncContext *s = &r->s;
    int mv_pos = s->mb_x * 2 + s->mb_y * 2 * s->b8_stride;
    int A[2] = {0}, B[2], C[2];
    int i, j;
    int mx, my;
    int avail_index = avail_indexes[subblock_no];
    int c_off = part_sizes_w[block_type];

    mv_pos += (subblock_no & 1) + (subblock_no >> 1)*s->b8_stride;
    if(subblock_no == 3)
        c_off = -1;

    if(r->avail_cache[avail_index - 1]){
        A[0] = s->current_picture_ptr->motion_val[0][mv_pos-1][0];
        A[1] = s->current_picture_ptr->motion_val[0][mv_pos-1][1];
    }
    if(r->avail_cache[avail_index - 4]){
        B[0] = s->current_picture_ptr->motion_val[0][mv_pos-s->b8_stride][0];
        B[1] = s->current_picture_ptr->motion_val[0][mv_pos-s->b8_stride][1];
    }else{
        B[0] = A[0];
        B[1] = A[1];
    }
    if(!r->avail_cache[avail_index - 4 + c_off]){
        if(r->avail_cache[avail_index - 4]){
            C[0] = s->current_picture_ptr->motion_val[0][mv_pos-s->b8_stride-1][0];
            C[1] = s->current_picture_ptr->motion_val[0][mv_pos-s->b8_stride-1][1];
        }else{
            C[0] = A[0];
            C[1] = A[1];
        }
    }else{
        C[0] = s->current_picture_ptr->motion_val[0][mv_pos-s->b8_stride+c_off][0];
        C[1] = s->current_picture_ptr->motion_val[0][mv_pos-s->b8_stride+c_off][1];
    }
    mx = mid_pred(A[0], B[0], C[0]);
    my = mid_pred(A[1], B[1], C[1]);
    mx += r->dmv[dmv_no][0];
    my += r->dmv[dmv_no][1];
    for(j = 0; j < part_sizes_h[block_type]; j++){
        for(i = 0; i < part_sizes_w[block_type]; i++){
            s->current_picture_ptr->motion_val[0][mv_pos + i + j*s->b8_stride][0] = mx;
            s->current_picture_ptr->motion_val[0][mv_pos + i + j*s->b8_stride][1] = my;
        }
    }
#ifdef JZC_PMON_P0
    //PMON_OFF(rv8vlc);
#endif
}

#define GET_PTS_DIFF(a, b) ((a - b + 8192) & 0x1FFF)

/**
 * Calculate motion vector component that should be added for direct blocks.
 */
static int calc_add_mv(RV34DecContext *r, int dir, int val)
{
    int refdist = GET_PTS_DIFF(r->next_pts, r->last_pts);
    int dist = dir ? -GET_PTS_DIFF(r->next_pts, r->cur_pts) : GET_PTS_DIFF(r->cur_pts, r->last_pts);
    int mul;

    if(!refdist) return 0;
    mul = (dist << 14) / refdist;
    return (val * mul + 0x2000) >> 14;
}

/**
 * Predict motion vector for B-frame macroblock.
 */
static inline void rv34_pred_b_vector(int A[2], int B[2], int C[2],
                                      int A_avail, int B_avail, int C_avail,
                                      int *mx, int *my)
{
    if(A_avail + B_avail + C_avail != 3){
        *mx = A[0] + B[0] + C[0];
        *my = A[1] + B[1] + C[1];
        if(A_avail + B_avail + C_avail == 2){
            *mx /= 2;
            *my /= 2;
        }
    }else{
        *mx = mid_pred(A[0], B[0], C[0]);
        *my = mid_pred(A[1], B[1], C[1]);
    }
}

/**
 * motion vector prediction for B-frames
 */
static void rv34_pred_mv_b(RV34DecContext *r, int block_type, int dir)
{
#ifdef JZC_PMON_P0
  //PMON_ON(rv8vlc);
#endif
    MpegEncContext *s = &r->s;
    int mb_pos = s->mb_x + s->mb_y * s->mb_stride;
    int mv_pos = s->mb_x * 2 + s->mb_y * 2 * s->b8_stride;
    int A[2], B[2], C[2];
    int has_A = 0, has_B = 0, has_C = 0;
    int mx, my;
    int i, j;
    Picture *cur_pic = s->current_picture_ptr;
    const int mask = dir ? MB_TYPE_L1 : MB_TYPE_L0;
    int type = cur_pic->mb_type[mb_pos];

    memset(A, 0, sizeof(A));
    memset(B, 0, sizeof(B));
    memset(C, 0, sizeof(C));
    if((r->avail_cache[6-1] & type) & mask){
        A[0] = cur_pic->motion_val[dir][mv_pos - 1][0];
        A[1] = cur_pic->motion_val[dir][mv_pos - 1][1];
        has_A = 1;
    }
    if((r->avail_cache[6-4] & type) & mask){
        B[0] = cur_pic->motion_val[dir][mv_pos - s->b8_stride][0];
        B[1] = cur_pic->motion_val[dir][mv_pos - s->b8_stride][1];
        has_B = 1;
    }
    if(r->avail_cache[6-4] && (r->avail_cache[6-2] & type) & mask){
        C[0] = cur_pic->motion_val[dir][mv_pos - s->b8_stride + 2][0];
        C[1] = cur_pic->motion_val[dir][mv_pos - s->b8_stride + 2][1];
        has_C = 1;
    }else if((s->mb_x+1) == s->mb_width && (r->avail_cache[6-5] & type) & mask){
        C[0] = cur_pic->motion_val[dir][mv_pos - s->b8_stride - 1][0];
        C[1] = cur_pic->motion_val[dir][mv_pos - s->b8_stride - 1][1];
        has_C = 1;
    }

    rv34_pred_b_vector(A, B, C, has_A, has_B, has_C, &mx, &my);

    mx += r->dmv[dir][0];
    my += r->dmv[dir][1];

    for(j = 0; j < 2; j++){
        for(i = 0; i < 2; i++){
            cur_pic->motion_val[dir][mv_pos + i + j*s->b8_stride][0] = mx;
            cur_pic->motion_val[dir][mv_pos + i + j*s->b8_stride][1] = my;
        }
    }
    if(block_type == RV34_MB_B_BACKWARD || block_type == RV34_MB_B_FORWARD){
        ZERO8x2(cur_pic->motion_val[!dir][mv_pos], s->b8_stride);
    }
#ifdef JZC_PMON_P0
    //PMON_OFF(rv8vlc);
#endif
}

/**
 * motion vector prediction - RV3 version
 */
static void rv34_pred_mv_rv3(RV34DecContext *r, int block_type, int dir)
{
#ifdef JZC_PMON_P0
  //PMON_ON(rv8vlc);
#endif
    MpegEncContext *s = &r->s;
    int mv_pos = s->mb_x * 2 + s->mb_y * 2 * s->b8_stride;
    int A[2] = {0}, B[2], C[2];
    int i, j, k;
    int mx, my;
    int avail_index = avail_indexes[0];

    if(r->avail_cache[avail_index - 1]){
        A[0] = s->current_picture_ptr->motion_val[0][mv_pos-1][0];
        A[1] = s->current_picture_ptr->motion_val[0][mv_pos-1][1];
    }
    if(r->avail_cache[avail_index - 4]){
        B[0] = s->current_picture_ptr->motion_val[0][mv_pos-s->b8_stride][0];
        B[1] = s->current_picture_ptr->motion_val[0][mv_pos-s->b8_stride][1];
    }else{
        B[0] = A[0];
        B[1] = A[1];
    }
    if(!r->avail_cache[avail_index - 4 + 2]){
        if(r->avail_cache[avail_index - 4] && (r->avail_cache[avail_index - 1])){
            C[0] = s->current_picture_ptr->motion_val[0][mv_pos-s->b8_stride-1][0];
            C[1] = s->current_picture_ptr->motion_val[0][mv_pos-s->b8_stride-1][1];
        }else{
            C[0] = A[0];
            C[1] = A[1];
        }
    }else{
        C[0] = s->current_picture_ptr->motion_val[0][mv_pos-s->b8_stride+2][0];
        C[1] = s->current_picture_ptr->motion_val[0][mv_pos-s->b8_stride+2][1];
    }
    mx = mid_pred(A[0], B[0], C[0]);
    my = mid_pred(A[1], B[1], C[1]);
    mx += r->dmv[0][0];
    my += r->dmv[0][1];
    for(j = 0; j < 2; j++){
        for(i = 0; i < 2; i++){
            for(k = 0; k < 2; k++){
                s->current_picture_ptr->motion_val[k][mv_pos + i + j*s->b8_stride][0] = mx;
                s->current_picture_ptr->motion_val[k][mv_pos + i + j*s->b8_stride][1] = my;
            }
        }
    }
#ifdef JZC_PMON_P0
    //PMON_OFF(rv8vlc);
#endif
}

static const int chroma_coeffs[3] = { 0, 3, 5 };

/**
 * generic motion compensation function
 *
 * @param r decoder context
 * @param block_type type of the current block
 * @param xoff horizontal offset from the start of the current block
 * @param yoff vertical offset from the start of the current block
 * @param mv_off offset to the motion vector information
 * @param width width of the current partition in 8x8 blocks
 * @param height height of the current partition in 8x8 blocks
 * @param dir motion compensation direction (i.e. from the last or the next reference frame)
 * @param thirdpel motion vectors are specified in 1/3 of pixel
 * @param qpel_mc a set of functions used to perform luma motion compensation
 * @param chroma_mc a set of functions used to perform chroma motion compensation
 */
static inline void rv34_mc(RV34DecContext *r, const int block_type,
			   const int xoff, const int yoff, int mv_off,
			   const int width, const int height, int dir,
			   qpel_mc_func (*qpel_mc)[16],
			   h264_chroma_mc_func (*chroma_mc))
{
  MpegEncContext *s = &r->s;
  uint8_t *Y, *U, *V, *srcY, *srcU, *srcV;
  int dxy, mx, my, umx, umy, lx, ly, uvmx, uvmy, src_x, src_y, uvsrc_x, uvsrc_y;
  int mv_pos = s->mb_x * 2 + s->mb_y * 2 * s->b8_stride + mv_off;
  int is16x16 = 1;

  int chroma_mx, chroma_my;
  //if (frame_num == 2)
  //printf("rv34_mc\n");
  mx = (s->current_picture_ptr->motion_val[dir][mv_pos][0] + (3 << 24)) / 3 - (1 << 24);
  my = (s->current_picture_ptr->motion_val[dir][mv_pos][1] + (3 << 24)) / 3 - (1 << 24);
  lx = (s->current_picture_ptr->motion_val[dir][mv_pos][0] + (3 << 24)) % 3;
  ly = (s->current_picture_ptr->motion_val[dir][mv_pos][1] + (3 << 24)) % 3;
  chroma_mx = (s->current_picture_ptr->motion_val[dir][mv_pos][0] + 1) >> 1;
  chroma_my = (s->current_picture_ptr->motion_val[dir][mv_pos][1] + 1) >> 1;
  umx = (chroma_mx + (3 << 24)) / 3 - (1 << 24);
  umy = (chroma_my + (3 << 24)) / 3 - (1 << 24);
  uvmx = chroma_coeffs[(chroma_mx + (3 << 24)) % 3];
  uvmy = chroma_coeffs[(chroma_my + (3 << 24)) % 3];

  dxy = ly*4 + lx;
  srcY = dir ? s->next_picture_ptr->data[0] : s->last_picture_ptr->data[0];
  srcU = dir ? s->next_picture_ptr->data[1] : s->last_picture_ptr->data[1];
  srcV = dir ? s->next_picture_ptr->data[2] : s->last_picture_ptr->data[2];
  src_x = s->mb_x * 16 + xoff + mx;
  src_y = s->mb_y * 16 + yoff + my;
  uvsrc_x = s->mb_x * 8 + (xoff >> 1) + umx;
  uvsrc_y = s->mb_y * 8 + (yoff >> 1) + umy;
  srcY += src_y * s->linesize + src_x;
  srcU += uvsrc_y * s->uvlinesize + uvsrc_x;
  srcV += uvsrc_y * s->uvlinesize + uvsrc_x;
  if(   (unsigned)(src_x - !!lx*2) > s->h_edge_pos - !!lx*2 - (width <<3) - 4
	|| (unsigned)(src_y - !!ly*2) > s->v_edge_pos - !!ly*2 - (height<<3) - 4){
    uint8_t *uvbuf= s->edge_emu_buffer + 22 * s->linesize;

    srcY -= 2 + 2*s->linesize;
    ff_emulated_edge_mc(s->edge_emu_buffer, srcY, s->linesize, (width<<3)+6, (height<<3)+6,
			src_x - 2, src_y - 2, s->h_edge_pos, s->v_edge_pos);
    srcY = s->edge_emu_buffer + 2 + 2*s->linesize;
    ff_emulated_edge_mc(uvbuf     , srcU, s->uvlinesize, (width<<2)+1, (height<<2)+1,
			uvsrc_x, uvsrc_y, s->h_edge_pos >> 1, s->v_edge_pos >> 1);
    ff_emulated_edge_mc(uvbuf + 16, srcV, s->uvlinesize, (width<<2)+1, (height<<2)+1,
			uvsrc_x, uvsrc_y, s->h_edge_pos >> 1, s->v_edge_pos >> 1);
    srcU = uvbuf;
    srcV = uvbuf + 16;
  }
#ifdef MC_USE_TCSM
  Y = MC_YBUF;
  U = MC_UBUF;
  V = U+8;
#else
  Y = s->dest[0] + xoff      + yoff     *s->linesize;
  U = s->dest[1] + (xoff>>1) + (yoff>>1)*s->uvlinesize;
  V = s->dest[2] + (xoff>>1) + (yoff>>1)*s->uvlinesize;
#endif

#ifdef JZC_PMON_P0
  //PMON_ON(rv8vlc);
#endif
  if(block_type == RV34_MB_P_16x8){
    //printf("RV34_MB_P_16x8 %d\n, dxy");
    qpel_mc[1][dxy](Y, srcY, s->linesize);
    Y    += 8;
    srcY += 8;
  }else if(block_type == RV34_MB_P_8x16){
    //printf("RV34_MB_P_8x16 %d\n, dxy");
    qpel_mc[1][dxy](Y, srcY, s->linesize);
#ifdef MC_USE_TCSM
    Y    += 128;//8*16
#else
    Y    += 8 * s->linesize;
#endif
    srcY += 8 * s->linesize;
  }
  is16x16 = (block_type != RV34_MB_P_8x8) && (block_type != RV34_MB_P_16x8) && (block_type != RV34_MB_P_8x16);

  //printf("%d %d\n", !is16x16, dxy);
  qpel_mc[!is16x16][dxy](Y, srcY, s->linesize);
  chroma_mc[2-width]   (U, srcU, s->uvlinesize, height*4, uvmx, uvmy);
  chroma_mc[2-width]   (V, srcV, s->uvlinesize, height*4, uvmx, uvmy);
#ifdef JZC_PMON_P0
  //PMON_OFF(rv8vlc);
#endif

#if 0
  //printf("mbx:%d mby:%d\n", s->mb_x, s->mb_y);
#ifdef MC_USE_TCSM
  Y = MC_YBUF;
  U = MC_UBUF;
  V = U+8;
  if (frame_num == 2 && s->mb_x == 20 && s->mb_y == 0)
    ptr_square(Y,1,16,16, 16);
#else
  //Y = s->dest[0] + xoff      + yoff     *s->linesize;
  Y = s->dest[0] + s->mb_x*16+ s->mb_y     *s->linesize;
  U = s->dest[1] + (xoff>>1) + (yoff>>1)*s->uvlinesize;
  V = s->dest[2] + (xoff>>1) + (yoff>>1)*s->uvlinesize;

  if (frame_num == 2 && s->mb_x == 20 && s->mb_y == 0)
    ptr_square(Y,1,16,16, s->linesize);
#endif
#endif
}

static void rv34_mc_1mv(RV34DecContext *r, const int block_type,
                        const int xoff, const int yoff, int mv_off,
                        const int width, const int height, int dir)
{
  rv34_mc(r, block_type, xoff, yoff, mv_off, width, height, dir,
	  r->s.dsp.put_rv30_tpel_pixels_tab,
	  r->s.dsp.put_h264_chroma_pixels_tab);
}

static void rv34_mc_2mv(RV34DecContext *r, const int block_type)
{
  rv34_mc(r, block_type, 0, 0, 0, 2, 2, 0,
	  r->s.dsp.put_rv30_tpel_pixels_tab,
	  r->s.dsp.put_h264_chroma_pixels_tab);
  rv34_mc(r, block_type, 0, 0, 0, 2, 2, 1,
	  r->s.dsp.avg_rv30_tpel_pixels_tab,
	  r->s.dsp.avg_h264_chroma_pixels_tab);
}

static void rv34_mc_2mv_skip(RV34DecContext *r)
{
    int i, j;
    for(j = 0; j < 2; j++)
        for(i = 0; i < 2; i++){
             rv34_mc(r, RV34_MB_P_8x8, i*8, j*8, i+j*r->s.b8_stride, 1, 1, 0,
		     r->s.dsp.put_rv30_tpel_pixels_tab,
		     r->s.dsp.put_h264_chroma_pixels_tab);
             rv34_mc(r, RV34_MB_P_8x8, i*8, j*8, i+j*r->s.b8_stride, 1, 1, 1,
		     r->s.dsp.avg_rv30_tpel_pixels_tab,
		     r->s.dsp.avg_h264_chroma_pixels_tab);
        }
}

/** number of motion vectors in each macroblock type */
static const int num_mvs[RV34_MB_TYPES] = { 0, 0, 1, 4, 1, 1, 0, 0, 2, 2, 2, 1 };

/**
 * Decode motion vector differences
 * and perform motion vector reconstruction and motion compensation.
 */
static int rv34_decode_mv(RV34DecContext *r, int block_type)
{
    MpegEncContext *s = &r->s;
    GetBitContext *gb = &s->gb;
    int i, j, k, l;
    int mv_pos = s->mb_x * 2 + s->mb_y * 2 * s->b8_stride;
    int next_bt;

    memset(r->dmv, 0, sizeof(r->dmv));
    for(i = 0; i < num_mvs[block_type]; i++){
        r->dmv[i][0] = svq3_get_se_golomb(gb);
        r->dmv[i][1] = svq3_get_se_golomb(gb);
    }

    switch(block_type){
    case RV34_MB_TYPE_INTRA:
    case RV34_MB_TYPE_INTRA16x16:
        ZERO8x2(s->current_picture_ptr->motion_val[0][s->mb_x * 2 + s->mb_y * 2 * s->b8_stride], s->b8_stride);
        return 0;
    case RV34_MB_SKIP:

        if(s->pict_type == FF_P_TYPE){

	  ZERO8x2(s->current_picture_ptr->motion_val[0][s->mb_x * 2 + s->mb_y * 2 * s->b8_stride], s->b8_stride);
	  rv34_mc_1mv (r, block_type, 0, 0, 0, 2, 2, 0);
	  break;
        }
    case RV34_MB_B_DIRECT:
        //surprisingly, it uses motion scheme from next reference frame
        next_bt = s->next_picture_ptr->mb_type[s->mb_x + s->mb_y * s->mb_stride];
        if(IS_INTRA(next_bt) || IS_SKIP(next_bt)){
            ZERO8x2(s->current_picture_ptr->motion_val[0][s->mb_x * 2 + s->mb_y * 2 * s->b8_stride], s->b8_stride);
            ZERO8x2(s->current_picture_ptr->motion_val[1][s->mb_x * 2 + s->mb_y * 2 * s->b8_stride], s->b8_stride);
        }else
            for(j = 0; j < 2; j++)
                for(i = 0; i < 2; i++)
                    for(k = 0; k < 2; k++)
		      for(l = 0; l < 2; l++){
                            s->current_picture_ptr->motion_val[l][mv_pos + i + j*s->b8_stride][k] = calc_add_mv(r, l, s->next_picture_ptr->motion_val[0][mv_pos + i + j*s->b8_stride][k]);
		      }
#ifdef JZC_PMON_P0
	//PMON_ON(rv8vlc);
#endif
        if(!(IS_16X8(next_bt) || IS_8X16(next_bt) || IS_8X8(next_bt))) //we can use whole macroblock MC
            rv34_mc_2mv(r, block_type);
        else
            rv34_mc_2mv_skip(r);
#ifdef JZC_PMON_P0
	//PMON_OFF(rv8vlc);
#endif
        ZERO8x2(s->current_picture_ptr->motion_val[0][s->mb_x * 2 + s->mb_y * 2 * s->b8_stride], s->b8_stride);
        break;
    case RV34_MB_P_16x16:
    case RV34_MB_P_MIX16x16:
        rv34_pred_mv(r, block_type, 0, 0);
        rv34_mc_1mv (r, block_type, 0, 0, 0, 2, 2, 0);
        break;
    case RV34_MB_B_FORWARD:
    case RV34_MB_B_BACKWARD:
        r->dmv[1][0] = r->dmv[0][0];
        r->dmv[1][1] = r->dmv[0][1];
	rv34_pred_mv_rv3(r, block_type, block_type == RV34_MB_B_BACKWARD);
        rv34_mc_1mv     (r, block_type, 0, 0, 0, 2, 2, block_type == RV34_MB_B_BACKWARD);
        break;
    case RV34_MB_P_16x8:
    case RV34_MB_P_8x16:
        rv34_pred_mv(r, block_type, 0, 0);
        rv34_pred_mv(r, block_type, 1 + (block_type == RV34_MB_P_16x8), 1);
        if(block_type == RV34_MB_P_16x8){
            rv34_mc_1mv(r, block_type, 0, 0, 0,            2, 1, 0);
            rv34_mc_1mv(r, block_type, 0, 8, s->b8_stride, 2, 1, 0);
        }
        if(block_type == RV34_MB_P_8x16){
            rv34_mc_1mv(r, block_type, 0, 0, 0, 1, 2, 0);
            rv34_mc_1mv(r, block_type, 8, 0, 1, 1, 2, 0);
        }
        break;
    case RV34_MB_B_BIDIR:
        rv34_pred_mv_b  (r, block_type, 0);
        rv34_pred_mv_b  (r, block_type, 1);
        rv34_mc_2mv     (r, block_type);
        break;
    case RV34_MB_P_8x8:
        for(i=0;i< 4;i++){
            rv34_pred_mv(r, block_type, i, i);
            rv34_mc_1mv (r, block_type, (i&1)<<3, (i&2)<<2, (i&1)+(i>>1)*s->b8_stride, 1, 1, 0);
        }
        break;
    }

    return 0;
}
/** @} */ // mv group

/**
 * @defgroup recons Macroblock reconstruction functions
 * @{
 */
/** mapping of RV30/40 intra prediction types to standard H.264 types */
static const int ittrans[9] = {
 DC_PRED, VERT_PRED, HOR_PRED, DIAG_DOWN_RIGHT_PRED, DIAG_DOWN_LEFT_PRED,
 VERT_RIGHT_PRED, VERT_LEFT_PRED, HOR_UP_PRED, HOR_DOWN_PRED,
};

/** mapping of RV30/40 intra 16x16 prediction types to standard H.264 types */
static const int ittrans16[4] = {
 DC_PRED8x8, VERT_PRED8x8, HOR_PRED8x8, PLANE_PRED8x8,
};

/**
 * Perform 4x4 intra prediction.
 */
static void rv34_pred_4x4_block(RV34DecContext *r, uint8_t *dst, int stride, int itype, int up, int left, int down, int right)
{
    uint8_t *prev = dst - stride + 4;
    uint32_t topleft;

    if(!up && !left)
        itype = DC_128_PRED;
    else if(!up){
        if(itype == VERT_PRED) itype = HOR_PRED;
        if(itype == DC_PRED)   itype = LEFT_DC_PRED;
    }else if(!left){
        if(itype == HOR_PRED)  itype = VERT_PRED;
        if(itype == DC_PRED)   itype = TOP_DC_PRED;
        if(itype == DIAG_DOWN_LEFT_PRED) itype = DIAG_DOWN_LEFT_PRED_RV40_NODOWN;
    }
    if(!down){
        if(itype == DIAG_DOWN_LEFT_PRED) itype = DIAG_DOWN_LEFT_PRED_RV40_NODOWN;
        if(itype == HOR_UP_PRED) itype = HOR_UP_PRED_RV40_NODOWN;
        if(itype == VERT_LEFT_PRED) itype = VERT_LEFT_PRED_RV40_NODOWN;
    }
    if(!right && up){
        topleft = dst[-stride + 3] * 0x01010101;
        prev = (uint8_t*)&topleft;
    }
    r->h.pred4x4[itype](dst, prev, stride);
}

/** add_pixels_clamped for 4x4 block */
static void rv34_add_4x4_block(uint8_t *dst, int stride, DCTELEM block[64], int off)
{
  int x, y;
  for(y = 0; y < 4; y++)
    for(x = 0; x < 4; x++)
      dst[x + y*stride] = av_clip_uint8(dst[x + y*stride] + block[off + x+y*8]);
}

static inline int adjust_pred16(int itype, int up, int left)
{
  if(!up && !left)
    itype = DC_128_PRED8x8;
  else if(!up){
    if(itype == PLANE_PRED8x8)itype = HOR_PRED8x8;
    if(itype == VERT_PRED8x8) itype = HOR_PRED8x8;
    if(itype == DC_PRED8x8)   itype = LEFT_DC_PRED8x8;
  }else if(!left){
    if(itype == PLANE_PRED8x8)itype = VERT_PRED8x8;
    if(itype == HOR_PRED8x8)  itype = VERT_PRED8x8;
    if(itype == DC_PRED8x8)   itype = TOP_DC_PRED8x8;
  }
  return itype;
}

static void add_pixels_clamped_mxu(DCTELEM *block, uint8_t *pixels, int line_size)
{
  int i;
  DCTELEM *b = block - 2;
  uint8_t *dst = pixels - line_size;
  
  for(i=0;i<8;i++) {
    S32LDIV(xr1,dst,line_size,0);
    S32LDI(xr2,b,4);
    S32LDI(xr3,b,4);
    
    Q8ADDE_AA(xr4,xr1,xr0,xr5);
    Q16ACCM_AA(xr4,xr3,xr2,xr5);
    Q16SAT(xr6,xr4,xr5);
    S32STD(xr6,dst,0);

    S32LDD(xr1,dst,4);
    S32LDI(xr2,b,4);
    S32LDI(xr3,b,4);
    
    Q8ADDE_AA(xr4,xr1,xr0,xr5);
    Q16ACCM_AA(xr4,xr3,xr2,xr5);
    Q16SAT(xr6,xr4,xr5);
    S32STD(xr6,dst,4);
  }
}

static void add_pixels_clamped_mxu2(DCTELEM *block, uint8_t *pixels, int line_size, uint8_t *mc)
{
  int i;
  DCTELEM *b = block - 2;
  uint8_t *dst = pixels - line_size;
  mc = mc - 16;

#if 0  
  if (frame_num == 2){
#ifdef MC_USE_TCSM
    printf("block 0x%04x, 0x%04x\n", block[0], block[1]);
    printf("mc 0x%02x, 0x%02x\n", mc[0], mc[1]);
#else
    printf("block 0x%04x, 0x%04x\n", block[0], block[1]);
    printf("pixels 0x%02x, 0x%02x\n", pixels[0], pixels[1]);
#endif
  }
#endif

  for(i=0;i<8;i++) {
#ifdef MC_USE_TCSM
    S32LDI(xr1,mc,16);
#else
    S32LDIV(xr1,dst,line_size,0);
#endif
    S32LDI(xr2,b,4);
    S32LDI(xr3,b,4);
    
    Q8ADDE_AA(xr4,xr1,xr0,xr5);
    Q16ACCM_AA(xr4,xr3,xr2,xr5);
    Q16SAT(xr6,xr4,xr5);
    S32STD(xr6,dst,0);

#ifdef MC_USE_TCSM
    S32LDD(xr1,mc,4);
#else
    S32LDD(xr1,dst,4);
#endif
    S32LDI(xr2,b,4);
    S32LDI(xr3,b,4);
    
    Q8ADDE_AA(xr4,xr1,xr0,xr5);
    Q16ACCM_AA(xr4,xr3,xr2,xr5);
    Q16SAT(xr6,xr4,xr5);
    S32STD(xr6,dst,4);
  }
  //if (frame_num == 2){
  //printf("end pixels 0x%02x, 0x%02x\n", pixels[0], pixels[1]);
  //}
}

static void rv34_output_macroblock(RV34DecContext *r, int8_t *intra_types, int cbp, int is16)
{
    MpegEncContext *s = &r->s;
    DSPContext *dsp = &s->dsp;
    int i, j;
    uint8_t *Y, *U, *V;
    int itype;
    int avail[6*8] = {0};
    int idx;

    // Set neighbour information.
    if(r->avail_cache[1])
        avail[0] = 1;
    if(r->avail_cache[2])
        avail[1] = avail[2] = 1;
    if(r->avail_cache[3])
        avail[3] = avail[4] = 1;
    if(r->avail_cache[4])
        avail[5] = 1;
    if(r->avail_cache[5])
        avail[8] = avail[16] = 1;
    if(r->avail_cache[9])
        avail[24] = avail[32] = 1;

    Y = s->dest[0];
    U = s->dest[1];
    V = s->dest[2];
    if(!is16){
        for(j = 0; j < 4; j++){
            idx = 9 + j*8;
            for(i = 0; i < 4; i++, cbp >>= 1, Y += 4, idx++){
                rv34_pred_4x4_block(r, Y, s->linesize, ittrans[intra_types[i]], avail[idx-8], avail[idx-1], avail[idx+7], avail[idx-7]);
                avail[idx] = 1;
                if(cbp & 1)
                    rv34_add_4x4_block(Y, s->linesize, s->block[(i>>1)+(j&2)], (i&1)*4+(j&1)*32);
            }
            Y += s->linesize * 4 - 4*4;
            intra_types += r->intra_types_stride;
        }
        intra_types -= r->intra_types_stride * 4;
        fill_rectangle(r->avail_cache + 6, 2, 2, 4, 0, 4);
        for(j = 0; j < 2; j++){
            idx = 6 + j*4;
            for(i = 0; i < 2; i++, cbp >>= 1, idx++){
                rv34_pred_4x4_block(r, U + i*4 + j*4*s->uvlinesize, s->uvlinesize, ittrans[intra_types[i*2+j*2*r->intra_types_stride]], r->avail_cache[idx-4], r->avail_cache[idx-1], !i && !j, r->avail_cache[idx-3]);
                rv34_pred_4x4_block(r, V + i*4 + j*4*s->uvlinesize, s->uvlinesize, ittrans[intra_types[i*2+j*2*r->intra_types_stride]], r->avail_cache[idx-4], r->avail_cache[idx-1], !i && !j, r->avail_cache[idx-3]);
                r->avail_cache[idx] = 1;
                if(cbp & 0x01)
                    rv34_add_4x4_block(U + i*4 + j*4*s->uvlinesize, s->uvlinesize, s->block[4], i*4+j*32);
                if(cbp & 0x10)
                    rv34_add_4x4_block(V + i*4 + j*4*s->uvlinesize, s->uvlinesize, s->block[5], i*4+j*32);
            }
        }
    }else{
        itype = ittrans16[intra_types[0]];
        itype = adjust_pred16(itype, r->avail_cache[6-4], r->avail_cache[6-1]);
        r->h.pred16x16[itype](Y, s->linesize);
        add_pixels_clamped_mxu(s->block[0], Y,     s->linesize);
        add_pixels_clamped_mxu(s->block[1], Y + 8, s->linesize);
	Y += s->linesize * 8;
        add_pixels_clamped_mxu(s->block[2], Y,     s->linesize);
        add_pixels_clamped_mxu(s->block[3], Y + 8, s->linesize);

        itype = ittrans16[intra_types[0]];
        if(itype == PLANE_PRED8x8) itype = DC_PRED8x8;
        itype = adjust_pred16(itype, r->avail_cache[6-4], r->avail_cache[6-1]);
        r->h.pred8x8[itype](U, s->uvlinesize);
        add_pixels_clamped_mxu(s->block[4], U, s->uvlinesize);
        r->h.pred8x8[itype](V, s->uvlinesize);
        add_pixels_clamped_mxu(s->block[5], V, s->uvlinesize);
    }
}

/** @} */ // recons group

/**
 * @addtogroup bitstream
 * Decode macroblock header and return CBP in case of success, -1 otherwise.
 */
static int rv34_decode_mb_header(RV34DecContext *r, int8_t *intra_types)
{
  MpegEncContext *s = &r->s;
  GetBitContext *gb = &s->gb;
  int mb_pos = s->mb_x + s->mb_y * s->mb_stride;
  int i, t;

  if(!r->si.type){
    r->is16 = get_bits1(gb);
    s->current_picture_ptr->mb_type[mb_pos] = r->is16 ? MB_TYPE_INTRA16x16 : MB_TYPE_INTRA;
    r->block_type = r->is16 ? RV34_MB_TYPE_INTRA16x16 : RV34_MB_TYPE_INTRA;
  }else{
    r->block_type = r->decode_mb_info(r);
    if(r->block_type == -1)
      return -1;
    s->current_picture_ptr->mb_type[mb_pos] = rv34_mb_type_to_lavc[r->block_type];
    r->mb_type[mb_pos] = r->block_type;
    if(r->block_type == RV34_MB_SKIP){
      if(s->pict_type == FF_P_TYPE)
	r->mb_type[mb_pos] = RV34_MB_P_16x16;
      if(s->pict_type == FF_B_TYPE)
	r->mb_type[mb_pos] = RV34_MB_B_DIRECT;
    }
    r->is16 = !!IS_INTRA16x16(s->current_picture_ptr->mb_type[mb_pos]);
#ifdef JZC_PMON_P0
    //PMON_ON(rv8vlc);
#endif
    rv34_decode_mv(r, r->block_type);
#ifdef JZC_PMON_P0
    //PMON_OFF(rv8vlc);
#endif

    if(r->block_type == RV34_MB_SKIP){
      fill_rectangle(intra_types, 4, 4, r->intra_types_stride, 0, sizeof(intra_types[0]));
      return 0;
    }
    r->chroma_vlc = 1;
    r->luma_vlc   = 0;
  }

  if(IS_INTRA(s->current_picture_ptr->mb_type[mb_pos])){
    if(r->is16){
      t = get_bits(gb, 2);
      fill_rectangle(intra_types, 4, 4, r->intra_types_stride, t, sizeof(intra_types[0]));
      r->luma_vlc   = 2;
    }else{
      if(r->decode_intra_types(r, gb, intra_types) < 0)
	return -1;
      r->luma_vlc   = 1;
    }
    r->chroma_vlc = 0;
    r->cur_vlcs = choose_vlc_set(r->si.quant, r->si.vlc_set, 0);
  }else{
    for(i = 0; i < 16; i++)
      intra_types[(i & 3) + (i>>2) * r->intra_types_stride] = 0;
    r->cur_vlcs = choose_vlc_set(r->si.quant, r->si.vlc_set, 1);
    if(r->mb_type[mb_pos] == RV34_MB_P_MIX16x16){
      r->is16 = 1;
      r->chroma_vlc = 1;
      r->luma_vlc   = 2;
      r->cur_vlcs = choose_vlc_set(r->si.quant, r->si.vlc_set, 0);
    }
  }

  return rv34_decode_cbp(gb, r->cur_vlcs, r->is16);
}

/**
 * @addtogroup recons
 * @{
 */
/**
 * mask for retrieving all bits in coded block pattern
 * corresponding to one 8x8 block
 */
#define LUMA_CBP_BLOCK_MASK 0x33

#define U_CBP_MASK 0x0F0000
#define V_CBP_MASK 0xF00000


static void rv34_apply_differences(RV34DecContext *r, int cbp)
{
  static const int shifts[4] = { 0, 2, 8, 10 };
  MpegEncContext *s = &r->s;
  int i;
  //uint8_t *mc = MC_YBUF;

  for(i = 0; i < 4; i++){
    if((cbp & (LUMA_CBP_BLOCK_MASK << shifts[i])) || r->block_type == RV34_MB_P_MIX16x16){
      //add_pixels_clamped_mxu2(s->block[i], s->dest[0] + (i&1)*8 + (i&2)*4*s->linesize, s->linesize, mc+(i&1)*8+(i&2)*64);
      add_pixels_clamped_mxu(s->block[i], s->dest[0] + (i&1)*8 + (i&2)*4*s->linesize, s->linesize);
    }
  }

  //mc = MC_UBUF;

  if(cbp & U_CBP_MASK)
    //add_pixels_clamped_mxu2(s->block[4], s->dest[1], s->uvlinesize, mc);
    add_pixels_clamped_mxu(s->block[4], s->dest[1], s->uvlinesize);
  if(cbp & V_CBP_MASK)
    //add_pixels_clamped_mxu2(s->block[5], s->dest[2], s->uvlinesize, mc+8);
    add_pixels_clamped_mxu(s->block[5], s->dest[2], s->uvlinesize);
}

static int is_mv_diff_gt_3(int16_t (*motion_val)[2], int step)
{
    int d;
    d = motion_val[0][0] - motion_val[-step][0];
    if(d < -3 || d > 3)
        return 1;
    d = motion_val[0][1] - motion_val[-step][1];
    if(d < -3 || d > 3)
        return 1;
    return 0;
}

static int rv34_set_deblock_coef(RV34DecContext *r)
{
    MpegEncContext *s = &r->s;
    int hmvmask = 0, vmvmask = 0, i, j;
    int midx = s->mb_x * 2 + s->mb_y * 2 * s->b8_stride;
    int16_t (*motion_val)[2] = &s->current_picture_ptr->motion_val[0][midx];
    for(j = 0; j < 16; j += 8){
        for(i = 0; i < 2; i++){
            if(is_mv_diff_gt_3(motion_val + i, 1))
                vmvmask |= 0x11 << (j + i*2);
            if((j || s->mb_y) && is_mv_diff_gt_3(motion_val + i, s->b8_stride))
                hmvmask |= 0x03 << (j + i*2);
        }
        motion_val += s->b8_stride;
    }
    if(s->first_slice_line)
        hmvmask &= ~0x000F;
    if(!s->mb_x)
      vmvmask &= ~0x1111;
    vmvmask |= (vmvmask & 0x4444) >> 1;
    hmvmask |= (hmvmask & 0x0F00) >> 4;
    if(s->mb_x)
      r->deblock_coefs[s->mb_x - 1 + s->mb_y*s->mb_stride] |= (vmvmask & 0x1111) << 3;
    if(!s->first_slice_line)
      r->deblock_coefs[s->mb_x + (s->mb_y - 1)*s->mb_stride] |= (hmvmask & 0xF) << 12;
    return hmvmask | vmvmask;
}

static int rv34_decode_macroblock(RV34DecContext *r, int8_t *intra_types)
{
  MpegEncContext *s = &r->s;
  GetBitContext *gb = &s->gb;
  int cbp, cbp2;
  int i, blknum, blkoff;
  DCTELEM *block16=BLK16_BUF;
  int luma_dc_quant;
  int dist;
  int mb_pos = s->mb_x + s->mb_y * s->mb_stride;
  int *ptr;
  // Calculate which neighbours are available. Maybe it's worth optimizing too.
  //pmon:8qian
#if 0
  memset(r->avail_cache, 0, sizeof(r->avail_cache));
#else
  dist = r->avail_cache;
  //i_pref(30, dist, 0);
  S32STD(xr0, dist, 0);
  S32STD(xr0, dist, 4);
  S32STD(xr0, dist, 8);
  S32STD(xr0, dist, 12);
  S32STD(xr0, dist, 16);
  S32STD(xr0, dist, 20);
  S32STD(xr0, dist, 24);
  S32STD(xr0, dist, 28);
  S32STD(xr0, dist, 32);
  S32STD(xr0, dist, 36);
  S32STD(xr0, dist, 40);
  S32STD(xr0, dist, 44);
#endif
  fill_rectangle(r->avail_cache + 6, 2, 2, 4, 1, 4);
  dist = (s->mb_x - s->resync_mb_x) + (s->mb_y - s->resync_mb_y) * s->mb_width;
  if(s->mb_x && dist)
    r->avail_cache[5] =
      r->avail_cache[9] = s->current_picture_ptr->mb_type[mb_pos - 1];
  if(dist >= s->mb_width)
    r->avail_cache[2] =
      r->avail_cache[3] = s->current_picture_ptr->mb_type[mb_pos - s->mb_stride];
  if(((s->mb_x+1) < s->mb_width) && dist >= s->mb_width - 1)
    r->avail_cache[4] = s->current_picture_ptr->mb_type[mb_pos - s->mb_stride + 1];
  if(s->mb_x && dist > s->mb_width)
    r->avail_cache[1] = s->current_picture_ptr->mb_type[mb_pos - s->mb_stride - 1];

  s->qscale = r->si.quant;

#ifdef JZC_PMON_P0
  //PMON_ON(rv8vlc);
#endif
  cbp = cbp2 = rv34_decode_mb_header(r, intra_types);//pmon:28yi
#ifdef JZC_PMON_P0
  //PMON_OFF(rv8vlc);
#endif

  r->cbp_luma  [mb_pos] = cbp;
  r->cbp_chroma[mb_pos] = cbp >> 16;
  if(s->pict_type == FF_I_TYPE){
    r->deblock_coefs[mb_pos] = 0xFFFF;
  }else{
    r->deblock_coefs[mb_pos] = rv34_set_deblock_coef(r) | r->cbp_luma[mb_pos];
  }
  s->current_picture_ptr->qscale_table[mb_pos] = s->qscale;

  if(cbp == -1)
    return -1;

  //pmon 1yi
  luma_dc_quant = r->block_type == RV34_MB_P_MIX16x16 ? r->luma_dc_quant_p[s->qscale] : r->luma_dc_quant_i[s->qscale];
  if(r->is16){
    memset(block16, 0, sizeof(*block16)<<4);
    rv34_decode_block16(block16, gb, r->cur_vlcs, 3, 0);
    rv34_dequant4x4_16x16(block16, rv34_qscale_tab[luma_dc_quant],rv34_qscale_tab[s->qscale]);
    rv34_inv_transform_noround(block16);
  }

  ptr=RECON_YBUF;
  for(i = 0; i < 16; i++, cbp >>= 1){//pmon 4yi2qian
    if(!r->is16 && !(cbp & 1)) continue;
    blknum = ((i & 2) >> 1) + ((i & 8) >> 2);
    blkoff = ((i & 1) << 2) + ((i & 4) << 3);
#if 0
    memset(tcsm_blk,0,sizeof(*tcsm_blk)<<4);
#else
    dist = tcsm_blk;
    //i_pref(30, dist, 0);
    S32STD(xr0, dist, 0);
    S32STD(xr0, dist, 4);
    S32STD(xr0, dist, 8);
    S32STD(xr0, dist, 12);
    S32STD(xr0, dist, 16);
    S32STD(xr0, dist, 20);
    S32STD(xr0, dist, 24);
    S32STD(xr0, dist, 28);
    //dist += 32;
    //i_pref(30, dist, 0);
    S32STD(xr0, dist, 32);
    S32STD(xr0, dist, 36);
    S32STD(xr0, dist, 40);
    S32STD(xr0, dist, 44);
    S32STD(xr0, dist, 48);
    S32STD(xr0, dist, 52);
    S32STD(xr0, dist, 56);
    S32STD(xr0, dist, 60);
#endif

    if(cbp & 1){
      rv34_decode_block(tcsm_blk, gb, r->cur_vlcs, r->luma_vlc, 0, rv34_qscale_tab[s->qscale],rv34_qscale_tab[s->qscale]);
      //rv34_dequant4x4(s->block[blknum] + blkoff, rv34_qscale_tab[s->qscale],rv34_qscale_tab[s->qscale]);
    }
    if(r->is16) //FIXME: optimize
      tcsm_blk[0] = block16[i];
    rv34_inv_transform(tcsm_blk,s->block[blknum] + blkoff);
#if 0
    idct_chain_tab[3] = TCSM0_PADDR((uint32_t)idct_out);
    idct_chain_tab[4] = TCSM0_PADDR((uint32_t)tcsm_blk);
    idct_polling_end_flag();
    clean_idct_end_flag();
    run_idct_ddma();
    S32LDD(xr1,idct_out2,0x0);
    S32LDD(xr2,idct_out2,0x4);
    S32LDD(xr3,idct_out2,0x8);
    S32LDD(xr4,idct_out2,0xc);
    S32LDD(xr5,idct_out2,0x10);
    S32LDD(xr6,idct_out2,0x14);
    S32LDD(xr7,idct_out2,0x18);
    S32LDD(xr8,idct_out2,0x1c);
    S32STD(xr1,ptr,0x0);
    S32STD(xr2,ptr,0x4);
    S32STD(xr3,ptr,0x10);
    S32STD(xr4,ptr,0x14);
    S32STD(xr5,ptr,0x20);
    S32STD(xr6,ptr,0x24);
    S32STD(xr7,ptr,0x30);
    S32STD(xr8,ptr,0x34);
    ptr=s->block[blknum] + blkoff;
    XCHG(idct_out,idct_out2);
    XCHG(tcsm_blk,tcsm_blk2);
#endif
  }

  if(r->block_type == RV34_MB_P_MIX16x16)
    r->cur_vlcs = choose_vlc_set(r->si.quant, r->si.vlc_set, 1);
#ifdef JZC_PMON_P0
  //PMON_ON(rv8vlc);
#endif
  for(; i < 24; i++, cbp >>= 1){//pmon 1yi4qian
    if(!(cbp & 1)) continue;
    blknum = ((i & 4) >> 2) + 4;
    blkoff = ((i & 1) << 2) + ((i & 2) << 4);
    memset(tcsm_blk,0,sizeof(*tcsm_blk)<<4);
#ifdef JZC_PMON_P0
    //PMON_ON(rv8vlc);
#endif
    rv34_decode_block(tcsm_blk, gb, r->cur_vlcs, r->chroma_vlc, 1, rv34_qscale_tab[rv34_chroma_quant[1][s->qscale]],rv34_qscale_tab[rv34_chroma_quant[0][s->qscale]]);
#ifdef JZC_PMON_P0
    //PMON_OFF(rv8vlc);
#endif
    //rv34_dequant4x4(s->block[blknum] + blkoff, rv34_qscale_tab[rv34_chroma_quant[1][s->qscale]],rv34_qscale_tab[rv34_chroma_quant[0][s->qscale]]);
    rv34_inv_transform(tcsm_blk,s->block[blknum] + blkoff);
#if 0
    idct_chain_tab[3] = TCSM0_PADDR((uint32_t)idct_out);
    idct_chain_tab[4] = TCSM0_PADDR((uint32_t)tcsm_blk);
    idct_polling_end_flag();
    clean_idct_end_flag();
    run_idct_ddma();
    S32LDD(xr1,idct_out2,0x0);
    S32LDD(xr2,idct_out2,0x4);
    S32LDD(xr3,idct_out2,0x8);
    S32LDD(xr4,idct_out2,0xc);
    S32LDD(xr5,idct_out2,0x10);
    S32LDD(xr6,idct_out2,0x14);
    S32LDD(xr7,idct_out2,0x18);
    S32LDD(xr8,idct_out2,0x1c);
    S32STD(xr1,ptr,0x0);
    S32STD(xr2,ptr,0x4);
    S32STD(xr3,ptr,0x10);
    S32STD(xr4,ptr,0x14);
    S32STD(xr5,ptr,0x20);
    S32STD(xr6,ptr,0x24);
    S32STD(xr7,ptr,0x30);
    S32STD(xr8,ptr,0x34);
    ptr=s->block[blknum] + blkoff;
    XCHG(idct_out,idct_out2);
    XCHG(tcsm_blk,tcsm_blk2);
#endif
  }
#ifdef JZC_PMON_P0
  //PMON_OFF(rv8vlc);
#endif

#if 0
  idct_polling_end_flag();//4yi5qian
  S32LDD(xr1,idct_out2,0x0);
  S32LDD(xr2,idct_out2,0x4);
  S32LDD(xr3,idct_out2,0x8);
  S32LDD(xr4,idct_out2,0xc);
  S32LDD(xr5,idct_out2,0x10);
  S32LDD(xr6,idct_out2,0x14);
  S32LDD(xr7,idct_out2,0x18);
  S32LDD(xr8,idct_out2,0x1c);
  S32STD(xr1,ptr,0x0);
  S32STD(xr2,ptr,0x4);
  S32STD(xr3,ptr,0x10);
  S32STD(xr4,ptr,0x14);
  S32STD(xr5,ptr,0x20);
  S32STD(xr6,ptr,0x24);
  S32STD(xr7,ptr,0x30);
  S32STD(xr8,ptr,0x34);
#endif
#if 0
  if (frame_num == 2)
    printf("mbx:%d mby:%d\n", s->mb_x, s->mb_y);
#ifdef MC_USE_TCSM
  uint8_t *Y = MC_YBUF;
  uint8_t *U = MC_UBUF;
  uint8_t *V = U+8;
  if (frame_num == 2)
    ptr_square(Y,1,16,16, 16);
#else
  uint8_t *Y = s->dest[0];
  if (frame_num == 2)
    ptr_square(Y,1,16,16, s->linesize);
#endif
#endif

  if(IS_INTRA(s->current_picture_ptr->mb_type[mb_pos])){
    //if (frame_num == 2)
    //printf("IS_INTRA\n");
    rv34_output_macroblock(r, intra_types, cbp2, r->is16);
  }else{
    //if (frame_num == 2)
    //printf("NOT_INTRA\n");
    rv34_apply_differences(r, cbp2);
  }

#if 0
  if (frame_num == 2){
    printf("mbx:%d mby:%d\n", s->mb_x, s->mb_y);
    Y = s->dest[0];
    ptr_square(Y,1,16,16, s->linesize);
  }
#endif
  return 0;
}

static int check_slice_end(RV34DecContext *r, MpegEncContext *s)
{
    int bits;
    if(s->mb_y >= s->mb_height)
        return 1;
    if(!s->mb_num_left)
        return 1;
    if(r->s.mb_skip_run > 1)
        return 0;
    bits = r->bits - get_bits_count(&s->gb);
    if(bits < 0 || (bits < 8 && !show_bits(&s->gb, bits)))
        return 1;
    return 0;
}

static inline int slice_compare(SliceInfo *si1, SliceInfo *si2)
{
    return si1->type   != si2->type  ||
           si1->start  >= si2->start ||
           si1->width  != si2->width ||
           si1->height != si2->height||
           si1->pts    != si2->pts;
}

static int rv34_decode_slice(RV34DecContext *r, int end, const uint8_t* buf, int buf_size)
{
  MpegEncContext *s = &r->s;
  GetBitContext *gb = &s->gb;
  int mb_pos;
  int res;

  init_get_bits(&r->s.gb, buf, buf_size*8);
  res = r->parse_slice_header(r, gb, &r->si);
  if(res < 0){
    av_log(s->avctx, AV_LOG_ERROR, "Incorrect or unknown slice header\n");
    return -1;
  }

  if ((s->mb_x == 0 && s->mb_y == 0) || s->current_picture_ptr==NULL) {
    if(s->width != r->si.width || s->height != r->si.height){
      av_log(s->avctx, AV_LOG_DEBUG, "Changing dimensions to %dx%d\n", r->si.width,r->si.height);
      MPV_common_end(s);
      s->width  = r->si.width;
      s->height = r->si.height;
      avcodec_set_dimensions(s->avctx, s->width, s->height);
      if(MPV_common_init(s) < 0)
	return -1;
      r->intra_types_stride = s->mb_width*4 + 4;
      r->intra_types_hist = av_realloc(r->intra_types_hist, r->intra_types_stride * 4 * 2 * sizeof(*r->intra_types_hist));
      r->intra_types = r->intra_types_hist + r->intra_types_stride * 4;
      r->mb_type = av_realloc(r->mb_type, r->s.mb_stride * r->s.mb_height * sizeof(*r->mb_type));
      r->cbp_luma   = av_realloc(r->cbp_luma,   r->s.mb_stride * r->s.mb_height * sizeof(*r->cbp_luma));
      r->cbp_chroma = av_realloc(r->cbp_chroma, r->s.mb_stride * r->s.mb_height * sizeof(*r->cbp_chroma));
      r->deblock_coefs = av_realloc(r->deblock_coefs, r->s.mb_stride * r->s.mb_height * sizeof(*r->deblock_coefs));
    }
    s->pict_type = r->si.type ? r->si.type : FF_I_TYPE;
    if(MPV_frame_start(s, s->avctx) < 0)
      return -1;
    ff_er_frame_start(s);
    r->cur_pts = r->si.pts;
    if(s->pict_type != FF_B_TYPE){
      r->last_pts = r->next_pts;
      r->next_pts = r->cur_pts;
    }
    s->mb_x = s->mb_y = 0;
  }

  r->si.end = end;
  s->qscale = r->si.quant;
  r->bits = buf_size*8;
  s->mb_num_left = r->si.end - r->si.start;
  r->s.mb_skip_run = 0;

  mb_pos = s->mb_x + s->mb_y * s->mb_width;
  if(r->si.start != mb_pos){
    av_log(s->avctx, AV_LOG_ERROR, "Slice indicates MB offset %d, got %d\n", r->si.start, mb_pos);
    s->mb_x = r->si.start % s->mb_width;
    s->mb_y = r->si.start / s->mb_width;
  }
  memset(r->intra_types_hist, -1, r->intra_types_stride * 4 * 2 * sizeof(*r->intra_types_hist));
  s->first_slice_line = 1;
  s->resync_mb_x= s->mb_x;
  s->resync_mb_y= s->mb_y;

  s->block=BLK_BUF;
  ff_init_block_index(s);
  while(!check_slice_end(r, s)) {
    ff_update_block_index(s);
#if 0
    s->dsp.clear_blocks(s->block[0]);
#else
    {
      int addr,i;
      for (i = 0; i < 6; i++){
	//for (j = 0; j < 4; j++){
	addr = &s->block[i][0];
	//i_pref(30, addr, 0);
	S32STD(xr0, addr, 0);
	S32STD(xr0, addr, 4);
	S32STD(xr0, addr, 8);
	S32STD(xr0, addr, 12);
	S32STD(xr0, addr, 16);
	S32STD(xr0, addr, 20);
	S32STD(xr0, addr, 24);
	S32STD(xr0, addr, 28);
	addr+=32;
	//i_pref(30, addr, 0);
	S32STD(xr0, addr, 0);
	S32STD(xr0, addr, 4);
	S32STD(xr0, addr, 8);
	S32STD(xr0, addr, 12);
	S32STD(xr0, addr, 16);
	S32STD(xr0, addr, 20);
	S32STD(xr0, addr, 24);
	S32STD(xr0, addr, 28);
	addr+=32;
	//i_pref(30, addr, 0);
	S32STD(xr0, addr, 0);
	S32STD(xr0, addr, 4);
	S32STD(xr0, addr, 8);
	S32STD(xr0, addr, 12);
	S32STD(xr0, addr, 16);
	S32STD(xr0, addr, 20);
	S32STD(xr0, addr, 24);
	S32STD(xr0, addr, 28);
	addr+=32;
	//i_pref(30, addr, 0);
	S32STD(xr0, addr, 0);
	S32STD(xr0, addr, 4);
	S32STD(xr0, addr, 8);
	S32STD(xr0, addr, 12);
	S32STD(xr0, addr, 16);
	S32STD(xr0, addr, 20);
	S32STD(xr0, addr, 24);
	S32STD(xr0, addr, 28);
	//}
      }
    }
#endif

#ifdef JZC_PMON_P0
    //PMON_ON(rv8vlc);
#endif
    //printf("decode_macroblock start\n");
    if(rv34_decode_macroblock(r, r->intra_types + s->mb_x * 4 + 4) < 0){
      ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x-1, s->mb_y, AC_ERROR|DC_ERROR|MV_ERROR);
      return -1;
    }
    //printf("decode_macroblock end\n");
#ifdef JZC_PMON_P0
    //PMON_OFF(rv8vlc);
#endif
    if (++s->mb_x == s->mb_width) {
      s->mb_x = 0;
      s->mb_y++;
      ff_init_block_index(s);

      memmove(r->intra_types_hist+r->intra_types_stride*3, r->intra_types+r->intra_types_stride*3, r->intra_types_stride * sizeof(*r->intra_types_hist));
      //memset(r->intra_types, -1, r->intra_types_stride * 4 * sizeof(*r->intra_types_hist));

      //if(r->loop_filter && s->mb_y >= 2)
      //r->loop_filter(r, s->mb_y - 2);
    }
    if(s->mb_x == s->resync_mb_x)
      s->first_slice_line=0;
    s->mb_num_left--;
  }
  ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x-1, s->mb_y, AC_END|DC_END|MV_END);

  return s->mb_y == s->mb_height;
}

/** @} */ // recons group end

/**
 * Initialize decoder.
 */
av_cold int ff_rv34_decode_init(AVCodecContext *avctx)
{
    RV34DecContext *r = avctx->priv_data;
    MpegEncContext *s = &r->s;

    MPV_decode_defaults(s);
    s->avctx= avctx;
    s->out_format = FMT_H263;
    s->codec_id= avctx->codec_id;

    s->width = avctx->width;
    s->height = avctx->height;

    r->s.avctx = avctx;
    avctx->flags |= CODEC_FLAG_EMU_EDGE;
    r->s.flags |= CODEC_FLAG_EMU_EDGE;
    avctx->pix_fmt = PIX_FMT_YUV420P;
    avctx->has_b_frames = 1;
    s->low_delay = 0;

    if (MPV_common_init(s) < 0)
        return -1;

    S32I2M(xr16, 0x7);
#if 0
    enable_idct() ;
    fresh_idct(1) ;
    fresh_idct(0) ;
    set_idct_type_video(REAL, //video
			1    //type
			) ;
    set_idct_stride(16, // in_stride
		    8  // out_stride
		    );
    set_idct_block_width(64, 64); //
    desp_enable_idct();
    set_idct_ddma_dha(TCSM0_PADDR(IDCT_DES_CHAIN));
    idct_chain_tab[0] = 0x80000000;
    idct_chain_tab[1] = 0x1403004c;
    idct_chain_tab[2] = 1 &((1 << 24) - 1);
#endif

    ff_h264_pred_init(&r->h, CODEC_ID_RV40);

    r->intra_types_stride = 4*s->mb_stride + 4;
    r->intra_types_hist = av_malloc(r->intra_types_stride * 4 * 2 * sizeof(*r->intra_types_hist));
    r->intra_types = r->intra_types_hist + r->intra_types_stride * 4;

    r->mb_type = av_mallocz(r->s.mb_stride * r->s.mb_height * sizeof(*r->mb_type));

    r->cbp_luma   = av_malloc(r->s.mb_stride * r->s.mb_height * sizeof(*r->cbp_luma));
    r->cbp_chroma = av_malloc(r->s.mb_stride * r->s.mb_height * sizeof(*r->cbp_chroma));
    r->deblock_coefs = av_malloc(r->s.mb_stride * r->s.mb_height * sizeof(*r->deblock_coefs));

    s->dsp.put_h264_chroma_pixels_tab[0]= put_rv30_chroma_mc8_c;
    s->dsp.put_h264_chroma_pixels_tab[1]= put_rv30_chroma_mc4_c;
    s->dsp.put_h264_chroma_pixels_tab[2]= put_rv30_chroma_mc2_c;
    s->dsp.avg_h264_chroma_pixels_tab[0]= avg_rv30_chroma_mc8_c;
    s->dsp.avg_h264_chroma_pixels_tab[1]= avg_rv30_chroma_mc4_c;
    s->dsp.avg_h264_chroma_pixels_tab[2]= avg_rv30_chroma_mc2_c;

    if(!intra_vlcs[0].cbppattern[0].bits)
        rv34_init_tables();

    return 0;
}

static int get_slice_offset(AVCodecContext *avctx, const uint8_t *buf, int n)
{
    if(avctx->slice_count) return avctx->slice_offset[n];
    else                   return AV_RL32(buf + n*8 - 4) == 1 ? AV_RL32(buf + n*8) :  AV_RB32(buf + n*8);
}

int ff_rv34_decode_frame(AVCodecContext *avctx,
                            void *data, int *data_size,
                            AVPacket *avpkt)
{
    const uint8_t *buf = avpkt->data;
    int buf_size = avpkt->size;
    RV34DecContext *r = avctx->priv_data;
    MpegEncContext *s = &r->s;
    AVFrame *pict = data;
    SliceInfo si;
    int i;
    int slice_count;
    const uint8_t *slices_hdr = NULL;
    int last = 0;

    frame_num++;
    //printf("decode_frame %d\n", frame_num);
    /* no supplementary picture */
    if (buf_size == 0) {
        /* special case for last picture */
        if (s->low_delay==0 && s->next_picture_ptr) {
            *pict= *(AVFrame*)s->next_picture_ptr;
            s->next_picture_ptr= NULL;

            *data_size = sizeof(AVFrame);
        }
        return 0;
    }

    if(!avctx->slice_count){
        slice_count = (*buf++) + 1;
        slices_hdr = buf + 4;
        buf += 8 * slice_count;
    }else
        slice_count = avctx->slice_count;

    //parse first slice header to check whether this frame can be decoded
    if(get_slice_offset(avctx, slices_hdr, 0) > buf_size){
        av_log(avctx, AV_LOG_ERROR, "Slice offset is greater than frame size\n");
        return -1;
    }
    init_get_bits(&s->gb, buf+get_slice_offset(avctx, slices_hdr, 0), buf_size-get_slice_offset(avctx, slices_hdr, 0));
    if(r->parse_slice_header(r, &r->s.gb, &si) < 0 || si.start){
        av_log(avctx, AV_LOG_ERROR, "First slice header is incorrect\n");
        return -1;
    }
    if((!s->last_picture_ptr || !s->last_picture_ptr->data[0]) && si.type == FF_B_TYPE)
        return -1;
    /* skip b frames if we are in a hurry */
    if(avctx->hurry_up && si.type==FF_B_TYPE) return buf_size;
    if(   (avctx->skip_frame >= AVDISCARD_NONREF && si.type==FF_B_TYPE)
       || (avctx->skip_frame >= AVDISCARD_NONKEY && si.type!=FF_I_TYPE)
       ||  avctx->skip_frame >= AVDISCARD_ALL)
        return buf_size;
    /* skip everything if we are in a hurry>=5 */
    if(avctx->hurry_up>=5)
        return buf_size;

    for(i=0; i<slice_count; i++){
        int offset= get_slice_offset(avctx, slices_hdr, i);
        int size;
        if(i+1 == slice_count)
            size= buf_size - offset;
        else
            size= get_slice_offset(avctx, slices_hdr, i+1) - offset;

        if(offset > buf_size){
            av_log(avctx, AV_LOG_ERROR, "Slice offset is greater than frame size\n");
            break;
        }

        r->si.end = s->mb_width * s->mb_height;
        if(i+1 < slice_count){
            init_get_bits(&s->gb, buf+get_slice_offset(avctx, slices_hdr, i+1), (buf_size-get_slice_offset(avctx, slices_hdr, i+1))*8);
            if(r->parse_slice_header(r, &r->s.gb, &si) < 0){
                if(i+2 < slice_count)
                    size = get_slice_offset(avctx, slices_hdr, i+2) - offset;
                else
                    size = buf_size - offset;
            }else
                r->si.end = si.start;
        }
#ifdef JZC_PMON_P0
	PMON_ON(rv8vlc);
#endif
	//printf("decode_slice start\n");
        last = rv34_decode_slice(r, r->si.end, buf + offset, size);
	//printf("decode_slice end\n");
#ifdef JZC_PMON_P0
	PMON_OFF(rv8vlc);
#endif

        s->mb_num_left = r->s.mb_x + r->s.mb_y*r->s.mb_width - r->si.start;
        if(last)
            break;
    }
    //printf("pict_type is %d\n", s->pict_type);
    if(last){
      //if(r->loop_filter)
      //r->loop_filter(r, s->mb_height - 1);
#ifdef JZC_CRC_VER
      {
	int crc_i;
	for(crc_i=0;crc_i<avctx->height;crc_i++)
	  crc_code = crc(s->current_picture.data[0]+crc_i*s->linesize, 
			 avctx->width, crc_code);
	for(crc_i=0;crc_i<(avctx->height>>1);crc_i++){
	  crc_code = crc(s->current_picture.data[1]+crc_i*s->uvlinesize, 
			 (avctx->width>>1), crc_code);
	  crc_code = crc(s->current_picture.data[2]+crc_i*s->uvlinesize, 
			 (avctx->width>>1), crc_code);
	}
	mp_msg(NULL,NULL,"frame: %d, crc_code: 0x%x\n", rvFrame, crc_code);
	rvFrame ++;  
      }
#endif //JZC_CRC_VER
#ifdef JZC_PMON_P0
      {
	int mb_num = ((s->width+15)/16)*((s->height+15)/16);
#if 0
	if (rv8_pmon_p0_frmcnt==0){
	  pmon_p0_fp=fopen("jz4760e_p0.pmon","aw+");
	  fprintf(pmon_p0_fp, "PMON nfl:%s\t(size: %d x %d; mb_num: %d) \n",filename,s->width,s->height,mb_num);
	}

	fprintf(pmon_p0_fp,"PMON frame num: %d\n",rv8_pmon_p0_frmcnt);
	fprintf(pmon_p0_fp,"PMON VLC  -D: %d; I:%d\n",
		rv8vlc_pmon_val/mb_num, rv8vlc_pmon_val_ex/mb_num);
	fprintf(pmon_p0_fp,"PMON WAIT -D: %d; I:%d\n",
		rv8wait_pmon_val/mb_num, rv8wait_pmon_val_ex/mb_num);
	fprintf(pmon_p0_fp,"PMON WHILE -D: %d; I:%d\n",
		rv8while_pmon_val/mb_num, rv8while_pmon_val_ex/mb_num);
#endif
	//all_pmon_val += rv8vlc_pmon_val;
	//printf("PMON:rv8vlc:%d %d %lld\n", rv8vlc_pmon_val, rv8vlc_pmon_val_ex, all_pmon_val);
	rv8vlc_pmon_val=0; rv8vlc_pmon_val_ex=0;
	rv8wait_pmon_val=0; rv8wait_pmon_val_ex=0;
	rv8while_pmon_val=0; rv8while_pmon_val_ex=0;
	rv8_pmon_p0_frmcnt++;
      }
#endif //JZC_PMON_P0
        ff_er_frame_end(s);
        MPV_frame_end(s);
        if (s->pict_type == FF_B_TYPE || s->low_delay) {
            *pict= *(AVFrame*)s->current_picture_ptr;
        } else if (s->last_picture_ptr != NULL) {
            *pict= *(AVFrame*)s->last_picture_ptr;
        }

        if(s->last_picture_ptr || s->low_delay){
            *data_size = sizeof(AVFrame);
            ff_print_debug_info(s, pict);
        }
        s->current_picture_ptr= NULL; //so we can detect if frame_end wasnt called (find some nicer solution...)
    }
    return buf_size;
}

av_cold int ff_rv34_decode_end(AVCodecContext *avctx)
{
    RV34DecContext *r = avctx->priv_data;

    MPV_common_end(&r->s);

    av_freep(&r->intra_types_hist);
    r->intra_types = NULL;
    av_freep(&r->mb_type);
    av_freep(&r->cbp_luma);
    av_freep(&r->cbp_chroma);
    av_freep(&r->deblock_coefs);

    return 0;
}
