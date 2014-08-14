#ifndef __place_k0_data__
#define __place_k0_data__
#endif

#undef printf

#include "t_motion.h"
//#include "t_intpid.h"
#include "t_vputlb.h"
#include "mpeg4_tcsm1.h"

#define MPEG_HPEL  0
#define MPEG_QPEL  1

enum SPelSFT {
  HPEL = 1,
  QPEL,
  EPEL,
};

#define IS_ILUT1  2
#define IS_ILUT2  3

#define MV_16X16 0
#define MV_8X8 1

#define JZC_BUG_QPEL_CHROMA 64
#define JZC_BUG_QPEL_CHROMA2 256

static char SubPel[] = {HPEL, QPEL, QPEL, EPEL,
			QPEL, QPEL, QPEL, QPEL,
			QPEL, QPEL, QPEL, QPEL};

volatile uint8_t *motion_buf, *motion_dha, *motion_dsa, *motion_douty, *motion_doutc, *motion_iwta;

static volatile int *dbg_ptr = TCSM1_DBG_BUF;

static volatile uint32_t *b_use = RECON_BUF_USE;

static void mpeg4_hpel(int mb_x, int mb_y, int dir, int bwt, int mspel, int no_rnd,
		       int *pos, int *uvpos, int *ymvx, int *ymvy, int *cmvx, int *cmvy);

static inline int JZC_round_chroma(int x){
  static const uint8_t JZC_chroma_roundtab[16] = {
    //  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
    0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1,
  };
  return JZC_chroma_roundtab[x & 0xf] + (x >> 3);
}

static inline void ms_check_mc_result(){
  while(*(volatile int *)motion_dsa != (0x80000000 | 0xFFFF) ){
  };

  uint8_t *c;
  uint8_t *c2;
  uint8_t t;
  uint8_t i;

  c = (*b_use)?RECON_UBUF1-16:RECON_UBUF0-16;
  c2 = RECON_UBUF2-8;

  S32LDI(xr1, c2,0x8);
  S32LDD(xr2, c2,0x4);
  S32LDD(xr3, c2,64);
  S32LDD(xr4, c2,68);
  S32LDI(xr5, c2,0x8);
  S32LDD(xr6, c2,0x4);
  S32LDD(xr7, c2,64);
  S32LDD(xr8, c2,68);

  i_pref(30, c, 0);
  S32SDI(xr1, c, 0x10);
  S32STD(xr2, c, 0x4);
  S32STD(xr3, c, 0x8);
  S32STD(xr4, c, 0xc);
  S32SDI(xr5, c, 0x10);
  S32STD(xr6, c, 0x4);
  S32STD(xr7, c, 0x8);
  S32STD(xr8, c, 0xc);

  S32LDI(xr1, c2,0x8);
  S32LDD(xr2, c2,0x4);
  S32LDD(xr3, c2,64);
  S32LDD(xr4, c2,68);
  S32LDI(xr5, c2,0x8);
  S32LDD(xr6, c2,0x4);
  S32LDD(xr7, c2,64);
  S32LDD(xr8, c2,68);

  i_pref(30, c, 0);
  S32SDI(xr1, c, 0x10);
  S32STD(xr2, c, 0x4);
  S32STD(xr3, c, 0x8);
  S32STD(xr4, c, 0xc);
  S32SDI(xr5, c, 0x10);
  S32STD(xr6, c, 0x4);
  S32STD(xr7, c, 0x8);
  S32STD(xr8, c, 0xc);

  S32LDI(xr1, c2,0x8);
  S32LDD(xr2, c2,0x4);
  S32LDD(xr3, c2,64);
  S32LDD(xr4, c2,68);
  S32LDI(xr5, c2,0x8);
  S32LDD(xr6, c2,0x4);
  S32LDD(xr7, c2,64);
  S32LDD(xr8, c2,68);

  i_pref(30, c, 0);
  S32SDI(xr1, c, 0x10);
  S32STD(xr2, c, 0x4);
  S32STD(xr3, c, 0x8);
  S32STD(xr4, c, 0xc);
  S32SDI(xr5, c, 0x10);
  S32STD(xr6, c, 0x4);
  S32STD(xr7, c, 0x8);
  S32STD(xr8, c, 0xc);

  S32LDI(xr1, c2,0x8);
  S32LDD(xr2, c2,0x4);
  S32LDD(xr3, c2,64);
  S32LDD(xr4, c2,68);
  S32LDI(xr5, c2,0x8);
  S32LDD(xr6, c2,0x4);
  S32LDD(xr7, c2,64);
  S32LDD(xr8, c2,68);

  i_pref(30, c, 0);
  S32SDI(xr1, c, 0x10);
  S32STD(xr2, c, 0x4);
  S32STD(xr3, c, 0x8);
  S32STD(xr4, c, 0xc);
  S32SDI(xr5, c, 0x10);
  S32STD(xr6, c, 0x4);
  S32STD(xr7, c, 0x8);
  S32STD(xr8, c, 0xc);

  return;
}

static inline void check_mc_result(){
  while(*(volatile int *)motion_dsa != (0x80000000 | 0xFFFF) ){
  };
  return;
}

