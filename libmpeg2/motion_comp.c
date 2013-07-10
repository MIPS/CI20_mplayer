/*
 * motion_comp.c
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Modified for use with MPlayer, see libmpeg2_changes.diff for the exact changes.
 * detailed changelog at http://svn.mplayerhq.hu/mplayer/trunk/
 * $Id: motion_comp.c,v 1.1.1.1 2012/03/27 04:02:57 dqliu Exp $
 */

#include "config.h"

#include <inttypes.h>

#include "mpeg2.h"
#include "attributes.h"
#include "mpeg2_internal.h"

#ifdef JZC_MXU_OPT
#include "../libjzcommon/jzasm.h"
#include "../libjzcommon/jzmedia.h"
#endif

mpeg2_mc_t mpeg2_mc;

void mpeg2_mc_init (uint32_t accel)
{
#if HAVE_MMX2
    if (accel & MPEG2_ACCEL_X86_MMXEXT)
	mpeg2_mc = mpeg2_mc_mmxext;
    else
#endif
#if HAVE_AMD3DNOW
    if (accel & MPEG2_ACCEL_X86_3DNOW)
	mpeg2_mc = mpeg2_mc_3dnow;
    else
#endif
#if HAVE_MMX
    if (accel & MPEG2_ACCEL_X86_MMX)
	mpeg2_mc = mpeg2_mc_mmx;
    else
#endif
#if HAVE_ALTIVEC
    if (accel & MPEG2_ACCEL_PPC_ALTIVEC)
	mpeg2_mc = mpeg2_mc_altivec;
    else
#endif
#if ARCH_ALPHA
    if (accel & MPEG2_ACCEL_ALPHA)
	mpeg2_mc = mpeg2_mc_alpha;
    else
#endif
#if HAVE_VIS
    if (accel & MPEG2_ACCEL_SPARC_VIS)
	mpeg2_mc = mpeg2_mc_vis;
    else
#endif
#if ARCH_ARM
    if (accel & MPEG2_ACCEL_ARM)
	mpeg2_mc = mpeg2_mc_arm;
    else
#endif
	mpeg2_mc = mpeg2_mc_c;
}

#ifdef JZC_MXU_OPT
static void MC_put_o_16_c (uint8_t * dest, const uint8_t * ref, const int stride, int height){
  uint32_t  ref_aln, ref_rs;
  ref_aln = ((uint32_t)ref - stride) & 0xfffffffc;
  ref_rs  = 4 - (((uint32_t)ref) & 3);
  dest -= stride;
  do {
    S32LDIV(xr1,ref_aln,stride,0x0);
    S32LDD(xr2,ref_aln,0x4);
    S32LDD(xr4,ref_aln,0x8);
    S32LDD(xr6,ref_aln,0xc);
    S32LDD(xr8,ref_aln,0x10);
    
    S32ALN(xr3,xr2,xr1,ref_rs);
    S32ALN(xr5,xr4,xr2,ref_rs);
    S32ALN(xr7,xr6,xr4,ref_rs);
    S32ALN(xr9,xr8,xr6,ref_rs);
    
    S32SDIV(xr3,dest,stride,0x0);
    S32STD(xr5,dest,0x4);
    S32STD(xr7,dest,0x8);
    S32STD(xr9,dest,0xc);
  } while (--height); 
}

static void MC_put_o_8_c (uint8_t * dest, const uint8_t * ref, const int stride, int height){
  uint32_t  ref_aln, ref_rs;
  ref_aln = (uint32_t)(ref-stride) & 0xfffffffc;
  ref_rs  = 4 - ((uint32_t)(ref-stride) & 3);
  dest -= stride;
  do {
    S32LDIV(xr1,ref_aln,stride,0x0);
    S32LDD(xr2,ref_aln,0x4);
    S32LDD(xr4,ref_aln,0x8);
    S32ALN(xr3,xr2,xr1,ref_rs);
    S32ALN(xr5,xr4,xr2,ref_rs);
    
    S32SDIV(xr3,dest,stride,0x0);
    S32STD(xr5,dest,0x4);
  } while (--height);
}

static void MC_avg_o_16_c (uint8_t * dest, const uint8_t * ref, const int stride, int height){
  uint32_t  ref_aln, ref_rs, dst_aln, dst_rs;
  ref_aln = ((uint32_t)ref - stride) & 0xfffffffc;
  ref_rs  = 4 - (((uint32_t)ref) & 3);
  dst_aln = ((uint32_t)dest - stride) & 0xfffffffc;
  dst_rs  = 4 - (((uint32_t)dest) & 3); 
  dest -= stride;
  do {
    S32LDIV(xr1,ref_aln,stride,0x0);
    S32LDD(xr2,ref_aln,0x4);
    S32LDD(xr4,ref_aln,0x8);
    S32LDD(xr6,ref_aln,0xc);
    S32LDD(xr8,ref_aln,0x10);
    
    S32ALN(xr3,xr2,xr1,ref_rs);     //xr3:ref[3]->ref[0]
    S32ALN(xr5,xr4,xr2,ref_rs);     //xr5:ref[7]->ref[4]
    S32ALN(xr7,xr6,xr4,ref_rs);     //xr7:ref[11]->ref[8]
    S32ALN(xr9,xr8,xr6,ref_rs);     //xr9:ref[15]->ref[12]
    
    S32LDIV(xr1,dst_aln,stride,0x0);
    S32LDD(xr2,dst_aln,0x4);
    S32LDD(xr4,dst_aln,0x8);
    S32LDD(xr6,dst_aln,0xc);
    S32LDD(xr8,dst_aln,0x10);
    
    S32ALN(xr1,xr2,xr1,dst_rs);     //xr1:dst[3]->dst[0]
    S32ALN(xr2,xr4,xr2,dst_rs);     //xr2:dst[7]->dst[4]
    S32ALN(xr4,xr6,xr4,dst_rs);     //xr4:dst[11]->dst[8]
    S32ALN(xr6,xr8,xr6,dst_rs);     //xr6:dst[15]->dst[12]
    
    Q8AVGR(xr3,xr3,xr1);
    Q8AVGR(xr5,xr5,xr2);
    Q8AVGR(xr7,xr7,xr4);
    Q8AVGR(xr9,xr9,xr6);
    
    S32SDIV(xr3,dest,stride,0x0);
    S32STD(xr5,dest,0x4);
    S32STD(xr7,dest,0x8);
    S32STD(xr9,dest,0xc);
  } while (--height);  
} 

