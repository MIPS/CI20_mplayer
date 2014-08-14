/*
 * H.263 decoder
 * Copyright (c) 2001 Fabrice Bellard
 * Copyright (c) 2002-2004 Michael Niedermayer <michaelni@gmx.at>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * H.263 decoder.
 */

#include "libavutil/cpu.h"
#include "internal.h"
#include "avcodec.h"
#include "dsputil.h"
#include "mpegvideo.h"
#include "h263.h"
#include "h263_parser.h"
#include "mpeg4video_parser.h"
#include "msmpeg4.h"
#include "vdpau_internal.h"
#include "flv.h"
#include "mpeg4video.h"
#include "mpeg4.h"

#include "jzsoc/jzmedia.h"
#include "jzsoc/jzasm.h"

#ifdef JZC_P1_OPT
#include "jzsoc/jzmedia.h"
#include "jzsoc/mpeg4_dcore.h"

#include "jzsoc/jzasm.h"
//#include "jzsoc/mpeg4_tcsm0.h"
#include "jzsoc/mpeg4_tcsm1.h"
#include "jzsoc/mpeg4_dblk.h"
#include "./jzsoc/jz4760_dcsc.h"
//#include "../libjzcommon/jz4760e_tcsm_init.c"
#include "jzsoc/jz4760_tcsm_init.c"

//mpeg4 motion
#include "jzsoc/t_motion.h"
#include "jzsoc/t_intpid.h"
#include "jzsoc/t_vputlb.h"

#ifdef JZC_PMON_P0
#include "../libjzcommon/jz4760e_pmon.h"
#endif
#include "../libjzcommon/jzasm.h"

MPEG4_MB_DecARGs *memmb1;
MPEG4_MB_DecARGs *memmb2;
MPEG4_MB_DecARGs *memmb3;
int memmb_use;
char *pmem = NULL;

MPEG4_MB_DecARGs *dMB;
MPEG4_Frame_GlbARGs dFRM;
MPEG4_Frame_GlbARGs *t1_dFRM;
#endif

#ifdef JZC_DCORE_OPT
#include "jzsoc/jz4760_2ddma_hw.h"
#endif //JZC_DCORE_OPT

#ifdef JZC_P1_OPT
extern char * dcore_sh_buf;
volatile int * task_fifo_wp;
volatile int * task_fifo_wp_d1;
volatile int * task_fifo_wp_d2;
volatile int * tcsm1_fifo_wp;
volatile int * tcsm0_fifo_rp;
#endif

//#define JZC_CRC_VER
#ifdef JZC_CRC_VER
# undef   fprintf
# undef   printf
# include "crc.c"
short crc_code;
short mpFrame;
#else
#undef printf
#endif

extern int use_jz_buf;

static void ptr_square(void * start_ptr,int size,int h,int w, int stride);

#define JZC_VMAU_OPT
#ifdef JZC_VMAU_OPT
#include "jzsoc/mpeg4_p1_vmau.h"
#endif

