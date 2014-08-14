/* libffmpeg2   */
#include "internal.h"
#include "avcodec.h"
#include "dsputil.h"
#include "mpegvideo.h"
#include "mpeg12.h"
#include "mpeg12data.h"
#include "mpeg12decdata.h"
#include "bytestream.h"
#include "vdpau_internal.h"
#include "xvmc_internal.h"

#include "../libjzcommon/jzmedia.h"
#include "../libjzcommon/jzasm.h"
#include "jzsoc/t_intpid.h"
#include "jzsoc/vmau.h"
#undef MPEG2_P1_USE_PADDR
#include "jzsoc/t_motion.h"
#include "jzsoc/ffmpeg2_tcsm1.h"

//#define JZC_PMON_P0
#ifdef JZC_PMON_P0
#define STA_CCLK
#include "../libjzcommon/jz4760e_pmon.h"

PMON_CREAT(param);
#endif

//#define CRC_CHECK
volatile int crc_index;
volatile int frame_num;
volatile int wait;
/* ------------ JZC HW OPT --------------- */
#ifdef JZC_DCORE_OPT
#include "jzsoc/ffmpeg2_tcsm0.h"
#include "jzsoc/ffmpeg2_tcsm1.h"
#include "jzsoc/ffmpeg2_dcore.h"
#include "../libjzcommon/jz4760e_dcsc.h"
#include "../libjzcommon/jz4760e_2ddma_hw.h"
extern unsigned int get_phy_addr (unsigned int vaddr);
extern void mp_msg();

MPEG2_MB_DecARGs * f1;

volatile int * mbnum_wp;
volatile int * mbnum_rp;
volatile unsigned int * task_fifo_wp;
volatile unsigned int * tcsm1_fifo_rp;
volatile int * unkown_block;
volatile unsigned int * p1_debug = (volatile unsigned int *)TCSM0_P1_DEBUG;
#endif

#ifdef JZC_TLB_OPT
volatile int tlb_i;
#include "../libjzcommon/t_vputlb.h"
#endif

#ifdef  JZC_TCSM_OPT
#include "../libjzcommon/jz4760e_tcsm_init.c"
#endif

#ifdef JZC_VLC_HW_OPT
volatile char * vc1_hw_bs_buffer;
int vc1_hw_codingset1=0xab;
int vc1_hw_codingset2=0xab;
#include "vlc_tables.h"
#include "vlc_bs.c"
#endif

/* Slice level init, can be put in frame level.
 * This func will configure the basic setup of MCE.
 */

#ifdef VMAU_OPT
static void motion_init_mpeg2(int intpid, int cintpid)
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

//           ebms,esms,earm,epmv,esa,ebme,cae, pgc,ch2en,pri,ckge,ofa,rot,rotdir,wm,ccf,irqe,rst,en
  SET_REG1_CTRL(0,  0,  0,   0,   0,  0,   1,  0xF, 1,    3,  1,   1,  0,  0,     0, 1,  0,   0,  1);
  
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
}


static void motion_config_mpeg2( MpegEncContext * s ) //--be invoked every frame
{

  SET_TAB1_RLUT(0,  get_phy_addr((unsigned int)(s->last_picture.data[0])), 0, 0);
  SET_TAB1_RLUT(16, get_phy_addr((unsigned int)(s->next_picture.data[0])), 0, 0);
  SET_TAB1_RLUT(1,  get_phy_addr((unsigned int)(s->last_picture.data[0])), 0, 0);
  SET_TAB1_RLUT(17, get_phy_addr((unsigned int)(s->next_picture.data[0])), 0, 0);
  //SET_TAB1_RLUT(16, get_phy_addr((int)(s->last_picture.data[0])), 0, 0);
  //SET_TAB1_RLUT(17, get_phy_addr((int)(s->next_picture.data[0])), 0, 0);

  SET_TAB2_RLUT(0,  get_phy_addr((unsigned int)(s->last_picture.data[1])), 0, 0, 0, 0);
  SET_TAB2_RLUT(16, get_phy_addr((unsigned int)(s->next_picture.data[1])), 0, 0, 0, 0);
  SET_TAB2_RLUT(1,  get_phy_addr((unsigned int)(s->last_picture.data[1])), 0, 0, 0, 0);
  SET_TAB2_RLUT(17, get_phy_addr((unsigned int)(s->next_picture.data[1])), 0, 0, 0, 0);


  SET_REG1_PINFO(0,/*rgr*/
		 0,/*its*/
		 6,/*its_sft*/
		 0,/*its_scale*/
		 0/*its_rnd*/);
  SET_REG2_PINFO(0,/*rgr*/
		 0,/*its*/
		 6,/*its_sft*/
		 0,/*its_scale*/
		 0/*its_rnd*/);

  SET_REG1_WINFO(0,/*wt*/
		 0, /*wtpd*/
		 IS_BIAVG,/*wtmd*/
		 1,/*biavg_rnd*/
		 0,/*wt_denom*/
		 0,/*wt_sft*/
		 0,/*wt_lcoef*/
		 0/*wt_rcoef*/);
  SET_REG1_WTRND(0);
  SET_REG2_WINFO1(0,/*wt*/
		  0, /*wtpd*/
		  IS_BIAVG,/*wtmd*/
		  1,/*biavg_rnd*/
		  0,/*wt_denom*/
		  0,/*wt_sft*/
		  0,/*wt_lcoef*/
		  0/*wt_rcoef*/);
  SET_REG2_WINFO2(0,/*wt_sft*/
		  0,/*wt_lcoef*/
		  0/*wt_rcoef*/);
  SET_REG2_WTRND(0, 0);

  //SET_REG1_STRD(s->linesize/16,0,DOUT_Y_STRD);
  SET_REG1_STRD(s->width,0, 16);
  SET_REG1_GEOM(s->height, s->width);
  //SET_REG2_STRD(s->linesize/16,0,DOUT_C_STRD);
  SET_REG2_STRD(s->width,0, 8);

}

static inline void vmau_setup(MpegEncContext *s)
{
    write_reg(VMAU_V_BASE+VMAU_GBL_RUN, 4);     //--reset VMAU 
    write_reg(VMAU_V_BASE+VMAU_SLV_VIDEO_TYPE, 0x04);                                   
    write_reg(VMAU_V_BASE+VMAU_SLV_DEC_STR,  (16<<16) | 16 );  
    write_reg(VMAU_V_BASE+VMAU_SLV_DEC_DONE, TCSM1_PADDR( VMAU_DEC_END_FLAG) );       
    write_reg(VMAU_V_BASE+VMAU_SLV_DEC_YADDR,TCSM1_PADDR( IDCT_YOUT)         );             
    write_reg(VMAU_V_BASE+VMAU_SLV_DEC_UADDR,TCSM1_PADDR( IDCT_COUT)         );        
    write_reg(VMAU_V_BASE+VMAU_SLV_DEC_VADDR,TCSM1_PADDR( IDCT_COUT+8)       );       
    write_reg(VMAU_V_BASE+VMAU_SLV_GBL_CTR, 0);                                       
    write_reg(VMAU_V_BASE+VMAU_SLV_Y_GS, s->width);                                 
    write_reg(VMAU_V_BASE+VMAU_SLV_GBL_CTR, 1); //--Fifo Mode                                       

    //-------------------QTable
    int BB = 0;
    for (BB=0;BB<64;BB+=4){
      *(volatile uint32_t *)(VMAU_V_BASE + VMAU_QT_BASE + BB) = (s->intra_matrix[BB] & 0xff ) |\
       ( (s->intra_matrix[BB+1] & 0xff) <<8) | ( (s->intra_matrix[BB+2]& 0xff) <<16 ) |\
       ( (s->intra_matrix[BB+3] & 0xff) <<24);
    }
    for (BB=0;BB<64;BB+=4){
      *(volatile uint32_t *)(VMAU_V_BASE + VMAU_QT_BASE+BB + 0x80)= (s->chroma_intra_matrix[BB]& 0xff)|\
       ( (s->chroma_intra_matrix[BB+1] & 0xff) <<8) | ( (s->chroma_intra_matrix[BB+2]& 0xff) <<16) |\
       ( (s->chroma_intra_matrix[BB+3] & 0xff) <<24);
    }  
    for (BB=0;BB<64;BB+=4){
      *(volatile uint32_t *)(VMAU_V_BASE + VMAU_QT_BASE + BB + 0x40) = (s->inter_matrix[BB] & 0xff ) |\
       ( (s->inter_matrix[BB+1] & 0xff) <<8 ) | ( (s->inter_matrix[BB+2] & 0xff) << 16) |\
       ( (s->inter_matrix[BB+3] & 0xff) <<24 );
    }
    for (BB=0;BB<64;BB+=4){
      *(volatile uint32_t *)(VMAU_V_BASE + VMAU_QT_BASE+BB + 0xC0)= (s->chroma_inter_matrix[BB]&0xff )|\
       ((s->chroma_inter_matrix[BB+1]& 0xff) <<8) | ((s->chroma_inter_matrix[BB+2] & 0xff ) <<16) |\
       ((s->chroma_inter_matrix[BB+3]& 0xff) <<24);
    }
}
#endif

#define MV_VLC_BITS 9
#define MBINCR_VLC_BITS 9
#define MB_PAT_VLC_BITS 9
#define MB_PTYPE_VLC_BITS 6
#define MB_BTYPE_VLC_BITS 6

/* function we need and defined */

static void get_blocks_info(MpegEncContext * s, MPEG2_MB_DecARGs * f1);

/* inward mplayer's function */
static inline int mpeg2_decode_block_non_intra(MpegEncContext *s,
                                        DCTELEM *block,
                                        int n);
static inline int mpeg2_decode_block_intra(MpegEncContext *s,
                                    DCTELEM *block,
                                    int n);
static inline int mpeg2_fast_decode_block_non_intra(MpegEncContext *s, DCTELEM *block, int n);
static inline int mpeg2_fast_decode_block_intra(MpegEncContext *s, DCTELEM *block, int n);
static int mpeg_decode_motion(MpegEncContext *s, int fcode, int pred);
static void exchange_uv(MpegEncContext *s);

static const enum PixelFormat pixfmt_xvmc_mpg2_420[] = {
                                           PIX_FMT_XVMC_MPEG2_IDCT,
                                           PIX_FMT_XVMC_MPEG2_MC,
                                           PIX_FMT_NONE};

uint8_t ff_mpeg12_static_rl_table_store[2][2][2*MAX_RUN + MAX_LEVEL + 3];


#define INIT_2D_VLC_RL(rl, static_size)\
{\
    static RL_VLC_ELEM rl_vlc_table[static_size];\
    INIT_VLC_STATIC(&rl.vlc, TEX_VLC_BITS, rl.n + 2,\
		    &rl.table_vlc[0][1], 4, 2,		\
             &rl.table_vlc[0][0], 4, 2, static_size);\
\
    rl.rl_vlc[0]= rl_vlc_table;\
    init_2d_vlc_rl(&rl);\
}

static void init_2d_vlc_rl(RLTable *rl)
{
    int i;

    for(i=0; i<rl->vlc.table_size; i++){
        int code= rl->vlc.table[i][0];
        int len = rl->vlc.table[i][1];
        int level, run;

        if(len==0){ // illegal code
            run= 65;
            level= MAX_LEVEL;
        }else if(len<0){ //more bits needed
            run= 0;
            level= code;
        }else{
            if(code==rl->n){ //esc
                run= 65;
                level= 0;
            }else if(code==rl->n+1){ //eob
                run= 0;
                level= 127;
            }else{
                run=   rl->table_run  [code] + 1;
                level= rl->table_level[code];
            }
        }
        rl->rl_vlc[0][i].len= len;
        rl->rl_vlc[0][i].level= level;
        rl->rl_vlc[0][i].run= run;
    }
}

/******************************************/
/* decoding */

static VLC mv_vlc;
static VLC mbincr_vlc;
static VLC mb_ptype_vlc;
static VLC mb_btype_vlc;
static VLC mb_pat_vlc;

static av_cold void ffmpeg2_init_vlcs(void)
{
    static int done = 0;

    if (!done) {
        done = 1;

        INIT_VLC_STATIC(&dc_lum_vlc, DC_VLC_BITS, 12,
                 ff_mpeg12_vlc_dc_lum_bits, 1, 1,
                 ff_mpeg12_vlc_dc_lum_code, 2, 2, 512);
        INIT_VLC_STATIC(&dc_chroma_vlc,  DC_VLC_BITS, 12,
                 ff_mpeg12_vlc_dc_chroma_bits, 1, 1,
                 ff_mpeg12_vlc_dc_chroma_code, 2, 2, 514);
        INIT_VLC_STATIC(&mv_vlc, MV_VLC_BITS, 17,
                 &ff_mpeg12_mbMotionVectorTable[0][1], 2, 1,
                 &ff_mpeg12_mbMotionVectorTable[0][0], 2, 1, 518);
        INIT_VLC_STATIC(&mbincr_vlc, MBINCR_VLC_BITS, 36,
                 &ff_mpeg12_mbAddrIncrTable[0][1], 2, 1,
                 &ff_mpeg12_mbAddrIncrTable[0][0], 2, 1, 538);
        INIT_VLC_STATIC(&mb_pat_vlc, MB_PAT_VLC_BITS, 64,
                 &ff_mpeg12_mbPatTable[0][1], 2, 1,
                 &ff_mpeg12_mbPatTable[0][0], 2, 1, 512);

        INIT_VLC_STATIC(&mb_ptype_vlc, MB_PTYPE_VLC_BITS, 7,
                 &table_mb_ptype[0][1], 2, 1,
                 &table_mb_ptype[0][0], 2, 1, 64);
        INIT_VLC_STATIC(&mb_btype_vlc, MB_BTYPE_VLC_BITS, 11,
                 &table_mb_btype[0][1], 2, 1,
                 &table_mb_btype[0][0], 2, 1, 64);
        init_rl(&ff_rl_mpeg1, ff_mpeg12_static_rl_table_store[0]);
        init_rl(&ff_rl_mpeg2, ff_mpeg12_static_rl_table_store[1]);

        INIT_2D_VLC_RL(ff_rl_mpeg1, 680);
        INIT_2D_VLC_RL(ff_rl_mpeg2, 674);
    }
}