static void MC_avg_o_8_c (uint8_t * dest, const uint8_t * ref, const int stride, int height){
  uint32_t  ref_aln, ref_rs, dst_aln, dst_rs;
  ref_aln = ((uint32_t)ref - stride) & 0xfffffffc;
  ref_rs  = 4 - (((uint32_t)ref) & 3);
  dst_aln = ((uint32_t)dest - stride) & 0xfffffffc;
  dst_rs  = 4 - (((uint32_t)dest) & 3);
  dest -= stride;
  do {
    S32LDIV(xr1,ref_aln,stride,0x0);
    S32LDD(xr2,ref_aln,0x4);
    S32LDD(xr4,ref_aln,0x8);           
    
    S32LDIV(xr6,dst_aln,stride,0x0);
    S32LDD(xr7,dst_aln,0x4);
    S32LDD(xr8,dst_aln,0x8);
    
    
    S32ALN(xr3,xr2,xr1,ref_rs);     //xr3:ref[3]->ref[0]
    S32ALN(xr5,xr4,xr2,ref_rs);     //xr5:ref[7]->ref[4]
    
    S32ALN(xr6,xr7,xr6,dst_rs);     //xr6:dst[3]->dst[0]
    S32ALN(xr7,xr8,xr7,dst_rs);     //xr7:dst[7]->dst[4]
    
    Q8AVGR(xr3,xr3,xr6);
    Q8AVGR(xr5,xr5,xr7);

    S32SDIV(xr3,dest,stride,0x0);
    S32STD(xr5,dest,0x4);
  } while (--height);  
}

static void MC_put_x_16_c (uint8_t * dest, const uint8_t * ref, const int stride, int height){ 
  uint32_t ref_aln,ref_rs0,ref_rs1;
  ref_aln  = ((uint32_t)ref -  stride) & 0xFFFFFFFC;
  ref_rs0  = 4 - ((uint32_t)(ref) & 3);
  ref_rs1  = ref_rs0 - 1;
  dest -= stride;
  do {
    S32LDIV(xr1,ref_aln,stride,0x0);
    S32LDD(xr2,ref_aln,0x4);
    S32LDD(xr3,ref_aln,0x8);
    S32LDD(xr4,ref_aln,0xc);
    S32LDD(xr5,ref_aln,0x10);
    
    S32ALN(xr15,xr2,xr1,ref_rs0); //xr15:ref[3]-[0]
    S32ALN(xr14,xr3,xr2,ref_rs0); //xr14:ref[7]-[4]
    S32ALN(xr13,xr4,xr3,ref_rs0); //xr13:ref[11]-[8]
    S32ALN(xr12,xr5,xr4,ref_rs0); //xr12:ref[15]-[12]
    
    S32ALNI(xr11,xr14,xr15,3);
    S32ALNI(xr10,xr13,xr14,3);
    S32ALNI(xr9, xr12,xr13,3);
    S32ALN(xr8,xr5,xr4,ref_rs1);  //xr8:ref[16]-[13]
    
    Q8AVGR(xr5,xr15,xr11);
    Q8AVGR(xr6,xr14,xr10);
    Q8AVGR(xr7,xr13,xr9);
    Q8AVGR(xr8,xr12,xr8);
    
    S32SDIV(xr5,dest,stride,0x0);
    S32STD(xr6,dest,0x4);
    S32STD(xr7,dest,0x8);
    S32STD(xr8,dest,0xc);
  } while (--height);
}

static void MC_put_x_8_c (uint8_t * dest, const uint8_t * ref, const int stride, int height){ 
  int32_t ref_aln,ref_rs0,ref_rs1;
  ref_aln  = ((uint32_t)ref) & 0xFFFFFFFC;
  
  ref_rs0  = 4 - ((uint32_t)(ref) & 3);
  ref_rs1  = ref_rs0 - 1;
  ref_aln -= stride;
  dest -= stride;
  do {
    S32LDIV(xr1,ref_aln,stride,0x0);
    S32LDD(xr2,ref_aln,0x4);
    S32LDD(xr3,ref_aln,0x8);
    
    S32ALN(xr15,xr2,xr1,ref_rs0); //xr15:ref[3]-[0]
    S32ALN(xr14,xr3,xr2,ref_rs0); //xr14:ref[7]-[4]
    
    S32ALNI(xr5,xr14,xr15,3);
    S32ALN(xr6,xr3,xr2,ref_rs1); //xr6:ref[8]-[5]
    Q8AVGR(xr5,xr5,xr15);
    Q8AVGR(xr6,xr6,xr14);
    
    S32SDIV(xr5,dest,stride,0x0);
    S32STD(xr6,dest,0x4);
  } while (--height);
}

