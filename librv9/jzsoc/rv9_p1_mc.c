/*******************************************************
 Motion Test Center
 ******************************************************/
#define __place_k0_data__
#undef printf
#undef fprintf
#include "t_motion.h"
#include "rv9_dcore.h"

#define MPEG_HPEL  0
#define MPEG_QPEL  1
#define H264_QPEL  2
#define H264_EPEL  3
#define RV8_TPEL   4
#define RV9_QPEL   5
#define RV9_CPEL   6 
#define WMV2_QPEL  7
#define VC1_QPEL   8
#define AVS_QPEL   9
#define VP6_QPEL   10
#define VP8_QPEL   11

enum SPelSFT {
    HPEL = 1,
    QPEL,
    EPEL,
};

#define IS_EC     1
#define IS_ILUT0  0
#define IS_ILUT1  2

static int rv40_task_write(RV9_MB_DecARGs *dMB,
			   int mv_off, volatile int *tdd, int *tkn, int blkh, int blkw, int boy, int box,
			   int is_bidir, int dir, int doe, int cflo, int chroma)
{
    int mvx, mvy, pos;

    mvx = dMB->motion_val[dir][mv_off][0] / (1<<chroma);
    mvy = dMB->motion_val[dir][mv_off][1] / (1<<chroma);

    pos = ((mvy & 0x3) << 2) | (mvx & 0x3);

    tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
    tdd[ 2*tkn[0]+1 ] = TDD_CMD(is_bidir,/*bidir*/
				dir,/*refdir*/
				0,/*fld*/
				0,/*fldsel*/
				0,/*rgr*/
				0,/*its*/
				doe,/*doe*/
				cflo,/*cflo*/
				0,/*ypos*/
				IS_ILUT0,/*lilmd*/
				IS_ILUT0,/*cilmd*/
				0, /*list*/     
				boy,/*boy*/
				box,/*box*/
				blkh,/*bh*/
				blkw,/*bw*/
				pos/*xpos*/);
    (*tkn)++;

    return 0;
}

static int rv40_1mv_mc(RV9_MB_DecARGs *dMB,
		       int mv_off, volatile int *tdd, int *tkn, int blkh, int blkw, int boy, int box,
		       int dir0, int dir1)
{
    int is_bidir = dir0 && dir1;
    // Y
    if( dir0 )
	rv40_task_write(dMB, mv_off, tdd, tkn, blkh, blkw, boy, box, is_bidir, 0, !dir1, !dir1, 0);

    if( dir1 )
	rv40_task_write(dMB, mv_off, tdd, tkn, blkh, blkw, boy, box, is_bidir, 1, 1, 1, 0);
    // C
    if( dir0 )
	rv40_task_write(dMB, mv_off, tdd, tkn, blkh, blkw, boy, box, is_bidir, 0, !dir1, 0, 1);

    if( dir1 )
	rv40_task_write(dMB, mv_off, tdd, tkn, blkh, blkw, boy, box, is_bidir, 1, 1, 0, 1);

    return 0;
}

static int rv40_2mv_mc(RV9_MB_DecARGs *dMB,
		       int * mv_off, volatile int *tdd, int *tkn, int blkh, int blkw, int * boy, int * box,
		       int dir0, int dir1)
{
    int is_bidir = dir0 && dir1;
    // Y
    if( dir0 ) {
	rv40_task_write(dMB, mv_off[0], tdd, tkn, blkh, blkw, boy[0], box[0], is_bidir, 0, 0, 0, 0);
	rv40_task_write(dMB, mv_off[1], tdd, tkn, blkh, blkw, boy[1], box[1], is_bidir, 0, !dir1, !dir1, 0);
    }

    if( dir1 ) {
	rv40_task_write(dMB, mv_off[0], tdd, tkn, blkh, blkw, boy[0], box[0], is_bidir, 1, 0, 0, 0);
	rv40_task_write(dMB, mv_off[1], tdd, tkn, blkh, blkw, boy[1], box[1], is_bidir, 1, 1, 1, 0);
    }
    // C
    if( dir0 ) {
	rv40_task_write(dMB, mv_off[0], tdd, tkn, blkh, blkw, boy[0], box[0], is_bidir, 0, 0, 0, 1);
	rv40_task_write(dMB, mv_off[1], tdd, tkn, blkh, blkw, boy[1], box[1], is_bidir, 0, !dir1, 0, 1);
    }

    if( dir1 ) {
	rv40_task_write(dMB, mv_off[0], tdd, tkn, blkh, blkw, boy[0], box[0], is_bidir, 1, 0, 0, 1);
	rv40_task_write(dMB, mv_off[1], tdd, tkn, blkh, blkw, boy[1], box[1], is_bidir, 1, 1, 0, 1);
    }

    return 0;
}

