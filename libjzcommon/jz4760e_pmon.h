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

#ifndef __JZ4760_PMON_H__
#define __JZ4760_PMON_H__
/* ===================================================================== */
/*  MAIN CPU PMON Macros                                                 */
/* ===================================================================== */
#include "jzasm.h"

#if defined(JZC_PMON_P0) || defined(JZC_PMON_P1)

#define PMON_GETRC() i_mfc0_2(16, 6)
#define PMON_GETLC() i_mfc0_2(16, 5)
#define PMON_GETRH() (i_mfc0_2(16, 4) & 0xFFFF)
#define PMON_GETLH() ((i_mfc0_2(16, 4)>>16) & 0xFFFF)

#ifdef STA_CCLK
#define PMON_ON(func)\
  ({  i_mtc0_2(0, 16, 4);\
    i_mtc0_2(0, 16, 5);\
    i_mtc0_2(0, 16, 6);\
    i_mtc0_2( (i_mfc0_2(16, 7) & 0xFFFF0FFF) | 0x0100, 16, 7);	\
  })
#define PMON_OFF(func)  i_mtc0_2(i_mfc0_2(16, 7) & ~0x100, 16, 7); func##_pmon_val += PMON_GETRC(); \
  func##_pmon_val_ex += PMON_GETLC();

#elif defined(STA_DCC)
#define PMON_ON(func)\
  ({  i_mtc0_2(0, 16, 4);\
    i_mtc0_2(0, 16, 5);\
    i_mtc0_2(0, 16, 6);\
    i_mtc0_2( (i_mfc0_2(16, 7) & 0xFFFF0FFF) | 0x1100, 16, 7);	\
  })
#define PMON_OFF(func)  i_mtc0_2(i_mfc0_2(16, 7) & ~0x100, 16, 7); func##_pmon_val += PMON_GETRC(); \
  func##_pmon_val_ex += PMON_GETLC();

#elif defined(STA_ICC)
#define PMON_ON(func)\
  ({  i_mtc0_2(0, 16, 4);\
    i_mtc0_2(0, 16, 5);\
    i_mtc0_2(0, 16, 6);\
    i_mtc0_2( (i_mfc0_2(16, 7) & 0xFFFF0FFF) | 0x1100, 16, 7);	\
  })
#define PMON_OFF(func)  i_mtc0_2(i_mfc0_2(16, 7) & ~0x100, 16, 7); func##_pmon_val += PMON_GETRC(); \
  func##_pmon_val_ex += PMON_GETLC();

#elif defined(STA_INSN)
#define PMON_ON(func)\
  ({  i_mtc0_2(0, 16, 4);\
    i_mtc0_2(0, 16, 5);\
    i_mtc0_2(0, 16, 6);\
    i_mtc0_2( (i_mfc0_2(16, 7) & 0xFFFF0FFF) | 0x2100, 16, 7);	\
  })
#define PMON_OFF(func)  i_mtc0_2(i_mfc0_2(16, 7) & ~0x100, 16, 7); func##_pmon_val += PMON_GETRC(); \
  func##_pmon_val_ex += PMON_GETLC();

#elif defined(STA_UINSN) /*useless instruction*/
#define PMON_ON(func)\
  ({  i_mtc0_2(0, 16, 4);\
    i_mtc0_2(0, 16, 5);\
    i_mtc0_2(0, 16, 6);\
    i_mtc0_2( (i_mfc0_2(16, 7) & 0xFFFF0FFF) | 0x2100, 16, 7);	\
  })
#define PMON_OFF(func)  i_mtc0_2(i_mfc0_2(16, 7) & ~0x100, 16, 7); func##_pmon_val += PMON_GETRC(); \
  func##_pmon_val_ex += PMON_GETLC();

#elif defined(STA_TLB) /*useless instruction*/
#define PMON_ON(func)\
  ({  i_mtc0_2(0, 16, 4);\
    i_mtc0_2(0, 16, 5);\
    i_mtc0_2(0, 16, 6);\
    i_mtc0_2( (i_mfc0_2(16, 7) & 0xFFFF0FFF) | 0x3100, 16, 7);	\
  })
#define PMON_OFF(func)  i_mtc0_2(i_mfc0_2(16, 7) & ~0x100, 16, 7); func##_pmon_val += PMON_GETRC(); \
  func##_pmon_val_ex += PMON_GETLC();
#endif

#define PMON_CREAT(func)   \
  uint32_t func##_pmon_val = 0; \
  uint32_t func##_pmon_val_ex = 0; \
  unsigned long long func##_pmon_val_1f = 0;

#define PMON_CLEAR(func)   \
  func##_pmon_val = 0;	   \
  func##_pmon_val_ex = 0;

#else //!(defined(JZC_PMON_P0) || defined(JZC_PMON_P1))

#define PMON_ON(func)
#define PMON_OFF(func)
#define PMON_CREAT(func)

#endif //(defined(JZC_PMON_P0) || defined(JZC_PMON_P1))


#endif /*__JZ4760_PMON_H__*/