static void MC_avg_x_16_c (uint8_t * dest, const uint8_t * ref, const int stride, int height){
  uint32_t ref_aln,ref_rs0,ref_rs1 , dst_aln, dst_rs;
  ref_aln  = ((uint32_t)ref - stride) & 0xFFFFFFFC;
  ref_rs0  = 4 - ((uint32_t)(ref) & 3);
  ref_rs1  = ref_rs0 - 1;
  dst_aln  = ((uint32_t)dest - stride) & 0xFFFFFFFC;
  dst_rs  = 4 - ((uint32_t)(dest) & 3);
  
  dest -= stride;
  do {
    S32LDIV(xr1,ref_aln,stride,0x0);
    S32LDD(xr2,ref_aln,0x4);
    S32LDD(xr3,ref_aln,0x8);
    S32LDD(xr4,ref_aln,0xc);
    S32LDD(xr5,ref_aln,0x10);
    
    S32ALN(xr15,xr2,xr1,ref_rs0); //xr15:ref[3]-[0]
    S32ALN(xr11,xr2,xr1,ref_rs1); //xr11:ref[4]-[1]
    
    S32ALN(xr14,xr3,xr2,ref_rs0); //xr14:ref[7]-[4]
    S32ALN(xr10,xr3,xr2,ref_rs1); //xr10:ref[8]-[5]
    
    S32ALN(xr13,xr4,xr3,ref_rs0); //xr13:ref[11]-[8]
    S32ALN(xr9,xr4,xr3,ref_rs1);  //xr9:ref[12]-[9]
    
    S32ALN(xr12,xr5,xr4,ref_rs0); //xr12:ref[15]-[12]
    S32ALN(xr8,xr5,xr4,ref_rs1);  //xr8:ref[16]-[13]
    
    Q8AVGR(xr15,xr15,xr11);
    Q8AVGR(xr14,xr14,xr10);
    Q8AVGR(xr13,xr13,xr9);
    Q8AVGR(xr12,xr12,xr8);

    S32LDIV(xr1,dst_aln,stride,0x0);
    S32LDD(xr2,dst_aln,0x4);
    S32LDD(xr4,dst_aln,0x8);
    S32LDD(xr6,dst_aln,0xc);
    S32LDD(xr8,dst_aln,0x10);
    
    S32ALN(xr11,xr2,xr1,dst_rs);     //xr11:dst[3]->dst[0]
    S32ALN(xr10,xr4,xr2,dst_rs);     //xr10:dst[7]->dst[4]
    S32ALN(xr9,xr6,xr4,dst_rs);     //xr9:dst[11]->dst[8]
    S32ALN(xr8,xr8,xr6,dst_rs);     //xr8:dst[15]->dst[12]
    
    Q8AVGR(xr15,xr15,xr11);
    Q8AVGR(xr14,xr14,xr10);
    Q8AVGR(xr13,xr13,xr9);
    Q8AVGR(xr12,xr12,xr8);       
    
    S32SDIV(xr15,dest,stride,0x0);
    S32STD(xr14,dest,0x4);
    S32STD(xr13,dest,0x8);
    S32STD(xr12,dest,0xc);
  } while (--height);
}

static void MC_avg_x_8_c (uint8_t * dest, const uint8_t * ref, const int stride, int height){
  uint32_t ref_aln,ref_rs0,ref_rs1 , dst_aln, dst_rs;
  ref_aln  = ((uint32_t)ref - stride) & 0xFFFFFFFC;
  ref_rs0  = 4 - ((uint32_t)(ref) & 3);
  ref_rs1  = ref_rs0 - 1;
  dst_aln  = ((uint32_t)dest - stride) & 0xFFFFFFFC;
  dst_rs  = 4 - ((uint32_t)(dest) & 3);
  dest -= stride;
  do {
    S32LDIV(xr1,ref_aln,stride,0x0);
    S32LDD(xr2,ref_aln,0x4);
    S32LDD(xr3,ref_aln,0x8);
    
    S32LDIV(xr6,dst_aln,stride,0x0);
    S32LDD(xr5,dst_aln,0x4);
    S32LDD(xr4,dst_aln,0x8);
    S32ALN(xr15,xr2,xr1,ref_rs0); //xr15:ref[3]-[0]
    S32ALN(xr11,xr2,xr1,ref_rs1); //xr11:ref[4]-[1]
    
    S32ALN(xr14,xr3,xr2,ref_rs0); //xr14:ref[7]-[4]
    S32ALN(xr10,xr3,xr2,ref_rs1); //xr10:ref[8]-[5]
    
    Q8AVGR(xr15,xr15,xr11);
    Q8AVGR(xr14,xr14,xr10);
    S32ALN(xr1,xr5,xr6,dst_rs);     //xr1:dst[3]->dst[0]
    S32ALN(xr2,xr4,xr5,dst_rs);     //xr2:dst[7]->dst[4]
    
    Q8AVGR(xr15,xr15,xr1);
    Q8AVGR(xr14,xr14,xr2);
    
    S32SDIV(xr15,dest,stride,0x0);
    S32STD(xr14,dest,0x4);
  } while (--height);
}