uint8_t dmbh[108] = 
  {0x01,0x00,0x01,0x01,0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x04,0x04,0x08,0x01,0x3f,0x00,0x00,0x00,
   0x3f,0x00,0x00,0x00,0x3f,0x00,0x00,0x00,0x3f,0x00,0x00,0x00,
   0x3f,0x00,0x00,0x00,0x3f,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
uint16_t dmbe[64*6] = 
  {0x00b1,0x0002,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
   0xfffc,0xfffe,0xfffe,0x0001,0x0000,0x0000,0x0000,0xffff,
   0x0007,0x0000,0x0001,0xffff,0x0000,0x0000,0x0000,0x0000,
   0xfffb,0xffff,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
   0x0002,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
   0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
   0xffff,0x0001,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
   0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
   0x00a9,0x0000,0x0000,0x0001,0x0000,0x0000,0x0000,0x0000,
   0x0001,0xfff7,0x0002,0x0001,0x0000,0x0000,0x0000,0x0000,
   0x000e,0x0000,0xffff,0x0000,0xffff,0x0000,0x0000,0x0000,
   0x0000,0xfffe,0x0000,0x0000,0xffff,0x0000,0x0000,0x0000,
   0xfffe,0x0000,0x0000,0x0001,0x0000,0x0000,0x0000,0x0000,
   0x0000,0x0002,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
   0xfffe,0x0000,0xffff,0x0000,0x0000,0x0000,0x0000,0x0000,
   0xffff,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
   0x00c7,0x000f,0x0001,0xffff,0x0000,0x0000,0x0000,0x0000,
   0x0006,0x0000,0x0002,0x0000,0x0000,0x0000,0x0000,0x0000,
   0xffff,0xfffc,0x0000,0xffff,0x0000,0x0000,0x0000,0x0000,
   0xffff,0x0000,0x0001,0x0001,0x0000,0x0000,0x0000,0x0000,
   0x0000,0x0000,0xffff,0x0000,0x0000,0x0000,0x0000,0x0000,
   0xffff,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
   0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
   0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
   0x00bd,0xfffe,0xffff,0x0000,0xffff,0x0000,0x0001,0x0000,
   0x0005,0x0009,0xfffe,0x0000,0x0000,0xffff,0x0000,0x0000,
   0xfffc,0x0005,0xffff,0x0001,0x0000,0x0000,0x0000,0x0000,
   0xffff,0xffff,0xfffe,0x0000,0x0000,0x0000,0x0001,0x0000,
   0x0000,0x0000,0xffff,0x0000,0x0000,0x0000,0x0000,0x0000,
   0x0000,0x0000,0x0001,0x0000,0x0000,0x0000,0x0000,0x0000,
   0xffff,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
   0x0001,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
   0x0091,0xffff,0xffff,0x0000,0x0000,0x0000,0x0000,0x0000,
   0x0002,0x0001,0x0001,0x0000,0x0000,0x0000,0x0000,0x0000,
   0x0000,0x0001,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
   0xffff,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
   0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
   0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
   0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
   0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
   0x006b,0x0002,0x0004,0x0001,0x0000,0x0000,0x0000,0x0000,
   0xfffe,0xfffe,0xffff,0x0000,0x0000,0x0000,0x0000,0x0000,
   0xffff,0xffff,0x0001,0xffff,0x0000,0x0000,0x0000,0x0000,
   0x0002,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
   0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
   0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
   0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
   0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000};

#ifdef JZC_P1_OPT
static void mpeg4_motion_set(int intpid, int cintpid){
  int i;

  for(i=0; i<16; i++){
    SET_TAB1_ILUT(i,/*idx*/
		  IntpFMT[intpid][i].intp[1],/*intp2*/
		  IntpFMT[intpid][i].intp_pkg[1],/*intp2_pkg*/
		  IntpFMT[intpid][i].hldgl,/*hldgl*/
		  IntpFMT[intpid][i].avsdgl,/*avsdgl*/
		  IntpFMT[intpid][i].intp_dir[1],/*intp2_dir*/
		  IntpFMT[intpid][i].intp_rnd[1],/*intp2_rnd*/
		  IntpFMT[intpid][i].intp_sft[1],/*intp2_sft*/
		  IntpFMT[intpid][i].intp_sintp[1],/*sintp2*/
		  IntpFMT[intpid][i].intp_srnd[1],/*sintp2_rnd*/
		  IntpFMT[intpid][i].intp_sbias[1],/*sintp2_bias*/
		  IntpFMT[intpid][i].intp[0],/*intp1*/
		  IntpFMT[intpid][i].tap,/*tap*/
		  IntpFMT[intpid][i].intp_pkg[0],/*intp1_pkg*/
		  IntpFMT[intpid][i].intp_dir[0],/*intp1_dir*/
		  IntpFMT[intpid][i].intp_rnd[0],/*intp1_rnd*/
		  IntpFMT[intpid][i].intp_sft[0],/*intp1_sft*/
		  IntpFMT[intpid][i].intp_sintp[0],/*sintp1*/
		  IntpFMT[intpid][i].intp_srnd[0],/*sintp1_rnd*/
		  IntpFMT[intpid][i].intp_sbias[0]/*sintp1_bias*/
		  );
    SET_TAB1_CLUT(i,/*idx*/
		  IntpFMT[intpid][i].intp_coef[0][7],/*coef8*/
		  IntpFMT[intpid][i].intp_coef[0][6],/*coef7*/
		  IntpFMT[intpid][i].intp_coef[0][5],/*coef6*/
		  IntpFMT[intpid][i].intp_coef[0][4],/*coef5*/
		  IntpFMT[intpid][i].intp_coef[0][3],/*coef4*/
		  IntpFMT[intpid][i].intp_coef[0][2],/*coef3*/
		  IntpFMT[intpid][i].intp_coef[0][1],/*coef2*/
		  IntpFMT[intpid][i].intp_coef[0][0] /*coef1*/
		  );
    SET_TAB1_CLUT(16+i,/*idx*/
		  IntpFMT[intpid][i].intp_coef[1][7],/*coef8*/
		  IntpFMT[intpid][i].intp_coef[1][6],/*coef7*/
		  IntpFMT[intpid][i].intp_coef[1][5],/*coef6*/
		  IntpFMT[intpid][i].intp_coef[1][4],/*coef5*/
		  IntpFMT[intpid][i].intp_coef[1][3],/*coef4*/
		  IntpFMT[intpid][i].intp_coef[1][2],/*coef3*/
		  IntpFMT[intpid][i].intp_coef[1][1],/*coef2*/
		  IntpFMT[intpid][i].intp_coef[1][0] /*coef1*/
		  );

    SET_TAB2_ILUT(i,/*idx*/
		  IntpFMT[cintpid][i].intp[1],/*intp2*/
		  IntpFMT[cintpid][i].intp_dir[1],/*intp2_dir*/
		  IntpFMT[cintpid][i].intp_sft[1],/*intp2_sft*/
		  IntpFMT[cintpid][i].intp_coef[1][0],/*intp2_lcoef*/
		  IntpFMT[cintpid][i].intp_coef[1][1],/*intp2_rcoef*/
		  IntpFMT[cintpid][i].intp_rnd[1],/*intp2_rnd*/
		  IntpFMT[cintpid][i].intp[0],/*intp1*/
		  IntpFMT[cintpid][i].intp_dir[0],/*intp1_dir*/
		  IntpFMT[cintpid][i].intp_sft[0],/*intp1_sft*/
		  IntpFMT[cintpid][i].intp_coef[0][0],/*intp1_lcoef*/
		  IntpFMT[cintpid][i].intp_coef[0][1],/*intp1_rcoef*/
		  IntpFMT[cintpid][i].intp_rnd[0]/*intp1_rnd*/
		  );
  }
}

static void mpeg4_motion_init(int intpid, int cintpid)
{
  int i;
  for(i=0; i<16; i++){
    SET_TAB1_ILUT(i,/*idx*/
		  IntpFMT[intpid][i].intp[1],/*intp2*/
		  IntpFMT[intpid][i].intp_pkg[1],/*intp2_pkg*/
		  IntpFMT[intpid][i].hldgl,/*hldgl*/
		  IntpFMT[intpid][i].avsdgl,/*avsdgl*/
		  IntpFMT[intpid][i].intp_dir[1],/*intp2_dir*/
		  IntpFMT[intpid][i].intp_rnd[1],/*intp2_rnd*/
		  IntpFMT[intpid][i].intp_sft[1],/*intp2_sft*/
		  IntpFMT[intpid][i].intp_sintp[1],/*sintp2*/
		  IntpFMT[intpid][i].intp_srnd[1],/*sintp2_rnd*/
		  IntpFMT[intpid][i].intp_sbias[1],/*sintp2_bias*/
		  IntpFMT[intpid][i].intp[0],/*intp1*/
		  IntpFMT[intpid][i].tap,/*tap*/
		  IntpFMT[intpid][i].intp_pkg[0],/*intp1_pkg*/
		  IntpFMT[intpid][i].intp_dir[0],/*intp1_dir*/
		  IntpFMT[intpid][i].intp_rnd[0],/*intp1_rnd*/
		  IntpFMT[intpid][i].intp_sft[0],/*intp1_sft*/
		  IntpFMT[intpid][i].intp_sintp[0],/*sintp1*/
		  IntpFMT[intpid][i].intp_srnd[0],/*sintp1_rnd*/
		  IntpFMT[intpid][i].intp_sbias[0]/*sintp1_bias*/
		  );
    SET_TAB1_CLUT(i,/*idx*/
		  IntpFMT[intpid][i].intp_coef[0][7],/*coef8*/
		  IntpFMT[intpid][i].intp_coef[0][6],/*coef7*/
		  IntpFMT[intpid][i].intp_coef[0][5],/*coef6*/
		  IntpFMT[intpid][i].intp_coef[0][4],/*coef5*/
		  IntpFMT[intpid][i].intp_coef[0][3],/*coef4*/
		  IntpFMT[intpid][i].intp_coef[0][2],/*coef3*/
		  IntpFMT[intpid][i].intp_coef[0][1],/*coef2*/
		  IntpFMT[intpid][i].intp_coef[0][0] /*coef1*/
		  );
    SET_TAB1_CLUT(16+i,/*idx*/
		  IntpFMT[intpid][i].intp_coef[1][7],/*coef8*/
		  IntpFMT[intpid][i].intp_coef[1][6],/*coef7*/
		  IntpFMT[intpid][i].intp_coef[1][5],/*coef6*/
		  IntpFMT[intpid][i].intp_coef[1][4],/*coef5*/
		  IntpFMT[intpid][i].intp_coef[1][3],/*coef4*/
		  IntpFMT[intpid][i].intp_coef[1][2],/*coef3*/
		  IntpFMT[intpid][i].intp_coef[1][1],/*coef2*/
		  IntpFMT[intpid][i].intp_coef[1][0] /*coef1*/
		  );

    SET_TAB2_ILUT(i,/*idx*/
		  IntpFMT[cintpid][i].intp[1],/*intp2*/
		  IntpFMT[cintpid][i].intp_dir[1],/*intp2_dir*/
		  IntpFMT[cintpid][i].intp_sft[1],/*intp2_sft*/
		  IntpFMT[cintpid][i].intp_coef[1][0],/*intp2_lcoef*/
		  IntpFMT[cintpid][i].intp_coef[1][1],/*intp2_rcoef*/
		  IntpFMT[cintpid][i].intp_rnd[1],/*intp2_rnd*/
		  IntpFMT[cintpid][i].intp[0],/*intp1*/
		  IntpFMT[cintpid][i].intp_dir[0],/*intp1_dir*/
		  IntpFMT[cintpid][i].intp_sft[0],/*intp1_sft*/
		  IntpFMT[cintpid][i].intp_coef[0][0],/*intp1_lcoef*/
		  IntpFMT[cintpid][i].intp_coef[0][1],/*intp1_rcoef*/
		  IntpFMT[cintpid][i].intp_rnd[0]/*intp1_rnd*/
		  );
  }

  SET_REG1_STAT(1,/*pfe*/
		1,/*lke*/
		1 /*tke*/);
  SET_REG2_STAT(1,/*pfe*/
		1,/*lke*/
		1 /*tke*/);
#if 0
  SET_REG1_CTRL(0,/*esms*/
		0,/*esa*/
		0,/*esmd*/
		0,/*csf*/
		0,/*cara*/
		0,/*cae*/
		0,/*crpm*/
		0xF,/*pgc*/
		3, /*pri*/
		0,/*ckge*/
		0,/*ofa*/
		0,/*rot*/
		USE_TDD,/*ddm*/
		0,/*wm*/
		1,/*ccf*/
		0,/*csl*/
		0,/*rst*/
		1 /*en*/);
#else
  //            ebms,esms,earm,epmv,esa,ebme,cae,pgc,ch2en,pri,ckge,ofa,rot,rotdir,wm,ccf,irqe,rst,en)
  //SET_REG1_CTRL(0,   0,   0,   0,   0,  0,   0,  0xF,1,    0,  0,   0,  0,  0,     0, 1,  0,   0,  1);
  SET_REG1_CTRL(0,   0,   0,   0,   0,  0,   0,  0xF,1,    3,  0,   1,  0,  0,     0, 1,  0,   0,  1);
#endif
  SET_REG1_BINFO_P0(AryFMT[intpid],/*ary*/
		    0,/*doe*/
		    0,/*expdy*/
		    0,/*expdx*/
		    0,/*ilmd*/
		    SubPel[intpid]-1,/*pel*/
		    0,/*fld*/
		    0,/*fldsel*/
		    0,/*boy*/
		    0,/*box*/
		    0,/*bh*/
		    0,/*bw*/
		    0/*pos*/);
  SET_REG2_BINFO_P0(0,/*ary*/
		    0,/*doe*/
		    0,/*expdy*/
		    0,/*expdx*/
		    0,/*ilmd*/
		    0,/*pel*/
		    0,/*fld*/
		    0,/*fldsel*/
		    0,/*boy*/
		    0,/*box*/
		    0,/*bh*/
		    0,/*bw*/
		    0/*pos*/);
}

static void motion_config_mpeg4(MpegEncContext *s)
{
  int i, j;

#if 0
  SET_REG1_CTRL(0,/*esms*/
		0,/*esa*/
		0,/*esmd*/
		0,/*csf*/
		0,/*cara*/
		0,/*cae*/
		0,/*crpm*/
		0xF,/*pgc*/
		0, /*pri*/
		0,/*ckge*/
		0,/*ofa*/
		0,/*rot*/
		USE_TDD,/*ddm*/
		0,/*wm*/
		1,/*ccf*/
		0,/*csl*/
		0,/*rst*/
		1 /*en*/);
#else
  //SET_REG1_CTRL(0,0,0,0,0,0,0,0xF,1,0,0,0,0,0,0,1,0,0,1);
  //             ebms,esms,earm,epmv,esa,ebme,cae,pgc,ch2en,pri,ckge,ofa,rot,rotdir,wm,ccf,irqe,rst,en)
  if (s->msmpeg4_version == 1 || s->msmpeg4_version == 2)
    SET_REG1_CTRL(0,   0,   0,   0,   0,  0,   1,  0xF,1,    0,  0,   0,  0,  0,     0, 1,  0,   0,  1);
  else
    SET_REG1_CTRL(0,   0,   0,   0,   0,  0,   1,  0xF,1,    0,  0,   1,  0,  0,     0, 1,  0,   0,  1);
#endif

  SET_TAB1_RLUT(0, get_phy_addr((int)(s->last_picture.data[0])), 0, 0);
  SET_TAB1_RLUT(16, get_phy_addr((int)(s->next_picture.data[0])), 0, 0);
  SET_TAB2_RLUT(0, get_phy_addr((int)(s->last_picture.data[1])), 0, 0, 0, 0);
  SET_TAB2_RLUT(16, get_phy_addr((int)(s->next_picture.data[1])), 0, 0, 0, 0);

  //0x20
  SET_REG1_PINFO(0,/*rgr*/
		 0,/*its*/
		 6,/*its_sft*/
		 0/*v->its_scale*/,/*its_scale*/
		 0/*v->its_rnd_y*//*its_rnd*/);
  SET_REG2_PINFO(0,/*rgr*/
		 0,/*its*/
		 6,/*its_sft*/
		 0/*v->its_scale*/,/*its_scale*/
		 0/*v->its_rnd_c*//*its_rnd*/);

  //0x24
  SET_REG1_WINFO(0,/*wt*/
		 0, /*wtpd*/
		 IS_BIAVG,/*wtmd*/
		 1,/*biavg_rnd*/
		 0,/*wt_denom*/
		 0,/*wt_sft*/
		 0,/*wt_lcoef*/
		 0/*wt_rcoef*/);
  //0x2c
  SET_REG1_WTRND(0);


  SET_REG2_WINFO1(0,/*wt*/
		  0, /*wtpd*/
		  IS_BIAVG,/*wtmd*/
		  1,/*biavg_rnd*/
		  0,/*wt_denom*/
		  0,/*wt_sft*/
		  0,/*wt_lcoef*/
		  0/*wt_rcoef*/);
  SET_REG2_WINFO2(0,/*wt_sft*/
		  0,/*wt_lcoef*/
		  0/*wt_rcoef*/);

  SET_REG2_WTRND(0, 0);
  SET_REG1_STRD(s->linesize/16,0,DOUT_Y_STRD);
  //SET_REG1_STRD(s->mb_width*16,0,DOUT_Y_STRD);
  SET_REG1_GEOM(s->mb_height*16,s->mb_width*16);
  SET_REG2_STRD(s->linesize/16,0,DOUT_C_STRD);
  //SET_REG2_STRD(s->mb_width*16,0,DOUT_C_STRD);
}

void mpeg4_p1_jumpto_main()
{
  i_jr(MPEG4_P1_MAIN);
}
#endif

av_cold int mpeg4_decode_end(AVCodecContext *avctx)
{
  MpegEncContext *s = avctx->priv_data;

  MPV_common_end(s);
  use_jz_buf=0;
  return 0;
}

static void linear_crc_frame(MpegEncContext *s){
#ifdef JZC_CRC_VER
  int crc_i;
  int crc_j = 0;

  char *ptr_y = s->current_picture.data[0];
  char *ptr_c = s->current_picture.data[1];
  jz_dcache_wb();
  //last_crc_code = crc_code;

  for(crc_i=0; crc_i<s->mb_height; crc_i++){
    for (crc_j = 0; crc_j < s->mb_width; crc_j++){
      crc_code = crc(ptr_y, 256, crc_code);
      ptr_y += 256;
    }
    //ptr_y += 1024;
  }

  for(crc_i=0; crc_i<s->mb_height; crc_i++){
    for (crc_j = 0; crc_j < s->mb_width; crc_j++){
      crc_code = crc(ptr_c, 128, crc_code);
      ptr_c += 128;
    }
    //ptr_c += 512;
  }

  //mpFrame++;

  printf("frame: %d, crc_code: 0x%x\n", mpFrame, crc_code);
#endif
}

static void rota_crc_frame(MpegEncContext *s){
#ifdef JZC_CRC_VER
#endif
}

/**
 * returns the number of bytes consumed for building the current frame
 */
static int get_consumed_bytes(MpegEncContext *s, int buf_size){
  int pos= (get_bits_count(&s->gb)+7)>>3;

  if(s->divx_packed || s->avctx->hwaccel){
    //we would have to scan through the whole buf to handle the weird reordering ...
    return buf_size;
  }else if(s->flags&CODEC_FLAG_TRUNCATED){
    pos -= s->parse_context.last_index;
    if(pos<0) pos=0; // padding is not really read so this might be -1
    return pos;
  }else{
    if(pos==0) pos=1; //avoid infinite loops (i doubt that is needed but ...)
    if(pos+10>buf_size) pos=buf_size; // oops ;)

    return pos;
  }
}

av_cold int mpeg4_decode_init(AVCodecContext *avctx)
{
  MpegEncContext *s = avctx->priv_data;

#ifdef JZC_CRC_VER
  mpFrame = 0;
  crc_code = 0;
#endif

#ifdef JZC_MXU_OPT
  S32I2M(xr16, 0x3);
#endif

#ifdef JZC_PMON_P0MB1
  i_mtc0_2(0, 16, 7);
#endif

#ifdef JZC_P1_OPT
  mpeg4_motion_init(MPEG_QPEL, MPEG_QPEL);
#endif

#ifdef JZC_TCSM_OPT
  AUX_RESET();
#endif

  s->avctx = avctx;
  s->out_format = FMT_H263;

  s->width  = avctx->coded_width;
  s->height = avctx->coded_height;
  s->workaround_bugs= avctx->workaround_bugs;

  // set defaults
  MPV_decode_defaults(s);
  s->quant_precision=5;
  s->decode_mb= ff_h263_decode_mb;
  s->low_delay= 1;
  avctx->pix_fmt= avctx->get_format(avctx, avctx->codec->pix_fmts);
  s->unrestricted_mv= 1;

  /* select sub codec */
  switch(avctx->codec->id) {
  case CODEC_ID_H263:
    s->unrestricted_mv= 0;
    avctx->chroma_sample_location = AVCHROMA_LOC_CENTER;
    break;
  case CODEC_ID_MPEG4:
    break;
  case CODEC_ID_MSMPEG4V1:
    s->h263_msmpeg4 = 1;
    s->h263_pred = 1;
    s->msmpeg4_version=1;
    break;
  case CODEC_ID_MSMPEG4V2:
    s->h263_msmpeg4 = 1;
    s->h263_pred = 1;
    s->msmpeg4_version=2;
    break;
  case CODEC_ID_MSMPEG4V3:
    s->h263_msmpeg4 = 1;
    s->h263_pred = 1;
    s->msmpeg4_version=3;
    break;
  case CODEC_ID_WMV1:
    s->h263_msmpeg4 = 1;
    s->h263_pred = 1;
    s->msmpeg4_version=4;
    break;
  case CODEC_ID_WMV2:
    s->h263_msmpeg4 = 1;
    s->h263_pred = 1;
    s->msmpeg4_version=5;
    break;
  case CODEC_ID_VC1:
  case CODEC_ID_WMV3:
    s->h263_msmpeg4 = 1;
    s->h263_pred = 1;
    s->msmpeg4_version=6;
    avctx->chroma_sample_location = AVCHROMA_LOC_LEFT;
    break;
  case CODEC_ID_H263I:
    break;
  case CODEC_ID_FLV1:
    s->h263_flv = 1;
    break;
  default:
    return -1;
  }
  s->codec_id= avctx->codec->id;
  avctx->hwaccel= ff_find_hwaccel(avctx->codec->id, avctx->pix_fmt);

#ifdef JZC_P1_OPT
  t1_dFRM=TCSM1_VUCADDR(TCSM1_DFRM_BUF);
  int * tmp_hm_buf = jz4740_alloc_frame(32, SPACE_HALF_MILLION_BYTE);

  if (tmp_hm_buf < 0)
    printf("JZ4740 ALLOC tmp_hm_buf ERROR !! \n");

  {
    int i, tmp;
    int *src, *dst;

    *((volatile int *)(TCSM1_VUCADDR(TCSM1_P1_TASK_DONE))) = 0;

    FILE *fp_text;
    int len, *reserved_mem;
    if (s->msmpeg4_version == 1 || s->msmpeg4_version == 2){
      fp_text = fopen("msmpeg4_p1.bin", "r+b");
    }else{
      fp_text = fopen("mpeg4_p1.bin", "r+b");
    }
    if (!fp_text)
      printf(" error while open mpeg4_p1.bin \n");
    int *load_buf = tmp_hm_buf;
    len = fread(load_buf, 4, SPACE_HALF_MILLION_BYTE, fp_text);
    //printf(" mpeg4 len of p1 task = %d\n",len);

    reserved_mem = (int *)TCSM1_VUCADDR(MPEG4_P1_MAIN);
    for(i=0; i<len; i++)
      reserved_mem[i] = load_buf[i];

    fclose(fp_text);
    jz_dcache_wb(); /*flush cache into reserved mem*/
    i_sync();
  }

#endif

  memmb1 = jz4740_alloc_frame(128,64*1024);
  memmb3 = jz4740_alloc_frame(128,TASK_BUF_LEN);

  /* for h263, we allocate the images after having read the header */
  if (avctx->codec->id != CODEC_ID_H263 && avctx->codec->id != CODEC_ID_MPEG4)
    if (MPV_common_init(s) < 0)
      return -1;

  mpeg4_decode_init_vlc(s);
  use_jz_buf=1;

  return 0;
}

static inline void JZC_clean_intra_table_entries(MpegEncContext *s)
{
  int wrap = s->b8_stride;
  int xy = s->block_index[0];
  int addr,i;

  s->dc_val[0][xy           ] =
    s->dc_val[0][xy + 1       ] =
    s->dc_val[0][xy     + wrap] =
    s->dc_val[0][xy + 1 + wrap] = 1024;
  /* ac pred */
#if 1
  addr = s->ac_val[0][xy];
  i_pref(30, addr, 0);
  S32STD(xr0, addr, 0);
  S32STD(xr0, addr, 4);
  S32STD(xr0, addr, 8);
  S32STD(xr0, addr, 12);
  S32STD(xr0, addr, 16);
  S32STD(xr0, addr, 20);
  S32STD(xr0, addr, 24);
  S32STD(xr0, addr, 28);
  addr = s->ac_val[0][xy+1];
  i_pref(30, addr, 0);
  S32STD(xr0, addr, 0);
  S32STD(xr0, addr, 4);
  S32STD(xr0, addr, 8);
  S32STD(xr0, addr, 12);
  S32STD(xr0, addr, 16);
  S32STD(xr0, addr, 20);
  S32STD(xr0, addr, 24);
  S32STD(xr0, addr, 28);

  addr = s->ac_val[0][xy + wrap];
  i_pref(30, addr, 0);
  S32STD(xr0, addr, 0);
  S32STD(xr0, addr, 4);
  S32STD(xr0, addr, 8);
  S32STD(xr0, addr, 12);
  S32STD(xr0, addr, 16);
  S32STD(xr0, addr, 20);
  S32STD(xr0, addr, 24);
  S32STD(xr0, addr, 28);
  addr = s->ac_val[0][xy + wrap + 1];
  i_pref(30, addr, 0);
  S32STD(xr0, addr, 0);
  S32STD(xr0, addr, 4);
  S32STD(xr0, addr, 8);
  S32STD(xr0, addr, 12);
  S32STD(xr0, addr, 16);
  S32STD(xr0, addr, 20);
  S32STD(xr0, addr, 24);
  S32STD(xr0, addr, 28);
#else
  memset(s->ac_val[0][xy       ], 0, 32 * sizeof(int16_t));
  memset(s->ac_val[0][xy + wrap], 0, 32 * sizeof(int16_t));
#endif
  if (s->msmpeg4_version>=3) {
    s->coded_block[xy           ] =
      s->coded_block[xy + 1       ] =
      s->coded_block[xy     + wrap] =
      s->coded_block[xy + 1 + wrap] = 0;
  }
  /* chroma */
  wrap = s->mb_stride;
  xy = s->mb_x + s->mb_y * wrap;
  s->dc_val[1][xy] =
    s->dc_val[2][xy] = 1024;
  /* ac pred */
#if 1
  addr = s->ac_val[1][xy];
  i_pref(30, addr, 0);
  S32STD(xr0, addr, 0);
  S32STD(xr0, addr, 4);
  S32STD(xr0, addr, 8);
  S32STD(xr0, addr, 12);
  S32STD(xr0, addr, 16);
  S32STD(xr0, addr, 20);
  S32STD(xr0, addr, 24);
  S32STD(xr0, addr, 28);

  addr = s->ac_val[2][xy];
  i_pref(30, addr, 0);
  S32STD(xr0, addr, 0);
  S32STD(xr0, addr, 4);
  S32STD(xr0, addr, 8);
  S32STD(xr0, addr, 12);
  S32STD(xr0, addr, 16);
  S32STD(xr0, addr, 20);
  S32STD(xr0, addr, 24);
  S32STD(xr0, addr, 28);
  //jz_dcache_wb();
#else
  memset(s->ac_val[1][xy], 0, 16 * sizeof(int16_t));
  memset(s->ac_val[2][xy], 0, 16 * sizeof(int16_t));
#endif

  s->mbintra_table[xy]= 0;
}

static inline void JZC_decode_mb(MpegEncContext *s){
  const int mb_xy = s->mb_y * s->mb_stride + s->mb_x;
  uint8_t *mbskip_ptr = &s->mbskip_table[mb_xy];
  const int age = s->current_picture.age;

  s->current_picture.qscale_table[mb_xy] = s->qscale;

  if (!s->mb_intra){
    if (s->h263_pred || s->h263_aic){
      if (s->mbintra_table[mb_xy]){
	JZC_clean_intra_table_entries(s);
	//s->mbintra_table[mb_xy]= 0;
      }
    }else{
      s->last_dc[0]=
	s->last_dc[1]=
	s->last_dc[2]= 128 << s->intra_dc_precision;
    }
  }
  else if (s->h263_pred || s->h263_aic){
    s->mbintra_table[mb_xy]=1;
  }

  if(1){
    assert(age);
    //printf("mb_skipped %d, reference %d\n", s->mb_skipped, s->current_picture.reference);
    if (s->mb_skipped) {
      s->mb_skipped= 0;
      assert(s->pict_type!=I_TYPE);

      (*mbskip_ptr) ++; /* indicate that this time we skipped it */
      if(*mbskip_ptr >99) 
	*mbskip_ptr= 99;

    } else if(!s->current_picture.reference){
      (*mbskip_ptr) ++; /* increase counter so the age can be compared cleanly */
      if(*mbskip_ptr >99) 
	*mbskip_ptr= 99;
    } else{
      *mbskip_ptr = 0; /* not skipped */
    }
  }
  return;
}

static int get_GlbARGs(MpegEncContext *s)
{
  int i = 0,j=0;
  dFRM.pict_type = s->pict_type;
  dFRM.draw_horiz_band = !(s->pict_type==FF_B_TYPE && s->avctx->draw_horiz_band && s->picture_structure==PICT_FRAME);
  dFRM.draw_horiz_band = 1;
  dFRM.no_rounding = s->no_rounding;
  dFRM.quarter_sample = s->quarter_sample;
  dFRM.out_format = s->out_format;
  dFRM.codec_id = s->codec_id;
  dFRM.chroma_x_shift = s->chroma_x_shift;
  dFRM.chroma_y_shift = s->chroma_y_shift;
  dFRM.mb_width = s->mb_width;
  dFRM.mb_height = s->mb_height;
  dFRM.width = s->width;
  dFRM.height = s->height;
  dFRM.linesize = s->linesize;
  dFRM.uvlinesize = s->uvlinesize;
  dFRM.mspel = s->mspel;
  dFRM.h263_aic = s->h263_aic;
  dFRM.hurry_up = s->hurry_up;
  dFRM.h263_msmpeg4 = s->h263_msmpeg4;
  dFRM.alternate_scan = s->alternate_scan;
  dFRM.mpeg_quant = s->mpeg_quant;
  //dFRM.edge = dFRM->draw_horiz_band;
  dFRM.edge = 0;
  memcpy(dFRM.raster_end, s->inter_scantable.raster_end, sizeof(char)*64);
  dFRM.workaround_bugs = s->workaround_bugs;
  dFRM.last_picture_data[0] = s->last_picture.data[0];
  dFRM.last_picture_data[1] = s->last_picture.data[1];
  dFRM.last_picture_data[2] = s->last_picture.data[2];
  dFRM.last_picture_data[3] = s->last_picture.data[3];
  dFRM.next_picture_data[0] = s->next_picture.data[0];
  dFRM.next_picture_data[1] = s->next_picture.data[1];
  dFRM.next_picture_data[2] = s->next_picture.data[2];
  dFRM.next_picture_data[3] = s->next_picture.data[3];

  memcpy(dFRM.intra_matrix, s->intra_matrix, sizeof(short)*64);
  memcpy(dFRM.inter_matrix, s->inter_matrix, sizeof(short)*64);

  dFRM.current_picture_data[0] = get_phy_addr(s->current_picture.data[0]);
  dFRM.current_picture_data[1] = get_phy_addr(s->current_picture.data[1]);

  dFRM.mem_addr = get_phy_addr(memmb1);

  memcpy(t1_dFRM, &dFRM, sizeof(MPEG4_Frame_GlbARGs));
#ifdef JZC_VMAU_OPT
  {
    unsigned short *ptr;
    unsigned vmau_addr = VMAU_VUCADDR(VMAU_V_BASE + VMAU_QT_BASE);
    int kk;
    ptr = s->intra_matrix;
    for ( kk = 0 ; kk < 16; kk++){
      unsigned int tmp;
      tmp = (ptr[kk*4] & 0xff) | ((ptr[kk*4+1] & 0xff) << 8) | ((ptr[kk*4+2] & 0xff)<<16) | ((ptr[kk*4+3] & 0xff) << 24);
      //printf("%x,", tmp);
      write_reg(vmau_addr, tmp);
      vmau_addr = vmau_addr + 4;
    }
    ptr = s->inter_matrix;
    for ( kk = 0 ; kk < 16; kk++){
      unsigned int tmp;
      tmp = (ptr[kk*4] & 0xff) | ((ptr[kk*4+1] & 0xff) << 8) | ((ptr[kk*4+2] & 0xff)<<16) | ((ptr[kk*4+3] & 0xff) << 24);
      //printf("%x,", tmp);
      write_reg(vmau_addr, tmp);
      vmau_addr = vmau_addr + 4;
    }
  }
#endif
  return 0;
}

static int get_MBARGs(MpegEncContext *s)
{
#if 1
  dMB->interlaced_dct = s->interlaced_dct;
  dMB->mb_intra = s->mb_intra;
  dMB->mv_dir = s->mv_dir;
  dMB->mv_type = s->mv_type;
  dMB->mb_x = s->mb_x;
  dMB->mb_y = s->mb_y;
  dMB->qscale = s->qscale;
  dMB->chroma_qscale = s->chroma_qscale;
  dMB->y_dc_scale = s->y_dc_scale;
  dMB->c_dc_scale = s->c_dc_scale;
  dMB->ac_pred = s->ac_pred;
  dMB->skip_idct = s->avctx->skip_idct;
#else
#endif

  memcpy(dMB->block_last_index, s->block_last_index, sizeof(int)*6);
  {
      int i, va, length, cbp;
      int cbp_mask = 1;
      length = (char *)dMB->block[0] - (char *)dMB;
      va = (int)dMB;
      for(i = 0; i < ((length + 31) / 32); i++) {
	    //for(i = 0; i < ((TASK_BUF_LEN + 31) / 32); i++) {
	  i_cache(0x19, va, 0);
	  va += 32;
      }

      va = (int)dMB->block[0];
      if(!dMB->mb_intra) {
	  for(i = 0; i < 6; i++){
	      if(dMB->code_cbp & cbp_mask) {
		  i_cache(0x19, va, 0);
		  va += 32;
		  i_cache(0x19, va, 0);
		  va += 32;
		  i_cache(0x19, va, 0);
		  va += 32;
		  i_cache(0x19, va, 0);
		  va += 32;
	      } else 
		  va += 128;
	      cbp_mask <<= 4;
	  }
      } else {
	  for(i = 0; i < 24; i++) {// 6 block X 4 cache line per block
	      i_cache(0x19, va, 0);
	      va += 32;
	  }
      }

      i_sync();
  }

  return 0;
}

static inline void ff_init_mbdest(MpegEncContext *s){
  int mb_size = 4; 
  
  s->dest[0] = s->current_picture.data[0] + ((s->mb_x - 1) << (mb_size + 4));
  s->dest[1] = s->current_picture.data[1] + ((s->mb_x - 1) << (mb_size + 3));
  if(!(s->pict_type==FF_B_TYPE && s->avctx->draw_horiz_band && s->picture_structure==PICT_FRAME)){
    s->dest[0] += ((s->mb_y * s->mb_width) << (mb_size + 4));
    if (s->mb_y != 0)
      s->dest[0] += (1024 * s->mb_y);
    s->dest[1] += ((s->mb_y * s->mb_width) << (mb_size + 3));
    if (s->mb_y != 0)
      s->dest[1] += (512 * s->mb_y);
  }
}

static inline void ff_update_mbdest(MpegEncContext *s){
  s->dest[0] += 256;
  s->dest[1] += 128;
}

static void ptr_square(void * start_ptr,int size,int h,int w, int stride){
  unsigned int* start_int=(int*)start_ptr;
  unsigned short* start_short=(short*)start_ptr;
  unsigned char* start_byte=(char*)start_ptr;
  int i, j;
  if(size==4){
    for(i=0;i<h;i++){
      for(j=0;j<w;j++){
	printf("0x%08x,",start_int[i*stride+j]);
      }
      printf("\n");
    }
  }
  if(size==2){
    for(i=0;i<h;i++){
      for(j=0;j<w;j++){
	printf("0x%04x,",start_short[i*stride+j]);
      }
      printf("\n");
    }
  }
  if(size==1){
    for(i=0;i<h;i++){
      for(j=0;j<w;j++){
	printf("0x%02x,",start_byte[i*stride+j]);
      }
      printf("\n");
    }
  }
}

static inline void JZC_h263_update_motion_val(MpegEncContext * s){
  const int mb_xy = s->mb_y * s->mb_stride + s->mb_x;
  //FIXME a lot of that is only needed for !low_delay
  const int wrap = s->b8_stride;
  const int xy = s->block_index[0];

  s->current_picture.mbskip_table[mb_xy]= s->mb_skipped;

  if (s->mv_type == MV_TYPE_16X16){
    int motion_x, motion_y;
    if (s->mb_intra){
      motion_x = 0;
      motion_y = 0;
    }else{
      motion_x = s->mv[0][0][0];
      motion_y = s->mv[0][0][1];
    }
    s->current_picture.motion_val[0][xy][0] = motion_x;
    s->current_picture.motion_val[0][xy][1] = motion_y;
    s->current_picture.motion_val[0][xy + 1][0] = motion_x;
    s->current_picture.motion_val[0][xy + 1][1] = motion_y;
    s->current_picture.motion_val[0][xy + wrap][0] = motion_x;
    s->current_picture.motion_val[0][xy + wrap][1] = motion_y;
    s->current_picture.motion_val[0][xy + 1 + wrap][0] = motion_x;
    s->current_picture.motion_val[0][xy + 1 + wrap][1] = motion_y;
  }
}

static int decode_slice(MpegEncContext *s){
  const int part_mask= s->partitioned_frame ? (AC_END|AC_ERROR) : 0x7F;
  const int mb_size= 16>>s->avctx->lowres;

#ifdef JZC_P1_OPT
  unsigned int tcsm0_fifo_rp_lh;
  unsigned int task_fifo_wp_lh;
  unsigned int task_fifo_wp_lh_overlap;

  tcsm1_fifo_wp = (int *)TCSM1_VUCADDR(TCSM1_MBNUM_WP);
  tcsm0_fifo_rp = (volatile int *)TCSM1_VUCADDR(TCSM1_P1_FIFO_RP);

  task_fifo_wp = (int *)memmb1; // used by P0
  task_fifo_wp_d1 = (int *)memmb1; // wp delay 1 MB
  task_fifo_wp_d2 = (int *)memmb1; // wp delay 2 MB
  *tcsm1_fifo_wp = 0; // write by P0, used by P1
  *tcsm0_fifo_rp = 0; // write once before p1 start

  int *ptr = (unsigned int*)TCSM1_VUCADDR(TCSM1_DBG_BUF);
  int mulsl = 0;
  int task_begin = memmb1;
  int task_end = task_begin + TASK_LEN;
  //printf("memmb1:%x task_begin:%x task_end:%x\n", memmb1, task_begin, task_end);
#endif

 mul_slice:
  s->last_resync_gb= s->gb;
  s->first_slice_line= 1;

  s->resync_mb_x= s->mb_x;
  s->resync_mb_y= s->mb_y;

#ifdef JZC_P1_OPT
  motion_config_mpeg4(s);
#endif

  ff_set_qscale(s, s->qscale);

  get_GlbARGs(s);

#ifdef JZC_P1_OPT
  if (!mulsl){
    int n;int temp=t1_dFRM;
    for(n=0;n<FRAME_T_CC_LINE;n++){
      i_cache(0x19,temp,0);
      temp +=32;
    }
    jz_dcache_wb();
    AUX_RESET();
    *((volatile int *)(TCSM1_VUCADDR(TCSM1_P1_TASK_DONE))) = 0;
    AUX_START();
  }
#endif

  for(; s->mb_y < s->mb_height; s->mb_y++) {
    /* per-row end of slice checks */
    if(s->msmpeg4_version && !mulsl){
      if(s->resync_mb_y + s->slice_height == s->mb_y){
	ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x-1, s->mb_y, AC_END|DC_END|MV_END);

	while (1){
	  if(s->msmpeg4_version){
	    if(s->slice_height==0 || s->mb_x!=0 || (s->mb_y%s->slice_height)!=0 || get_bits_count(&s->gb) > s->gb.size_in_bits)
	      break;
	  }else{
	    if(ff_h263_resync(s)<0)
	      break;
	  }

	  if(s->msmpeg4_version<4 && s->h263_pred)
	    ff_mpeg4_clean_buffers(s);

	  mulsl = 1;
	  goto mul_slice;
	}
#if 1
	int cnt, add_num = 5;
	for(cnt = 0; cnt < add_num; cnt++){
	  do{
	    tcsm0_fifo_rp_lh = (unsigned int)*tcsm0_fifo_rp & 0xFFFFF;
	    task_fifo_wp_lh = (unsigned int)task_fifo_wp & 0xFFFFF;
	    task_fifo_wp_lh_overlap = task_fifo_wp_lh + TASK_BUF_LEN;
	  } while ( !( (task_fifo_wp_lh_overlap <= tcsm0_fifo_rp_lh) || (task_fifo_wp_lh > tcsm0_fifo_rp_lh) ) );

	  dMB = task_fifo_wp;	  
	  if (cnt == 2){
	    dMB->real_num = -1;
	  }else if (cnt == 0){
	    dMB->real_num = 2;
	  }
	  else
	    dMB->real_num = 1;
	  get_MBARGs(s);

	  task_fifo_wp += (TASK_BUF_LEN + 3)>>2;

	  int reach_tcsm0_end = ((unsigned int)(task_fifo_wp + (TASK_BUF_LEN>>2))) >= task_end;
	  if (reach_tcsm0_end)
	    task_fifo_wp = (int *)task_begin;

	  task_fifo_wp_d2 = task_fifo_wp_d1;
	  task_fifo_wp_d1 = task_fifo_wp;
	  (*tcsm1_fifo_wp)++;
	}
	
	int nn, tmp;
	int time_out = 0;
	do{
	  tmp = *((volatile int *)(TCSM1_VUCADDR(TCSM1_P1_TASK_DONE)));
	  time_out++;
	  if(time_out > 10000000){
	      printf("- Waiting AUX for too long times, maybe it deads.\n");
	      break;
	  }
	}while (tmp == 0);
	AUX_RESET();
#endif

	return 0;
      }
    }

    if (mulsl)
      mulsl = 0;

    if(s->msmpeg4_version==1){
      s->last_dc[0]=
	s->last_dc[1]=
	s->last_dc[2]= 128;
    }

    jz_init_block_index(s);

    for(; s->mb_x < s->mb_width; s->mb_x++) {
      int ret;
      jz_update_block_index(s);

      if(s->resync_mb_x == s->mb_x && s->resync_mb_y+1 == s->mb_y){
	s->first_slice_line=0;
      }

      /* DCT & quantize */

      s->mv_dir = MV_DIR_FORWARD;
      s->mv_type = MV_TYPE_16X16;


#ifdef JZC_P1_OPT
#if 1
      int cccnt = 0;
      do{
	tcsm0_fifo_rp_lh = (volatile int)*tcsm0_fifo_rp & 0xFFFFF;
	task_fifo_wp_lh = (unsigned int)task_fifo_wp & 0xFFFFF;
	task_fifo_wp_lh_overlap = task_fifo_wp_lh + TASK_BUF_LEN;
#if 0
	cccnt++;
	if (cccnt > 100000){
	  printf("mbx %d, mby %d, 22222 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, %d, %d\n", 
		 s->mb_x, s->mb_y, ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7]);
	}
#endif
      } while ( !( (task_fifo_wp_lh_overlap <= tcsm0_fifo_rp_lh) || (task_fifo_wp_lh > tcsm0_fifo_rp_lh) ) );
#endif
      dMB = task_fifo_wp;
      dMB->real_num = 1;
#endif

      ret= s->decode_mb(s, dMB->block);

      if (s->pict_type!=FF_B_TYPE)
	JZC_h263_update_motion_val(s);

      JZC_decode_mb(s);

      get_MBARGs(s);
#ifdef JZC_P1_OPT
      task_fifo_wp += (TASK_BUF_LEN + 3) >> 2;
      int reach_tcsm0_end = ((unsigned int)(task_fifo_wp + (TASK_BUF_LEN>>2))) >= task_end;

      if (reach_tcsm0_end)
	task_fifo_wp = (int *)task_begin;
      //printf("task_fifo_wp:%x %x\n", task_fifo_wp, task_end);
      task_fifo_wp_d2 = task_fifo_wp_d1;
      task_fifo_wp_d1 = task_fifo_wp;

      (*tcsm1_fifo_wp)++;
#endif

      if(ret<0){
	const int xy= s->mb_x + s->mb_y*s->mb_stride;

	if(ret==SLICE_END){
	  if(s->loop_filter)
	    ff_h263_loop_filter(s);
	  
	  //printf("%d %d %d %06X\n", s->mb_x, s->mb_y, s->gb.size*8 - get_bits_count(&s->gb), show_bits(&s->gb, 24));
	  ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x, s->mb_y, (AC_END|DC_END|MV_END)&part_mask);
	  
	  s->padding_bug_score--;
	  
	  if(++s->mb_x >= s->mb_width){
	    s->mb_x=0;
	    ff_draw_horiz_band(s, s->mb_y*mb_size, mb_size);
	    s->mb_y++;
	  }

	  while (s->mb_y<s->mb_height){
	  //while (1){
	    if(s->msmpeg4_version){
	      if(s->slice_height==0 || s->mb_x!=0 || (s->mb_y%s->slice_height)!=0 || get_bits_count(&s->gb) > s->gb.size_in_bits)
		break;
	    }else{
	      if(ff_h263_resync(s)<0)
		break;
	    }

	    if(s->msmpeg4_version<4 && s->h263_pred)
	      ff_mpeg4_clean_buffers(s);

	    mulsl = 1;
	    goto mul_slice;
	  }

	  //sl_end:
#ifdef JZC_P1_OPT
	  int cnt, add_num = 5;
	  for(cnt = 0; cnt < add_num; cnt++){

	    do{
	      //printf("333333333333333333333333333333333\n");
	      tcsm0_fifo_rp_lh = (unsigned int)*tcsm0_fifo_rp & 0xFFFFF;
	      task_fifo_wp_lh = (unsigned int)task_fifo_wp & 0xFFFFF;
	      task_fifo_wp_lh_overlap = task_fifo_wp_lh + TASK_BUF_LEN;
	    } while ( !( (task_fifo_wp_lh_overlap <= tcsm0_fifo_rp_lh) || (task_fifo_wp_lh > tcsm0_fifo_rp_lh) ) );

	    dMB = task_fifo_wp;
	    if (cnt == 2){
	      //printf("88888888888888888888888888888888888\n");
	      //printf("real_num -1 %d %d\n", dMB->mb_x, dMB->mb_y);
	      dMB->real_num = -1;
	    }else if (cnt == 0){
	      dMB->real_num = 2;
	    }
	    else
	      dMB->real_num = 1;
	    get_MBARGs(s);

	    task_fifo_wp += (TASK_BUF_LEN + 3)>>2;

	    int reach_tcsm0_end = ((unsigned int)(task_fifo_wp + (TASK_BUF_LEN>>2))) >= task_end;
	    if (reach_tcsm0_end)
	      task_fifo_wp = (int *)task_begin;

	    task_fifo_wp_d2 = task_fifo_wp_d1;
	    task_fifo_wp_d1 = task_fifo_wp;
	    (*tcsm1_fifo_wp)++;
	  }

	  int nn, tmp;
	  int time_out = 0;
	  //int *prt = (unsigned int*)TCSM1_VUCADDR(TCSM1_DBG_BUF);
	  do{
	    //printf("777777 0x%08x, 0x%08x, 0x%08x, 0x%08x\n", 
	    //prt[0], prt[1], prt[2], prt[3]);  
	    tmp = *((volatile int *)(TCSM1_VUCADDR(TCSM1_P1_TASK_DONE)));
	    time_out++;
	    if(time_out > 10000000){
		printf("- Waiting AUX for too long times, maybe it deads.\n");
		break;
	    }
	  }while (tmp == 0);
	  AUX_RESET();
#endif
	  return 0;
	}else if(ret==SLICE_NOEND){
	  dMB = task_fifo_wp;
	  dMB->real_num = -1;
	  (*tcsm1_fifo_wp)++;
	  (*tcsm1_fifo_wp)++;
	  (*tcsm1_fifo_wp)++;
	  av_log(s->avctx, AV_LOG_ERROR, "Slice mismatch at MB: %d\n", xy);
	  ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x+1, s->mb_y, (AC_END|DC_END|MV_END)&part_mask);
	  return -1;
	}
	dMB = task_fifo_wp;
	dMB->real_num = -1;

	(*tcsm1_fifo_wp)++;
	(*tcsm1_fifo_wp)++;
	(*tcsm1_fifo_wp)++;
	av_log(s->avctx, AV_LOG_ERROR, "Error at MB: %d\n", xy);
	ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x, s->mb_y, (AC_ERROR|DC_ERROR|MV_ERROR)&part_mask);

	return -1;
      }

      if(s->loop_filter)
	ff_h263_loop_filter(s);
    }

    ff_draw_horiz_band(s, s->mb_y*mb_size, mb_size);

    s->mb_x= 0;
  }
