#include "h264_tcsm1.h"

void chroma_dc_dequant_idct_c_ori(DCTELEM *block, int qmul){
    const int stride= 16*2;
    const int xStride= 16;
    int a,b,c,d,e;

    a= block[stride*0 + xStride*0];
    b= block[stride*0 + xStride*1];
    c= block[stride*1 + xStride*0];
    d= block[stride*1 + xStride*1];

    e= a-b;
    a= a+b;
    b= c-d;
    c= c+d;

    block[stride*0 + xStride*0]= ((a+c)*qmul) >> 7;
    block[stride*0 + xStride*1]= ((e+b)*qmul) >> 7;
    block[stride*1 + xStride*0]= ((a-c)*qmul) >> 7;
    block[stride*1 + xStride*1]= ((e-b)*qmul) >> 7;
}

void luma_dc_dequant_idct_c_ori(DCTELEM *block, int qmul){
    int i;
    int temp1[8];
    int *tp = temp1-2;
    DCTELEM *blk = block-32;
         
    for (i=0; i<4;i++) {
        if (i==2)
	  blk = blk + 96;
        else
          blk = blk + 32;
        S16LDD(xr1,blk,0,ptn1);
        S16LDD(xr1,blk,32,ptn0);  //xr1:b0,b16 
        S16LDD(xr3,blk,128,ptn1);
        S16LDD(xr3,blk,160,ptn0); //xr3:b64,b80

        Q16ADD_AS_WW(xr5,xr1,xr3,xr6);  //xr5:b0+b64(z0),b16+b80(z3)  xr6:b0-b64(z1),b16-b80(z2)
        S32SFL(xr7,xr5,xr6,xr8,ptn3);   //xr7:z0,z1, xr8:z3,z2
        Q16ADD_AS_WW(xr1,xr7,xr8,xr2); //xr1:z0+z3,z1+z2, xr2:z0-z3,z1-z2

        tp = tp + 2;
        S32ALNI(xr3,xr1,xr1,ptn2);
        S32STD(xr3,tp,0);
        S32STD(xr2,tp,4);
     }

     S32I2M(xr14,128);
     int pp[2]= {0,0};
     short *ppt;
     ppt = (short*)pp;
     tp = (int *)(((short*)temp1)-1);
     
     blk = block - 16;
     for (i=0; i<4; i++)
     {
        tp = (int *)(((short *)tp) + 1);
	S16LDD(xr1,tp,0,ptn1);
        S16LDD(xr1,tp,8,ptn0);  //xr1:t0,t4 
        S16LDD(xr2,tp,16,ptn1);  
        S16LDD(xr2,tp,24,ptn0); //xr2:t8,t12

        Q16ADD_AS_WW(xr3,xr1,xr2,xr4);  //xr3:t0+b8(z0),t4+t12(z3)  xr4:t0-t8(z1),t4-t12(z2)
        S32SFL(xr7,xr3,xr4,xr8,ptn3);   //xr7:z0,z1  xr8:z3,z2
        Q16ADD_AS_WW(xr1,xr7,xr8,xr2);  //xr1:z0+z3,z1+z2  xr2:z0-z3,z1-z2

        S16STD(xr1,ppt,0,ptn1);
        S16STD(xr1,ppt,2,ptn0);
        S16STD(xr2,ppt,4,ptn0);
        S16STD(xr2,ppt,6,ptn1);
                
        S32MUL(xr4,xr3,qmul,(int)ppt[0]);
        S32MUL(xr5,xr4,qmul,(int)ppt[1]);
        S32MUL(xr6,xr5,qmul,(int)ppt[2]);
        S32MUL(xr7,xr6,qmul,(int)ppt[3]);
        
        D32ADD_AA(xr1,xr3,xr14,xr1);    //xr1:(z0+z3)*qmul+128
        D32ADD_AA(xr2,xr4,xr14,xr2);
        D32ADD_AA(xr3,xr5,xr14,xr3);
        D32ADD_AA(xr4,xr6,xr14,xr4);

        D32SAR(xr1,xr1,xr2,xr2,8);
        D32SAR(xr3,xr3,xr4,xr4,8);
        
        if (i == 2)
	  blk = blk + 48;
        else
          blk = blk + 16;
        S16STD(xr1,blk,0,ptn0);
        S16STD(xr2,blk,64,ptn0);
        S16STD(xr3,blk,256,ptn0);
        S16STD(xr4,blk,320,ptn0);
    }
}

