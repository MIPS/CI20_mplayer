
#define MUL_12 0x00010002
#define MUL_34 0x00030004
#define MUL_56 0x00050006
#define MUL_78 0x00070008
#define MUX_H16 0x0000ffff

// for block 4x4
#define VERT_PRED             0
#define HOR_PRED              1
#define DC_PRED               2
#define DIAG_DOWN_LEFT_PRED   3
#define DIAG_DOWN_RIGHT_PRED  4
#define VERT_RIGHT_PRED       5
#define HOR_DOWN_PRED         6
#define VERT_LEFT_PRED        7
#define HOR_UP_PRED           8
#define LEFT_DC_PRED          9
#define TOP_DC_PRED           10
#define DC_128_PRED           11
// for block 8x8 and 16x16
#define DC_PRED8x8            0
#define HOR_PRED8x8           1
#define VERT_PRED8x8          2
#define PLANE_PRED8x8         3
#define LEFT_DC_PRED8x8       4
#define TOP_DC_PRED8x8        5
#define DC_128_PRED8x8        6


void Intra_pred_chroma(int chroma_pred_mode, uint8_t *dst, uint8_t *src, uint8_t *top)
{
  int i;

  uint8_t *src_top;  // top address
  uint8_t *src_topleft, *src_left;  // left address

  int pred_mode = chroma_pred_mode;

  src_left = src - 0x1;

  switch (pred_mode) {

  case VERT_PRED8x8:
    S32LDD(xr5, top, 0x0);
    S32LDD(xr6, top, 0x4);
    break;

  case HOR_PRED8x8:
    dst -= PREVIOUS_CHROMA_STRIDE;
    for (i=0; i<8; i++) {
      S8LDD(xr1, src_left, 0x0, ptn7);
      src_left = src_left + PREVIOUS_CHROMA_STRIDE;
      S32SDIV(xr1, dst, PREVIOUS_CHROMA_STRIDE, 0x0);
      S32STD(xr1, dst, 0x4);
    }
    break;

  case DC_PRED8x8:
    // load top
    S32LDD(xr11, top, 0x0);
    S32LDD(xr12, top, 0x4);
    // load left (4 x 7 = 28 instructions)
    S8LDD(xr7, src_left, 0x0, ptn0);
    S8LDI(xr7, src_left, PREVIOUS_CHROMA_STRIDE, ptn1);
    S8LDI(xr7, src_left, PREVIOUS_CHROMA_STRIDE, ptn2);
    S8LDI(xr7, src_left, PREVIOUS_CHROMA_STRIDE, ptn3);
    S8LDI(xr8, src_left, PREVIOUS_CHROMA_STRIDE, ptn0);
    S8LDI(xr8, src_left, PREVIOUS_CHROMA_STRIDE, ptn1);
    S8LDI(xr8, src_left, PREVIOUS_CHROMA_STRIDE, ptn2);
    S8LDI(xr8, src_left, PREVIOUS_CHROMA_STRIDE, ptn3);
    // AVG
    D8SUMC(xr1, xr11, xr7);
    Q16ADD_AA_XW(xr2, xr1, xr1, xr0);
    D32SLR(xr3, xr2, xr0, xr0, 0x3);
    S32SFL(xr4, xr3, xr3, xr0, ptn0);
    S32SFL(xr0, xr4, xr4, xr5, ptn3);

    D8SUMC(xr1, xr12, xr8);
    Q16ADD_AA_XW(xr2, xr1, xr1, xr0);
    D32SLR(xr3, xr2, xr0, xr0, 0x3);
    S32SFL(xr4, xr3, xr3, xr0, ptn0);
    S32SFL(xr0, xr4, xr4, xr6, ptn3);

    D32SLR(xr2, xr1, xr0, xr0, 0x2);
    S32SFL(xr3, xr2, xr2, xr4, ptn0);
    S32SFL(xr0, xr3, xr3, xr8, ptn3);
    S32SFL(xr0, xr4, xr4, xr9, ptn3);
    // store
    S32STD(xr5, dst, 0x0);
    S32STD(xr8, dst, 0x4);
    S32SDIV(xr5, dst, PREVIOUS_CHROMA_STRIDE, 0x0);
    S32STD(xr8, dst, 0x4);
    S32SDIV(xr5, dst, PREVIOUS_CHROMA_STRIDE, 0x0);
    S32STD(xr8, dst, 0x4);
    S32SDIV(xr5, dst, PREVIOUS_CHROMA_STRIDE, 0x0);
    S32STD(xr8, dst, 0x4);

    S32SDIV(xr9, dst, PREVIOUS_CHROMA_STRIDE, 0x0);
    S32STD(xr6, dst, 0x4);
    S32SDIV(xr9, dst, PREVIOUS_CHROMA_STRIDE, 0x0);
    S32STD(xr6, dst, 0x4);
    S32SDIV(xr9, dst, PREVIOUS_CHROMA_STRIDE, 0x0);
    S32STD(xr6, dst, 0x4);
    S32SDIV(xr9, dst, PREVIOUS_CHROMA_STRIDE, 0x0);
    S32STD(xr6, dst, 0x4);
    break;

  case PLANE_PRED8x8:
    src_top = top;
    src_topleft = src_top - 4;
    src_left = src - 0x4;

    //----- H, LOAD -----
    S32LDD(xr1, src_top, -4);  // xr1 <- src_top[-4];  xr1: lt, 0, 0, 0 ;
    S32LDD(xr3, src_top, 0x0);   // xr3 <- src_top[0] ;  xr3: t3, t2, t1, t0 ;
    S32LDDR(xr2, src_top, 0x4);  // xr2 <- src_top[4] ;  xr2: t4, t5, t6, t7 ;
    S32ALNI(xr1, xr3, xr1, ptn1);//                      xr1: t2, t1, t0, lt ;
    S32I2M(xr8, MUL_12); // xr8: 0x00010002 ;
    S32I2M(xr9, MUL_34); // xr9: 0x00030004 ;
    //----- H, SUM -----
    Q8ADDE_SS(xr3, xr2, xr1, xr4);  // xr3[31:16] <- t4-t2 ;  xr3[15:0] <- t5-t1 ;
    // xr4[31:16] <- t6-t0 ;  xr4[15:0] <- t7-lt;

    S32LDD(xr1, src_topleft, 0x0);          // xr1[31:24] <- src_topleft[3] (lt) ;

    D16MUL_WW(xr5, xr8, xr3, xr6);    // xr5 <- 1*(t4-t2) ;  xr6 <- 2*(t5-t1) ;
    D16MAC_AA_WW(xr5, xr9, xr4, xr6); // xr5 <- 1*(t4-t2)+3*(t6-t0) ; xr6 <- 2*(t5-t1)+4*(t7-lt) ;

    S32LDD(xr12, src_left, 0x0);//xr12[31:24] <- src_topleft[stride+3] (l0) ;
    S32LDIV(xr3, src_left, PREVIOUS_CHROMA_STRIDE, 0x0); // xr3[31:24] <- src_topleft[2*stride+3] (l1) ;

    D32ADD_AA(xr7, xr5, xr6, xr0); // xr7 <- 1*(t4-t2)+3*(t6-t0)+2*(t5-t1)+4*(t7-lt) ;
    //----- V, LOAD -----
    //  S32LDD(xr1, src_topleft, 0x0);          // xr1[31:24] <- src_topleft[3] (lt) ;
    //  S32LDIV(xr12, src_topleft, stride, 0x0);//xr12[31:24] <- src_topleft[stride+3] (l0) ;
    //  S32LDIV(xr3, src_topleft, stride, 0x0); // xr3[31:24] <- src_topleft[2*stride+3] (l1) ;
    S32LDIV(xr4, src_left, PREVIOUS_CHROMA_STRIDE, 0x0); // xr4[31:24] <- src_topleft[3*stride+3] (l2) ;
    S32SFL(xr5, xr12, xr1, xr0, ptn2);      // xr5[31:16] <- l0, lt ;
    S32SFL(xr6, xr4, xr3, xr0, ptn2);       // xr8[31:16] <- l2, l1 ;
    S32SFL(xr10, xr6, xr5, xr0, ptn3);      // xr10[31:0] <- l2, l1, l0, lt ;
    src_left += PREVIOUS_CHROMA_STRIDE;
    S32LDIV(xr4, src_left, PREVIOUS_CHROMA_STRIDE, 0x0);
    S32LDIV(xr3, src_left, PREVIOUS_CHROMA_STRIDE, 0x0);
    S32LDIV(xr12, src_left, PREVIOUS_CHROMA_STRIDE, 0x0);
    S32LDIV(xr1, src_left, PREVIOUS_CHROMA_STRIDE, 0x0);
    S32SFL(xr6, xr4, xr3, xr0, ptn2);
    S32SFL(xr5, xr12, xr1, xr0, ptn2);
    S32SFL(xr11, xr6, xr5, xr0, ptn3); // xr11[31:0] <- l4, l5, l6, l7 ;
    //----- V, SUM -----
    Q8ADDE_SS(xr3, xr11, xr10, xr4);

    S32LUI(xr1, 0x1, ptn0); // xr1[31:0]: 0x00000001 ;

    D16MUL_WW(xr5, xr8, xr3, xr6);
    D16MAC_AA_WW(xr5, xr9, xr4, xr6);

    D32ADD_AA(xr13, xr5, xr6, xr0); // xr13 <- 1*(l4-l2)+3*(l6-l0)+2*(l5-l1)+4*(l7-lt) ;

    //----- P, CAL ----- useful XRs:xr13, xr7, xr2, xr11;
    //  S32LUI(xr1, 0x1, ptn0); // xr1[31:0]: 0x00000001 ;
    D32SLL(xr5, xr1, xr1, xr6, 0x4); // xr5: 0x00000010;  xr6: 0x00000010; 
    D32SLL(xr3, xr13, xr7, xr4, 0x4);
    D32ACC_AA(xr5, xr13, xr3, xr0); // xr5: 17*V+16
    D32ACC_AA(xr6, xr7, xr4, xr0);  // xr6: 17*H+16

    Q8ACCE_AA(xr0, xr2, xr11, xr1);  // xr1[15:0]: src1[0] + src2[8] + 1

    D32SLR(xr8, xr5, xr6, xr9, 0x5); // xr8: (17*V+16) >> 5 ;  xr9: (17*H+16) >> 5 ;

    //  Q8ACCE_AA(xr0, xr2, xr11, xr1);  // xr1[15:0]: src1[0] + src2[8] + 1
    D32SLL(xr2, xr1, xr0, xr0, 0x4); // xr2[15:0]: 16*(src1[0] + src2[16] + 1)

    Q16ADD_AA_WW(xr7, xr8, xr9, xr0); // xr7: V+H
    S32I2M(xr4, MUX_H16); // xr4: 0x0000ffff ;
    D32SLL(xr12, xr7, xr0, xr0, 0x1);
    D32ADD_AA(xr5, xr12, xr7, xr0);   // xr5: 3*(V+H)
    //  S32LUI(xr12, 0x3, ptn0); // xr12[31:0]: 0x00000003 ;
    //  D16MUL_WW(xr0, xr7, xr12, xr5);   // xr5: 3*(V+H)

    //  S32I2M(xr4, MUX_H16); // xr4: 0x0000ffff ;

    Q16ADD_SS_WW(xr6, xr2, xr5, xr0); // xr6: 16*(src1[0] + src2[16] + 1) - 3*(V+H)

    //  S32I2M(xr4, MUX_H16); // xr4: 0x0000ffff ;

    S32SFL(xr0, xr8, xr8, xr14, ptn3);// xr14[31:16]: V ;  xr14[15:0]: V ;
    S32SFL(xr0, xr6, xr6, xr5, ptn3); // xr5[31:16]: a ;  xr5[15:0]: a ;
    D32SLL(xr7, xr9, xr0, xr0, 0x1);
    S32SFL(xr0, xr7, xr7, xr8, ptn3); // xr8[31:16]: 2H ;  xr8[15:0]: 2H ;

    //  S32I2M(xr4, MUX_H16); // xr4: 0x0000ffff ;
    S32AND(xr9, xr4, xr9);

    Q16ADD_AA_WW(xr15, xr5, xr9, xr0);   // xr15[31:16]: a ;  xr15[15:0]: a + H ;

    dst -= PREVIOUS_CHROMA_STRIDE;
    //----- SRC, STORE -----
    for (i=0; i<8; i++) {
      Q16ADD_AA_WW(xr1, xr15, xr8, xr0);
      Q16ADD_AA_WW(xr2, xr1, xr8, xr0);
      Q16SAR(xr9, xr15, xr1, xr1, 0x5);
      Q16ADD_AA_WW(xr3, xr2, xr8, xr0);

      Q16SAT(xr10, xr9, xr1);
      //    Q16SAR(xr9, xr15, xr1, xr1, 0x5);
      Q16SAR(xr2, xr2, xr3, xr3, 0x5);

      //    Q16SAT(xr10, xr9, xr1);
      Q16SAT(xr11, xr2, xr3);

      S32SDIVR(xr10, dst, PREVIOUS_CHROMA_STRIDE, 0x0);

      Q16ADD_AA_WW(xr15, xr15, xr14, xr0);

      S32STDR(xr11, dst, 0x4);
    }
    break;

  case LEFT_DC_PRED8x8:
    S8LDD(xr7, src_left, 0x0, ptn0);
    S8LDI(xr7, src_left, PREVIOUS_CHROMA_STRIDE, ptn1);
    S8LDI(xr7, src_left, PREVIOUS_CHROMA_STRIDE, ptn2);
    S8LDI(xr7, src_left, PREVIOUS_CHROMA_STRIDE, ptn3);
    S8LDI(xr8, src_left, PREVIOUS_CHROMA_STRIDE, ptn0);
    S8LDI(xr8, src_left, PREVIOUS_CHROMA_STRIDE, ptn1);
    S8LDI(xr8, src_left, PREVIOUS_CHROMA_STRIDE, ptn2);
    S8LDI(xr8, src_left, PREVIOUS_CHROMA_STRIDE, ptn3);
    // AVG
    D8SUMC(xr1, xr7, xr8);
    D32SLR(xr2, xr1, xr0, xr0, 0x2);
    S32SFL(xr3, xr2, xr2, xr4, ptn0);
    S32SFL(xr0, xr3, xr3, xr5, ptn3);
    S32SFL(xr0, xr4, xr4, xr6, ptn3);
    // store
    S32STD(xr5, dst, 0x0);
    S32STD(xr5, dst, 0x4);
    S32SDIV(xr5, dst, PREVIOUS_CHROMA_STRIDE, 0x0);
    S32STD(xr5, dst, 0x4);
    S32SDIV(xr5, dst, PREVIOUS_CHROMA_STRIDE, 0x0);
    S32STD(xr5, dst, 0x4);
    S32SDIV(xr5, dst, PREVIOUS_CHROMA_STRIDE, 0x0);
    S32STD(xr5, dst, 0x4);
    S32SDIV(xr6, dst, PREVIOUS_CHROMA_STRIDE, 0x0);
    S32STD(xr6, dst, 0x4);
    S32SDIV(xr6, dst, PREVIOUS_CHROMA_STRIDE, 0x0);
    S32STD(xr6, dst, 0x4);
    S32SDIV(xr6, dst, PREVIOUS_CHROMA_STRIDE, 0x0);
    S32STD(xr6, dst, 0x4);
    S32SDIV(xr6, dst, PREVIOUS_CHROMA_STRIDE, 0x0);
    S32STD(xr6, dst, 0x4);
    break;

  case TOP_DC_PRED8x8:
    src_top = top;
    // load top
    S32LDD(xr7, src_top, 0x0);
    S32LDD(xr8, src_top, 0x4);
    // AVG
    D8SUMC(xr1, xr7, xr8);
    D32SLR(xr2, xr1, xr0, xr0, 0x2);
    S32SFL(xr3, xr2, xr2, xr4, ptn0);
    S32SFL(xr0, xr3, xr3, xr5, ptn3);
    S32SFL(xr0, xr4, xr4, xr6, ptn3);
    break;

  case DC_128_PRED8x8:
    //load
    S32LUI(xr5, 0x80, ptn7);
    S32OR(xr6, xr5, xr0);
    break;

  default:
    break;

  }

  if ( (pred_mode == VERT_PRED8x8) ||
       (pred_mode == DC_128_PRED8x8) ||
       (pred_mode == TOP_DC_PRED8x8) ) {

    dst -= PREVIOUS_CHROMA_STRIDE;
    for(i=0; i<8; i++){
      S32SDIV(xr5, dst, PREVIOUS_CHROMA_STRIDE, 0x0);
      S32STD(xr6, dst, 0x4);
    }
  }

}