static void MC_put_y_16_c (uint8_t * dest, const uint8_t * ref, const int stride, int height){ 
  int32_t ref_aln,ref_rs;
  ref_aln = ((uint32_t)ref) & 0xFFFFFFFC;
  ref_rs  = 4 - ((uint32_t)ref &  3);
  dest -= stride;
  S32LDD(xr1,ref_aln,0x0);
  S32LDD(xr2,ref_aln,0x4);
  S32LDD(xr3,ref_aln,0x8);
  S32LDD(xr4,ref_aln,0xc);
  S32LDD(xr5,ref_aln,0x10);
  
  S32ALN(xr6,xr2,xr1,ref_rs); //xr6:ref[3]-[0]
  S32ALN(xr7,xr3,xr2,ref_rs); //xr7:ref[7]-[4]
  S32ALN(xr8,xr4,xr3,ref_rs); //xr8:ref[11]-[8]
  S32ALN(xr9,xr5,xr4,ref_rs); //xr9:ref[15]-[12]
  do {
    S32LDIV(xr1,ref_aln,stride,0x0);
    S32LDD(xr2,ref_aln,0x4);
    S32LDD(xr3,ref_aln,0x8);
    S32LDD(xr4,ref_aln,0xc);
    S32LDD(xr5,ref_aln,0x10);
    
    S32ALN(xr10,xr2,xr1,ref_rs); //xr10:ref[3+stride]-[0+stride]
    S32ALN(xr11,xr3,xr2,ref_rs); //xr11:ref[7+stride]-[4+stride]
    S32ALN(xr12,xr4,xr3,ref_rs); //xr12:ref[11+stride]-[8+stride]
    S32ALN(xr13,xr5,xr4,ref_rs); //xr13:ref[15+stride]-[12+stride]
    
    Q8AVGR(xr1,xr6,xr10);
    Q8AVGR(xr2,xr7,xr11);
    Q8AVGR(xr3,xr8,xr12);
    Q8AVGR(xr4,xr9,xr13);
    
    S32SDIV(xr1,dest,stride,0x0);
    S32STD(xr2,dest,0x4);
    S32STD(xr3,dest,0x8);
    S32STD(xr4,dest,0xc);
    
    S32ALNI(xr6,xr10,xr0,0);
    S32ALNI(xr7,xr11,xr0,0);
    S32ALNI(xr8,xr12,xr0,0);
    S32ALNI(xr9,xr13,xr0,0);
    
  } while (--height); 
}

static void MC_put_y_8_c (uint8_t * dest, const uint8_t * ref, const int stride, int height){
  int32_t ref_aln,ref_rs;
  ref_aln = ((uint32_t)ref) & 0xFFFFFFFC;
  ref_rs  = 4 - ((uint32_t)ref &  3);
  dest -= stride;
  S32LDD(xr1,ref_aln,0x0);
  S32LDD(xr2,ref_aln,0x4);
  S32LDD(xr3,ref_aln,0x8);
  
  S32ALN(xr4,xr2,xr1,ref_rs); //xr4:ref[3]-[0]
  S32ALN(xr5,xr3,xr2,ref_rs); //xr5:ref[7]-[4]
  do {
    S32LDIV(xr1,ref_aln,stride,0x0);
    S32LDD(xr2,ref_aln,0x4);
    S32LDD(xr3,ref_aln,0x8);
    
    S32ALN(xr6,xr2,xr1,ref_rs); //xr6:ref[3+stride]-[0+stride]
    S32ALN(xr7,xr3,xr2,ref_rs); //xr7:ref[7+stride]-[4+stride]
    
    Q8AVGR(xr1,xr6,xr4);
    Q8AVGR(xr2,xr7,xr5);
    
    S32SDIV(xr1,dest,stride,0x0);
    S32STD(xr2,dest,0x4);
    
    S32ALNI(xr4,xr6,xr0,0);
    S32ALNI(xr5,xr7,xr0,0);
  } while (--height);
}

static void MC_avg_y_16_c (uint8_t * dest, const uint8_t * ref, const int stride, int height){
  int32_t ref_aln,ref_rs,dst_aln,dst_rs;
  ref_aln = ((uint32_t)ref) & 0xFFFFFFFC;
  ref_rs  = 4 - ((uint32_t)ref &  3);
  dst_aln  = ((uint32_t)dest - stride) & 0xFFFFFFFC;
  dst_rs  = 4 - ((uint32_t)(dest) & 3);
  dest -= stride;
  S32LDD(xr1,ref_aln,0x0);
  S32LDD(xr2,ref_aln,0x4);
  S32LDD(xr3,ref_aln,0x8);
  S32LDD(xr4,ref_aln,0xc);
  S32LDD(xr5,ref_aln,0x10);
  
  S32ALN(xr6,xr2,xr1,ref_rs); //xr6:ref[3]-[0]
  S32ALN(xr7,xr3,xr2,ref_rs); //xr7:ref[7]-[4]
  S32ALN(xr8,xr4,xr3,ref_rs); //xr8:ref[11]-[8]
  S32ALN(xr9,xr5,xr4,ref_rs); //xr9:ref[15]-[12]
  do {
    S32LDIV(xr1,ref_aln,stride,0x0);
    S32LDD(xr2,ref_aln,0x4);
    S32LDD(xr3,ref_aln,0x8);
    S32LDD(xr4,ref_aln,0xc);
    S32LDD(xr5,ref_aln,0x10);
    
    S32ALN(xr10,xr2,xr1,ref_rs); //xr10:ref[3+stride]-[0+stride]
    S32ALN(xr11,xr3,xr2,ref_rs); //xr11:ref[7+stride]-[4+stride]
    S32ALN(xr12,xr4,xr3,ref_rs); //xr12:ref[11+stride]-[8+stride]
    S32ALN(xr13,xr5,xr4,ref_rs); //xr13:ref[15+stride]-[12+stride]
    
    Q8AVGR(xr6,xr6,xr10);
    Q8AVGR(xr7,xr7,xr11);
    Q8AVGR(xr8,xr8,xr12);
    Q8AVGR(xr9,xr9,xr13);
    
    S32LDIV(xr1,dst_aln,stride,0x0);
    S32LDD(xr2,dst_aln,0x4);
    S32LDD(xr3,dst_aln,0x8);
    S32LDD(xr4,dst_aln,0xc);
    S32LDD(xr5,dst_aln,0x10);
    
    S32ALN(xr1,xr2,xr1,dst_rs);     //xr1:dst[3]->dst[0]
    S32ALN(xr2,xr3,xr2,dst_rs);     //xr2:dst[7]->dst[4]
    S32ALN(xr3,xr4,xr3,dst_rs);     //xr3:dst[11]->dst[8]
    S32ALN(xr4,xr5,xr4,dst_rs);     //xr4:dst[15]->dst[12]
    
    Q8AVGR(xr1,xr6,xr1);
    Q8AVGR(xr2,xr7,xr2);
    Q8AVGR(xr3,xr8,xr3);
    Q8AVGR(xr4,xr9,xr4);
    
    S32SDIV(xr1,dest,stride,0x0);
    S32STD(xr2,dest,0x4);
    S32STD(xr3,dest,0x8);
    S32STD(xr4,dest,0xc);
    
    S32ALNI(xr6,xr10,xr0,0);
    S32ALNI(xr7,xr11,xr0,0);
    S32ALNI(xr8,xr12,xr0,0);
    S32ALNI(xr9,xr13,xr0,0);
  } while (--height);
}