static av_always_inline void ff_h264_idct_copy_c(uint8_t *dst, DCTELEM *block){
    S32LDD(xr1,block,0);
    S32LDI(xr2,block,4);
    S32LDI(xr3,block,4);
    S32LDI(xr4,block,4);
    S32STD(xr1,dst,0);
    S32SDI(xr2,dst,PREVIOUS_LUMA_STRIDE);
    S32SDI(xr3,dst,PREVIOUS_LUMA_STRIDE);
    S32SDI(xr4,dst,PREVIOUS_LUMA_STRIDE);
}

static av_always_inline void ff_h264_ac_add_c(uint8_t *dst, DCTELEM *block, int stride){
    DCTELEM *blk = block;
    uint8_t *dst_mid = dst;
    int strd = stride;

    S32LDD(xr1, dst_mid, 0); //d3,d2,d1,d0
    S32LDD(xr2,blk,0); //b1,b0
    S32LDD(xr3,blk,4);   //b3,b2        
    Q8ACCE_AA(xr3,xr1,xr0,xr2);

    S32LDIV(xr11, dst_mid, strd, 0); //d3,d2,d1,d0
    S32LDI(xr12,blk,8); //b1,b0
    S32LDD(xr13,blk,4);   //b3,b2        
    Q16SAT(xr4,xr3,xr2);
    Q8ACCE_AA(xr13,xr11,xr0,xr12);

    S32LDIV(xr1, dst_mid, strd, 0); //d3,d2,d1,d0
    S32LDI(xr2,blk,8); //b1,b0
    S32LDD(xr3,blk,4);   //b3,b2        
    Q16SAT(xr5,xr13,xr12);
    Q8ACCE_AA(xr3,xr1,xr0,xr2);

    S32LDIV(xr11, dst_mid, strd, 0); //d3,d2,d1,d0
    S32LDI(xr12,blk,8); //b1,b0
    S32LDD(xr13,blk,4);   //b3,b2        
    Q16SAT(xr6,xr3,xr2);
    Q8ACCE_AA(xr13,xr11,xr0,xr12);

    S32STD(xr4,dst,0);
    S32SDIV(xr5, dst, strd, 0);
    Q16SAT(xr7,xr13,xr12);
    S32SDIV(xr6, dst, strd, 0); //d3,d2,d1,d0
    S32SDIV(xr7, dst, strd, 0); //d3,d2,d1,d0
}

#if 0
void chroma_dc_dequant_idct_mxu(DCTELEM *block, int qmul){
  short ppt[4];
  S32LDD(xr1,block,0); // xr1[31:16]:b; xr1[15:0]:a;
  S32LDD(xr2,block,4); // xr2[31:16]:d; xr1[15:0]:c;
  Q16ADD_AS_WW(xr3,xr1,xr2,xr4); //xr3[31:16]:(b+d), xr3[15:0]:(a+c); xr4[31:16]:(b-d), xr4[15:0]:(a-c);
  S32SFL(xr3,xr3,xr4,xr4,ptn3);  //xr3[31:16]:(b+d), xr3[15:0]:(b-d); xr4[31:16]:(a+c), xr4[15:0]:(a-c);
  Q16ADD_AS_WW(xr2,xr4,xr3,xr1); //xr2[31:16]:(a+c)+(b+d), xr2[15:0]:(a-c)+(b-d);
                                 //xr1[31:16]:(a+c)-(b+d), xr1[15:0]:(a-c)-(b-d);
  /* block[0] = (a+c) + (b+d);
     block[1] = (a+c) - (b+d);
     block[2] = (a-c) + (b-d);
     block[3] = (a-c) - (b-d); */
  S32STD(xr2,ppt,0);
  S32STD(xr1,ppt,4);
  S32MUL(xr2,xr1,qmul,(int)ppt[1]);
  S32MUL(xr3,xr2,qmul,(int)ppt[3]);
  S32MUL(xr4,xr3,qmul,(int)ppt[0]);
  S32MUL(xr5,xr4,qmul,(int)ppt[2]);
  D32SAR(xr1,xr1,xr2,xr2,7);
  D32SAR(xr3,xr3,xr4,xr4,7);
  S32SFL(xr0,xr2,xr1,xr1,ptn3);
  S32SFL(xr0,xr4,xr3,xr3,ptn3);
  S32STD(xr1,block,0);
  S32STD(xr3,block,4);
}
#endif