static int rv40_4mv_mc(RV9_MB_DecARGs *dMB,
		       int * mv_off, volatile int *tdd, int *tkn, int blkh, int blkw, int * boy, int * box,
		       int dir0, int dir1)
{
    int is_bidir = dir0 && dir1;
    // Y
    if( dir0 ) {
	rv40_task_write(dMB, mv_off[0], tdd, tkn, blkh, blkw, boy[0], box[0], is_bidir, 0, 0, 0, 0);
	rv40_task_write(dMB, mv_off[1], tdd, tkn, blkh, blkw, boy[1], box[1], is_bidir, 0, 0, 0, 0);
	rv40_task_write(dMB, mv_off[2], tdd, tkn, blkh, blkw, boy[2], box[2], is_bidir, 0, 0, 0, 0);
	rv40_task_write(dMB, mv_off[3], tdd, tkn, blkh, blkw, boy[3], box[3], is_bidir, 0, !dir1, !dir1, 0);
    }

    if( dir1 ) {
	rv40_task_write(dMB, mv_off[0], tdd, tkn, blkh, blkw, boy[0], box[0], is_bidir, 1, 0, 0, 0);
	rv40_task_write(dMB, mv_off[1], tdd, tkn, blkh, blkw, boy[1], box[1], is_bidir, 1, 0, 0, 0);
	rv40_task_write(dMB, mv_off[2], tdd, tkn, blkh, blkw, boy[2], box[2], is_bidir, 1, 0, 0, 0);
	rv40_task_write(dMB, mv_off[3], tdd, tkn, blkh, blkw, boy[3], box[3], is_bidir, 1, 1, 1, 0);
    }
    // C
    if( dir0 ) {
	rv40_task_write(dMB, mv_off[0], tdd, tkn, blkh, blkw, boy[0], box[0], is_bidir, 0, 0, 0, 1);
	rv40_task_write(dMB, mv_off[1], tdd, tkn, blkh, blkw, boy[1], box[1], is_bidir, 0, 0, 0, 1);
	rv40_task_write(dMB, mv_off[2], tdd, tkn, blkh, blkw, boy[2], box[2], is_bidir, 0, 0, 0, 1);
	rv40_task_write(dMB, mv_off[3], tdd, tkn, blkh, blkw, boy[3], box[3], is_bidir, 0, !dir1, 0, 1);
    }

    if( dir1 ) {
	rv40_task_write(dMB, mv_off[0], tdd, tkn, blkh, blkw, boy[0], box[0], is_bidir, 1, 0, 0, 1);
	rv40_task_write(dMB, mv_off[1], tdd, tkn, blkh, blkw, boy[1], box[1], is_bidir, 1, 0, 0, 1);
	rv40_task_write(dMB, mv_off[2], tdd, tkn, blkh, blkw, boy[2], box[2], is_bidir, 1, 0, 0, 1);
	rv40_task_write(dMB, mv_off[3], tdd, tkn, blkh, blkw, boy[3], box[3], is_bidir, 1, 1, 0, 1);
    }

    return 0;
}