static inline int get_dmv(MpegEncContext *s)
{
    if(get_bits1(&s->gb))
        return 1 - (get_bits1(&s->gb) << 1);
    else
        return 0;
}

static inline int get_qscale(MpegEncContext *s)
{
    int qscale = get_bits(&s->gb, 5);
    if (s->q_scale_type) {
        return non_linear_qscale[qscale];
    } else {
        return qscale << 1;
    }
}

/* motion type (for MPEG-2) */
#define MT_FIELD 1
#define MT_FRAME 2
#define MT_16X8  2
#define MT_DMV   3

static int mpeg_decode_mb(MpegEncContext *s)
{
    int i, j, k, cbp, val, mb_type, motion_type;
    const int mb_block_count = 4 + (1<< s->chroma_format);

    //dprintf(s->avctx, "decode_mb: x=%d y=%d\n", s->mb_x, s->mb_y);
    assert(s->mb_skipped==0);
    /* skip mb handling */
    if (s->mb_skip_run-- != 0) {
        if (s->pict_type == FF_P_TYPE) {
            s->mb_skipped = 1;
            s->current_picture.mb_type[ s->mb_x + s->mb_y*s->mb_stride ]= MB_TYPE_SKIP | MB_TYPE_L0 | MB_TYPE_16x16;
        } else {
            int mb_type;

            if(s->mb_x)
                mb_type= s->current_picture.mb_type[ s->mb_x + s->mb_y*s->mb_stride - 1];
            else
                mb_type= s->current_picture.mb_type[ s->mb_width + (s->mb_y-1)*s->mb_stride - 1]; // FIXME not sure if this is allowed in MPEG at all
            if(IS_INTRA(mb_type))
                return -1;

            s->current_picture.mb_type[ s->mb_x + s->mb_y*s->mb_stride ]=
                mb_type | MB_TYPE_SKIP;
//            assert(s->current_picture.mb_type[ s->mb_x + s->mb_y*s->mb_stride - 1]&(MB_TYPE_16x16|MB_TYPE_16x8));

            if((s->mv[0][0][0]|s->mv[0][0][1]|s->mv[1][0][0]|s->mv[1][0][1])==0)
                s->mb_skipped = 1;
        }

        return 0;
    }
    
    switch(s->pict_type) {
    default:
    case FF_I_TYPE:
        if (get_bits1(&s->gb) == 0) {
            if (get_bits1(&s->gb) == 0){
                av_log(s->avctx, AV_LOG_ERROR, "invalid mb type in I Frame at %d %d\n", s->mb_x, s->mb_y);
                return -1;
            }
            mb_type = MB_TYPE_QUANT | MB_TYPE_INTRA;
        } else {
            mb_type = MB_TYPE_INTRA;
        }
        break;
    case FF_P_TYPE:
        mb_type = get_vlc2(&s->gb, mb_ptype_vlc.table, MB_PTYPE_VLC_BITS, 1);
        if (mb_type < 0){
            av_log(s->avctx, AV_LOG_ERROR, "invalid mb type in P Frame at %d %d\n", s->mb_x, s->mb_y);
            return -1;
        }
        mb_type = ptype2mb_type[ mb_type ];
        break;
    case FF_B_TYPE:
        mb_type = get_vlc2(&s->gb, mb_btype_vlc.table, MB_BTYPE_VLC_BITS, 1);
        if (mb_type < 0){
            av_log(s->avctx, AV_LOG_ERROR, "invalid mb type in B Frame at %d %d\n", s->mb_x, s->mb_y);
            return -1;
        }
        mb_type = btype2mb_type[ mb_type ];
        break;
    }
    
    //dprintf(s->avctx, "mb_type=%x\n", mb_type);
    //    motion_type = 0; /* avoid warning */
    if (IS_INTRA(mb_type)) {
        s->dsp.clear_blocks(s->progressive_sequence ? (DCTELEM *)f1->blocks[0] : s->block[0]);

        if(!s->chroma_y_shift){
            s->dsp.clear_blocks(s->progressive_sequence ? (DCTELEM *)f1->blocks[6] : s->block[6]);
        }
        /* compute DCT type */
        if (s->picture_structure == PICT_FRAME && //FIXME add an interlaced_dct coded var?
            !s->frame_pred_frame_dct) {
            s->interlaced_dct = get_bits1(&s->gb);
	}

        if (IS_QUANT(mb_type))
            s->qscale = get_qscale(s);

        if (s->concealment_motion_vectors) {
            /* just parse them */
            if (s->picture_structure != PICT_FRAME)
                skip_bits1(&s->gb); /* field select */

            s->mv[0][0][0]= s->last_mv[0][0][0]= s->last_mv[0][1][0] =
                mpeg_decode_motion(s, s->mpeg_f_code[0][0], s->last_mv[0][0][0]);
            s->mv[0][0][1]= s->last_mv[0][0][1]= s->last_mv[0][1][1] =
                mpeg_decode_motion(s, s->mpeg_f_code[0][1], s->last_mv[0][0][1]);

            skip_bits1(&s->gb); /* marker */
        }else
            memset(s->last_mv, 0, sizeof(s->last_mv)); /* reset mv prediction */
	s->mb_intra = 1;
        //if 1, we memcpy blocks in xvmcvideo
        if(CONFIG_MPEG_XVMC_DECODER && s->avctx->xvmc_acceleration > 1){
            ff_xvmc_pack_pblocks(s,-1);//inter are always full blocks
            if(s->swap_uv){
                exchange_uv(s);
            }
        }
        
        if(s->flags2 & CODEC_FLAG2_FAST){
            for(i=0;i<6;i++) {
                mpeg2_fast_decode_block_intra(s, s->progressive_sequence ? (DCTELEM *)f1->blocks[i] : *s->pblocks[i], i);
            }
        }else{
            for(i=0;i<mb_block_count;i++) {
                if (mpeg2_decode_block_intra(s, s->progressive_sequence ? (DCTELEM *)f1->blocks[i] : *s->pblocks[i], i) < 0)
                    return -1;
            }
        }
    } else {
        if (mb_type & MB_TYPE_ZERO_MV){
            assert(mb_type & MB_TYPE_CBP);

            s->mv_dir = MV_DIR_FORWARD;
            if(s->picture_structure == PICT_FRAME){
                if(!s->frame_pred_frame_dct)
                    s->interlaced_dct = get_bits1(&s->gb);
                s->mv_type = MV_TYPE_16X16;
            }else{
                s->mv_type = MV_TYPE_FIELD;
                mb_type |= MB_TYPE_INTERLACED;
                s->field_select[0][0]= s->picture_structure - 1;
            }

            if (IS_QUANT(mb_type))
                s->qscale = get_qscale(s);

            s->last_mv[0][0][0] = 0;
            s->last_mv[0][0][1] = 0;
            s->last_mv[0][1][0] = 0;
            s->last_mv[0][1][1] = 0;
            s->mv[0][0][0] = 0;
            s->mv[0][0][1] = 0;
        }else{
            assert(mb_type & MB_TYPE_L0L1);
            //FIXME decide if MBs in field pictures are MB_TYPE_INTERLACED
            /* get additional motion vector type */
            if (s->frame_pred_frame_dct)
                motion_type = MT_FRAME;
            else{
	        motion_type = get_bits(&s->gb, 2);
                if (s->picture_structure == PICT_FRAME && HAS_CBP(mb_type))
                    s->interlaced_dct = get_bits1(&s->gb);
            }

            if (IS_QUANT(mb_type))
                s->qscale = get_qscale(s);

            /* motion vectors */
            s->mv_dir= (mb_type>>13)&3;
            dprintf(s->avctx, "motion_type=%d\n", motion_type);

	    /* According to the motion type, we choose the type of mv and calculate the val of mv */
            switch(motion_type) {
            case MT_FRAME: /* or MT_16X8 */
                if (s->picture_structure == PICT_FRAME) {
                    mb_type |= MB_TYPE_16x16;
                    s->mv_type = MV_TYPE_16X16;
                    for(i=0;i<2;i++) {
                        if (USES_LIST(mb_type, i)) {
                            /* MT_FRAME */
                            s->mv[i][0][0]= s->last_mv[i][0][0]= s->last_mv[i][1][0] =
                                mpeg_decode_motion(s, s->mpeg_f_code[i][0], s->last_mv[i][0][0]);
                            s->mv[i][0][1]= s->last_mv[i][0][1]= s->last_mv[i][1][1] =
                                mpeg_decode_motion(s, s->mpeg_f_code[i][1], s->last_mv[i][0][1]);
                            /* full_pel: only for MPEG-1 */
                            if (s->full_pel[i]){
                                s->mv[i][0][0] <<= 1;
                                s->mv[i][0][1] <<= 1;
                            }
                        }
                    }
                } else {
                    mb_type |= MB_TYPE_16x8 | MB_TYPE_INTERLACED;
                    s->mv_type = MV_TYPE_16X8;
                    for(i=0;i<2;i++) {
                        if (USES_LIST(mb_type, i)) {
                            /* MT_16X8 */
                            for(j=0;j<2;j++) {
                                s->field_select[i][j] = get_bits1(&s->gb);
                                for(k=0;k<2;k++) {
                                    val = mpeg_decode_motion(s, s->mpeg_f_code[i][k],
                                                             s->last_mv[i][j][k]);
                                    s->last_mv[i][j][k] = val;
                                    s->mv[i][j][k] = val;
                                }
                            }
                        }
                    }
                }
                break;
            case MT_FIELD:
                s->mv_type = MV_TYPE_FIELD;
                if (s->picture_structure == PICT_FRAME) {
                    mb_type |= MB_TYPE_16x8 | MB_TYPE_INTERLACED;
                    for(i=0;i<2;i++) {
                        if (USES_LIST(mb_type, i)) {
                            for(j=0;j<2;j++) {
                                s->field_select[i][j] = get_bits1(&s->gb);
                                val = mpeg_decode_motion(s, s->mpeg_f_code[i][0],
                                                         s->last_mv[i][j][0]);
                                s->last_mv[i][j][0] = val;
                                s->mv[i][j][0] = val;
                                dprintf(s->avctx, "fmx=%d\n", val);
                                val = mpeg_decode_motion(s, s->mpeg_f_code[i][1],
                                                         s->last_mv[i][j][1] >> 1);
                                s->last_mv[i][j][1] = val << 1;
                                s->mv[i][j][1] = val;
                                dprintf(s->avctx, "fmy=%d\n", val);
                            }
                        }
                    }
                } else {
                    mb_type |= MB_TYPE_16x16 | MB_TYPE_INTERLACED;
                    for(i=0;i<2;i++) {
                        if (USES_LIST(mb_type, i)) {
                            s->field_select[i][0] = get_bits1(&s->gb);
                            for(k=0;k<2;k++) {
                                val = mpeg_decode_motion(s, s->mpeg_f_code[i][k],
                                                         s->last_mv[i][0][k]);
                                s->last_mv[i][0][k] = val;
                                s->last_mv[i][1][k] = val;
                                s->mv[i][0][k] = val;
                            }
                        }
                    }
                }
                break;
            case MT_DMV:
                s->mv_type = MV_TYPE_DMV;
                for(i=0;i<2;i++) {
                    if (USES_LIST(mb_type, i)) {
                        int dmx, dmy, mx, my, m;
                        const int my_shift= s->picture_structure == PICT_FRAME;

                        mx = mpeg_decode_motion(s, s->mpeg_f_code[i][0],
                                                s->last_mv[i][0][0]);
                        s->last_mv[i][0][0] = mx;
                        s->last_mv[i][1][0] = mx;
                        dmx = get_dmv(s);
                        my = mpeg_decode_motion(s, s->mpeg_f_code[i][1],
                                                s->last_mv[i][0][1] >> my_shift);
                        dmy = get_dmv(s);


                        s->last_mv[i][0][1] = my<<my_shift;
                        s->last_mv[i][1][1] = my<<my_shift;

                        s->mv[i][0][0] = mx;
                        s->mv[i][0][1] = my;
                        s->mv[i][1][0] = mx;//not used
                        s->mv[i][1][1] = my;//not used

                        if (s->picture_structure == PICT_FRAME) {
                            mb_type |= MB_TYPE_16x16 | MB_TYPE_INTERLACED;

                            //m = 1 + 2 * s->top_field_first;
                            m = s->top_field_first ? 1 : 3;

                            /* top -> top pred */
                            s->mv[i][2][0] = ((mx * m + (mx > 0)) >> 1) + dmx;
                            s->mv[i][2][1] = ((my * m + (my > 0)) >> 1) + dmy - 1;
                            m = 4 - m;
                            s->mv[i][3][0] = ((mx * m + (mx > 0)) >> 1) + dmx;
                            s->mv[i][3][1] = ((my * m + (my > 0)) >> 1) + dmy + 1;
                        } else {
                            mb_type |= MB_TYPE_16x16;

                            s->mv[i][2][0] = ((mx + (mx > 0)) >> 1) + dmx;
                            s->mv[i][2][1] = ((my + (my > 0)) >> 1) + dmy;
                            if(s->picture_structure == PICT_TOP_FIELD)
                                s->mv[i][2][1]--;
                            else
                                s->mv[i][2][1]++;
                        }
                    }
                }
                break;
            default:
                av_log(s->avctx, AV_LOG_ERROR, "00 motion_type at %d %d\n", s->mb_x, s->mb_y);
                return -1;
            }
        }

        s->mb_intra = 0;
        if (HAS_CBP(mb_type)) {
            s->dsp.clear_blocks(s->progressive_sequence ? (DCTELEM *)f1->blocks[0] : s->block[0]);

            cbp = get_vlc2(&s->gb, mb_pat_vlc.table, MB_PAT_VLC_BITS, 1);
            if(mb_block_count > 6){
                 cbp<<= mb_block_count-6;
                 cbp |= get_bits(&s->gb, mb_block_count-6);
                 s->dsp.clear_blocks(s->progressive_sequence ? (DCTELEM *)f1->blocks[6] : s->block[6]);
            }
            if (cbp <= 0){
                av_log(s->avctx, AV_LOG_ERROR, "invalid cbp at %d %d\n", s->mb_x, s->mb_y);
                return -1;
            }

	    j = 0;
            if(s->flags2 & CODEC_FLAG2_FAST){
                for(i=0;i<6;i++) {
                    if(cbp & 32) {
                        mpeg2_fast_decode_block_non_intra(s, s->progressive_sequence ? (DCTELEM *)f1->blocks[j++] : *s->pblocks[i], i);////----
                    } else {
                        s->block_last_index[i] = -1;
                    }
                    cbp+=cbp;
                }
            }else{
                cbp<<= 12-mb_block_count;
                  for(i=0;i<mb_block_count;i++) {
                      if ( cbp & (1<<11) ) {
                          if (mpeg2_decode_block_non_intra(s, s->progressive_sequence ? (DCTELEM *)f1->blocks[j++] : *s->pblocks[i], i) < 0)///---
                              return -1;
                      } else {
                          s->block_last_index[i] = -1;
                      }
                      cbp+=cbp;
                  }
            }
            /* if the inter block has no cbp, then no vlc, so mb_number is not right */
        }else{
            for(i=0;i<12;i++)
                s->block_last_index[i] = -1;
	}
    }
    s->current_picture.mb_type[ s->mb_x + s->mb_y*s->mb_stride ]= mb_type;
    
    return 0;
}

