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

#ifndef __JZ4760_SPU_H__
#define __JZ4760_SPU_H__

#ifdef JZC_RTL_SIM
# include "instructions.h"
#else
# include "jzasm.h"
#endif //JZC_RTL_SIM

#define SP_CTRL_DPT(val)       do{i_mtc0_2(val , 22, 0);}while(0)
#define SP_ADDR_DPT(val)       do{i_mtc0_2(val , 22, 1);}while(0)
#define SP_PUSH()              do{i_mtc0_2(0x80, 22, 2);}while(0)
#define SP_CLZ()               do{i_mtc0_2(0x0 , 22, 2);}while(0)
#define SP_DO()                do{i_mtc0_2(0x1 , 22, 2);}while(0)

#endif /*__JZ4760_SPU_H__*/
