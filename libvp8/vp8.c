#include "../libavcore/imgutils.h"
#include "avcodec.h"
#include "./vp56.h"
#include "./vp8data.h"
#include "./vp8dsp.h"
#include "./h264pred.h"
#include "rectangle.h"

#define JZC_VP8_OPT
#ifdef JZC_VP8_OPT
#include "vp8_dcore.h"
#include "t_intpid.h"
#include "t_motion_p0.h"
#include "vp8_tcsm1.h"
#include "vp8_tcsm0.h"
//#include "vp8_sram.h"
#include "../libjzcommon/com_config.h"
#include "../libjzcommon/jzmedia.h"
#include "../libjzcommon/jzasm.h"
#include "../libjzcommon/jz4760e_dcsc.h"
#include "../libjzcommon/jz4760e_tcsm_init.c"
#include "../libjzcommon/jz4760e_2ddma_hw.h"
#include "../libjzcommon/jz4760e_pmon.h"
#include "../libjzcommon/t_vputlb.h"
#include "./vp8_vmau.h"
#include "./vp8_config.h"
#endif

#define JZC_VP8_PMON
#ifdef JZC_VP8_PMON
PMON_CREAT(p0test);
PMON_CREAT(p0allfrm);
PMON_CREAT(mbmd);

PMON_CREAT(mcpoll);
PMON_CREAT(gp0poll);
PMON_CREAT(vmaupoll);

#endif

#define JZC_CRC_VER
#ifdef JZC_CRC_VER
# undef   fprintf
# undef   printf
# include "../libjzcommon/crc.c"
short crc_code;
#else
#undef printf
#endif

int vpFrame = 0;
extern int use_jz_buf;

#ifdef JZC_VP8_OPT
unsigned char  dc_qlookup[128] = {
  4,    5,    6,    7,    8,    9,   10,   10,   11,   12,   13,   14,   15,   16,   17,   17,
  18,   19,   20,   20,   21,   21,   22,   22,   23,   23,   24,   25,   25,   26,   27,   28,
  29,   30,   31,   32,   33,   34,   35,   36,   37,   37,   38,   39,   40,   41,   42,   43,
  44,   45,   46,   46,   47,   48,   49,   50,   51,   52,   53,   54,   55,   56,   57,   58,
  59,   60,   61,   62,   63,   64,   65,   66,   67,   68,   69,   70,   71,   72,   73,   74,
  75,   76,   76,   77,   78,   79,   80,   81,   82,   83,   84,   85,   86,   87,   88,   89,
  91,   93,   95,   96,   98,  100,  101,  102,  104,  106,  108,  110,  112,  114,  116,  118,
  122,  124,  126,  128,  130,  132,  134,  136,  138,  140,  143,  145,  148,  151,  154,  157,
};

unsigned char ac_qlookup[128] = {
  4,    5,    6,    7,    8,    9,   10,   11,   12,   13,   14,   15,   16,   17,   18,   19,
  20,   21,   22,   23,   24,   25,   26,   27,   28,   29,   30,   31,   32,   33,   34,   35,
  36,   37,   38,   39,   40,   41,   42,   43,   44,   45,   46,   47,   48,   49,   50,   51,
  52,   53,   54,   55,   56,   57,   58,   60,   62,   64,   66,   68,   70,   72,   74,   76,
  78,   80,   82,   84,   86,   88,   90,   92,   94,   96,   98,  100,  102,  104,  106,  108,
  110,  112,  114,  116,  119,  122,  125,  128,  131,  134,  137,  140,  143,  146,  149,  152,
  155,  158,  161,  164,  167,  170,  173,  177,  181,  185,  189,  193,  197,  201,  205,  209,
  213,  217,  221,  225,  229,  234,  239,  245,  249,  254,  259-256,  264-256,  269-256,  274-256,  279-256,  284-256,
};

unsigned int ac_qlookup_raw[128] = {
  4,    5,    6,    7,    8,    9,   10,   11,   12,   13,   14,   15,   16,   17,   18,   19,
  20,   21,   22,   23,   24,   25,   26,   27,   28,   29,   30,   31,   32,   33,   34,   35,
  36,   37,   38,   39,   40,   41,   42,   43,   44,   45,   46,   47,   48,   49,   50,   51,
  52,   53,   54,   55,   56,   57,   58,   60,   62,   64,   66,   68,   70,   72,   74,   76,
  78,   80,   82,   84,   86,   88,   90,   92,   94,   96,   98,  100,  102,  104,  106,  108,
  110,  112,  114,  116,  119,  122,  125,  128,  131,  134,  137,  140,  143,  146,  149,  152,
  155,  158,  161,  164,  167,  170,  173,  177,  181,  185,  189,  193,  197,  201,  205,  209,
  213,  217,  221,  225,  229,  234,  239,  245,  249,  254,  259,  264,  269,  274,  279,  284,
};

unsigned char ac2q_lookup[128] = {
};

unsigned char vp8_4x4_pred_lookup[10] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 15
};

unsigned char vp8_16x16_pred_lookup[4] = {
  0, 1, 2, 15
};

static int count = 0;

typedef struct _resid{
  DCTELEM block_dc[16];
  DCTELEM block[6][4][16];
}resid;

static resid *block0;
static resid *block1;
static resid *block2;
static resid *blk;

static VMAU_CHN_REG *mau_reg_ptr0;
static VMAU_CHN_REG *mau_reg_ptr1;
static VMAU_CHN_REG *mau_reg_ptr2;

static VP8_MB_DecARGs *mb0;
static VP8_MB_DecARGs *mb1;
static VP8_MB_DecARGs *mb2;

static uint8_t *motion_dha0;
static uint8_t *motion_dha1;
static uint8_t *motion_dha2;
static volatile uint32_t *motion_dsa0;
static volatile uint32_t *motion_dsa1;
static volatile uint32_t *motion_dsa2;

typedef struct {
  int base[4];
  int ydc_delta[4];
  int y2dc_delta[4];
  int y2ac_delta[4];
  int uvdc_delta[4];
  int uvac_delta[4];
}quant_par;

quant_par vp8_qpar;
#endif

typedef struct {
  AVCodecContext *avctx;
  DSPContext dsp;
  VP8DSPContext vp8dsp;
  H264PredContext hpc;
  vp8_mc_func put_pixels_tab[3][3][3];
  AVFrame frames[4];
  AVFrame *framep[4];
  uint8_t *edge_emu_buffer;
  VP56RangeCoder c;   ///< header context, includes mb modes and motion vectors
  int profile;

  int mb_width;   /* number of horizontal MB */
  int mb_height;  /* number of vertical MB */
  int linesize;
  int uvlinesize;

  int keyframe;
  int invisible;
  int update_last;    ///< update VP56_FRAME_PREVIOUS with the current one
  int update_golden;  ///< VP56_FRAME_NONE if not updated, or which frame to copy if so
  int update_altref;
  int deblock_filter;

  /**
   * If this flag is not set, all the probability updates
   * are discarded after this frame is decoded.
   */
  int update_probabilities;

  /**
   * All coefficients are contained in separate arith coding contexts.
   * There can be 1, 2, 4, or 8 of these after the header context.
   */
  int num_coeff_partitions;
  VP56RangeCoder coeff_partition[8];

  VP8Macroblock *macroblocks;
  VP8Macroblock *macroblocks_base;
  VP8FilterStrength *filter_strength;

  uint8_t *intra4x4_pred_mode_top;
  uint8_t intra4x4_pred_mode_left[4];
  uint8_t *segmentation_map;

  /**
   * Cache of the top row needed for intra prediction
   * 16 for luma, 8 for each chroma plane
   */
  uint8_t (*top_border)[16+8+8];

  /**
   * For coeff decode, we need to know whether the above block had non-zero
   * coefficients. This means for each macroblock, we need data for 4 luma
   * blocks, 2 u blocks, 2 v blocks, and the luma dc block, for a total of 9
   * per macroblock. We keep the last row in top_nnz.
   */
  uint8_t (*top_nnz)[9];
  DECLARE_ALIGNED(8, uint8_t, left_nnz)[9];

  /**
   * This is the index plus one of the last non-zero coeff
   * for each of the blocks in the current macroblock.
   * So, 0 -> no coeffs
   *     1 -> dc-only (special transform)
   *     2+-> full transform
   */
  DECLARE_ALIGNED(16, uint8_t, non_zero_count_cache)[6][4];
  DECLARE_ALIGNED(16, DCTELEM, block)[6][4][16];
  DECLARE_ALIGNED(16, DCTELEM, block_dc)[16];
  uint8_t intra4x4_pred_mode_mb[16];

  int chroma_pred_mode;    ///< 8x8c pred mode of the current macroblock
  int segment;             ///< segment of the current macroblock

  int mbskip_enabled;
  int sign_bias[4]; ///< one state [0, 1] per ref frame type
  int ref_count[3];

  /**
   * Base parameters for segmentation, i.e. per-macroblock parameters.
   * These must be kept unchanged even if segmentation is not used for
   * a frame, since the values persist between interframes.
   */
  struct {
    int enabled;
    int absolute_vals;
    int update_map;
    int8_t base_quant[4];
    int8_t filter_level[4];     ///< base loop filter level
  } segmentation;

  /**
   * Macroblocks can have one of 4 different quants in a frame when
   * segmentation is enabled.
   * If segmentation is disabled, only the first segment's values are used.
   */
  struct {
    // [0] - DC qmul  [1] - AC qmul
    int16_t luma_qmul[2];
    int16_t luma_dc_qmul[2];    ///< luma dc-only block quant
    int16_t chroma_qmul[2];
  } qmat[4];

  struct {
    int simple;
    int level;
    int sharpness;
  } filter;

  struct {
    int enabled;    ///< whether each mb can have a different strength based on mode/ref

    /**
     * filter strength adjustment for the following macroblock modes:
     * [0] - i4x4
     * [1] - zero mv
     * [2] - inter modes except for zero or split mv
     * [3] - split mv
     *  i16x16 modes never have any adjustment
     */
    int8_t mode[4];

    /**
     * filter strength adjustment for macroblocks that reference:
     * [0] - intra / VP56_FRAME_CURRENT
     * [1] - VP56_FRAME_PREVIOUS
     * [2] - VP56_FRAME_GOLDEN
     * [3] - altref / VP56_FRAME_GOLDEN2
     */
    int8_t ref[4];
  } lf_delta;

  /**
   * These are all of the updatable probabilities for binary decisions.
   * They are only implictly reset on keyframes, making it quite likely
   * for an interframe to desync if a prior frame's header was corrupt
   * or missing outright!
   */
  struct {
    uint8_t segmentid[3];
    uint8_t mbskip;
    uint8_t intra;
    uint8_t last;
    uint8_t golden;
    uint8_t pred16x16[4];
    uint8_t pred8x8c[3];
    /* Padded to allow overreads */
    uint8_t token[4][17][3][NUM_DCT_TOKENS-1];
    uint8_t mvc[2][19];
  } prob[2];
} VP8Context;

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

#ifdef JZC_VP8_OPT
void motion_init_vp8(int intpid, int cintpid)
{
  int i;

  for(i=0; i<16; i++){
    SET_TAB1_ILUT(i,/*idx*/
		  IntpFMT[intpid][i].intp[1],/*intp2*/
		  IntpFMT[intpid][i].intp_pkg[1],/*intp2_pkg*/
		  IntpFMT[intpid][i].hldgl,/*hldgl*/
		  IntpFMT[intpid][i].avsdgl,/*avsdgl*/
		  IntpFMT[intpid][i].intp_dir[1],/*intp2_dir*/
		  IntpFMT[intpid][i].intp_rnd[1],/*intp2_rnd*/
		  IntpFMT[intpid][i].intp_sft[1],/*intp2_sft*/
		  IntpFMT[intpid][i].intp_sintp[1],/*sintp2*/
		  IntpFMT[intpid][i].intp_srnd[1],/*sintp2_rnd*/
		  IntpFMT[intpid][i].intp_sbias[1],/*sintp2_bias*/
		  IntpFMT[intpid][i].intp[0],/*intp1*/
		  IntpFMT[intpid][i].tap,/*tap*/
		  IntpFMT[intpid][i].intp_pkg[0],/*intp1_pkg*/
		  IntpFMT[intpid][i].intp_dir[0],/*intp1_dir*/
		  IntpFMT[intpid][i].intp_rnd[0],/*intp1_rnd*/
		  IntpFMT[intpid][i].intp_sft[0],/*intp1_sft*/
		  IntpFMT[intpid][i].intp_sintp[0],/*sintp1*/
		  IntpFMT[intpid][i].intp_srnd[0],/*sintp1_rnd*/
		  IntpFMT[intpid][i].intp_sbias[0]/*sintp1_bias*/
		  );
    SET_TAB1_CLUT(i,/*idx*/
		  IntpFMT[intpid][i].intp_coef[0][7],/*coef8*/
		  IntpFMT[intpid][i].intp_coef[0][6],/*coef7*/
		  IntpFMT[intpid][i].intp_coef[0][5],/*coef6*/
		  IntpFMT[intpid][i].intp_coef[0][4],/*coef5*/
		  IntpFMT[intpid][i].intp_coef[0][3],/*coef4*/
		  IntpFMT[intpid][i].intp_coef[0][2],/*coef3*/
		  IntpFMT[intpid][i].intp_coef[0][1],/*coef2*/
		  IntpFMT[intpid][i].intp_coef[0][0] /*coef1*/
		  );
    SET_TAB1_CLUT(16+i,/*idx*/
		  IntpFMT[intpid][i].intp_coef[1][7],/*coef8*/
		  IntpFMT[intpid][i].intp_coef[1][6],/*coef7*/
		  IntpFMT[intpid][i].intp_coef[1][5],/*coef6*/
		  IntpFMT[intpid][i].intp_coef[1][4],/*coef5*/
		  IntpFMT[intpid][i].intp_coef[1][3],/*coef4*/
		  IntpFMT[intpid][i].intp_coef[1][2],/*coef3*/
		  IntpFMT[intpid][i].intp_coef[1][1],/*coef2*/
		  IntpFMT[intpid][i].intp_coef[1][0] /*coef1*/
		  );

    SET_TAB2_ILUT(i,/*idx*/
		  IntpFMT[cintpid][i].intp[1],/*intp2*/
		  IntpFMT[cintpid][i].intp_dir[1],/*intp2_dir*/
		  IntpFMT[cintpid][i].intp_sft[1],/*intp2_sft*/
		  IntpFMT[cintpid][i].intp_coef[1][0],/*intp2_lcoef*/
		  IntpFMT[cintpid][i].intp_coef[1][1],/*intp2_rcoef*/
		  IntpFMT[cintpid][i].intp_rnd[1],/*intp2_rnd*/
		  IntpFMT[cintpid][i].intp[0],/*intp1*/
		  IntpFMT[cintpid][i].intp_dir[0],/*intp1_dir*/
		  IntpFMT[cintpid][i].intp_sft[0],/*intp1_sft*/
		  IntpFMT[cintpid][i].intp_coef[0][0],/*intp1_lcoef*/
		  IntpFMT[cintpid][i].intp_coef[0][1],/*intp1_rcoef*/
		  IntpFMT[cintpid][i].intp_rnd[0]/*intp1_rnd*/
		  );
  }

  SET_REG1_STAT(1,/*pfe*/
		1,/*lke*/
		1 /*tke*/);
  SET_REG2_STAT(1,/*pfe*/
		1,/*lke*/
		1 /*tke*/);

  //            ebms,esms,earm,epmv,esa,ebme,cae,pgc,ch2en,pri,ckge,ofa,rot,rotdir,wm,ccf,irqe,rst,en)
  SET_REG1_CTRL(0,   0,   0,   0,   0,  0,   0,  0xF,1,    3,  1,   0,  0,  0,     0, 1,  0,   0,  1);

  SET_REG1_BINFO(AryFMT[intpid],/*ary*/
		 0,/*doe*/
		 0,/*expdy*/
		 0,/*expdx*/
		 0,/*ilmd*/
		 SubPel[intpid]-1,/*pel*/
		 0,/*fld*/
		 0,/*fldsel*/
		 0,/*boy*/
		 0,/*box*/
		 0,/*bh*/
		 0,/*bw*/
		 0/*pos*/);
  SET_REG2_BINFO(0,/*ary*/
		 0,/*doe*/
		 0,/*expdy*/
		 0,/*expdx*/
		 0,/*ilmd*/
		 0,/*pel*/
		 0,/*fld*/
		 0,/*fldsel*/
		 0,/*boy*/
		 0,/*box*/
		 0,/*bh*/
		 0,/*bw*/
		 0/*pos*/);

  SET_REG1_PINFO(0,/*rgr*/
		 0,/*its*/
		 0,/*its_sft*/
		 0,/*its_scale*/
		 0/*its_rnd*/);
  SET_REG2_PINFO(0,/*rgr*/
		 0,/*its*/
		 0,/*its_sft*/
		 0,/*its_scale*/
		 0/*its_rnd*/);

  SET_REG1_WINFO(0,/*wt*/
		 0, /*wtpd*/
		 0,/*wtmd*/
		 0,/*biavg_rnd*/
		 0,/*wt_denom*/
		 0,/*wt_sft*/
		 0,/*wt_lcoef*/
		 0/*wt_rcoef*/);
  SET_REG1_WTRND(0);

  SET_REG2_WINFO1(0,/*wt*/
		  0, /*wtpd*/
		  0,/*wtmd*/
		  0,/*biavg_rnd*/
		  0,/*wt_denom*/
		  0,/*wt_sft*/
		  0,/*wt_lcoef*/
		  0/*wt_rcoef*/);
  SET_REG2_WINFO2(0,/*wt_sft*/
		  0,/*wt_lcoef*/
		  0/*wt_rcoef*/);
  SET_REG2_WTRND(0, 0);
}