void load_left_16x16_mxu(uint8_t * src_left)
{
  S8LDD(xr7, src_left, 0x0, ptn0);
  S8LDI(xr7, src_left, PREVIOUS_LUMA_STRIDE, ptn1);
  S8LDI(xr7, src_left, PREVIOUS_LUMA_STRIDE, ptn2);
  S8LDI(xr7, src_left, PREVIOUS_LUMA_STRIDE, ptn3);

  S8LDI(xr8, src_left, PREVIOUS_LUMA_STRIDE, ptn0);
  S8LDI(xr8, src_left, PREVIOUS_LUMA_STRIDE, ptn1);
  S8LDI(xr8, src_left, PREVIOUS_LUMA_STRIDE, ptn2);
  S8LDI(xr8, src_left, PREVIOUS_LUMA_STRIDE, ptn3);

  S8LDI(xr9, src_left, PREVIOUS_LUMA_STRIDE, ptn0);
  S8LDI(xr9, src_left, PREVIOUS_LUMA_STRIDE, ptn1);
  S8LDI(xr9, src_left, PREVIOUS_LUMA_STRIDE, ptn2);
  S8LDI(xr9, src_left, PREVIOUS_LUMA_STRIDE, ptn3);

  S8LDI(xr10, src_left, PREVIOUS_LUMA_STRIDE, ptn0);
  S8LDI(xr10, src_left, PREVIOUS_LUMA_STRIDE, ptn1);
  S8LDI(xr10, src_left, PREVIOUS_LUMA_STRIDE, ptn2);
  S8LDI(xr10, src_left, PREVIOUS_LUMA_STRIDE, ptn3);

}