static inline void MPV_motion_p1(uint8_t mv_type, uint8_t quarter_sample, uint8_t dir, uint8_t no_rnd,
				 uint16_t mb_x, uint16_t mb_y, int32_t *mv, int32_t width, int32_t height, int bugs){
  //int dxy, mx, my, src_x, src_y, motion_x, motion_y;
  int motion_4x[8], motion_4y[8];
  int cmvx[2],cmvy[2];
  int i;
  //uint8_t *ptr, *dest;
  int dxy_tlb[4] = {0,2,8,10};
  int ydxy[8];
  int cdxy[2];
  int bwt = 0;
  if (dir & 1)
    bwt = 1;

  motion_buf = (volatile uint8_t *)TCSM1_MOTION_DHA;
  motion_dha = motion_buf;
  motion_dsa = TCSM1_MOTION_DSA;

  if (*b_use){
    motion_douty = RECON_YBUF1;
  }else{
    motion_douty = RECON_YBUF0;
  }
  motion_doutc = RECON_UBUF2;

  SET_REG1_DSTA(TCSM1_PADDR((int)motion_douty));
  SET_REG2_DSTA(TCSM1_PADDR((int)motion_doutc));
  SET_REG1_DSA(TCSM1_PADDR((int)motion_dsa));
  SET_REG2_DSA(TCSM1_PADDR((int)motion_dsa));

  switch(mv_type){
  case MV_16X16:
    if (quarter_sample){
      if (dir & 1){//FORWARD
	motion_4x[0] = mv[0];
	motion_4y[0] = mv[1];

	ydxy[0] = ((motion_4y[0] & 3) << 2) | (motion_4x[0] & 3);

	if(bugs&JZC_BUG_QPEL_CHROMA2){
	  static const int rtab[8]= {0,0,1,1,0,0,0,1};
	  cmvx[0] = (motion_4x[0]>>1) + rtab[motion_4x[0]&7];
	  cmvy[0] = (motion_4y[0]>>1) + rtab[motion_4y[0]&7];
	}else if(bugs&JZC_BUG_QPEL_CHROMA){
	  cmvx[0] = (motion_4x[0]>>1)|(motion_4x[0]&1);
	  cmvy[0] = (motion_4y[0]>>1)|(motion_4y[0]&1);
	}else{
	  cmvx[0] = motion_4x[0]/2;
	  cmvy[0] = motion_4y[0]/2;
	}

	cmvx[0] = (cmvx[0]>>1)|(cmvx[0]&1);
	cmvy[0] = (cmvy[0]>>1)|(cmvy[0]&1);
      
	cdxy[0] = (cmvx[0]&1) | ((cmvy[0]&1)<<1);
	cdxy[0] = dxy_tlb[cdxy[0]];
      }

      if (dir & 2){//BACKWARD
	motion_4x[1] = mv[8];
	motion_4y[1] = mv[9];

	ydxy[1] = ((motion_4y[1] & 3) << 2) | (motion_4x[1] & 3);

	if(bugs&JZC_BUG_QPEL_CHROMA2){
	  static const int rtab[8]= {0,0,1,1,0,0,0,1};
	  cmvx[1] = (motion_4x[1]>>1) + rtab[motion_4x[1]&7];
	  cmvy[1] = (motion_4y[1]>>1) + rtab[motion_4y[1]&7];
	}else if(bugs&JZC_BUG_QPEL_CHROMA){
	  cmvx[1] = (motion_4x[1]>>1)|(motion_4x[1]&1);
	  cmvy[1] = (motion_4y[1]>>1)|(motion_4y[1]&1);
	}else{
	  cmvx[1] = motion_4x[1]/2;
	  cmvy[1] = motion_4y[1]/2;
	}

	cmvx[1] = (cmvx[1]>>1)|(cmvx[1]&1);
	cmvy[1] = (cmvy[1]>>1)|(cmvy[1]&1);
      
	cdxy[1] = (cmvx[1]&1) | ((cmvy[1]&1)<<1);
	cdxy[1] = dxy_tlb[cdxy[1]];
      }
      mpeg4_qpel(mb_x, mb_y, (dir&3), bwt, 0, no_rnd, ydxy, cdxy, motion_4x, motion_4y, cmvx, cmvy);
    }else{
      if (dir & 1){//FORWARD
	motion_4x[0] = mv[0];
	motion_4y[0] = mv[1];
	ydxy[0] = ((motion_4y[0] & 1) << 1) | (motion_4x[0] & 1);
	cdxy[0] = ydxy[0] | (motion_4y[0] & 2) | ((motion_4x[0] & 2) >> 1);
	cmvx[0] = motion_4x[0];cmvy[0] = motion_4y[0];
#if 1
	if (cmvx[0] > 1 || cmvx[0] < -1)
	  cmvx[0] = cmvx[0] >> 1;
	if (cmvy[0] > 1 || cmvy[0] < -1)
	  cmvy[0] = cmvy[0] >> 1;
#endif
	ydxy[0] = dxy_tlb[ydxy[0]];
	cdxy[0] = dxy_tlb[cdxy[0]];

	//mpeg4_hpel(mb_x, mb_y, 0, bwt, 0, no_rnd, ydxy, cdxy, motion_4x[0], motion_4y[0], cmvx[0], cmvy[0]);
      }
#if 1
      if (dir & 2){//BACKWARD
	motion_4x[1] = mv[8];
	motion_4y[1] = mv[9];
	ydxy[1] = ((motion_4y[1] & 1) << 1) | (motion_4x[1] & 1);
	cdxy[1] = ydxy[1] | (motion_4y[1] & 2) | ((motion_4x[1] & 2) >> 1);
	cmvx[1] = motion_4x[1];cmvy[1] = motion_4y[1];
	if (cmvx[1] > 1 || cmvx[1] < -1)
	  cmvx[1] = cmvx[1] >> 1;
	if (cmvy[1] > 1 || cmvy[1] < -1)
	  cmvy[1] = cmvy[1] >> 1;
	ydxy[1] = dxy_tlb[ydxy[1]];
	cdxy[1] = dxy_tlb[cdxy[1]];
      }
#endif
      mpeg4_hpel(mb_x, mb_y, (dir&3), bwt, 0, no_rnd, ydxy, cdxy, motion_4x, motion_4y, cmvx, cmvy);
    }
    break;
#if 1
  case MV_8X8:
    cmvx[0] = 0;cmvx[1] = 0;cmvy[0] = 0;cmvy[1] = 0;
      
    if (quarter_sample){
      if (dir & 1){//FORWARD
	for(i=0;i<4;i++){
	  motion_4x[i] = mv[i*2];
	  motion_4y[i] = mv[i*2+1];

	  ydxy[i] = ((motion_4y[i] & 3) << 2) | (motion_4x[i] & 3);

	  /* WARNING: do no forget half pels */
	  if ((mb_x * 16 + (i & 1) * 8 + (motion_4x[i]>>2)) < -16)
	    motion_4x[i] = -16;
	  else if ((mb_x * 16 + (i & 1) * 8 + (motion_4x[i]>>2)) >= width){
	    motion_4x[i] = (width - (mb_x * 16 + (i & 1) * 8)) * 4;
	    ydxy[i] &= ~3;
	  }

	  if ((mb_y * 16 + (i >>1) * 8 + (motion_4y[i]>>2)) < -16)
	    motion_4y[i] = -16;
	  else if ((mb_y * 16 + (i >>1) * 8 + (motion_4y[i]>>2)) >= height){
	    motion_4y[i] = (height - (mb_y * 16 + (i >>1) * 8)) * 4;
	    ydxy[i] &= ~12;
	  }

	  cmvx[0] += mv[i*2]/2;
	  cmvy[0] += mv[i*2+1]/2;
	}
	cmvx[0] = JZC_round_chroma(cmvx[0]);
	cmvy[0] = JZC_round_chroma(cmvy[0]);

	cdxy[0] = ((cmvy[0] & 1) << 1) | (cmvx[0] & 1); 
	cdxy[0] = dxy_tlb[cdxy[0]];
      }

      if (dir & 2){//BACKWARD
	for(i=0;i<4;i++){
	  motion_4x[i+4] = mv[i*2+8];
	  motion_4y[i+4] = mv[i*2+1+8];

	  ydxy[i+4] = ((motion_4y[i+4] & 3) << 2) | (motion_4x[i+4] & 3);

	  /* WARNING: do no forget half pels */
	  if ((mb_x * 16 + (i & 1) * 8 + (motion_4x[i+4]>>2)) < -16)
	    motion_4x[i+4] = -16;
	  else if ((mb_x * 16 + (i & 1) * 8 + (motion_4x[i+4]>>2)) >= width){
	    motion_4x[i+4] = (width - (mb_x * 16 + (i & 1) * 8)) * 4;
	    ydxy[i+4] &= ~3;
	  }

	  if ((mb_y * 16 + (i >>1) * 8 + (motion_4y[i+4]>>2)) < -16)
	    motion_4y[i+4] = -16;
	  else if ((mb_y * 16 + (i >>1) * 8 + (motion_4y[i+4]>>2)) >= height){
	    motion_4y[i+4] = (height - (mb_y * 16 + (i >>1) * 8)) * 4;
	    ydxy[i+4] &= ~12;
	  }

	  cmvx[1] += mv[i*2+8]/2;
	  cmvy[1] += mv[i*2+1+8]/2;
	}
	cmvx[1] = JZC_round_chroma(cmvx[1]);
	cmvy[1] = JZC_round_chroma(cmvy[1]);

	cdxy[1] = ((cmvy[1] & 1) << 1) | (cmvx[1] & 1); 
	cdxy[1] = dxy_tlb[cdxy[1]];
      }

      mpeg4_qpel_4mv(mb_x, mb_y, dir&3, bwt, 0, no_rnd, ydxy, cdxy, motion_4x, motion_4y, cmvx, cmvy);      
    }else{
      if (dir & 1){//FORWARD
	for (i = 0; i < 4; i++){
	  motion_4x[i] = mv[i*2];
	  motion_4y[i] = mv[i*2+1];
	    
	  ydxy[i] = ((motion_4y[i] & 1) << 1) | (motion_4x[i] & 1);

	  if ((mb_x * 16 + (i & 1) * 8 + (motion_4x[i]>>1)) < -16)
	    motion_4x[i] = -16;
	  else if ((mb_x * 16 + (i & 1) * 8 + (motion_4x[i]>>1)) >= width){
	    motion_4x[i] = (width - (mb_x * 16 + (i & 1) * 8)) * 2;
	    ydxy[i] &= ~1;
	  }

	  if ((mb_y * 16 + (i >>1) * 8 + (motion_4y[i]>>1)) < -16)
	    motion_4y[i] = -16;
	  else if ((mb_y * 16 + (i >>1) * 8 + (motion_4y[i]>>1)) >= height){
	    motion_4y[i] = (height - (mb_y * 16 + (i >>1) * 8)) * 2;
	    ydxy[i] &= ~2;
	  }

	  ydxy[i] = dxy_tlb[ydxy[i]];

	  cmvx[0] += mv[i*2];
	  cmvy[0] += mv[i*2+1];
	}
	cmvx[0] = JZC_round_chroma(cmvx[0]);
	cmvy[0] = JZC_round_chroma(cmvy[0]);

	cdxy[0] = ((cmvy[0] & 1) << 1) | (cmvx[0] & 1); 
	cdxy[0] = dxy_tlb[cdxy[0]];

	//mpeg4_hpel_4mv(mb_x, mb_y, 0, bwt, 0, no_rnd, ydxy, cdxy, motion_4x, motion_4y, cmvx[0], cmvy[0]);
      }
      if (dir & 2){//BACKWARD
	for (i = 0; i < 4; i++){
	  motion_4x[i+4] = mv[i*2+8];
	  motion_4y[i+4] = mv[i*2+1+8];
	  ydxy[i+4] = ((motion_4y[i+4] & 1) << 1) | (motion_4x[i+4] & 1);

	  if ((mb_x * 16 + (i & 1) * 8 + (motion_4x[i+4]>>1)) < -16)
	    motion_4x[i+4] = -16;
	  else if ((mb_x * 16 + (i & 1) * 8 + (motion_4x[i+4]>>1)) >= width){
	    motion_4x[i+4] = (width - (mb_x * 16 + (i & 1) * 8)) * 2;
	    ydxy[i+4] &= ~1;
	  }

	  if ((mb_y * 16 + (i >>1) * 8 + (motion_4y[i+4]>>1)) < -16)
	    motion_4y[i+4] = -16;
	  else if ((mb_y * 16 + (i >>1) * 8 + (motion_4y[i+4]>>1)) >= height){
	    motion_4y[i+4] = (height - (mb_y * 16 + (i >>1) * 8)) * 2;
	    ydxy[i+4] &= ~2;
	  }

	  ydxy[i+4] = dxy_tlb[ydxy[i+4]];

	  cmvx[1] += mv[i*2+8];
	  cmvy[1] += mv[i*2+1+8];
	}

	cmvx[1] = JZC_round_chroma(cmvx[1]);
	cmvy[1] = JZC_round_chroma(cmvy[1]);

	cdxy[1] = ((cmvy[1] & 1) << 1) | (cmvx[1] & 1); 
	cdxy[1] = dxy_tlb[cdxy[1]];
	//mpeg4_hpel_4mv(mb_x, mb_y, 1, bwt, 0, no_rnd, &ydxy[4], &cdxy[1], &motion_4x[4], &motion_4y[4], cmvx[1], cmvy[1]);
      }
      mpeg4_hpel_4mv(mb_x, mb_y, dir&3, bwt, 0, no_rnd, ydxy, cdxy, motion_4x, motion_4y, cmvx, cmvy);
    }
    break;
#endif
  }

  return;
}