void motion_config_vp8(VP8Context *s)
{
  //            ebms,esms,earm,epmv,esa,ebme,cae,pgc,ch2en,pri,ckge,ofa,rot,rotdir,wm,ccf,irqe,rst,en)
  SET_REG1_CTRL(0,   0,   0,   0,   0,  0,   1,  0xF,1,    3,  1,   1,  0,  0,     0, 1,  0,   0,  1);

  //SET_REG1_STRD(s->linesize/16,0,PREVIOUS_LUMA_STRIDE);
  //printf("motion_config_vp8 %d %d\n", s->linesize/16, s->mb_width*16);
  SET_REG1_STRD((s->mb_width+4)*16,0,16);
  SET_REG1_GEOM(s->mb_height*16,s->mb_width*16);    
  //SET_REG2_STRD(s->linesize/16,0,PREVIOUS_CHROMA_STRIDE);
  SET_REG2_STRD((s->mb_width+4)*16,0,8);
}

static void vp8_quant_init(){
  int i;

  //printf("vp8_quant_init start\n");
  for ( i = 0 ; i<128; i++){
    int tmp;
    tmp = ((int)(ac_qlookup_raw[i]*155)/100);
    ac2q_lookup[i] = tmp <8? 8: (tmp > 255? (tmp-256): tmp);
    //printf("%d-%x,", i, ac2q_lookup[i]);    
    //printf("%d\n", ac_qlookup_raw[i]);    
  }

  unsigned int vmau_qt_addr = VMAU_V_BASE + VMAU_QT_BASE;

  unsigned int *qlook_ptr;
  qlook_ptr = ac_qlookup;
  for ( i = 0 ; i < 128/4 ; i ++){
    write_reg(vmau_qt_addr, qlook_ptr[i]);
    vmau_qt_addr += 4;
  } 

  qlook_ptr = ac2q_lookup;
  for ( i = 0 ; i < 128/4 ; i ++){
    write_reg(vmau_qt_addr, qlook_ptr[i]);
    vmau_qt_addr += 4;
  } 

  qlook_ptr = dc_qlookup;
  for ( i = 0 ; i < 128/4 ; i ++){
    write_reg(vmau_qt_addr, qlook_ptr[i]);
    vmau_qt_addr += 4;
  }
}

static void get_mb_args(VP8Context *s, VP8Macroblock *mb, int mb_x, int mb_y){
  mb0->mb_x = mb_x;
  mb0->mb_y = mb_y;
  mb0->segment = s->segment;
  memcpy(&(mb0->mb), mb, sizeof(VP8Macroblock));

  return;
}

uint8_t BlkIdx[4] = {0, 2, 8, 10};
uint8_t SblkIdx[4] = {0, 1, 4, 5};

static av_always_inline
void motion_task(int *tdd, int ref, 
		 int mvy, int mvx,
		 int blkh, int blkw,
		 int boy, int box)
{
  tdd[0] = TDD_MV(mvy, mvx);
  tdd[1] = TDD_CMD(0,/*bidir*/
		   0,/*refdir*/
		   0,/*fld*/
		   0,/*fldsel*/
		   0,/*rgr*/
		   0,/*its*/
		   0,/*doe*/
		   0,/*cflo*/
		   mvy & 0x7,/*ypos*/
		   IS_ILUT0,/*lilmd*/
		   IS_EC,/*cilmd*/
		   ref,/*list*/
		   boy,/*boy*/
		   box,/*box*/
		   blkh,/*bh*/
		   blkw,/*bw*/
		   mvx & 0x7/*xpos*/);
}

static av_always_inline
void vp8_motion_split(int mbpar, int mbref, int mb_x, int mb_y, VP56mv *mv){
  VP56mv *bmv = mv;

  volatile int *tdd = (int *)motion_dha0;
  int tkn = 0;

  if (mbpar == 3){
    int i,j;
    
    for (i = 0; i < 4; i++){
      int acc_mvx=0, acc_mvy=0;
      *tdd++ = TDD_HEAD(1,/*vld*/
			1,/*lk*/
			0,/*op*/
			SubPel[VP8_QPEL]-1,/*ch1pel*/
			SubPel[H264_EPEL],/*ch2pel*/
			TDD_POS_AUTO/*TDD_POS_SPEC*/,/*posmd*/
			TDD_MV_SPEC/*TDD_MV_SPEC*/,/*mvmd*/
			5,/*tkn*/
			mb_y,/*mby*/
			mb_x/*mbx*/);

      for (j = 0; j < 4; j++){
	acc_mvx += bmv[ BlkIdx[i] + SblkIdx[j] ].x;
	acc_mvy += bmv[ BlkIdx[i] + SblkIdx[j] ].y;

	*tdd++ = TDD_MV(bmv[ BlkIdx[i] + SblkIdx[j] ].y, bmv[ BlkIdx[i] + SblkIdx[j] ].x);
	//*tdd++ = TDD_MV(bmv[i*4+j].y, bmv[i*4+j].x);
	*tdd++ = TDD_CMD(0,/*bidir*/
			 0,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 0,/*doe*/
			 0,/*cflo*/
			 bmv[i*4+j].y & 0x7,/*ypos*/
			 IS_ILUT0,/*lilmd*/
			 IS_EC,/*cilmd*/
			 mbref,/*list*/
			 (i & 0x2) + (j & 0x2)/2,/*boy*/
			 (i & 0x1)*2 + (j & 0x1),/*box*/
			 BLK_H4,/*bh*/
			 BLK_W4,/*bw*/
			 bmv[i*4+j].x & 0x7/*xpos*/);
      }
      tdd[-1] |= 0x1<<TDD_CFLO_SFT;
      acc_mvy = (acc_mvy<0)? ((acc_mvy - 2)/4) : ((acc_mvy + 2)/4); 
      //acc_mvy = (acc_mvy<0)? ((acc_mvy + 3)/4) : ((acc_mvy + 2)/4);
      acc_mvx = (acc_mvx<0)? ((acc_mvx - 2)/4) : ((acc_mvx + 2)/4); 
      //acc_mvx = (acc_mvx<0)? ((acc_mvx + 3)/4) : ((acc_mvx + 2)/4);

      *tdd++ = TDD_MV(acc_mvy, acc_mvx);
      *tdd++ = TDD_CMD(0,/*bidir*/
		       0,/*refdir*/
		       0,/*fld*/
		       0,/*fldsel*/
		       0,/*rgr*/
		       0,/*its*/
		       0,/*doe*/
		       0,/*cflo*/
		       acc_mvy & 0x7,/*ypos*/
		       IS_ILUT0,/*lilmd*/
		       IS_EC,/*cilmd*/
		       mbref,/*list*/
		       (i & 0x2),/*boy*/
		       (i & 0x1)*2,/*box*/
		       BLK_H8,/*bh*/
		       BLK_W8,/*bw*/
		       acc_mvx & 0x7/*xpos*/);
    }
    tdd[-1] |= 0x1<<TDD_DOE_SFT;
    tdd[-3] |= 0x1<<TDD_DOE_SFT;
    *tdd++ = TDD_HEAD(1,/*vld*/
		      1,/*lk*/
		      0,/*op*/
		      0,/*ch1pel*/
		      0,/*ch2pel*/ 
		      TDD_POS_AUTO,/*posmd*/
		      TDD_MV_AUTO,/*mvmd*/ 
		      0,/*tkn*/
		      mb_y,/*mby*/
		      mb_x/*mbx*/);
    *tdd++ = SYN_HEAD(1, 0, 2, 0, 0xFFFF);
    tdd = (int *)motion_dha0;
    for (i = 0; i < 48; i++){
      //printf("tdd[%d] is 0x%08x\n", i, tdd[i]);
    }
  }else if(mbpar == 2){
    *tdd++ = TDD_HEAD(1,/*vld*/
		      1,/*lk*/
		      0,/*op*/
		      SubPel[VP8_QPEL]-1,/*ch1pel*/
		      SubPel[H264_EPEL],/*ch2pel*/ 
		      TDD_POS_AUTO,/*posmd*/
		      TDD_MV_AUTO,/*mvmd*/ 
		      4,/*tkn*/
		      mb_y,/*mby*/
		      mb_x/*mbx*/);
    motion_task(tdd, mbref, bmv[0].y, bmv[0].x, BLK_H8, BLK_W8, 0, 0);
    tdd += 2;
    motion_task(tdd, mbref, bmv[1].y, bmv[1].x, BLK_H8, BLK_W8, 0, 2);
    tdd += 2;
    motion_task(tdd, mbref, bmv[2].y, bmv[2].x, BLK_H8, BLK_W8, 2, 0);
    tdd += 2;
    motion_task(tdd, mbref, bmv[3].y, bmv[3].x, BLK_H8, BLK_W8, 2, 2);
    tdd += 2;
    tdd[-1] |= 0x1<<TDD_DOE_SFT;
    *tdd++ = TDD_HEAD(1,/*vld*/
		      1,/*lk*/
		      0,/*op*/
		      0,/*ch1pel*/
		      0,/*ch2pel*/ 
		      TDD_POS_AUTO,/*posmd*/
		      TDD_MV_AUTO,/*mvmd*/ 
		      0,/*tkn*/
		      mb_y,/*mby*/
		      mb_x/*mbx*/);
    *tdd++ = SYN_HEAD(1, 0, 2, 0, 0xFFFF);
  }else if (mbpar == 1){
    *tdd++ = TDD_HEAD(1,/*vld*/
		      1,/*lk*/
		      0,/*op*/
		      SubPel[VP8_QPEL]-1,/*ch1pel*/
		      SubPel[H264_EPEL],/*ch2pel*/ 
		      TDD_POS_AUTO,/*posmd*/
		      TDD_MV_AUTO,/*mvmd*/ 
		      2,/*tkn*/
		      mb_y,/*mby*/
		      mb_x/*mbx*/);
    motion_task(tdd, mbref, bmv[0].y, bmv[0].x, BLK_H16, BLK_W8, 0, 0);
    tdd += 2;
    motion_task(tdd, mbref, bmv[1].y, bmv[1].x, BLK_H16, BLK_W8, 0, 2);
    tdd += 2;
    tdd[-1] |= 0x1<<TDD_DOE_SFT;
    *tdd++ = TDD_HEAD(1,/*vld*/
		      1,/*lk*/
		      0,/*op*/
		      0,/*ch1pel*/
		      0,/*ch2pel*/ 
		      TDD_POS_AUTO,/*posmd*/
		      TDD_MV_AUTO,/*mvmd*/ 
		      0,/*tkn*/
		      mb_y,/*mby*/
		      mb_x/*mbx*/);
    *tdd++ = SYN_HEAD(1, 0, 2, 0, 0xFFFF);
  }else{
    *tdd++ = TDD_HEAD(1,/*vld*/
		      1,/*lk*/
		      0,/*op*/
		      SubPel[VP8_QPEL]-1,/*ch1pel*/
		      SubPel[H264_EPEL],/*ch2pel*/ 
		      TDD_POS_AUTO,/*posmd*/
		      TDD_MV_AUTO,/*mvmd*/ 
		      2,/*tkn*/
		      mb_y,/*mby*/
		      mb_x/*mbx*/);
    motion_task(tdd, mbref, bmv[0].y, bmv[0].x, BLK_H8, BLK_W16, 0, 0);
    tdd += 2;
    motion_task(tdd, mbref, bmv[1].y, bmv[1].x, BLK_H8, BLK_W16, 2, 0);
    tdd += 2;
    tdd[-1] |= 0x1<<TDD_DOE_SFT;
    *tdd++ = TDD_HEAD(1,/*vld*/
		      1,/*lk*/
		      0,/*op*/
		      0,/*ch1pel*/
		      0,/*ch2pel*/ 
		      TDD_POS_AUTO,/*posmd*/
		      TDD_MV_AUTO,/*mvmd*/ 
		      0,/*tkn*/
		      mb_y,/*mby*/
		      mb_x/*mbx*/);
    *tdd++ = SYN_HEAD(1, 0, 2, 0, 0xFFFF);
  }

  SET_REG1_DSA(TCSM1_PADDR((int)motion_dsa0));
  SET_REG1_DDC(TCSM1_PADDR((int)motion_dha0) | 0x1);

  return;
}

static av_always_inline
void JZC_vp8_mc_part(VP56mv *mv, int mb_x, int mb_y, int list){
  volatile int *tdd = (int *)motion_dha0;
  int tkn = 0;

  tdd[tkn++] = TDD_HEAD(1,/*vld*/
			1,/*lk*/
			0,/*op*/
			1,/*ch1pel*/
			3,/*ch2pel*/
			0/*TDD_POS_SPEC*/,/*posmd*/
			0/*TDD_MV_SPEC*/,/*mvmd*/
			1,/*tkn*/
			mb_y,/*mby*/
			mb_x/*mbx*/);

  tdd[tkn++] = TDD_MV(mv->y, mv->x);
  tdd[tkn++] = TDD_CMD(0,/*bidir*/
		       0,/*refdir*/
		       0,/*fld*/
		       0,/*fldsel*/
		       0,/*rgr*/
		       0,/*its*/
		       1,/*doe*/
		       0,/*cflo*/
		       mv->y&0x7,/*ypos*/
		       IS_ILUT0,/*lilmd*/
		       IS_EC,/*cilmd*/
		       list,/*list*/
		       0,/*boy*/
		       0,/*box*/
		       BLK_H16,/*bh*/
		       BLK_W16,/*bw*/
		       mv->x&0x7/*xpos*/);

  tdd[tkn++] = TDD_HEAD(1,/*vld*/
			1,/*lk*/
			0,/*op*/
			0,/*ch1pel*/
			0,/*ch2pel*/
			0/*TDD_POS_SPEC*/,/*posmd*/
			0/*TDD_MV_SPEC*/,/*mvmd*/
			0,/*tkn*/
			0xFF,/*mby*/
			0xFF/*mbx*/);

  tdd[tkn++] = SYN_HEAD(1, 0, 2, 0, 0xFFFF);

  if (vpFrame == 5 && mb_x == 0 && mb_y == 0){
    for (int i = 0; i < 8; i++){
      //printf("tdd[%d] is 0x%08x\n", i, tdd[i]);
    }
  }

  SET_REG1_DSA(TCSM1_PADDR((int)motion_dsa0));
  SET_REG1_DDC(TCSM1_PADDR((int)motion_dha0) | 0x1);

  return;
}

static av_always_inline
void JZC_inter_predict(VP8Macroblock *mb,int mb_x, int mb_y){
  VP56mv *bmv = mb->bmv;

  if (mb->mode < VP8_MVMODE_SPLIT) {
    JZC_vp8_mc_part(&mb->mv, mb_x, mb_y, mb->ref_frame);
  }else{
    vp8_motion_split(mb->partitioning, mb->ref_frame, mb_x, mb_y, bmv);
  }
#if 0
  if (vpFrame == 4 && mb_x == 9 && mb_y == 6){
    printf("GLOBAL_CTRL:0x%08x\n", GET_REG1_CTRL());
    printf("STATUS:0x%08x\n", GET_REG1_STAT());
    printf("MBPOS:0x%08x\n", GET_REG1_MBPOS());
    printf("MVPA:0x%08x\n", GET_REG1_MVPA());
    printf("RAWA:0x%08x\n", GET_REG1_RAWA());
    printf("MVINFO:0x%08x\n", GET_REG1_CMV());
    printf("ROA:0x%08x\n", GET_REG1_REFA());
    printf("DSTA:0x%08x\n", GET_REG1_DSTA());
    printf("PINFO:0x%08x\n", GET_REG1_PINFO());
    printf("WINFO1:0x%08x\n", GET_REG1_WINFO());
    printf("WTRD:0x%08x\n", GET_REG1_WTRND());
    printf("BINFO:;0x%08x\n", GET_REG1_BINFO());
    printf("INFO1:0x%08x\n", GET_REG1_IINFO1());
    printf("INFO2:0x%08x\n", GET_REG1_IINFO2());
    printf("TAP1L:0x%08x\n", GET_REG1_TAP1L());
    printf("TAP1M:0x%08x\n", GET_REG1_TAP1M());
    printf("TAP2L:0x%08x\n", GET_REG1_TAP2L());
    printf("TAP2M:0x%08x\n", GET_REG1_TAP2M());
    printf("STRID:0x%08x\n", GET_REG1_STRD());
    printf("GEOM:0x%08x\n", GET_REG1_GEOM());
    printf("DDC:0x%08x\n", GET_REG1_DDC());
    printf("DSA:0x%08x\n", GET_REG1_DSA());
    printf("REF1:0x%08x\n", GET_TAB1_RLUT(1));
    printf("REF2:0x%08x\n", GET_TAB1_RLUT(2));
    printf("REF3:0x%08x\n", GET_TAB1_RLUT(3));
  }
#endif
  //printf("motion_dsa 0x%08x\n", *(volatile int *)motion_dsa);

#if 0
  //if (1){
  if (vpFrame == 29 && mb_x == 8 && mb_y == 14){
    volatile uint8_t *res = VMAU_MC_Y0;
    //uint8_t *res = TCSM1_VCADDR(RECON_YBUF1);
    printf("mc poll mb_x:%d mb_y:%d\n", mb_x, mb_y);
    ptr_square(res, 1, 128, 16, 16);
    printf("\n");
    res = VMAU_MC_C0;
    //res = TCSM1_VCADDR(RECON_CBUF1);
    ptr_square(res, 1, 8, 8, 16);
    printf("\n");
    ptr_square(res+8, 1, 8, 8, 16);
    printf("\n");
  }
#endif
  return;
}
#endif