/* as H.263, but only 17 codes */
static int mpeg_decode_motion(MpegEncContext *s, int fcode, int pred)
{
    int code, sign, val, l, shift;

    code = get_vlc2(&s->gb, mv_vlc.table, MV_VLC_BITS, 2);
    if (code == 0) {
        return pred;
    }
    if (code < 0) {
        return 0xffff;
    }

    sign = get_bits1(&s->gb);
    shift = fcode - 1;
    val = code;
    if (shift) {
        val = (val - 1) << shift;
        val |= get_bits(&s->gb, shift);
        val++;
    }
    if (sign)
        val = -val;
    val += pred;

    /* modulo decoding */
    l= INT_BIT - 5 - shift;
    val = (val<<l)>>l;
    return val;
}

static inline int mpeg2_decode_block_non_intra_hw(MpegEncContext *s, DCTELEM *block, int n)
{
    int level, i, j, run;
    RLTable *rl = &ff_rl_mpeg1;
    uint8_t * const scantable= s->intra_scantable.permutated;
    const uint16_t *quant_matrix;
    const int qscale= s->qscale;
    int mismatch;
    mismatch = 1;
    {
        OPEN_READER(re, &s->gb);
        i = -1;
    
        if (n < 4)
            quant_matrix = s->inter_matrix;
        else
            quant_matrix = s->chroma_inter_matrix;

        // special case for first coefficient, no need to add second VLC table
        UPDATE_CACHE(re, &s->gb);
        if (((int32_t)GET_CACHE(re, &s->gb)) < 0) {
	  //level = ( (3*qscale*quant_matrix[0])>>5 ) ;
	    level = 1;
            if(GET_CACHE(re, &s->gb)&0x40000000){
                level= -level;
            }
            if( s->interlaced_dct || (s->mv_type != MV_TYPE_16X16) ) { //-- won't use MCE and MAU
                level = (3*qscale*quant_matrix[0] )>>5;
	        if(level<0)
		   level = -level; 
            }
            //mismatch ^= level;
            block[0] = level;
            i++;
            SKIP_BITS(re, &s->gb, 2);
            if(((int32_t)GET_CACHE(re, &s->gb)) <= (int32_t)0xBFFFFFFF)
                goto end;
        }
#if MIN_CACHE_BITS < 19
        UPDATE_CACHE(re, &s->gb);
#endif
        /* now quantify & encode AC coefficients */
        for(;;) {
            GET_RL_VLC(level, run, re, &s->gb, rl->rl_vlc[0], TEX_VLC_BITS, 2, 0);

            if(level != 0) {
                i += run;
                j = scantable[i];
                //level= ((level*2+1)*qscale*quant_matrix[j])>>5;
                //level = (level ^ SHOW_SBITS(re, &s->gb, 1)) - SHOW_SBITS(re, &s->gb, 1);
                level = (level ^ SHOW_SBITS(re, &s->gb, 1)) - SHOW_SBITS(re, &s->gb, 1);
                SKIP_BITS(re, &s->gb, 1);
            } else {
                /* escape */
                run = SHOW_UBITS(re, &s->gb, 6)+1; LAST_SKIP_BITS(re, &s->gb, 6);
                UPDATE_CACHE(re, &s->gb);
                level = SHOW_SBITS(re, &s->gb, 12); SKIP_BITS(re, &s->gb, 12);
                i += run;
                j = scantable[i];
                /*
                if(level<0){
                    level= ((-level*2+1)*qscale*quant_matrix[j])>>5;
                    level= -level;
                }else{
                    level= ((level*2+1)*qscale*quant_matrix[j])>>5;
		}*/
#if 0//----------------------------------------------------------------- need futher polishing
                if( s->interlaced_dct || (s->mv_type != MV_TYPE_16X16) )
                {
                    if(level<0){
                        level= ((-level*2+1)*qscale*quant_matrix[j])>>5;
                        level= -level;
                    }else{
                        level= ((level*2+1)*qscale*quant_matrix[j])>>5;
                    }
                }
#endif//--------------------------------------------------------------------------------
            }
            if (i > 63){
                av_log(s->avctx, AV_LOG_ERROR, "ac-tex damaged at %d %d\n", s->mb_x, s->mb_y);
                return -1;
            }
            //mismatch ^= level;
            block[j] = level;
#if MIN_CACHE_BITS < 19
            UPDATE_CACHE(re, &s->gb);
#endif
            if(((int32_t)GET_CACHE(re, &s->gb)) <= (int32_t)0xBFFFFFFF)
                break;
#if MIN_CACHE_BITS >= 19
            UPDATE_CACHE(re, &s->gb);
#endif
        }
end:
        LAST_SKIP_BITS(re, &s->gb, 2);
        CLOSE_READER(re, &s->gb);
    }
    //block[63] ^= (mismatch & 1);
    s->block_last_index[n] = i;
    return 0;
}

static inline int mpeg2_decode_block_non_intra(MpegEncContext *s,
                               DCTELEM *block,
                               int n)
{
  if(s->progressive_sequence)
    return mpeg2_decode_block_non_intra_hw(s, block, n);

    int level, i, j, run;
    RLTable *rl = &ff_rl_mpeg1;
    uint8_t * const scantable= s->intra_scantable.permutated;
    const uint16_t *quant_matrix;
    const int qscale= s->qscale;
    int mismatch;

    mismatch = 1;

    {
        OPEN_READER(re, &s->gb);
        i = -1;
        if (n < 4)
            quant_matrix = s->inter_matrix;
        else
            quant_matrix = s->chroma_inter_matrix;

        // special case for first coefficient, no need to add second VLC table
        UPDATE_CACHE(re, &s->gb);
        if (((int32_t)GET_CACHE(re, &s->gb)) < 0) {
            level= (3*qscale*quant_matrix[0])>>5;
            if(GET_CACHE(re, &s->gb)&0x40000000)
                level= -level;
            block[0] = level;
            mismatch ^= level;
            i++;
            SKIP_BITS(re, &s->gb, 2);
            if(((int32_t)GET_CACHE(re, &s->gb)) <= (int32_t)0xBFFFFFFF)
                goto end;
        }
#if MIN_CACHE_BITS < 19
        UPDATE_CACHE(re, &s->gb);
#endif

        /* now quantify & encode AC coefficients */
        for(;;) {
            GET_RL_VLC(level, run, re, &s->gb, rl->rl_vlc[0], TEX_VLC_BITS, 2, 0);

            if(level != 0) {
                i += run;
                j = scantable[i];
                level= ((level*2+1)*qscale*quant_matrix[j])>>5;
                level = (level ^ SHOW_SBITS(re, &s->gb, 1)) - SHOW_SBITS(re, &s->gb, 1);
                SKIP_BITS(re, &s->gb, 1);
            } else {
                /* escape */
                run = SHOW_UBITS(re, &s->gb, 6)+1; LAST_SKIP_BITS(re, &s->gb, 6);
                UPDATE_CACHE(re, &s->gb);
                level = SHOW_SBITS(re, &s->gb, 12); SKIP_BITS(re, &s->gb, 12);

                i += run;
                j = scantable[i];
                if(level<0){
                    level= ((-level*2+1)*qscale*quant_matrix[j])>>5;
                    level= -level;
                }else{
                    level= ((level*2+1)*qscale*quant_matrix[j])>>5;
                }
            }
            if (i > 63){
                av_log(s->avctx, AV_LOG_ERROR, "ac-tex damaged at %d %d\n", s->mb_x, s->mb_y);
                return -1;
            }

            mismatch ^= level;
            block[j] = level;
#if MIN_CACHE_BITS < 19
            UPDATE_CACHE(re, &s->gb);
#endif
            if(((int32_t)GET_CACHE(re, &s->gb)) <= (int32_t)0xBFFFFFFF)
                break;
#if MIN_CACHE_BITS >= 19
            UPDATE_CACHE(re, &s->gb);
#endif
        }
end:
        LAST_SKIP_BITS(re, &s->gb, 2);
        CLOSE_READER(re, &s->gb);
    }
    block[63] ^= (mismatch & 1);

    s->block_last_index[n] = i;
    return 0;
}

static inline int mpeg2_fast_decode_block_non_intra(MpegEncContext *s,
                               DCTELEM *block,
                               int n)
{
    int level, i, j, run;
    RLTable *rl = &ff_rl_mpeg1;
    uint8_t * const scantable= s->intra_scantable.permutated;
    const int qscale= s->qscale;
    OPEN_READER(re, &s->gb);
    i = -1;

    // special case for first coefficient, no need to add second VLC table
    UPDATE_CACHE(re, &s->gb);
    if (((int32_t)GET_CACHE(re, &s->gb)) < 0) {
        level= (3*qscale)>>1;
        if(GET_CACHE(re, &s->gb)&0x40000000)
            level= -level;
        block[0] = level;
        i++;
        SKIP_BITS(re, &s->gb, 2);
        if(((int32_t)GET_CACHE(re, &s->gb)) <= (int32_t)0xBFFFFFFF)
            goto end;
    }
#if MIN_CACHE_BITS < 19
    UPDATE_CACHE(re, &s->gb);
#endif

    /* now quantify & encode AC coefficients */
    for(;;) {
        GET_RL_VLC(level, run, re, &s->gb, rl->rl_vlc[0], TEX_VLC_BITS, 2, 0);

        if(level != 0) {
            i += run;
            j = scantable[i];
            level= ((level*2+1)*qscale)>>1;
            level = (level ^ SHOW_SBITS(re, &s->gb, 1)) - SHOW_SBITS(re, &s->gb, 1);
            SKIP_BITS(re, &s->gb, 1);
        } else {
            /* escape */
            run = SHOW_UBITS(re, &s->gb, 6)+1; LAST_SKIP_BITS(re, &s->gb, 6);
            UPDATE_CACHE(re, &s->gb);
            level = SHOW_SBITS(re, &s->gb, 12); SKIP_BITS(re, &s->gb, 12);

            i += run;
            j = scantable[i];
            if(level<0){
                level= ((-level*2+1)*qscale)>>1;
                level= -level;
            }else{
                level= ((level*2+1)*qscale)>>1;
            }
        }

        block[j] = level;
#if MIN_CACHE_BITS < 19
        UPDATE_CACHE(re, &s->gb);
#endif
        if(((int32_t)GET_CACHE(re, &s->gb)) <= (int32_t)0xBFFFFFFF)
            break;
#if MIN_CACHE_BITS >=19
        UPDATE_CACHE(re, &s->gb);
#endif
    }
end:
    LAST_SKIP_BITS(re, &s->gb, 2);
    CLOSE_READER(re, &s->gb);
    s->block_last_index[n] = i;
    return 0;
}


static inline int mpeg2_decode_block_intra_hw(MpegEncContext *s,
                               DCTELEM *block,
                               int n)
{
    int level, dc, diff, i, j, run;
    int component;
    RLTable *rl;

    uint8_t const * scantable = s->intra_scantable.permutated; 
    //const uint16_t *quant_matrix;
    //const int qscale = s->qscale;
    //    int mismatch;
    /* DC coefficient */

    if (n < 4){
      //quant_matrix = s->intra_matrix;
        component = 0;
    }else{
      //quant_matrix = s->chroma_intra_matrix;
        component = (n&1) + 1;
    }
    diff = decode_dc(&s->gb, component);
    if (diff >= 0xffff)
        return -1;

    dc = s->last_dc[component];
    dc += diff;
    s->last_dc[component] = dc ;
    block[0] = (dc << (3-s->intra_dc_precision)) << 4; //-- here need * 16
    //mismatch = block[0] ^ 1;
    i = 0;
    if (s->intra_vlc_format)
        rl = &ff_rl_mpeg2;
    else
        rl = &ff_rl_mpeg1;
    {
        OPEN_READER(re, &s->gb);
        // now quantify & calculate AC coefficients
        for(;;) {
            UPDATE_CACHE(re, &s->gb);
            GET_RL_VLC(level, run, re, &s->gb, rl->rl_vlc[0], TEX_VLC_BITS, 2, 0);
            if(level == 127){
                break;
            } else if(level != 0) {
                i += run;
                j = scantable[i];
                /////change order   
                level = (level ^ SHOW_SBITS(re, &s->gb, 1)) - SHOW_SBITS(re, &s->gb, 1); 
                //level = (level*qscale*quant_matrix[j])>>4;
                LAST_SKIP_BITS(re, &s->gb, 1);
            } else {
	      // escape 
                run = SHOW_UBITS(re, &s->gb, 6)+1; LAST_SKIP_BITS(re, &s->gb, 6);
                UPDATE_CACHE(re, &s->gb);
                level = SHOW_SBITS(re, &s->gb, 12); 
                //////////////////////////////////////
                SKIP_BITS(re, &s->gb, 12);
                i += run;
                j = scantable[i];
                /*
                if(level<0){
		  level= (-level*qscale*quant_matrix[j])>>4;
		  level= -level;
                }else{
		  level= (level*qscale*quant_matrix[j])>>4;
		  }*/
            }
            if (i > 63){
                av_log(s->avctx, AV_LOG_ERROR, "ac-tex damaged at %d %d\n", s->mb_x, s->mb_y);
                return -1;
            }
            //mismatch^= level;
            block[j] = level;
        } 
        CLOSE_READER(re, &s->gb);
    }
    //block[63]^= mismatch&1;
    s->block_last_index[n] = i;
    return 0;
}

