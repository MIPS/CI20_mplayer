
#include "h264_cabac_hw.h"
#undef printf

//__cache_text0__ 
static av_always_inline 
int get_cabac_hw2(unsigned int ctx_id){
  int hw_rsl;
  CPU_SET_CTRL(0,0,0, 0, 0, 0, 0, 0, 0, ctx_id);
#if 0
  i_nop;i_nop;
  do {hw_rsl = i_mfc0_2(21, (CABAC_CPU_IF_RSLT));
  }while (hw_rsl < 0);
#else
  i_mfc0_2(21, (CABAC_CPU_IF_RSLT));
  hw_rsl = i_mfc0_2(21, (CABAC_CPU_IF_RSLT));
#endif
  return hw_rsl;
}
 
//__cache_text0__ 
static av_always_inline 
int get_cabac_terminate_hw(){
  int hw_rsl;
  CPU_SET_CTRL(0,0,0, 0, 0, 0, 0, 0, 0, (unsigned int)276);
#if 1
  i_nop;i_nop;
  do {hw_rsl = i_mfc0_2(21, (CABAC_CPU_IF_RSLT));
  }while (hw_rsl < 0);
#else
  i_mfc0_2(21, (CABAC_CPU_IF_RSLT));
  hw_rsl = i_mfc0_2(21, (CABAC_CPU_IF_RSLT));
#endif
  return hw_rsl;
}

#if 0
static int get_cabac_hw(CABACContext *c, uint8_t * const state){
  int hw_rsl;
  unsigned int ctx_id;
  ctx_id = state - cabac_state_base;

  CPU_SET_CTRL(0,0,0, 0, 0, 0, 0, 0, 0, ctx_id);
  do {hw_rsl = i_mfc0_2(21, (CABAC_CPU_IF_RSLT));
  }while (hw_rsl < 0);

  return hw_rsl;
}
#endif

#if 0
static int get_cabac_bypass_hw(CABACContext *c){
  int hw_rsl;

  CPU_SET_CTRL(0,0,0, 0, 0, 0, 1, 1, 0, 0);
  do {hw_rsl = i_mfc0_2(21, (CABAC_CPU_IF_RSLT));
  }while (hw_rsl < 0);

  return hw_rsl;
}
#endif

//__cache_text0__
static av_always_inline
int decode_cabac_b_mb_type_hw(int ctx) {
  CPU_SET_HYBRID_PARA(ctx);
  CPU_SET_HYBRID_CMD(7);
  CPU_POLL_HYBRID();
  return CPU_GET_HYBRID();
}

//__cache_text0__
static av_always_inline 
int decode_cabac_p_mb_type_hw() {
  CPU_SET_HYBRID_CMD(6);
  CPU_POLL_HYBRID();
  return CPU_GET_HYBRID();
}

//__cache_text0__
static av_always_inline 
int decode_cabac_mb_cbp_hw( H264Context *h) {
  short cbp_a,cbp_b;
  cbp_a = h->left_mb_avail ? h->left_cbp : (h->left_cbp|0xf);
  cbp_b = h->top_mb_avail ? h->top_cbp  : (h->top_cbp|0xf);
  CPU_SET_HYBRID_PARA(cbp_a|(cbp_b<<16));
  CPU_SET_HYBRID_CMD(5);
  CPU_POLL_HYBRID();
  return CPU_GET_HYBRID();
}

//__cache_text0__
static av_always_inline int decode_cabac_mb_intra4x4_pred_mode_hw( int pred_mode ) {
  CPU_SET_HYBRID_PARA(pred_mode);
  CPU_SET_HYBRID_CMD(4);
  CPU_POLL_HYBRID();
  return CPU_GET_HYBRID();
}

//__cache_text0__
static av_always_inline 
int decode_cabac_p_mb_sub_type_hw() {
  CPU_SET_HYBRID_CMD(2);
  CPU_POLL_HYBRID();
  return CPU_GET_HYBRID();
}

//__cache_text0__
static av_always_inline 
int decode_cabac_b_mb_sub_type_hw() {
  CPU_SET_HYBRID_CMD(3);
  CPU_POLL_HYBRID();
  return CPU_GET_HYBRID();
}

__cache_text0__
static int decode_cabac_mb_ref_hw( H264Context *h, int list, int n ) {
  int scan_n = scan8[n];
  int refa = h->ref_cache[list][scan_n - 1]>0;
  int refb = h->ref_cache[list][scan_n - 8]>0;
  int ref;
  int ctx;
  if( h->slice_type == FF_B_TYPE) {
    int di=n>>2;
    refa  &= !h->direct_slim_cache[6+di+(di&0x2)-1];
    refb  &= !h->direct_slim_cache[6+di+(di&0x2)-4];
  }

  ctx=refa+(2*refb);
  CPU_SET_HYBRID_PARA(ctx);
  CPU_SET_HYBRID_CMD(1);
  CPU_POLL_HYBRID();
  ref=CPU_GET_HYBRID();

  return ref;
}

