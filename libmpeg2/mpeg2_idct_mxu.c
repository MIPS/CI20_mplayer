
#define  wxr5  0x5A827642
#define  wxr6  0x5A8230FC
#define  wxr7  0x7D876A6E
#define  wxr8  0x18F9471D
#define  wxr9  0x6A6E18F9
#define  wxr10  0x471D7D87

static int32_t whirl_idct[6] = {wxr5, wxr6, wxr7, wxr8, wxr9, wxr10};
static void mpeg2_idct_copy_mxu (int16_t * block, uint8_t * dest, const int stride, uint8_t idct_row)
{
  int i, tmp0, tmp1;
  int16_t  *blk;
  int32_t wf = (int32_t)whirl_idct;

  S32LDD(xr5, wf, 0x0);         // xr5(w7, w3)
  S32LDD(xr6, wf, 0x4);         // xr6(w9, w8)
  S32LDD(xr7, wf, 0x8);         // xr7(w11,w10)
  S32LDD(xr8, wf, 0xc);         // xr8(w13,w12)
  S32LDD(xr9, wf, 0x10);        // xr9(w6, w0)
  S32LDD(xr10,wf, 0x14);

  blk = block - 8;
  for (i = 0; i < idct_row; i++)       /* idct rows */
  {
      S32LDI(xr1, blk, 0x10);       //  xr1 (x4, x0)
      tmp0 = S32M2I(xr1);
      S32LDD(xr2, blk, 0x4);        //  xr2 (x7, x3)
      S32LDD(xr3, blk, 0x8);        //  xr3 (x6, x1)
      S32LDD(xr4, blk, 0xc);        //  xr4 (x5, x2)
      // shortcut
      S32SFL(xr0, xr1, xr1, xr11, ptn3);
      D16MUL_HW(xr11, xr5, xr11, xr12);
      D16MACF_AA_WW(xr11, xr0, xr0, xr12);
      tmp0 >>= 16;

      S32OR(xr12,xr2,xr3);
      S32OR(xr12,xr12,xr4);
      tmp1 = (S32M2I(xr12));
      tmp0 |= tmp1;

      if (tmp0 == 0) {
        S32STD(xr11, blk, 0x0);
        S32STD(xr11, blk, 0x4);
        S32STD(xr11, blk, 0x8);
        S32STD(xr11, blk, 0xc);
        continue;
      }
      S32SFL(xr1,xr1,xr2,xr2, ptn3);  //xr1:s1, s3, xr2: s0, s2
      S32SFL(xr3,xr3,xr4,xr4, ptn3);  //xr3:s5, s7, xr4: s4, s6

      D16MUL_WW(xr11, xr2, xr5, xr12);//xr11: s0*c4, xr12: s2*c2
      D16MAC_AA_WW(xr11,xr4,xr6,xr12);//xr11: s0*c4+s4*c4, xr12: s2*c2+s6*c6

      D16MUL_WW(xr13, xr2, xr6, xr14);//xr13: s0*c4, xr14: s2*c6
      D16MAC_SS_WW(xr13,xr4,xr5,xr14);//xr13: s0*c4 - s4*c4, xr14: s2*c6-s6*c2

      D16MUL_HW(xr2, xr1, xr7, xr4);  //xr2: s1*c1, xr4: s1*c3 
      D16MAC_AS_LW(xr2,xr1,xr9,xr4);  //xr2: s1*c1+s3*c3, xr4: s1*c3-s3*c7
      D16MAC_AS_HW(xr2,xr3,xr10,xr4); //xr2: s1*c1+s3*c3+s5*c5, xr4: s1*c3-s3*c7-s5*c1
      D16MAC_AS_LW(xr2,xr3,xr8,xr4);  //xr2: s1*c1+s3*c3+s5*c5+s7*c7,
                                      //xr4: s1*c3-s3*c7-s5*c1-s7*c5
      D16MACF_AA_WW(xr2, xr0, xr0, xr4); //xr2: rnd(s1*c1+s3*c3+s5*c5+s7*c7)>>16
                                         //   : rnd(s1*c3-s3*c7-s5*c1-s7*c5)>>16

      D16MACF_AA_WW(xr11, xr0, xr0, xr13);//xr11: rnd(s0*c4+s4*c4)>>16, rnd(s0*c4 - s4*c4)>>16
      D16MACF_AA_WW(xr12, xr0, xr0, xr14);//xr12: rnd(s2*c2+s6*c6)>>16, rnd(s2*c6-s6*c2)>>16

      D16MUL_HW(xr4, xr1, xr8, xr15);     //xr4: s1*c7, xr15:s1*c5
      D16MAC_SS_LW(xr4,xr1,xr10,xr15);    //xr4: s1*c7-s3*c5, xr15: s1*c5-s3*c1
      D16MAC_AA_HW(xr4,xr3,xr9,xr15);     //xr4: s1*c7-s3*c5+s5*c3, xr15: s1*c5-s3*c1+s5*c7
      D16MAC_SA_LW(xr4,xr3,xr7,xr15);     //xr4: s1*c7-s3*c5+s5*c3-s7*c1
                                          //xr15: s1*c5-s3*c1+s5*c7+s7*c3
      Q16ADD_AS_WW(xr11,xr11,xr12,xr12);  //xr11: rnd(s0*c4+s4*c4)>>16+rnd(s2*c2+s6*c6)>>16
                                          //      rnd(s0*c4-s4*c4)>>16+rnd(s2*c6-s6*c2)>>16
                                          //xr12: rnd(s0*c4+s4*c4)>>16-rnd(s2*c2+s6*c6)>>16
                                          //      rnd(s0*c4-s4*c4)>>16-rnd(s2*c6-s6*c2)>>16
      D16MACF_AA_WW(xr15, xr0, xr0, xr4); //xr15: rnd(s1*c5-s3*c1+s5*c7+s7*c3)>>16
                                          //    : rnd(s1*c7-s3*c5+s5*c3-s7*c1)>>16

      Q16ADD_AS_WW(xr11, xr11, xr2, xr2);
              //xr11: rnd(s0*c4+s4*c4)>>16+rnd(s2*c2+s6*c6)>>16 + rnd(s1*c1+s3*c3+s5*c5+s7*c7)>>16
              //    : rnd(s0*c4-s4*c4)>>16+rnd(s2*c6-s6*c2)>>16 + rnd(s1*c3-s3*c7-s5*c1-s7*c5)>>16
              //xr2: rnd(s0*c4+s4*c4)>>16+rnd(s2*c2+s6*c6)>>16 - rnd(s1*c1+s3*c3+s5*c5+s7*c7)>>16
              //   : rnd(s0*c4-s4*c4)>>16+rnd(s2*c6-s6*c2)>>16 - rnd(s1*c3-s3*c7-s5*c1-s7*c5)>>16

      Q16ADD_AS_XW(xr12, xr12, xr15, xr15);
              //xr12: rnd(s0*c4+s4*c4)>>16-rnd(s2*c2+s6*c6)>>16+rnd(s1*c5-s3*c1+s5*c7+s7*c3)>>16
              //    : rnd(s0*c4-s4*c4)>>16+rnd(s2*c6-s6*c2)>>16+rnd(s1*c7-s3*c5+s5*c3-s7*c1)>>16
              //xr15: rnd(s0*c4+s4*c4)>>16-rnd(s2*c2+s6*c6)>>16-rnd(s1*c5-s3*c1+s5*c7+s7*c3)>>16
              //    : rnd(s0*c4-s4*c4)>>16+rnd(s2*c6-s6*c2)>>16-rnd(s1*c7-s3*c5+s5*c3-s7*c1)>>16

      S32SFL(xr11,xr11,xr12,xr12, ptn3);
              //xr11: rnd(s0*c4+s4*c4)>>16+rnd(s2*c2+s6*c6)>>16 + rnd(s1*c1+s3*c3+s5*c5+s7*c7)>>16
              //    : rnd(s0*c4+s4*c4)>>16-rnd(s2*c2+s6*c6)>>16+rnd(s1*c5-s3*c1+s5*c7+s7*c3)>>16
              //xr12: rnd(s0*c4-s4*c4)>>16+rnd(s2*c6-s6*c2)>>16 + rnd(s1*c3-s3*c7-s5*c1-s7*c5)>>16
              //    : rnd(s0*c4-s4*c4)>>16+rnd(s2*c6-s6*c2)>>16+rnd(s1*c7-s3*c5+s5*c3-s7*c1)>>16
      S32SFL(xr12,xr12,xr11,xr11, ptn3);
              //xr12: rnd(s0*c4-s4*c4)>>16+rnd(s2*c6-s6*c2)>>16 + rnd(s1*c3-s3*c7-s5*c1-s7*c5)>>16
              //    : rnd(s0*c4+s4*c4)>>16+rnd(s2*c2+s6*c6)>>16 + rnd(s1*c1+s3*c3+s5*c5+s7*c7)>>16
              //xr11: rnd(s0*c4-s4*c4)>>16+rnd(s2*c6-s6*c2)>>16+rnd(s1*c7-s3*c5+s5*c3-s7*c1)>>16
              //    : rnd(s0*c4+s4*c4)>>16-rnd(s2*c2+s6*c6)>>16+rnd(s1*c5-s3*c1+s5*c7+s7*c3)>>16

      S32STD(xr12, blk, 0x0);
      S32STD(xr11, blk, 0x4);
      S32STD(xr15, blk, 0x8);
      S32STD(xr2, blk, 0xc);
    }

  blk  = block - 2;
  for (i = 0; i < 4; i++)               /* idct columns */
    {
      S32LDI(xr1, blk, 0x4);   //xr1: ss0, s0
      S32LDD(xr3, blk, 0x20);  //xr3: ss2, s2
      S32I2M(xr5,wxr5);        //xr5: c4 , c2
      S32LDD(xr11, blk, 0x40); //xr11: ss4, s4
      S32LDD(xr13, blk, 0x60); //xr13: ss6, s6

      D16MUL_HW(xr15, xr5, xr1, xr9);    //xr15: ss0*c4, xr9: s0*c4
      D16MAC_AA_HW(xr15,xr5,xr11,xr9);   //xr15: ss0*c4+ss4*c4, xr9: s0*c4+s4*c4
      D16MACF_AA_WW(xr15, xr0, xr0, xr9);//xr15: rnd(ss0*c4+ss4*c4)>>16
                                         //   :  rnd(s0*c4+s4*c4)>>16
      D16MUL_LW(xr10, xr5, xr3, xr9);    //xr10: ss2*c2, xr9: s2*c2
      D16MAC_AA_LW(xr10,xr6,xr13,xr9);   //xr10: ss2*c2+ss6*c6, xr9: s2*c2+s6*c6
      D16MACF_AA_WW(xr10, xr0, xr0, xr9);//xr10: rnd(ss2*c2+ss6*c6)>>16
                                         //    : rnd(s2*c2 + s6*c6)>>16
      S32LDD(xr2, blk, 0x10);            //xr2: ss1, s1
      S32LDD(xr4, blk, 0x30);            //xr4: ss3, s3
      Q16ADD_AS_WW(xr15,xr15,xr10,xr9);  //xr15: rnd(ss0*c4+ss4*c4)>>16+rnd(ss2*c2+ss6*c6)>>16
                                         //    :rnd(s0*c4+s4*c4)>>16 + rnd(s2*c2 + s6*c6)>>16
                                         //xr9: rnd(ss0*c4+ss4*c4)>>16 - rnd(ss2*c2+ss6*c6)>>16
                                         //   : rnd(s0*c4+s4*c4)>>16 - rnd(s2*c2 + s6*c6)>>16
      D16MUL_HW(xr10, xr5, xr1, xr1);    //xr10: ss0*c4, xr1: s0*c4
      D16MAC_SS_HW(xr10,xr5,xr11,xr1);   //xr10: ss0*c4-ss4*c4, xr1: s0*c4 - s4*c4
      D16MACF_AA_WW(xr10, xr0, xr0, xr1);//xr10: rnd(ss0*c4-ss4*c4)>>16
                                         //    : rnd(s0*c4 - s4*c4)>>16
      D16MUL_LW(xr11, xr6, xr3, xr1);    //xr11: ss2*c6, xr1: s2*c6
      D16MAC_SS_LW(xr11,xr5,xr13,xr1);   //xr11: ss2*c6-ss6*c2, xr1: s2*c6-s6*c2
      D16MACF_AA_WW(xr11, xr0, xr0, xr1);//xr11: rnd(ss2*c6-ss6*c2)>>16
                                         //    : rnd(s2*c6 - s6*c2)>>16
      S32LDD(xr12, blk, 0x50);           //xr12: ss5, s5
      S32LDD(xr14, blk, 0x70);           //xr14: ss7, s7
      Q16ADD_AS_WW(xr10,xr10,xr11,xr1);  //xr10: rnd(ss0*c4-ss4*c4)>>16)+rnd(ss2*c6-ss6*c2)>>16
                                         //    : rnd(s0*c4 - s4*c4)>>16 +rnd(s2*c6 - s6*c2)>>16
                                         //xr1 : rnd(ss0*c4-ss4*c4)>>16-rnd(ss2*c6-ss6*c2)>>16
                                         //    : rnd(s0*c4 - s4*c4)>>16-rnd(s2*c6 - s6*c2)>>16

      D16MUL_HW(xr11, xr7, xr2, xr13);   //xr11: ss1*c1, xr13: s1*c1
      D16MAC_AA_LW(xr11,xr7,xr4,xr13);   //xr11: ss1*c1+ss3*c3, xr13: s1*c1+s3*c3
      D16MAC_AA_LW(xr11,xr8,xr12,xr13);  //xr11: ss1*c1+ss3*c3+ss5*c5
                                         //xr13: s1*c1+s3*c3+s5*c5
      D16MAC_AA_HW(xr11,xr8,xr14,xr13);  //xr11: ss1*c1+ss3*c3+ss5*c5+ss7*c7
                                         //xr13: s1*c1+s3*c3+s5*c5+s7*c7
      D16MACF_AA_WW(xr11, xr0, xr0, xr13);//xr11: rnd(ss1*c1+ss3*c3+ss5*c5+ss7*c7)>>16
                                          //    : rnd(s1*c1+s3*c3+s5*c5+s7*c7)>>16

      D16MUL_LW(xr3, xr7, xr2, xr13);    //xr3: ss1*c3, xr13: s1*c3
      D16MAC_SS_HW(xr3,xr8,xr4,xr13);    //xr3: ss1*c3-ss3*c7, xr13: s1*c3-s3*c7
      D16MAC_SS_HW(xr3,xr7,xr12,xr13);   //xr3: ss1*c3-ss3*c7-ss5*c1
                                         //xr13: s1*c3-s3*c7-s5*c1
      D16MAC_SS_LW(xr3,xr8,xr14,xr13);   //xr3: ss1*c3-ss3*c7-ss5*c1-ss7*c5
                                         //xr13: s1*c3-s3*c7-s7*c5
      D16MACF_AA_WW(xr3, xr0, xr0, xr13);//xr3: rnd(ss1*c3-ss3*c7-ss5*c1-ss7*c5)>>16
                                         //   : rnd(s1*c3-s3*c7-s7*c5)>>16
      D16MUL_LW(xr5, xr8, xr2, xr13);    //xr5: ss1*c5, xr13:s1*c5
      D16MAC_SS_HW(xr5,xr7,xr4,xr13);    //xr5: ss1*c5-ss3*c1, xr13:s1*c5-s3*c1
      D16MAC_AA_HW(xr5,xr8,xr12,xr13);   //xr5: ss1*c5-ss3*c1+ss5*c7
                                         //   : s1*c5 - s3*c1+ s5*c7
      D16MAC_AA_LW(xr5,xr7,xr14,xr13);   //xr5: ss1*c5-ss3*c1+ss5*c7+ss7*c1
                                         //   : s1*c5 - s3*c1+ s5*c7+ s7*c1
      D16MACF_AA_WW(xr5, xr0, xr0, xr13);//xr5: rnd(ss1*c5-ss3*c1+ss5*c7+ss7*c1)>>16
                                         //   : rnd(s1*c5 - s3*c1+ s5*c7+ s7*c1)>>16

      D16MUL_HW(xr2, xr8, xr2, xr13);    //xr2: ss1*c7, xr13: s1*c7
      D16MAC_SS_LW(xr2,xr8,xr4,xr13);    //xr2: ss1*c7-ss3*c5, xr13: s1*c7-s3*c5
      D16MAC_AA_LW(xr2,xr7,xr12,xr13);   //xr2: ss1*c7-ss3*c5+ss5*c1
                                         //xr13: s1*c7-s3*c5+s5*c1
      D16MAC_SS_HW(xr2,xr7,xr14,xr13);   //xr2: ss1*c7-ss3*c5+ss5*c1-ss7*c3
                                         //xr13: s1*c7-s3*c5+s5*c1-s7*c3
      D16MACF_AA_WW(xr2, xr0, xr0, xr13);//xr2: rnd(ss1*c7-ss3*c5+ss5*c1-ss7*c3)>>16
                                         //   : rnd(s1*c7-s3*c5+s5*c1-s7*c3)>>16
      Q16ADD_AS_WW(xr15,xr15,xr11,xr11); //xr15:rnd(ss0*c4+ss4*c4)>>16+rnd(ss2*c2+ss6*c6)>>16+
                                         //     rnd(ss1*c1+ss3*c3+ss5*c5+ss7*c7)>>16
                                         //     rnd(s0*c4+s4*c4)>>16 + rnd(s2*c2 + s6*c6)>>16+
                                         //     rnd(s1*c1+s3*c3+s5*c5+s7*c7)>>16

                                         //xr11:rnd(ss0*c4+ss4*c4)>>16+rnd(ss2*c2+ss6*c6)>>16-
                                         //     rnd(ss1*c1+ss3*c3+ss5*c5+ss7*c7)>>16
                                         //     rnd(s0*c4+s4*c4)>>16 + rnd(s2*c2 + s6*c6)>>16-
                                         //     rnd(s1*c1+s3*c3+s5*c5+s7*c7)>>16
      Q16ADD_AS_WW(xr10,xr10,xr3,xr3);   //xr10:rnd(ss0*c4-ss4*c4)>>16)+rnd(ss2*c6-ss6*c2)>>16+
                                         //     rnd(ss1*c3-ss3*c7-ss5*c1-ss7*c5)>>16
                                         //     rnd(s0*c4 - s4*c4)>>16 +rnd(s2*c6 - s6*c2)>>16+
                                         //     rnd(s1*c3-s3*c7-s7*c5)>>16
                                         //xr10:rnd(ss0*c4-ss4*c4)>>16)+rnd(ss2*c6-ss6*c2)>>16-
                                         //     rnd(ss1*c3-ss3*c7-ss5*c1-ss7*c5)>>16
                                         //     rnd(s0*c4 - s4*c4)>>16 +rnd(s2*c6 - s6*c2)>>16-
                                         //     rnd(s1*c3-s3*c7-s7*c5)>>16
      Q16ADD_AS_WW(xr1,xr1,xr5,xr5);     //xr1: rnd(ss0*c4-ss4*c4)>>16-rnd(ss2*c6-ss6*c2)>>16+
                                         //     rnd(ss1*c5-ss3*c1+ss5*c7+ss7*c1)>>16
                                         //     rnd(s0*c4 - s4*c4)>>16 +rnd(s2*c6 - s6*c2)>>16+
                                         //     rnd(s1*c5 - s3*c1+ s5*c7+ s7*c1)>>16
                                         //xr1: rnd(ss0*c4-ss4*c4)>>16-rnd(ss2*c6-ss6*c2)>>16-
                                         //     rnd(ss1*c5-ss3*c1+ss5*c7+ss7*c1)>>16
                                         //     rnd(s0*c4 - s4*c4)>>16 +rnd(s2*c6 - s6*c2)>>16-
                                         //     rnd(s1*c5 - s3*c1+ s5*c7+ s7*c1)>>16
      Q16ADD_AS_WW(xr9,xr9,xr2,xr2);     //xr9: rnd(ss0*c4+ss4*c4)>>16 - rnd(ss2*c2+ss6*c6)>>16+
                                         //     rnd(ss1*c7-ss3*c5+ss5*c1-ss7*c3)>>16
                                         //     rnd(s0*c4+s4*c4)>>16 - rnd(s2*c2 + s6*c6)>>16+
                                         //     rnd(s1*c7-s3*c5+s5*c1-s7*c3)>>16
                                         //xr9: rnd(ss0*c4+ss4*c4)>>16 - rnd(ss2*c2+ss6*c6)>>16-
                                         //     rnd(ss1*c7-ss3*c5+ss5*c1-ss7*c3)>>16
                                         //     rnd(s0*c4+s4*c4)>>16 - rnd(s2*c2 + s6*c6)>>16-
                                         //     rnd(s1*c7-s3*c5+s5*c1-s7*c3)>>16

      S32STD(xr15, blk, 0x00);
      S32STD(xr10, blk, 0x10);
      S32STD(xr1, blk, 0x20);
      S32STD(xr9, blk, 0x30);
      S32STD(xr2, blk, 0x40);
      S32STD(xr5, blk, 0x50);
      S32STD(xr3, blk, 0x60);
      S32STD(xr11, blk, 0x70);
    }
  blk = block -8;
  dest -= stride;
  for (i=0; i< 8; i++) {
    S32LDI(xr1, blk, 0x10);
    S32LDD(xr2, blk, 0x4);
    S32LDD(xr3, blk, 0x8);
    S32LDD(xr4, blk, 0xc);
    S32STD(xr0, blk, 0x0);
    S32STD(xr0, blk, 0x4); 
    S32STD(xr0, blk, 0x8);
    S32STD(xr0, blk, 0xc);
    Q16SAT(xr5, xr2, xr1);
    Q16SAT(xr6, xr4, xr3);
    S32SDIV(xr5, dest, stride, 0x0);
    S32STD(xr6, dest, 0x4);
  }
}

