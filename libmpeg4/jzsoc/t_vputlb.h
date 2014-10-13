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

#ifndef __T_VPUTLB_H__
#define __T_VPUTLB_H__

extern volatile unsigned char *vpu_base;
#define VPUTLB_V_BASE       (int)vpu_base
//#define VPUTLB_V_BASE       (0xB3240000)

#define PSIZE_4M            0
#define PSIZE_8M            1
#define PSIZE_16M           2
#define PSIZE_32M           3

#define VALID_SFT           0
#define VALID_MSK           0x1
#define PSIZE_SFT           1
#define PSIZE_MSK           0x3
#define PTAG_SFT            12
#define PTAG_MSK            0x3FF
#define VTAG_SFT            22
#define VTAG_MSK            0x3FF

#define SET_VPU_TLB(entry, valid, psize, vtag, ptag)	 \
  ({ *(volatile int *)(VPUTLB_V_BASE + (entry)*4) = 	 \
      ((valid) & VALID_MSK)<<VALID_SFT |		 \
      ((psize) & PSIZE_MSK)<<PSIZE_SFT |		 \
      ((vtag ) & VTAG_MSK )<<VTAG_SFT  |                 \
      ((ptag ) & PTAG_MSK )<<PTAG_SFT  ;                 \
  })

#define GET_VPU_TLB(entry) (*(volatile int *)(VPUTLB_V_BASE + (entry)*4))

#define VPUCTRL_V_BASE      0xB3408000

#define SET_VPU_CTRL(ack, stop, rst)			 \
  ({ *(volatile int *)(VPUCTRL_V_BASE) =		 \
      ((ack ) & 0x1)<<2 |				 \
      ((stop) & 0x1)<<1 |				 \
      ((rst ) & 0x1)<<0 ;				 \
  })

#define GET_VPU_CTRL()     (*(volatile int *)(VPUCTRL_V_BASE))

#endif /*__T_MOTION_H__*/