static void linear_crc_frame(uint8_t *ybuf, uint8_t *cbuf, int mb_width, int mb_height){
#ifdef JZC_CRC_VER
  int crc_i;
  int crc_j = 0;

  char *ptr_y = ybuf;
  char *ptr_c = cbuf;

  for(crc_i=0; crc_i<mb_height; crc_i++){
    for (crc_j = 0; crc_j < mb_width; crc_j++){
      crc_code = crc(ptr_y, 256, crc_code);
      ptr_y += 256;
    }
    ptr_y += 1024;
  }

  for(crc_i=0; crc_i<mb_height; crc_i++){
    for (crc_j = 0; crc_j < mb_width; crc_j++){
      crc_code = crc(ptr_c, 128, crc_code);
      ptr_c += 128;
    }
    ptr_c += 512;
  }

  printf("frame: %d, crc_code: 0x%x\n", vpFrame, crc_code);
#endif  
}

static void printf_frm_data(uint8_t *ybuf, uint8_t *cbuf, int mb_width, int mb_height){
  int i,j;

  char *ptr_y = ybuf;
  char *ptr_c = cbuf;

  for (j = 0; j < mb_height; j++){
    for (i = 0; i < mb_width; i++){
      printf("mbx:%d mby:%d\n", i, j);
      ptr_square(ptr_y, 1, 16, 16, 16);
      ptr_y += 256;
    }
    ptr_y += 1024;
  }

  for (j = 0; j < mb_height; j++){
    for (i = 0; i < mb_width; i++){
      printf("mbx:%d mby:%d\n", i, j);
      ptr_square(ptr_c, 1, 8, 8, 16);
      ptr_c += 128;
    }
    ptr_c += 512;
  }

  ptr_c = cbuf+8;
  for (j = 0; j < mb_height; j++){
    for (i = 0; i < mb_width; i++){
      printf("mbx:%d mby:%d\n", i, j);
      ptr_square(ptr_c, 1, 8, 8, 16);
      ptr_c += 128;
    }
    ptr_c += 512;
  }
}

static void vp8_decode_flush(AVCodecContext *avctx)
{
  VP8Context *s = avctx->priv_data;
  int i;

  for (i = 0; i < 4; i++)
    if (s->frames[i].data[0])
      avctx->release_buffer(avctx, &s->frames[i]);
  memset(s->framep, 0, sizeof(s->framep));

  av_freep(&s->macroblocks_base);
  av_freep(&s->filter_strength);
  av_freep(&s->intra4x4_pred_mode_top);
  av_freep(&s->top_nnz);
  av_freep(&s->edge_emu_buffer);
  av_freep(&s->top_border);
  av_freep(&s->segmentation_map);

  s->macroblocks        = NULL;
}

static int update_dimensions(VP8Context *s, int width, int height)
{
  if (av_image_check_size(width, height, 0, s->avctx))
    return AVERROR_INVALIDDATA;

  vp8_decode_flush(s->avctx);

  avcodec_set_dimensions(s->avctx, width, height);

  s->mb_width  = (s->avctx->coded_width +15) / 16;
  s->mb_height = (s->avctx->coded_height+15) / 16;

  s->macroblocks_base        = av_mallocz((s->mb_width+s->mb_height*2+1)*sizeof(*s->macroblocks));
  s->filter_strength         = av_mallocz(s->mb_width*sizeof(*s->filter_strength));
  s->intra4x4_pred_mode_top  = av_mallocz(s->mb_width*4);
  s->top_nnz                 = av_mallocz(s->mb_width*sizeof(*s->top_nnz));
  s->top_border              = av_mallocz((s->mb_width+1)*sizeof(*s->top_border));
  s->segmentation_map        = av_mallocz(s->mb_width*s->mb_height);

  if (!s->macroblocks_base || !s->filter_strength || !s->intra4x4_pred_mode_top ||
      !s->top_nnz || !s->top_border || !s->segmentation_map)
    return AVERROR(ENOMEM);

  s->macroblocks        = s->macroblocks_base + 1;

  return 0;
}

static void parse_segment_info(VP8Context *s)
{
  VP56RangeCoder *c = &s->c;
  int i;

  s->segmentation.update_map = vp8_rac_get(c);

  if (vp8_rac_get(c)) { // update segment feature data
    s->segmentation.absolute_vals = vp8_rac_get(c);

    for (i = 0; i < 4; i++)
      s->segmentation.base_quant[i]   = vp8_rac_get_sint(c, 7);

    for (i = 0; i < 4; i++)
      s->segmentation.filter_level[i] = vp8_rac_get_sint(c, 6);
  }
  if (s->segmentation.update_map)
    for (i = 0; i < 3; i++)
      s->prob->segmentid[i] = vp8_rac_get(c) ? vp8_rac_get_uint(c, 8) : 255;
}

static void update_lf_deltas(VP8Context *s)
{
  VP56RangeCoder *c = &s->c;
  int i;

  for (i = 0; i < 4; i++)
    s->lf_delta.ref[i]  = vp8_rac_get_sint(c, 6);

  for (i = 0; i < 4; i++)
    s->lf_delta.mode[i] = vp8_rac_get_sint(c, 6);
}

static int setup_partitions(VP8Context *s, const uint8_t *buf, int buf_size)
{
  const uint8_t *sizes = buf;
  int i;

  s->num_coeff_partitions = 1 << vp8_rac_get_uint(&s->c, 2);

  buf      += 3*(s->num_coeff_partitions-1);
  buf_size -= 3*(s->num_coeff_partitions-1);
  if (buf_size < 0)
    return -1;

  for (i = 0; i < s->num_coeff_partitions-1; i++) {
    int size = AV_RL24(sizes + 3*i);
    if (buf_size - size < 0)
      return -1;

    ff_vp56_init_range_decoder(&s->coeff_partition[i], buf, size);
    buf      += size;
    buf_size -= size;
  }
  ff_vp56_init_range_decoder(&s->coeff_partition[i], buf, buf_size);

  return 0;
}

static void get_quants(VP8Context *s)
{
  VP56RangeCoder *c = &s->c;
  int i, base_qi;

  int yac_qi     = vp8_rac_get_uint(c, 7);
  int ydc_delta  = vp8_rac_get_sint(c, 4);
  int y2dc_delta = vp8_rac_get_sint(c, 4);
  int y2ac_delta = vp8_rac_get_sint(c, 4);
  int uvdc_delta = vp8_rac_get_sint(c, 4);
  int uvac_delta = vp8_rac_get_sint(c, 4);

  for (i = 0; i < 4; i++) {
    if (s->segmentation.enabled) {
      base_qi = s->segmentation.base_quant[i];
      if (!s->segmentation.absolute_vals)
	base_qi += yac_qi;
    } else
      base_qi = yac_qi;
#ifdef JZC_VP8_OPT
    vp8_qpar.base[i] = base_qi;
#endif
    s->qmat[i].luma_qmul[0]    =       vp8_dc_qlookup[av_clip(base_qi + ydc_delta , 0, 127)];
    s->qmat[i].luma_qmul[1]    =       vp8_ac_qlookup[av_clip(base_qi             , 0, 127)];
    s->qmat[i].luma_dc_qmul[0] =   2 * vp8_dc_qlookup[av_clip(base_qi + y2dc_delta, 0, 127)];
    s->qmat[i].luma_dc_qmul[1] = 155 * vp8_ac_qlookup[av_clip(base_qi + y2ac_delta, 0, 127)] / 100;
    s->qmat[i].chroma_qmul[0]  =       vp8_dc_qlookup[av_clip(base_qi + uvdc_delta, 0, 127)];
    s->qmat[i].chroma_qmul[1]  =       vp8_ac_qlookup[av_clip(base_qi + uvac_delta, 0, 127)];

    s->qmat[i].luma_dc_qmul[1] = FFMAX(s->qmat[i].luma_dc_qmul[1], 8);
    s->qmat[i].chroma_qmul[0]  = FFMIN(s->qmat[i].chroma_qmul[0], 132);
  }
  
#ifdef JZC_VP8_OPT
  for (i = 0; i < 4; i++){
    vp8_qpar.ydc_delta[i] = av_clip(vp8_qpar.base[i]+ydc_delta, 0, 127) - vp8_qpar.base[i];
    vp8_qpar.y2dc_delta[i] = av_clip(vp8_qpar.base[i]+y2dc_delta, 0, 127) - vp8_qpar.base[i];
    vp8_qpar.y2ac_delta[i] = av_clip(vp8_qpar.base[i]+y2ac_delta, 0, 127) - vp8_qpar.base[i];
    vp8_qpar.uvdc_delta[i] = av_clip(vp8_qpar.base[i]+uvdc_delta, 0, 127) - vp8_qpar.base[i];
    vp8_qpar.uvac_delta[i] = av_clip(vp8_qpar.base[i]+uvac_delta, 0, 127) - vp8_qpar.base[i];
  }
#endif
}

/**
 * Determine which buffers golden and altref should be updated with after this frame.
 * The spec isn't clear here, so I'm going by my understanding of what libvpx does
 *
 * Intra frames update all 3 references
 * Inter frames update VP56_FRAME_PREVIOUS if the update_last flag is set
 * If the update (golden|altref) flag is set, it's updated with the current frame
 *      if update_last is set, and VP56_FRAME_PREVIOUS otherwise.
 * If the flag is not set, the number read means:
 *      0: no update
 *      1: VP56_FRAME_PREVIOUS
 *      2: update golden with altref, or update altref with golden
 */
static VP56Frame ref_to_update(VP8Context *s, int update, VP56Frame ref)
{
  VP56RangeCoder *c = &s->c;

  if (update)
    return VP56_FRAME_CURRENT;

  switch (vp8_rac_get_uint(c, 2)) {
  case 1:
    return VP56_FRAME_PREVIOUS;
  case 2:
    return (ref == VP56_FRAME_GOLDEN) ? VP56_FRAME_GOLDEN2 : VP56_FRAME_GOLDEN;
  }
  return VP56_FRAME_NONE;
}

static void update_refs(VP8Context *s)
{
  VP56RangeCoder *c = &s->c;

  int update_golden = vp8_rac_get(c);
  int update_altref = vp8_rac_get(c);

  s->update_golden = ref_to_update(s, update_golden, VP56_FRAME_GOLDEN);
  s->update_altref = ref_to_update(s, update_altref, VP56_FRAME_GOLDEN2);
}

static int decode_frame_header(VP8Context *s, const uint8_t *buf, int buf_size)
{
  VP56RangeCoder *c = &s->c;
  int header_size, hscale, vscale, i, j, k, l, m, ret;
  int width  = s->avctx->width;
  int height = s->avctx->height;

  s->keyframe  = !(buf[0] & 1);
  s->profile   =  (buf[0]>>1) & 7;
  s->invisible = !(buf[0] & 0x10);
  header_size  = AV_RL24(buf) >> 5;
  buf      += 3;
  buf_size -= 3;

  if (s->profile > 3)
    av_log(s->avctx, AV_LOG_WARNING, "Unknown profile %d\n", s->profile);

  if (!s->profile){
    memcpy(s->put_pixels_tab, s->vp8dsp.put_vp8_epel_pixels_tab, sizeof(s->put_pixels_tab));
#ifdef JZC_VP8_OPT
    //printf("motion_init start\n");
    motion_init_vp8(VP8_QPEL,H264_EPEL);
    //printf("motion_init end\n");
    //*(volatile int *)TCSM1_VCADDR(TCSM1_MOTION_DSA0) = 0x8000FFFF;
    //printf("decode_init end\n");
#endif
  }else{    // profile 1-3 use bilinear, 4+ aren't defined so whatever
    memcpy(s->put_pixels_tab, s->vp8dsp.put_vp8_bilinear_pixels_tab, sizeof(s->put_pixels_tab));
#ifdef JZC_VP8_OPT
    //printf("motion_init start\n");
    motion_init_vp8(VP8_BILN,H264_EPEL);
    //printf("motion_init end\n");
    //*(volatile int *)TCSM1_VCADDR(TCSM1_MOTION_DSA0) = 0x8000FFFF;
    //printf("decode_init end\n");
#endif
  }

  if (header_size > buf_size - 7*s->keyframe) {
    av_log(s->avctx, AV_LOG_ERROR, "Header size larger than data provided\n");
    return AVERROR_INVALIDDATA;
  }

  if (s->keyframe) {
    if (AV_RL24(buf) != 0x2a019d) {
      av_log(s->avctx, AV_LOG_ERROR, "Invalid start code 0x%x\n", AV_RL24(buf));
      return AVERROR_INVALIDDATA;
    }
    width  = AV_RL16(buf+3) & 0x3fff;
    height = AV_RL16(buf+5) & 0x3fff;
    hscale = buf[4] >> 6;
    vscale = buf[6] >> 6;
    buf      += 7;
    buf_size -= 7;

    if (hscale || vscale)
      av_log_missing_feature(s->avctx, "Upscaling", 1);

    s->update_golden = s->update_altref = VP56_FRAME_CURRENT;
    for (i = 0; i < 4; i++)
      for (j = 0; j < 16; j++)
	memcpy(s->prob->token[i][j], vp8_token_default_probs[i][vp8_coeff_band[j]],
	       sizeof(s->prob->token[i][j]));
    memcpy(s->prob->pred16x16, vp8_pred16x16_prob_inter, sizeof(s->prob->pred16x16));
    memcpy(s->prob->pred8x8c , vp8_pred8x8c_prob_inter , sizeof(s->prob->pred8x8c));
    memcpy(s->prob->mvc      , vp8_mv_default_prob     , sizeof(s->prob->mvc));
    memset(&s->segmentation, 0, sizeof(s->segmentation));
  }

  if (!s->macroblocks_base || /* first frame */
      width != s->avctx->width || height != s->avctx->height) {
    if ((ret = update_dimensions(s, width, height) < 0))
      return ret;
  }

  ff_vp56_init_range_decoder(c, buf, header_size);
  buf      += header_size;
  buf_size -= header_size;

  if (s->keyframe) {
    if (vp8_rac_get(c))
      av_log(s->avctx, AV_LOG_WARNING, "Unspecified colorspace\n");
    vp8_rac_get(c); // whether we can skip clamping in dsp functions
  }

  if ((s->segmentation.enabled = vp8_rac_get(c)))
    parse_segment_info(s);
  else
    s->segmentation.update_map = 0; // FIXME: move this to some init function?

  s->filter.simple    = vp8_rac_get(c);
  s->filter.level     = vp8_rac_get_uint(c, 6);
  s->filter.sharpness = vp8_rac_get_uint(c, 3);

  if ((s->lf_delta.enabled = vp8_rac_get(c)))
    if (vp8_rac_get(c))
      update_lf_deltas(s);

  if (setup_partitions(s, buf, buf_size)) {
    av_log(s->avctx, AV_LOG_ERROR, "Invalid partitions\n");
    return AVERROR_INVALIDDATA;
  }

  get_quants(s);

  if (!s->keyframe) {
    update_refs(s);
    s->sign_bias[VP56_FRAME_GOLDEN]               = vp8_rac_get(c);
    s->sign_bias[VP56_FRAME_GOLDEN2 /* altref */] = vp8_rac_get(c);
  }

  // if we aren't saving this frame's probabilities for future frames,
  // make a copy of the current probabilities
  if (!(s->update_probabilities = vp8_rac_get(c)))
    s->prob[1] = s->prob[0];

  s->update_last = s->keyframe || vp8_rac_get(c);

  for (i = 0; i < 4; i++)
    for (j = 0; j < 8; j++)
      for (k = 0; k < 3; k++)
	for (l = 0; l < NUM_DCT_TOKENS-1; l++)
	  if (vp56_rac_get_prob_branchy(c, vp8_token_update_probs[i][j][k][l])) {
	    int prob = vp8_rac_get_uint(c, 8);
	    for (m = 0; vp8_coeff_band_indexes[j][m] >= 0; m++)
	      s->prob->token[i][vp8_coeff_band_indexes[j][m]][k][l] = prob;
	  }

  if ((s->mbskip_enabled = vp8_rac_get(c)))
    s->prob->mbskip = vp8_rac_get_uint(c, 8);

  if (!s->keyframe) {
    s->prob->intra  = vp8_rac_get_uint(c, 8);
    s->prob->last   = vp8_rac_get_uint(c, 8);
    s->prob->golden = vp8_rac_get_uint(c, 8);

    if (vp8_rac_get(c))
      for (i = 0; i < 4; i++)
	s->prob->pred16x16[i] = vp8_rac_get_uint(c, 8);
    if (vp8_rac_get(c))
      for (i = 0; i < 3; i++)
	s->prob->pred8x8c[i]  = vp8_rac_get_uint(c, 8);

    // 17.2 MV probability update
    for (i = 0; i < 2; i++)
      for (j = 0; j < 19; j++)
	if (vp56_rac_get_prob_branchy(c, vp8_mv_update_prob[i][j]))
	  s->prob->mvc[i][j] = vp8_rac_get_nn(c);
  }

  return 0;
}