#ifdef JZC_P1_OPT
  int cnt, add_num = 5;
  for(cnt = 0; cnt < add_num; cnt++){

    do{
      //printf("333333333333333333333333333333333\n");
      tcsm0_fifo_rp_lh = (unsigned int)*tcsm0_fifo_rp & 0xFFFFF;
      task_fifo_wp_lh = (unsigned int)task_fifo_wp & 0xFFFFF;
      task_fifo_wp_lh_overlap = task_fifo_wp_lh + TASK_BUF_LEN;
    } while ( !( (task_fifo_wp_lh_overlap <= tcsm0_fifo_rp_lh) || (task_fifo_wp_lh > tcsm0_fifo_rp_lh) ) );

    dMB = task_fifo_wp;
    if (cnt == 2){
      //printf("88888888888888888888888888888888888\n");
      //printf("real_num -1 %d %d\n", dMB->mb_x, dMB->mb_y);
      dMB->real_num = -1;
    }else if (cnt == 0){
      dMB->real_num = 2;
    }else
      dMB->real_num = 1;
    get_MBARGs(s);

    task_fifo_wp += (TASK_BUF_LEN + 3)>>2;

    int reach_tcsm0_end = ((unsigned int)(task_fifo_wp + (TASK_BUF_LEN>>2))) >= task_end;
    if (reach_tcsm0_end)
      task_fifo_wp = (int *)task_begin;

    task_fifo_wp_d2 = task_fifo_wp_d1;
    task_fifo_wp_d1 = task_fifo_wp;
    (*tcsm1_fifo_wp)++;
  }

  int nn, tmp;
  int time_out = 0;
  do{
    //printf("666666666666666666666666666666666666\n");
    tmp = *((volatile int *)(TCSM1_VUCADDR(TCSM1_P1_TASK_DONE)));
    time_out++;
    if(time_out > 10000000){
	printf("- Waiting AUX for too long times, maybe it deads.\n");
	break;
    }
  }while (tmp == 0);
  AUX_RESET();