static inline int mpeg2_decode_block_intra(MpegEncContext *s,
                               DCTELEM *block,
                               int n)
{
  if(s->progressive_sequence)
    return mpeg2_decode_block_intra_hw(s, block, n);

    int level, dc, diff, i, j, run;
    int component;
    RLTable *rl;
    uint8_t * const scantable= s->intra_scantable.permutated;
    const uint16_t *quant_matrix;
    const int qscale= s->qscale;
    int mismatch;

    /* DC coefficient */
    if (n < 4){
        quant_matrix = s->intra_matrix;
        component = 0;
    }else{
        quant_matrix = s->chroma_intra_matrix;
        component = (n&1) + 1;
    }
    diff = decode_dc(&s->gb, component);
    if (diff >= 0xffff)
        return -1;
    dc = s->last_dc[component];
    dc += diff;
    s->last_dc[component] = dc;
    block[0] = dc << (3 - s->intra_dc_precision);
    dprintf(s->avctx, "dc=%d\n", block[0]);
    mismatch = block[0] ^ 1;
    i = 0;
    if (s->intra_vlc_format)
        rl = &ff_rl_mpeg2;
    else
        rl = &ff_rl_mpeg1;

    {
        OPEN_READER(re, &s->gb);
        /* now quantify & encode AC coefficients */
        for(;;) {
            UPDATE_CACHE(re, &s->gb);
            GET_RL_VLC(level, run, re, &s->gb, rl->rl_vlc[0], TEX_VLC_BITS, 2, 0);

            if(level == 127){
                break;
            } else if(level != 0) {
                i += run;
                j = scantable[i];
                level= (level*qscale*quant_matrix[j])>>4;
                level = (level ^ SHOW_SBITS(re, &s->gb, 1)) - SHOW_SBITS(re, &s->gb, 1);
                LAST_SKIP_BITS(re, &s->gb, 1);
            } else {
                /* escape */
                run = SHOW_UBITS(re, &s->gb, 6)+1; LAST_SKIP_BITS(re, &s->gb, 6);
                UPDATE_CACHE(re, &s->gb);
                level = SHOW_SBITS(re, &s->gb, 12); SKIP_BITS(re, &s->gb, 12);
                i += run;
                j = scantable[i];
                if(level<0){
                    level= (-level*qscale*quant_matrix[j])>>4;
                    level= -level;
                }else{
                    level= (level*qscale*quant_matrix[j])>>4;
                }
            }
            if (i > 63){
                av_log(s->avctx, AV_LOG_ERROR, "ac-tex damaged at %d %d\n", s->mb_x, s->mb_y);
                return -1;
            }

            mismatch^= level;
            block[j] = level;
        }
        CLOSE_READER(re, &s->gb);
    }
    block[63]^= mismatch&1;

    s->block_last_index[n] = i;
    return 0;
}

static inline int mpeg2_fast_decode_block_intra(MpegEncContext *s,
                               DCTELEM *block,
                               int n)
{
    int level, dc, diff, j, run;
    int component;
    RLTable *rl;
    uint8_t * scantable= s->intra_scantable.permutated;
    const uint16_t *quant_matrix;
    const int qscale= s->qscale;

    /* DC coefficient */
    if (n < 4){
        quant_matrix = s->intra_matrix;
        component = 0;
    }else{
        quant_matrix = s->chroma_intra_matrix;
        component = (n&1) + 1;
    }
    diff = decode_dc(&s->gb, component);
    if (diff >= 0xffff)
        return -1;
    dc = s->last_dc[component];
    dc += diff;
    s->last_dc[component] = dc;
    block[0] = dc << (3 - s->intra_dc_precision);
    if (s->intra_vlc_format)
        rl = &ff_rl_mpeg2;
    else
        rl = &ff_rl_mpeg1;

    {
        OPEN_READER(re, &s->gb);
        /* now quantify & encode AC coefficients */
        for(;;) {
            UPDATE_CACHE(re, &s->gb);
            GET_RL_VLC(level, run, re, &s->gb, rl->rl_vlc[0], TEX_VLC_BITS, 2, 0);

            if(level == 127){
                break;
            } else if(level != 0) {
                scantable += run;
                j = *scantable;
                level= (level*qscale*quant_matrix[j])>>4;
                level = (level ^ SHOW_SBITS(re, &s->gb, 1)) - SHOW_SBITS(re, &s->gb, 1);
                LAST_SKIP_BITS(re, &s->gb, 1);
            } else {
                /* escape */
                run = SHOW_UBITS(re, &s->gb, 6)+1; LAST_SKIP_BITS(re, &s->gb, 6);
                UPDATE_CACHE(re, &s->gb);
                level = SHOW_SBITS(re, &s->gb, 12); SKIP_BITS(re, &s->gb, 12);
                scantable += run;
                j = *scantable;
                if(level<0){
                    level= (-level*qscale*quant_matrix[j])>>4;
                    level= -level;
                }else{
                    level= (level*qscale*quant_matrix[j])>>4;
                }
            }

            block[j] = level;
        }
        CLOSE_READER(re, &s->gb);
    }

    s->block_last_index[n] = scantable - s->intra_scantable.permutated;
    
    return 0;
}

/* extern int use_jz_buf:
 *   When set to 1, the output format will be linear display. 
 *   Note: 1. add the ID to libmpcodec/vd_ffmpeg.c in 'video_line' option.
 *         2. the data[0,1,2] space is big enough, so feel comfortable to merge uv into data[1].
 */
extern int use_jz_buf; 
extern int use_jz_buf_change; 

typedef struct Mpeg1Context {
    MpegEncContext mpeg_enc_ctx;
    int mpeg_enc_ctx_allocated; /* true if decoding context allocated */
    int repeat_field; /* true if we must repeat the field */
    AVPanScan pan_scan;              /**< some temporary storage for the panscan */
    int slice_count;
    int swap_uv;//indicate VCR2
    int save_aspect_info;
    int save_width, save_height, save_progressive_seq;
    AVRational frame_rate_ext;       ///< MPEG-2 specific framerate modificator
    int sync;                        ///< Did we reach a sync point like a GOP/SEQ/KEYFrame?
} Mpeg1Context;


static av_cold int mpeg_decode_init(AVCodecContext *avctx)
{
    Mpeg1Context *s = avctx->priv_data;
    MpegEncContext *s2 = &s->mpeg_enc_ctx;
    int i;

    S32I2M(xr16, 0x7);//-- Enable MXU 
    use_jz_buf = 1; //-- video linear display
    use_jz_buf_change = 0;

    /* we need some permutation to store matrices,
     * until MPV_common_init() sets the real permutation. */
    for(i=0;i<64;i++)
       s2->dsp.idct_permutation[i]=i;

    MPV_decode_defaults(s2);

    s->mpeg_enc_ctx.avctx= avctx;
    s->mpeg_enc_ctx.flags= avctx->flags;
    s->mpeg_enc_ctx.flags2= avctx->flags2;
    ff_mpeg12_common_init(&s->mpeg_enc_ctx);
    ffmpeg2_init_vlcs();

    s->mpeg_enc_ctx_allocated = 0;
    s->mpeg_enc_ctx.picture_number = 0;
    s->repeat_field = 0;
    s->mpeg_enc_ctx.codec_id= avctx->codec->id;
    avctx->color_range= AVCOL_RANGE_MPEG;
    if (avctx->codec->id == CODEC_ID_MPEG1VIDEO)
        avctx->chroma_sample_location = AVCHROMA_LOC_CENTER;
    else
        avctx->chroma_sample_location = AVCHROMA_LOC_LEFT;

    /* -------------------- prin info ------------------------ */
    //av_log(NULL, AV_LOG_WARNING, "++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    //av_log(NULL, AV_LOG_WARNING, "libffmpeg2\n");
    //av_log(NULL, AV_LOG_WARNING, "++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

    /* -------------------- Init Aux ------------------------- */
#ifdef JZC_DCORE_OPT
    tcsm_init();
#endif

#ifdef JZC_DCORE_OPT
    int * tmp_hm_buf = av_malloc(SPACE_HALF_MILLION_BYTE);
    if (tmp_hm_buf < 0)
      mp_msg(NULL,NULL,"JZ4740 ALLOC tmp_hm_buf ERROR !! \n");
    AUX_RESET();
    //    int i;
    *((volatile int *)(TCSM0_P1_TASK_DONE)) = 0;

#ifdef ANDROID
    if(loadfile("ffmpeg2_p1.bin", (int *)TCSM1_VCADDR(VC1_P1_MAIN), SPACE_HALF_MILLION_BYTE, 1) == -1){
      mp_msg(NULL,NULL,"LOAD MPEG2_P1_BIN ERROR.....................\n");
      return -1;
    }
#else
    FILE *fp_text;
    int len, *reserved_mem;
    fp_text = fopen("./ffmpeg2_p1.bin", "r+b");
    if (!fp_text)
      mp_msg(NULL, NULL, " error while open mpeg2_p1.bin \n");
    int *load_buf = tmp_hm_buf;
    len = fread(load_buf, 4, SPACE_HALF_MILLION_BYTE, fp_text);
    mp_msg(NULL,NULL," mpeg2 len of p1 task = %d\n",len);

    reserved_mem = (int *)TCSM1_VCADDR(MPEG2_P1_MAIN);
    for(i=0; i<len; i++)
      reserved_mem[i] = load_buf[i];

    fclose(fp_text);
#endif
    av_free(tmp_hm_buf);
    jz_dcache_wb(); /*flush cache into reserved mem*/
    i_sync();

#endif    
    mbnum_wp = (volatile int *)TCSM1_VCADDR(TCSM1_MBNUM_WP);
    mbnum_rp = (volatile int *)TCSM1_VCADDR(TCSM1_MBNUM_RP);
    unkown_block = (volatile int *)TCSM1_VCADDR(UNKNOWN_BLOCK);
    *unkown_block = 0;
    frame_num = 0;
    wait = 0;
    return 0;
}

static void quant_matrix_rebuild(uint16_t *matrix, const uint8_t *old_perm,
                                     const uint8_t *new_perm){
    uint16_t temp_matrix[64];
    int i;

    memcpy(temp_matrix,matrix,64*sizeof(uint16_t));

    for(i=0;i<64;i++){
        matrix[new_perm[i]] = temp_matrix[old_perm[i]];
    }
}

static enum PixelFormat mpeg_get_pixelformat(AVCodecContext *avctx){
    Mpeg1Context *s1 = avctx->priv_data;
    MpegEncContext *s = &s1->mpeg_enc_ctx;

    if(avctx->xvmc_acceleration)
        return avctx->get_format(avctx,pixfmt_xvmc_mpg2_420);
    else if(avctx->codec->capabilities&CODEC_CAP_HWACCEL_VDPAU){
        if(avctx->codec_id == CODEC_ID_MPEG1VIDEO)
            return PIX_FMT_VDPAU_MPEG1;
        else
            return PIX_FMT_VDPAU_MPEG2;
    }else{
        if(s->chroma_format <  2)
            return avctx->get_format(avctx,ff_hwaccel_pixfmt_list_420);
        else if(s->chroma_format == 2)
            return PIX_FMT_YUV422P;
        else
            return PIX_FMT_YUV444P;
    }
}

/* Call this function when we know all parameters.
 * It may be called in different places for MPEG-1 and MPEG-2. */
static int mpeg_decode_postinit(AVCodecContext *avctx){
    Mpeg1Context *s1 = avctx->priv_data;
    MpegEncContext *s = &s1->mpeg_enc_ctx;
    uint8_t old_permutation[64];

    if (
        (s1->mpeg_enc_ctx_allocated == 0)||
        avctx->coded_width  != s->width ||
        avctx->coded_height != s->height||
        s1->save_width != s->width ||
        s1->save_height != s->height ||
        s1->save_aspect_info != s->aspect_ratio_info||
        s1->save_progressive_seq != s->progressive_sequence ||
        0)
    {

        if (s1->mpeg_enc_ctx_allocated) {
            ParseContext pc= s->parse_context;
            s->parse_context.buffer=0;
            MPV_common_end(s);
            s->parse_context= pc;
        }

        if( (s->width == 0 )||(s->height == 0))
            return -2;

        avcodec_set_dimensions(avctx, s->width, s->height);
        avctx->bit_rate = s->bit_rate;
        s1->save_aspect_info = s->aspect_ratio_info;
        s1->save_width = s->width;
        s1->save_height = s->height;
        s1->save_progressive_seq = s->progressive_sequence;

        /* low_delay may be forced, in this case we will have B-frames
         * that behave like P-frames. */
        avctx->has_b_frames = !(s->low_delay);

        assert((avctx->sub_id==1) == (avctx->codec_id==CODEC_ID_MPEG1VIDEO));
        if(avctx->codec_id==CODEC_ID_MPEG1VIDEO){
            //MPEG-1 fps
            avctx->time_base.den= ff_frame_rate_tab[s->frame_rate_index].num;
            avctx->time_base.num= ff_frame_rate_tab[s->frame_rate_index].den;
            //MPEG-1 aspect
            avctx->sample_aspect_ratio= av_d2q(
                    1.0/ff_mpeg1_aspect[s->aspect_ratio_info], 255);
            avctx->ticks_per_frame=1;
        }else{//MPEG-2
        //MPEG-2 fps
            av_reduce(
                &s->avctx->time_base.den,
                &s->avctx->time_base.num,
                ff_frame_rate_tab[s->frame_rate_index].num * s1->frame_rate_ext.num*2,
                ff_frame_rate_tab[s->frame_rate_index].den * s1->frame_rate_ext.den,
                1<<30);
            avctx->ticks_per_frame=2;
        //MPEG-2 aspect
            if(s->aspect_ratio_info > 1){
                //we ignore the spec here as reality does not match the spec, see for example
                // res_change_ffmpeg_aspect.ts and sequence-display-aspect.mpg
                if( (s1->pan_scan.width == 0 )||(s1->pan_scan.height == 0) || 1){
                    s->avctx->sample_aspect_ratio=
                        av_div_q(
                         ff_mpeg2_aspect[s->aspect_ratio_info],
                         (AVRational){s->width, s->height}
                         );
                }else{
                    s->avctx->sample_aspect_ratio=
                        av_div_q(
                         ff_mpeg2_aspect[s->aspect_ratio_info],
                         (AVRational){s1->pan_scan.width, s1->pan_scan.height}
                        );
                }
            }else{
                s->avctx->sample_aspect_ratio=
                    ff_mpeg2_aspect[s->aspect_ratio_info];
            }
        }//MPEG-2

        avctx->pix_fmt = mpeg_get_pixelformat(avctx);
        avctx->hwaccel = ff_find_hwaccel(avctx->codec->id, avctx->pix_fmt);
        //until then pix_fmt may be changed right after codec init
        if( avctx->pix_fmt == PIX_FMT_XVMC_MPEG2_IDCT ||
            avctx->hwaccel ||
            s->avctx->codec->capabilities&CODEC_CAP_HWACCEL_VDPAU )
            if( avctx->idct_algo == FF_IDCT_AUTO )
                avctx->idct_algo = FF_IDCT_SIMPLE;

        /* Quantization matrices may need reordering
         * if DCT permutation is changed. */
        memcpy(old_permutation,s->dsp.idct_permutation,64*sizeof(uint8_t));

        if (MPV_common_init(s) < 0)
            return -2;

        quant_matrix_rebuild(s->intra_matrix,       old_permutation,s->dsp.idct_permutation);
        quant_matrix_rebuild(s->inter_matrix,       old_permutation,s->dsp.idct_permutation);
        quant_matrix_rebuild(s->chroma_intra_matrix,old_permutation,s->dsp.idct_permutation);
        quant_matrix_rebuild(s->chroma_inter_matrix,old_permutation,s->dsp.idct_permutation);

        s1->mpeg_enc_ctx_allocated = 1;
    }
    return 0;
}