static av_always_inline
void clamp_mv(VP8Context *s, VP56mv *dst, const VP56mv *src, int mb_x, int mb_y)
{
#define MARGIN (16 << 2)
  dst->x = av_clip(src->x, -((mb_x << 6) + MARGIN),
		   ((s->mb_width  - 1 - mb_x) << 6) + MARGIN);
  dst->y = av_clip(src->y, -((mb_y << 6) + MARGIN),
		   ((s->mb_height - 1 - mb_y) << 6) + MARGIN);
}

static av_always_inline
void find_near_mvs(VP8Context *s, VP8Macroblock *mb,
                   VP56mv near[2], VP56mv *best, uint8_t cnt[4])
{
  VP8Macroblock *mb_edge[3] = { mb + 2 /* top */,
				mb - 1 /* left */,
				mb + 1 /* top-left */ };
  enum { EDGE_TOP, EDGE_LEFT, EDGE_TOPLEFT };
  VP56mv near_mv[4]  = {{ 0 }};
  enum { CNT_ZERO, CNT_NEAREST, CNT_NEAR, CNT_SPLITMV };
  int idx = CNT_ZERO;
  int best_idx = CNT_ZERO;
  int cur_sign_bias = s->sign_bias[mb->ref_frame];
  int *sign_bias = s->sign_bias;

  /* Process MB on top, left and top-left */
#define MV_EDGE_CHECK(n)\
    {\
        VP8Macroblock *edge = mb_edge[n];\
        int edge_ref = edge->ref_frame;\
        if (edge_ref != VP56_FRAME_CURRENT) {\
            uint32_t mv = AV_RN32A(&edge->mv);\
            if (mv) {\
                if (cur_sign_bias != sign_bias[edge_ref]) {\
                    /* SWAR negate of the values in mv. */\
                    mv = ~mv;\
                    mv = ((mv&0x7fff7fff) + 0x00010001) ^ (mv&0x80008000);\
                }\
                if (!n || mv != AV_RN32A(&near_mv[idx]))\
                    AV_WN32A(&near_mv[++idx], mv);\
                cnt[idx]      += 1 + (n != 2);\
            } else\
                cnt[CNT_ZERO] += 1 + (n != 2);\
        }\
    }
  MV_EDGE_CHECK(0)
    MV_EDGE_CHECK(1)
    MV_EDGE_CHECK(2)

    /* If we have three distinct MVs, merge first and last if they're the same */
    if (cnt[CNT_SPLITMV] && AV_RN32A(&near_mv[1+EDGE_TOP]) == AV_RN32A(&near_mv[1+EDGE_TOPLEFT]))
      cnt[CNT_NEAREST] += 1;

  cnt[CNT_SPLITMV] = ((mb_edge[EDGE_LEFT]->mode   == VP8_MVMODE_SPLIT) +
		      (mb_edge[EDGE_TOP]->mode    == VP8_MVMODE_SPLIT)) * 2 +
    (mb_edge[EDGE_TOPLEFT]->mode == VP8_MVMODE_SPLIT);

  /* Swap near and nearest if necessary */
  if (cnt[CNT_NEAR] > cnt[CNT_NEAREST]) {
    FFSWAP(uint8_t,     cnt[CNT_NEAREST],     cnt[CNT_NEAR]);
    FFSWAP( VP56mv, near_mv[CNT_NEAREST], near_mv[CNT_NEAR]);
  }

  /* Choose the best mv out of 0,0 and the nearest mv */
  if (cnt[CNT_NEAREST] >= cnt[CNT_ZERO])
    best_idx = CNT_NEAREST;

  mb->mv  = near_mv[best_idx];
  near[0] = near_mv[CNT_NEAREST];
  near[1] = near_mv[CNT_NEAR];
}

/**
 * Motion vector coding, 17.1.
 */
static int read_mv_component(VP56RangeCoder *c, const uint8_t *p)
{
  int bit, x = 0;

  if (vp56_rac_get_prob_branchy(c, p[0])) {
    int i;

    for (i = 0; i < 3; i++)
      x += vp56_rac_get_prob(c, p[9 + i]) << i;
    for (i = 9; i > 3; i--)
      x += vp56_rac_get_prob(c, p[9 + i]) << i;
    if (!(x & 0xFFF0) || vp56_rac_get_prob(c, p[12]))
      x += 8;
  } else {
    // small_mvtree
    const uint8_t *ps = p+2;
    bit = vp56_rac_get_prob(c, *ps);
    ps += 1 + 3*bit;
    x  += 4*bit;
    bit = vp56_rac_get_prob(c, *ps);
    ps += 1 + bit;
    x  += 2*bit;
    x  += vp56_rac_get_prob(c, *ps);
  }

  return (x && vp56_rac_get_prob(c, p[1])) ? -x : x;
}

static av_always_inline
const uint8_t *get_submv_prob(uint32_t left, uint32_t top)
{
  if (left == top)
    return vp8_submv_prob[4-!!left];
  if (!top)
    return vp8_submv_prob[2];
  return vp8_submv_prob[1-!!left];
}

/**
 * Split motion vector prediction, 16.4.
 * @returns the number of motion vectors parsed (2, 4 or 16)
 */
static av_always_inline
int decode_splitmvs(VP8Context *s, VP56RangeCoder *c, VP8Macroblock *mb)
{
  int part_idx;
  int n, num;
  VP8Macroblock *top_mb  = &mb[2];
  VP8Macroblock *left_mb = &mb[-1];
  const uint8_t *mbsplits_left = vp8_mbsplits[left_mb->partitioning],
    *mbsplits_top = vp8_mbsplits[top_mb->partitioning],
    *mbsplits_cur, *firstidx;
  VP56mv *top_mv  = top_mb->bmv;
  VP56mv *left_mv = left_mb->bmv;
  VP56mv *cur_mv  = mb->bmv;

  if (vp56_rac_get_prob_branchy(c, vp8_mbsplit_prob[0])) {
    if (vp56_rac_get_prob_branchy(c, vp8_mbsplit_prob[1])) {
      part_idx = VP8_SPLITMVMODE_16x8 + vp56_rac_get_prob(c, vp8_mbsplit_prob[2]);
    } else {
      part_idx = VP8_SPLITMVMODE_8x8;
    }
  } else {
    part_idx = VP8_SPLITMVMODE_4x4;
  }

  num = vp8_mbsplit_count[part_idx];
  mbsplits_cur = vp8_mbsplits[part_idx],
    firstidx = vp8_mbfirstidx[part_idx];
  mb->partitioning = part_idx;

  for (n = 0; n < num; n++) {
    int k = firstidx[n];
    uint32_t left, above;
    const uint8_t *submv_prob;

    if (!(k & 3))
      left = AV_RN32A(&left_mv[mbsplits_left[k + 3]]);
    else
      left  = AV_RN32A(&cur_mv[mbsplits_cur[k - 1]]);
    if (k <= 3)
      above = AV_RN32A(&top_mv[mbsplits_top[k + 12]]);
    else
      above = AV_RN32A(&cur_mv[mbsplits_cur[k - 4]]);

    submv_prob = get_submv_prob(left, above);

    if (vp56_rac_get_prob_branchy(c, submv_prob[0])) {
      if (vp56_rac_get_prob_branchy(c, submv_prob[1])) {
	if (vp56_rac_get_prob_branchy(c, submv_prob[2])) {
	  mb->bmv[n].y = mb->mv.y + read_mv_component(c, s->prob->mvc[0]);
	  mb->bmv[n].x = mb->mv.x + read_mv_component(c, s->prob->mvc[1]);
	} else {
	  AV_ZERO32(&mb->bmv[n]);
	}
      } else {
	AV_WN32A(&mb->bmv[n], above);
      }
    } else {
      AV_WN32A(&mb->bmv[n], left);
    }
  }

  return num;
}

static av_always_inline
void decode_intra4x4_modes(VP8Context *s, VP56RangeCoder *c,
                           int mb_x, int keyframe)
{
  uint8_t *intra4x4 = s->intra4x4_pred_mode_mb;
  if (keyframe) {
    int x, y;
    uint8_t* const top = s->intra4x4_pred_mode_top + 4 * mb_x;
    uint8_t* const left = s->intra4x4_pred_mode_left;
    for (y = 0; y < 4; y++) {
      for (x = 0; x < 4; x++) {
	const uint8_t *ctx;
	ctx = vp8_pred4x4_prob_intra[top[x]][left[y]];
	*intra4x4 = vp8_rac_get_tree(c, vp8_pred4x4_tree, ctx);
	left[y] = top[x] = *intra4x4;
	intra4x4++;
      }
    }
  } else {
    int i;
    for (i = 0; i < 16; i++)
      intra4x4[i] = vp8_rac_get_tree(c, vp8_pred4x4_tree, vp8_pred4x4_prob_inter);
  }
}

static av_always_inline
void decode_mb_mode(VP8Context *s, VP8Macroblock *mb, int mb_x, int mb_y, uint8_t *segment)
{
  VP56RangeCoder *c = &s->c;

  if (s->segmentation.update_map)
    *segment = vp8_rac_get_tree(c, vp8_segmentid_tree, s->prob->segmentid);
  s->segment = *segment;

  mb->skip = s->mbskip_enabled ? vp56_rac_get_prob(c, s->prob->mbskip) : 0;

  if (s->keyframe) {
    mb->mode = vp8_rac_get_tree(c, vp8_pred16x16_tree_intra, vp8_pred16x16_prob_intra);

    if (mb->mode == MODE_I4x4) {
      decode_intra4x4_modes(s, c, mb_x, 1);
    } else {
      const uint32_t modes = vp8_pred4x4_mode[mb->mode] * 0x01010101u;
      AV_WN32A(s->intra4x4_pred_mode_top + 4 * mb_x, modes);
      AV_WN32A(s->intra4x4_pred_mode_left, modes);
    }

    s->chroma_pred_mode = vp8_rac_get_tree(c, vp8_pred8x8c_tree, vp8_pred8x8c_prob_intra);
    mb->ref_frame = VP56_FRAME_CURRENT;
  } else if (vp56_rac_get_prob_branchy(c, s->prob->intra)) {
    VP56mv near[2], best;
    uint8_t cnt[4] = { 0 };

    // inter MB, 16.2
    if (vp56_rac_get_prob_branchy(c, s->prob->last))
      mb->ref_frame = vp56_rac_get_prob(c, s->prob->golden) ?
	VP56_FRAME_GOLDEN2 /* altref */ : VP56_FRAME_GOLDEN;
    else
      mb->ref_frame = VP56_FRAME_PREVIOUS;
    s->ref_count[mb->ref_frame-1]++;

    // motion vectors, 16.3
    find_near_mvs(s, mb, near, &best, cnt);
    if (vp56_rac_get_prob_branchy(c, vp8_mode_contexts[cnt[0]][0])) {
      if (vp56_rac_get_prob_branchy(c, vp8_mode_contexts[cnt[1]][1])) {
	if (vp56_rac_get_prob_branchy(c, vp8_mode_contexts[cnt[2]][2])) {
	  if (vp56_rac_get_prob_branchy(c, vp8_mode_contexts[cnt[3]][3])) {
	    mb->mode = VP8_MVMODE_SPLIT;
	    clamp_mv(s, &mb->mv, &mb->mv, mb_x, mb_y);
	    mb->mv = mb->bmv[decode_splitmvs(s, c, mb) - 1];
	  } else {
	    mb->mode = VP8_MVMODE_NEW;
	    clamp_mv(s, &mb->mv, &mb->mv, mb_x, mb_y);
	    mb->mv.y += read_mv_component(c, s->prob->mvc[0]);
	    mb->mv.x += read_mv_component(c, s->prob->mvc[1]);
	  }
	} else {
	  mb->mode = VP8_MVMODE_NEAR;
	  clamp_mv(s, &mb->mv, &near[1], mb_x, mb_y);
	}
      } else {
	mb->mode = VP8_MVMODE_NEAREST;
	clamp_mv(s, &mb->mv, &near[0], mb_x, mb_y);
      }
    } else {
      mb->mode = VP8_MVMODE_ZERO;
      AV_ZERO32(&mb->mv);
    }
    if (mb->mode != VP8_MVMODE_SPLIT) {
      mb->partitioning = VP8_SPLITMVMODE_NONE;
      mb->bmv[0] = mb->mv;
    }
  } else {
    // intra MB, 16.1
    mb->mode = vp8_rac_get_tree(c, vp8_pred16x16_tree_inter, s->prob->pred16x16);

    if (mb->mode == MODE_I4x4)
      decode_intra4x4_modes(s, c, mb_x, 0);

    s->chroma_pred_mode = vp8_rac_get_tree(c, vp8_pred8x8c_tree, s->prob->pred8x8c);
    mb->ref_frame = VP56_FRAME_CURRENT;
    mb->partitioning = VP8_SPLITMVMODE_NONE;
    AV_ZERO32(&mb->bmv[0]);
  }
}

/**
 * @param c arithmetic bitstream reader context
 * @param block destination for block coefficients
 * @param probs probabilities to use when reading trees from the bitstream
 * @param i initial coeff index, 0 unless a separate DC block is coded
 * @param zero_nhood the initial prediction context for number of surrounding
 *                   all-zero blocks (only left/top, so 0-2)
 * @param qmul array holding the dc/ac dequant factor at position 0/1
 * @return 0 if no coeffs were decoded
 *         otherwise, the index of the last coeff decoded plus one
 */
static int decode_block_coeffs_internal(VP56RangeCoder *c, DCTELEM block[16],
                                        uint8_t probs[8][3][NUM_DCT_TOKENS-1],
                                        int i, uint8_t *token_prob, int16_t qmul[2])
{
  goto skip_eob;
  do {
    int coeff;
    if (!vp56_rac_get_prob_branchy(c, token_prob[0]))   // DCT_EOB
      return i;

  skip_eob:
    if (!vp56_rac_get_prob_branchy(c, token_prob[1])) { // DCT_0
      if (++i == 16)
	return i; // invalid input; blocks should end with EOB
      token_prob = probs[i][0];
      goto skip_eob;
    }

    if (!vp56_rac_get_prob_branchy(c, token_prob[2])) { // DCT_1
      coeff = 1;
      token_prob = probs[i+1][1];
    } else {
      if (!vp56_rac_get_prob_branchy(c, token_prob[3])) { // DCT 2,3,4
	coeff = vp56_rac_get_prob_branchy(c, token_prob[4]);
	if (coeff)
	  coeff += vp56_rac_get_prob(c, token_prob[5]);
	coeff += 2;
      } else {
	// DCT_CAT*
	if (!vp56_rac_get_prob_branchy(c, token_prob[6])) {
	  if (!vp56_rac_get_prob_branchy(c, token_prob[7])) { // DCT_CAT1
	    coeff  = 5 + vp56_rac_get_prob(c, vp8_dct_cat1_prob[0]);
	  } else {                                    // DCT_CAT2
	    coeff  = 7;
	    coeff += vp56_rac_get_prob(c, vp8_dct_cat2_prob[0]) << 1;
	    coeff += vp56_rac_get_prob(c, vp8_dct_cat2_prob[1]);
	  }
	} else {    // DCT_CAT3 and up
	  int a = vp56_rac_get_prob(c, token_prob[8]);
	  int b = vp56_rac_get_prob(c, token_prob[9+a]);
	  int cat = (a<<1) + b;
	  coeff  = 3 + (8<<cat);
	  coeff += vp8_rac_get_coeff(c, vp8_dct_cat_prob[cat]);
	}
      }
      token_prob = probs[i+1][2];
    }
    //block[zigzag_scan[i]] = (vp8_rac_get(c) ? -coeff : coeff) * qmul[!!i];
    block[zigzag_scan[i]] = (vp8_rac_get(c) ? -coeff : coeff);
  } while (++i < 16);

  return i;
}

static av_always_inline
int decode_block_coeffs(VP56RangeCoder *c, DCTELEM block[16],
                        uint8_t probs[8][3][NUM_DCT_TOKENS-1],
                        int i, int zero_nhood, int16_t qmul[2])
{
  uint8_t *token_prob = probs[i][zero_nhood];
  if (!vp56_rac_get_prob_branchy(c, token_prob[0]))   // DCT_EOB
    return 0;
  return decode_block_coeffs_internal(c, block, probs, i, token_prob, qmul);
}

