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
 * $Id: rv9_sram.h,v 1.2 2012/09/10 01:31:00 kznan Exp $
 *
 ****************************************************************************/

#ifndef __RV9_SRAM_H__
#define __RV9_SRAM_H__


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
#define SRAM_VUCADDR(a)       ((((unsigned)(a)) & 0xFFFF) | 0xB32F0000) 

#define SRAM_GP2_BUF          (SRAM_BANK0)
#endif /*__RV9_SRAM_H__*/
