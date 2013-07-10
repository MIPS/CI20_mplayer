#define VP6_P1_USE_PADDR
#define MPEG4_P1_USE_PADDR

#include "jzsys.h"
#include "jzasm.h"
#include "jzmedia.h"
#include "mpeg4_p1_type.h"
//#include "../../libjzcommon/jz4760e_2ddma_hw.h"
#include "jz4760_2ddma_hw.h"
#include "mpeg4_dcore.h"
//#include "mpeg4_tcsm0.h"
#include "mpeg4_tcsm1.h"
#include "mpeg4_sram.h"
#include "mpeg4_dblk.h"
//include "jz4760_mc_hw.h"
#include "../../libjzcommon/jz4760e_dcsc.h"

#include "mpeg4_p1_mc.c"

#define JZC_VMAU_OPT
#ifdef JZC_VMAU_OPT
#include "mpeg4_p1_vmau.h"
#endif

#define mpeg4_STOP_P1()						\
  ({								\
    ((volatile int *)TCSM1_P1_FIFO_RP)[0]=0;	\
    *((volatile int *)(TCSM1_P1_TASK_DONE)) = 0x1;	\
    i_nop;							\
    i_nop;							\
    i_wait();							\
  })

#define CODEC_FLAG_PSNR            0x8000
#define FF_MB_DECISION_RD     2
#define CODEC_FLAG_GRAY            0x2000
#define ENABLE_GRAY 1
#define DBLK_OPT

#define __p1_text __attribute__ ((__section__ (".p1_text")))
#define __p1_main __attribute__ ((__section__ (".p1_main")))
#define __p1_data __attribute__ ((__section__ (".p1_data")))

extern int _gp;

MPEG4_Frame_GlbARGs *dFRM;
MPEG4_MB_DecARGs *dMB, *dMB_L, *dMB_N,*dMB_X;

