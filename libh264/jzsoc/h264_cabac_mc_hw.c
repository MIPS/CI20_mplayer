#include "t_motion.h"
#include "h264_dcore.h"
#include "h264_tcsm1.h"

#define EDGE_WIDTH 32
#define IS_WT1    1
#define IS_EC     1
#define IS_ILUT0  0
#define IS_ILUT1  2

void motion_task_cabac(H264_Slice_GlbARGs *SLICE_T, H264_MB_Ctrl_DecARGs *dmb, int mv_n, 
		 int i8x8,
		 int *tdd, int *tkn, 
		 int list0, int list1,
		 int blkh, int blkw,
		 int boy, int box)
{
  int i, dir[2], is_bidir, mvy, mvx;
  int8_t * Inter_Dec_base=(uint32_t)dmb+sizeof(struct H264_MB_Ctrl_DecARGs);
  int mv_num=*(uint32_t*)Inter_Dec_base;
  int8_t (*ref_base)[8]=Inter_Dec_base+4;
  short (*mv_base)[2]=Inter_Dec_base+4+4+((32+8)<<(SLICE_T->slice_type-2));

  dir[0] = list0 != 0;
  dir[1] = list1 != 0;
  is_bidir = dir[0] && dir[1];

  for(i=0; i<2; i++){
    if(dir[i]){
      i_movn(mv_base,mv_base+mv_num,i); 
      mvx = mv_base[mv_n][0];
      mvy = mv_base[mv_n][1];
      
      int ref_cnt= ref_base[i][i8x8];
      int mx = mvx + dmb->mb_x*16*4 + box*4;
      int my = mvy + dmb->mb_y*16*4 + boy*4;

      const int full_mx= mx>>2;
      const int full_my= my>>2;

      if(full_mx < -EDGE_WIDTH + 2){
	mvx += (-EDGE_WIDTH+2-full_mx)<<2;
      }
      if(full_mx + 16 > 16*SLICE_T->mb_width + EDGE_WIDTH - 3){
	mvx -= (full_mx+16-16*SLICE_T->mb_width-EDGE_WIDTH+3)<<2;
      }
      if(full_my < -EDGE_WIDTH + 2){
	mvy += (-EDGE_WIDTH+2-full_my)<<2;
      }
      if(full_my + 16 > 16*SLICE_T->mb_height + EDGE_WIDTH - 3){
	mvy -= (full_my+16-16*SLICE_T->mb_height-EDGE_WIDTH+3)<<2;
      }

      tdd[ 2*tkn[0]   ] = TDD_MV(mvy, mvx);
      tdd[ 2*tkn[0]+1 ] = TDD_CMD(is_bidir,/*bidir*/
				  i,/*refdir*/
				  0,/*fld*/
				  0,/*fldsel*/
				  0,/*rgr*/
				  0,/*its*/
				  0,/*doe*/
				  0,/*cflo*/
				  mvy & 0x7,/*ypos*/
				  IS_ILUT0,/*lilmd*/
				  IS_EC,/*cilmd*/
                                  ref_cnt, /*list*/     
				  boy,/*boy*/
				  box,/*box*/
				  blkh,/*bh*/
				  blkw,/*bw*/
				  mvx & 0x7/*xpos*/);
      (*tkn)++;
    }
  }
}


