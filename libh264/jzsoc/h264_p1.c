#define P1_USE_PADDR

#define av_always_inline __attribute__((always_inline)) inline
#define __p1_text __attribute__ ((__section__ (".p1_text")))
#define __p1_main __attribute__ ((__section__ (".p1_main")))
#define __p1_data __attribute__ ((__section__ (".p1_data")))
#define __place_k0_data__

#include "../../libjzcommon/jzasm.h"
#include "../../libjzcommon/jzmedia.h"
#include "../../libjzcommon/t_vputlb.h"
#include "t_motion.h"
#include "t_intpid.h"
#include "h264_p1_types.h"
#include "t_sde_aux.h"
#include "mem_def_falcon.h"
#include "t_gp.h"
#include "t_vmau.h"
#include "jz4770_dblk_hw.h"
#include "eyer_driver_aux.c"
#include "t_sde_aux.c"
#include "h264_p1_dblk.c"


__p1_main int main() {
  int c,i;
  S32I2M(xr16, 0x3);
  unsigned int y_qp;
  unsigned int u_qp;
  unsigned int v_qp;
  volatile int *tdd;
  int mb_num = 0;
  *((volatile unsigned int *)P1_START_BASE) = 0 ;

  SET_VPU_SCHE0(0, 0);
  SET_VPU_SCHE1(0, 0);
  SET_VPU_SCHE2(0, 0);
  SET_VPU_SCHE3(0, 0);
  SET_VPU_SCHE4(0, 1);
  SET_VPU_SCHC(0, /*sch4_act*/
	       1,
	       3, /*sch4_pch1*/
	       1,
	       2, /*sch4_pch0*/
	       0, /*sch3_act*/
	       1,
	       1, /*sch3_pch0*/
	       0, /*sch2_act*/ 
	       1, /*sch2_pe0*/
	       1, /*sch2_pch0*/
	       0, /*sch1_act*/
	       1, /*sch1_pe0*/
	       0  /*sch1_pch0*/
	       );

  const unsigned char (*vpu_chr_tab)[256] = CHR_QT_TAB_BASE;
#if 0
  unsigned int *dbg_ptr = SRAM_END;
  int kk;
  for(kk=0; kk<10; kk++) {
    write_reg(SRAM_END + kk * 4, 0x0);
  }
#endif
  aux_slic_info_p slice_info_ptr = AUX_SLICE_INFO_BASE;
  unsigned int mb_x = slice_info_ptr->start_mx; 
  unsigned int mb_y = slice_info_ptr->start_my;
  unsigned int sde_mb_x = slice_info_ptr->start_mx;
  unsigned int sde_mb_y = slice_info_ptr->start_my;
  unsigned char mb_width = slice_info_ptr->mb_width;
  unsigned char slice_num = slice_info_ptr->slice_num;
  int deblock_filter = slice_info_ptr->deblocking_filter;
  int ref_direct = (slice_info_ptr->sbits_or_ref0cnt > 1 || slice_info_ptr->bscnt_or_ref1cnt > 1);
  ref_direct *= 0xF; 
  mb_num = 0;
  int slice_type = slice_info_ptr->slice_type;
  int *qp_ptr = MAU_QP_BASE;
  *qp_ptr = slice_info_ptr->qscale;

  volatile unsigned int dblk_y_chn_addr = DBLK_CHAIN0_P_BASE;
  volatile unsigned int dblk_uv_chn_addr = DBLK_CHAIN1_P_BASE;
  volatile unsigned int next_dblk_y_chn_addr = DBLK_CHAIN0_P_BASE+DBLK_CHAIN_ONE_SIZE;
  volatile unsigned int next_dblk_uv_chn_addr = DBLK_CHAIN1_P_BASE+DBLK_CHAIN_ONE_SIZE;
  unsigned int aux_sde_mb_terminate = 0 ;
  unsigned int mau_main_src_addr= MAU_SRC_BASE;
  unsigned int next_mau_main_src_addr = mau_main_src_addr + MAU_SRC_ONE_SIZE;
  unsigned int sde_ctrl_dout_caddr = SDE_CTRL_DOUT_ADDR;
  unsigned int next_sde_ctrl_dout_caddr = SDE_CTRL_DOUT_ADDR1;
  volatile unsigned int *neighbor_info = SDE_NEIGHBOR_DIN_ADDR;
  volatile unsigned int *next_neighbor_info = SDE_NEIGHBOR_DIN_ADDR1;
  volatile unsigned int *wfifo_ptr = P2AUX_WIDX_BASE;
  volatile unsigned int vmau_glb_idx = 1;
  unsigned int ctrl_fifo_idx = 0;
  *wfifo_ptr = vmau_glb_idx;   

  volatile unsigned int *gp2_dha = GP2_DHA_BASE;
  unsigned int *gp2_index_ptr = GP2_INDEX_BASE;
  *((volatile int*)(DDMA_GP2_VBASE + OFST_DHA)) = aux_do_get_phy_addr(GP2_DHA_BASE);
  NEIGHBOR_INFO_p aux_sde_top_info_ptr = SDE_NEIGHBOR_INFO_TOP_BASE;
  volatile unsigned int next_motion_dha = MOTION_CHN_FIFO_BASE;
  volatile unsigned int *motion_dha = MOTION_CHN_FIFO_BASE;

  volatile vmau_slv_reg_p mau_slv_reg_ptr;
  volatile int next_mau_slv_reg_ptr = MAU_CHN_BASE;
  mau_slv_reg_ptr = MAU_CHN_BASE;
  *((volatile unsigned int*)motion_dha) = 0 ;
  SET_REG1_DDC(aux_do_get_phy_addr(motion_dha) | 0x1);

  volatile int *sde_sync_end = SDE_SYNC_DOUT_ADDR;
  volatile unsigned int *gp2_out_y_ptr = GP2_OUT_Y_BASE;
  volatile unsigned int *gp2_out_uv_ptr = GP2_OUT_UV_BASE;
  int y_line_strd = (mb_width + 4) * 256;
  int uv_line_strd = y_line_strd/2;
  
  int b_slice = (slice_type == SDE_B_TYPE); 
  int h264_depth_offset=DBLK_REG18(slice_info_ptr->slice_alpha_c0_offset,slice_info_ptr->slice_beta_offset,b_slice);
  unsigned char qp_div_tab[64];
  for (i=0 ; i<64; i++){
    qp_div_tab[i] = ((unsigned int)(i/6)) | ((i%6)<<4);
  }

  cfg_sde(sde_mb_x, mb_num, mb_width,neighbor_info,sde_ctrl_dout_caddr,mau_main_src_addr,aux_sde_top_info_ptr);
  *((volatile int*)(SDE_PBASE + SDE_DESPC_OFST)) = ((aux_do_get_phy_addr(neighbor_info)) & 0xfffffffc) + 1;
  mb_num++;
  sde_mb_x ++;
  if ( sde_mb_x == mb_width ){
    sde_mb_x = 0;
    sde_mb_y ++;
  }

  do {
    unsigned int *sde_ptr = sde_ctrl_dout_caddr; 
    unsigned int dblk_mv_addr = DBLK_MV_ADDR_V_BASE + ctrl_fifo_idx*56*4;  
    struct MB_INTRA_BAC *mb_intra;
    mb_intra   = (struct MB_INTRA_BAC   *)sde_ctrl_dout_caddr;
    
    uint8_t *ly, *lc;    
    ly = *gp2_out_y_ptr + mb_y*y_line_strd + mb_x*256;
    lc = *gp2_out_uv_ptr + mb_y*uv_line_strd + mb_x*128;
    
    mau_slv_reg_ptr = next_mau_slv_reg_ptr;
    next_mau_slv_reg_ptr = (ctrl_fifo_idx == MAU_SFIFO_DEP -1 )? MAU_CHN_BASE: (((unsigned int )mau_slv_reg_ptr) + MAU_CHN_ONE_SIZE);
    motion_dha = next_motion_dha;
    next_motion_dha = (ctrl_fifo_idx == MOTION_CHN_FIFO_ENTRY-1)? MOTION_CHN_FIFO_BASE:((unsigned int)motion_dha + MOTION_CHN_SIZE); 
    gp2_dha = GP2_DHA_BASE + ctrl_fifo_idx*GP2_DHA_ONE_SIZE; 
    gp2_index_ptr = GP2_INDEX_BASE+ctrl_fifo_idx*4;

    dblk_y_chn_addr = DBLK_CHAIN0_P_BASE + ctrl_fifo_idx*DBLK_CHAIN_ONE_SIZE;
    next_dblk_y_chn_addr = (ctrl_fifo_idx == MAU_SFIFO_DEP -1 )? DBLK_CHAIN0_P_BASE: (((unsigned int )dblk_y_chn_addr) +DBLK_CHAIN_ONE_SIZE );
    dblk_uv_chn_addr = DBLK_CHAIN1_P_BASE + ctrl_fifo_idx*DBLK_CHAIN_ONE_SIZE;
    next_dblk_uv_chn_addr = (ctrl_fifo_idx == MAU_SFIFO_DEP -1 )? DBLK_CHAIN1_P_BASE: (((unsigned int )dblk_uv_chn_addr) + DBLK_CHAIN_ONE_SIZE);

    if (aux_sde_mb_terminate == 0){
      cfg_sde(sde_mb_x, mb_num, mb_width,next_neighbor_info,next_sde_ctrl_dout_caddr,next_mau_main_src_addr,aux_sde_top_info_ptr);
      int sde_end, sde_err = 0;
      do {
	sde_err = ((*(volatile int *)(SDE_VBASE + SDE_CTRL_OFST)) & 0x8) ? 1 :0;        
        if (sde_err) goto end;
	sde_end = *sde_sync_end;
      } while (sde_end == 0);
      *sde_sync_end = 0;

      do{
      }while(vmau_glb_idx >= (*wfifo_ptr+P0_AUX_FIFO_DEP));
      
      if (!(mb_intra->decoder_error & 0x1)){
	SET_SDE_DESPC( ((aux_do_get_phy_addr(next_neighbor_info)) & 0xfffffffc) + 1 );
      }else{
	aux_sde_mb_terminate = 1;
      }
    }

    { // mc
#define TDD_TASK(mv, par, boy, box, bh, bw)            \
{                                                      \
  for(blk_i=0; blk_i<2; blk_i++)                       \
    if(dir[blk_i]){                                    \
       *tdd++ = *(int *)mv[blk_i];                     \
       *tdd++ = TDD_CMD(bidir,/*bidir*/                \
			blk_i,/*refdir*/               \
			0,/*fld*/                      \
			0,/*fldsel*/                   \
			0,/*rgr*/                      \
			0,/*its*/                      \
			0,/*doe*/                      \
			0,/*cflo*/                     \
			mv[ blk_i ][1],/*ypos*/        \
			IS_ILUT0,/*lilmd*/             \
			IS_EC,/*cilmd*/                \
			ptr->ref[blk_i][par],          \
			boy,/*boy*/                    \
			box,/*box*/                    \
			bh,/*bh*/                      \
			bw,/*bw*/                      \
			0/*xpos*/);                    \
       tkn++;                                          \
    }                                                  \
}
      unsigned short sde_mb_type = *(unsigned short *)sde_ctrl_dout_caddr;
      int tkn = 0;
      *((unsigned int*)next_motion_dha) = 0;
      tdd = (int *)motion_dha;
      *tdd++;
      if(IS_INTRA(sde_mb_type)){
	tdd[0] = TDD_CFG(1,/*vld*/
			   1,/*lk*/
			   REG1_DDC/*cidx*/);
	tdd[1] = next_motion_dha | 0x132C0000;
	
	tdd[2] = TDD_SYNC(1,/*vld*/
			  1,/*lk*/
			  1, /*crst*/
			  0xFFFF/*id*/);

	tdd[-1] = TDD_HEAD(1,/*vld*/
			   1,/*lk*/
			   QPEL-1,/*ch1pel*/
			   EPEL,/*ch2pel*/ 
			   TDD_POS_AUTO,/*posmd*/
			   TDD_MV_AUTO,/*mvmd*/ 
			   0,/*tkn*/
			   0,/*mby*/
			   0/*mbx*/);
      } else {
    	int blk_i, bidir, dir[2];
	short *mvp[2];
	mb_inter_p_bac_p ptr = sde_ctrl_dout_caddr;
	if( slice_type == SDE_P_TYPE ){
	  mvp[0] = ptr->mv;
	} else {
	  mvp[0] = ((mb_inter_b_bac_p)ptr)->mv;
	  mvp[1] = &((mb_inter_b_bac_p)ptr)->mv[ptr->mv_len>>1];
	}
	
	if(IS_16X16(sde_mb_type)){
	  dir[0] = IS_DIR(sde_mb_type, 0, 0);
	  dir[1] = IS_DIR(sde_mb_type, 0, 1);
	  bidir = dir[0] && dir[1];
	  TDD_TASK(mvp, 0, 0, 0, 3, 3);
	}else if(IS_16X8(sde_mb_type)){
	  dir[0] = IS_DIR(sde_mb_type, 0, 0);
	  dir[1] = IS_DIR(sde_mb_type, 0, 1);
	  bidir = dir[0] && dir[1];
	  TDD_TASK(mvp, 0, 0, 0, 2, 3);
	  mvp[0] +=2;
	  mvp[1] +=2;
	  dir[0] = IS_DIR(sde_mb_type, 1, 0);
	  dir[1] = IS_DIR(sde_mb_type, 1, 1);
	  bidir = dir[0] && dir[1];
	  TDD_TASK(mvp, 2, 2, 0, 2, 3);
	}else if(IS_8X16(sde_mb_type)){
	  dir[0] = IS_DIR(sde_mb_type, 0, 0);
	  dir[1] = IS_DIR(sde_mb_type, 0, 1);
	  bidir = dir[0] && dir[1];
	  TDD_TASK(mvp, 0, 0, 0, 3, 2);
	  mvp[0] +=2;
	  mvp[1] +=2;
	  dir[0] = IS_DIR(sde_mb_type, 1, 0);
	  dir[1] = IS_DIR(sde_mb_type, 1, 1);
	  bidir = dir[0] && dir[1];
	  TDD_TASK(mvp, 1, 0, 2, 3, 2);
	}else{
	  int i;
	  for(i=0; i<4; i++){
	    const int sub_mb_type= ptr->sub_mb_type[i];
	    const int n= 4*i;
	    dir[0] = IS_DIR(sub_mb_type, 0, 0);
	    dir[1] = IS_DIR(sub_mb_type, 0, 1);
	    bidir = dir[0] && dir[1];
	    
	    if(IS_SUB_8X8(sub_mb_type)){
	      TDD_TASK(mvp, i, (i & 0x2)/*boy*/, (i & 0x1)*2/*box*/, 2, 2);
	      mvp[0] +=2;
	      mvp[1] +=2;
	    }else if(IS_SUB_8X4(sub_mb_type)){
	      TDD_TASK(mvp, i, (i & 0x2)/*boy*/, (i & 0x1)*2/*box*/, 1, 2);
	      mvp[0] +=2;
	      mvp[1] +=2;
	      TDD_TASK(mvp, i, (i & 0x2)+1/*boy*/, (i & 0x1)*2/*box*/, 1, 2);
	      mvp[0] +=2;
	      mvp[1] +=2;
	    }else if(IS_SUB_4X8(sub_mb_type)){
	      TDD_TASK(mvp, i, (i & 0x2)/*boy*/, (i & 0x1)*2/*box*/, 2, 1);
	      mvp[0] +=2;
	      mvp[1] +=2;
	      TDD_TASK(mvp, i, (i & 0x2)/*boy*/, (i & 0x1)*2+1/*box*/, 2, 1);
	      mvp[0] +=2;
	      mvp[1] +=2;
	    }else{
	      int j;
	      for(j=0; j<4; j++){
		TDD_TASK(mvp, i, (i & 0x2) + (j & 0x2)/2/*boy*/, (i & 0x1)*2 + (j & 0x1)/*box*/, 1, 1);
		mvp[0] +=2;
		mvp[1] +=2;
	      } //j
	    } //BLK4X4
	  } //i
	} //BLK8X8
	tdd[-1] |= 0x1<<TDD_DOE_SFT;
	tdd[0] = TDD_CFG(1,/*vld*/
			 1,/*lk*/
			 REG1_DDC/*cidx*/);
	tdd[1] = next_motion_dha | 0x132C0000;
	
	tdd[2] = TDD_SYNC(1,/*vld*/
			  1,/*lk*/
			  1, /*crst*/
			  0xFFFF/*id*/);

	*(volatile int *)motion_dha = TDD_HEAD(1,/*vld*/
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

    *qp_ptr += mb_intra->qscal_diff;
    if(((unsigned)*qp_ptr) > 51){
      if(*qp_ptr<0) *qp_ptr+= 52;
      else          *qp_ptr-= 52;
    }
    y_qp = *qp_ptr;
    u_qp = vpu_chr_tab[0][y_qp];
    v_qp = vpu_chr_tab[1][y_qp];

    int last_glb_idx = (aux_sde_mb_terminate == 1) ? (vmau_glb_idx+1) : vmau_glb_idx;
    hw_dblk_mb_aux_c(sde_ptr,slice_type,mb_x,mb_y,y_qp,u_qp,v_qp,dblk_y_chn_addr,dblk_uv_chn_addr,
		     vmau_glb_idx,&aux_sde_top_info_ptr[mb_x],dblk_mv_addr,h264_depth_offset,deblock_filter);
    backup_sde_bac_aux(mb_x,ref_direct,sde_ctrl_dout_caddr,aux_sde_top_info_ptr);

    if (aux_sde_mb_terminate == 1){
      unsigned int * dblk_dha_ptr = dblk_y_chn_addr;
      dblk_dha_ptr[DBLK_CHN_SCH_ID_IDX] = vmau_glb_idx + 1;
      dblk_dha_ptr = dblk_uv_chn_addr;
      dblk_dha_ptr[DBLK_CHN_SCH_ID_IDX] = vmau_glb_idx + 1;
    }

    {
      unsigned int gp2_out_up_y_ptr = (ly - y_line_strd) + 12*16;
      unsigned int gp2_out_up_uv_ptr =(lc - uv_line_strd) + 4*16;    
      
      unsigned int gp2_out_l_y_ptr = (ly - 256) + 12;
      unsigned int gp2_out_l_uv_ptr =(lc - 128) + 4;    

      write_reg((&gp2_dha[1]), ly);
      write_reg((&gp2_dha[5]), lc);
      write_reg((&gp2_dha[9]), lc + 8);
      write_reg((&gp2_dha[13]), gp2_out_up_y_ptr);
      write_reg((&gp2_dha[17]), gp2_out_up_uv_ptr);
      write_reg((&gp2_dha[21]), gp2_out_up_uv_ptr + 8);
      
      if ((ctrl_fifo_idx == MAU_SFIFO_DEP -1)){
	gp2_dha[25] = gp2_out_l_y_ptr;
	gp2_dha[29] = gp2_out_l_uv_ptr;
	gp2_dha[33] = gp2_out_l_uv_ptr + 8;
      }else if ((ctrl_fifo_idx == 0)){
        if (mb_num == 1){
          gp2_dha[25] = gp2_out_l_y_ptr;
	  gp2_dha[29] = gp2_out_l_uv_ptr;
	  gp2_dha[33] = gp2_out_l_uv_ptr + 8;
        }else{
	  gp2_dha[25] = 0x132e0000;
	  gp2_dha[29] = 0x132e0000;
	  gp2_dha[33] = 0x132e0000;
        }
      }

      if (mb_y == slice_info_ptr->mb_height - 1){
	write_reg((&gp2_dha[3]), GP_GSC(0,16,16*16));
	write_reg((&gp2_dha[7]), GP_GSC(0,8,8*8));    
	write_reg((&gp2_dha[11]), GP_GSC(0,8,8*8));
      }else{
        write_reg((&gp2_dha[3]), GP_GSC(0,16,16*12));
	write_reg((&gp2_dha[7]), GP_GSC(0,8,8*4));    
	write_reg((&gp2_dha[11]), GP_GSC(0,8,8*4));
      }
      *gp2_index_ptr = vmau_glb_idx+1;
    }	

    mau_slv_reg_ptr->ncchn_addr = aux_do_get_phy_addr(next_mau_slv_reg_ptr);	
    mau_slv_reg_ptr->main_len = mb_intra->coef_cnt;
    mau_slv_reg_ptr->main_cbp = mb_intra->nnz_dct;
    mau_slv_reg_ptr->main_addr = aux_do_get_phy_addr(mau_main_src_addr);    
    mau_slv_reg_ptr->c_pred_mode[0] = mb_intra->chroma_pred_mode;
    mau_slv_reg_ptr->y_pred_mode[0] = mb_intra->intra4x4_pred_mode0;
    mau_slv_reg_ptr->y_pred_mode[1] = mb_intra->intra4x4_pred_mode1;
    mau_slv_reg_ptr->top_lr_avalid = mb_intra->intra_avail;
    mau_slv_reg_ptr->id = last_glb_idx;
    mau_slv_reg_ptr->dec_incr = (ctrl_fifo_idx == (MAU_SFIFO_DEP-1))? 0x0 : 0x80;
    mau_slv_reg_ptr->quant_para = (qp_div_tab[y_qp] & 0x7F) 
      | ((qp_div_tab[u_qp] & 0x7F) << H264_CQP_DIV6_SFT) 
      | (((qp_div_tab[v_qp] & 0x7F) << H264_C1QP_DIV6_SFT));

    SET_VPU_SCHE0(last_glb_idx/16, last_glb_idx);

    if (aux_sde_mb_terminate == 1) {
      *(volatile int *)next_motion_dha = TDD_HEAD(1,/*vld*/
						  0,/*lk*/
						  QPEL-1,/*ch1pel*/
						  EPEL,/*ch2pel*/ 
						  TDD_POS_AUTO,/*posmd*/
						  TDD_MV_AUTO,/*mvmd*/ 
						  0,/*tkn*/
						  0,/*mby*/
						  0/*mbx*/);
      
      break;
    }

    mau_main_src_addr = next_mau_main_src_addr;
    if ( ctrl_fifo_idx == MAU_SFIFO_DEP -1 ){
      next_mau_main_src_addr = MAU_SRC_BASE;
    }else{
      next_mau_main_src_addr = (mau_main_src_addr + MAU_SRC_ONE_SIZE);
    }

    ctrl_fifo_idx++;
    if ( ctrl_fifo_idx == MAU_SFIFO_DEP ){
      ctrl_fifo_idx = 0;
    }

    
    vmau_glb_idx++;    
    mb_x = sde_mb_x;
    mb_y = sde_mb_y;
    sde_mb_x ++;
    if (sde_mb_x == mb_width ){
      sde_mb_x = 0;
      sde_mb_y ++;
    }
    
    mb_num++;
    XCHG2(sde_ctrl_dout_caddr, next_sde_ctrl_dout_caddr);
    XCHG2(neighbor_info, next_neighbor_info);
  } while(aux_sde_mb_terminate != 1);

  do{
  }while((GET_VPU_SCHE4()&0x1F) != ((vmau_glb_idx+1)&0x1F) );

  if (ctrl_fifo_idx != 7){
    unsigned char *left_ybak = DBLK_YDST_BASE + 12;
    unsigned char *left_ubak = DBLK_UDST_BASE + 4;
    unsigned char *left_vbak = DBLK_VDST_BASE + 4;
    unsigned char *ysrc = MAU_YDST_BASE + ctrl_fifo_idx*16 + 12;
    unsigned char *usrc = MAU_UDST_BASE + ctrl_fifo_idx*8 + 4;
    unsigned char *vsrc = MAU_VDST_BASE + ctrl_fifo_idx*8 + 4;    
    for(i=0; i<16; i++){
      *(unsigned int *)(left_ybak + i*DBLK_YDST_STR) = *(unsigned int *)(ysrc + i*DBLK_YDST_STR);       
    }
    for(i=0; i<8; i++){
      *(unsigned int *)(left_ubak + i*DBLK_UDST_STR) = *(unsigned int *)(usrc + i*DBLK_UDST_STR);       
      *(unsigned int *)(left_vbak + i*DBLK_UDST_STR) = *(unsigned int *)(vsrc + i*DBLK_UDST_STR);       
    }
  }

 end:
  SET_VPU_SCHC(0, /*sch4_act*/
	       0, /*sch4_pe1*/
	       0, /*sch4_pch1*/
	       0, /*sch4_pe0*/
	       0, /*sch4_pch0*/
	       0, /*sch3_act*/
	       0, /*sch3_pe0*/
	       0, /*sch3_pch0*/
	       0, /*sch2_act*/ 
	       0, /*sch2_pe0*/
	       0, /*sch2_pch0*/
	       0, /*sch1_act*/
	       0, /*sch1_pe0*/
	       0  /*sch1_pch0*/
	       );
  #define MSG 0x132A0014
  *(volatile unsigned int *)MSG = 1;

  *(volatile unsigned int *)P1_START_BASE = AUX_END_VALUE;
  i_nop;
  i_nop;
  i_nop;
  i_nop;
  i_wait();
}