#endif// JZC_P1_OPT

  assert(s->mb_x==0 && s->mb_y==s->mb_height);

  /* try to detect the padding bug */
  if(      s->codec_id==CODEC_ID_MPEG4
	   &&   (s->workaround_bugs&FF_BUG_AUTODETECT)
	   &&    get_bits_left(&s->gb) >=0
	   &&    get_bits_left(&s->gb) < 48
	   //       &&   !s->resync_marker
	   &&   !s->data_partitioning){

    const int bits_count= get_bits_count(&s->gb);
    const int bits_left = s->gb.size_in_bits - bits_count;

    if(bits_left==0){
      s->padding_bug_score+=16;
    } else if(bits_left != 1){
      int v= show_bits(&s->gb, 8);
      v|= 0x7F >> (7-(bits_count&7));

      if(v==0x7F && bits_left<=8)
	s->padding_bug_score--;
      else if(v==0x7F && ((get_bits_count(&s->gb)+8)&8) && bits_left<=16)
	s->padding_bug_score+= 4;
      else
	s->padding_bug_score++;
    }
  }

  if(s->workaround_bugs&FF_BUG_AUTODETECT){
    if(s->padding_bug_score > -2 && !s->data_partitioning /*&& (s->divx_version>=0 || !s->resync_marker)*/)
      s->workaround_bugs |=  FF_BUG_NO_PADDING;
    else
      s->workaround_bugs &= ~FF_BUG_NO_PADDING;
  }

  // handle formats which don't have unique end markers
  if(s->msmpeg4_version || (s->workaround_bugs&FF_BUG_NO_PADDING)){ //FIXME perhaps solve this more cleanly
    int left= get_bits_left(&s->gb);
    int max_extra=7;

    /* no markers in M$ crap */
    if(s->msmpeg4_version && s->pict_type==FF_I_TYPE)
      max_extra+= 17;

    /* buggy padding but the frame should still end approximately at the bitstream end */
    if((s->workaround_bugs&FF_BUG_NO_PADDING) && s->error_recognition>=3)
      max_extra+= 48;
    else if((s->workaround_bugs&FF_BUG_NO_PADDING))
      max_extra+= 256*256*256*64;

    if(left>max_extra){
      av_log(s->avctx, AV_LOG_ERROR, "discarding %d junk bits at end, next would be %X\n", left, show_bits(&s->gb, 24));
    }
    else if(left<0){
      av_log(s->avctx, AV_LOG_ERROR, "overreading %d bits\n", -left);
    }else
      ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x-1, s->mb_y, AC_END|DC_END|MV_END);

    return 0;
  }

  av_log(s->avctx, AV_LOG_ERROR, "slice end not reached but screenspace end (%d left %06X, score= %d)\n",
	 get_bits_left(&s->gb),
	 show_bits(&s->gb, 24), s->padding_bug_score);

  ff_er_add_slice(s, s->resync_mb_x, s->resync_mb_y, s->mb_x, s->mb_y, (AC_END|DC_END|MV_END)&part_mask);

  return -1;
}