static void mpeg4_hpel(int mb_x, int mb_y, int dir, int bwt, int mspel, int no_rnd,
		       int *pos, int *uvpos, int *ymvx, int *ymvy, int *cmvx, int *cmvy){
  volatile int *tdd = (int *)motion_dha;
  int tkn = 0;
  motion_dsa[0] = 0x0;

  SET_REG1_BINFO(0,0,0,0,no_rnd? (IS_ILUT1): (IS_ILUT2),0,0,0,0,0,0,0,0);

  tdd[tkn++] = TDD_HEAD(1,/*vld*/
			1,/*lk*/
			0,/*op*/
			0,/*ch1pel*/
			0,/*ch2pel*/
			1/*TDD_POS_SPEC*/,/*posmd*/
			1/*TDD_MV_SPEC*/,/*mvmd*/
			(dir == 3)?4:2,/*tkn*/
			mb_y,/*mby*/
			mb_x/*mbx*/);

  if (dir & 1){
    tdd[tkn++] = TDD_MV(ymvy[0], ymvx[0]);
    //dbg_ptr[0] = tdd[tkn-1];
    tdd[tkn++] = TDD_CMD(bwt,/*bidir*/
			 0,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 (dir & 2)?0:1,/*doe*/
			 (dir & 2)?0:1,/*cflo*/
			 0,/*ypos*/
			 no_rnd?(IS_ILUT1):(IS_ILUT2),/*lilmd*/
			 0,/*cilmd*/
			 0,/*list*/
			 0,/*boy*/
			 0,/*box*/
			 BLK_H16,/*bh*/
			 BLK_W16,/*bw*/
			 pos[0]/*xpos*/);
    //dbg_ptr[1] = tdd[tkn-1];
  }

  if (dir & 2){
    tdd[tkn++] = TDD_MV(ymvy[1], ymvx[1]);

    tdd[tkn++] = TDD_CMD(bwt,/*bidir*/
			 1,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 1,/*doe*/
			 1,/*cflo*/
			 0,/*ypos*/
			 no_rnd?(IS_ILUT1):(IS_ILUT2),/*lilmd*/
			 0,/*cilmd*/
			 0,/*list*/
			 0,/*boy*/
			 0,/*box*/
			 BLK_H16,/*bh*/
			 BLK_W16,/*bw*/
			 pos[1]/*xpos*/);
  }

  if (dir & 1){
    tdd[tkn++] = TDD_MV(cmvy[0], cmvx[0]);

    tdd[tkn++] = TDD_CMD(bwt,/*bidir*/
			 0,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 (dir & 2)?0:1,/*doe*/
			 0,/*cflo*/
			 0,/*ypos*/
			 0,/*lilmd*/
			 no_rnd?(IS_ILUT1):(IS_ILUT2),/*cilmd*/
			 0,/*list*/
			 0,/*boy*/
			 0,/*box*/
			 BLK_H16,/*bh*/
			 BLK_W16,/*bw*/
			 uvpos[0]/*xpos*/);
  }

  if (dir & 2){
    tdd[tkn++] = TDD_MV(cmvy[1], cmvx[1]);

    tdd[tkn++] = TDD_CMD(bwt,/*bidir*/
			 1,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 1,/*doe*/
			 0,/*cflo*/
			 0,/*ypos*/
			 0,/*lilmd*/
			 no_rnd?(IS_ILUT1):(IS_ILUT2),/*cilmd*/
			 0,/*list*/
			 0,/*boy*/
			 0,/*box*/
			 BLK_H16,/*bh*/
			 BLK_W16,/*bw*/
			 uvpos[1]/*xpos*/);
  }
#if 1
  tdd[tkn++] = TDD_HEAD(1,/*vld*/
			1,/*lk*/
			0,/*op*/
			0,/*ch1pel*/
			0,/*ch2pel*/
			1/*TDD_POS_SPEC*/,/*posmd*/
			1/*TDD_MV_SPEC*/,/*mvmd*/
			0,/*tkn*/
			0xFF,/*mby*/
			0xFF/*mbx*/);
#endif
  tdd[tkn++] = SYN_HEAD(1, 0, 2, 0, 0xFFFF);

  //tdd[tkn++] = SYN_HEAD(1, 0, 2, 0, 0xFFFF);

  SET_REG1_DDC(TCSM1_PADDR((int)motion_dha) | 0x1);

  return;
}

