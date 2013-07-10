#define VP6_P1_USE_PADDR
#define MPEG4_P1_USE_PADDR

#include "jzsys.h"
#include "jzasm.h"
#include "jzmedia.h"
#include "mpeg4_p1_type.h"
//#include "../../libjzcommon/jz4760e_2ddma_hw.h"
#include "jz4760_2ddma_hw.h"
#include "mpeg4_dcore.h"
#include "mpeg4_tcsm1.h"
#include "mpeg4_dblk.h"
//include "jz4760_mc_hw.h"
#include "../../libjzcommon/jz4760e_dcsc.h"

#ifdef JZC_PMON_P1
#include "../libjzcommon/jz4760e_aux_pmon.h"
#endif

#ifdef JZC_PMON_P1MB
PMON_CREAT(p1mc);
int p1mc;
#endif
#include "mpeg4_p1_mc.c"

//#define JZC_VMAU_OPT

#define mpeg4_STOP_P1()						\
  ({								\
    ((volatile int *)TCSM1_P1_FIFO_RP)[0]=0;	\
    *((volatile int *)(TCSM1_P1_TASK_DONE)) = 0x1;	\
    i_nop;							\
    i_nop;							\
    i_wait();							\
  })

#include "mpeg4_aux_idct.c"
#define CODEC_FLAG_PSNR            0x8000
#define FF_MB_DECISION_RD     2
#define CODEC_FLAG_GRAY            0x2000
#define ENABLE_GRAY 1

#define __p1_text __attribute__ ((__section__ (".p1_text")))
#define __p1_main __attribute__ ((__section__ (".p1_main")))
#define __p1_data __attribute__ ((__section__ (".p1_data")))

extern int _gp;

uint32_t current_picture_ptr[3];
MPEG4_Frame_GlbARGs *dFRM;
MPEG4_MB_DecARGs *dMB, *dMB_L, *dMB_N,*dMB_X;