static void MC_avg_y_8_c (uint8_t * dest, const uint8_t * ref, const int stride, int height){
  int32_t ref_aln,ref_rs,dst_aln,dst_rs;
  ref_aln = ((uint32_t)ref) & 0xFFFFFFFC;
  ref_rs  = 4 - ((uint32_t)ref &  3);
  dst_aln  = ((uint32_t)dest - stride) & 0xFFFFFFFC;
  dst_rs  = 4 - ((uint32_t)(dest) & 3);
  dest -= stride;
  S32LDD(xr1,ref_aln,0x0);
  S32LDD(xr2,ref_aln,0x4);
  S32LDD(xr3,ref_aln,0x8);
  
  S32ALN(xr6,xr2,xr1,ref_rs); //xr6:ref[3]-[0]
  S32ALN(xr7,xr3,xr2,ref_rs); //xr7:ref[7]-[4]
  
  do {
    S32LDIV(xr1,ref_aln,stride,0x0);
    S32LDD(xr2,ref_aln,0x4);
    S32LDD(xr3,ref_aln,0x8);
    
    S32ALN(xr10,xr2,xr1,ref_rs); //xr10:ref[3+stride]-[0+stride]
    S32ALN(xr11,xr3,xr2,ref_rs); //xr11:ref[7+stride]-[4+stride]
    
    Q8AVGR(xr6,xr6,xr10);
    Q8AVGR(xr7,xr7,xr11);
    
    S32LDIV(xr1,dst_aln,stride,0x0);
    S32LDD(xr2,dst_aln,0x4);
    S32LDD(xr3,dst_aln,0x8);
    S32ALN(xr1,xr2,xr1,dst_rs);     //xr1:dst[3]->dst[0]
    S32ALN(xr2,xr3,xr2,dst_rs);     //xr2:dst[7]->dst[4]
     
    Q8AVGR(xr1,xr6,xr1);
    Q8AVGR(xr2,xr7,xr2);
    
    S32SDIV(xr1,dest,stride,0x0);
    S32STD(xr2,dest,0x4);
    
    S32ALNI(xr6,xr10,xr0,0);
    S32ALNI(xr7,xr11,xr0,0);
  } while (--height);
}

static void MC_put_xy_16_c (uint8_t * dest, const uint8_t * ref, const int stride, int height){ 
  uint32_t ref_aln,ref_aln2,ref_rs,ref_rs2;
  ref_aln  = ((uint32_t)ref - stride) & 0xFFFFFFFC;
  ref_aln2 = ((uint32_t)ref ) & 0xFFFFFFFC;
  
  ref_rs   = 4 - ((uint32_t)ref & 3);
  ref_rs2  = 4 - (((uint32_t)ref + stride) & 3);
  S32I2M(xr15,0x02020202);
  dest -= stride;
  do { 
    S32LDIV(xr1,ref_aln,stride,0x0);
    S32LDD(xr2,ref_aln,0x4);
    S32LDD(xr3,ref_aln,0x8);
    S32LDD(xr4,ref_aln,0xc);
    S32LDD(xr5,ref_aln,0x10);
    S32LDD(xr6,ref_aln,0x14);       
     
    S32ALN(xr1,xr2,xr1,ref_rs);  //xr1:ref[3]-[0]
    S32ALN(xr2,xr3,xr2,ref_rs);  //xr2:ref[7]-[4]
    S32ALN(xr3,xr4,xr3,ref_rs);  //xr3:ref[11]-[8] 
    S32ALN(xr4,xr5,xr4,ref_rs);  //xr4:ref[15]-[12]
    S32ALN(xr5,xr6,xr5,ref_rs);  //xr5:ref[19]-[16]   
    S32ALNI(xr8,xr5,xr4,3);       //xr8:ref[16]-[13]
    S32ALNI(xr7,xr4,xr3,3);       //xr7:ref[12]-[9]
    S32ALNI(xr6,xr3,xr2,3);       //xr6:ref[8]-[5]
    S32ALNI(xr5,xr2,xr1,3);       //xr5:ref[4]-[1]
    
    Q8ADDE_AA(xr1,xr1,xr5,xr14);
    Q8ADDE_AA(xr2,xr2,xr6,xr13);      
    Q8ADDE_AA(xr3,xr3,xr7,xr12);
    Q8ADDE_AA(xr4,xr4,xr8,xr11);
    
    S32LDIV(xr5,ref_aln2,stride,0x0);
    S32LDD(xr6,ref_aln2,0x4);
    S32LDD(xr7,ref_aln2,0x8);
    S32LDD(xr8,ref_aln2,0xc);
    S32LDD(xr9,ref_aln2,0x10);
    S32LDD(xr10,ref_aln2,0x14);
    
    S32ALN(xr5,xr6,xr5,ref_rs2);  //xr5:ref[stride+3]-[stride+0]
    S32ALN(xr6,xr7,xr6,ref_rs2);  //xr6:ref[stride+7]-[stride+4]
    S32ALN(xr7,xr8,xr7,ref_rs2);  //xr7:ref[stride+11]-[stride+8]
    S32ALN(xr8,xr9,xr8,ref_rs2);  //xr8:ref[stride+15]-[stride+12]
    S32ALN(xr9,xr10,xr9,ref_rs2); //xr9:
    
    Q8ACCE_AA(xr1,xr15,xr5,xr14);
    Q8ACCE_AA(xr2,xr15,xr6,xr13);
    Q8ACCE_AA(xr3,xr15,xr7,xr12);
    Q8ACCE_AA(xr4,xr15,xr8,xr11);
    S32ALNI(xr5,xr6,xr5,3);
    S32ALNI(xr6,xr7,xr6,3);
    S32ALNI(xr7,xr8,xr7,3);
    S32ALNI(xr8,xr9,xr8,3);        
    
    Q8ACCE_AA(xr1,xr0,xr5,xr14);
    Q8ACCE_AA(xr2,xr0,xr6,xr13);
    Q8ACCE_AA(xr3,xr0,xr7,xr12);
    Q8ACCE_AA(xr4,xr0,xr8,xr11);
    
    Q16SAR(xr1,xr1,xr14,xr14,2);
    Q16SAR(xr2,xr2,xr13,xr13,2);
    Q16SAR(xr3,xr3,xr12,xr12,2);
    Q16SAR(xr4,xr4,xr11,xr11,2);
    
    Q16SAT(xr1,xr1,xr14);
    Q16SAT(xr2,xr2,xr13);
    Q16SAT(xr3,xr3,xr12);
    Q16SAT(xr4,xr4,xr11);
    S32SDIV(xr1,dest,stride,0x0);
    S32STD(xr2,dest,0x4);
    S32STD(xr3,dest,0x8);
    S32STD(xr4,dest,0xc);
  } while (--height);
}

