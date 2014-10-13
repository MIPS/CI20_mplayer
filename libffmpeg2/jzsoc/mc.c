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

//-- JZ VMAU MCE
#include "../../libjzcommon/jzmedia.h"
#include "../../libjzcommon/jzasm.h"
#include "ffmpeg2_p1_type.h"
#include "ffmpeg2_tcsm1.h"
#include "ffmpeg2_dcore.h"
#include "jzdsputil.h"

#define MPEG2_P1_USE_PADDR
#include "t_motion.h" 
#include "t_intpid.h"
#include "vmau.h"

//#define IS_ILUT1  2
//#define IS_ILUT2  3
#define MPEG_HPEL  0

static void inline frame_mc_16x16( MPEG2_FRAME_GloARGs* dFRM, MPEG2_MB_DecARGs * dMB, unsigned int * dha)
{
    uint16_t mbx   = dMB->mb_x;
    uint16_t mby   = dMB->mb_y;
    uint8_t  dir   = dMB->mv_dir & 0x3;

    if ( !dMB->interlaced_dct ) //if interlacd_dct, we have to use C version idct, so 'ofa' should be 0;
    {
      //              ebms,esms,earm,epmv,esa,ebme,cae, pgc,ch2en,pri,ckge,ofa,rot,rotdir,wm,ccf,irqe,rst,en
         SET_REG1_CTRL(0,   0,   0,   0,   0,  0,   1,  0xF, 1,    3,  1,   1,  0,  0,     0, 1,  0,   0,  1);
    }else{
         SET_REG1_CTRL(0,   0,   0,   0,   0,  0,   1,  0xF, 1,    3,  1,   0,  0,  0,     0, 1,  0,   0,  1);
    }
    int *ymv_f = dMB->mv[0][0]; 
    int *ymv_b = dMB->mv[1][0]; 
    int cmv_f[2];
    int cmv_b[2];
    cmv_f[0] = ymv_f[0]/2;
    cmv_f[1] = ymv_f[1]/2;
    cmv_b[0] = ymv_b[0]/2;
    cmv_b[1] = ymv_b[1]/2;

    /*---------------      MC chain     ------------*/
    unsigned int *tdd= dha;
    int tkn = 0;
    tdd[tkn++]=TDD_HEAD(1,/*vld*/
	  		1,/*lk*/
                        0,/*op*/
			SubPel[MPEG_HPEL]-1,/*ch1pel*/
			SubPel[MPEG_HPEL]-1,/*ch2pel*/
			0,/*posmd*/
			1,/*mvmd*/
			(dir==3)? 4: 2,/*tkn*/
			mby,/*mby*/
			mbx/*mbx*/
			);
    if (dir & 1){
      tdd[tkn++] = TDD_MV(ymv_f[1], ymv_f[0]);
      tdd[tkn++] = TDD_CMD((dir==3)? 1: 0,/*bidir*/
			 0,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 (dir & 2)?0:1,/*doe*/
			 (dir & 2)?0:1,/*cflo*/
			 0,//pos_f[1],/*ypos*/
			 3,//no_rnd?(IS_ILUT1):(IS_ILUT2),/*lilmd*/   /*--always no rnd??*/
			 3,/*cilmd*/
			 0,/*list*/
			 0,/*boy*/
			 0,/*box*/
			 BLK_H16,/*bh*/
			 BLK_W16,/*bw*/
			 0//pos_f[0]/*xpos*/
			 );
    }
    if (dir & 2){
      tdd[tkn++] = TDD_MV (ymv_b[1], ymv_b[0]);
      tdd[tkn++] = TDD_CMD((dir==3)? 1: 0,/*bidir*/
			 1,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 1,/*doe*/
			 1,/*cflo*/
			 0,//pos_b[1],/*ypos*/
			 3,//no_rnd?(IS_ILUT1):(IS_ILUT2),/*lilmd*/
			 3,/*cilmd*/
			 1,/*list*/
			 0,/*boy*/
			 0,/*box*/
			 BLK_H16,/*bh*/
			 BLK_W16,/*bw*/
			 0//pos_b[0]/*xpos*/
			 );
   }
   if (dir & 1){
      tdd[tkn++] = TDD_MV((cmv_f[1]), (cmv_f[0]));
      tdd[tkn++] = TDD_CMD((dir==3)? 1: 0,/*bidir*/
			 0,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 (dir & 2)?0:1,/*doe*/
			 0,/*cflo*/
			 0,//c_pos_f[1],/*ypos*/
			 3,/*lilmd*/
			 3,//no_rnd?(IS_ILUT1):(IS_ILUT2),/*cilmd*/
			 0,/*list*/
			 0,/*boy*/
			 0,/*box*/
			 BLK_H16,/*bh*/
			 BLK_W16,/*bw*/
			 0//c_pos_f[0]/*xpos*/
			 );
    }
    if (dir & 2){
      tdd[tkn++] = TDD_MV((cmv_b[1]), (cmv_b[0]));
      tdd[tkn++] = TDD_CMD((dir== 3)? 1: 0,/*bidir*/
			 1,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 1,/*doe*/
			 0,/*cflo*/
			 0,//c_pos_b[1],/*ypos*/
		         3,/*lilmd*/
			 3,//no_rnd?(IS_ILUT1):(IS_ILUT2),/*cilmd*/
			 1,/*list*/
			 0,/*boy*/
			 0,/*box*/
			 BLK_H16,/*bh*/
			 BLK_W16,/*bw*/
			 0//c_pos_b[0]/*xpos*/
			 );
    }
    tdd[tkn++] = TDD_HEAD(1,/*vld*/
		      1,/*lk*/
		      0,/*op*/
		      SubPel[MPEG_HPEL]-1,/*ch1pel*/
		      SubPel[MPEG_HPEL]-1,/*ch2pel*/
		      0,/*posmd*/
		      1,/*mvmd*/
		      0,/*tkn*/
		      0x0,/*mby*/
		      0x0 /*mbx*/);

    tdd[tkn++] = SYN_HEAD(1, 0, 2, 0, 0xFFFF);
    dMB->cbp |= (MAU_C_ERR_MSK << MAU_C_ERR_SFT ) | (MAU_Y_ERR_MSK << MAU_Y_ERR_SFT );
}
