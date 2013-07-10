#define __place_k0_data__

#include "t_motion.h"
#include "t_intpid.h"
#include "../../libjzcommon/t_vputlb.h"
#include "mem_def_falcon.h"

void motion_task(int bar,
		 int *tdd, int *tkn, 
		 int list0, int list1,
		 int blkh, int blkw,
		 int boy, int box,short *mvp[2], struct MB_INTER_P_BAC *ptr)
{
  int i, dir[2], refn[2], is_bidir;
  dir[0] = list0 != 0;
  dir[1] = list1 != 0;
  is_bidir = dir[0] && dir[1];
 
  for(i=0; i<2; i++){
    if(dir[i]){
      int refli = ptr->ref[i][bar];
      tdd[ 2*tkn[0]] = *(int *)mvp[i];
      tdd[2*tkn[0]+1] = TDD_CMD(is_bidir,/*bidir*/
				i,/*refdir*/
				0,/*fld*/
				0,/*fldsel*/
				0,/*rgr*/
				0,/*its*/
				0,/*doe*/
				0,/*cflo*/
			        mvp[i][1] & 0x7,/*ypos*/
				IS_ILUT0,/*lilmd*/
				IS_EC,/*cilmd*/
				refli,//h->ref_cache[i][ scan8[n] ],/*list*/
				boy,/*boy*/
				box,/*box*/
				blkh,/*bh*/
				blkw,/*bw*/
				mvp[i][0] & 0x7/*xpos*/);
      (*tkn)++;
    }
  }
}