static void MC_put_xy_8_c (uint8_t * dest, const uint8_t * ref, const int stride, int height){ 
  uint32_t ref_aln, ref_rs, ref_rs1;
  ref_aln = ((uint32_t)ref ) & 0xFFFFFFFC;
  
  ref_rs = 4 - ((uint32_t)ref & 3);
  ref_rs1 = ref_rs - 1;
  S32I2M(xr15,0x02020202);
  dest -= stride;
  S32LDD(xr1,ref_aln,0x0);
  S32LDD(xr2,ref_aln,0x4);
  S32LDD(xr3,ref_aln,0x8);
  
  S32ALN(xr4,xr2,xr1,ref_rs);  //xr4:ref[3]-[0]
  S32ALN(xr5,xr3,xr2,ref_rs);  //xr5:ref[7]-[4]
  
  S32ALN(xr6,xr2,xr1,ref_rs1); //xr6:ref[4]-[1]
  S32ALN(xr7,xr3,xr2,ref_rs1); //xr7:ref[8]-[5]
  do {
    S32LDIV(xr1,ref_aln,stride,0x0);
    S32LDD(xr2,ref_aln,0x4);
    S32LDD(xr3,ref_aln,0x8);
    
    S32ALN(xr8,xr2,xr1,ref_rs);  //xr8:ref[stride+3]-[stride+0]
    S32ALN(xr9,xr3,xr2,ref_rs);  //xr9:ref[stride+7]-[stride+4]
    
    S32ALN(xr10,xr2,xr1,ref_rs1);   //xr10:ref[stride+4]-[stride+1]
    S32ALN(xr11,xr3,xr2,ref_rs1);   //xr11:ref[stride+8]-[stride+5]
    
    Q8ADDE_AA(xr1,xr4,xr6,xr14);
    Q8ADDE_AA(xr13,xr5,xr7,xr12);
    Q8ACCE_AA(xr1,xr8,xr10,xr14);
    Q8ACCE_AA(xr13,xr9,xr11,xr12);
    Q8ACCE_AA(xr1,xr15,xr0,xr14);
    Q8ACCE_AA(xr13,xr15,xr0,xr12);
    Q16SAR(xr1,xr1,xr14,xr14,2);
    Q16SAR(xr13,xr13,xr12,xr12,2);
    
    Q16SAT(xr1,xr1,xr14);
    Q16SAT(xr2,xr13,xr12);
    S32SDIV(xr1,dest,stride,0x0);
    S32STD(xr2,dest,0x4);
    
    S32ALNI(xr4,xr8,xr0,0);
    S32ALNI(xr5,xr9,xr0,0);
    S32ALNI(xr6,xr10,xr0,0);
    S32ALNI(xr7,xr11,xr0,0);
  }while (--height); 
}