static void print_rota_frm(MpegEncContext *s, int *yidx, int *uidx, int *vidx){
  int i,j;
  char *ptr = s->current_picture.data[2];
  //char *ptr = rota_buf1;

  if (yidx[0] >= 0){
    for (j = yidx[2]; j < yidx[3]; j++){ 
      for (i = yidx[0]; i < yidx[1]; i++){
        printf("mbx:%d mby%d\n", i, j);
        ptr_square((ptr + i*256 + j*s->mb_height*256), 1, 16, 16, 16);
      }
    }
  }

  ptr = s->current_picture.data[3];
  //ptr = rota_buf2;
  if (uidx[0] >= 0){
    for (j = uidx[2]; j < uidx[3]; j++){
      for (i = uidx[0]; i < uidx[1]; i++){
        printf("mbx:%d mby%d\n", i, j);
        ptr_square((ptr + i*128 + j*s->mb_height*128), 1, 8, 8, 16);
      }
    }
  }

  if (vidx[0] >= 0){
    for (j = vidx[2]; j < vidx[3]; j++){
      for (i = vidx[0]; i < vidx[1]; i++){
        printf("mbx:%d mby%d\n", i, j);
        ptr_square((ptr + i*128 + j*s->mb_height*128 + 8), 1, 8, 8, 16);
      }
    }
  }
}