void luma_dc_dequant_idct_c(DCTELEM *block, int qmul){
    int i;
    int temp1[8];
    int *tp = temp1-2;
    DCTELEM *blk = block-32;
         
    for (i=0; i<4;i++) {
        if (i==2)
	  blk = blk + 96;
        else
          blk = blk + 32;
        S16LDD(xr1,blk,0,ptn1);
        S16LDD(xr1,blk,32,ptn0);  //xr1:b0,b16 
        S16LDD(xr3,blk,128,ptn1);
        S16LDD(xr3,blk,160,ptn0); //xr3:b64,b80

        Q16ADD_AS_WW(xr5,xr1,xr3,xr6);  //xr5:b0+b64(z0),b16+b80(z3)  xr6:b0-b64(z1),b16-b80(z2)
        S32SFL(xr7,xr5,xr6,xr8,ptn3);   //xr7:z0,z1, xr8:z3,z2
        Q16ADD_AS_WW(xr1,xr7,xr8,xr2); //xr1:z0+z3,z1+z2, xr2:z0-z3,z1-z2

        tp = tp + 2;
        S32ALNI(xr3,xr1,xr1,ptn2);
        S32STD(xr3,tp,0);
        S32STD(xr2,tp,4);
     }

     S32I2M(xr14,128);
     int pp[2]= {0,0};
     short *ppt;
     ppt = (short*)pp;
     tp = (int *)(((short*)temp1)-1);
     
     blk = block - 16;
     for (i=0; i<4; i++)
     {
        tp = (int *)(((short *)tp) + 1);
	S16LDD(xr1,tp,0,ptn1);
        S16LDD(xr1,tp,8,ptn0);  //xr1:t0,t4 
        S16LDD(xr2,tp,16,ptn1);  
        S16LDD(xr2,tp,24,ptn0); //xr2:t8,t12

        Q16ADD_AS_WW(xr3,xr1,xr2,xr4);  //xr3:t0+b8(z0),t4+t12(z3)  xr4:t0-t8(z1),t4-t12(z2)
        S32SFL(xr7,xr3,xr4,xr8,ptn3);   //xr7:z0,z1  xr8:z3,z2
        Q16ADD_AS_WW(xr1,xr7,xr8,xr2);  //xr1:z0+z3,z1+z2  xr2:z0-z3,z1-z2

        S16STD(xr1,ppt,0,ptn1);
        S16STD(xr1,ppt,2,ptn0);
        S16STD(xr2,ppt,4,ptn0);
        S16STD(xr2,ppt,6,ptn1);
                
        S32MUL(xr4,xr3,qmul,(int)ppt[0]);
        S32MUL(xr5,xr4,qmul,(int)ppt[1]);
        S32MUL(xr6,xr5,qmul,(int)ppt[2]);
        S32MUL(xr7,xr6,qmul,(int)ppt[3]);
        
        D32ADD_AA(xr1,xr3,xr14,xr1);    //xr1:(z0+z3)*qmul+128
        D32ADD_AA(xr2,xr4,xr14,xr2);
        D32ADD_AA(xr3,xr5,xr14,xr3);
        D32ADD_AA(xr4,xr6,xr14,xr4);

        D32SAR(xr1,xr1,xr2,xr2,8);
        D32SAR(xr3,xr3,xr4,xr4,8);
        
        if (i == 2)
	  blk = blk + 48;
        else
          blk = blk + 16;
        S16STD(xr1,blk,0,ptn0);
        S16STD(xr2,blk,64,ptn0);
        S16STD(xr3,blk,256,ptn0);
        S16STD(xr4,blk,320,ptn0);
    }
}