static void MC_avg_xy_16_c (uint8_t * dest, const uint8_t * ref, const int stride, int height){
  uint32_t ref_aln,ref_aln2,ref_rs,ref_rs2,dst_aln,dst_rs;
  ref_aln  = ((uint32_t)ref - stride) & 0xFFFFFFFC;
  ref_aln2 = ((uint32_t)ref ) & 0xFFFFFFFC;
  ref_rs   = 4 - ((uint32_t)ref & 3);
  ref_rs2  = 4 - (((uint32_t)ref + stride) & 3);
  
  dst_aln  = ((uint32_t)dest - stride) & 0xFFFFFFFC;
  dst_rs  = 4 - ((uint32_t)(dest) & 3);
  S32I2M(xr15,0x02020202);
  dest -= stride;
  do {
    S32LDIV(xr1,ref_aln,stride,0x0);
    S32LDD(xr2,ref_aln,0x4);
    S32LDD(xr3,ref_aln,0x8);
    S32LDD(xr4,ref_aln,0xc);
    S32LDD(xr5,ref_aln,0x10);
    S32LDD(xr6,ref_aln,0x14);
    
    S32ALN(xr1,xr2,xr1,ref_rs);  //xr1:ref[3]-[0]
    S32ALN(xr2,xr3,xr2,ref_rs);  //xr2:ref[7]-[4]
    S32ALN(xr3,xr4,xr3,ref_rs);  //xr3:ref[11]-[8]
    S32ALN(xr4,xr5,xr4,ref_rs);  //xr4:ref[15]-[12]
    S32ALN(xr5,xr6,xr5,ref_rs);  //xr5:ref[19]-[16]
    
    S32ALNI(xr8,xr5,xr4,3);       //xr8:ref[16]-[13]
    S32ALNI(xr7,xr4,xr3,3);       //xr7:ref[12]-[9] 
    S32ALNI(xr6,xr3,xr2,3);       //xr6:ref[8]-[5]
    S32ALNI(xr5,xr2,xr1,3);       //xr5:ref[4]-[1]

    Q8ADDE_AA(xr1,xr1,xr5,xr14);
    Q8ADDE_AA(xr2,xr2,xr6,xr13);
    Q8ADDE_AA(xr3,xr3,xr7,xr12);
    Q8ADDE_AA(xr4,xr4,xr8,xr11);
    
    S32LDIV(xr5,ref_aln2,stride,0x0);
    S32LDD(xr6,ref_aln2,0x4);
    S32LDD(xr7,ref_aln2,0x8);
    S32LDD(xr8,ref_aln2,0xc);
    S32LDD(xr9,ref_aln2,0x10);
    S32LDD(xr10,ref_aln2,0x14);
    
    S32ALN(xr5,xr6,xr5,ref_rs2);  //xr5:ref[stride+3]-[stride+0]
    S32ALN(xr6,xr7,xr6,ref_rs2);  //xr6:ref[stride+7]-[stride+4]
    S32ALN(xr7,xr8,xr7,ref_rs2);  //xr7:ref[stride+11]-[stride+8]
    S32ALN(xr8,xr9,xr8,ref_rs2);  //xr8:ref[stride+15]-[stride+12]
    S32ALN(xr9,xr10,xr9,ref_rs2); //xr9: 
    
    Q8ACCE_AA(xr1,xr15,xr5,xr14);
    Q8ACCE_AA(xr2,xr15,xr6,xr13);
    Q8ACCE_AA(xr3,xr15,xr7,xr12);
    Q8ACCE_AA(xr4,xr15,xr8,xr11);
    
    S32ALNI(xr5,xr6,xr5,3);
    S32ALNI(xr6,xr7,xr6,3);
    S32ALNI(xr7,xr8,xr7,3);
    S32ALNI(xr8,xr9,xr8,3);

    Q8ACCE_AA(xr1,xr0,xr5,xr14);
    Q8ACCE_AA(xr2,xr0,xr6,xr13);
    Q8ACCE_AA(xr3,xr0,xr7,xr12);
    Q8ACCE_AA(xr4,xr0,xr8,xr11);
    
    Q16SAR(xr1,xr1,xr14,xr14,2);
    Q16SAR(xr2,xr2,xr13,xr13,2);
    Q16SAR(xr3,xr3,xr12,xr12,2);
    Q16SAR(xr4,xr4,xr11,xr11,2);

    Q16SAT(xr14,xr1,xr14);
    Q16SAT(xr13,xr2,xr13);
    Q16SAT(xr12,xr3,xr12);
    Q16SAT(xr11,xr4,xr11);
    
    S32LDIV(xr1,dst_aln,stride,0x0);
    S32LDD(xr2,dst_aln,0x4);
    S32LDD(xr4,dst_aln,0x8);
    S32LDD(xr6,dst_aln,0xc);
    S32LDD(xr8,dst_aln,0x10);

    S32ALN(xr1,xr2,xr1,dst_rs);     //xr1:dst[3]->dst[0]
    S32ALN(xr2,xr4,xr2,dst_rs);     //xr2:dst[7]->dst[4]
    S32ALN(xr4,xr6,xr4,dst_rs);     //xr4:dst[11]->dst[8]
    S32ALN(xr6,xr8,xr6,dst_rs);     //xr6:dst[15]->dst[12]
    
    Q8AVGR(xr1,xr14,xr1);
    Q8AVGR(xr2,xr13,xr2);
    Q8AVGR(xr3,xr12,xr4);
    Q8AVGR(xr4,xr11,xr6);
    
    S32SDIV(xr1,dest,stride,0x0);
    S32STD(xr2,dest,0x4);
    S32STD(xr3,dest,0x8);
    S32STD(xr4,dest,0xc);
  } while (--height);
}