void mpeg4_hpel_4mv(int mb_x, int mb_y, int dir, int bwt, int mspel, int no_rnd,
		    int *pos, int *cpos, int *ymvx, int *ymvy, int *cmvx, int *cmvy){
  volatile int *tdd = (int *)motion_dha;
  int tkn = 0;
  motion_dsa[0] = 0x0;

  SET_REG1_BINFO(0,0,0,0,no_rnd? (IS_ILUT1): (IS_ILUT2),0,0,0,0,0,0,0,0);

  tdd[tkn++] = TDD_HEAD(1,/*vld*/
			1,/*lk*/
			0,/*op*/
			0,/*ch1pel*/
			0,/*ch2pel*/
			1/*TDD_POS_SPEC*/,/*posmd*/
			1/*TDD_MV_SPEC*/,/*mvmd*/
			(dir == 3)?10:5,/*tkn*/
			mb_y,/*mby*/
			mb_x/*mbx*/);

  if (dir & 1){
    tdd[tkn++] = TDD_MV(ymvy[0], ymvx[0]);

    tdd[tkn++] = TDD_CMD(bwt,/*bidir*/
			 0,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 0,/*doe*/
			 0,/*cflo*/
			 0,/*ypos*/
			 no_rnd?(IS_ILUT1):(IS_ILUT2),/*lilmd*/
			 0,/*cilmd*/
			 0,/*list*/
			 0,/*boy*/
			 0,/*box*/
			 BLK_H8,/*bh*/
			 BLK_W8,/*bw*/
			 pos[0]/*xpos*/);

    tdd[tkn++] = TDD_MV(ymvy[1], ymvx[1]);

    tdd[tkn++] = TDD_CMD(bwt,/*bidir*/
			 0,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 0,/*doe*/
			 0,/*cflo*/
			 0,/*ypos*/
			 no_rnd?(IS_ILUT1):(IS_ILUT2),/*lilmd*/
			 0,/*cilmd*/
			 0,/*list*/
			 0,/*boy*/
			 2,/*box*/
			 BLK_H8,/*bh*/
			 BLK_W8,/*bw*/
			 pos[1]/*xpos*/);

    tdd[tkn++] = TDD_MV(ymvy[2], ymvx[2]);

    tdd[tkn++] = TDD_CMD(bwt,/*bidir*/
			 0,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 0,/*doe*/
			 0,/*cflo*/
			 0,/*ypos*/
			 no_rnd?(IS_ILUT1):(IS_ILUT2),/*lilmd*/
			 0,/*cilmd*/
			 0,/*list*/
			 2,/*boy*/
			 0,/*box*/
			 BLK_H8,/*bh*/
			 BLK_W8,/*bw*/
			 pos[2]/*xpos*/);

    tdd[tkn++] = TDD_MV(ymvy[3], ymvx[3]);

    tdd[tkn++] = TDD_CMD(bwt,/*bidir*/
			 0,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 (dir&2)?0:1,/*doe*/
			 (dir&2)?0:1,/*cflo*/
			 0,/*ypos*/
			 no_rnd?(IS_ILUT1):(IS_ILUT2),/*lilmd*/
			 0,/*cilmd*/
			 0,/*list*/
			 2,/*boy*/
			 2,/*box*/
			 BLK_H8,/*bh*/
			 BLK_W8,/*bw*/
			 pos[3]/*xpos*/);
  }

  if (dir & 2){
    tdd[tkn++] = TDD_MV(ymvy[4], ymvx[4]);

    tdd[tkn++] = TDD_CMD(bwt,/*bidir*/
			 1,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 0,/*doe*/
			 0,/*cflo*/
			 0,/*ypos*/
			 no_rnd?(IS_ILUT1):(IS_ILUT2),/*lilmd*/
			 0,/*cilmd*/
			 0,/*list*/
			 0,/*boy*/
			 0,/*box*/
			 BLK_H8,/*bh*/
			 BLK_W8,/*bw*/
			 pos[4]/*xpos*/);

    tdd[tkn++] = TDD_MV(ymvy[5], ymvx[5]);

    tdd[tkn++] = TDD_CMD(bwt,/*bidir*/
			 1,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 0,/*doe*/
			 0,/*cflo*/
			 0,/*ypos*/
			 no_rnd?(IS_ILUT1):(IS_ILUT2),/*lilmd*/
			 0,/*cilmd*/
			 0,/*list*/
			 0,/*boy*/
			 2,/*box*/
			 BLK_H8,/*bh*/
			 BLK_W8,/*bw*/
			 pos[5]/*xpos*/);

    tdd[tkn++] = TDD_MV(ymvy[6], ymvx[6]);

    tdd[tkn++] = TDD_CMD(bwt,/*bidir*/
			 1,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 0,/*doe*/
			 0,/*cflo*/
			 0,/*ypos*/
			 no_rnd?(IS_ILUT1):(IS_ILUT2),/*lilmd*/
			 0,/*cilmd*/
			 0,/*list*/
			 2,/*boy*/
			 0,/*box*/
			 BLK_H8,/*bh*/
			 BLK_W8,/*bw*/
			 pos[6]/*xpos*/);

    tdd[tkn++] = TDD_MV(ymvy[7], ymvx[7]);

    tdd[tkn++] = TDD_CMD(bwt,/*bidir*/
			 1,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 1,/*doe*/
			 1,/*cflo*/
			 0,/*ypos*/
			 no_rnd?(IS_ILUT1):(IS_ILUT2),/*lilmd*/
			 0,/*cilmd*/
			 0,/*list*/
			 2,/*boy*/
			 2,/*box*/
			 BLK_H8,/*bh*/
			 BLK_W8,/*bw*/
			 pos[7]/*xpos*/);
  }

  if (dir & 1){
    tdd[tkn++] = TDD_MV(cmvy[0], cmvx[0]);

    tdd[tkn++] = TDD_CMD(bwt,/*bidir*/
			 0,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 (dir&2)?0:1,/*doe*/
			 0,/*cflo*/
			 0,/*ypos*/
			 0,/*lilmd*/
			 no_rnd?(IS_ILUT1):(IS_ILUT2),/*cilmd*/
			 0,/*list*/
			 0,/*boy*/
			 0,/*box*/
			 BLK_H16,/*bh*/
			 BLK_W16,/*bw*/
			 cpos[0]/*xpos*/);
  }

  if (dir & 2){
    tdd[tkn++] = TDD_MV(cmvy[1], cmvx[1]);

    tdd[tkn++] = TDD_CMD(bwt,/*bidir*/
			 1,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 1,/*doe*/
			 0,/*cflo*/
			 0,/*ypos*/
			 0,/*lilmd*/
			 no_rnd?(IS_ILUT1):(IS_ILUT2),/*cilmd*/
			 0,/*list*/
			 0,/*boy*/
			 0,/*box*/
			 BLK_H16,/*bh*/
			 BLK_W16,/*bw*/
			 cpos[1]/*xpos*/);
  }

  tdd[tkn++] = TDD_HEAD(1,/*vld*/
			1,/*lk*/
			0,/*op*/
			0,/*ch1pel*/
			0,/*ch2pel*/
			TDD_POS_SPEC,/*posmd*/
			TDD_MV_SPEC,/*mvmd*/
			0,/*tkn*/
			0xFF,/*mby*/
			0xFF/*mbx*/);

  tdd[tkn++] = SYN_HEAD(1, 0, 2, 0, 0xFFFF);

  
  SET_REG1_DDC(TCSM1_PADDR((int)motion_dha) | 0x1);

  return;
}

