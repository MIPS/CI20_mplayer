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

#ifndef __JZ4760_DCSC_H__
#define __JZ4760_DCSC_H__

#ifdef _RTL_SIM_
# include "instructions.h"
#else
# include "jzasm.h"
#endif //_RTL_SIM_

#define P1_BASE_VADDR	    (0xB32A0000)
extern volatile unsigned char *aux_base;

#define P1_BASE_VADDR	    (aux_base)
#define AUX_START()               \
  ({						\
    *(volatile unsigned int *)P1_BASE_VADDR=1;	\
    *(volatile unsigned int *)P1_BASE_VADDR=(2 | 0x800000);	\
  })

#define AUX_RESET()               \
  ({						\
    *(volatile unsigned int *)P1_BASE_VADDR=1;	\
  })

#endif /* __JZ4760_DCSC_H__ */