void motion_execute_cabac(H264_Slice_GlbARGs *SLICE_T, H264_MB_Ctrl_DecARGs *dmb, uint8_t *motion_dha)
{
  uint8_t *motion_dsa = motion_dha + 0x108;      
  const int mb_type= dmb->mb_type;
  volatile int *tdd = (int *)motion_dha;
  int tkn = 0;
  motion_dsa[0] = 0x0;
  tdd++;
  
  if(IS_16X16(mb_type)){
    motion_task_cabac(SLICE_T, dmb, 0, 0, tdd, &tkn,
		    IS_DIR(mb_type, 0, 0), IS_DIR(mb_type, 0, 1),
		3/*blkh*/, 3/*blkw*/, 0/*boy*/, 0/*box*/);
  }else if(IS_16X8(mb_type)){
    motion_task_cabac(SLICE_T, dmb, 0, 0, tdd, &tkn,
		IS_DIR(mb_type, 0, 0), IS_DIR(mb_type, 0, 1),
		2/*blkh*/, 3/*blkw*/, 0/*boy*/, 0/*box*/);
    
    motion_task_cabac(SLICE_T, dmb, 1, 2, tdd, &tkn,
		IS_DIR(mb_type, 1, 0), IS_DIR(mb_type, 1, 1),
		2/*blkh*/, 3/*blkw*/, 2/*boy*/, 0/*box*/);
  }else if(IS_8X16(mb_type)){
    motion_task_cabac(SLICE_T, dmb, 0, 0, tdd, &tkn,
		IS_DIR(mb_type, 0, 0), IS_DIR(mb_type, 0, 1),
		3/*blkh*/, 2/*blkw*/, 0/*boy*/, 0/*box*/);
    
    motion_task_cabac(SLICE_T, dmb, 1, 1, tdd, &tkn,
		IS_DIR(mb_type, 1, 0), IS_DIR(mb_type, 1, 1),
		3/*blkh*/, 2/*blkw*/, 0/*boy*/, 2/*box*/);
  }else{
    int i;
    int mv_n=0;
    for(i=0; i<4; i++){
      const int sub_mb_type= dmb->sub_mb_type[i];
      if(IS_SUB_8X8(sub_mb_type)){
	motion_task_cabac(SLICE_T, dmb, mv_n, i, tdd, &tkn,
		    IS_DIR(sub_mb_type, 0, 0), IS_DIR(sub_mb_type, 0, 1),
		    2/*blkh*/, 2/*blkw*/, (i & 0x2)/*boy*/, (i & 0x1)*2/*box*/);
	mv_n++;
      }else if(IS_SUB_8X4(sub_mb_type)){
	motion_task_cabac(SLICE_T, dmb, mv_n, i, tdd, &tkn,
		    IS_DIR(sub_mb_type, 0, 0), IS_DIR(sub_mb_type, 0, 1),
		    1/*blkh*/, 2/*blkw*/, (i & 0x2)/*boy*/, (i & 0x1)*2/*box*/);
	mv_n++;
	motion_task_cabac(SLICE_T, dmb, mv_n, i, tdd, &tkn,
		    IS_DIR(sub_mb_type, 0, 0), IS_DIR(sub_mb_type, 0, 1),
		    1/*blkh*/, 2/*blkw*/, (i & 0x2)+1/*boy*/, (i & 0x1)*2/*box*/);
	mv_n++;
      }else if(IS_SUB_4X8(sub_mb_type)){
	motion_task_cabac(SLICE_T, dmb, mv_n, i, tdd, &tkn,
		    IS_DIR(sub_mb_type, 0, 0), IS_DIR(sub_mb_type, 0, 1),
		    2/*blkh*/, 1/*blkw*/, (i & 0x2)/*boy*/, (i & 0x1)*2/*box*/);
	mv_n++;
	motion_task_cabac(SLICE_T, dmb, mv_n, i, tdd, &tkn,
		    IS_DIR(sub_mb_type, 0, 0), IS_DIR(sub_mb_type, 0, 1),
		    2/*blkh*/, 1/*blkw*/, (i & 0x2)/*boy*/, (i & 0x1)*2+1/*box*/);
	mv_n++;
      }else{
	int j;
	for(j=0; j<4; j++){
	  motion_task_cabac(SLICE_T, dmb, mv_n, i, tdd, &tkn,
		      IS_DIR(sub_mb_type, 0, 0), IS_DIR(sub_mb_type, 0, 1),
		      1/*blkh*/, 1/*blkw*/, 
		      (i & 0x2) + (j & 0x2)/2/*boy*/, (i & 0x1)*2 + (j & 0x1)/*box*/);
	  mv_n++;
	} //j
      } //BLK4X4
    } //i
  } //BLK8X8
  
  tdd[2*tkn-1] |= 0x1<<TDD_DOE_SFT;
  tdd[-1] = TDD_HEAD(1,/*vld*/
		     1,/*lk*/
		     0,/*sync*/
		     1,/*ch1pel*/
		     2,/*ch2pel*/ 
		     TDD_POS_SPEC,/*posmd*/
		     TDD_MV_AUTO,/*mvmd*/ 
		     1,/*ch2en*/
		     tkn,/*tkn*/
		     dmb->mb_y,/*mby*/
		     dmb->mb_x/*mbx*/);
  
  tdd[2*tkn] = TDD_HEAD(1,/*vld*/
			0,/*lk*/
			1,/*sync*/
			1,/*ch1pel*/
			2,/*ch2pel*/ 
			TDD_POS_SPEC,/*posmd*/
			TDD_MV_AUTO,/*mvmd*/ 
			1,/*ch2en*/
			0,/*tkn*/
			0xFF,/*mby*/
			0xFF/*mbx*/);
}
