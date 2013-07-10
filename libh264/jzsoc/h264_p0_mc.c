#define __place_k0_data__

#include "t_motion.h"
#include "t_intpid.h"
#include "../../libjzcommon/t_vputlb.h"
#include "mem_def_falcon.h"

#define EDGE_WIDTH 32
#define ROA_ALN     256
#define DOUT_Y_STRD 16
#define DOUT_C_STRD 8
 
void motion_init_h264(int intpid, int cintpid)
{
  int i;
  /************* motion DOUT buffer alloc ***************/
  motion_dsa = MOTION_DSA_BASE;//motion_dha + 0x200;
  motion_iwta = MOTION_IWTA_BASE;//(uint8_t *)MOTION_BUF_BASE + 0x1000; //C_TCSM0_BASE

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
  SET_REG1_CTRL(0,/*esms*/
		0,/*erms*/
		0,/*earm*/
		0,/*pmve*/
		0,/*emet*/
		0,/*esa*/
		1,/*cae*/
		0,/*csf*/
		0xF,/*pgc*/
		1,/*ch2en*/
		3,/*pri*/
		1,/*ckge*/
		1,//old 0,lhuang,/*ofa*/
		0,/*rote*/
		0,/*rotdir*/
		0,/*wm*/
		1,/*ccf*/
		0,/*irqe*/
		0,/*rst*/
		1 /*en*/);  
  SET_REG1_BINFO(AryFMT[intpid],/*ary*/
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
  SET_REG2_BINFO(0,/*ary*/
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

void motion_config(H264Context *h)
{
  int i, j;
  MpegEncContext * const s = &h->s;

  SET_REG1_CTRL(0,/*esms*/
		0,/*erms*/
		0,/*earm*/
		0,/*pmve*/
		0,/*emet*/
		0,/*esa*/
		1,/*cae*/
		0,/*csf*/
		0xF,/*pgc*/
		1,/*ch2en*/
		3,/*pri*/
		1,/*ckge*/
		1,//0, fix lhuang/*ofa*/
		0,/*rote*/
		0,/*rotdir*/
		0,/*wm*/
		1,/*ccf*/
		0,/*irqe*/
		0,/*rst*/
		1 /*en*/);

  
  for(i=0; i<16; i++){ 
    SET_TAB1_RLUT(i,do_get_phy_addr((int)(h->ref_list[0][i].data[0])),  
 		  h->luma_weight[i][0][0], h->luma_weight[i][0][1]); 
    SET_TAB1_RLUT(16+i, do_get_phy_addr((int)(h->ref_list[1][i].data[0])), 
 		  h->luma_weight[i][1][0], h->luma_weight[i][1][1]); 
    SET_TAB2_RLUT(i,do_get_phy_addr((int)(h->ref_list[0][i].data[1])),  
 		  h->chroma_weight[i][0][1][0], h->chroma_weight[i][0][1][1], 
 		  h->chroma_weight[i][0][0][0], h->chroma_weight[i][0][0][1]); 
    SET_TAB2_RLUT(16+i, do_get_phy_addr((int)(h->ref_list[1][i].data[1])),  
 		  h->chroma_weight[i][1][1][0], h->chroma_weight[i][1][1][1], 
 		  h->chroma_weight[i][1][0][0], h->chroma_weight[i][1][0][1]); 
  } 
  
  for(j=0; j<16; j++){
    for(i=0; i<4; i++){
      unsigned char *tmp_ptr;
      unsigned int tmp;
      tmp_ptr = &tmp;
      tmp_ptr[0] = h->implicit_weight[j][i*4+0][0];
      tmp_ptr[1] = h->implicit_weight[j][i*4+1][0];
      tmp_ptr[2] = h->implicit_weight[j][i*4+2][0];
      tmp_ptr[3] = h->implicit_weight[j][i*4+3][0];
      write_reg((&motion_iwta[j*16+4*i]), tmp);
    }
  }

  SET_REG1_PINFO(0,/*rgr*/
		 0,/*its*/
		 0,/*its_sft*/
		 0,/*its_scale*/
		 0/*its_rnd*/);
  SET_REG2_PINFO(0,/*rgr*/
		 0,/*its*/
		 0,/*its_sft*/
		 0,/*its_scale*/
		 0/*its_rnd*/);


  SET_REG1_WINFO(0,/*wt*/
		 (h->use_weight == IS_WT1), /*wtpd*/
		 h->use_weight,/*wtmd*/
		 1,/*biavg_rnd*/
		 h->luma_log2_weight_denom,/*wt_denom*/
		 5,/*wt_sft*/
		 0,/*wt_lcoef*/
		 0/*wt_rcoef*/);
  SET_REG1_WTRND(1<<5);

  SET_REG2_WINFO1(0,/*wt*/
		  h->use_weight_chroma && (h->use_weight == IS_WT1), /*wtpd*/
		  h->use_weight,/*wtmd*/
		  1,/*biavg_rnd*/
		  h->chroma_log2_weight_denom,/*wt_denom*/
		  5,/*wt_sft*/
		  0,/*wt_lcoef*/
		  0/*wt_rcoef*/);
  SET_REG2_WINFO2(5,/*wt_sft*/
		  0,/*wt_lcoef*/
		  0/*wt_rcoef*/);
  SET_REG2_WTRND(1<<5, 1<<5);

  SET_REG1_STRD(s->linesize/16,0,DOUT_Y_STRD);
  SET_REG1_GEOM(s->mb_height*16,s->mb_width*16);
  SET_REG1_DSA(do_get_phy_addr(motion_dsa));
  SET_REG1_IWTA(do_get_phy_addr(motion_iwta));
  SET_REG2_STRD(s->linesize/16,0,DOUT_C_STRD);
}