void mpeg4_qpel(int mb_x, int mb_y, int dir, int bwt, int mspel, int no_rnd,
		    int *pos, int *cpos, int *ymvx, int *ymvy, int *cmvx, int *cmvy){
  volatile int *tdd = (int *)motion_dha;

  int tkn = 0;
  motion_dsa[0] = 0x0;

  tdd[tkn++] = TDD_HEAD(1,/*vld*/
			1,/*lk*/
			0,/*op*/
			1,/*ch1pel*/
			0,/*ch2pel*/
			1/*TDD_POS_SPEC*/,/*posmd*/
			1/*TDD_MV_SPEC*/,/*mvmd*/
			(dir == 3)?4:2,/*tkn*/
			mb_y,/*mby*/
			mb_x/*mbx*/);

  if (dir & 1){
    tdd[tkn++] = TDD_MV(ymvy[0], ymvx[0]);

    tdd[tkn++] = TDD_CMD(bwt,/*bidir*/
			 0,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 (dir & 2)?0:1,/*doe*/
			 (dir & 2)?0:1,/*cflo*/
			 0,/*ypos*/
			 0,/*lilmd*/
			 0,/*cilmd*/
			 0,/*list*/
			 0,/*boy*/
			 0,/*box*/
			 BLK_H16,/*bh*/
			 BLK_W16,/*bw*/
			 pos[0]/*xpos*/);
  }

  if (dir & 2){
    tdd[tkn++] = TDD_MV(ymvy[1], ymvx[1]);

    tdd[tkn++] = TDD_CMD(bwt,/*bidir*/
			 1,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 1,/*doe*/
			 1,/*cflo*/
			 0,/*ypos*/
			 0,/*lilmd*/
			 0,/*cilmd*/
			 0,/*list*/
			 0,/*boy*/
			 0,/*box*/
			 BLK_H16,/*bh*/
			 BLK_W16,/*bw*/
			 pos[1]/*xpos*/);
  }

  if (dir & 1){
    tdd[tkn++] = TDD_MV(cmvy[0], cmvx[0]);

    tdd[tkn++] = TDD_CMD(bwt,/*bidir*/
			 0,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 (dir & 2)?0:1,/*doe*/
			 0,/*cflo*/
			 0,/*ypos*/
			 0,/*lilmd*/
			 no_rnd?(IS_ILUT1):(IS_ILUT2),/*cilmd*/
			 0,/*list*/
			 0,/*boy*/
			 0,/*box*/
			 BLK_H16,/*bh*/
			 BLK_W16,/*bw*/
			 cpos[0]/*xpos*/);
  }

  if (dir & 2){
    tdd[tkn++] = TDD_MV(cmvy[1], cmvx[1]);

    tdd[tkn++] = TDD_CMD(bwt,/*bidir*/
			 1,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 1,/*doe*/
			 0,/*cflo*/
			 0,/*ypos*/
			 0,/*lilmd*/
			 no_rnd?(IS_ILUT1):(IS_ILUT2),/*cilmd*/
			 0,/*list*/
			 0,/*boy*/
			 0,/*box*/
			 BLK_H16,/*bh*/
			 BLK_W16,/*bw*/
			 cpos[1]/*xpos*/);
  }

  tdd[tkn++] = TDD_HEAD(1,/*vld*/
			1,/*lk*/
			0,/*op*/
			0,/*ch1pel*/
			0,/*ch2pel*/
			TDD_POS_SPEC,/*posmd*/
			TDD_MV_SPEC,/*mvmd*/
			0,/*tkn*/
			0xFF,/*mby*/
			0xFF/*mbx*/);

  tdd[tkn++] = SYN_HEAD(1, 0, 2, 0, 0xFFFF);

  SET_REG1_DDC(TCSM1_PADDR((int)motion_dha) | 0x1);

  return;  
}