__cache_text0__
int decode_cabac_mb_mvd_hw( short * blk_mvd ) {
#if 0
    uint16_t hvx,hvy;
    S32LDD(xr1,blk_mvd,-4);
    S32LDD(xr2,blk_mvd,-32);
    D16CPS(xr1,xr1,xr1);//xr1:abs_mv_left
    D16CPS(xr2,xr2,xr2);//xr2:abs_mv_top
    S32SFL(xr3,xr0,xr1,xr4,3);//xr3: abs_mvy_left_word, xr4:abs_mvx_left_word
    S32SFL(xr5,xr0,xr2,xr6,3);//xr5: abs_mvy_top_word, xr6:abs_mvx_top_word
    D32ASUM_AA(xr3,xr5,xr6,xr4);//xr3: sum_mvy, xr4: sum_mvx
    S32I2M(xr1,2);
    S32I2M(xr2,32);
    S32SLT(xr5,xr1,xr4);//amvdx>=3
    S32SLT(xr6,xr2,xr4);//amvdx>32
    S32SLT(xr7,xr1,xr3);//amvdy>=3
    S32SLT(xr8,xr2,xr3);//amvdy>32
    D32ASUM_AA(xr7,xr8,xr6,xr5);//(amvd>=3) + (amvd>32)
    //    printf("Xctx=%d\n",S32M2I(xr5));
    CPU_SET_MVD_EN(1,S32M2I(xr5));
    int ctx=4+S32M2I(xr7);
    CPU_POLL_MVD();
    hvx=((int)CPU_GET_MVD())>>5;
    //    printf("Yctx=%d\n",ctx);
    CPU_SET_MVD_EN(1,ctx);
    CPU_POLL_MVD();
    hvy=((int)CPU_GET_MVD())>>5;
    //    printf("return 0x%x\n",hvx|(hvy<<16));
    return hvx|(hvy<<16);
#else
    uint32_t hvx,hvy;
    S32I2M(xr9,0xa0);
    S32LDD(xr1,blk_mvd,-4);
    S32LDD(xr2,blk_mvd,-32);
    D16CPS(xr1,xr1,xr1);//xr1:abs_mv_left
    D16CPS(xr2,xr2,xr2);//xr2:abs_mv_top
    S32SFL(xr3,xr0,xr1,xr4,3);//xr3: abs_mvy_left_word, xr4:abs_mvx_left_word
    S32SFL(xr5,xr0,xr2,xr6,3);//xr5: abs_mvy_top_word, xr6:abs_mvx_top_word
    D32ASUM_AA(xr3,xr5,xr6,xr4);//xr3: sum_mvy, xr4: sum_mvx
    S32I2M(xr1,2);
    S32I2M(xr2,32);
    S32SLT(xr5,xr1,xr4);//amvdx>=3
    S32SLT(xr6,xr2,xr4);//amvdx>32
    D32ACC_AA(xr5,xr6,xr9,xr0);//(amvdx>=3) + (amvdx>32) + mvd_en
    D32SLL(xr5,xr5,xr0,xr0,11);//xr5: left shift 11: ctrl_data for x
    int tmp=S32M2I(xr5);
    S32SLT(xr7,xr1,xr3);//amvdy>=3
    i_mtc0_2(tmp, 21, CABAC_CPU_IF_CTRL);
    S32SLT(xr8,xr2,xr3);//amvdy>32
    D32ACC_AA(xr7,xr8,xr9,xr0);//(amvdy>=3) + (amvdy>32) + mvd_en
    D32ACC_AA(xr7,xr1,xr1,xr0);//ctx+=4 for mvdy
    D32SLL(xr7,xr7,xr0,xr0,11);//xr7: left shift 11: ctrl_data for y
    tmp=S32M2I(xr7);
    CPU_POLL_MVD();
    hvx=((int)CPU_GET_MVD());
    i_mtc0_2(tmp, 21, CABAC_CPU_IF_CTRL);
    S32I2M(xr10,hvx);
    D32SLR(xr10,xr10,xr0,xr0,5);
    CPU_POLL_MVD();
    hvy=((int)CPU_GET_MVD());
    S32I2M(xr11,hvy);
    D32SLR(xr11,xr11,xr0,xr0,5);
    S32SFL(xr11,xr11,xr10,xr10,3);
    return S32M2I(xr10);
#endif
}

int cabac_slice_init_hw(H264Context *h) {
  int i;
  char * bs_phy_addr;
  int * ctx_tbl;

  cabac_state_base = &h->cabac_state[0];

  int va; 
  va = (int)0x80000000;
  for(i=0; i<512; i++) { 
    i_cache(0x1,va,0);
    va += 32; 
  }   
  i_sync(); 

  bs_phy_addr = get_phy_addr(h->cabac.bytestream_start);
  //printf("bs_phy_addr = 0x%x h->cabac.bytestream_start = 0x%x\n", bs_phy_addr, h->cabac.bytestream_start);
  CPU_SET_BSADDR((unsigned int)(bs_phy_addr) & 0xfffffffc);
  CPU_SET_BSOFST(((unsigned int)h->cabac.bytestream_start & 0x3) << 3);
  CPU_SET_CTRL(0,0,0, 1, 0, 0, 0, 1, 0, 0);//slice init

  ctx_tbl = (int *)(CABAC_VBASE+CABAC_CTX_TBL_OFST);
  for(i=0;i<460;i++) ctx_tbl[i]=lps_comb[h->cabac_state[i]];
  ctx_tbl[276] = lps_comb[126];

  CPU_POLL_SLICE_INIT();
}