static int mpeg1_decode_picture(AVCodecContext *avctx,
                                const uint8_t *buf, int buf_size)
{
    Mpeg1Context *s1 = avctx->priv_data;
    MpegEncContext *s = &s1->mpeg_enc_ctx;
    int ref, f_code, vbv_delay;

    init_get_bits(&s->gb, buf, buf_size*8);

    ref = get_bits(&s->gb, 10); /* temporal ref */
    s->pict_type = get_bits(&s->gb, 3);
    if(s->pict_type == 0 || s->pict_type > 3)
        return -1;

    vbv_delay= get_bits(&s->gb, 16);
    if (s->pict_type == FF_P_TYPE || s->pict_type == FF_B_TYPE) {
        s->full_pel[0] = get_bits1(&s->gb);
        f_code = get_bits(&s->gb, 3);
        if (f_code == 0 && avctx->error_recognition >= FF_ER_COMPLIANT)
            return -1;
        s->mpeg_f_code[0][0] = f_code;
        s->mpeg_f_code[0][1] = f_code;
    }
    if (s->pict_type == FF_B_TYPE) {
        s->full_pel[1] = get_bits1(&s->gb);
        f_code = get_bits(&s->gb, 3);
        if (f_code == 0 && avctx->error_recognition >= FF_ER_COMPLIANT)
            return -1;
        s->mpeg_f_code[1][0] = f_code;
        s->mpeg_f_code[1][1] = f_code;
    }
    s->current_picture.pict_type= s->pict_type;
    s->current_picture.key_frame= s->pict_type == FF_I_TYPE;

    if(avctx->debug & FF_DEBUG_PICT_INFO)
        av_log(avctx, AV_LOG_DEBUG, "vbv_delay %d, ref %d type:%d\n", vbv_delay, ref, s->pict_type);

    s->y_dc_scale = 8;
    s->c_dc_scale = 8;
    return 0;
}

static void mpeg_decode_sequence_extension(Mpeg1Context *s1)
{
    MpegEncContext *s= &s1->mpeg_enc_ctx;
    int horiz_size_ext, vert_size_ext;
    int bit_rate_ext;

    skip_bits(&s->gb, 1); /* profile and level esc*/
    s->avctx->profile= get_bits(&s->gb, 3);
    s->avctx->level= get_bits(&s->gb, 4);
    s->progressive_sequence = get_bits1(&s->gb); /* progressive_sequence */

    use_jz_buf = s->progressive_sequence;
    if(!use_jz_buf && frame_num == 1)
      use_jz_buf_change = 1;

    s->chroma_format = get_bits(&s->gb, 2); /* chroma_format 1=420, 2=422, 3=444 */
    horiz_size_ext = get_bits(&s->gb, 2);
    vert_size_ext = get_bits(&s->gb, 2);
    s->width |= (horiz_size_ext << 12);
    s->height |= (vert_size_ext << 12);
    bit_rate_ext = get_bits(&s->gb, 12);  /* XXX: handle it */
    s->bit_rate += (bit_rate_ext << 18) * 400;
    skip_bits1(&s->gb); /* marker */
    s->avctx->rc_buffer_size += get_bits(&s->gb, 8)*1024*16<<10;

    s->low_delay = get_bits1(&s->gb);
    if(s->flags & CODEC_FLAG_LOW_DELAY) s->low_delay=1;

    s1->frame_rate_ext.num = get_bits(&s->gb, 2)+1;
    s1->frame_rate_ext.den = get_bits(&s->gb, 5)+1;

    dprintf(s->avctx, "sequence extension\n");
    s->codec_id= s->avctx->codec_id= CODEC_ID_MPEG2VIDEO;
    s->avctx->sub_id = 2; /* indicates MPEG-2 found */

    if(s->avctx->debug & FF_DEBUG_PICT_INFO)
        av_log(s->avctx, AV_LOG_DEBUG, "profile: %d, level: %d vbv buffer: %d, bitrate:%d\n",
               s->avctx->profile, s->avctx->level, s->avctx->rc_buffer_size, s->bit_rate);

}

static void mpeg_decode_sequence_display_extension(Mpeg1Context *s1)
{
    MpegEncContext *s= &s1->mpeg_enc_ctx;
    int color_description, w, h;

    skip_bits(&s->gb, 3); /* video format */
    color_description= get_bits1(&s->gb);
    if(color_description){
        s->avctx->color_primaries= get_bits(&s->gb, 8);
        s->avctx->color_trc      = get_bits(&s->gb, 8);
        s->avctx->colorspace     = get_bits(&s->gb, 8);
    }
    w= get_bits(&s->gb, 14);
    skip_bits(&s->gb, 1); //marker
    h= get_bits(&s->gb, 14);
    // remaining 3 bits are zero padding

    s1->pan_scan.width= 16*w;
    s1->pan_scan.height=16*h;

    if(s->avctx->debug & FF_DEBUG_PICT_INFO)
        av_log(s->avctx, AV_LOG_DEBUG, "sde w:%d, h:%d\n", w, h);
}

static void mpeg_decode_picture_display_extension(Mpeg1Context *s1)
{
    MpegEncContext *s= &s1->mpeg_enc_ctx;
    int i,nofco;

    nofco = 1;
    if(s->progressive_sequence){
        if(s->repeat_first_field){
            nofco++;
            if(s->top_field_first)
                nofco++;
        }
    }else{
        if(s->picture_structure == PICT_FRAME){
            nofco++;
            if(s->repeat_first_field)
                nofco++;
        }
    }
    for(i=0; i<nofco; i++){
        s1->pan_scan.position[i][0]= get_sbits(&s->gb, 16);
        skip_bits(&s->gb, 1); //marker
        s1->pan_scan.position[i][1]= get_sbits(&s->gb, 16);
        skip_bits(&s->gb, 1); //marker
    }

    if(s->avctx->debug & FF_DEBUG_PICT_INFO)
        av_log(s->avctx, AV_LOG_DEBUG, "pde (%d,%d) (%d,%d) (%d,%d)\n",
            s1->pan_scan.position[0][0], s1->pan_scan.position[0][1],
            s1->pan_scan.position[1][0], s1->pan_scan.position[1][1],
            s1->pan_scan.position[2][0], s1->pan_scan.position[2][1]
        );
}

static int load_matrix(MpegEncContext *s, uint16_t matrix0[64], uint16_t matrix1[64], int intra){
    int i;

    for(i=0; i<64; i++) {
        int j = s->dsp.idct_permutation[ ff_zigzag_direct[i] ];
        int v = get_bits(&s->gb, 8);
        if(v==0){
            av_log(s->avctx, AV_LOG_ERROR, "matrix damaged\n");
            return -1;
        }
        if(intra && i==0 && v!=8){
            av_log(s->avctx, AV_LOG_ERROR, "intra matrix invalid, ignoring\n");
            v= 8; // needed by pink.mpg / issue1046
        }
        matrix0[j] = v;
        if(matrix1)
            matrix1[j] = v;
    }
    return 0;
}

static void mpeg_decode_quant_matrix_extension(MpegEncContext *s)
{
    dprintf(s->avctx, "matrix extension\n");

    if(get_bits1(&s->gb)) load_matrix(s, s->chroma_intra_matrix, s->intra_matrix, 1);
    if(get_bits1(&s->gb)) load_matrix(s, s->chroma_inter_matrix, s->inter_matrix, 0);
    if(get_bits1(&s->gb)) load_matrix(s, s->chroma_intra_matrix, NULL           , 1);
    if(get_bits1(&s->gb)) load_matrix(s, s->chroma_inter_matrix, NULL           , 0);
}

static void mpeg_decode_picture_coding_extension(Mpeg1Context *s1)
{
    MpegEncContext *s= &s1->mpeg_enc_ctx;

    s->full_pel[0] = s->full_pel[1] = 0;
    s->mpeg_f_code[0][0] = get_bits(&s->gb, 4);
    s->mpeg_f_code[0][1] = get_bits(&s->gb, 4);
    s->mpeg_f_code[1][0] = get_bits(&s->gb, 4);
    s->mpeg_f_code[1][1] = get_bits(&s->gb, 4);
    if(!s->pict_type && s1->mpeg_enc_ctx_allocated){
        av_log(s->avctx, AV_LOG_ERROR, "Missing picture start code, guessing missing values\n");
        if(s->mpeg_f_code[1][0] == 15 && s->mpeg_f_code[1][1]==15){
            if(s->mpeg_f_code[0][0] == 15 && s->mpeg_f_code[0][1] == 15)
                s->pict_type= FF_I_TYPE;
            else
                s->pict_type= FF_P_TYPE;
        }else
            s->pict_type= FF_B_TYPE;
        s->current_picture.pict_type= s->pict_type;
        s->current_picture.key_frame= s->pict_type == FF_I_TYPE;
    }
    s->intra_dc_precision = get_bits(&s->gb, 2);
    s->picture_structure = get_bits(&s->gb, 2);
    s->top_field_first = get_bits1(&s->gb);
    s->frame_pred_frame_dct = get_bits1(&s->gb);
    s->concealment_motion_vectors = get_bits1(&s->gb);
    s->q_scale_type = get_bits1(&s->gb);
    s->intra_vlc_format = get_bits1(&s->gb);
    s->alternate_scan = get_bits1(&s->gb);
    s->repeat_first_field = get_bits1(&s->gb);
    s->chroma_420_type = get_bits1(&s->gb);
    s->progressive_frame = get_bits1(&s->gb);

    if(s->progressive_sequence && !s->progressive_frame){
        s->progressive_frame= 1;
        av_log(s->avctx, AV_LOG_ERROR, "interlaced frame in progressive sequence, ignoring\n");
    }

    if(s->picture_structure==0 || (s->progressive_frame && s->picture_structure!=PICT_FRAME)){
        av_log(s->avctx, AV_LOG_ERROR, "picture_structure %d invalid, ignoring\n", s->picture_structure);
        s->picture_structure= PICT_FRAME;
    }

    if(s->progressive_sequence && !s->frame_pred_frame_dct){
        av_log(s->avctx, AV_LOG_ERROR, "invalid frame_pred_frame_dct\n");
        s->frame_pred_frame_dct= 1;
    }

    if(s->picture_structure == PICT_FRAME){
        s->first_field=0;
        s->v_edge_pos= 16*s->mb_height;
    }else{
        s->first_field ^= 1;
        s->v_edge_pos=  8*s->mb_height;
        memset(s->mbskip_table, 0, s->mb_stride*s->mb_height);
    }

    if(s->alternate_scan){
        ff_init_scantable(s->dsp.idct_permutation, &s->inter_scantable  , ff_alternate_vertical_scan);
        ff_init_scantable(s->dsp.idct_permutation, &s->intra_scantable  , ff_alternate_vertical_scan);
    }else{
        ff_init_scantable(s->dsp.idct_permutation, &s->inter_scantable  , ff_zigzag_direct);
        ff_init_scantable(s->dsp.idct_permutation, &s->intra_scantable  , ff_zigzag_direct);
    }

    /* composite display not parsed */
    dprintf(s->avctx, "intra_dc_precision=%d\n", s->intra_dc_precision);
    dprintf(s->avctx, "picture_structure=%d\n", s->picture_structure);
    dprintf(s->avctx, "top field first=%d\n", s->top_field_first);
    dprintf(s->avctx, "repeat first field=%d\n", s->repeat_first_field);
    dprintf(s->avctx, "conceal=%d\n", s->concealment_motion_vectors);
    dprintf(s->avctx, "intra_vlc_format=%d\n", s->intra_vlc_format);
    dprintf(s->avctx, "alternate_scan=%d\n", s->alternate_scan);
    dprintf(s->avctx, "frame_pred_frame_dct=%d\n", s->frame_pred_frame_dct);
    dprintf(s->avctx, "progressive_frame=%d\n", s->progressive_frame);
}

static void exchange_uv(MpegEncContext *s){
    DCTELEM (*tmp)[64];

    tmp           = s->pblocks[4];
    s->pblocks[4] = s->pblocks[5];
    s->pblocks[5] = tmp;
}