static av_always_inline
void decode_mb_coeffs(VP8Context *s, VP56RangeCoder *c, VP8Macroblock *mb,
                      uint8_t t_nnz[9], uint8_t l_nnz[9])
{
  int i, x, y, luma_start = 0, luma_ctx = 3;
  int nnz_pred, nnz, nnz_total = 0;
  int segment = s->segment;
  int block_dc = 0;

  if (mb->mode != MODE_I4x4 && mb->mode != VP8_MVMODE_SPLIT) {
    nnz_pred = t_nnz[8] + l_nnz[8];

    // decode DC values and do hadamard
    nnz = decode_block_coeffs(c, block0->block_dc, s->prob->token[1], 0, nnz_pred,
			      s->qmat[segment].luma_dc_qmul);
    l_nnz[8] = t_nnz[8] = !!nnz;
    if (nnz) {
      nnz_total += nnz;
      block_dc = 1;
#if 0
      if (nnz == 1)
	s->vp8dsp.vp8_luma_dc_wht_dc(s->block, s->block_dc);
      else
	s->vp8dsp.vp8_luma_dc_wht(s->block, s->block_dc);
#endif
#ifdef JZC_VP8_OPT
      mau_reg_ptr1->main_cbp |= (0x1<<MAU_Y_DC_DCT_SFT);
      mau_reg_ptr1->id_mlen = 800;
#endif

    }
    luma_start = 1;
    luma_ctx = 0;
  }

  // luma blocks
  for (y = 0; y < 4; y++)
    for (x = 0; x < 4; x++) {
      nnz_pred = l_nnz[y] + t_nnz[x];
      nnz = decode_block_coeffs(c, block0->block[y][x], s->prob->token[luma_ctx], luma_start,
				nnz_pred, s->qmat[segment].luma_qmul);
      // nnz+block_dc may be one more than the actual last index, but we don't care
      s->non_zero_count_cache[y][x] = nnz + block_dc;
      t_nnz[x] = l_nnz[y] = !!nnz;
      nnz_total += nnz;
    }

  // chroma blocks
  // TODO: what to do about dimensions? 2nd dim for luma is x,
  // but for chroma it's (y<<1)|x
  for (i = 4; i < 6; i++)
    for (y = 0; y < 2; y++)
      for (x = 0; x < 2; x++) {
	nnz_pred = l_nnz[i+2*y] + t_nnz[i+2*x];
	nnz = decode_block_coeffs(c, block0->block[i][(y<<1)+x], s->prob->token[2], 0,
				  nnz_pred, s->qmat[segment].chroma_qmul);
	s->non_zero_count_cache[i][(y<<1)+x] = nnz;
	t_nnz[i+2*x] = l_nnz[i+2*y] = !!nnz;
	nnz_total += nnz;
      }

  // if there were no coded coeffs despite the macroblock not being marked skip,
  // we MUST not do the inner loop filter and should not do IDCT
  // Since skip isn't used for bitstream prediction, just manually set it.
  if (!nnz_total){
    //printf("mb_skip\n");
    mb->skip = 1;
  }
}

static av_always_inline
void backup_mb_border(uint8_t *top_border, uint8_t *src_y, uint8_t *src_cb, uint8_t *src_cr,
                      int linesize, int uvlinesize, int simple)
{
  AV_COPY128(top_border, src_y + 15*linesize);
  if (!simple) {
    AV_COPY64(top_border+16, src_cb + 7*uvlinesize);
    AV_COPY64(top_border+24, src_cr + 7*uvlinesize);
  }
}

static av_always_inline
void xchg_mb_border(uint8_t *top_border, uint8_t *src_y, uint8_t *src_cb, uint8_t *src_cr,
                    int linesize, int uvlinesize, int mb_x, int mb_y, int mb_width,
                    int simple, int xchg)
{
  uint8_t *top_border_m1 = top_border-32;     // for TL prediction
  src_y  -=   linesize;
  src_cb -= uvlinesize;
  src_cr -= uvlinesize;

#define XCHG(a,b,xchg) do {                     \
        if (xchg) AV_SWAP64(b,a);               \
        else      AV_COPY64(b,a);               \
    } while (0)

  XCHG(top_border_m1+8, src_y-8, xchg);
  XCHG(top_border,      src_y,   xchg);
  XCHG(top_border+8,    src_y+8, 1);
  if (mb_x < mb_width-1)
    XCHG(top_border+32, src_y+16, 1);

  // only copy chroma for normal loop filter
  // or to initialize the top row to 127
  if (!simple || !mb_y) {
    XCHG(top_border_m1+16, src_cb-8, xchg);
    XCHG(top_border_m1+24, src_cr-8, xchg);
    XCHG(top_border+16,    src_cb, 1);
    XCHG(top_border+24,    src_cr, 1);
  }
}

static av_always_inline
int check_intra_pred_mode(int mode, int mb_x, int mb_y)
{
  if (mode == DC_PRED8x8) {
    if (!mb_x) {
      mode = mb_y ? TOP_DC_PRED8x8 : DC_128_PRED8x8;
    } else if (!mb_y) {
      mode = LEFT_DC_PRED8x8;
    }
  }
  return mode;
}

static av_always_inline
void intra_predict(VP8Context *s, uint8_t *dst[3], VP8Macroblock *mb,
                   int mb_x, int mb_y)
{
  int x, y, mode, nnz, tr;
  int top_lr_avalid=0;

  // for the first row, we need to run xchg_mb_border to init the top edge to 127
  // otherwise, skip it if we aren't going to deblock
#if 0
  if (s->deblock_filter || !mb_y)
    xchg_mb_border(s->top_border[mb_x+1], dst[0], dst[1], dst[2],
		   s->linesize, s->uvlinesize, mb_x, mb_y, s->mb_width,
		   s->filter.simple, 1);
#endif

  if (mb->mode < MODE_I4x4) {
    mode = check_intra_pred_mode(mb->mode, mb_x, mb_y);
    //s->hpc.pred16x16[mode](dst[0], s->linesize);
#ifdef JZC_VP8_OPT
    mau_reg_ptr1->main_cbp |= (MAU_C_ERR_MSK<<MAU_C_ERR_SFT)|(MAU_Y_PREDE_MSK<<MAU_Y_PREDE_SFT)|(MAU_Y_ERR_MSK<<MAU_Y_ERR_SFT)|(IMB<<MAU_MTX_SFT)|0xFFFFFF;
    mau_reg_ptr1->id_mlen |= 0x1<<16;

    if (mode > 3){
      mau_reg_ptr1->y_pred_mode[0] = 0;
      mau_reg_ptr1->y_pred_mode[1] = 0;
    }else{
      mau_reg_ptr1->y_pred_mode[0] = vp8_16x16_pred_lookup[mode];
      mau_reg_ptr1->y_pred_mode[1] = 0;
    }
    //if (vpFrame == 27)
    //printf("mode is %d\n", mode);
    top_lr_avalid = 0x3;
    if (mb_x == 0 && mb_y == 0)
      top_lr_avalid = 0x0;
    else if (mb_x == 0)
      top_lr_avalid = 0x1;
    else if (mb_y == 0)
      top_lr_avalid = 0x2;
    //printf("top_lr_avalid is %d\n", top_lr_avalid);
#endif
  } else {
    uint8_t *ptr = dst[0];
    uint8_t *intra4x4 = s->intra4x4_pred_mode_mb;

    // all blocks on the right edge of the macroblock use bottom edge
    // the top macroblock for their topright edge
    uint8_t *tr_right = ptr - s->linesize + 16;

    // if we're on the right edge of the frame, said edge is extended
    // from the top macroblock
    if (mb_x == s->mb_width-1) {
      tr = tr_right[-1]*0x01010101;
      tr_right = (uint8_t *)&tr;
    }

    if (mb->skip)
      AV_ZERO128(s->non_zero_count_cache);

#ifdef JZC_VP8_OPT
    mau_reg_ptr1->y_pred_mode[0] = 0; 
    mau_reg_ptr1->y_pred_mode[1] = 0; 
#endif
    for (y = 0; y < 4; y++) {
      uint8_t *topright = ptr + 4 - s->linesize;
      for (x = 0; x < 4; x++) {
	if (x == 3)
	  topright = tr_right;

#ifdef JZC_VP8_OPT
	if (y < 2){
	  mau_reg_ptr1->y_pred_mode[0] |= (vp8_4x4_pred_lookup[intra4x4[x]])<<(y*16+x*4);
	}else{
	  mau_reg_ptr1->y_pred_mode[1] |= (vp8_4x4_pred_lookup[intra4x4[x]])<<((y-2)*16+x*4);
	}
#endif
	//s->hpc.pred4x4[intra4x4[x]](ptr+4*x, topright, s->linesize);

	nnz = s->non_zero_count_cache[y][x];
	if (nnz) {
	  //if (nnz == 1)
	  //s->vp8dsp.vp8_idct_dc_add(ptr+4*x, s->block[y][x], s->linesize);
	  //else
	  //s->vp8dsp.vp8_idct_add(ptr+4*x, s->block[y][x], s->linesize);
	}
	topright += 4;
      }

      ptr   += 4*s->linesize;
      intra4x4 += 4;
    }
#ifdef JZC_VP8_OPT
    mau_reg_ptr1->main_cbp |= (MAU_C_ERR_MSK<<MAU_C_ERR_SFT)|(MAU_Y_PREDE_MSK<<MAU_Y_PREDE_SFT)|(MAU_Y_ERR_MSK<<MAU_Y_ERR_SFT)|(BMB<<MAU_MTX_SFT)|0xFFFFFF;
    mau_reg_ptr1->id_mlen |= 0x1<<16;

    top_lr_avalid = 0x3;
    if (mb_x == 0 && mb_y == 0)
      top_lr_avalid = 0x0;
    else if (mb_x == 0)
      top_lr_avalid = 0x1;
    else if (mb_y == 0)
      top_lr_avalid = 0x2;
#endif
  }

  mode = check_intra_pred_mode(s->chroma_pred_mode, mb_x, mb_y);
#ifdef JZC_VP8_OPT
  if (mode > 3)
    mau_reg_ptr1->tlr_cpmd = (top_lr_avalid&0xF)<<16;
  else if (mode == 3)
    mau_reg_ptr1->tlr_cpmd = ((top_lr_avalid&0xF)<<16)| 0xF;
  else
    mau_reg_ptr1->tlr_cpmd = ((top_lr_avalid&0xF)<<16)| mode;
#endif
  //s->hpc.pred8x8[mode](dst[1], s->uvlinesize);
  //s->hpc.pred8x8[mode](dst[2], s->uvlinesize);
#if 0
  if (s->deblock_filter || !mb_y)
    xchg_mb_border(s->top_border[mb_x+1], dst[0], dst[1], dst[2],
		   s->linesize, s->uvlinesize, mb_x, mb_y, s->mb_width,
		   s->filter.simple, 0);
#endif
}

/**
 * Generic MC function.
 *
 * @param s VP8 decoding context
 * @param luma 1 for luma (Y) planes, 0 for chroma (Cb/Cr) planes
 * @param dst target buffer for block data at block position
 * @param src reference picture buffer at origin (0, 0)
 * @param mv motion vector (relative to block position) to get pixel data from
 * @param x_off horizontal position of block from origin (0, 0)
 * @param y_off vertical position of block from origin (0, 0)
 * @param block_w width of block (16, 8 or 4)
 * @param block_h height of block (always same as block_w)
 * @param width width of src/dst plane data
 * @param height height of src/dst plane data
 * @param linesize size of a single line of plane data, including padding
 * @param mc_func motion compensation function pointers (bilinear or sixtap MC)
 */
static av_always_inline
void vp8_mc(VP8Context *s, int luma,
            uint8_t *dst, uint8_t *src, const VP56mv *mv,
            int x_off, int y_off, int block_w, int block_h,
            int width, int height, int linesize,
            vp8_mc_func mc_func[3][3])
{
  if (AV_RN32A(mv)) {
    static const uint8_t idx[8] = { 0, 1, 2, 1, 2, 1, 2, 1 };
    int mx = (mv->x << luma)&7, mx_idx = idx[mx];
    int my = (mv->y << luma)&7, my_idx = idx[my];

    x_off += mv->x >> (3 - luma);
    y_off += mv->y >> (3 - luma);

    // edge emulation
    src += y_off * linesize + x_off;
    if (x_off < 2 || x_off >= width  - block_w - 3 ||
	y_off < 2 || y_off >= height - block_h - 3) {
      ff_emulated_edge_mc(s->edge_emu_buffer, src - 2 * linesize - 2, linesize,
			  block_w + 5, block_h + 5,
			  x_off - 2, y_off - 2, width, height);
      src = s->edge_emu_buffer + 2 + linesize * 2;
    }
    mc_func[my_idx][mx_idx](dst, linesize, src, linesize, block_h, mx, my);
  } else
    mc_func[0][0](dst, linesize, src + y_off * linesize + x_off, linesize, block_h, 0, 0);
}

static av_always_inline
void vp8_mc_part(VP8Context *s, uint8_t *dst[3],
                 AVFrame *ref_frame, int x_off, int y_off,
                 int bx_off, int by_off,
                 int block_w, int block_h,
                 int width, int height, VP56mv *mv)
{
  VP56mv uvmv = *mv;

  /* Y */
  vp8_mc(s, 1, dst[0] + by_off * s->linesize + bx_off,
	 ref_frame->data[0], mv, x_off + bx_off, y_off + by_off,
	 block_w, block_h, width, height, s->linesize,
	 s->put_pixels_tab[block_w == 8]);

  /* U/V */
  if (s->profile == 3) {
    uvmv.x &= ~7;
    uvmv.y &= ~7;
  }
  x_off   >>= 1; y_off   >>= 1;
  bx_off  >>= 1; by_off  >>= 1;
  width   >>= 1; height  >>= 1;
  block_w >>= 1; block_h >>= 1;
  vp8_mc(s, 0, dst[1] + by_off * s->uvlinesize + bx_off,
	 ref_frame->data[1], &uvmv, x_off + bx_off, y_off + by_off,
	 block_w, block_h, width, height, s->uvlinesize,
	 s->put_pixels_tab[1 + (block_w == 4)]);
  vp8_mc(s, 0, dst[2] + by_off * s->uvlinesize + bx_off,
	 ref_frame->data[2], &uvmv, x_off + bx_off, y_off + by_off,
	 block_w, block_h, width, height, s->uvlinesize,
	 s->put_pixels_tab[1 + (block_w == 4)]);
}

/* Fetch pixels for estimated mv 4 macroblocks ahead.
 * Optimized for 64-byte cache lines.  Inspired by ffh264 prefetch_motion. */
static av_always_inline void prefetch_motion(VP8Context *s, VP8Macroblock *mb, int mb_x, int mb_y, int mb_xy, int ref)
{
  /* Don't prefetch refs that haven't been used very often this frame. */
  if (s->ref_count[ref-1] > (mb_xy >> 5)) {
    int x_off = mb_x << 4, y_off = mb_y << 4;
    int mx = (mb->mv.x>>2) + x_off + 8;
    int my = (mb->mv.y>>2) + y_off;
    uint8_t **src= s->framep[ref]->data;
    int off= mx + (my + (mb_x&3)*4)*s->linesize + 64;
    s->dsp.prefetch(src[0]+off, s->linesize, 4);
    off= (mx>>1) + ((my>>1) + (mb_x&7))*s->uvlinesize + 64;
    s->dsp.prefetch(src[1]+off, src[2]-src[1], 2);
  }
}

/**
 * Apply motion vectors to prediction buffer, chapter 18.
 */
static av_always_inline
void inter_predict(VP8Context *s, uint8_t *dst[3], VP8Macroblock *mb,
                   int mb_x, int mb_y)
{
  int x_off = mb_x << 4, y_off = mb_y << 4;
  int width = 16*s->mb_width, height = 16*s->mb_height;
  AVFrame *ref = s->framep[mb->ref_frame];
  VP56mv *bmv = mb->bmv;

  if (mb->mode < VP8_MVMODE_SPLIT) {
    vp8_mc_part(s, dst, ref, x_off, y_off,
		0, 0, 16, 16, width, height, &mb->mv);
  } else switch (mb->partitioning) {
  case VP8_SPLITMVMODE_4x4: {
    int x, y;
    VP56mv uvmv;

    /* Y */
    for (y = 0; y < 4; y++) {
      for (x = 0; x < 4; x++) {
	vp8_mc(s, 1, dst[0] + 4*y*s->linesize + x*4,
	       ref->data[0], &bmv[4*y + x],
	       4*x + x_off, 4*y + y_off, 4, 4,
	       width, height, s->linesize,
	       s->put_pixels_tab[2]);
      }
    }

    /* U/V */
    x_off >>= 1; y_off >>= 1; width >>= 1; height >>= 1;
    for (y = 0; y < 2; y++) {
      for (x = 0; x < 2; x++) {
	uvmv.x = mb->bmv[ 2*y    * 4 + 2*x  ].x +
	  mb->bmv[ 2*y    * 4 + 2*x+1].x +
	  mb->bmv[(2*y+1) * 4 + 2*x  ].x +
	  mb->bmv[(2*y+1) * 4 + 2*x+1].x;
	uvmv.y = mb->bmv[ 2*y    * 4 + 2*x  ].y +
	  mb->bmv[ 2*y    * 4 + 2*x+1].y +
	  mb->bmv[(2*y+1) * 4 + 2*x  ].y +
	  mb->bmv[(2*y+1) * 4 + 2*x+1].y;
	uvmv.x = (uvmv.x + 2 + (uvmv.x >> (INT_BIT-1))) >> 2;
	uvmv.y = (uvmv.y + 2 + (uvmv.y >> (INT_BIT-1))) >> 2;
	if (s->profile == 3) {
	  uvmv.x &= ~7;
	  uvmv.y &= ~7;
	}
	vp8_mc(s, 0, dst[1] + 4*y*s->uvlinesize + x*4,
	       ref->data[1], &uvmv,
	       4*x + x_off, 4*y + y_off, 4, 4,
	       width, height, s->uvlinesize,
	       s->put_pixels_tab[2]);
	vp8_mc(s, 0, dst[2] + 4*y*s->uvlinesize + x*4,
	       ref->data[2], &uvmv,
	       4*x + x_off, 4*y + y_off, 4, 4,
	       width, height, s->uvlinesize,
	       s->put_pixels_tab[2]);
      }
    }
    break;
  }
  case VP8_SPLITMVMODE_16x8:
    vp8_mc_part(s, dst, ref, x_off, y_off,
		0, 0, 16, 8, width, height, &bmv[0]);
    vp8_mc_part(s, dst, ref, x_off, y_off,
		0, 8, 16, 8, width, height, &bmv[1]);
    break;
  case VP8_SPLITMVMODE_8x16:
    vp8_mc_part(s, dst, ref, x_off, y_off,
		0, 0, 8, 16, width, height, &bmv[0]);
    vp8_mc_part(s, dst, ref, x_off, y_off,
		8, 0, 8, 16, width, height, &bmv[1]);
    break;
  case VP8_SPLITMVMODE_8x8:
    vp8_mc_part(s, dst, ref, x_off, y_off,
		0, 0, 8, 8, width, height, &bmv[0]);
    vp8_mc_part(s, dst, ref, x_off, y_off,
		8, 0, 8, 8, width, height, &bmv[1]);
    vp8_mc_part(s, dst, ref, x_off, y_off,
		0, 8, 8, 8, width, height, &bmv[2]);
    vp8_mc_part(s, dst, ref, x_off, y_off,
		8, 8, 8, 8, width, height, &bmv[3]);
    break;
  }
}

