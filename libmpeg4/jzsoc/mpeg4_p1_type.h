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
 * JZC MPEG4 Decoder Type Defines
 *
 ****************************************************************************/

#ifndef __JZC_MPEG4_AUX_TYPE_H__
#define __JZC_MPEG4_AUX_TYPE_H__

typedef short DCTELEM;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef int int32_t;
typedef short int16_t;
typedef char int8_t;

#define TRUE 1

#define I_TYPE  1 // Intra
#define P_TYPE  2 // Predicted
#define B_TYPE  3

#define CODEC_FLAG_GRAY         0x2000

int OFFTAB[4] = {0, 4, 32 , 36};
#define XCHG4(a,b,c,d,t)   t=a;a=b;b=c;c=d;d=t
#define XCHG3(a,b,c,t)   t=a;a=b;b=c;c=t
#define XCHG2(a,b,t)   t=a;a=b;b=t

#endif //__JZC_MPEG4_AUX_TYPE_H__