static void print_frminfo(MpegEncContext *s, int *yidx, int *uidx, int *vidx){
  int i,j;
  char *ptr = s->current_picture.data[0];

  if (yidx[0] >= 0){
    for (j = yidx[2]; j < yidx[3]; j++){ 
      for (i = yidx[0]; i < yidx[1]; i++){
        printf("mbx:%d mby:%d\n", i, j);
        ptr_square((ptr + i*256 + j*s->mb_width*256), 1, 16, 16, 16);
      }
    }
  }

  ptr = s->current_picture.data[1];
  if (uidx[0] >= 0){
    for (j = uidx[2]; j < uidx[3]; j++){
      for (i = uidx[0]; i < uidx[1]; i++){
        printf("mbx:%d mby:%d\n", i, j);
        ptr_square((ptr + i*128 + j*s->mb_width*128), 1, 8, 8, 16);
      }
    }
  }

  if (vidx[0] >= 0){
    for (j = vidx[2]; j < vidx[3]; j++){
      for (i = vidx[0]; i < vidx[1]; i++){
        printf("mbx:%d mby:%d\n", i, j);
        ptr_square((ptr + i*128 + j*s->mb_width*128 + 8), 1, 8, 8, 16);
      }
    }
  }
}

static void print_last_frminfo(MpegEncContext *s, int *yidx, int *uidx, int *vidx){
  int i,j;
  char *ptr = s->last_picture.data[0];

  printf("print last frame\n");
  if (yidx[0] >= 0){
    for (j = yidx[2]; j < yidx[3]; j++){
      for (i = yidx[0]; i < yidx[1]; i++){
        printf("mbx:%d mby%d\n", i, j);
        ptr_square((ptr + i*256 + j*s->mb_width*256 + j*1024), 1, 16, 16, 16);
      }
    }
  }

  ptr = s->last_picture.data[1];
  if (uidx[0] >= 0){
    for (j = uidx[2]; j < uidx[3]; j++){
      for (i = uidx[0]; i < uidx[1]; i++){
        printf("mbx:%d mby%d\n", i, j);
        ptr_square((ptr + i*128 + j*s->mb_width*128 + j*512), 1, 8, 8, 16);
      }
    }
  }

  if (vidx[0] >= 0){
    for (j = vidx[2]; j < vidx[3]; j++){
      for (i = vidx[0]; i < vidx[1]; i++){
        printf("mbx:%d mby%d\n", i, j);
        ptr_square((ptr + i*128 + j*s->mb_width*128 + j*512 + 8), 1, 8, 8, 16);
      }
    }
  }
}

static void check_mb_edge(MpegEncContext *s){
  if (!(s->v_edge_pos % 16) && !(s->h_edge_pos % 16))
    return;

  if (s->height % 16){
    uint8_t *sptr = s->current_picture.data[0];
    uint8_t *dptr = sptr;
    int i,j;

    sptr += (s->height / 16 * 16 * s->width);
    int useful_line = (s->height % 16);
    int lost_line = 16 - useful_line;

    for (i = 0; i < s->mb_width; i++){
      sptr += 16 * (useful_line - 1);
      dptr = sptr+16;
      for (j = 0; j < lost_line; j++){
	memcpy(dptr, sptr, 16);
	dptr+=16;
      }
      sptr += 16 * (lost_line + 1);
    }
  }

  if (s->height % 16){
    uint8_t *sptr = s->current_picture.data[1];
    uint8_t *dptr = sptr;
    int i,j;

    sptr += (s->height / 16 * 16 * s->width / 2);
    int useful_line = (s->height % 16) / 2;
    int lost_line = 8 - useful_line;

    for (i = 0; i < s->mb_width; i++){
      sptr += 16 * (useful_line - 1);
      dptr = sptr+16;
      for (j = 0; j < lost_line; j++){
	memcpy(dptr, sptr, 16);
	dptr+=16;
      }
      sptr += 16 * (lost_line + 1);
    }
  }

  if (s->width % 16){
    uint8_t *sptr = s->current_picture.data[0];
    uint8_t *dptr = sptr;
    int i,j,k;

    sptr += s->width / 16 * 256;
    int useful_line = (s->width % 16);
    int lost_line = 16 - useful_line;
    for (i = 0; i < s->mb_height; i++){
      sptr += useful_line - 1;
      dptr = sptr + 1;
      for (j = 0; j < 16; j++){
	for (k = 0; k < lost_line; k++){
	  *dptr = *sptr;
	  dptr++;
	}
	sptr += 16;
      }
      sptr = s->current_picture.data[0] + s->mb_width * 256 * (i+1) + s->width / 16 * 256;
    }
  }

  if (s->width % 16){
    uint8_t *sptr = s->current_picture.data[1];
    uint8_t *dptr = sptr;
    int i,j,k;

    sptr += s->width / 16 * 128;
    int useful_line = (s->width % 16) / 2;
    int lost_line = 8 - useful_line;
    for (i = 0; i < s->mb_height; i++){
      sptr += useful_line - 1;
      dptr = sptr + 1;
      for (j = 0; j < 8; j++){
	for (k = 0; k < lost_line; k++){
	  *dptr = *sptr;
	  dptr++;
	}
	sptr += 8;
	dptr = sptr + 1;
	for (k = 0; k < lost_line; k++){
	  *dptr = *sptr;
	  dptr++;
	}
	sptr += 8;
      }
      sptr = s->current_picture.data[1] + s->mb_width * 128 * (i+1) + s->width / 16 * 128;
    }
  }

  return;
}

static void mpeg4_tile_stuff(MpegEncContext *s){
#if 0
  volatile uint8_t *tile_y, *tile_c, *pxl, *edge, *tile, *dest;
  int mb_i, mb_j, i, j;
    
  tile_y = s->current_picture.data[0];
  tile_c = s->current_picture.data[1];
      
  for(mb_i=0; mb_i<s->mb_width; mb_i++){
    // Y
    pxl = tile_y + mb_i*16*16;
    edge = pxl - s->linesize*(EDGE_WIDTH/16);
    for(j=0; j<16; j++){
      memcpy(edge, pxl, 16);
      memcpy(edge+s->linesize, pxl, 16);
      edge += 16;
    } 
    // C
    pxl = tile_c + mb_i*16*8;
    edge = pxl - s->linesize;
    for(j=0; j<8; j++){
      memcpy(edge, pxl, 16);
      memcpy(edge+s->linesize/2, pxl, 16);
      edge += 16;
    } 
  }   
  /********************* bottom ***********************/
  for(mb_i=0; mb_i<s->mb_width; mb_i++){
    // Y
    edge = tile_y + s->mb_height*s->linesize + mb_i*16*16;
    pxl = edge - s->linesize + 16*15;
    for(j=0; j<16; j++){
      memcpy(edge, pxl, 16);
      memcpy(edge+s->linesize, pxl, 16);
      edge += 16;
    }
    // C
    edge = tile_c + s->mb_height*s->uvlinesize + mb_i*16*8;
    pxl = edge - s->uvlinesize + 16*7;
    for(j=0; j<8; j++){
      memcpy(edge, pxl, 16);
      memcpy(edge+s->uvlinesize, pxl, 16);
      edge += 16;
    }
  }

  /********************* left ***********************/
  for(mb_i=-2; mb_i<s->mb_height+2; mb_i++){
    // Y
    for(j=0; j<16; j++){
      pxl = tile_y + mb_i*s->linesize + 16*j;
      edge = pxl - 16*(EDGE_WIDTH);
      memset(edge, pxl[0], 16);
      memset(edge+16*16, pxl[0], 16);
    }
    // C
    for(j=0; j<8; j++){
      pxl = tile_c + mb_i*s->uvlinesize + 16*j;
      edge = pxl - 8*(EDGE_WIDTH);
      memset(edge, pxl[0], 8);
      memset(edge+8, pxl[8], 8);
      memset(edge+8*16, pxl[0], 8);
      memset(edge+8*16+8, pxl[8], 8);
    }
  }
  /********************* right ***********************/
  for(mb_i=-2; mb_i<s->mb_height+2; mb_i++){
    // Y
    for(j=0; j<16; j++){
      pxl = tile_y + (s->mb_width-1)*16*16 + 15 + mb_i*s->linesize + 16*j;
      edge = pxl - 15 + 16*16;
      memset(edge, pxl[0], 16);
      memset(edge+16*16, pxl[0], 16);
    }
    // C
    for(j=0; j<8; j++){
      pxl = tile_c + (s->mb_width-1)*16*8 + 7 + mb_i*s->uvlinesize + 16*j;
      edge = pxl - 7 + 16*8;
      memset(edge, pxl[0], 8);
      memset(edge+8, pxl[8], 8);
      memset(edge+8*16, pxl[0], 8);
      memset(edge+8*16+8, pxl[8], 8);
    }
  }
#else
  volatile uint8_t *tile_y, *tile_c;
  tile_y = s->current_picture.data[0];
  tile_c = s->current_picture.data[1];

  uint32_t src_ptr,dst_ptr,src_cptr,dst_cptr,i,j,k;
  src_ptr=tile_y;
  dst_ptr=tile_y-s->linesize*2-512-32;
  src_cptr=tile_c;
  dst_cptr=tile_c-s->linesize-256-16;
  for(k=0;k<2;k++){
    S8LDD(xr5,src_ptr,0,7);
    S8LDD(xr6,src_cptr,0,7);
    S8LDD(xr7,src_cptr,8,7);

    S32LDD(xr8,src_ptr,0);
    S32LDD(xr9,src_ptr,4);
    S32LDD(xr10,src_ptr,8);
    S32LDD(xr11,src_ptr,12);
    S32LDD(xr12,src_cptr,0);
    S32LDD(xr13,src_cptr,4);
    S32LDD(xr14,src_cptr,8);
    S32LDD(xr15,src_cptr,12);
    for(i=0;i<2;i++){
      for(j=0;j<16;j++){
	S32SDI(xr5,dst_ptr,32);
	S32STD(xr5,dst_ptr,4);
	S32STD(xr5,dst_ptr,8);
	S32STD(xr5,dst_ptr,12);
	S32STD(xr5,dst_ptr,16);
	S32STD(xr5,dst_ptr,20);
	S32STD(xr5,dst_ptr,24);
	S32STD(xr5,dst_ptr,28);
      }
      for(j=0;j<8;j++){
	S32SDI(xr8,dst_ptr,32);
	S32STD(xr9,dst_ptr,4);
	S32STD(xr10,dst_ptr,8);
	S32STD(xr11,dst_ptr,12);
	S32STD(xr8,dst_ptr,16);
	S32STD(xr9,dst_ptr,20);
	S32STD(xr10,dst_ptr,24);
	S32STD(xr11,dst_ptr,28);
      }
      for(j=0;j<16;j++){
	S32SDI(xr6,dst_cptr,16);
	S32STD(xr6,dst_cptr,4);
	S32STD(xr7,dst_cptr,8);
	S32STD(xr7,dst_cptr,12);
      }
      for(j=0;j<8;j++){
	S32SDI(xr12,dst_cptr,16);
	S32STD(xr13,dst_cptr,4);
	S32STD(xr14,dst_cptr,8);
	S32STD(xr15,dst_cptr,12);
      }
      dst_ptr+=s->linesize-768;
      dst_cptr+=s->uvlinesize-384;
    }
    src_ptr=tile_y+(s->mb_height-1)*s->linesize+240;
    src_cptr=tile_c+(s->mb_height-1)*s->uvlinesize+112;
    dst_ptr=tile_y+(s->mb_height)*s->linesize-512-32;
    dst_cptr=tile_c+(s->mb_height)*s->uvlinesize-256-16;
  }
  src_ptr=tile_y+(s->mb_width-1)*256;
  dst_ptr=tile_y+(s->mb_width-1)*256-s->linesize*2-32;
  src_cptr=tile_c+(s->mb_width-1)*128;
  dst_cptr=tile_c+(s->mb_width-1)*128-s->linesize-16;
  for(k=0;k<2;k++){
    S8LDD(xr5,src_ptr,15,7);
    S8LDD(xr6,src_cptr,7,7);
    S8LDD(xr7,src_cptr,15,7);

    S32LDD(xr8,src_ptr,0);
    S32LDD(xr9,src_ptr,4);
    S32LDD(xr10,src_ptr,8);
    S32LDD(xr11,src_ptr,12);
    S32LDD(xr12,src_cptr,0);
    S32LDD(xr13,src_cptr,4);
    S32LDD(xr14,src_cptr,8);
    S32LDD(xr15,src_cptr,12);
    for(i=0;i<2;i++){
      for(j=0;j<8;j++){
	S32SDI(xr8,dst_ptr,32);
	S32STD(xr9,dst_ptr,4);
	S32STD(xr10,dst_ptr,8);
	S32STD(xr11,dst_ptr,12);
	S32STD(xr8,dst_ptr,16);
	S32STD(xr9,dst_ptr,20);
	S32STD(xr10,dst_ptr,24);
	S32STD(xr11,dst_ptr,28);
      }
      for(j=0;j<16;j++){
	S32SDI(xr5,dst_ptr,32);
	S32STD(xr5,dst_ptr,4);
	S32STD(xr5,dst_ptr,8);
	S32STD(xr5,dst_ptr,12);
	S32STD(xr5,dst_ptr,16);
	S32STD(xr5,dst_ptr,20);
	S32STD(xr5,dst_ptr,24);
	S32STD(xr5,dst_ptr,28);
      }
      for(j=0;j<8;j++){
	S32SDI(xr12,dst_cptr,16);
	S32STD(xr13,dst_cptr,4);
	S32STD(xr14,dst_cptr,8);
	S32STD(xr15,dst_cptr,12);
      }
      for(j=0;j<16;j++){
	S32SDI(xr6,dst_cptr,16);
	S32STD(xr6,dst_cptr,4);
	S32STD(xr7,dst_cptr,8);
	S32STD(xr7,dst_cptr,12);
      }
      dst_ptr+=s->linesize-768;
      dst_cptr+=s->uvlinesize-384;
    }
    src_ptr=tile_y+(s->mb_height-1)*s->linesize+(s->mb_width-1)*256+240;
    src_cptr=tile_c+(s->mb_height-1)*s->uvlinesize+(s->mb_width-1)*128+112;
    dst_ptr=tile_y+(s->mb_height)*s->linesize+(s->mb_width-1)*256-32;
    dst_cptr=tile_c+(s->mb_height)*s->uvlinesize+(s->mb_width-1)*128-16;
  }
#endif
}