static av_always_inline void idct_mb(VP8Context *s, uint8_t *dst[3], VP8Macroblock *mb)
{
  int x, y, ch;

  if (mb->mode != MODE_I4x4) {
    uint8_t *y_dst = dst[0];
    for (y = 0; y < 4; y++) {
      uint32_t nnz4 = AV_RN32A(s->non_zero_count_cache[y]);
      if (nnz4) {
	if (nnz4&~0x01010101) {
	  for (x = 0; x < 4; x++) {
	    int nnz = s->non_zero_count_cache[y][x];
	    if (nnz) {
	      if (nnz == 1)
		s->vp8dsp.vp8_idct_dc_add(y_dst+4*x, s->block[y][x], s->linesize);
	      else
		s->vp8dsp.vp8_idct_add(y_dst+4*x, s->block[y][x], s->linesize);
	    }
	  }
	} else {
	  s->vp8dsp.vp8_idct_dc_add4y(y_dst, s->block[y], s->linesize);
	}
      }
      y_dst += 4*s->linesize;
    }
  }

  for (ch = 0; ch < 2; ch++) {
    uint32_t nnz4 = AV_RN32A(s->non_zero_count_cache[4+ch]);
    if (nnz4) {
      uint8_t *ch_dst = dst[1+ch];
      if (nnz4&~0x01010101) {
	for (y = 0; y < 2; y++) {
	  for (x = 0; x < 2; x++) {
	    int nnz = s->non_zero_count_cache[4+ch][(y<<1)+x];
	    if (nnz) {
	      if (nnz == 1)
		s->vp8dsp.vp8_idct_dc_add(ch_dst+4*x, s->block[4+ch][(y<<1)+x], s->uvlinesize);
	      else
		s->vp8dsp.vp8_idct_add(ch_dst+4*x, s->block[4+ch][(y<<1)+x], s->uvlinesize);
	    }
	  }
	  ch_dst += 4*s->uvlinesize;
	}
      } else {
	s->vp8dsp.vp8_idct_dc_add4uv(ch_dst, s->block[4+ch], s->uvlinesize);
      }
    }
  }
}

static av_always_inline void filter_level_for_mb(VP8Context *s, VP8Macroblock *mb, VP8FilterStrength *f )
{
  int interior_limit, filter_level;

  if (s->segmentation.enabled) {
    filter_level = s->segmentation.filter_level[s->segment];
    if (!s->segmentation.absolute_vals)
      filter_level += s->filter.level;
  } else
    filter_level = s->filter.level;

  if (s->lf_delta.enabled) {
    filter_level += s->lf_delta.ref[mb->ref_frame];

    if (mb->ref_frame == VP56_FRAME_CURRENT) {
      if (mb->mode == MODE_I4x4)
	filter_level += s->lf_delta.mode[0];
    } else {
      if (mb->mode == VP8_MVMODE_ZERO)
	filter_level += s->lf_delta.mode[1];
      else if (mb->mode == VP8_MVMODE_SPLIT)
	filter_level += s->lf_delta.mode[3];
      else
	filter_level += s->lf_delta.mode[2];
    }
  }
  filter_level = av_clip(filter_level, 0, 63);

  interior_limit = filter_level;
  if (s->filter.sharpness) {
    interior_limit >>= s->filter.sharpness > 4 ? 2 : 1;
    interior_limit = FFMIN(interior_limit, 9 - s->filter.sharpness);
  }
  interior_limit = FFMAX(interior_limit, 1);

  f->filter_level = filter_level;
  f->inner_limit = interior_limit;
  f->inner_filter = !mb->skip || mb->mode == MODE_I4x4 || mb->mode == VP8_MVMODE_SPLIT;
}

static av_always_inline void filter_mb(VP8Context *s, uint8_t *dst[3], VP8FilterStrength *f, int mb_x, int mb_y)
{
  int mbedge_lim, bedge_lim, hev_thresh;
  int filter_level = f->filter_level;
  int inner_limit = f->inner_limit;
  int inner_filter = f->inner_filter;
  int linesize = s->linesize;
  int uvlinesize = s->uvlinesize;

  if (!filter_level)
    return;

  mbedge_lim = 2*(filter_level+2) + inner_limit;
  bedge_lim = 2* filter_level    + inner_limit;
  hev_thresh = filter_level >= 15;

  if (s->keyframe) {
    if (filter_level >= 40)
      hev_thresh = 2;
  } else {
    if (filter_level >= 40)
      hev_thresh = 3;
    else if (filter_level >= 20)
      hev_thresh = 2;
  }

  if (mb_x) {
    s->vp8dsp.vp8_h_loop_filter16y(dst[0],     linesize,
				   mbedge_lim, inner_limit, hev_thresh);
    s->vp8dsp.vp8_h_loop_filter8uv(dst[1],     dst[2],      uvlinesize,
				   mbedge_lim, inner_limit, hev_thresh);
  }

  if (inner_filter) {
    s->vp8dsp.vp8_h_loop_filter16y_inner(dst[0]+ 4, linesize, bedge_lim,
					 inner_limit, hev_thresh);
    s->vp8dsp.vp8_h_loop_filter16y_inner(dst[0]+ 8, linesize, bedge_lim,
					 inner_limit, hev_thresh);
    s->vp8dsp.vp8_h_loop_filter16y_inner(dst[0]+12, linesize, bedge_lim,
					 inner_limit, hev_thresh);
    s->vp8dsp.vp8_h_loop_filter8uv_inner(dst[1] + 4, dst[2] + 4,
					 uvlinesize,  bedge_lim,
					 inner_limit, hev_thresh);
  }

  if (mb_y) {
    s->vp8dsp.vp8_v_loop_filter16y(dst[0],     linesize,
				   mbedge_lim, inner_limit, hev_thresh);
    s->vp8dsp.vp8_v_loop_filter8uv(dst[1],     dst[2],      uvlinesize,
				   mbedge_lim, inner_limit, hev_thresh);
  }

  if (inner_filter) {
    s->vp8dsp.vp8_v_loop_filter16y_inner(dst[0]+ 4*linesize,
					 linesize,    bedge_lim,
					 inner_limit, hev_thresh);
    s->vp8dsp.vp8_v_loop_filter16y_inner(dst[0]+ 8*linesize,
					 linesize,    bedge_lim,
					 inner_limit, hev_thresh);
    s->vp8dsp.vp8_v_loop_filter16y_inner(dst[0]+12*linesize,
					 linesize,    bedge_lim,
					 inner_limit, hev_thresh);
    s->vp8dsp.vp8_v_loop_filter8uv_inner(dst[1] + 4 * uvlinesize,
					 dst[2] + 4 * uvlinesize,
					 uvlinesize,  bedge_lim,
					 inner_limit, hev_thresh);
  }
}

static av_always_inline void filter_mb_simple(VP8Context *s, uint8_t *dst, VP8FilterStrength *f, int mb_x, int mb_y)
{
  int mbedge_lim, bedge_lim;
  int filter_level = f->filter_level;
  int inner_limit = f->inner_limit;
  int inner_filter = f->inner_filter;
  int linesize = s->linesize;

  if (!filter_level)
    return;

  mbedge_lim = 2*(filter_level+2) + inner_limit;
  bedge_lim = 2* filter_level    + inner_limit;

  if (mb_x)
    s->vp8dsp.vp8_h_loop_filter_simple(dst, linesize, mbedge_lim);
  if (inner_filter) {
    s->vp8dsp.vp8_h_loop_filter_simple(dst+ 4, linesize, bedge_lim);
    s->vp8dsp.vp8_h_loop_filter_simple(dst+ 8, linesize, bedge_lim);
    s->vp8dsp.vp8_h_loop_filter_simple(dst+12, linesize, bedge_lim);
  }

  if (mb_y)
    s->vp8dsp.vp8_v_loop_filter_simple(dst, linesize, mbedge_lim);
  if (inner_filter) {
    s->vp8dsp.vp8_v_loop_filter_simple(dst+ 4*linesize, linesize, bedge_lim);
    s->vp8dsp.vp8_v_loop_filter_simple(dst+ 8*linesize, linesize, bedge_lim);
    s->vp8dsp.vp8_v_loop_filter_simple(dst+12*linesize, linesize, bedge_lim);
  }
}

static void filter_mb_row(VP8Context *s, int mb_y)
{
  VP8FilterStrength *f = s->filter_strength;
  uint8_t *dst[3] = {
    s->framep[VP56_FRAME_CURRENT]->data[0] + 16*mb_y*s->linesize,
    s->framep[VP56_FRAME_CURRENT]->data[1] +  8*mb_y*s->uvlinesize,
    s->framep[VP56_FRAME_CURRENT]->data[2] +  8*mb_y*s->uvlinesize
  };
  int mb_x;

  for (mb_x = 0; mb_x < s->mb_width; mb_x++) {
    backup_mb_border(s->top_border[mb_x+1], dst[0], dst[1], dst[2], s->linesize, s->uvlinesize, 0);
    //filter_mb(s, dst, f++, mb_x, mb_y);
    dst[0] += 16;
    dst[1] += 8;
    dst[2] += 8;
  }
}

static void filter_mb_row_simple(VP8Context *s, int mb_y)
{
  VP8FilterStrength *f = s->filter_strength;
  uint8_t *dst = s->framep[VP56_FRAME_CURRENT]->data[0] + 16*mb_y*s->linesize;
  int mb_x;

  for (mb_x = 0; mb_x < s->mb_width; mb_x++) {
    backup_mb_border(s->top_border[mb_x+1], dst, NULL, NULL, s->linesize, 0, 1);
    //filter_mb_simple(s, dst, f++, mb_x, mb_y);
    dst += 16;
  }
}

static inline void clear_blocks_c_mxu(DCTELEM *blocks,DCTELEM n){
  uint32_t i;
  uint32_t block = (uint32_t)blocks;
  for(i=0;i<n;i++){
    S32STD(xr0, block, 0);
    S32STD(xr0, block, 4);
    S32STD(xr0, block, 8);

    S32STD(xr0, block, 12);
    S32STD(xr0, block, 16);
    S32STD(xr0, block, 20);
    S32STD(xr0, block, 24);
    S32STD(xr0, block, 28);

    block += 32;
  }
}