__p1_main int main() {
  S32I2M(xr16, 0x3);

  volatile int *dbg_ptr = TCSM1_DBG_BUF;
  memset(dbg_ptr, 0, 64);
  dbg_ptr[0] = 111;

  int xchg_tmp, count, i, j;
  volatile int *gp0_chain_ptr;
  volatile int *gp1_chain_ptr;
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

  volatile int *dma_chain_ptr = TCSM1_VDMA_CHAIN;
  dma_chain_ptr[0] = addr_rp[0] | 0x1;
  dma_chain_ptr[1] = TCSM1_PADDR(dMB);
  dma_chain_ptr[3] = TASK_BUF_LEN / 4 - 1;
  while(*mbnum_wp<=*mbnum_rp+2);//wait until the first two mb is ready

  write_reg(0x1321000c, 0);
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

  write_reg(0x1321000c, 0);
  write_reg(0x13210008, TCSM1_PADDR(dma_chain_ptr) | 0x3);
  while ((read_reg(0x1321000c, 0) & 0x2) == 0);

  (*mbnum_rp)++;
  *addr_rp += (TASK_BUF_LEN);
  if((int)(*addr_rp)>(task_end - TASK_BUF_LEN))
    *addr_rp=task_begin;
  ((volatile int *)TCSM1_P1_FIFO_RP)[0]=*addr_rp;

  dest_y = dFRM->current_picture_data[0] -768; //for no edge
  dest_c = dFRM->current_picture_data[1] -384;

  if (dMB->mb_x != 0 || dMB->mb_y != 0){//for mul-slice
    dest_y += (dMB->mb_y * dFRM->mb_width * 256 + dMB->mb_x * 256 + dMB->mb_y * 1024 + 1024 - 256);
    //+1024 is correct of -1280, -256 is twice dest cal in while
    dest_c += (dMB->mb_y * dFRM->mb_width * 128 + dMB->mb_x * 128 + dMB->mb_y * 512 + 512 - 128);
    if (dMB->mb_x == 0 || dMB->mb_x == 1){//insure step over the edge buf
      dest_y -= 1024;
      dest_c -= 512;
    }
    count += (dMB->mb_y * dFRM->mb_width + dMB->mb_x);
    mulslice=1;
  }

#ifdef JZC_VMAU_OPT
  volatile int *mauend = MAU_ENDF_BASE;
  mauend[0] = 0x0;
  unsigned int tmp = ((VMAU_MPEG4 & MAU_VIDEO_MSK) << MAU_VIDEO_SFT);
  write_reg(VMAU_V_BASE+VMAU_SLV_GBL_RUN, VMAU_RESET);

  write_reg(VMAU_V_BASE+VMAU_SLV_VIDEO_TYPE, tmp);
  write_reg(VMAU_V_BASE+VMAU_SLV_NCCHN_ADDR, TCSM1_PADDR(MAU_CHN_BASE));//12 ///???

  write_reg(VMAU_V_BASE+VMAU_SLV_POS, 0); //0x60
  write_reg(VMAU_V_BASE+VMAU_SLV_DEC_STR,(16<<16)|16); //0x74

  write_reg(VMAU_V_BASE+VMAU_SLV_GBL_CTR, 0);
  write_reg(VMAU_V_BASE+VMAU_SLV_Y_GS, dFRM->mb_width*16);//0x54
  write_reg(VMAU_V_BASE+VMAU_SLV_DEC_DONE, TCSM1_PADDR(MAU_ENDF_BASE));
#ifdef DBLK_OPT
  write_reg(VMAU_V_BASE+VMAU_SLV_GBL_CTR, (1<<24) | 1);//for dblk
#else
  write_reg(VMAU_V_BASE+VMAU_SLV_GBL_CTR, 1);
#endif

  vmau_slv_reg_p mau_slv_reg_ptr;
  mau_slv_reg_ptr = MAU_CHN_BASE;
  mau_slv_reg_ptr->ncchn_addr = TCSM1_PADDR(MAU_CHN_BASE);
  mau_slv_reg_ptr->main_len = 768;
#endif

#ifdef DBLK_OPT
#if 0
  hw_dblk_mpeg4_init(TCSM1_PADDR(TCSM1_DBLK_CHN_BASE), TCSM1_PADDR(TCSM1_DBLK_CHN_END_BASE), dFRM->mb_width, dFRM->mb_height, 0, 0, dFRM->current_picture_data[0], dFRM->current_picture_data[1], TCSM1_PADDR(TCSM1_DBLK_DMA_END_BASE), TCSM1_PADDR(TCSM1_DBLK_SLICE_END_BASE), dFRM->mb_width*256, dFRM->mb_width*128, (dFRM->mb_width*16 * dFRM->mb_height*16) < (176*144));
#else
  hw_dblk_mpeg4_init(TCSM1_PADDR(TCSM1_DBLK_CHN_BASE), TCSM1_PADDR(TCSM1_DBLK_CHN_END_BASE), dFRM->mb_width, dFRM->mb_height, 0, 0, dFRM->current_picture_data[0], dFRM->current_picture_data[1], TCSM1_PADDR(TCSM1_DBLK_DMA_END_BASE), TCSM1_PADDR(TCSM1_DBLK_SLICE_END_BASE), dFRM->linesize, dFRM->uvlinesize, (dFRM->mb_width*16 * dFRM->mb_height*16) < (176*144));
#endif
  int *dblk_chn_ptr = TCSM1_DBLK_CHN_BASE;
  volatile int *dblk_chn_end_ptr = TCSM1_DBLK_CHN_END_BASE;
  volatile int *dblk_dma_end_ptr = TCSM1_DBLK_DMA_END_BASE;
  volatile int *dblk_sli_end_ptr = TCSM1_DBLK_SLICE_END_BASE;
  dblk_dma_end_ptr[0] = -1;
  dblk_sli_end_ptr[0] = -1;
  int last_mb = -1;
  int last_mb2 = -1;
#endif

  while (dMB->real_num > 0){
#if 1
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
      if (!dMB_L->mb_intra){//need check mc result{
	uint16_t *src;
	src=dMB_L->block[0];
	src-=8;
	uint32_t cbp_mask = 0x1;
	for(i=0;i<6;i++){
	  if((dMB_L->code_cbp&cbp_mask)!=0){
	    for(j=0;j<8;j++){
	      S32LDI(xr1,src,0x10);
	      S32LDD(xr2,src,0x4);
	      S32LDD(xr3,src,0x8);
	      S32LDD(xr4,src,0xc);
	      S32SDI(xr1,dst,0x10);
	      S32STD(xr2,dst,0x4);
	      S32STD(xr3,dst,0x8);
	      S32STD(xr4,dst,0xc);
	    }
	  }else{
	    src+=64;
	  }
	  cbp_mask<<=4;
	}
      
	dbg_ptr[0]++;
	check_mc_result();
	dbg_ptr[1]++;
      }

      if (!dMB->mb_intra){//nees start mc
	MPV_motion_p1(dMB->mv_type, dFRM->quarter_sample, dMB->mv_dir, dFRM->no_rounding,
		      mb_x, mb_y, dMB->mv, dFRM->width, dFRM->height, dFRM->workaround_bugs);
      }
    }

    { //FIXME precal  
      if (!dMB_L->mb_intra) {
	if(!(dFRM->h263_msmpeg4 || dFRM->codec_id==1 || dFRM->codec_id==2 || (dFRM->codec_id==13 && !dFRM->mpeg_quant))){
	  dbg_ptr[4]++;
	  while (mauend[0] == 0xFFFFFFFF);
	  dbg_ptr[5]++;
	  mauend[0] = 0xFFFFFFFF;
	  write_reg(VMAU_V_BASE+VMAU_SLV_DEC_YADDR,TCSM1_PADDR(yuv_dest1));
	  write_reg(VMAU_V_BASE+VMAU_SLV_DEC_UADDR,TCSM1_PADDR(yuv_dest1 + 256));
	  write_reg(VMAU_V_BASE+VMAU_SLV_DEC_VADDR,TCSM1_PADDR(yuv_dest1 + 256+8));

	  mau_slv_reg_ptr->main_len = (uint32_t)dst-msrc_buf+16;
	  mau_slv_reg_ptr->main_cbp = (dMB_L->code_cbp) | (MAU_Y_ERR_MSK << MAU_Y_ERR_SFT ) | (MAU_C_ERR_MSK << MAU_C_ERR_SFT );
	  mau_slv_reg_ptr->main_addr = TCSM1_PADDR(msrc_buf);
	  mau_slv_reg_ptr->quant_para = ((!(dFRM->mpeg_quant))<<23) | ((dMB_L->qscale & QUANT_QP_MSK) << QUANT_QP_SFT);
	  
	  write_reg(VMAU_V_BASE+VMAU_SLV_GBL_RUN, VMAU_RUN);
	} else if(dFRM->codec_id != 19){
	  dbg_ptr[4]++;
	  while (mauend[0] == 0xFFFFFFFF);
	  dbg_ptr[5]++;
	  mauend[0] = 0xFFFFFFFF;
	  write_reg(VMAU_V_BASE+VMAU_SLV_DEC_YADDR,TCSM1_PADDR(yuv_dest1));
	  write_reg(VMAU_V_BASE+VMAU_SLV_DEC_UADDR,TCSM1_PADDR(yuv_dest1 + 256));
	  write_reg(VMAU_V_BASE+VMAU_SLV_DEC_VADDR,TCSM1_PADDR(yuv_dest1 + 256+8));

	  mau_slv_reg_ptr->main_len = (uint32_t)dst-msrc_buf+16;
	  mau_slv_reg_ptr->main_cbp = (dMB_L->code_cbp) | (MAU_Y_ERR_MSK << MAU_Y_ERR_SFT ) | (MAU_C_ERR_MSK << MAU_C_ERR_SFT );
	  mau_slv_reg_ptr->main_addr = TCSM1_PADDR(msrc_buf);
	  mau_slv_reg_ptr->quant_para = (0x1<<23) | ((dMB_L->qscale & QUANT_QP_MSK) << QUANT_QP_SFT);
	  write_reg(VMAU_V_BASE+VMAU_SLV_GBL_RUN, VMAU_RUN);
	}
      } else {
	if(!(dFRM->codec_id==1 || dFRM->codec_id==2)){
	  dbg_ptr[4]++;
	  while (mauend[0] == 0xFFFFFFFF);
	  dbg_ptr[5]++;
	  mauend[0] = 0xFFFFFFFF;
	  write_reg(VMAU_V_BASE+VMAU_SLV_DEC_YADDR,TCSM1_PADDR(yuv_dest1));
	  write_reg(VMAU_V_BASE+VMAU_SLV_DEC_UADDR,TCSM1_PADDR(yuv_dest1 + 256));
	  write_reg(VMAU_V_BASE+VMAU_SLV_DEC_VADDR,TCSM1_PADDR(yuv_dest1 + 256+8));

	  mau_slv_reg_ptr->main_len = 768;
	  mau_slv_reg_ptr->main_cbp = 0x1111111;
	  mau_slv_reg_ptr->main_addr = TCSM1_PADDR(dMB_L->block[0]);
	  mau_slv_reg_ptr->quant_para = ((!(dFRM->mpeg_quant))<<23) | ((dMB_L->qscale & QUANT_QP_MSK) << QUANT_QP_SFT);
	  write_reg(VMAU_V_BASE+VMAU_SLV_GBL_RUN, VMAU_RUN);
	}else{
	  dbg_ptr[4]++;
	  while (mauend[0] == 0xFFFFFFFF);
	  dbg_ptr[5]++;
	  mauend[0] = 0xFFFFFFFF;
	  write_reg(VMAU_V_BASE+VMAU_SLV_DEC_YADDR,TCSM1_PADDR(yuv_dest1));
	  write_reg(VMAU_V_BASE+VMAU_SLV_DEC_UADDR,TCSM1_PADDR(yuv_dest1 + 256));
	  write_reg(VMAU_V_BASE+VMAU_SLV_DEC_VADDR,TCSM1_PADDR(yuv_dest1 + 256+8));

	  mau_slv_reg_ptr->main_cbp = 0x1111111;
	  mau_slv_reg_ptr->main_addr = TCSM1_PADDR(dMB_L->block[0]);
	  mau_slv_reg_ptr->quant_para = ((!(dFRM->mpeg_quant))<<23) | ((dMB_L->qscale & QUANT_QP_MSK) << QUANT_QP_SFT);
	  write_reg(VMAU_V_BASE+VMAU_SLV_GBL_RUN, VMAU_RUN);
	}
      }
    }

  skip_dbg:
    if(count > 1){
#ifdef DBLK_OPT
      dblk_chn_ptr[0] = (TCSM1_PADDR(dblk_chn_ptr)&0xFFFFFF);

      last_mb = dMB_X->mb_x | dMB_X->mb_y << 16;
      dblk_chn_end_ptr[0] = -1;
      write_dblk_reg(DEBLK_REG_TRIG, DBLK_RUN);
      while(*((volatile int*)dblk_chn_end_ptr) == -1); //waiting read chain task end
      *((volatile int*)dblk_chn_end_ptr) = -1;
      
      if (count > 2){
	dbg_ptr[2]++;
	while ((*(volatile int *)dblk_dma_end_ptr) != last_mb2);
	dbg_ptr[3]++;
      }
      last_mb2 = last_mb;
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
#endif
  skip_all:

    while(*mbnum_wp<=*mbnum_rp+2){
    }//wait until the next next mb is ready

    if (count > 0){
#ifdef DBLK_OPT
      dbg_ptr[6]++;
      while ((read_reg(0x1321000c, 0) & 0x2) == 0);//poll dma
      dbg_ptr[7]++;
#endif

      *addr_rp+=(TASK_BUF_LEN);
      if((int)(*addr_rp)>=(task_end-TASK_BUF_LEN))
	*addr_rp=task_begin;
      ((volatile int *)TCSM1_P1_FIFO_RP)[0]=*addr_rp;
      (*mbnum_rp)++;
    }

  mulslice:
    XCHG4(dMB_L,dMB,dMB_N,dMB_X,xchg_tmp);
    XCHG3(yuv_dest,yuv_dest1,yuv_dest2,xchg_tmp);
    XCHG2(msrc_buf,msrc_buf1,xchg_tmp);

    dma_chain_ptr[0] = addr_rp[0] | 0x1;
    dma_chain_ptr[1] = TCSM1_PADDR(dMB_N);
    dma_chain_ptr[3] = TASK_BUF_LEN / 4 - 1;
    //write_reg(0x1321000c, 0);
    write_reg(0x13210008, TCSM1_PADDR(dma_chain_ptr) | 0x3);
#ifndef DBLK_OPT
    while ((read_reg(0x1321000c, 0) & 0x2) == 0);
#endif

    count++;
  }

#ifdef DBLK_OPT
  dblk_chn_ptr[0] = (TCSM1_PADDR(dblk_chn_ptr)&0xFFFFFF) | (0x1<<31);

  dblk_chn_end_ptr[0] = -1;
  write_dblk_reg(DEBLK_REG_TRIG, DBLK_RUN);
  while(*((volatile int*)dblk_chn_end_ptr) == -1); //waiting read chain task end
  *((volatile int*)dblk_chn_end_ptr) = -1;
  while (read_dblk_reg(DEBLK_REG_GSTA)&0x1 == 0);
#else
  //poll_gp1_end();
#endif

  *((volatile int *)TCSM1_P1_TASK_DONE) = 0x1;
  i_nop;  
  i_nop;    
  i_nop;      
  i_nop;  
  i_wait();

  return 0;
}