void mpeg4_qpel_4mv(int mb_x, int mb_y, int dir, int bwt, int mspel, int no_rnd,
		    int *pos, int *cpos, int *ymvx, int *ymvy, int *cmvx, int *cmvy){
  volatile int *tdd = (int *)motion_dha;
  int tkn = 0;
  motion_dsa[0] = 0x0;

  tdd[tkn++] = TDD_HEAD(1,/*vld*/
			1,/*lk*/
			0,/*op*/
			1,/*ch1pel*/
			0,/*ch2pel*/
			1/*TDD_POS_SPEC*/,/*posmd*/
			1/*TDD_MV_SPEC*/,/*mvmd*/
			(dir == 3)?10:5,/*tkn*/
			mb_y,/*mby*/
			mb_x/*mbx*/);

  if (dir & 1){
    tdd[tkn++] = TDD_MV(ymvy[0], ymvx[0]);

    tdd[tkn++] = TDD_CMD(bwt,/*bidir*/
			 0,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 0,/*doe*/
			 0,/*cflo*/
			 0,/*ypos*/
			 0,/*lilmd*/
			 0,/*cilmd*/
			 0,/*list*/
			 0,/*boy*/
			 0,/*box*/
			 BLK_H8,/*bh*/
			 BLK_W8,/*bw*/
			 pos[0]/*xpos*/);

    tdd[tkn++] = TDD_MV(ymvy[1], ymvx[1]);

    tdd[tkn++] = TDD_CMD(bwt,/*bidir*/
			 0,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 0,/*doe*/
			 0,/*cflo*/
			 0,/*ypos*/
			 0,/*lilmd*/
			 0,/*cilmd*/
			 0,/*list*/
			 0,/*boy*/
			 2,/*box*/
			 BLK_H8,/*bh*/
			 BLK_W8,/*bw*/
			 pos[1]/*xpos*/);

    tdd[tkn++] = TDD_MV(ymvy[2], ymvx[2]);

    tdd[tkn++] = TDD_CMD(bwt,/*bidir*/
			 0,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 0,/*doe*/
			 0,/*cflo*/
			 0,/*ypos*/
			 0,/*lilmd*/
			 0,/*cilmd*/
			 0,/*list*/
			 2,/*boy*/
			 0,/*box*/
			 BLK_H8,/*bh*/
			 BLK_W8,/*bw*/
			 pos[2]/*xpos*/);

    tdd[tkn++] = TDD_MV(ymvy[3], ymvx[3]);

    tdd[tkn++] = TDD_CMD(bwt,/*bidir*/
			 0,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 (dir&2)?0:1,/*doe*/
			 (dir&2)?0:1,/*cflo*/
			 0,/*ypos*/
			 0,/*lilmd*/
			 0,/*cilmd*/
			 0,/*list*/
			 2,/*boy*/
			 2,/*box*/
			 BLK_H8,/*bh*/
			 BLK_W8,/*bw*/
			 pos[3]/*xpos*/);
  }

  if (dir & 2){
    tdd[tkn++] = TDD_MV(ymvy[4], ymvx[4]);

    tdd[tkn++] = TDD_CMD(bwt,/*bidir*/
			 1,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 0,/*doe*/
			 0,/*cflo*/
			 0,/*ypos*/
			 0,/*lilmd*/
			 0,/*cilmd*/
			 0,/*list*/
			 0,/*boy*/
			 0,/*box*/
			 BLK_H8,/*bh*/
			 BLK_W8,/*bw*/
			 pos[4]/*xpos*/);

    tdd[tkn++] = TDD_MV(ymvy[5], ymvx[5]);

    tdd[tkn++] = TDD_CMD(bwt,/*bidir*/
			 1,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 0,/*doe*/
			 0,/*cflo*/
			 0,/*ypos*/
			 0,/*lilmd*/
			 0,/*cilmd*/
			 0,/*list*/
			 0,/*boy*/
			 2,/*box*/
			 BLK_H8,/*bh*/
			 BLK_W8,/*bw*/
			 pos[5]/*xpos*/);

    tdd[tkn++] = TDD_MV(ymvy[6], ymvx[6]);

    tdd[tkn++] = TDD_CMD(bwt,/*bidir*/
			 1,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 0,/*doe*/
			 0,/*cflo*/
			 0,/*ypos*/
			 0,/*lilmd*/
			 0,/*cilmd*/
			 0,/*list*/
			 2,/*boy*/
			 0,/*box*/
			 BLK_H8,/*bh*/
			 BLK_W8,/*bw*/
			 pos[6]/*xpos*/);

    tdd[tkn++] = TDD_MV(ymvy[7], ymvx[7]);

    tdd[tkn++] = TDD_CMD(bwt,/*bidir*/
			 1,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 1,/*doe*/
			 1,/*cflo*/
			 0,/*ypos*/
			 0,/*lilmd*/
			 0,/*cilmd*/
			 0,/*list*/
			 2,/*boy*/
			 2,/*box*/
			 BLK_H8,/*bh*/
			 BLK_W8,/*bw*/
			 pos[7]/*xpos*/);
  }

  if (dir & 1){
    tdd[tkn++] = TDD_MV(cmvy[0], cmvx[0]);

    tdd[tkn++] = TDD_CMD(bwt,/*bidir*/
			 0,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 (dir&2)?0:1,/*doe*/
			 0,/*cflo*/
			 0,/*ypos*/
			 0,/*lilmd*/
			 no_rnd?(IS_ILUT1):(IS_ILUT2),/*cilmd*/
			 0,/*list*/
			 0,/*boy*/
			 0,/*box*/
			 BLK_H16,/*bh*/
			 BLK_W16,/*bw*/
			 cpos[0]/*xpos*/);
  }

  if (dir & 2){
    tdd[tkn++] = TDD_MV(cmvy[1], cmvx[1]);

    tdd[tkn++] = TDD_CMD(bwt,/*bidir*/
			 1,/*refdir*/
			 0,/*fld*/
			 0,/*fldsel*/
			 0,/*rgr*/
			 0,/*its*/
			 1,/*doe*/
			 0,/*cflo*/
			 0,/*ypos*/
			 0,/*lilmd*/
			 no_rnd?(IS_ILUT1):(IS_ILUT2),/*cilmd*/
			 0,/*list*/
			 0,/*boy*/
			 0,/*box*/
			 BLK_H16,/*bh*/
			 BLK_W16,/*bw*/
			 cpos[1]/*xpos*/);
  }

  tdd[tkn++] = TDD_HEAD(1,/*vld*/
			1,/*lk*/
			0,/*op*/
			0,/*ch1pel*/
			0,/*ch2pel*/
			TDD_POS_SPEC,/*posmd*/
			TDD_MV_SPEC,/*mvmd*/
			0,/*tkn*/
			0xFF,/*mby*/
			0xFF/*mbx*/);

  tdd[tkn++] = SYN_HEAD(1, 0, 2, 0, 0xFFFF);
  
  SET_REG1_DDC(TCSM1_PADDR((int)motion_dha) | 0x1);

  return;
}