void motion_execute(unsigned int *mc_dha,unsigned int next_motion_dha, unsigned int sde_ctrl_dout,int slice_type, int mb_x, int mb_y)
{

  int mv_len;
  *((volatile unsigned int*)next_motion_dha) = 0 ;
  volatile unsigned int *tdd = (int *)mc_dha;
  int tkn = 0;
  tdd++;

  int blk_i, bidir, dir[2];
  short *mvp[2];
  mb_inter_p_bac_p ptr = (struct MB_INTER_P_BAC *)sde_ctrl_dout;
  int mb_type = ptr->mb_type;
  if( slice_type == SDE_P_TYPE ){
    mvp[0] = ptr->mv;
  } else {
    mvp[0] = ((mb_inter_b_bac_p)ptr)->mv;
    mvp[1] = &((mb_inter_b_bac_p)ptr)->mv[ptr->mv_len>>1];
  }
  
  if(IS_INTRA(mb_type)){
    tdd[ 0 ] = TDD_CFG(1,/*vld*/
		       1,/*lk*/
		       REG1_DDC/*cidx*/);
    tdd[1] = aux_do_get_phy_addr(next_motion_dha);
    tdd[2] = TDD_SYNC(1,/*vld*/
		      1,/*lk*/
		      1, /*crst*/
		      0xFFFF/*id*/);
    
    tdd[-1] = TDD_HEAD(1,/*vld*/
		       1,/*lk*/
		       QPEL-1,/*ch1pel*/
		       EPEL,/*ch2pel*/ 
		       TDD_POS_SPEC,/*posmd*/
		       TDD_MV_AUTO,/*mvmd*/ 
		       0,/*tkn*/
		       mb_y,/*mby*/
		       mb_x/*mbx*/);
  }else{
    if(IS_16X16(mb_type)){
      motion_task(0,tdd, &tkn,
		  IS_DIR(mb_type, 0, 0), IS_DIR(mb_type, 0, 1),
		  3/*blkh*/, 3/*blkw*/, 0/*boy*/, 0/*box*/,mvp,ptr);
    }else if(IS_16X8(mb_type)){
      motion_task(0,tdd, &tkn,
		  IS_DIR(mb_type, 0, 0), IS_DIR(mb_type, 0, 1),
		  2/*blkh*/, 3/*blkw*/, 0/*boy*/, 0/*box*/,mvp,ptr);
      mvp[0] +=2;
      mvp[1] +=2;
      motion_task(2,tdd, &tkn,
		  IS_DIR(mb_type, 1, 0), IS_DIR(mb_type, 1, 1),
		  2/*blkh*/, 3/*blkw*/, 2/*boy*/, 0/*box*/,mvp,ptr);
    }else if(IS_8X16(mb_type)){
      motion_task(0,tdd, &tkn,
		  IS_DIR(mb_type, 0, 0), IS_DIR(mb_type, 0, 1),
		  3/*blkh*/, 2/*blkw*/, 0/*boy*/, 0/*box*/,mvp,ptr);
      
      mvp[0] +=2;
      mvp[1] +=2;
      motion_task(1,tdd, &tkn,
		  IS_DIR(mb_type, 1, 0), IS_DIR(mb_type, 1, 1),
		  3/*blkh*/, 2/*blkw*/, 0/*boy*/, 2/*box*/,mvp,ptr);
    }else{
      int i;
      for(i=0; i<4; i++){
	const int sub_mb_type= ptr->sub_mb_type[i];
	const int n= 4*i;
	if(IS_SUB_8X8(sub_mb_type)){
	  motion_task(i,tdd, &tkn,
		      IS_DIR(sub_mb_type, 0, 0), IS_DIR(sub_mb_type, 0, 1),
		      2/*blkh*/, 2/*blkw*/, (i & 0x2)/*boy*/, (i & 0x1)*2/*box*/,mvp,ptr);
	  mvp[0] +=2;
	  mvp[1] +=2;
	}else if(IS_SUB_8X4(sub_mb_type)){
	  motion_task(i,tdd, &tkn,
		      IS_DIR(sub_mb_type, 0, 0), IS_DIR(sub_mb_type, 0, 1),
		      1/*blkh*/, 2/*blkw*/, (i & 0x2)/*boy*/, (i & 0x1)*2/*box*/,mvp,ptr);
	  mvp[0] +=2;
	  mvp[1] +=2;
	  motion_task(i,tdd, &tkn,
		      IS_DIR(sub_mb_type, 0, 0), IS_DIR(sub_mb_type, 0, 1),
		      1/*blkh*/, 2/*blkw*/, (i & 0x2)+1/*boy*/, (i & 0x1)*2/*box*/,mvp,ptr);
	  mvp[0] +=2;
	  mvp[1] +=2;
	}else if(IS_SUB_4X8(sub_mb_type)){
	  motion_task(i,tdd, &tkn,
		      IS_DIR(sub_mb_type, 0, 0), IS_DIR(sub_mb_type, 0, 1),
		      2/*blkh*/, 1/*blkw*/, (i & 0x2)/*boy*/, (i & 0x1)*2/*box*/,mvp,ptr);
	  mvp[0] +=2;
	  mvp[1] +=2;
	  motion_task(i,tdd, &tkn,
		      IS_DIR(sub_mb_type, 0, 0), IS_DIR(sub_mb_type, 0, 1),
		      2/*blkh*/, 1/*blkw*/, (i & 0x2)/*boy*/, (i & 0x1)*2+1/*box*/,mvp,ptr);
	  mvp[0] +=2;
	  mvp[1] +=2;
	}else{
	  int j;
	  for(j=0; j<4; j++){
	    motion_task(i,tdd, &tkn,
			IS_DIR(sub_mb_type, 0, 0), IS_DIR(sub_mb_type, 0, 1),
			1/*blkh*/, 1/*blkw*/, 
			(i & 0x2) + (j & 0x2)/2/*boy*/, (i & 0x1)*2 + (j & 0x1)/*box*/,mvp,ptr);
	    mvp[0] +=2;
	    mvp[1] +=2;
	  } //j
	} //BLK4X4
      } //i
    } //BLK8X8

    tdd[2*tkn-1] |= 0x1<<TDD_DOE_SFT;
    tdd[2*tkn] = TDD_CFG(1,/*vld*/
			   1,/*lk*/
			   REG1_DDC/*cidx*/);
    tdd[2*tkn+1] = aux_do_get_phy_addr(next_motion_dha);
    tdd[2*tkn+2] = TDD_SYNC(1,/*vld*/
			    1,/*lk*/
			    1, /*crst*/
			    0xFFFF/*id*/);
    
    tdd[-1] = TDD_HEAD(1,/*vld*/
		       1,/*lk*/
		       QPEL-1,/*ch1pel*/
		       EPEL,/*ch2pel*/ 
		       TDD_POS_AUTO,/*posmd*/
		       TDD_MV_AUTO,/*mvmd*/ 
		       tkn,/*tkn*/
		       mb_y,/*mby*/
		       mb_x/*mbx*/);
  }
}