static void mpeg2_idct_add_mxu (int16_t * block, uint8_t * dest, const int stride,uint8_t idct_row)
{
  int i;
  int16_t  *blk;
  int32_t wf = (int32_t)whirl_idct;

  S32LDD(xr5, wf, 0x0);         // xr5(w7, w3)
  S32LDD(xr6, wf, 0x4);         // xr6(w9, w8)
  S32LDD(xr7, wf, 0x8);         // xr7(w11,w10)
  S32LDD(xr8, wf, 0xc);         // xr8(w13,w12)
  S32LDD(xr9, wf, 0x10);        // xr9(w6, w0)
  S32LDD(xr10,wf, 0x14);

  blk = block - 8;
  for(i=0; i<idct_row; i++) { /* idct rows */
      S32LDI(xr1, blk, 0x10);        //  xr1 (x4, x0)
      S32LDD(xr2, blk, 0x4);        //  xr2 (x7, x3)
      S32LDD(xr3, blk, 0x8);        //  xr3 (x6, x1)
      S32LDD(xr4, blk, 0xc);        //  xr4 (x5, x2)

      S32SFL(xr1,xr1,xr2,xr2, ptn3);
      S32SFL(xr3,xr3,xr4,xr4, ptn3);

      D16MUL_WW(xr11, xr2, xr5, xr12);
      D16MAC_AA_WW(xr11,xr4,xr6,xr12);

      D16MUL_WW(xr13, xr2, xr6, xr14);
      D16MAC_SS_WW(xr13,xr4,xr5,xr14);

      D16MUL_HW(xr2, xr1, xr7, xr4);
      D16MAC_AS_LW(xr2,xr1,xr9,xr4);
      D16MAC_AS_HW(xr2,xr3,xr10,xr4);
      D16MAC_AS_LW(xr2,xr3,xr8,xr4);
      D16MACF_AA_WW(xr2, xr0, xr0, xr4);
      D16MACF_AA_WW(xr11, xr0, xr0, xr13);
      D16MACF_AA_WW(xr12, xr0, xr0, xr14);

      D16MUL_HW(xr4, xr1, xr8, xr15);
      D16MAC_SS_LW(xr4,xr1,xr10,xr15);
      D16MAC_AA_HW(xr4,xr3,xr9,xr15);
      D16MAC_SA_LW(xr4,xr3,xr7,xr15);
      Q16ADD_AS_WW(xr11,xr11,xr12,xr12); 
      D16MACF_AA_WW(xr15, xr0, xr0, xr4);

      Q16ADD_AS_WW(xr11, xr11, xr2, xr2);
      Q16ADD_AS_XW(xr12, xr12, xr15, xr15);

      S32SFL(xr11,xr11,xr12,xr12, ptn3);
      S32SFL(xr12,xr12,xr11,xr11, ptn3);

      S32STD(xr12, blk, 0x0);
      S32STD(xr11, blk, 0x4);
      S32STD(xr15, blk, 0x8);
      S32STD(xr2, blk, 0xc);
  }
  blk  = block - 2;
  for (i = 0; i < 4; i++)               /* idct columns */
    {
      S32LDI(xr1, blk, 0x4);
      S32LDD(xr3, blk, 0x20);
      S32I2M(xr5,wxr5);
      S32LDD(xr11, blk, 0x40);
      S32LDD(xr13, blk, 0x60);

      D16MUL_HW(xr15, xr5, xr1, xr9);
      D16MAC_AA_HW(xr15,xr5,xr11,xr9);
      D16MACF_AA_WW(xr15, xr0, xr0, xr9);
      D16MUL_LW(xr10, xr5, xr3, xr9);
      D16MAC_AA_LW(xr10,xr6,xr13,xr9);
      D16MACF_AA_WW(xr10, xr0, xr0, xr9);
      S32LDD(xr2, blk, 0x10);
      S32LDD(xr4, blk, 0x30);
      Q16ADD_AS_WW(xr15,xr15,xr10,xr9);

      D16MUL_HW(xr10, xr5, xr1, xr1);
      D16MAC_SS_HW(xr10,xr5,xr11,xr1);
      D16MACF_AA_WW(xr10, xr0, xr0, xr1);
      D16MUL_LW(xr11, xr6, xr3, xr1);
      D16MAC_SS_LW(xr11,xr5,xr13,xr1);
      D16MACF_AA_WW(xr11, xr0, xr0, xr1);
      S32LDD(xr12, blk, 0x50);
      S32LDD(xr14, blk, 0x70);
      Q16ADD_AS_WW(xr10,xr10,xr11,xr1);

      D16MUL_HW(xr11, xr7, xr2, xr13);
      D16MAC_AA_LW(xr11,xr7,xr4,xr13);
      D16MAC_AA_LW(xr11,xr8,xr12,xr13);
      D16MAC_AA_HW(xr11,xr8,xr14,xr13);
      D16MACF_AA_WW(xr11, xr0, xr0, xr13);
     
       D16MUL_LW(xr3, xr7, xr2, xr13);
      D16MAC_SS_HW(xr3,xr8,xr4,xr13);
      D16MAC_SS_HW(xr3,xr7,xr12,xr13);
      D16MAC_SS_LW(xr3,xr8,xr14,xr13);
      D16MACF_AA_WW(xr3, xr0, xr0, xr13);

      D16MUL_LW(xr5, xr8, xr2, xr13);
      D16MAC_SS_HW(xr5,xr7,xr4,xr13);
      D16MAC_AA_HW(xr5,xr8,xr12,xr13);
      D16MAC_AA_LW(xr5,xr7,xr14,xr13);
      D16MACF_AA_WW(xr5, xr0, xr0, xr13);

      D16MUL_HW(xr2, xr8, xr2, xr13);
      D16MAC_SS_LW(xr2,xr8,xr4,xr13);
      D16MAC_AA_LW(xr2,xr7,xr12,xr13);
      D16MAC_SS_HW(xr2,xr7,xr14,xr13);
      D16MACF_AA_WW(xr2, xr0, xr0, xr13);

      Q16ADD_AS_WW(xr15,xr15,xr11,xr11);
      Q16ADD_AS_WW(xr10,xr10,xr3,xr3);
      Q16ADD_AS_WW(xr1,xr1,xr5,xr5);
      Q16ADD_AS_WW(xr9,xr9,xr2,xr2);

      S32STD(xr15, blk, 0x00);
      S32STD(xr10, blk, 0x10);
      S32STD(xr1, blk, 0x20);
      S32STD(xr9, blk, 0x30);
      S32STD(xr2, blk, 0x40);
      S32STD(xr5, blk, 0x50);
      S32STD(xr3, blk, 0x60);
      S32STD(xr11, blk, 0x70);
    }
  blk = block - 8;
  dest -= stride;
  for (i=0; i< 8; i++) {
    S32LDIV(xr5, dest, stride, 0x0);
    S32LDI(xr1, blk, 0x10);
    S32LDD(xr2, blk, 0x4);
    Q8ACCE_AA(xr2, xr5, xr0, xr1);

    S32LDD(xr6, dest, 0x4);
    S32LDD(xr3, blk, 0x8);
    S32LDD(xr4, blk, 0xc);
    Q8ACCE_AA(xr4, xr6, xr0, xr3);
    S32STD(xr0, blk, 0x0);
    S32STD(xr0, blk, 0x4);
    S32STD(xr0, blk, 0x8);
    S32STD(xr0, blk, 0xc);
    Q16SAT(xr5, xr2, xr1);
    S32STD(xr5, dest, 0x0);
    Q16SAT(xr6, xr4, xr3);
    S32STD(xr6, dest, 0x4);
  }
}

#define MXU_MEMSET(addr,data,len)   \
    do {                            \
       int32_t mxu_i;               \
       int32_t local = (int32_t)(addr)-4;  \
       int cline = ((len)>>5);		   \
       for (mxu_i=0; mxu_i < cline; mxu_i++) \
       {                                     \
	   i_pref(30,local,4);		     \
	   S32SDI(xr0,local,4);              \
           S32SDI(xr0,local,4);              \
           S32SDI(xr0,local,4);              \
           S32SDI(xr0,local,4);              \
           S32SDI(xr0,local,4);              \
           S32SDI(xr0,local,4);              \
           S32SDI(xr0,local,4);              \    
           S32SDI(xr0,local,4);              \
       }                                     \
    }while(0)