static int rv40_decode_mv_aux(RV9_Slice_GlbARGs *dSlice,RV9_MB_DecARGs *dMB, uint8_t *motion_dha)
{
    int i;
    int block_type = dMB->mbtype;

    volatile int *tdd = (int *)motion_dha;
    int mv_off[4], boy[4], box[4];
    int tkn = 0;
    tdd++;	  	  

    switch(block_type){
    case RV34_MB_SKIP:
	if(dSlice->pict_type == FF_P_TYPE){
	    rv40_1mv_mc(dMB, 0, tdd, &tkn, 3, 3, 0, 0, 1, 0);
	    mc_flag = 1;
	    break;
	}
    case RV34_MB_B_DIRECT:
	//surprisingly, it uses motion scheme from next reference frame
	if(!(IS_16X8(dMB->next_bt) || IS_8X16(dMB->next_bt) || IS_8X8(dMB->next_bt))) {
	    rv40_1mv_mc(dMB, 0, tdd, &tkn, 3, 3, 0, 0, 1, 1);
	} else {
	    for(i = 0; i < 4; i++) {
		mv_off[i] = i;
		boy[i] = i & 0x2;
		box[i] = (i & 0x1) * 2;
	    }
	    rv40_4mv_mc(dMB, mv_off, tdd, &tkn, 2, 2, boy, box, 1, 1);
	}
	mc_flag = 1;
	break;
    case RV34_MB_P_16x16:
    case RV34_MB_P_MIX16x16:
	rv40_1mv_mc(dMB, 0, tdd, &tkn, 3, 3, 0, 0, 1, 0);
	mc_flag = 1;
	break;
    case RV34_MB_B_FORWARD:
    case RV34_MB_B_BACKWARD:
	rv40_1mv_mc(dMB, 0, tdd, &tkn, 3, 3, 0, 0, block_type == RV34_MB_B_FORWARD, block_type == RV34_MB_B_BACKWARD);
	mc_flag = 1;
	break;
    case RV34_MB_P_16x8:
    case RV34_MB_P_8x16:
	if(block_type == RV34_MB_P_16x8){
	    mv_off[0] = 0;
	    boy[0] = 0;
	    box[0] = 0;
	    mv_off[1] = 2;
	    boy[1] = 2;
	    box[1] = 0;

	    rv40_2mv_mc(dMB, mv_off, tdd, &tkn, 2, 3, boy, box, 1, 0);
	}
	if(block_type == RV34_MB_P_8x16){
	    mv_off[0] = 0;
	    boy[0] = 0;
	    box[0] = 0;
	    mv_off[1] = 1;
	    boy[1] = 0;
	    box[1] = 2;

	    rv40_2mv_mc(dMB, mv_off, tdd, &tkn, 3, 2, boy, box, 1, 0);
	}
	mc_flag = 1;
	break;
    case RV34_MB_B_BIDIR:
	rv40_1mv_mc(dMB, 0, tdd, &tkn, 3, 3, 0, 0, 1, 1);
	mc_flag = 1;
	break;
    case RV34_MB_P_8x8:
	for(i = 0; i < 4; i++) {
	    mv_off[i] = i;
	    boy[i] = i & 0x2;
	    box[i] = (i & 0x1) * 2;
	}
	rv40_4mv_mc(dMB, mv_off, tdd, &tkn, 2, 2, boy, box, 1, 0);
	mc_flag = 1;
	break;
    }

    tdd[-1] = TDD_HEAD(1,/*vld*/
		       1,/*lk*/
		       1,/*ch1pel*/
		       1,/*ch2pel*/ 
		       TDD_POS_SPEC,/*posmd*/
		       TDD_MV_SPEC,/*mvmd*/ 
		       tkn,/*tkn*/
		       dMB->mb_y,/*mby*/
		       dMB->mb_x/*mbx*/);

    tdd[2*tkn] = TDD_HEAD(1,/*vld*/
			  1,/*lk*/
			  0,/*ch1pel*/
			  0,/*ch2pel*/ 
			  TDD_POS_SPEC,/*posmd*/
			  TDD_MV_SPEC,/*mvmd*/
			  0,/*tkn*/
			  0,/*mby*/
			  0/*mbx*/);

    tdd[2*tkn+1] = TDD_SYNC(1,/*vld*/
			    0,/*lk*/
			    0, /*crst*/
			    0xFFFF/*id*/);

    return 0;
}