static const short block_offset[24]={
  //Y
  0,  4,  4*PREVIOUS_LUMA_STRIDE,   4*PREVIOUS_LUMA_STRIDE+4,
  8,  12, 4*PREVIOUS_LUMA_STRIDE+8, 4*PREVIOUS_LUMA_STRIDE+12,
  8*PREVIOUS_LUMA_STRIDE,           8*PREVIOUS_LUMA_STRIDE+4,
  12*PREVIOUS_LUMA_STRIDE,          12*PREVIOUS_LUMA_STRIDE+4,
  8*PREVIOUS_LUMA_STRIDE+8,         8*PREVIOUS_LUMA_STRIDE+12,
  12*PREVIOUS_LUMA_STRIDE+8,        12*PREVIOUS_LUMA_STRIDE+12,
  //U-V
  PREVIOUS_OFFSET_U,                PREVIOUS_OFFSET_U+4,
  PREVIOUS_OFFSET_U+4*PREVIOUS_CHROMA_STRIDE,   PREVIOUS_OFFSET_U+4*PREVIOUS_CHROMA_STRIDE+4,
  PREVIOUS_OFFSET_V,                PREVIOUS_OFFSET_V+4,
  PREVIOUS_OFFSET_V+4*PREVIOUS_CHROMA_STRIDE,   PREVIOUS_OFFSET_V+4*PREVIOUS_CHROMA_STRIDE+4,
};

void func_add_error(unsigned int error_add_cbp, unsigned char *recon_ptr, short *add_error_buf){
  int i;
  if (error_add_cbp & 0x10000000) {
    for (i = 0 ; i< 16; i++)
      ff_h264_idct_copy_c(recon_ptr+block_offset[i], add_error_buf+i*8);
  } else {
    for (i = 0 ; i< 16; i++){
      if(error_add_cbp & (1<<i))
	ff_h264_ac_add_c(recon_ptr+block_offset[i], add_error_buf+i*16, PREVIOUS_LUMA_STRIDE);
    }
  }

  for (i = 16 ; i< 24; i++){
    if(error_add_cbp & (1<<i))
      ff_h264_ac_add_c(recon_ptr+block_offset[i], add_error_buf+i*16, PREVIOUS_CHROMA_STRIDE);
  }
}

#if 0
typedef struct H264_VLC_RES{
  int     total_coeff;
  uint8_t *index;
  short   *coeff;
}H264_VLC_RES;

static H264_VLC_RES vlc_res_comp;

static void reproduce_blk_vlc(H264_VLC_RES *vlc_res, DCTELEM *block, uint8_t *scantable, int coeff_count){%
  uint8_t *index = vlc_res->index;
  short   *coeff = vlc_res->coeff;
  int i;
  for(i=0;i<coeff_count;i++) {
    block[scantable[index[i]]] = coeff[i];
  }
  vlc_res->index += coeff_count;
  vlc_res->coeff += coeff_count;
}
#endif