static int mpeg_field_start(MpegEncContext *s, const uint8_t *buf, int buf_size){
    AVCodecContext *avctx= s->avctx;
    Mpeg1Context *s1 = (Mpeg1Context*)s;

    /* start frame decoding */
    if(s->first_field || s->picture_structure==PICT_FRAME){
        if(MPV_frame_start(s, avctx) < 0)
            return -1;

        ff_er_frame_start(s);

        /* first check if we must repeat the frame */
        s->current_picture_ptr->repeat_pict = 0;
        if (s->repeat_first_field) {
            if (s->progressive_sequence) {
                if (s->top_field_first)
                    s->current_picture_ptr->repeat_pict = 4;
                else
                    s->current_picture_ptr->repeat_pict = 2;
            } else if (s->progressive_frame) {
                s->current_picture_ptr->repeat_pict = 1;
            }
        }

        *s->current_picture_ptr->pan_scan= s1->pan_scan;
    }else{ //second field
            int i;

            if(!s->current_picture_ptr){
                av_log(s->avctx, AV_LOG_ERROR, "first field missing\n");
                return -1;
            }

            for(i=0; i<4; i++){
                s->current_picture.data[i] = s->current_picture_ptr->data[i];
                if(s->picture_structure == PICT_BOTTOM_FIELD){
                    s->current_picture.data[i] += s->current_picture_ptr->linesize[i];
                }
            }
    }

    if (avctx->hwaccel) {
        if (avctx->hwaccel->start_frame(avctx, buf, buf_size) < 0)
            return -1;
    }

// MPV_frame_start will call this function too,
// but we need to call it on every field
    if(CONFIG_MPEG_XVMC_DECODER && s->avctx->xvmc_acceleration)
        if(ff_xvmc_field_start(s,avctx) < 0)
            return -1;

    return 0;
}

#define DECODE_SLICE_ERROR -1
#define DECODE_SLICE_OK 0

/* crc check used for correct data of s->picture.data */
static void crc_check(MpegEncContext * s, int mbnumber)
{
  int i, j;
  uint8_t * src_y = s->current_picture.data[0] + mbnumber * 256;
  uint8_t * src_c = s->current_picture.data[1] + mbnumber * 128;

  av_log(NULL, AV_LOG_WARNING, "blocks : %d\n", mbnumber);
  av_log(NULL, AV_LOG_WARNING, "----------------------- Y block -----------------------\n");
  for(i = 0; i < 16; i++){
    for(j = 0; j < 16; j++)
      av_log(NULL, AV_LOG_WARNING, "%2x ", src_y[i*16 + j]);
    av_log(NULL, AV_LOG_WARNING, "\n");
  }
  av_log(NULL, AV_LOG_WARNING, "----------------------- UV block -----------------------\n");
  for(i = 0; i < 8; i++){
    for(j = 0; j < 8; j++)
      av_log(NULL, AV_LOG_WARNING, "%2x ", src_c[i*16 + j]);
    av_log(NULL, AV_LOG_WARNING, "** ");
    for( ; j < 16; j++)
      av_log(NULL, AV_LOG_WARNING, "%2x ", src_c[i*16 + j]);
    av_log(NULL, AV_LOG_WARNING, "\n");
  }
  av_log(NULL, AV_LOG_WARNING, "-------------------------- end --------------------------\n");
}

/**
 * decodes a slice. MpegEncContext.mb_y must be set to the MB row from the startcode
 * @return DECODE_SLICE_ERROR if the slice is damaged<br>
 *         DECODE_SLICE_OK if this slice is ok<br>
 */
static int mpeg_decode_slice(Mpeg1Context *s1,  int mb_y,
                             const uint8_t **buf, int buf_size)
{
    MpegEncContext *s = &s1->mpeg_enc_ctx;
    AVCodecContext *avctx= s->avctx;
    const int field_pic= s->picture_structure != PICT_FRAME;
    const int lowres= s->avctx->lowres;
    int tmp;

    /* init while decoding every slice */
    s->resync_mb_x=
    s->resync_mb_y= -1;

    assert(mb_y < s->mb_height);

    init_get_bits(&s->gb, *buf, buf_size*8);

    ff_mpeg1_clean_buffers(s);
    s->interlaced_dct = 0;

    s->qscale = get_qscale(s);

    if(s->qscale == 0){
        av_log(s->avctx, AV_LOG_ERROR, "qscale == 0\n");
        return -1;
    }

    /* extra slice info */
    while (get_bits1(&s->gb) != 0) {
        skip_bits(&s->gb, 8);
    }

    s->mb_x=0;

    if(mb_y==0 && s->codec_tag == AV_RL32("SLIF")){
        skip_bits1(&s->gb);
    }else{
      /* search first mb in a slice */
        for(;;) {
            int code = get_vlc2(&s->gb, mbincr_vlc.table, MBINCR_VLC_BITS, 2);
            if (code < 0){
                av_log(s->avctx, AV_LOG_ERROR, "first mb_incr damaged\n");
                return -1;
            }
            if (code >= 33) {
                if (code == 33) {
                    s->mb_x += 33;
                }
                /* otherwise, stuffing, nothing to do */
            } else {
                s->mb_x += code;
                break;
            }
        }
    }

    if(s->mb_x >= (unsigned)s->mb_width){
        av_log(s->avctx, AV_LOG_ERROR, "initial skip overflow\n");
        return -1;
    }

#if 0
    /* if we have the hardware accel, then we could use it instead of software below */
    if (avctx->hwaccel) {
        const uint8_t *buf_end, *buf_start = *buf - 4; /* include start_code */
        int start_code = -1;
        buf_end = ff_find_start_code(buf_start + 2, *buf + buf_size, &start_code);
        if (buf_end < *buf + buf_size)
            buf_end -= 4;
        s->mb_y = mb_y;
        if (avctx->hwaccel->decode_slice(avctx, buf_start, buf_end - buf_start) < 0)
            return DECODE_SLICE_ERROR;
        *buf = buf_end;
        return DECODE_SLICE_OK;
    }
#endif//

    /* software decode slice when we have no hardware */
    s->resync_mb_x= s->mb_x;
    s->resync_mb_y= s->mb_y= mb_y;
    s->mb_skip_run= 0;
    //here s->dest init, s->dest is a continuous buffer used for storing current picture's pixels.
    ff_init_block_index(s); 

#ifdef JZC_DCORE_OPT
    volatile unsigned int tcsm1_fifo_rp_lh = 0;
    volatile unsigned int task_fifo_wp_lh = 0;
    volatile unsigned int task_fifo_wp_lh_overlap = 0;
    int reach_task_end = 0;
#endif

    for(;;) {

        if(s->progressive_sequence){
          do{
            tcsm1_fifo_rp_lh = (volatile unsigned int)(*tcsm1_fifo_rp) & 0x0000FFFF;
            task_fifo_wp_lh  = (volatile unsigned int)task_fifo_wp & 0x0000FFFF;
            task_fifo_wp_lh_overlap = ((volatile unsigned int)task_fifo_wp + TASK_BUF_LEN) & 0x0000FFFF;
          }while( !(task_fifo_wp_lh_overlap <= tcsm1_fifo_rp_lh || task_fifo_wp_lh > tcsm1_fifo_rp_lh || (task_fifo_wp_lh == tcsm1_fifo_rp_lh && *mbnum_wp == *mbnum_rp)) );
          
          f1 = (MPEG2_MB_DecARGs *)task_fifo_wp;
        }

	/* decode the coefficient and mv info about current mb */
	if(mpeg_decode_mb(s) < 0){
	  return -1;
	}

        if(s->current_picture.motion_val[0] && !s->encoding){ //note motion_val is normally NULL unless we want to extract the MVs
            const int wrap = s->b8_stride;
            int xy = s->mb_x*2 + s->mb_y*2*wrap;
            int b8_xy= 4*(s->mb_x + s->mb_y*s->mb_stride);
            int motion_x, motion_y, dir, i;

            for(i=0; i<2; i++){
                for(dir=0; dir<2; dir++){
                    if (s->mb_intra || (dir==1 && s->pict_type != FF_B_TYPE)) {
                        motion_x = motion_y = 0;
                    }else if (s->mv_type == MV_TYPE_16X16 || (s->mv_type == MV_TYPE_FIELD && field_pic)){
                        motion_x = s->mv[dir][0][0];
                        motion_y = s->mv[dir][0][1];
                    } else /*if ((s->mv_type == MV_TYPE_FIELD) || (s->mv_type == MV_TYPE_16X8))*/ {
                        motion_x = s->mv[dir][i][0];
                        motion_y = s->mv[dir][i][1];
                    }

                    s->current_picture.motion_val[dir][xy    ][0] = motion_x;
                    s->current_picture.motion_val[dir][xy    ][1] = motion_y;
                    s->current_picture.motion_val[dir][xy + 1][0] = motion_x;
                    s->current_picture.motion_val[dir][xy + 1][1] = motion_y;
                    s->current_picture.ref_index [dir][b8_xy    ]=
                    s->current_picture.ref_index [dir][b8_xy + 1]= s->field_select[dir][i];
                    assert(s->field_select[dir][i]==0 || s->field_select[dir][i]==1);
                }
                xy += wrap;
                b8_xy +=2;
            }
        }

        if(s->progressive_sequence){
          get_blocks_info(s, f1);
          
          task_fifo_wp += ((f1->block_size + 3)>>2);
          
          reach_task_end = (((unsigned int)task_fifo_wp & 0x0000FFFF) + TASK_BUF_LEN) > 0x4000;
          if (reach_task_end)
            task_fifo_wp = (unsigned int *)TCSM0_TASK_FIFO;
          
          (*mbnum_wp)++;
          
          if( s->interlaced_dct || (s->mv_type != MV_TYPE_16X16)){
            *unkown_block = 1;
            av_log(NULL, AV_LOG_WARNING, "\n\n exit : unkown block type in bit stream! \n");
            do{
              tmp = *((volatile int *)(TCSM0_P1_TASK_DONE));
            }while (tmp == 0);
            AUX_RESET();
            exit_player();
          }

          if (!s->mb_intra) {	/* this is in MPV_decode_mb before. */
	      s->last_dc[0] =
              s->last_dc[1] =
              s->last_dc[2] = 128 << s->intra_dc_precision;
          }
        } else {
          s->dest[0] += 16 >> lowres;
          s->dest[1] +=(16 >> lowres) >> s->chroma_x_shift;
          s->dest[2] +=(16 >> lowres) >> s->chroma_x_shift;

          /* Accoding to the info decoded below, we add the pred val and the decode val */
          MPV_decode_mb(s, s->block);
        }

	/* If the width_x length >= width length, then a slice is decoded over. */
        if (++s->mb_x >= s->mb_width) {
            const int mb_size= 16>>s->avctx->lowres;

            ff_draw_horiz_band(s, mb_size*(s->mb_y>>field_pic), mb_size);

            s->mb_x = 0;
            s->mb_y += 1<<field_pic;

            if(s->mb_y >= s->mb_height){
                int left= get_bits_left(&s->gb);
                int is_d10= s->chroma_format==2 && s->pict_type==FF_I_TYPE && avctx->profile==0 && avctx->level==5
                            && s->intra_dc_precision == 2 && s->q_scale_type == 1 && s->alternate_scan == 0
                            && s->progressive_frame == 0 /* vbv_delay == 0xBBB || 0xE10*/;

                if(left < 0 || (left && show_bits(&s->gb, FFMIN(left, 23)) && !is_d10)
                   || (avctx->error_recognition >= FF_ER_AGGRESSIVE && left>8)){
                    av_log(avctx, AV_LOG_ERROR, "end mismatch left=%d %0X\n", left, show_bits(&s->gb, FFMIN(left, 23)));
                    return -1;
                }else
                    goto eos;
            }

            ff_init_block_index(s);
        }

        /* skip mb handling (find the mb_skip_run when a group mb is end) */
        if (s->mb_skip_run == -1) {
            /* read increment again */
            s->mb_skip_run = 0;
            for(;;) {
                int code = get_vlc2(&s->gb, mbincr_vlc.table, MBINCR_VLC_BITS, 2);
                if (code < 0){
                    av_log(s->avctx, AV_LOG_ERROR, "mb incr damaged\n");
                    return -1;
                }
                if (code >= 33) {
                    if (code == 33) {
                        s->mb_skip_run += 33;
                    }else if(code == 35){
		      /* code == 35 means slice end, but we remaining have some skip mb, then we send error message */
		      if(s->mb_skip_run != 0 || show_bits(&s->gb, 15) != 0){
                            av_log(s->avctx, AV_LOG_ERROR, "slice mismatch\n");
                            return -1;
                        }
                        goto eos; /* end of slice */
                    }
                    /* otherwise, stuffing, nothing to do */
                } else {
                    s->mb_skip_run += code;
                    break;
                }
            }
            if(s->mb_skip_run){
                int i;
                if(s->pict_type == FF_I_TYPE){
                    av_log(s->avctx, AV_LOG_ERROR, "skipped MB in I frame at %d %d\n", s->mb_x, s->mb_y);
                    return -1;
                }

                /* skip mb */
                s->mb_intra = 0;
                for(i=0;i<12;i++)
                    s->block_last_index[i] = -1;
                if(s->picture_structure == PICT_FRAME)
                    s->mv_type = MV_TYPE_16X16;
                else
                    s->mv_type = MV_TYPE_FIELD;
                if (s->pict_type == FF_P_TYPE) {
                    /* if P type, zero motion vector is implied */
                    s->mv_dir = MV_DIR_FORWARD;
                    s->mv[0][0][0] = s->mv[0][0][1] = 0;
                    s->last_mv[0][0][0] = s->last_mv[0][0][1] = 0;
                    s->last_mv[0][1][0] = s->last_mv[0][1][1] = 0;
                    s->field_select[0][0] = (s->picture_structure - 1) & 1;
                } else {
                    /* if B type, reuse previous vectors and directions */
                    s->mv[0][0][0] = s->last_mv[0][0][0];
                    s->mv[0][0][1] = s->last_mv[0][0][1];
                    s->mv[1][0][0] = s->last_mv[1][0][0];
                    s->mv[1][0][1] = s->last_mv[1][0][1];
                }
            }
        }
    }
eos: // end of slice
    *buf += (get_bits_count(&s->gb)-1)/8;
    //printf("y %d %d %d %d\n", s->resync_mb_x, s->resync_mb_y, s->mb_x, s->mb_y);

    return 0;
}