static int vp8_decode_frame(AVCodecContext *avctx, void *data, int *data_size,
                            AVPacket *avpkt)
{
  VP8Context *s = avctx->priv_data;
  int ret, mb_x, mb_y, i, y, referenced;
  enum AVDiscard skip_thresh;
  AVFrame *av_uninit(curframe);
  uint8_t *dest_y;
  uint8_t *dest_c;

  vpFrame++;
#ifdef JZC_CRC_VER
  av_log(s->avctx, AV_LOG_ERROR, "decode_frame start\n");
  printf("decode_frame %d\n", vpFrame);
#endif

#ifdef JZC_VP8_PMON
  PMON_ON(p0allfrm);
#endif
  if ((ret = decode_frame_header(s, avpkt->data, avpkt->size)) < 0)
    return ret;

  referenced = s->update_last || s->update_golden == VP56_FRAME_CURRENT
    || s->update_altref == VP56_FRAME_CURRENT;

  skip_thresh = !referenced ? AVDISCARD_NONREF :
    !s->keyframe ? AVDISCARD_NONKEY : AVDISCARD_ALL;

  if (avctx->skip_frame >= skip_thresh) {
    s->invisible = 1;
    goto skip_decode;
  }
  s->deblock_filter = s->filter.level && avctx->skip_loop_filter < skip_thresh;

  for (i = 0; i < 4; i++)
    if (&s->frames[i] != s->framep[VP56_FRAME_PREVIOUS] &&
	&s->frames[i] != s->framep[VP56_FRAME_GOLDEN] &&
	&s->frames[i] != s->framep[VP56_FRAME_GOLDEN2]) {
      curframe = s->framep[VP56_FRAME_CURRENT] = &s->frames[i];
      break;
    }

  if (!curframe->data[0]){
    //avctx->release_buffer(avctx, curframe);
    if ((ret = avctx->get_buffer(avctx, curframe))) {
      av_log(avctx, AV_LOG_ERROR, "get_buffer() failed!\n");
      return ret;
    }
  }else{
    //memset(curframe->data[0], 128, 256*s->mb_width*s->mb_height);
    //memset(curframe->data[1], 3, 128*s->mb_width*s->mb_height);
  }
  //memset(curframe->data[0], 5, 256*s->mb_width*s->mb_height);
  //memset(curframe->data[1], 3, 128*s->mb_width*s->mb_height);

  //printf("curframe:0x%08x, 0x%08x\n", curframe->data[0], curframe->data[1]);
  curframe->key_frame = s->keyframe;
  curframe->pict_type = s->keyframe ? FF_I_TYPE : FF_P_TYPE;
  curframe->reference = referenced ? 3 : 0;

  //printf("cur_frame:0x%08x, 0x%08x\n", curframe->data[0], curframe->data[1]);

  // Given that arithmetic probabilities are updated every frame, it's quite likely
  // that the values we have on a random interframe are complete junk if we didn't
  // start decode on a keyframe. So just don't display anything rather than junk.
  if (!s->keyframe && (!s->framep[VP56_FRAME_PREVIOUS] ||
		       !s->framep[VP56_FRAME_GOLDEN] ||
		       !s->framep[VP56_FRAME_GOLDEN2])) {
    av_log(avctx, AV_LOG_WARNING, "Discarding interframe without a prior keyframe!\n");
    return AVERROR_INVALIDDATA;
  }

  s->linesize   = curframe->linesize[0];
  s->uvlinesize = curframe->linesize[1];

  if (!s->edge_emu_buffer)
    s->edge_emu_buffer = av_malloc(21*s->linesize);

  memset(s->top_nnz, 0, s->mb_width*sizeof(*s->top_nnz));

  /* Zero macroblock structures for top/top-left prediction from outside the frame. */
  memset(s->macroblocks + s->mb_height*2 - 1, 0, (s->mb_width+1)*sizeof(*s->macroblocks));

  // top edge of 127 for intra prediction
  memset(s->top_border, 127, (s->mb_width+1)*sizeof(*s->top_border));
  memset(s->ref_count, 0, sizeof(s->ref_count));
  if (s->keyframe)
    memset(s->intra4x4_pred_mode_top, DC_PRED, s->mb_width*4);

#ifdef JZC_VP8_OPT
  count = 0;
  int xchg_tmp;

  volatile int *gp0_chain_ptr0 = (volatile int *)(DDMA_GP0_DES_CHAIN);
  volatile int *gp1_chain_ptr0 = (volatile int *)TCSM1_VCADDR(DDMA_GP1_DES_CHAIN0);

  gp0_chain_ptr0[2] = GP_STRD(DOUT_Y_STRD,GP_FRM_NML,16);
  gp0_chain_ptr0[3] = GP_UNIT(GP_TAG_LK,16,256);
  gp0_chain_ptr0[6] = GP_STRD(DOUT_C_STRD,GP_FRM_NML,16);
  gp0_chain_ptr0[7] = GP_UNIT(GP_TAG_LK,8,64);
  gp0_chain_ptr0[10] = GP_STRD(DOUT_C_STRD,GP_FRM_NML,16);
  gp0_chain_ptr0[11] = GP_UNIT(GP_TAG_LK,8,64);
  gp0_chain_ptr0[12] = TCSM0_PADDR(TCSM0_GP1_POLL_END);
  gp0_chain_ptr0[13] = TCSM1_PADDR(TCSM1_GP1_POLL_END);
  gp0_chain_ptr0[14] = GP_STRD(4,GP_FRM_NML,4);
  gp0_chain_ptr0[15] = GP_UNIT(GP_TAG_UL,4,4);
  set_gp0_dha(TCSM0_PADDR(gp0_chain_ptr0));
  *((volatile int *)TCSM1_VCADDR(TCSM1_GP1_POLL_END)) = 1;

  mau_reg_ptr0 = (VMAU_CHN_BASE0);
  mau_reg_ptr1 = (VMAU_CHN_BASE1);
  mau_reg_ptr2 = (VMAU_CHN_BASE2);

#if 0
  volatile uint8_t *mau_dst_ybuf0 = TCSM1_PADDR(RECON_YBUF0);
  volatile uint8_t *mau_dst_cbuf0 = TCSM1_PADDR(RECON_CBUF0);
  volatile uint8_t *mau_dst_ybuf1 = TCSM1_PADDR(RECON_YBUF1);
  volatile uint8_t *mau_dst_cbuf1 = TCSM1_PADDR(RECON_CBUF1);
  volatile uint8_t *mau_dst_ybuf2 = TCSM1_PADDR(RECON_YBUF2);
  volatile uint8_t *mau_dst_cbuf2 = TCSM1_PADDR(RECON_CBUF2);
#else
  volatile uint8_t *mau_dst_ybuf0 = TCSM0_PADDR(T0_RECON_YBUF0);
  volatile uint8_t *mau_dst_cbuf0 = TCSM0_PADDR(T0_RECON_CBUF0);
  volatile uint8_t *mau_dst_ybuf1 = TCSM0_PADDR(T0_RECON_YBUF1);
  volatile uint8_t *mau_dst_cbuf1 = TCSM0_PADDR(T0_RECON_CBUF1);
  volatile uint8_t *mau_dst_ybuf2 = TCSM0_PADDR(T0_RECON_YBUF2);
  volatile uint8_t *mau_dst_cbuf2 = TCSM0_PADDR(T0_RECON_CBUF2);
#endif

  block0 = TCSM0_VCADDR(VP8_BLOCK0);
  block1 = TCSM0_VCADDR(VP8_BLOCK1);
  block2 = TCSM0_VCADDR(VP8_BLOCK2);

  motion_dha0 = TCSM1_VCADDR(TCSM1_MOTION_DHA0);
  motion_dha1 = TCSM1_VCADDR(TCSM1_MOTION_DHA1);
  motion_dha2 = TCSM1_VCADDR(TCSM1_MOTION_DHA2);
  motion_dsa0 = TCSM1_VCADDR(TCSM1_MOTION_DSA0);
  motion_dsa1 = TCSM1_VCADDR(TCSM1_MOTION_DSA1);
  motion_dsa2 = TCSM1_VCADDR(TCSM1_MOTION_DSA2);

  memset(mb0, 0, sizeof(VP8_MB_DecARGs));
  memset(mb1, 0, sizeof(VP8_MB_DecARGs));
  memset(mb2, 0, sizeof(VP8_MB_DecARGs));

  motion_config_vp8(s);
  vp8_quant_init();

  uint8_t *dst[3] = {
    get_phy_addr((int*)curframe->data[0]),
    get_phy_addr((int*)curframe->data[1]),
    get_phy_addr((int*)curframe->data[1])
    //curframe->data[2]
  };
  
  if (!s->keyframe){
    if (s->framep[1]){
      //printf("framep1:0x%08x\n", s->framep[1]->data[0]);
      SET_TAB1_RLUT(1, get_phy_addr((int)(s->framep[1]->data[0])), 0, 0);
      SET_TAB2_RLUT(1, get_phy_addr((int)(s->framep[1]->data[1])), 0, 0, 0, 0);
    }
    if (s->framep[2]){
      //printf("framep2:0x%08x\n", s->framep[2]->data[0]);
      SET_TAB1_RLUT(2, get_phy_addr((int)(s->framep[2]->data[0])), 0, 0);
      SET_TAB2_RLUT(2, get_phy_addr((int)(s->framep[2]->data[1])), 0, 0, 0, 0);
    }
    if (s->framep[3]){
      //printf("framep3:0x%08x\n", s->framep[3]->data[0]);
      SET_TAB1_RLUT(3, get_phy_addr((int)(s->framep[3]->data[0])), 0, 0);
      SET_TAB2_RLUT(3, get_phy_addr((int)(s->framep[3]->data[1])), 0, 0, 0, 0);
    }
  }

  unsigned int tmp = ((VPX & MAU_VIDEO_MSK) << MAU_VIDEO_SFT);
  write_reg(VMAU_V_BASE+VMAU_SLV_GBL_RUN, VMAU_RESET);
  write_reg(VMAU_V_BASE+VMAU_SLV_VIDEO_TYPE, tmp);
	      
  write_reg(VMAU_V_BASE+VMAU_SLV_DEC_STR,(DOUT_C_STRD<<16)|DOUT_Y_STRD);
  write_reg(VMAU_V_BASE+VMAU_SLV_Y_GS, s->mb_width*16);
  write_reg(VMAU_V_BASE+VMAU_SLV_GBL_CTR, VMAU_CTRL_FIFO_M);
	      
  *((volatile uint32_t *)(VMAU_END_FLAG)) = 1;
#endif

  jz_dcache_wb();
  for (mb_y = 0; mb_y < s->mb_height; mb_y++) {
    VP56RangeCoder *c = &s->coeff_partition[mb_y & (s->num_coeff_partitions-1)];
    VP8Macroblock *mb = s->macroblocks + (s->mb_height - mb_y - 1)*2;
    int mb_xy = mb_y*s->mb_width;

    memset(mb - 1, 0, sizeof(*mb));   // zero left macroblock
    memset(s->left_nnz, 0, sizeof(s->left_nnz));
    AV_WN32A(s->intra4x4_pred_mode_left, DC_PRED*0x01010101);

    for (mb_x = 0; mb_x < s->mb_width; mb_x++, mb_xy++, mb++) {
      /* Prefetch the current frame, 4 MBs ahead */
      //printf("mbx:%d mby:%d\n", mb_x, mb_y);
#ifdef JZC_VP8_PMON
      //PMON_ON(mbmd);
#endif
      decode_mb_mode(s, mb, mb_x, mb_y, s->segmentation_map + mb_xy);
#ifdef JZC_VP8_PMON
      //PMON_OFF(mbmd);
#endif

      //prefetch_motion(s, mb, mb_x, mb_y, mb_xy, VP56_FRAME_PREVIOUS);

#ifdef JZC_VP8_OPT
      //memset(block0, 0, sizeof(resid));
      clear_blocks_c_mxu(block0,25);

      mau_reg_ptr1->main_cbp = 0;
      mau_reg_ptr1->id_mlen = 768;
#endif

      if (!mb->skip){
	decode_mb_coeffs(s, c, mb, s->top_nnz[mb_x], s->left_nnz);
#if 0
	if (1){
	//if (vpFrame == 1 && mb_x == 0 && mb_y == 1){
	  ptr_square(block0->block_dc, 2, 1, 16, 16);
	  ptr_square(block0->block, 2, 96, 4, 4);
	  printf("\n");
	}
#endif
      }else{
	//printf("mb_skip\n");
      }

      if (mb2->mb.mode > MODE_I4x4){
#ifdef JZC_VP8_PMON
	//PMON_ON(mcpoll);
#endif
	while(*(volatile int *)motion_dsa2 != (0x80000000 | 0xFFFF) ){
	  //printf("motion_dsa is 0x%08x\n", *(volatile int *)motion_dsa);
	};
#ifdef JZC_VP8_PMON
	//PMON_OFF(mcpoll);
#endif
	*(volatile int *)motion_dsa2 = 0;
#if 0
	if (vpFrame == 2 && mb2->mb_x == 0 && mb2->mb_y == 0){
	  volatile uint8_t *res = VMAU_MC_Y0;
	  printf("mc poll mb_x:%d mb_y:%d\n", mb_x, mb_y);
	  ptr_square(res, 1, 256, 16, 16);
	  printf("\n");
	  res = VMAU_MC_C0;
	  ptr_square(res, 1, 128, 8, 8);
	  //printf("\n");
	  //ptr_square(res+8, 1, 8, 8, 16);
	  printf("\n");
	}
#endif
      }

      if (mb->mode <= MODE_I4x4){
	intra_predict(s, dst, mb, mb_x, mb_y);
      }else{
	JZC_inter_predict(mb, mb_x, mb_y);
      }

      get_mb_args(s, mb, mb_x, mb_y);

      if (mb->skip){
	AV_ZERO64(s->left_nnz);
	AV_WN64(s->top_nnz[mb_x], 0);   // array of 9, so unaligned

	// Reset DC block predictors if they would exist if the mb had coefficients
	if (mb->mode != MODE_I4x4 && mb->mode != VP8_MVMODE_SPLIT) {
	  s->left_nnz[8]      = 0;
	  s->top_nnz[mb_x][8] = 0;
	}
      }

#ifdef JZC_VP8_OPT
      //PMON_ON(p0test);
#endif
      if (count > 0){
	if (mb2->mb.mode <= MODE_I4x4){
	  //printf("intra pred\n");
#if 0
	  if (count > 1){
	    if (vpFrame == 1 && mb1->mb_x == 8 && mb1->mb_y == 8){
	      uint8_t *res = TCSM1_VCADDR(mau_dst_ybuf2);
	      printf("mb_x:%d mb_y:%d\n", mb1->mb_x, mb1->mb_y);
	      ptr_square(res, 1, 16, 16, 20);
	      printf("\n");
	      res = TCSM1_VCADDR(mau_dst_cbuf2);
	      ptr_square(res, 1, 8, 8, 12);
	      printf("\n");
	      ptr_square(res+12*8, 1, 8, 8, 12);
	      printf("\n");
	    }
	  }
#endif
	  if (mb2->mb.skip && (((mau_reg_ptr0->main_cbp>>MAU_MTX_SFT)&0x3) != BMB)){
	    mau_reg_ptr0->main_cbp = (0xB<<26)|(3<<24);
	    mau_reg_ptr0->id_mlen = 0x1<<16;
	  }

	  int idx = mb2->segment;
	  mau_reg_ptr0->quant_para = 0;
	  if (vp8_qpar.base[idx] == 122){
	    mau_reg_ptr0->quant_para = ((vp8_qpar.base[idx]-1)&VPX_QP_MSK)|(((vp8_qpar.ydc_delta[idx]+1)&VPX_Y1DC_MSK)<<VPX_Y1DC_SFT)|(((vp8_qpar.y2dc_delta[idx]+1)&VPX_Y2DC_MSK)<<VPX_Y2DC_SFT)|(((vp8_qpar.y2ac_delta[idx]+1)&VPX_Y2AC_MSK)<<VPX_Y2AC_SFT)|(((vp8_qpar.uvdc_delta[idx]+1)&VPX_UVDC_MSK)<<VPX_UVDC_SFT)|(((vp8_qpar.uvac_delta[idx]+1)&VPX_UVAC_MSK)<<VPX_UVAC_SFT);
	  }else{
	    mau_reg_ptr0->quant_para = (vp8_qpar.base[idx]&VPX_QP_MSK)|((vp8_qpar.ydc_delta[idx]&VPX_Y1DC_MSK)<<VPX_Y1DC_SFT)|((vp8_qpar.y2dc_delta[idx]&VPX_Y2DC_MSK)<<VPX_Y2DC_SFT)|((vp8_qpar.y2ac_delta[idx]&VPX_Y2AC_MSK)<<VPX_Y2AC_SFT)|((vp8_qpar.uvdc_delta[idx]&VPX_UVDC_MSK)<<VPX_UVDC_SFT)|((vp8_qpar.uvac_delta[idx]&VPX_UVAC_MSK)<<VPX_UVAC_SFT);
	  }

	  if (mau_reg_ptr0->main_cbp & (MAU_Y_DC_DCT_MSK<<MAU_Y_DC_DCT_SFT))
	    mau_reg_ptr0->main_addr=TCSM0_PADDR(block2->block_dc);
	  else
	    mau_reg_ptr0->main_addr=TCSM0_PADDR(block2->block);
	  mau_reg_ptr0->ncchn_addr=TCSM0_PADDR(mau_reg_ptr0);
	  mau_reg_ptr0->aux_cbp=0;

#ifdef JZC_VP8_PMON
	  //PMON_ON(vmaupoll);
#endif
	  while(*((volatile uint32_t *)(VMAU_END_FLAG)) == 0);
#ifdef JZC_VP8_PMON
	  //PMON_OFF(vmaupoll);
#endif
	  *((volatile uint32_t *)(VMAU_END_FLAG)) = 0;

	  write_reg(VMAU_V_BASE+VMAU_SLV_DEC_YADDR,mau_dst_ybuf0);
	  write_reg(VMAU_V_BASE+VMAU_SLV_DEC_UADDR,mau_dst_cbuf0);
	  write_reg(VMAU_V_BASE+VMAU_SLV_DEC_VADDR,mau_dst_cbuf0+VP8_DCBUF_LEN);

#if 0
	  //if (1){
	  if (vpFrame == 2 && mb2->mb_x == 0 && mb2->mb_y == 1){
	    uint32_t *mau_l = mau_reg_ptr0;
	    int i;
	    for (i = 0; i < 10; i++)
	      printf("%d 0x%08x\n", i, mau_l[i]);
	  }
#endif
	  write_reg(VMAU_V_BASE+VMAU_SLV_NCCHN_ADDR,TCSM0_PADDR(mau_reg_ptr0));
	  write_reg(VMAU_V_BASE+VMAU_SLV_DEC_DONE, TCSM0_PADDR(VMAU_END_FLAG));
	  write_reg(VMAU_V_BASE+VMAU_SLV_GBL_RUN, 1); //run
	  //printf("VMAU poll end\n");
	}else{
	  //if (vpFrame == 5)
	  //printf("inter pred\n");
#if 0
	  if (count > 1){
	    if (vpFrame == 9){
	      uint8_t *res = TCSM1_VCADDR(mau_dst_ybuf2);
	      printf("mb_x:%d mb_y:%d\n", mb1->mb_x, mb1->mb_y);
	      ptr_square(res, 1, 16, 16, 20);
	      printf("\n");
	      res = TCSM1_VCADDR(mau_dst_cbuf2);
	      ptr_square(res, 1, 8, 8, 12);
	      printf("\n");
	      ptr_square(res+12*8, 1, 8, 8, 12);
	      printf("\n");
	    }
	  }
#endif
	  mau_reg_ptr0->main_cbp |= (MAU_C_ERR_MSK<<MAU_C_ERR_SFT) | (MAU_Y_ERR_MSK<<MAU_Y_ERR_SFT) | 0xFFFFFF;

	  int idx = mb2->segment;
	  mau_reg_ptr0->quant_para = 0;
	  mau_reg_ptr0->quant_para = (vp8_qpar.base[idx]&VPX_QP_MSK)|((vp8_qpar.ydc_delta[idx]&VPX_Y1DC_MSK)<<VPX_Y1DC_SFT)|((vp8_qpar.y2dc_delta[idx]&VPX_Y2DC_MSK)<<VPX_Y2DC_SFT)|((vp8_qpar.y2ac_delta[idx]&VPX_Y2AC_MSK)<<VPX_Y2AC_SFT)|((vp8_qpar.uvdc_delta[idx]&VPX_UVDC_MSK)<<VPX_UVDC_SFT)|((vp8_qpar.uvac_delta[idx]&VPX_UVAC_MSK)<<VPX_UVAC_SFT);

	  if (mau_reg_ptr0->main_cbp & (MAU_Y_DC_DCT_MSK<<MAU_Y_DC_DCT_SFT)){
	    mau_reg_ptr0->main_addr = TCSM0_PADDR(block2->block_dc);
	    mau_reg_ptr0->id_mlen = 0x00010320;
	  }else{
	    mau_reg_ptr0->main_addr = TCSM0_PADDR(block2->block);
	    mau_reg_ptr0->id_mlen = 0x00010300;
	  }
	  mau_reg_ptr0->ncchn_addr = TCSM0_PADDR(mau_reg_ptr0);
	  mau_reg_ptr0->tlr_cpmd = 0;
	  mau_reg_ptr0->y_pred_mode[0] = 0;
	  mau_reg_ptr0->y_pred_mode[1] = 0;

#ifdef JZC_VP8_PMON
	  //PMON_ON(vmaupoll);
#endif
	  while(*((volatile uint32_t *)(VMAU_END_FLAG)) == 0);
#ifdef JZC_VP8_PMON
	  //PMON_OFF(vmaupoll);
#endif
	  *((volatile uint32_t *)(VMAU_END_FLAG)) = 0;

	  write_reg(VMAU_V_BASE+VMAU_SLV_DEC_YADDR,mau_dst_ybuf0);
	  write_reg(VMAU_V_BASE+VMAU_SLV_DEC_UADDR,mau_dst_cbuf0);
	  write_reg(VMAU_V_BASE+VMAU_SLV_DEC_VADDR,mau_dst_cbuf0+VP8_DCBUF_LEN);
#if 0
	  //if (1){
	  if (vpFrame == 2 && mb2->mb_x == 0 && mb2->mb_y == 1){
	    uint32_t *mau_l = mau_reg_ptr0;
	    int i;
	    for (i = 0; i < 10; i++)
	      printf("%d 0x%08x\n", i, mau_l[i]);
	  }
#endif
	  write_reg(VMAU_V_BASE+VMAU_SLV_NCCHN_ADDR,TCSM0_PADDR(mau_reg_ptr0));
	  write_reg(VMAU_V_BASE+VMAU_SLV_DEC_DONE, TCSM0_PADDR(VMAU_END_FLAG));
	  write_reg(VMAU_V_BASE+VMAU_SLV_GBL_RUN, 1); //run
	}
      }
#ifdef JZC_VP8_OPT
      //PMON_OFF(p0test);
#endif


#if 1

      if (count > 1){
#if 0
	//if (1){
	if (vpFrame == 1 && (mb1->mb_x == 1||mb1->mb_x == 0) && mb1->mb_y == 7){
	  uint8_t *res = TCSM0_VCADDR(mau_dst_ybuf2);
	  printf("mb_x:%d mb_y:%d\n", mb1->mb_x, mb1->mb_y);
	  ptr_square(res, 1, 16, 16, 20);
	  printf("\n");
	  res = TCSM0_VCADDR(mau_dst_cbuf2);
	  ptr_square(res, 1, 8, 8, 12);
	  printf("\n");
	  ptr_square(res+12*8, 1, 8, 8, 12);
	  printf("\n");
	}
#endif

#ifdef JZC_VP8_PMON
	//PMON_ON(gp0poll);
#endif
	while (*((volatile int *)TCSM1_VCADDR(TCSM1_GP1_POLL_END)) == 0);
#ifdef JZC_VP8_PMON
	//PMON_OFF(gp0poll);
#endif
	*((volatile int *)TCSM1_VCADDR(TCSM1_GP1_POLL_END)) = 0;

	gp0_chain_ptr0[0] = mau_dst_ybuf2;
	gp0_chain_ptr0[1] = dst[0];

	gp0_chain_ptr0[4] = mau_dst_cbuf2;
	gp0_chain_ptr0[5] = dst[1];

	gp0_chain_ptr0[8] = mau_dst_cbuf2+96;
	gp0_chain_ptr0[9] = dst[1]+8;

	//memset(dst[1], 3, 128);
	set_gp0_dcs();
	//jz_dcache_wb();
#if 0
	if (vpFrame == 1 && (mb1->mb_x == 9||mb1->mb_x == 8) && mb1->mb_y == 8){
	  printf("tcsm0 to ram res\n");
	  //jz_dcache_wb();
	  ptr_square(dst[1], 1, 8, 8, 8);
	  printf("\n");
	}
#endif
	dst[0] += 256;
	dst[1] += 128;
	if (mb1->mb_x == s->mb_width-1){
	  //printf("%d %d\n", mb1->mb_x, s->mb_width);
	  dst[0] += 1024;
	  dst[1] += 512;
	}
      }
#endif
      count++;
      XCHG3(mau_dst_ybuf0,mau_dst_ybuf1,mau_dst_ybuf2,xchg_tmp);
      XCHG3(mau_dst_cbuf0,mau_dst_cbuf1,mau_dst_cbuf2,xchg_tmp);
      XCHG3(mau_reg_ptr0,mau_reg_ptr1,mau_reg_ptr2,xchg_tmp);
      XCHG3(block0,block1,block2,xchg_tmp);
      XCHG3(mb0,mb1,mb2,xchg_tmp);
      XCHG3(motion_dha0,motion_dha1,motion_dha2,xchg_tmp);
      XCHG3(motion_dsa0,motion_dsa1,motion_dsa2,xchg_tmp);
    }

    jz_dcache_wb();
#if 0
    if (s->deblock_filter) {
      if (s->filter.simple)
	filter_mb_row_simple(s, mb_y);
      else
	filter_mb_row(s, mb_y);
    }
#endif
  }
  jz_dcache_wb();
#ifdef JZC_VP8_OPT
  if (mb2->mb.mode > MODE_I4x4){
    while(*(volatile int *)motion_dsa2 != (0x80000000 | 0xFFFF) ){
      //printf("motion_dsa is 0x%08x\n", *(volatile int *)motion_dsa);
    };
    *(volatile int *)motion_dsa2 = 0;
  }

  while(*((volatile uint32_t *)(VMAU_END_FLAG)) == 0);
  *((volatile uint32_t *)(VMAU_END_FLAG)) = 0;

  uint8_t *res = TCSM1_VCADDR(mau_dst_ybuf2);
#if 0
  if (vpFrame == 9){
    printf("mb_x:%d mb_y:%d\n", mb1->mb_x, mb1->mb_y);
    ptr_square(res, 1, 16, 16, 20);
    printf("\n");
    res = TCSM1_VCADDR(mau_dst_cbuf2);
    ptr_square(res, 1, 8, 8, 12);
    printf("\n");
    ptr_square(res+12*8, 1, 8, 8, 12);
    printf("\n");
  }
#endif

  int idx = mb2->segment;
  if (mb2->mb.mode <= MODE_I4x4){
    if (mb2->mb.skip && (((mau_reg_ptr0->main_cbp>>MAU_MTX_SFT)&0x3) != BMB)){
      mau_reg_ptr0->main_cbp = (0xB<<26)|(3<<24);
      mau_reg_ptr0->id_mlen = 0x1<<16;
    }

    mau_reg_ptr0->quant_para = 0;
    if (vp8_qpar.base[idx] == 122){
      mau_reg_ptr0->quant_para = ((vp8_qpar.base[idx]-1)&VPX_QP_MSK)|(((vp8_qpar.ydc_delta[idx]+1)&VPX_Y1DC_MSK)<<VPX_Y1DC_SFT)|(((vp8_qpar.y2dc_delta[idx]+1)&VPX_Y2DC_MSK)<<VPX_Y2DC_SFT)|(((vp8_qpar.y2ac_delta[idx]+1)&VPX_Y2AC_MSK)<<VPX_Y2AC_SFT)|(((vp8_qpar.uvdc_delta[idx]+1)&VPX_UVDC_MSK)<<VPX_UVDC_SFT)|(((vp8_qpar.uvac_delta[idx]+1)&VPX_UVAC_MSK)<<VPX_UVAC_SFT);
    }else{
      mau_reg_ptr0->quant_para = (vp8_qpar.base[idx]&VPX_QP_MSK)|((vp8_qpar.ydc_delta[idx]&VPX_Y1DC_MSK)<<VPX_Y1DC_SFT)|((vp8_qpar.y2dc_delta[idx]&VPX_Y2DC_MSK)<<VPX_Y2DC_SFT)|((vp8_qpar.y2ac_delta[idx]&VPX_Y2AC_MSK)<<VPX_Y2AC_SFT)|((vp8_qpar.uvdc_delta[idx]&VPX_UVDC_MSK)<<VPX_UVDC_SFT)|((vp8_qpar.uvac_delta[idx]&VPX_UVAC_MSK)<<VPX_UVAC_SFT);
    }

    if (mau_reg_ptr0->main_cbp & (MAU_Y_DC_DCT_MSK<<MAU_Y_DC_DCT_SFT))
      mau_reg_ptr0->main_addr=TCSM0_PADDR(block2->block_dc);
    else
      mau_reg_ptr0->main_addr=TCSM0_PADDR(block2->block);
    mau_reg_ptr0->ncchn_addr=TCSM0_PADDR(mau_reg_ptr0);
    mau_reg_ptr0->aux_cbp=0;

    write_reg(VMAU_V_BASE+VMAU_SLV_DEC_YADDR,mau_dst_ybuf0);
    write_reg(VMAU_V_BASE+VMAU_SLV_DEC_UADDR,mau_dst_cbuf0);
    write_reg(VMAU_V_BASE+VMAU_SLV_DEC_VADDR,mau_dst_cbuf0+VP8_DCBUF_LEN);
  
    write_reg(VMAU_V_BASE+VMAU_SLV_NCCHN_ADDR,TCSM0_PADDR(mau_reg_ptr0));
    write_reg(VMAU_V_BASE+VMAU_SLV_DEC_DONE, TCSM0_PADDR(VMAU_END_FLAG));
    write_reg(VMAU_V_BASE+VMAU_SLV_GBL_RUN, 1); //run
  }else{
    mau_reg_ptr0->main_cbp |= (MAU_C_ERR_MSK<<MAU_C_ERR_SFT) | (MAU_Y_ERR_MSK<<MAU_Y_ERR_SFT) | 0xFFFFFF;

    mau_reg_ptr0->quant_para = 0;
    mau_reg_ptr0->quant_para = (vp8_qpar.base[idx]&VPX_QP_MSK)|((vp8_qpar.ydc_delta[idx]&VPX_Y1DC_MSK)<<VPX_Y1DC_SFT)|((vp8_qpar.y2dc_delta[idx]&VPX_Y2DC_MSK)<<VPX_Y2DC_SFT)|((vp8_qpar.y2ac_delta[idx]&VPX_Y2AC_MSK)<<VPX_Y2AC_SFT)|((vp8_qpar.uvdc_delta[idx]&VPX_UVDC_MSK)<<VPX_UVDC_SFT)|((vp8_qpar.uvac_delta[idx]&VPX_UVAC_MSK)<<VPX_UVAC_SFT);

    if (mau_reg_ptr0->main_cbp & (MAU_Y_DC_DCT_MSK<<MAU_Y_DC_DCT_SFT)){
      mau_reg_ptr0->main_addr = TCSM0_PADDR(block2->block_dc);
      mau_reg_ptr0->id_mlen = 0x00010320;
    }else{
      mau_reg_ptr0->main_addr = TCSM0_PADDR(block2->block);
      mau_reg_ptr0->id_mlen = 0x00010300;
    }
    mau_reg_ptr0->ncchn_addr = TCSM0_PADDR(mau_reg_ptr0);
    mau_reg_ptr0->tlr_cpmd = 0;
    mau_reg_ptr0->y_pred_mode[0] = 0;
    mau_reg_ptr0->y_pred_mode[1] = 0;

    write_reg(VMAU_V_BASE+VMAU_SLV_DEC_YADDR,mau_dst_ybuf0);
    write_reg(VMAU_V_BASE+VMAU_SLV_DEC_UADDR,mau_dst_cbuf0);
    write_reg(VMAU_V_BASE+VMAU_SLV_DEC_VADDR,mau_dst_cbuf0+VP8_DCBUF_LEN);

    write_reg(VMAU_V_BASE+VMAU_SLV_NCCHN_ADDR,TCSM0_PADDR(mau_reg_ptr0));
    write_reg(VMAU_V_BASE+VMAU_SLV_DEC_DONE, TCSM0_PADDR(VMAU_END_FLAG));
    write_reg(VMAU_V_BASE+VMAU_SLV_GBL_RUN, 1); //run
  }
#if 1
  while (*((volatile int *)TCSM1_VCADDR(TCSM1_GP1_POLL_END)) == 0);
  *((volatile int *)TCSM1_VCADDR(TCSM1_GP1_POLL_END)) = 0;
  gp0_chain_ptr0[0] = mau_dst_ybuf2;
  gp0_chain_ptr0[1] = (dst[0]);

  gp0_chain_ptr0[4] = mau_dst_cbuf2;
  gp0_chain_ptr0[5] = (dst[1]);

  gp0_chain_ptr0[8] = mau_dst_cbuf2+96;
  gp0_chain_ptr0[9] = (dst[1]+8);

  set_gp0_dcs();
#endif

  dst[0] += 256;
  dst[1] += 128;

  XCHG3(mau_dst_ybuf0,mau_dst_ybuf1,mau_dst_ybuf2,xchg_tmp);
  XCHG3(mau_dst_cbuf0,mau_dst_cbuf1,mau_dst_cbuf2,xchg_tmp);
  XCHG3(mau_reg_ptr0,mau_reg_ptr1,mau_reg_ptr2,xchg_tmp);
  XCHG3(block0,block1,block2,xchg_tmp);
  XCHG3(mb0,mb1,mb2,xchg_tmp);

  while(*((volatile uint32_t *)(VMAU_END_FLAG)) == 0);
  *((volatile uint32_t *)(VMAU_END_FLAG)) = 0;

  res = TCSM1_VCADDR(mau_dst_ybuf2);
#if 0
  if (vpFrame == 9){
    printf("mb_x:%d mb_y:%d\n", mb1->mb_x, mb1->mb_y);
    ptr_square(res, 1, 16, 16, 20);
    printf("\n");
    res = TCSM1_VCADDR(mau_dst_cbuf2);
    ptr_square(res, 1, 8, 8, 12);
    printf("\n");
    ptr_square(res+12*8, 1, 8, 8, 12);
    printf("\n");
  }
#endif

#if 1
  while (*((volatile int *)TCSM1_VCADDR(TCSM1_GP1_POLL_END)) == 0);
  *((volatile int *)TCSM1_VCADDR(TCSM1_GP1_POLL_END)) = 0;
  gp0_chain_ptr0[0] = mau_dst_ybuf2;
  gp0_chain_ptr0[1] = (dst[0]);

  gp0_chain_ptr0[4] = mau_dst_cbuf2;
  gp0_chain_ptr0[5] = (dst[1]);

  gp0_chain_ptr0[8] = mau_dst_cbuf2+96;
  gp0_chain_ptr0[9] = (dst[1]+8);

  set_gp0_dcs();

  while (*((volatile int *)TCSM1_VCADDR(TCSM1_GP1_POLL_END)) == 0);
  *((volatile int *)TCSM1_VCADDR(TCSM1_GP1_POLL_END)) = 0;
#endif
#endif//jzc_vp8_opt

#ifdef JZC_VP8_PMON
  PMON_OFF(p0allfrm);
#endif

#ifdef JZC_VP8_PMON
  int mbnum=s->mb_width*s->mb_height;
  printf("PMON:p0test:%d  mbmd:%d  p0allfrm:%d,%d\n", p0test_pmon_val/mbnum/vpFrame, mbmd_pmon_val/mbnum, p0allfrm_pmon_val, p0allfrm_pmon_val/mbnum/vpFrame);
  printf("PMON:mcpoll:%d,%d  gp0poll:%d,%d  vmaupoll:%d,%d\n", mcpoll_pmon_val, mcpoll_pmon_val/mbnum, gp0poll_pmon_val, gp0poll_pmon_val/mbnum, vmaupoll_pmon_val, vmaupoll_pmon_val/mbnum);
  //PMON_CLEAR(p0test);
  PMON_CLEAR(mcpoll);
  PMON_CLEAR(gp0poll);
  PMON_CLEAR(vmaupoll);
  PMON_CLEAR(mbmd);
#endif

  jz_dcache_wb();
  //if (0)
  if (vpFrame == 0)
    printf_frm_data(curframe->data[0], curframe->data[1], s->mb_width, s->mb_height);
#ifdef JZC_CRC_VER
  linear_crc_frame(curframe->data[0], curframe->data[1], s->mb_width, s->mb_height);
#endif

 skip_decode:
  // if future frames don't use the updated probabilities,
  // reset them to the values we saved
  if (!s->update_probabilities)
    s->prob[0] = s->prob[1];

  // check if golden and altref are swapped
  if (s->update_altref == VP56_FRAME_GOLDEN &&
      s->update_golden == VP56_FRAME_GOLDEN2)
    FFSWAP(AVFrame *, s->framep[VP56_FRAME_GOLDEN], s->framep[VP56_FRAME_GOLDEN2]);
  else {
    if (s->update_altref != VP56_FRAME_NONE)
      s->framep[VP56_FRAME_GOLDEN2] = s->framep[s->update_altref];

    if (s->update_golden != VP56_FRAME_NONE)
      s->framep[VP56_FRAME_GOLDEN] = s->framep[s->update_golden];
  }

  if (s->update_last) // move cur->prev
    s->framep[VP56_FRAME_PREVIOUS] = s->framep[VP56_FRAME_CURRENT];

  // release no longer referenced frames
  for (i = 0; i < 4; i++)
    if (s->frames[i].data[0] &&
	&s->frames[i] != s->framep[VP56_FRAME_CURRENT] &&
	&s->frames[i] != s->framep[VP56_FRAME_PREVIOUS] &&
	&s->frames[i] != s->framep[VP56_FRAME_GOLDEN] &&
	&s->frames[i] != s->framep[VP56_FRAME_GOLDEN2])
      avctx->release_buffer(avctx, &s->frames[i]);

  if (!s->invisible) {
    *(AVFrame*)data = *s->framep[VP56_FRAME_CURRENT];
    *data_size = sizeof(AVFrame);
  }

#ifdef JZC_CRC_VER
  av_log(s->avctx, AV_LOG_ERROR, "decode_frame end\n");    
#endif
  return avpkt->size;
}

