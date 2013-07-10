/*****************************************************************************
 *
 * JZ4760 TCSM1 Space Seperate
 *
 * $Id: rv9_tcsm1.h,v 1.4 2012/11/14 02:30:09 kznan Exp $
 *
 ****************************************************************************/

#ifndef __RV9_TCSM1_H__
#define __RV9_TCSM1_H__

#define TCSM1_BANK0 0xF4000000
#define TCSM1_BANK1 0xF4001000
#define TCSM1_BANK2 0xF4002000
#define TCSM1_BANK3 0xF4003000

#define SPACE_HALF_MILLION_BYTE 0x8000

#define TCSM1_PADDR(a)        ((((unsigned)(a)) & 0xFFFF) | 0x132C0000) 
//#define TCSM1_VCADDR(a)       ((((unsigned)(a)) & 0xFFFF) | 0xB32C0000) 
extern volatile unsigned char * tcsm1_base;
#define TCSM1_VCADDR(a)       (tcsm1_base + (((unsigned)(a)) & 0xFFFF)) 

#define P1_MAIN_ADDR (TCSM1_BANK0)

#define TASK_FIFO_ADDR   (TCSM1_BANK2)
#define FIFO_ADDR_READY  (TASK_FIFO_ADDR + 4)
#define P1_CUR_DMB       (FIFO_ADDR_READY + 4)
#define TCSM1_SLICE_BUF  (P1_CUR_DMB + 4)
#define TCSM1_P1_FIFO_RP (TCSM1_SLICE_BUF+(SLICE_T_CC_LINE*32))
#define TCSM1_P1_TASK_DONE (TCSM1_P1_FIFO_RP + 4)

#define TCSM1_CMD_LEN           (16 << 2)
#define TCSM1_MBNUM_WP          (TCSM1_P1_TASK_DONE + 4)
#define TCSM1_DCORE_SHARE_ADDR  (TCSM1_MBNUM_WP+4)
#define TCSM1_FIRST_MBLEN       (TCSM1_DCORE_SHARE_ADDR+4)
#define TCSM1_DBG_RESERVE       (TCSM1_FIRST_MBLEN+4)
#define TCSM1_P1_STOP           TCSM1_DBG_RESERVE       
#define TCSM1_BUG_W             (TCSM1_P1_STOP + 4)
#define TCSM1_BUG_H             (TCSM1_BUG_W + 4)
#define TCSM1_BUG_TRIG          (TCSM1_BUG_H + 4)

/* ----------------------  TASK BUF  ------------------- */
#define TASK_BUF_LEN            ((sizeof(struct RV9_MB_DecARGs) + 3) & (~(0x3)))
#define CIRCLE_BUF_LEN          ((sizeof(struct RV9_MB_DecARGs) + 127) & (~(0x7f)))

#define TASK_BUF0               (TCSM1_MBNUM_WP + TCSM1_CMD_LEN)
#define TASK_BUF1               (TASK_BUF0+CIRCLE_BUF_LEN)
#define TASK_BUF2               (TASK_BUF1+CIRCLE_BUF_LEN)
#define TASK_BUF3               (TASK_BUF2+CIRCLE_BUF_LEN)
#define TASK_BUF_END            (TASK_BUF3+CIRCLE_BUF_LEN)

/* ------------------------  VDMA  --------------------- */
#define VDMA_CHAIN_LEN          (8<<2)
#define VDMA_CHAIN              ((TASK_BUF_END + 15) & (~(0xF)))
#define VDMA_END                (VDMA_CHAIN + VDMA_CHAIN_LEN)

/* -------------------------  MC  ---------------------- */
#define RV9_FIFO_DEP  3

#define MC_CHN_ONELEN           (35 << 2)
#define MC_CHN_DEP              RV9_FIFO_DEP
#define MC_CHN_LEN              (MC_CHN_ONELEN*MC_CHN_DEP)
#define TCSM1_MOTION_DHA        (VDMA_END + 4)
#define TCSM1_MOTION_DSA        (TCSM1_MOTION_DHA + MC_CHN_LEN)

/* ------------------------  VMAU  --------------------- */
#define VMAU_CHN_ONELEN         (9<<2)
#define VMAU_CHN_DEP            RV9_FIFO_DEP
#define VMAU_CHN_LEN            (VMAU_CHN_ONELEN*VMAU_CHN_DEP)
#define VMAU_DEC_END            (TCSM1_MOTION_DSA+0x4)
#define VMAU_CHN_BASE           (VMAU_DEC_END+0x4)
#define VMAU_END_FLAG           (VMAU_CHN_BASE+VMAU_CHN_LEN)

#define VMAU_YOUT_ADDR          (VMAU_END_FLAG + 4)
#define VMAU_COUT_ADDR          (VMAU_YOUT_ADDR + 256)
#define VMAU_YOUT_ADDR1         (VMAU_COUT_ADDR + 128)
#define VMAU_COUT_ADDR1         (VMAU_YOUT_ADDR1 + 256)
#define VMAU_YOUT_ADDR2         (VMAU_COUT_ADDR1 + 128)
#define VMAU_COUT_ADDR2         (VMAU_YOUT_ADDR2 + 256)
#define VMAU_END                (VMAU_COUT_ADDR2 + 128)

#define VMAU_YOUT_STR           (16)
#define VMAU_COUT_STR           (16)
#define PREVIOUS_LUMA_STRIDE    VMAU_YOUT_STR
#define PREVIOUS_CHROMA_STRIDE  VMAU_COUT_STR

/* ------------------------  DBLK  ---------------------- */
#define DBLK_CHAIN_LEN          (4<<2)
#define DBLK_ENDA               (VMAU_END)
#define DBLK_DMA_ENDA           (DBLK_ENDA + 4)
#define DBLK_CHAIN              (DBLK_DMA_ENDA + 4)
#define DBLK_END                (DBLK_CHAIN + DBLK_CHAIN_LEN)

#define TCSM1_MC_BUG_SPACE      DBLK_END

/* ------------------------  DEBUG  --------------------- */
#define DEBUG                   (TCSM1_MC_BUG_SPACE + 4)
#define EDGE_WIDTH 32
#endif 