static int slice_decode_thread(AVCodecContext *c, void *arg){
    MpegEncContext *s= *(void**)arg;
    const uint8_t *buf= s->gb.buffer;
    int mb_y= s->start_mb_y;
    const int field_pic= s->picture_structure != PICT_FRAME;

    s->error_count= (3*(s->end_mb_y - s->start_mb_y)*s->mb_width) >> field_pic;

    for(;;){
        uint32_t start_code;
        int ret;

	ret= mpeg_decode_slice((Mpeg1Context*)s, mb_y, &buf, s->gb.buffer_end - buf);
        emms_c();
//av_log(c, AV_LOG_DEBUG, "ret:%d resync:%d/%d mb:%d/%d ts:%d/%d ec:%d\n",
//ret, s->resync_mb_x, s->resync_mb_y, s->mb_x, s->mb_y, s->start_mb_y, s->end_mb_y, s->error_count);
        if(ret < 0){
            if(s->resync_mb_x>=0 && s->resync_mb_y>=0)
                ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x, s->mb_y, AC_ERROR|DC_ERROR|MV_ERROR);
        }else{
            ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x-1, s->mb_y, AC_END|DC_END|MV_END);
        }

        if(s->mb_y == s->end_mb_y)
            return 0;

        start_code= -1;
        buf = ff_find_start_code(buf, s->gb.buffer_end, &start_code);
        mb_y= start_code - SLICE_MIN_START_CODE;
        if(mb_y < 0 || mb_y >= s->end_mb_y)
            return -1;
    }

    return 0; //not reached
}

/**
 * Handle slice ends.
 * @return 1 if it seems to be the last slice
 */
static int slice_end(AVCodecContext *avctx, AVFrame *pict)
{
    Mpeg1Context *s1 = avctx->priv_data;
    MpegEncContext *s = &s1->mpeg_enc_ctx;

    if (!s1->mpeg_enc_ctx_allocated || !s->current_picture_ptr)
        return 0;

    if (s->avctx->hwaccel) {
        if (s->avctx->hwaccel->end_frame(s->avctx) < 0)
            av_log(avctx, AV_LOG_ERROR, "hardware accelerator failed to decode picture\n");
    }

    if(CONFIG_MPEG_XVMC_DECODER && s->avctx->xvmc_acceleration)
        ff_xvmc_field_end(s);

    /* end of slice reached */
    if (/*s->mb_y<<field_pic == s->mb_height &&*/ !s->first_field) {
        /* end of image */

        s->current_picture_ptr->qscale_type= FF_QSCALE_TYPE_MPEG2;

        ff_er_frame_end(s);

        MPV_frame_end(s);

        if (s->pict_type == FF_B_TYPE || s->low_delay) {
            *pict= *(AVFrame*)s->current_picture_ptr;
            ff_print_debug_info(s, pict);
        } else {
            s->picture_number++;
            /* latency of 1 frame for I- and P-frames */
            /* XXX: use another variable than picture_number */
            if (s->last_picture_ptr != NULL) {
                *pict= *(AVFrame*)s->last_picture_ptr;
                 ff_print_debug_info(s, pict);
            }
        }

        return 1;
    } else {
        return 0;
    }
}

static int mpeg1_decode_sequence(AVCodecContext *avctx,
                                 const uint8_t *buf, int buf_size)
{
    Mpeg1Context *s1 = avctx->priv_data;
    MpegEncContext *s = &s1->mpeg_enc_ctx;
    int width,height;
    int i, v, j;

    init_get_bits(&s->gb, buf, buf_size*8);

    width = get_bits(&s->gb, 12);
    height = get_bits(&s->gb, 12);
    if (width <= 0 || height <= 0)
        return -1;
    s->aspect_ratio_info= get_bits(&s->gb, 4);
    if (s->aspect_ratio_info == 0) {
        av_log(avctx, AV_LOG_ERROR, "aspect ratio has forbidden 0 value\n");
        if (avctx->error_recognition >= FF_ER_COMPLIANT)
            return -1;
    }
    s->frame_rate_index = get_bits(&s->gb, 4);
    if (s->frame_rate_index == 0 || s->frame_rate_index > 13)
        return -1;
    s->bit_rate = get_bits(&s->gb, 18) * 400;
    if (get_bits1(&s->gb) == 0) /* marker */
        return -1;
    s->width = width;
    s->height = height;

    s->avctx->rc_buffer_size= get_bits(&s->gb, 10) * 1024*16;
    skip_bits(&s->gb, 1);

    /* get matrix */
    if (get_bits1(&s->gb)) {
        load_matrix(s, s->chroma_intra_matrix, s->intra_matrix, 1);
    } else {
        for(i=0;i<64;i++) {
            j = s->dsp.idct_permutation[i];
            v = ff_mpeg1_default_intra_matrix[i];
            s->intra_matrix[j] = v;
            s->chroma_intra_matrix[j] = v;
        }
    }
    if (get_bits1(&s->gb)) {
        load_matrix(s, s->chroma_inter_matrix, s->inter_matrix, 0);
    } else {
        for(i=0;i<64;i++) {
            int j= s->dsp.idct_permutation[i];
            v = ff_mpeg1_default_non_intra_matrix[i];
            s->inter_matrix[j] = v;
            s->chroma_inter_matrix[j] = v;
        }
    }

    if(show_bits(&s->gb, 23) != 0){
        av_log(s->avctx, AV_LOG_ERROR, "sequence header damaged\n");
        return -1;
    }

    /* we set MPEG-2 parameters so that it emulates MPEG-1 */
    s->progressive_sequence = 1;
    s->progressive_frame = 1;
    s->picture_structure = PICT_FRAME;
    s->frame_pred_frame_dct = 1;
    s->chroma_format = 1;
    s->codec_id= s->avctx->codec_id= CODEC_ID_MPEG1VIDEO;
    avctx->sub_id = 1; /* indicates MPEG-1 */
    s->out_format = FMT_MPEG1;
    s->swap_uv = 0;//AFAIK VCR2 does not have SEQ_HEADER
    if(s->flags & CODEC_FLAG_LOW_DELAY) s->low_delay=1;

    if(s->avctx->debug & FF_DEBUG_PICT_INFO)
        av_log(s->avctx, AV_LOG_DEBUG, "vbv buffer: %d, bitrate:%d\n",
               s->avctx->rc_buffer_size, s->bit_rate);

    return 0;
}

static int vcr2_init_sequence(AVCodecContext *avctx)
{
    Mpeg1Context *s1 = avctx->priv_data;
    MpegEncContext *s = &s1->mpeg_enc_ctx;
    int i, v;

    /* start new MPEG-1 context decoding */
    s->out_format = FMT_MPEG1;
    if (s1->mpeg_enc_ctx_allocated) {
        MPV_common_end(s);
    }
    s->width  = avctx->coded_width;
    s->height = avctx->coded_height;
    avctx->has_b_frames= 0; //true?
    s->low_delay= 1;

    avctx->pix_fmt = mpeg_get_pixelformat(avctx);
    avctx->hwaccel = ff_find_hwaccel(avctx->codec->id, avctx->pix_fmt);

    if( avctx->pix_fmt == PIX_FMT_XVMC_MPEG2_IDCT || avctx->hwaccel ||
        s->avctx->codec->capabilities&CODEC_CAP_HWACCEL_VDPAU )
        if( avctx->idct_algo == FF_IDCT_AUTO )
            avctx->idct_algo = FF_IDCT_SIMPLE;

    if (MPV_common_init(s) < 0)
        return -1;
    exchange_uv(s);//common init reset pblocks, so we swap them here
    s->swap_uv = 1;// in case of xvmc we need to swap uv for each MB
    s1->mpeg_enc_ctx_allocated = 1;

    for(i=0;i<64;i++) {
        int j= s->dsp.idct_permutation[i];
        v = ff_mpeg1_default_intra_matrix[i];
        s->intra_matrix[j] = v;
        s->chroma_intra_matrix[j] = v;

        v = ff_mpeg1_default_non_intra_matrix[i];
        s->inter_matrix[j] = v;
        s->chroma_inter_matrix[j] = v;
    }

    s->progressive_sequence = 1;
    s->progressive_frame = 1;
    s->picture_structure = PICT_FRAME;
    s->frame_pred_frame_dct = 1;
    s->chroma_format = 1;
    s->codec_id= s->avctx->codec_id= CODEC_ID_MPEG2VIDEO;
    avctx->sub_id = 2; /* indicates MPEG-2 */
    s1->save_width           = s->width;
    s1->save_height          = s->height;
    s1->save_progressive_seq = s->progressive_sequence;
    return 0;
}


static void mpeg_decode_user_data(AVCodecContext *avctx,
                                  const uint8_t *p, int buf_size)
{
    const uint8_t *buf_end = p+buf_size;

    /* we parse the DTG active format information */
    if (buf_end - p >= 5 &&
        p[0] == 'D' && p[1] == 'T' && p[2] == 'G' && p[3] == '1') {
        int flags = p[4];
        p += 5;
        if (flags & 0x80) {
            /* skip event id */
            p += 2;
        }
        if (flags & 0x40) {
            if (buf_end - p < 1)
                return;
            avctx->dtg_active_format = p[0] & 0x0f;
        }
    }
}

static void mpeg_decode_gop(AVCodecContext *avctx,
                            const uint8_t *buf, int buf_size){
    Mpeg1Context *s1 = avctx->priv_data;
    MpegEncContext *s = &s1->mpeg_enc_ctx;

    int drop_frame_flag;
    int time_code_hours, time_code_minutes;
    int time_code_seconds, time_code_pictures;
    int broken_link;

    init_get_bits(&s->gb, buf, buf_size*8);

    drop_frame_flag = get_bits1(&s->gb);

    time_code_hours=get_bits(&s->gb,5);
    time_code_minutes = get_bits(&s->gb,6);
    skip_bits1(&s->gb);//marker bit
    time_code_seconds = get_bits(&s->gb,6);
    time_code_pictures = get_bits(&s->gb,6);

    s->closed_gop = get_bits1(&s->gb);
    /*broken_link indicate that after editing the
      reference frames of the first B-Frames after GOP I-Frame
      are missing (open gop)*/
    broken_link = get_bits1(&s->gb);

    if(s->avctx->debug & FF_DEBUG_PICT_INFO)
        av_log(s->avctx, AV_LOG_DEBUG, "GOP (%2d:%02d:%02d.[%02d]) closed_gop=%d broken_link=%d\n",
            time_code_hours, time_code_minutes, time_code_seconds,
            time_code_pictures, s->closed_gop, broken_link);
}

/* function defined by jz */
static void get_frame_info(MpegEncContext * s, MPEG2_FRAME_GloARGs * dFRM){
    dFRM->width = s->width;
    dFRM->height = s->height;
    dFRM->pict_type = s->pict_type;
    dFRM->picture_structure = s->picture_structure;
    dFRM->q_scale_type = s->q_scale_type;
    dFRM->no_rounding = s->no_rounding;
    dFRM->first_field = s->first_field;
    dFRM->linesize = s->current_picture.linesize[0];
    dFRM->uvlinesize = s->current_picture.linesize[1];
    dFRM->cp_data0 = (uint8_t *)get_phy_addr((unsigned int)s->current_picture.data[0]);
    dFRM->cp_data1 = (uint8_t *)get_phy_addr((unsigned int)s->current_picture.data[1]);
}

static void get_blocks_info(MpegEncContext * s, MPEG2_MB_DecARGs * f1){
    f1->mb_x = s->mb_x;
    f1->mb_y = s->mb_y;
    f1->qscale = s->qscale;
    f1->interlaced_dct = s->interlaced_dct;
    f1->mb_intra = s->mb_intra;
    f1->mv_dir = s->mv_dir;
    f1->mv_type = s->mv_type;
    f1->block_size = sizeof(MPEG2_MB_DecARGs);
    memcpy(f1->mv, s->mv, sizeof(s->mv[0][0][0])*16);
    memcpy(f1->field_select, s->field_select, sizeof(s->field_select[0][0]) * 4);

    int i=0;
    int count = 0;
    int cbp = 0;
    for(i=0;i<6;i++)
    {
        if(s->block_last_index[i] >= 0)
	{
	   count++;
           cbp |= 1 << (i*4);
        }
    }
    f1->len_count = count;
    f1->cbp = cbp;
}

static int decode_chunks(AVCodecContext *avctx, uint8_t * mbdata_ptr,
                             AVFrame *picture, int *data_size,
                             const uint8_t *buf, int buf_size);

/* handle buffering and image synchronisation */
static int mpeg_decode_frame(AVCodecContext *avctx,
                             void *data, int *data_size,
                             AVPacket *avpkt)
{
    const uint8_t *buf = avpkt->data;
    int buf_size = avpkt->size;
    Mpeg1Context *s = avctx->priv_data;
    AVFrame *picture = data;
    MpegEncContext *s2 = &s->mpeg_enc_ctx;
    //dprintf(avctx, "fill_buffer\n");

    if (buf_size == 0 || (buf_size == 4 && AV_RB32(buf) == SEQ_END_CODE)) {
        /* special case for last picture */
        if (s2->low_delay==0 && s2->next_picture_ptr) {
            *picture= *(AVFrame*)s2->next_picture_ptr;
            s2->next_picture_ptr= NULL;

            *data_size = sizeof(AVFrame);
        }
        return buf_size;
    }

    if(s2->flags&CODEC_FLAG_TRUNCATED){
        int next= ff_mpeg1_find_frame_end(&s2->parse_context, buf, buf_size, NULL);

        if( ff_combine_frame(&s2->parse_context, next, (const uint8_t **)&buf, &buf_size) < 0 )
            return buf_size;
    }

#if 0
    if (s->repeat_field % 2 == 1) {
        s->repeat_field++;
        //fprintf(stderr,"\nRepeating last frame: %d -> %d! pict: %d %d", avctx->frame_number-1, avctx->frame_number,
        //        s2->picture_number, s->repeat_field);
        if (avctx->flags & CODEC_FLAG_REPEAT_FIELD) {
            *data_size = sizeof(AVPicture);
            goto the_end;
        }
    }
#endif

    if(s->mpeg_enc_ctx_allocated==0 && avctx->codec_tag == AV_RL32("VCR2"))
        vcr2_init_sequence(avctx);

    s->slice_count= 0;

    /* whether we need malloc a new buffer for extradata */
    if(avctx->extradata && !avctx->frame_number)
        decode_chunks(avctx, NULL, picture, data_size, avctx->extradata, avctx->extradata_size);

    return decode_chunks(avctx, NULL, picture, data_size, buf, buf_size);
}

