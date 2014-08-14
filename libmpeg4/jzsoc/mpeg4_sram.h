/*****************************************************************************
 *
 * JZ4760 SRAM Space Seperate
 *
 * $Id: mpeg4_sram.h,v 1.3 2012/11/28 09:01:24 hpwang Exp $
 *
 ****************************************************************************/

#ifndef __MPEG4_SRAM_H__
#define __MPEG4_SRAM_H__


#define SRAM_BANK0  0x132F0000
#define SRAM_BANK1  0x132F1000
#define SRAM_BANK2  0x132F2000
#define SRAM_BANK3  0x132F3000
/*
  XXXX_PADDR:       physical address
  XXXX_VCADDR:      virtual cache-able address
  XXXX_VUCADDR:     virtual un-cache-able address 
*/
#define SRAM_PADDR(a)         ((((unsigned)(a)) & 0xFFFF) | 0x132F0000) 
#define SRAM_VCADDR(a)        ((((unsigned)(a)) & 0xFFFF) | 0xB32F0000) 
#define SRAM_VUCADDR(a)       (sram_base + ((a) & 0xFFFF))

#define SRAM_YBUF0    (SRAM_BANK1)
#define SRAM_UBUF0    (SRAM_YBUF0 + 256)

#define SRAM_YBUF1    (SRAM_UBUF0 + 256)
#define SRAM_UBUF1    (SRAM_YBUF1 + 256)

#define SRAM_YBUF3    (SRAM_UBUF1 + 256)
#define SRAM_UBUF3    (SRAM_YBUF3 + 256)

#define SRAM_BUF0               (SRAM_BANK2)
#define SRAM_BUF1               (SRAM_BUF0+TASK_BUF_LEN)
#define SRAM_BUF2               (SRAM_BUF1+TASK_BUF_LEN)
#define SRAM_BUF3               (SRAM_BUF2+TASK_BUF_LEN)

#define SRAM_VMAU_CHN           (SRAM_BANK2)
#define SRAM_VMAU_END           (SRAM_VMAU_CHN+40)
#define SRAM_DMA_CHN            (SRAM_BANK3)
#define SRAM_DBLK_CHAN_BASE     (SRAM_BANK3)

#define SRAM_MB0 SRAM_BANK0
#define SRAM_GP2_CHIN SRAM_MB0 + TASK_BUF_LEN

#endif /*__RV9_SRAM_H__*/