static inline unsigned int hw_idct_cavlc_config(H264_MB_Ctrl_DecARGs *dmb_A, short *add_error_buf0,
					  unsigned int *intra_top_y, unsigned char *recon_ptr2){
  int i;
  unsigned int error_add_cbp = 0;
  int cbp = dmb_A->cbp;
  int mb_type = dmb_A->mb_type;
  int is_16x16 = IS_INTRA16x16(mb_type);
  int is_8x8dct = IS_8x8DCT(mb_type);

  short *coeff_start = 0;//NULL
  
  if(cbp || is_16x16){
    H264_MB_Res *res = (H264_MB_Res *) ( (((unsigned int)(dmb_A) + dmb_A->ctrl_len + 3)) & (~0x3) );
    uint8_t *hw_nnz = res->dmb_nnz;
    coeff_start = res->mb;

    if (is_16x16) {
      luma_dc_dequant_idct_c_ori(coeff_start, dmb_A->dc_dequant4_coeff_Y);
      for( i = 0; i < 16; i++ ) {
	//int hw_nnz_num = hw_nnz[ scan4x4[i] ];
	//int hw_nnz_num = hw_nnz[ i ];
	if(hw_nnz[ scan4x4[i] ] | coeff_start[i*16]){
	  error_add_cbp |= (1 << i);
	}
      }
    } else {
#if 1
      int inter_dct_msk;
      int ddi;
      if (IS_8x8DCT(mb_type)) {
	ddi = 4;
	error_add_cbp |= 0x01000000;
	inter_dct_msk = 0xf;
      }else{
	ddi = 1;
	inter_dct_msk = 0x1;
      }

      for( i = 0; i < 16; i+=ddi ) {
	int hw_nnz_num = hw_nnz[ scan4x4[i] ];
	if(hw_nnz_num){
	  error_add_cbp |= (inter_dct_msk << i);
	}
      }
#else
      //error_add_cbp = res->error_add_cbp;
#endif
    }
    
    if( cbp&0x30 ){
      chroma_dc_dequant_idct_c_ori(coeff_start + 256, dmb_A->dc_dequant4_coeff_U);
      chroma_dc_dequant_idct_c_ori(coeff_start + 320, dmb_A->dc_dequant4_coeff_V);
      
      for(i=16; i<16+8; i++){
	int hw_nnz_num = hw_nnz[i];
	if(hw_nnz_num || coeff_start[i*16]){
	  error_add_cbp |= (1 << i);
	}
      }
    }
  }
  
  unsigned int * idct_chain_ptr= IDCT_DES_CHAIN;
  int idct_dha=TCSM1_PADDR(((int*)IDCT_DES_CHAIN)+19);

  if(IS_INTRA4x4(mb_type)){
    idct_dha=TCSM1_PADDR(IDCT_DES_CHAIN);
    H264_MB_Intra_DecARGs *INTRA_T=(H264_MB_Intra_DecARGs*)((unsigned)(dmb_A)+sizeof(struct H264_MB_Ctrl_DecARGs));
    uint8_t * top_ptr= intra_top_y+1;
    uint8_t * left_ptr = recon_ptr2+15;
#if 0
    *(idct_chain_ptr)++ = 0x80000000;
    *(idct_chain_ptr)++ = is_8x8dct ? INTRA_8x8_NOD_HEAD : INTRA_4x4_NOD_HEAD ;
    if(is_8x8dct){
      *(idct_chain_ptr)++ = ((unsigned int *)top_ptr)[5];
    }
#else
    idct_chain_ptr[2] = ((unsigned int *)top_ptr)[5];
#endif  
    idct_chain_ptr[3] = ((unsigned int *)top_ptr)[4];
    idct_chain_ptr[4] = left_ptr[12*PREVIOUS_LUMA_STRIDE] | (left_ptr[13*PREVIOUS_LUMA_STRIDE] << 8) | left_ptr[14*PREVIOUS_LUMA_STRIDE]<<16 | (left_ptr[15*PREVIOUS_LUMA_STRIDE] << 24);
    idct_chain_ptr[5] = ((unsigned int *)top_ptr)[3];
    idct_chain_ptr[6] = left_ptr[8*PREVIOUS_LUMA_STRIDE] | (left_ptr[9*PREVIOUS_LUMA_STRIDE] << 8) | left_ptr[10*PREVIOUS_LUMA_STRIDE]<<16 | (left_ptr[11*PREVIOUS_LUMA_STRIDE] << 24);
    idct_chain_ptr[7] = ((unsigned int *)top_ptr)[2];
    idct_chain_ptr[8] = left_ptr[4*PREVIOUS_LUMA_STRIDE] | (left_ptr[5*PREVIOUS_LUMA_STRIDE] << 8) | left_ptr[6*PREVIOUS_LUMA_STRIDE]<<16 | (left_ptr[7*PREVIOUS_LUMA_STRIDE] << 24);
    idct_chain_ptr[9] = ((unsigned int *)top_ptr)[1];
    idct_chain_ptr[10] = left_ptr[0*PREVIOUS_LUMA_STRIDE] | (left_ptr[1*PREVIOUS_LUMA_STRIDE] << 8) | left_ptr[2*PREVIOUS_LUMA_STRIDE]<<16 | (left_ptr[3*PREVIOUS_LUMA_STRIDE] << 24);
    idct_chain_ptr[11] = ((unsigned int *)top_ptr)[0];
    idct_chain_ptr[12] = INTRA_T->intra4x4_pred_mode1;
    idct_chain_ptr[13] = INTRA_T->intra4x4_pred_mode0;

    if(is_8x8dct){
      error_add_cbp |= 0x11000000;
      idct_chain_ptr[14] = (INTRA_T->topleft_samples_available) |
	(((INTRA_T->topright_samples_available) & 0x2) ? 1: 0) |
	(((INTRA_T->topright_samples_available) & 0x20) ? 2: 0) |
	(((INTRA_T->topright_samples_available) & 0x200) ? 4: 0) |
	(((INTRA_T->topright_samples_available) & 0x2000) ? 8: 0);
    }else{
      error_add_cbp |= 0x10000000;
      int dct_topright_valid = 0 ; 
      uint64_t pred_mode = (uint64_t)INTRA_T->intra4x4_pred_mode0 | ((uint64_t)INTRA_T->intra4x4_pred_mode1 << 32);
      for(i=0; i<16; i++){
	int dir = (pred_mode >> i*4) & 0xf;
	dct_topright_valid |= (dir == DIAG_DOWN_LEFT_PRED || dir == VERT_LEFT_PRED) ? (1 << i) : 0;
      }
      idct_chain_ptr[14] = dct_topright_valid & INTRA_T->topright_samples_available;
    }
    idct_chain_ptr[15] = (top_ptr[-1] & 0xFFFF) << 24 ;
    idct_chain_ptr[16] = error_add_cbp & 0xff00ffff;
    idct_chain_ptr[17] = TCSM1_PADDR(add_error_buf0);
    idct_chain_ptr[18] = TCSM1_PADDR(coeff_start);

    error_add_cbp &= 0xffff0000;
  }

#if 1
  idct_chain_ptr[21] = (error_add_cbp & (is_8x8dct ? 0xFFFF1111 : 0xFFFFFFFF))&(IS_INTRA4x4(mb_type)?0xefffffff:0xffffffff);
  idct_chain_ptr[22] = TCSM1_PADDR(add_error_buf0);
  idct_chain_ptr[23] = TCSM1_PADDR(coeff_start);

  set_idct_ddma_dha((TCSM1_PADDR(idct_dha)));
#else
  *(idct_chain_ptr)++ = 0x80000000;
  *(idct_chain_ptr)++ = INTER_NOD_HEAD;
  *(idct_chain_ptr)++ = (error_add_cbp & (is_8x8dct ? 0xFFFF1111 : 0xFFFFFFFF))&(IS_INTRA4x4(mb_type)?0xefffffff:0xffffffff);
  *(idct_chain_ptr)++ = TCSM1_PADDR(add_error_buf0);
  *(idct_chain_ptr)++ = TCSM1_PADDR(coeff_start);
  set_idct_ddma_dha((TCSM1_PADDR(IDCT_DES_CHAIN)));
#endif
  return error_add_cbp;
}