static int decode_chunks(AVCodecContext *avctx, uint8_t * mbdata_ptr,
                             AVFrame *picture, int *data_size,
                             const uint8_t *buf, int buf_size)
{
    Mpeg1Context *s = avctx->priv_data;
    MpegEncContext *s2 = &s->mpeg_enc_ctx;
    const uint8_t *buf_ptr = buf;
    const uint8_t *buf_end = buf + buf_size;
    int ret, input_size;
    int last_code= 0;

#ifdef JZC_DCORE_OPT
    MPEG2_FRAME_GloARGs dFRM;
    MPEG2_FRAME_GloARGs * f = (MPEG2_FRAME_GloARGs *)TCSM1_VCADDR(TCSM1_DFRM_BUF);

    task_fifo_wp =  (volatile uint32_t *)TCSM0_TASK_FIFO;
    tcsm1_fifo_rp = (volatile uint32_t *)TCSM1_VCADDR(TCSM1_FIFO_RP);
    *((volatile int *)(TCSM0_P1_TASK_DONE)) = 1;
    *mbnum_wp = 0;
    *mbnum_rp = 0;
    crc_index = 0;
    int tmp = 0;
#endif
    frame_num++;
    //av_log(NULL, AV_LOG_WARNING, "frame_num == %d, use_jz_buf = %d\n", frame_num, use_jz_buf);

    for(;;) {
        /* find next start code */
        uint32_t start_code = -1;

        buf_ptr = ff_find_start_code(buf_ptr,buf_end, &start_code);
        if (start_code > 0x1ff){
            if(s2->pict_type != FF_B_TYPE || avctx->skip_frame <= AVDISCARD_DEFAULT){
	      /*  we do not care the thread, so make it less */
                if(avctx->thread_count > 1){
                    int i;

                    avctx->execute(avctx, slice_decode_thread, &s2->thread_context[0], NULL, s->slice_count, sizeof(void*));
                    for(i=0; i<s->slice_count; i++)
                        s2->error_count += s2->thread_context[i]->error_count;
                }

                if (CONFIG_MPEG_VDPAU_DECODER && avctx->codec->capabilities&CODEC_CAP_HWACCEL_VDPAU)
                    ff_vdpau_mpeg_picture_complete(s2, buf, buf_size, s->slice_count);

                if (slice_end(avctx, picture)) {
                    if(s2->last_picture_ptr || s2->low_delay) //FIXME merge with the stuff in mpeg_decode_slice
                        *data_size = sizeof(AVPicture);
                }
            }
            s2->pict_type= 0;

#ifdef JZC_DCORE_OPT
            if( s2->progressive_sequence || \
                !((s2->last_picture_ptr==NULL) && (s2->pict_type==FF_B_TYPE) && (!s2->closed_gop)) ){
              do{
                tmp = *((volatile int *)(TCSM0_P1_TASK_DONE));
              }while (tmp == 0);
              AUX_RESET();
            }
#endif

#ifdef JZC_PMON_P0
            if(*mbnum_wp){
              wait += param_pmon_val / (*mbnum_wp);
              av_log(NULL, AV_LOG_WARNING, "waiting for %d, val = %d\n", param_pmon_val / (*mbnum_wp), wait / frame_num);
            }
            PMON_CLEAR(param);
#endif
            return FFMAX(0, buf_ptr - buf - s2->parse_context.last_index);
        }

        input_size = buf_end - buf_ptr;

        if(avctx->debug & FF_DEBUG_STARTCODE){
            av_log(avctx, AV_LOG_DEBUG, "%3X at %td left %d\n", start_code, buf_ptr-buf, input_size);
        }

        /* prepare data for next start code */
        switch(start_code) {
        case SEQ_START_CODE:
            if(last_code == 0){
            mpeg1_decode_sequence(avctx, buf_ptr,
                                    input_size);
                s->sync=1;
            }else{
                av_log(avctx, AV_LOG_ERROR, "ignoring SEQ_START_CODE after %X\n", last_code);
            }
            break;

        case PICTURE_START_CODE:
            if(last_code == 0 || last_code == SLICE_MIN_START_CODE){
            if(mpeg_decode_postinit(avctx) < 0){
                av_log(avctx, AV_LOG_ERROR, "mpeg_decode_postinit() failure\n");
                return -1;
            }

            /* we have a complete image: we try to decompress it */
            if(mpeg1_decode_picture(avctx,
                                    buf_ptr, input_size) < 0)
                s2->pict_type=0;
                s2->first_slice = 1;
            last_code= PICTURE_START_CODE;
            }else{
                av_log(avctx, AV_LOG_ERROR, "ignoring pic after %X\n", last_code);
            }
            break;
        case EXT_START_CODE:
            init_get_bits(&s2->gb, buf_ptr, input_size*8);

            switch(get_bits(&s2->gb, 4)) {
            case 0x1:
                if(last_code == 0){
                mpeg_decode_sequence_extension(s);
                }else{
                    av_log(avctx, AV_LOG_ERROR, "ignoring seq ext after %X\n", last_code);
                }
                break;
            case 0x2:
                mpeg_decode_sequence_display_extension(s);
                break;
            case 0x3:
                mpeg_decode_quant_matrix_extension(s2);
                break;
            case 0x7:
                mpeg_decode_picture_display_extension(s);
                break;
            case 0x8:
                if(last_code == PICTURE_START_CODE){
                mpeg_decode_picture_coding_extension(s);
                }else{
                    av_log(avctx, AV_LOG_ERROR, "ignoring pic cod ext after %X\n", last_code);
                }
                break;
            }
            break;
        case USER_START_CODE:
            mpeg_decode_user_data(avctx,
                                    buf_ptr, input_size);
            break;
        case GOP_START_CODE:
            if(last_code == 0){
            s2->first_field=0;
            mpeg_decode_gop(avctx,
                                    buf_ptr, input_size);
                s->sync=1;
            }else{
                av_log(avctx, AV_LOG_ERROR, "ignoring GOP_START_CODE after %X\n", last_code);
            }
            break;
        default:
            if (start_code >= SLICE_MIN_START_CODE &&
                start_code <= SLICE_MAX_START_CODE && last_code!=0) {
                const int field_pic= s2->picture_structure != PICT_FRAME;
                int mb_y= (start_code - SLICE_MIN_START_CODE) << field_pic;
                last_code= SLICE_MIN_START_CODE;

                if(s2->picture_structure == PICT_BOTTOM_FIELD)
                    mb_y++;

                if (mb_y >= s2->mb_height){
                    av_log(s2->avctx, AV_LOG_ERROR, "slice below image (%d >= %d)\n", mb_y, s2->mb_height);

                    if(s2->progressive_sequence){
                      *unkown_block = 1;
                      do{
                        tmp = *((volatile int *)(TCSM0_P1_TASK_DONE));
                      }while (tmp == 0);
                      AUX_RESET();
                    }

                    return -1;
                }

                if(s2->last_picture_ptr==NULL){
                /* Skip B-frames if we do not have reference frames and gop is not closed */
                    if(s2->pict_type==FF_B_TYPE){
                        if(!s2->closed_gop)
                            break;
                    }
                }
                if(s2->pict_type==FF_I_TYPE)
                    s->sync=1;

                if(s2->next_picture_ptr==NULL){
                /* Skip P-frames if we do not have a reference frame or we have an invalid header. */
                    if(s2->pict_type==FF_P_TYPE && !s->sync) break;
                }

                /* Skip B-frames if we are in a hurry. */
                if(avctx->hurry_up && s2->pict_type==FF_B_TYPE) break;
                if(  (avctx->skip_frame >= AVDISCARD_NONREF && s2->pict_type==FF_B_TYPE)
                    ||(avctx->skip_frame >= AVDISCARD_NONKEY && s2->pict_type!=FF_I_TYPE)
                    || avctx->skip_frame >= AVDISCARD_ALL)
                    break;

                /* Skip everything if we are in a hurry>=5. */
                if(avctx->hurry_up>=5) break;

                if (!s->mpeg_enc_ctx_allocated) break;

                if(s2->codec_id == CODEC_ID_MPEG2VIDEO){
                    if(mb_y < avctx->skip_top || mb_y >= s2->mb_height - avctx->skip_bottom)
                        break;
                }

                if(!s2->pict_type){
                    av_log(avctx, AV_LOG_ERROR, "Missing picture start code\n");
                    break;
                }

                //if(s2->pict_type != FF_I_TYPE)
                  //return 0;

                if(s2->first_slice){
                  //s2->first_slice=0;
                    if(mpeg_field_start(s2, buf, buf_size) < 0)
                        return -1;
                    
                }
                
                if(!s2->current_picture_ptr){
                    av_log(avctx, AV_LOG_ERROR, "current_picture not initialized\n");
                    return -1;
                }

                if (avctx->codec->capabilities&CODEC_CAP_HWACCEL_VDPAU) {
                    s->slice_count++;
                    break;
                }

                if(s2->first_slice){
                  s2->first_slice=0;
                  
                  if(s2->progressive_sequence){
                    get_frame_info(s2, &dFRM);
                    memcpy(f, &dFRM, sizeof(dFRM));
                    /* prepare mc and idct */
                    vmau_setup(s2);
                    if( s2->pict_type != FF_I_TYPE ){
                      motion_init_mpeg2(MPEG_HPEL, MPEG_HPEL);
                      motion_config_mpeg2(s2);
                    }
                    
#ifdef JZC_DCORE_OPT
                    AUX_RESET();
                    *((volatile int *)(TCSM0_P1_TASK_DONE)) = 0;
                    AUX_START();
#endif
                  }
                }

                if(avctx->thread_count > 1){
                    int threshold= (s2->mb_height*s->slice_count + avctx->thread_count/2) / avctx->thread_count;
                    if(threshold <= mb_y){
                        MpegEncContext *thread_context= s2->thread_context[s->slice_count];

                        thread_context->start_mb_y= mb_y;
                        thread_context->end_mb_y  = s2->mb_height;
                        if(s->slice_count){
                            s2->thread_context[s->slice_count-1]->end_mb_y= mb_y;
                            ff_update_duplicate_context(thread_context, s2);
                        }
                        init_get_bits(&thread_context->gb, buf_ptr, input_size*8);
                        s->slice_count++;
                    }
                    buf_ptr += 2; //FIXME add minimum number of bytes per slice
                }else{
                    ret = mpeg_decode_slice(s, mb_y, &buf_ptr, input_size);
                    emms_c();

                    if(ret < 0){
                        if(s2->resync_mb_x>=0 && s2->resync_mb_y>=0)
                            ff_er_add_slice(s2, s2->resync_mb_x, s2->resync_mb_y, s2->mb_x, s2->mb_y, AC_ERROR|DC_ERROR|MV_ERROR);
                    }else{
                        ff_er_add_slice(s2, s2->resync_mb_x, s2->resync_mb_y, s2->mb_x-1, s2->mb_y, AC_END|DC_END|MV_END);
                    }
                }
            }
            break;
        }
    }
}

static void flush(AVCodecContext *avctx){
    Mpeg1Context *s = avctx->priv_data;

    s->sync=0;

    ff_mpeg_flush(avctx);
}

static int mpeg_decode_end(AVCodecContext *avctx)
{
    Mpeg1Context *s = avctx->priv_data;

    if (s->mpeg_enc_ctx_allocated)
        MPV_common_end(&s->mpeg_enc_ctx);

    use_jz_buf = 0;
    return 0;
}

AVCodec mpeg2video_decoder = {
    "mpeg2video",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_MPEG2VIDEO,
    sizeof(Mpeg1Context),
    mpeg_decode_init,
    NULL,
    mpeg_decode_end,
    mpeg_decode_frame,
    CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1 | CODEC_CAP_TRUNCATED | CODEC_CAP_DELAY,
    .flush= flush,
    .max_lowres= 3,
    .long_name= NULL_IF_CONFIG_SMALL("MPEG-2 video"),
};

//legacy decoder
AVCodec mpegvideo_decoder = {
    "mpegvideo",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_MPEG2VIDEO,
    sizeof(Mpeg1Context),
    mpeg_decode_init,
    NULL,
    mpeg_decode_end,
    mpeg_decode_frame,
    CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1 | CODEC_CAP_TRUNCATED | CODEC_CAP_DELAY,
    .flush= flush,
    .max_lowres= 3,
    .long_name= NULL_IF_CONFIG_SMALL("MPEG-1 video"),
};

#if CONFIG_MPEG_XVMC_DECODER
static av_cold int mpeg_mc_decode_init(AVCodecContext *avctx){
    if( avctx->thread_count > 1)
        return -1;
    if( !(avctx->slice_flags & SLICE_FLAG_CODED_ORDER) )
        return -1;
    if( !(avctx->slice_flags & SLICE_FLAG_ALLOW_FIELD) ){
        dprintf(avctx, "mpeg12.c: XvMC decoder will work better if SLICE_FLAG_ALLOW_FIELD is set\n");
    }
    mpeg_decode_init(avctx);

    avctx->pix_fmt = PIX_FMT_XVMC_MPEG2_IDCT;
    avctx->xvmc_acceleration = 2;//2 - the blocks are packed!

    return 0;
}

AVCodec mpeg_xvmc_decoder = {
    "mpegvideo_xvmc",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_MPEG2VIDEO_XVMC,
    sizeof(Mpeg1Context),
    mpeg_mc_decode_init,
    NULL,
    mpeg_decode_end,
    mpeg_decode_frame,
    CODEC_CAP_DRAW_HORIZ_BAND | CODEC_CAP_DR1 | CODEC_CAP_TRUNCATED| CODEC_CAP_HWACCEL | CODEC_CAP_DELAY,
    .flush= flush,
    .long_name = NULL_IF_CONFIG_SMALL("MPEG-1/2 video XvMC (X-Video Motion Compensation)"),
};

#endif

#if CONFIG_MPEG_VDPAU_DECODER
AVCodec mpeg_vdpau_decoder = {
    "mpegvideo_vdpau",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_MPEG2VIDEO,
    sizeof(Mpeg1Context),
    mpeg_decode_init,
    NULL,
    mpeg_decode_end,
    mpeg_decode_frame,
    CODEC_CAP_DR1 | CODEC_CAP_TRUNCATED | CODEC_CAP_HWACCEL_VDPAU | CODEC_CAP_DELAY,
    .flush= flush,
    .long_name = NULL_IF_CONFIG_SMALL("MPEG-1/2 video (VDPAU acceleration)"),
};
#endif