void Intra_pred_luma_16x16(int luma_16x16_pred_mode, uint8_t *dst, uint8_t *src, uint8_t *top, H264_MB_Ctrl_DecARGs *dmb_A)
{
  uint8_t *src_top;  // top address
  uint8_t *src_left;  // left address
  uint8_t *src_topleft;  // left address
  unsigned int i;

  int pred_mode = luma_16x16_pred_mode;

  src_left = src - 1;


  switch (pred_mode) {

  case DC_PRED8x8:
    src_top = top;
     // load top
    S32LDD(xr11, src_top, 0x0);
    S32LDD(xr12, src_top, 0x4);
    S32LDD(xr13, src_top, 0x8);
    S32LDD(xr14, src_top, 0xc);
    // load left
    load_left_16x16_mxu(src_left);
    // AVG
    D8SUMC(xr1, xr11, xr12);
    D8SUMC(xr2, xr13, xr14);
    D8SUMC(xr3, xr7, xr8);
    D8SUMC(xr4, xr9, xr10);
    Q16ADD_AA_WW(xr5, xr1, xr2, xr0);
    Q16ACC_AA(xr5, xr3, xr4, xr0);
    Q16ADD_AA_XW(xr7, xr5, xr5, xr0);
    D32SLR(xr8, xr7, xr0, xr0, 0x5);
    S32SFL(xr9, xr8, xr8, xr0, ptn0);
    S32SFL(xr0, xr9, xr9, xr1, ptn3);
    break;

  case VERT_PRED8x8:
    src_top = top;
    //load
    S32LDD(xr1, src_top, 0x0);
    S32LDD(xr2, src_top, 0x4);
    S32LDD(xr3, src_top, 0x8);
    S32LDD(xr4, src_top, 0xc);
    //store
    dst -= PREVIOUS_LUMA_STRIDE;
    for (i=0; i<16; i++) {
      S32SDIV(xr1, dst, PREVIOUS_LUMA_STRIDE,0x0);
      S32STD(xr2, dst, 0x4);
      S32STD(xr3, dst, 0x8);
      S32STD(xr4, dst, 0xc);
    }
    break;

  case HOR_PRED8x8:
    src_left = src - 0x1;
    dst -= PREVIOUS_LUMA_STRIDE;
    for (i=0; i<16; i++) {
      S8LDD(xr1, src_left, 0x0, ptn7);
      src_left = src_left + PREVIOUS_LUMA_STRIDE;
      S32SDIV(xr1, dst, PREVIOUS_LUMA_STRIDE, 0x0);
      S32STD(xr1, dst, 0x4);
      S32STD(xr1, dst, 0x8);
      S32STD(xr1, dst, 0xc);
    }
    break;

  case PLANE_PRED8x8:
    src_top = top;
    src_topleft = src_top - 4;
    src_left = src - 0x4;

    //----- H, LOAD -----
    S32LDD(xr1, src_top, -4);  // xr1 <- src_top[-4];  xr1: lt, 0, 0, 0 ;
    S32LDD(xr5, src_top, 0x0);   // xr5 <- src_top[0] ;  xr5: t3, t2, t1, t0 ;
    S32LDD(xr2, src_top, 0x4);   // xr2 <- src_top[4] ;  xr2: t7, t6, t5, t4 ;
    S32LDDR(xr3, src_top, 0x8);  // xr3 <- src_top[8] ;  xr3: t8, t9, t10, t11 ;
    S32LDDR(xr4, src_top, 0xc);  // xr4 <- src_top[12];  xr4: t12, t13, t14, t15 ;

    S32ALNI(xr1, xr5, xr1, ptn1);  //                    xr1: t2, t1, t0, lt ;
    S32ALNI(xr2, xr2, xr5, ptn1);  //                    xr2: t6, t5, t4, t3 ;   ---xr5 is free to use ;
    S32I2M(xr9, MUL_12);  // xr9 : 0x00010002 ;
    S32I2M(xr10, MUL_34); // xr10: 0x00030004 ;
    //----- H, SUM -----
    Q8ADDE_SS(xr5, xr3, xr2, xr6);  // xr5[31:16] <- t8-t6 ;  xr5[15:0] <- t9-t5 ;
    // xr6[31:16] <- t10-t4;  xr6[15:0] <- t11-t3;

    S32I2M(xr11, MUL_56); // xr11: 0x00050006 ;

    D16MUL_WW(xr13, xr9, xr5, xr14);     // xr13 <- 1*(t8-t6) ;  xr14 <- 2*(t9-t5) ;
    D16MAC_AA_WW(xr13, xr10, xr6, xr14); // xr13 <- 1*(t8-t6)+3*(t10-t4) ; xr14 <- 2*(t9-t5)+4*(t11-t3) ;
    Q8ADDE_SS(xr5, xr4, xr1, xr6);  // xr5[31:16] <- t12-t2;  xr5[15:0] <- t13-t1;
    // xr6[31:16] <- t14-t0;  xr6[15:0] <- t15-lt;

    S32I2M(xr12, MUL_78); // xr12: 0x00070008 ;

    D16MAC_AA_WW(xr13, xr11, xr5, xr14); // xr13 <- 1*(t8-t6)+3*(t10-t4)+5*(t12-t2) ;
    // xr14 <- 2*(t9-t5)+4*(t11-t3)+6*(t13-t1) ;
    D16MAC_AA_WW(xr13, xr12, xr6, xr14); // xr13 <- 1*(t8-t6)+3*(t10-t4)+5*(t12-t2)+7*(t14-t0) ;
    // xr14 <- 2*(t9-t5)+4*(t11-t3)+6*(t13-t1)+8*(t15-lt) ;
    S32LDD(xr1, src_topleft, 0x0);          // xr1[31:24] <- src_topleft[3] (lt) ;
    S32LDD(xr2, src_left, 0x0); // xr2[31:24] <- src_topleft[stride+3] (l0) ;
    D32ADD_AA(xr15, xr13, xr14, xr0); // xr15 <- 1*(t8-t6)+3*(t10-t4)+5*(t12-t2)+7*(t14-t0)
    //       + 2*(t9-t5)+4*(t11-t3)+6*(t13-t1)+8*(t15-lt) ;
    //----- V, LOAD -----
    //  S32LDD(xr1, src_topleft, 0x0);          // xr1[31:24] <- src_topleft[3] (lt) ;
    //  S32LDIV(xr2, src_topleft, stride, 0x0); // xr2[31:24] <- src_topleft[stride+3] (l0) ;
    S32LDIV(xr3, src_left, PREVIOUS_LUMA_STRIDE, 0x0); // xr3[31:24] <- src_topleft[2*stride+3] (l1) ;
    S32LDIV(xr8, src_left, PREVIOUS_LUMA_STRIDE, 0x0); // xr9[31:24] <- src_topleft[3*stride+3] (l2) ;
    S32SFL(xr5, xr2, xr1, xr0, ptn2);       // xr5[31:16] <- l0, lt ;
    S32SFL(xr6, xr8, xr3, xr0, ptn2);       // xr8[31:16] <- l2, l1 ;
    S32SFL(xr7, xr6, xr5, xr0, ptn3);       // xr7[31: 0] <- l2, l1, l0, lt ;

    S32LDIV(xr1, src_left, PREVIOUS_LUMA_STRIDE, 0x0);
    S32LDIV(xr2, src_left, PREVIOUS_LUMA_STRIDE, 0x0);
    S32LDIV(xr3, src_left, PREVIOUS_LUMA_STRIDE, 0x0);
    S32LDIV(xr8, src_left, PREVIOUS_LUMA_STRIDE, 0x0);
    S32SFL(xr5, xr2, xr1, xr0, ptn2);
    S32SFL(xr6, xr8, xr3, xr0, ptn2);
    S32SFL(xr13, xr6, xr5, xr0, ptn3); // xr13[31:0] <- l6, l5, l4, l3 ;

    src_left += PREVIOUS_LUMA_STRIDE;

    S32LDIV(xr8, src_left, PREVIOUS_LUMA_STRIDE, 0x0);
    S32LDIV(xr3, src_left, PREVIOUS_LUMA_STRIDE, 0x0);
    S32LDIV(xr2, src_left, PREVIOUS_LUMA_STRIDE, 0x0);
    S32LDIV(xr1, src_left, PREVIOUS_LUMA_STRIDE, 0x0);
    S32SFL(xr6, xr8, xr3, xr0, ptn2);
    S32SFL(xr5, xr2, xr1, xr0, ptn2);
    S32SFL(xr14, xr6, xr5, xr0, ptn3); // xr14[31:0] <- l8, l9, l10, l11 ;

    S32LDIV(xr8, src_left, PREVIOUS_LUMA_STRIDE, 0x0);
    S32LDIV(xr3, src_left, PREVIOUS_LUMA_STRIDE, 0x0);
    S32LDIV(xr2, src_left, PREVIOUS_LUMA_STRIDE, 0x0);
    S32LDIV(xr1, src_left, PREVIOUS_LUMA_STRIDE, 0x0);
    S32SFL(xr6, xr8, xr3, xr0, ptn2);
    S32SFL(xr5, xr2, xr1, xr0, ptn2);
    S32SFL(xr1, xr6, xr5, xr0, ptn3); // xr1[31: 0] <- l12, l13, l14, l15 ;

    //----- V, SUM -----
    Q8ADDE_SS(xr5, xr14, xr13, xr6);
    Q8ADDE_SS(xr2, xr1, xr7, xr3);

    D16MUL_WW(xr13, xr9, xr5, xr14);
    D16MAC_AA_WW(xr13, xr10, xr6, xr14);

    D16MAC_AA_WW(xr13, xr11, xr2, xr14);
    D16MAC_AA_WW(xr13, xr12, xr3, xr14);

    D32SLR(xr2, xr11, xr12, xr3, 0x8); // xr2: 0x00000500 ;  xr3: 0x00000700 ;
    D32SLR(xr11, xr2, xr3, xr12, 0x8); //xr11: 0x00000005 ; xr12: 0x00000007 ;

    D32ADD_AA(xr14, xr13, xr14, xr0); // xr14 <- 1*(l8-l6)+3*(l10-l4)+5*(l12-l2)+7*(l14-l0)
    //       + 2*(l9-l5)+4*(l11-l3)+6*(l13-l1)+8*(l15-lt) ;
    //----- P, CAL -----
    //  D32SLR(xr2, xr11, xr12, xr3, 0x8); // xr2: 0x00000500 ;  xr3: 0x00000700 ;
    //  D32SLR(xr11, xr2, xr3, xr12, 0x8); //xr11: 0x00000005 ; xr12: 0x00000007 ;

    D16MUL_WW(xr0, xr15, xr11, xr2); // xr2: 5*H ;
    D16MUL_WW(xr0, xr14, xr11, xr3); // xr3: 5*V ;

    D32SLR(xr8, xr11, xr0, xr0, 0x2); // xr8: 0x00000001 ;
    D32SLL(xr13, xr8, xr0, xr0, 0x5); //xr13: 0x00000020 ;

    Q8ACCE_AA(xr0, xr1, xr4, xr8);   // xr8[15:0]: src1[0] + src2[16] + 1

    D32ADD_AA(xr5, xr2, xr13, xr0); // xr5: 5*H+32 ;
    D32ADD_AA(xr6, xr3, xr13, xr0); // xr6: 5*V+32 ;

    D32SLR(xr2, xr5, xr6, xr3, 0x6); // xr2: ( 5*H+32 ) >> 6 ;  xr3: ( 5*V+32 ) >> 6 ;

    //  Q8ACCE_AA(xr0, xr1, xr4, xr8);   // xr8[15:0]: src1[0] + src2[16] + 1
    D32SLL(xr5, xr8, xr0, xr0, 0x4); // xr5[15:0]: 16*(src1[0] + src2[16] + 1)

    Q16ADD_AA_WW(xr7, xr2, xr3, xr0); // xr7: V+H
    //  S32NOR(xr0, xr0, xr0); // idle
    S32I2M(xr4, MUX_H16); // xr4: 0x0000ffff ;
    D16MUL_WW(xr0, xr7, xr12, xr8);   // xr8: 7*(V+H)

    S32SFL(xr0, xr3, xr3, xr14, ptn3); // xr14[31:16]: V ;  xr14[15:0]: V ;
    D32SLL(xr7, xr2, xr0, xr0, 0x1);

    Q16ADD_SS_WW(xr9, xr5, xr8, xr0); // xr9: 16*(src1[0] + src2[16] + 1) - 7*(V+H)
    S32SFL(xr0, xr9, xr9, xr5, ptn3); // xr5[31:16]: a ;  xr5[15:0]: a ;
    //  S32SFL(xr0, xr3, xr3, xr14, ptn3); // xr14[31:16]: V ;  xr14[15:0]: V ;
    //  D32SLL(xr7, xr2, xr0, xr0, 0x1);
    S32SFL(xr0, xr7, xr7, xr8, ptn3);  // xr8[31:16]: 2H ;  xr8[15:0]: 2H ;

    S32AND(xr2, xr4, xr2);

    Q16ADD_AA_WW(xr15, xr5, xr2, xr0); // xr15[31:16]: a ;  xr15[15:0]: a + H ;

    dst -= PREVIOUS_LUMA_STRIDE;
    //----- SRC, STORE -----
    for (i=0; i<16; i++) {
      Q16ADD_AA_WW(xr1, xr15, xr8, xr0);
      Q16ADD_AA_WW(xr2, xr1, xr8, xr0);
      Q16SAR(xr9, xr15, xr1, xr1, 0x5);
      Q16ADD_AA_WW(xr3, xr2, xr8, xr0);
      Q16SAT(xr10, xr9, xr1);
      Q16ADD_AA_WW(xr4, xr3, xr8, xr0);
      Q16SAR(xr2, xr2, xr3, xr3, 0x5);
      Q16ADD_AA_WW(xr5, xr4, xr8, xr0);
      Q16SAT(xr11, xr2, xr3);
      Q16ADD_AA_WW(xr6, xr5, xr8, xr0);
      Q16SAR(xr4, xr4, xr5, xr5, 0x5);
      Q16ADD_AA_WW(xr7, xr6, xr8, xr0);
      Q16SAR(xr6, xr6, xr7, xr7, 0x5);
      Q16SAT(xr12, xr4, xr5);
      Q16SAT(xr13, xr6, xr7);

      S32SDIVR(xr10, dst, PREVIOUS_LUMA_STRIDE, 0x0);
      S32STDR(xr11, dst, 0x4);
      S32STDR(xr12, dst, 0x8);
      //    S32STDR(xr13, dst, 0xc);

      Q16ADD_AA_WW(xr15, xr15, xr14, xr0);

      S32STDR(xr13, dst, 0xc);
    }
    break;

  case LEFT_DC_PRED8x8:
    // load left 
    load_left_16x16_mxu(src_left);
    // AVG
    D8SUMC(xr1, xr7, xr8);
    D8SUMC(xr2, xr9, xr10);
    Q16ADD_AA_WW(xr5, xr1, xr2, xr0);
    Q16ADD_AA_XW(xr7, xr5, xr5, xr0);
    D32SLR(xr8, xr7, xr0, xr0, 0x4);
    S32SFL(xr9, xr8, xr8, xr0, ptn0);
    S32SFL(xr0, xr9, xr9, xr1, ptn3);
    break;

  case TOP_DC_PRED8x8:
    src_top = top;
    // load top
    S32LDD(xr1, src_top, 0x0);
    S32LDD(xr2, src_top, 0x4);
    S32LDD(xr3, src_top, 0x8);
    S32LDD(xr4, src_top, 0xc);
    // AVG
    D8SUMC(xr1, xr1, xr2);
    D8SUMC(xr2, xr3, xr4);
    Q16ADD_AA_WW(xr5, xr1, xr2, xr0);
    Q16ADD_AA_XW(xr7, xr5, xr5, xr0);
    D32SLR(xr8, xr7, xr0, xr0, 0x4);
    S32SFL(xr9, xr8, xr8, xr0, ptn0);
    S32SFL(xr0, xr9, xr9, xr1, ptn3);
    break;

  case DC_128_PRED8x8:
    //load
    S32LUI(xr1, 0x80, ptn7);
    break;

  default:
    break;

  }

  if ( ( pred_mode == DC_PRED8x8 ) ||
       ( pred_mode == LEFT_DC_PRED8x8 ) ||
       ( pred_mode == TOP_DC_PRED8x8 ) ||
       ( pred_mode == DC_128_PRED8x8 ) ) {

    // store
    dst -= PREVIOUS_LUMA_STRIDE;
    for(i=0; i<16; i++){
      S32SDIV(xr1, dst, PREVIOUS_LUMA_STRIDE, 0x0);
      S32STD(xr1, dst, 0x4);
      S32STD(xr1, dst, 0x8);
      S32STD(xr1, dst, 0xc);
    }
  }

}