__p1_main int main() {
  S32I2M(xr16, 0x3);

  int xchg_tmp, count, i, j;
  int *gp0_chain_ptr, *gp1_chain_ptr, *dma_chain_ptr;
  uint8_t *dest_y;
  uint8_t *dest_c;
  uint8_t *yuv_dest = RECON_YBUF0;
  uint8_t *yuv_dest1 = RECON_YBUF1; 
  uint8_t *yuv_dest2 = RECON_YBUF3; 
  uint32_t *buf_use = RECON_BUF_USE;
  uint32_t msrc_buf,msrc_buf1;
  const uint16_t off_tab[6] = {0, 8, 8 * RECON_BUF_STRD, 8 * RECON_BUF_STRD + 8, 16*RECON_BUF_STRD, 16*RECON_BUF_STRD+8};

  volatile int *mbnum_wp=TCSM1_MBNUM_WP;
  volatile int *mbnum_rp=TCSM1_MBNUM_RP;
  volatile int *addr_rp=TCSM1_ADDR_RP;

  volatile int *dbg_ptr = TCSM1_DBG_BUF;

  uint16_t mulslice = 0;
  count = 0;
  int maures = 1;
  msrc_buf = SOURSE_BUF;
  msrc_buf1 = SOURSE_BUF1;

  dFRM=TCSM1_DFRM_BUF;

  int task_begin = dFRM->mem_addr;
  int task_end = task_begin + TASK_LEN;

  *mbnum_rp=0;
  *addr_rp=task_begin;

  dMB = TASK_BUF1;//curr mb
  dMB_L = TASK_BUF0;
  dMB_N = TASK_BUF2;
  dMB_X = RESULT_BUF0;

  dma_chain_ptr = TCSM1_VDMA_CHAIN;
  dma_chain_ptr[0] = addr_rp[0] | 0x1;
  dma_chain_ptr[1] = TCSM1_PADDR(dMB);
  dma_chain_ptr[3] = TASK_BUF_LEN / 4 - 1;
  while(*mbnum_wp<=*mbnum_rp+2);//wait until the first two mb is ready

  write_reg(0x13210008, TCSM1_PADDR(dma_chain_ptr) | 0x3);
  while ((read_reg(0x1321000c, 0) & 0x2) == 0);

  (*mbnum_rp)++;
  *addr_rp+= (TASK_BUF_LEN);
  if((int)(*addr_rp)>(task_end - TASK_BUF_LEN))
    *addr_rp=task_begin;
  ((volatile int *)TCSM1_P1_FIFO_RP)[0]=*addr_rp;

  dma_chain_ptr[0] = addr_rp[0] | 0x1;
  dma_chain_ptr[1] = TCSM1_PADDR(dMB_N);
  dma_chain_ptr[3] = TASK_BUF_LEN / 4 - 1;

  write_reg(0x13210008, TCSM1_PADDR(dma_chain_ptr) | 0x3);
  while ((read_reg(0x1321000c, 0) & 0x2) == 0);

  (*mbnum_rp)++;
  *addr_rp += (TASK_BUF_LEN);
  if((int)(*addr_rp)>(task_end - TASK_BUF_LEN))
    *addr_rp=task_begin;
  ((volatile int *)TCSM1_P1_FIFO_RP)[0]=*addr_rp;

  dest_y = dFRM->current_picture_data[0] -512; //for no edge
  dest_c = dFRM->current_picture_data[1] -256;

  while (dMB->real_num > 0){
    int mb_x, mb_y;
    int last_mb_y,last_mb_x;
    mb_x = dMB->mb_x;
    mb_y = dMB->mb_y;
    uint16_t *dst=msrc_buf-16;

    dest_y += 256;//for no edge
    dest_c += 128;

    if ((mb_x == 0 && mb_y == 0) || mulslice == 1){
      if (!dMB->mb_intra){//nees start mc
	MPV_motion_p1(dMB->mv_type, dFRM->quarter_sample, dMB->mv_dir, dFRM->no_rounding,
		      mb_x, mb_y, dMB->mv, dFRM->width, dFRM->height, dFRM->workaround_bugs);
      }
      if (mulslice == 1){
	mulslice = 0;
	goto mulslice;
      }
      goto skip_all;
    }else{
      if (!dMB_L->mb_intra){//need check mc result
	ms_check_mc_result();
      }
#if 1
      if (*buf_use == 0){
	yuv_dest = RECON_YBUF0;//this is last buf for idct now.
	*buf_use = 1;
      }else{
	yuv_dest = RECON_YBUF1;
	*buf_use = 0;
      }
#endif
      if (!dMB->mb_intra){//nees start mc
	MPV_motion_p1(dMB->mv_type, dFRM->quarter_sample, dMB->mv_dir, dFRM->no_rounding,
		      mb_x, mb_y, dMB->mv, dFRM->width, dFRM->height, dFRM->workaround_bugs);
      }
    }

    { //FIXME precal
      const int linesize= 16; //not s->linesize as this would be wrong for field pics
      const int uvlinesize= 16;

      if (!dMB_L->mb_intra) {

	if(!(dFRM->h263_msmpeg4 || dFRM->codec_id==1 || dFRM->codec_id==2 || (dFRM->codec_id==13 && !dFRM->mpeg_quant))){
	  add_dequant_dct_opt(dMB_L->block[0], 0, yuv_dest + off_tab[0], RECON_BUF_STRD, dMB_L->qscale);
	  add_dequant_dct_opt(dMB_L->block[1], 1, yuv_dest + off_tab[1], RECON_BUF_STRD, dMB_L->qscale);
	  add_dequant_dct_opt(dMB_L->block[2], 2, yuv_dest + off_tab[2], RECON_BUF_STRD, dMB_L->qscale);
	  add_dequant_dct_opt(dMB_L->block[3], 3, yuv_dest + off_tab[3], RECON_BUF_STRD, dMB_L->qscale);
	  add_dequant_dct_opt(dMB_L->block[4], 4, yuv_dest + off_tab[4], RECON_BUF_STRD, dMB_L->chroma_qscale);
	  add_dequant_dct_opt(dMB_L->block[5], 5, yuv_dest + off_tab[5], RECON_BUF_STRD, dMB_L->chroma_qscale);
	} else if(dFRM->codec_id != 19){
	  add_dct_opt(dMB_L->block[0], 0, yuv_dest + off_tab[0], RECON_BUF_STRD);
	  add_dct_opt(dMB_L->block[1], 1, yuv_dest + off_tab[1], RECON_BUF_STRD);
	  add_dct_opt(dMB_L->block[2], 2, yuv_dest + off_tab[2], RECON_BUF_STRD);
	  add_dct_opt(dMB_L->block[3], 3, yuv_dest + off_tab[3], RECON_BUF_STRD);
	  add_dct_opt(dMB_L->block[4], 4, yuv_dest + off_tab[4], RECON_BUF_STRD);
	  add_dct_opt(dMB_L->block[5], 5, yuv_dest + off_tab[5], RECON_BUF_STRD);
	}
      } else {
	/* dct only in intra block */
	if(!(dFRM->codec_id==1 || dFRM->codec_id==2)){
	  put_dct_opt(dMB_L->block[0], 0, yuv_dest + off_tab[0], dMB_L->qscale);
	  put_dct_opt(dMB_L->block[1], 1, yuv_dest + off_tab[1], dMB_L->qscale);
	  put_dct_opt(dMB_L->block[2], 2, yuv_dest + off_tab[2], dMB_L->qscale);
	  put_dct_opt(dMB_L->block[3], 3, yuv_dest + off_tab[3], dMB_L->qscale);

	  put_dct_opt(dMB_L->block[4], 4, yuv_dest + off_tab[4], dMB_L->chroma_qscale);
	  put_dct_opt(dMB_L->block[5], 5, yuv_dest + off_tab[5], dMB_L->chroma_qscale);
	}else{
	  ff_simple_idct_put_mxu(yuv_dest + off_tab[0],8, 16, dMB->block[0]);
	  ff_simple_idct_put_mxu(yuv_dest + off_tab[1],8, 16, dMB->block[1]);
	  ff_simple_idct_put_mxu(yuv_dest + off_tab[2],8, 16, dMB->block[2]);
	  ff_simple_idct_put_mxu(yuv_dest + off_tab[3],8, 16, dMB->block[3]);

	  ff_simple_idct_put_mxu(yuv_dest + off_tab[4],8, 16, dMB->block[4]);                        
	  ff_simple_idct_put_mxu(yuv_dest + off_tab[5],8, 16, dMB->block[5]);                        
	}
      }

    }

  skip_dbg:
    if(count > 0){
      //poll_gp1_end();
#if 0
      while(*((volatile int *)(TCSM1_P1_POLL)) == 0);
      *((volatile int *)(TCSM1_P1_POLL)) = 0;

      gp1_chain_ptr[0]=TCSM1_PADDR(yuv_dest);
      gp1_chain_ptr[1]=(dest_y);
      gp1_chain_ptr[4]=TCSM1_PADDR(yuv_dest + off_tab[4]);
      gp1_chain_ptr[5]=(dest_c);

      gp1_chain_ptr[8] = TCSM0_PADDR(TCSM0_P1_POLL);
      gp1_chain_ptr[9] = TCSM1_PADDR(TCSM1_DBG_BUF+256);
      gp1_chain_ptr[12] = TCSM0_PADDR(TCSM0_P1_POLL);
      gp1_chain_ptr[13] = TCSM1_PADDR(TCSM1_DBG_BUF+256);

      gp1_chain_ptr[16] = TCSM0_PADDR(TCSM0_P1_POLL);
      gp1_chain_ptr[17] = TCSM1_PADDR(TCSM1_P1_POLL);
      gp1_chain_ptr[18] = GP_STRD(4,GP_FRM_NML,4);
      gp1_chain_ptr[19] = GP_UNIT(GP_TAG_UL,4,4);
      
      set_gp1_dha(TCSM1_PADDR(gp1_chain_ptr));
      set_gp1_dcs();
      //poll_gp1_end();
      //mpeg4_STOP_P1();
#else
      dma_chain_ptr[0] = TCSM1_PADDR(yuv_dest);
      dma_chain_ptr[1] = dest_y;
      dma_chain_ptr[1] |= 0x1;
      dma_chain_ptr[3] = (256 / 4 - 1);
      write_reg(0x13210008, TCSM1_PADDR(dma_chain_ptr) | 0x3);
      while ((read_reg(0x1321000c, 0) & 0x2) == 0);

      dma_chain_ptr[0] = TCSM1_PADDR(yuv_dest + off_tab[4]);
      dma_chain_ptr[1] = dest_c;
      dma_chain_ptr[1] |= 0x1;
      dma_chain_ptr[3] = (128 / 4 - 1);
      write_reg(0x13210008, TCSM1_PADDR(dma_chain_ptr) | 0x3);
      while ((read_reg(0x1321000c, 0) & 0x2) == 0);
#endif
    }
    last_mb_y=dMB_L->mb_y;
    last_mb_x=dMB_L->mb_x;

  skip_all:

    while(*mbnum_wp<=*mbnum_rp+2){
    }//wait until the next next mb is ready


    if (count > 0){
      *addr_rp+=(TASK_BUF_LEN);
      if((int)(*addr_rp)>=(task_end-TASK_BUF_LEN))
	*addr_rp=task_begin;
      ((volatile int *)TCSM1_P1_FIFO_RP)[0]=*addr_rp;
      (*mbnum_rp)++;
    }

  mulslice:
    //XCHG3(dMB_L,dMB,dMB_N,xchg_tmp);
    XCHG4(dMB_L,dMB,dMB_N,dMB_X,xchg_tmp);
    //XCHG3(yuv_dest,yuv_dest1,yuv_dest2,xchg_tmp);
    XCHG2(msrc_buf,msrc_buf1,xchg_tmp);

    dma_chain_ptr[0] = addr_rp[0] | 0x1;
    dma_chain_ptr[1] = TCSM1_PADDR(dMB_N);
    dma_chain_ptr[3] = TASK_BUF_LEN / 4 - 1;
    write_reg(0x13210008, TCSM1_PADDR(dma_chain_ptr) | 0x3);
    while ((read_reg(0x1321000c, 0) & 0x2) == 0);
    count++;
  }

  *((volatile int *)TCSM1_P1_TASK_DONE) = 0x1;
  i_nop;  
  i_nop;    
  i_nop;      
  i_nop;  
  i_wait();

  return 0;
}