static void MC_avg_xy_8_c (uint8_t * dest, const uint8_t * ref, const int stride, int height){
  uint32_t ref_aln, ref_rs, ref_rs1, dst_rs, dst_aln;
  ref_aln = ((uint32_t)ref ) & 0xFFFFFFFC;
  ref_rs = 4 - ((uint32_t)ref & 3);
  dst_aln  = ((uint32_t)dest - stride) & 0xFFFFFFFC;
  dst_rs  = 4 - ((uint32_t)(dest) & 3);
  ref_rs1 = ref_rs - 1;
  S32I2M(xr15,0x02020202);
  dest -= stride;
  S32LDD(xr1,ref_aln,0x0);
  S32LDD(xr2,ref_aln,0x4);
  S32LDD(xr3,ref_aln,0x8);
  
  S32ALN(xr4,xr2,xr1,ref_rs);  //xr4:ref[3]-[0]
  S32ALN(xr5,xr3,xr2,ref_rs);  //xr5:ref[7]-[4]
  
  S32ALN(xr6,xr2,xr1,ref_rs1); //xr6:ref[4]-[1]
  S32ALN(xr7,xr3,xr2,ref_rs1); //xr7:ref[8]-[5]
  do {
    S32LDIV(xr1,ref_aln,stride,0x0);
    S32LDD(xr2,ref_aln,0x4);
    S32LDD(xr3,ref_aln,0x8);
    
    S32ALN(xr8,xr2,xr1,ref_rs);  //xr8:ref[stride+3]-[stride+0]
    S32ALN(xr9,xr3,xr2,ref_rs);  //xr9:ref[stride+7]-[stride+4]
    
    S32ALN(xr10,xr2,xr1,ref_rs1);   //xr10:ref[stride+4]-[stride+1]
    S32ALN(xr11,xr3,xr2,ref_rs1);   //xr11:ref[stride+8]-[stride+5]
    
    Q8ADDE_AA(xr1,xr4,xr6,xr14);
    Q8ADDE_AA(xr13,xr5,xr7,xr12);
    Q8ACCE_AA(xr1,xr8,xr10,xr14);
    Q8ACCE_AA(xr13,xr9,xr11,xr12);
    Q8ACCE_AA(xr1,xr15,xr0,xr14);
    Q8ACCE_AA(xr13,xr15,xr0,xr12);
    
    Q16SAR(xr1,xr1,xr14,xr14,2);
    Q16SAR(xr13,xr13,xr12,xr12,2);
    Q16SAT(xr14,xr1,xr14);
    Q16SAT(xr12,xr13,xr12);
    
    S32LDIV(xr1,dst_aln,stride,0x0);
    S32LDD(xr2,dst_aln,0x4);
    S32LDD(xr4,dst_aln,0x8);
    S32ALN(xr1,xr2,xr1,dst_rs);     //xr1:dst[3]->dst[0]
    S32ALN(xr2,xr4,xr2,dst_rs);     //xr2:dst[7]->dst[4]
    
    Q8AVGR(xr1,xr14,xr1);
    Q8AVGR(xr2,xr12,xr2);
    
    S32SDIV(xr1,dest,stride,0x0);
    S32STD(xr2,dest,0x4);

    S32ALNI(xr4,xr8,xr0,0);
    S32ALNI(xr5,xr9,xr0,0);
    S32ALNI(xr6,xr10,xr0,0);
    S32ALNI(xr7,xr11,xr0,0);
  }while (--height);
}

mpeg2_mc_t mpeg2_mc_c = { {MC_put_o_16_c, MC_put_x_16_c, MC_put_y_16_c, MC_put_xy_16_c, MC_put_o_8_c, MC_put_x_8_c, MC_put_y_8_c, MC_put_xy_8_c}, {MC_avg_o_16_c, MC_avg_x_16_c, MC_avg_y_16_c, MC_avg_xy_16_c, MC_avg_o_8_c, MC_avg_x_8_c, MC_avg_y_8_c, MC_avg_xy_8_c} };
#else
#define avg2(a,b) ((a+b+1)>>1)
#define avg4(a,b,c,d) ((a+b+c+d+2)>>2)

#define predict_o(i) (ref[i])
#define predict_x(i) (avg2 (ref[i], ref[i+1]))
#define predict_y(i) (avg2 (ref[i], (ref+stride)[i]))
#define predict_xy(i) (avg4 (ref[i], ref[i+1], \
			     (ref+stride)[i], (ref+stride)[i+1]))

#define put(predictor,i) dest[i] = predictor (i)
#define avg(predictor,i) dest[i] = avg2 (predictor (i), dest[i])

/* mc function template */

#define MC_FUNC(op,xy)							\
static void MC_##op##_##xy##_16_c (uint8_t * dest, const uint8_t * ref,	\
				   const int stride, int height)	\
{									\
    do {								\
	op (predict_##xy, 0);						\
	op (predict_##xy, 1);						\
	op (predict_##xy, 2);						\
	op (predict_##xy, 3);						\
	op (predict_##xy, 4);						\
	op (predict_##xy, 5);						\
	op (predict_##xy, 6);						\
	op (predict_##xy, 7);						\
	op (predict_##xy, 8);						\
	op (predict_##xy, 9);						\
	op (predict_##xy, 10);						\
	op (predict_##xy, 11);						\
	op (predict_##xy, 12);						\
	op (predict_##xy, 13);						\
	op (predict_##xy, 14);						\
	op (predict_##xy, 15);						\
	ref += stride;							\
	dest += stride;							\
    } while (--height);							\
}									\
static void MC_##op##_##xy##_8_c (uint8_t * dest, const uint8_t * ref,	\
				  const int stride, int height)		\
{									\
    do {								\
	op (predict_##xy, 0);						\
	op (predict_##xy, 1);						\
	op (predict_##xy, 2);						\
	op (predict_##xy, 3);						\
	op (predict_##xy, 4);						\
	op (predict_##xy, 5);						\
	op (predict_##xy, 6);						\
	op (predict_##xy, 7);						\
	ref += stride;							\
	dest += stride;							\
    } while (--height);							\
}

/* definitions of the actual mc functions */

MC_FUNC (put,o)
MC_FUNC (avg,o)
MC_FUNC (put,x)
MC_FUNC (avg,x)
MC_FUNC (put,y)
MC_FUNC (avg,y)
MC_FUNC (put,xy)
MC_FUNC (avg,xy)

MPEG2_MC_EXTERN (c)
#endif
