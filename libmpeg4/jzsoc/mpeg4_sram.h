/*
 * Copyright (C) 2006-2014 Ingenic Semiconductor CO.,LTD.
 *
 * This file is part of MPlayer.
 *
 * MPlayer is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * MPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with MPlayer; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

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