static av_cold int vp8_decode_init(AVCodecContext *avctx)
{
  VP8Context *s = avctx->priv_data;

  use_jz_buf = 1;
  s->avctx = avctx;
  avctx->pix_fmt = PIX_FMT_YUV420P;

  dsputil_init(&s->dsp, avctx);
  ff_h264_pred_init(&s->hpc, CODEC_ID_VP8);
  ff_vp8dsp_init(&s->vp8dsp);

  // intra pred needs edge emulation among other things
  if (avctx->flags&CODEC_FLAG_EMU_EDGE) {
    av_log(avctx, AV_LOG_ERROR, "Edge emulation not supported\n");
    return AVERROR_PATCHWELCOME;
  }

#ifdef JZC_VP8_OPT
  S32I2M(xr16, 0x7);//open mxu

  AUX_RESET();
  tcsm_init();

  SET_VPU_GLBC(1, 0, 0, 4, 4);

  vpFrame = 0;

  mb0 = jz4740_alloc_frame(256, sizeof(VP8_MB_DecARGs));
  mb1 = jz4740_alloc_frame(256, sizeof(VP8_MB_DecARGs));
  mb2 = jz4740_alloc_frame(256, sizeof(VP8_MB_DecARGs));

  //blk = jz4740_alloc_frame(256, sizeof(resid));
  memset(TCSM1_VCADDR(ZERO_BLOCK), 0, 800);
  *((volatile int *)TCSM0_GP1_POLL_END) = 1;
#endif

#ifdef JZC_CRC_VER
  crc_code=0;
#endif

  return 0;
}

static av_cold int vp8_decode_free(AVCodecContext *avctx)
{
  vp8_decode_flush(avctx);
#ifdef JZC_VP8_OPT
  use_jz_buf = 0;
#endif
  return 0;
}

AVCodec vp8_decoder = {
  "vp8",
  AVMEDIA_TYPE_VIDEO,
  CODEC_ID_VP8,
  sizeof(VP8Context),
  vp8_decode_init,
  NULL,
  vp8_decode_free,
  vp8_decode_frame,
  CODEC_CAP_DR1,
  .flush = vp8_decode_flush,
  .long_name = NULL_IF_CONFIG_SMALL("On2 VP8"),
};