int mpeg4_decode_frame(AVCodecContext *avctx,
		       void *data, int *data_size,
		       AVPacket *avpkt)
{
  const uint8_t *buf = avpkt->data;
  int buf_size = avpkt->size;
  MpegEncContext *s = avctx->priv_data;
  int ret;
  AVFrame *pict = data;

#ifdef PRINT_FRAME_TIME
  uint64_t time= rdtsc();
#endif
  s->flags= avctx->flags;
  s->flags2= avctx->flags2;

#ifdef JZC_CRC_VER
  mpFrame++;
  //printf("mpeg4_decode_frame %d\n", mpFrame);
#endif

  /* no supplementary picture */
  if (buf_size == 0) {
    /* special case for last picture */
    if (s->low_delay==0 && s->next_picture_ptr) {
      *pict= *(AVFrame*)s->next_picture_ptr;
      s->next_picture_ptr= NULL;

      *data_size = sizeof(AVFrame);
    }

    return 0;
  }

  if(s->flags&CODEC_FLAG_TRUNCATED){
    int next;

    if(CONFIG_MPEG4_DECODER && s->codec_id==CODEC_ID_MPEG4){
      next= ff_mpeg4_find_frame_end(&s->parse_context, buf, buf_size);
    }else if(CONFIG_H263_DECODER && s->codec_id==CODEC_ID_H263){
      next= ff_h263_find_frame_end(&s->parse_context, buf, buf_size);
    }else{
      av_log(s->avctx, AV_LOG_ERROR, "this codec does not support truncated bitstreams\n");
      return -1;
    }

    if( ff_combine_frame(&s->parse_context, next, (const uint8_t **)&buf, &buf_size) < 0 )
      return buf_size;
  }


 retry:

  if(s->bitstream_buffer_size && (s->divx_packed || buf_size<20)){ //divx 5.01+/xvid frame reorder
    init_get_bits(&s->gb, s->bitstream_buffer, s->bitstream_buffer_size*8);
  }else
    init_get_bits(&s->gb, buf, buf_size*8);
  s->bitstream_buffer_size=0;

  if (!s->context_initialized) {
    if (MPV_common_init(s) < 0) //we need the idct permutaton for reading a custom matrix
      return -1;
  }

  /* We need to set current_picture_ptr before reading the header,
   * otherwise we cannot store anyting in there */
  if(s->current_picture_ptr==NULL || s->current_picture_ptr->data[0]){
    int i= ff_find_unused_picture(s, 0);
    s->current_picture_ptr= &s->picture[i];
  }

  /* let's go :-) */
  if (CONFIG_WMV2_DECODER && s->msmpeg4_version==5) {
    //printf("WMV2\n");
    ret= ff_wmv2_decode_picture_header(s);
  } else if (CONFIG_MSMPEG4_DECODER && s->msmpeg4_version) {
    //printf("decode_frame, msmpeg4\n");
    ret = msmpeg4_decode_picture_header(s);
  } else if (CONFIG_MPEG4_DECODER && s->h263_pred) {
    //printf("decode_frame, ff_mpeg4\n");
    if(s->avctx->extradata_size && s->picture_number==0){
      GetBitContext gb;

      init_get_bits(&gb, s->avctx->extradata, s->avctx->extradata_size*8);
      //ret = ff_mpeg4_decode_picture_header(s, &gb);
     ret = JZC_mpeg4_decode_picture_header(s, &gb);
    }
    //ret = ff_mpeg4_decode_picture_header(s, &s->gb);
    ret = JZC_mpeg4_decode_picture_header(s, &s->gb);
  } else if (CONFIG_H263I_DECODER && s->codec_id == CODEC_ID_H263I) {
    //printf("H263I\n");
    ret = ff_intel_h263_decode_picture_header(s);
  } else if (CONFIG_FLV_DECODER && s->h263_flv) {
    //printf("FLV\n");
    ret = ff_flv_decode_picture_header(s);
  } else {
    ret = h263_decode_picture_header(s);
  }

  if(ret==FRAME_SKIPPED) return get_consumed_bytes(s, buf_size);

  /* skip if the header was thrashed */
  if (ret < 0){
    //av_log(s->avctx, AV_LOG_ERROR, "header damaged\n");
    return -1;
  }

  avctx->has_b_frames= !s->low_delay;

  if(s->xvid_build==-1 && s->divx_version==-1 && s->lavc_build==-1){
    if(s->stream_codec_tag == AV_RL32("XVID") ||
       s->codec_tag == AV_RL32("XVID") || s->codec_tag == AV_RL32("XVIX") ||
       s->codec_tag == AV_RL32("RMP4") ||
       s->codec_tag == AV_RL32("SIPP")
       )
      s->xvid_build= 0;
#if 0
    if(s->codec_tag == AV_RL32("DIVX") && s->vo_type==0 && s->vol_control_parameters==1
       && s->padding_bug_score > 0 && s->low_delay) // XVID with modified fourcc
      s->xvid_build= 0;
#endif
  }

  if(s->xvid_build==-1 && s->divx_version==-1 && s->lavc_build==-1){
    if(s->codec_tag == AV_RL32("DIVX") && s->vo_type==0 && s->vol_control_parameters==0)
      s->divx_version= 400; //divx 4
  }

  if(s->xvid_build>=0 && s->divx_version>=0){
    s->divx_version=
      s->divx_build= -1;
  }

  if(s->workaround_bugs&FF_BUG_AUTODETECT){
    if(s->codec_tag == AV_RL32("XVIX"))
      s->workaround_bugs|= FF_BUG_XVID_ILACE;

    if(s->codec_tag == AV_RL32("UMP4")){
      s->workaround_bugs|= FF_BUG_UMP4;
    }

    if(s->divx_version>=500 && s->divx_build<1814){
      s->workaround_bugs|= FF_BUG_QPEL_CHROMA;
    }

    if(s->divx_version>502 && s->divx_build<1814){
      s->workaround_bugs|= FF_BUG_QPEL_CHROMA2;
    }

    if(s->xvid_build<=3U)
      s->padding_bug_score= 256*256*256*64;

    if(s->xvid_build<=1U)
      s->workaround_bugs|= FF_BUG_QPEL_CHROMA;

    if(s->xvid_build<=12U)
      s->workaround_bugs|= FF_BUG_EDGE;

    if(s->xvid_build<=32U)
      s->workaround_bugs|= FF_BUG_DC_CLIP;

#define SET_QPEL_FUNC(postfix1, postfix2)				\
    s->dsp.put_ ## postfix1 = ff_put_ ## postfix2;			\
    s->dsp.put_no_rnd_ ## postfix1 = ff_put_no_rnd_ ## postfix2;	\
    s->dsp.avg_ ## postfix1 = ff_avg_ ## postfix2;

    if(s->lavc_build<4653U)
      s->workaround_bugs|= FF_BUG_STD_QPEL;

    if(s->lavc_build<4655U)
      s->workaround_bugs|= FF_BUG_DIRECT_BLOCKSIZE;

    if(s->lavc_build<4670U){
      s->workaround_bugs|= FF_BUG_EDGE;
    }

    if(s->lavc_build<=4712U)
      s->workaround_bugs|= FF_BUG_DC_CLIP;

    if(s->divx_version>=0)
      s->workaround_bugs|= FF_BUG_DIRECT_BLOCKSIZE;
    //printf("padding_bug_score: %d\n", s->padding_bug_score);
    if(s->divx_version==501 && s->divx_build==20020416)
      s->padding_bug_score= 256*256*256*64;

    if(s->divx_version<500U){
      s->workaround_bugs|= FF_BUG_EDGE;
    }

    if(s->divx_version>=0)
      s->workaround_bugs|= FF_BUG_HPEL_CHROMA;
#if 0
    if(s->divx_version==500)
      s->padding_bug_score= 256*256*256*64;

    /* very ugly XVID padding bug detection FIXME/XXX solve this differently
     * Let us hope this at least works.
     */
    if(   s->resync_marker==0 && s->data_partitioning==0 && s->divx_version==-1
	  && s->codec_id==CODEC_ID_MPEG4 && s->vo_type==0)
      s->workaround_bugs|= FF_BUG_NO_PADDING;

    if(s->lavc_build<4609U) //FIXME not sure about the version num but a 4609 file seems ok
      s->workaround_bugs|= FF_BUG_NO_PADDING;
#endif
  }

  if(s->workaround_bugs& FF_BUG_STD_QPEL){
      SET_QPEL_FUNC(qpel_pixels_tab[0][ 5], qpel16_mc11_old_c)
      SET_QPEL_FUNC(qpel_pixels_tab[0][ 7], qpel16_mc31_old_c)
      SET_QPEL_FUNC(qpel_pixels_tab[0][ 9], qpel16_mc12_old_c)
      SET_QPEL_FUNC(qpel_pixels_tab[0][11], qpel16_mc32_old_c)
      SET_QPEL_FUNC(qpel_pixels_tab[0][13], qpel16_mc13_old_c)
      SET_QPEL_FUNC(qpel_pixels_tab[0][15], qpel16_mc33_old_c)

      SET_QPEL_FUNC(qpel_pixels_tab[1][ 5], qpel8_mc11_old_c)
      SET_QPEL_FUNC(qpel_pixels_tab[1][ 7], qpel8_mc31_old_c)
      SET_QPEL_FUNC(qpel_pixels_tab[1][ 9], qpel8_mc12_old_c)
      SET_QPEL_FUNC(qpel_pixels_tab[1][11], qpel8_mc32_old_c)
      SET_QPEL_FUNC(qpel_pixels_tab[1][13], qpel8_mc13_old_c)
      SET_QPEL_FUNC(qpel_pixels_tab[1][15], qpel8_mc33_old_c)
      }

  if(avctx->debug & FF_DEBUG_BUGS)
    av_log(s->avctx, AV_LOG_DEBUG, "bugs: %X lavc_build:%d xvid_build:%d divx_version:%d divx_build:%d %s\n",
	   s->workaround_bugs, s->lavc_build, s->xvid_build, s->divx_version, s->divx_build,
	   s->divx_packed ? "p" : "");

#if 0 // dump bits per frame / qp / complexity
  {
    static FILE *f=NULL;
    if(!f) f=fopen("rate_qp_cplx.txt", "w");
    fprintf(f, "%d %d %f\n", buf_size, s->qscale, buf_size*(double)s->qscale);
  }
#endif

#if HAVE_MMX
  if (s->codec_id == CODEC_ID_MPEG4 && s->xvid_build>=0 && avctx->idct_algo == FF_IDCT_AUTO && (av_get_cpu_flags() & AV_CPU_FLAG_MMX)) {
    avctx->idct_algo= FF_IDCT_XVIDMMX;
    avctx->coded_width= 0; // force reinit
    //        dsputil_init(&s->dsp, avctx);
    s->picture_number=0;
  }
#endif

  /* After H263 & mpeg4 header decode we have the height, width,*/
  /* and other parameters. So then we could init the picture   */
  /* FIXME: By the way H263 decoder is evolving it should have */
  /* an H263EncContext                                         */

  if (   s->width  != avctx->coded_width
	 || s->height != avctx->coded_height) {
    /* H.263 could change picture size any time */
    ParseContext pc= s->parse_context; //FIXME move these demuxng hack to avformat
    s->parse_context.buffer=0;
		if (s->width > 1920 || s->height > 1080)
			return -1;
    MPV_common_end(s);
    s->parse_context= pc;
  }
  if (!s->context_initialized) {
    avcodec_set_dimensions(avctx, s->width, s->height);

    goto retry;
  }
  if((s->codec_id==CODEC_ID_H263 || s->codec_id==CODEC_ID_H263P || s->codec_id == CODEC_ID_H263I))
    s->gob_index = ff_h263_get_gob_height(s);

  // for hurry_up==5
  s->current_picture.pict_type= s->pict_type;
  s->current_picture.key_frame= s->pict_type == FF_I_TYPE;

  /* skip B-frames if we don't have reference frames */
  if(s->last_picture_ptr==NULL && (s->pict_type==FF_B_TYPE || s->dropable)) return get_consumed_bytes(s, buf_size);
  /* skip b frames if we are in a hurry */
  if(avctx->hurry_up && s->pict_type==FF_B_TYPE) return get_consumed_bytes(s, buf_size);
  if(   (avctx->skip_frame >= AVDISCARD_NONREF && s->pict_type==FF_B_TYPE)
	|| (avctx->skip_frame >= AVDISCARD_NONKEY && s->pict_type!=FF_I_TYPE)
	||  avctx->skip_frame >= AVDISCARD_ALL)
    return get_consumed_bytes(s, buf_size);
  /* skip everything if we are in a hurry>=5 */
  if(avctx->hurry_up>=5) return get_consumed_bytes(s, buf_size);

  if(s->next_p_frame_damaged){
    if(s->pict_type==FF_B_TYPE)
      return get_consumed_bytes(s, buf_size);
    else
      s->next_p_frame_damaged=0;
  }

  if((s->avctx->flags2 & CODEC_FLAG2_FAST) && s->pict_type==FF_B_TYPE){
    s->me.qpel_put= s->dsp.put_2tap_qpel_pixels_tab;
    s->me.qpel_avg= s->dsp.avg_2tap_qpel_pixels_tab;
  }else if((!s->no_rounding) || s->pict_type==FF_B_TYPE){
    s->me.qpel_put= s->dsp.put_qpel_pixels_tab;
    s->me.qpel_avg= s->dsp.avg_qpel_pixels_tab;
    mpeg4_motion_set(MPEG_QPEL, MPEG_HPEL);
  }else{
    s->me.qpel_put= s->dsp.put_no_rnd_qpel_pixels_tab;
    s->me.qpel_avg= s->dsp.avg_qpel_pixels_tab;
    mpeg4_motion_set(MPEG_NRND, MPEG_HPEL);
  }

  if(MPV_frame_start(s, avctx) < 0)
    return -1;

  if (CONFIG_MPEG4_VDPAU_DECODER && (s->avctx->codec->capabilities & CODEC_CAP_HWACCEL_VDPAU)) {
    ff_vdpau_mpeg4_decode_picture(s, s->gb.buffer, s->gb.buffer_end - s->gb.buffer);
    goto frame_end;
  }

  if (avctx->hwaccel) {
    if (avctx->hwaccel->start_frame(avctx, s->gb.buffer, s->gb.buffer_end - s->gb.buffer) < 0)
      return -1;
  }

  ff_er_frame_start(s);

  //the second part of the wmv2 header contains the MB skip bits which are stored in current_picture->mb_type
  //which is not available before MPV_frame_start()
  if (CONFIG_WMV2_DECODER && s->msmpeg4_version==5){
    ret = ff_wmv2_decode_secondary_picture_header(s);
    if(ret<0) return ret;
    if(ret==1) goto intrax8_decoded;
  }

  /* decode each macroblock */
  s->mb_x=0;
  s->mb_y=0;

  decode_slice(s);
  while(s->mb_y<s->mb_height){
    if(s->msmpeg4_version){
      if(s->slice_height==0 || s->mb_x!=0 || (s->mb_y%s->slice_height)!=0 || get_bits_count(&s->gb) > s->gb.size_in_bits)
	break;
    }else{
      if(ff_h263_resync(s)<0)
	break;
    }

    if(s->msmpeg4_version<4 && s->h263_pred)
      ff_mpeg4_clean_buffers(s);

    printf("mul slice %d %d\n", s->mb_x, s->mb_y);
    decode_slice(s);
  }

  if (s->h263_msmpeg4 && s->msmpeg4_version<4 && s->pict_type==FF_I_TYPE)
    if(!CONFIG_MSMPEG4_DECODER || msmpeg4_decode_ext_header(s, buf_size) < 0){
      s->error_status_table[s->mb_num-1]= AC_ERROR|DC_ERROR|MV_ERROR;
    }

  assert(s->bitstream_buffer_size==0);
 frame_end:
  /* divx 5.01+ bistream reorder stuff */
  if(s->codec_id==CODEC_ID_MPEG4 && s->divx_packed){
    int current_pos= get_bits_count(&s->gb)>>3;
    int startcode_found=0;

    if(buf_size - current_pos > 5){
      int i;
      for(i=current_pos; i<buf_size-3; i++){
	if(buf[i]==0 && buf[i+1]==0 && buf[i+2]==1 && buf[i+3]==0xB6){
	  startcode_found=1;
	  break;
	}
      }
    }
    if(s->gb.buffer == s->bitstream_buffer && buf_size>7 && s->xvid_build>=0){ //xvid style
      startcode_found=1;
      current_pos=0;
    }

    if(startcode_found){
      av_fast_malloc(
		     &s->bitstream_buffer,
		     &s->allocated_bitstream_buffer_size,
		     buf_size - current_pos + FF_INPUT_BUFFER_PADDING_SIZE);
      if (!s->bitstream_buffer)
	return AVERROR(ENOMEM);
      memcpy(s->bitstream_buffer, buf + current_pos, buf_size - current_pos);
      s->bitstream_buffer_size= buf_size - current_pos;
    }
  }

 intrax8_decoded:
  //ff_er_frame_end(s);//hpwang 20101119

  if (avctx->hwaccel) {
    if (avctx->hwaccel->end_frame(avctx) < 0)
      return -1;
  }

  MPV_frame_end(s);

  check_mb_edge(s);
#if 1
  if (dFRM.edge)
    mpeg4_tile_stuff(s);
#endif

  linear_crc_frame(s);

#if 0
  jz_dcache_wb();
  if (mpFrame == 12){
    int yidx[4];
    int uidx[4];
    int vidx[4];
        
    //printf("Frame34 cu 0x%08x\n", s->current_picture.data[0]);
          
    yidx[0] = 0;
    yidx[1] = s->mb_width;
    //yidx[1] = 8;
    yidx[2] = 0;
    yidx[3] = s->mb_height;
    //yidx[3] = 1;

    uidx[0] = 0;
    //uidx[1] = 2;
    uidx[1] = s->mb_width;
    uidx[2] = 0;
    //uidx[3] = 1;
    uidx[3] = s->mb_height;
            
    vidx[0] = 0;
    vidx[1] = s->mb_width;
    vidx[2] = 0;
    vidx[3] = s->mb_height;
        
    print_frminfo(s, yidx, uidx, vidx);

    yidx[0] = 0;
    yidx[1] = s->mb_height;
    //yidx[1] = 3;
    yidx[2] = 0;
    yidx[3] = s->mb_width;
    //yidx[3] = 1;

    uidx[0] = 0;
    uidx[1] = s->mb_height;
    uidx[2] = 0;
    uidx[3] = s->mb_width;
    //uidx[3] = 31;
            
    vidx[0] = 0;
    vidx[1] = s->mb_height;
    vidx[2] = 0;
    vidx[3] = s->mb_width;
    //print_rota_frm(s, yidx, uidx, vidx);
  } 
#endif
  assert(s->current_picture.pict_type == s->current_picture_ptr->pict_type);
  assert(s->current_picture.pict_type == s->pict_type);
  if (s->pict_type == FF_B_TYPE || s->low_delay) {
    *pict= *(AVFrame*)s->current_picture_ptr;
  } else if (s->last_picture_ptr != NULL) {
    *pict= *(AVFrame*)s->last_picture_ptr;
  }

  if(s->last_picture_ptr || s->low_delay){
    *data_size = sizeof(AVFrame);
    ff_print_debug_info(s, pict);
  }

#ifdef PRINT_FRAME_TIME
  av_log(avctx, AV_LOG_DEBUG, "%"PRId64"\n", rdtsc()-time);
#endif
  return get_consumed_bytes(s, buf_size);
}
