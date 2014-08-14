#include "../libjzcommon/jzasm.h"
#include "../libjzcommon/jzmedia.h"


//16x16
void pred16x16_vertical_test(uint8_t *dst, int stride, uint8_t *top){
  int i;
  uint8_t *src_top; 
  src_top = top;
  //load
  S32LDD(xr1, src_top, 0x0);
  S32LDD(xr2, src_top, 0x4);
  S32LDD(xr3, src_top, 0x8);
  S32LDD(xr4, src_top, 0xc);
  //store
  dst -= stride;
  for (i=0; i<16; i++) {
    S32SDIV(xr1, dst, stride, 0x0);
    S32STD(xr2, dst, 0x4);
    S32STD(xr3, dst, 0x8);
    S32STD(xr4, dst, 0xc);
  }
}

void pred16x16_horizontal_test(uint8_t *src, int stride, uint8_t *left){
    int i;

    for(i=0; i<16; i++){
        ((uint32_t*)(src+i*stride))[0]=
        ((uint32_t*)(src+i*stride))[1]=
        ((uint32_t*)(src+i*stride))[2]=
        ((uint32_t*)(src+i*stride))[3]= left[i]*0x01010101;
    }
}

void pred16x16_dc_test(uint8_t *src, int stride, uint8_t *top, uint8_t *left){
    int i, dc=0;
    for(i=0; i<16; i++){
      dc += left[i];
    }
    for(i=0; i<16; i++){
      dc += top[i];
    }
    dc= 0x01010101*((dc + 16)>>5);

    for(i=0; i<16; i++){
        ((uint32_t*)(src+i*stride))[0]=
        ((uint32_t*)(src+i*stride))[1]=
        ((uint32_t*)(src+i*stride))[2]=
        ((uint32_t*)(src+i*stride))[3]= dc;
    }
}


void pred16x16_plane_compat_test(uint8_t *src, int stride, uint8_t *top, uint8_t *left){
  int i, j, k;
  int a;

  uint8_t *dst;
  const uint8_t * const src0 = top+7;
  const uint8_t *src1 = left+8;
  const uint8_t *src2 = left+6;      // == src+6*stride-1;
  int H = top[8] - top[6];
  int V = left[8] - left[6]; 
  //k == 2
  H += 2*(top[9]-top[5]);
  V += 2*(left[9]-left[5]);

  //k == 3
  H += 3*(top[10] - top[4]);
  V += 3*(left[10]-left[4]);

  //k == 4
  H += 4*(top[11]-top[3]);  
  V += 4*(left[11]-left[3]);

  //k == 5
  H += 5*(top[12]-top[2]);  
  V += 5*(left[12]-left[2]);

  //k == 6
  H += 6*(top[13]-top[1]);  
  V += 6*(left[13]-left[1]);

  //k == 7
  H += 7*(top[14]-top[0]);  
  V += 7*(left[14]-left[0]);

  //k == 8
  H += 8*(top[15]-top[-1]);  
  V += 8*(left[15]-left[-1]);

  H = ( H + (H>>2) ) >> 4;
  V = ( V + (V>>2) ) >> 4;

  a = 16*(left[15] + top[15] + 1) - 7*(V+H);
  for(j=16; j>0; --j) {
    int b = a;
    a += V;
    dst = src - 4;
    for(i=0; i<4; i++){
      S32I2M(xr1,b);
      S32I2M(xr2,b+H);
      S32I2M(xr3,b+2*H);
      S32I2M(xr4,b+3*H);
      D32SARL(xr5,xr2,xr1,5);
      D32SARL(xr6,xr4,xr3,5);
      Q16SAT(xr1,xr6,xr5);
      S32SDI(xr1,dst,0x4);
      b += 4*H;
    }
    src += stride;
  }
}

void pred16x16_left_dc_test(uint8_t *src, int stride, uint8_t *left){
    int i, dc=0;

    for(i=0;i<16; i++){
        dc+= left[i];
    }
    dc= 0x01010101*((dc + 8)>>4);

    for(i=0; i<16; i++){
        ((uint32_t*)(src+i*stride))[0]=
        ((uint32_t*)(src+i*stride))[1]=
        ((uint32_t*)(src+i*stride))[2]=
        ((uint32_t*)(src+i*stride))[3]= dc;
    }
}

void pred16x16_top_dc_test(uint8_t *src, int stride, uint8_t *top){
    int i, dc=0;
    for(i=0;i<16; i++){
      dc+= top[i];
    }
    dc= 0x01010101*((dc + 8)>>4);

    for(i=0; i<16; i++){
        ((uint32_t*)(src+i*stride))[0]=
        ((uint32_t*)(src+i*stride))[1]=
        ((uint32_t*)(src+i*stride))[2]=
        ((uint32_t*)(src+i*stride))[3]= dc;
    }
}

void pred16x16_128_dc_test(uint8_t *src, int stride){
    int i;

    for(i=0; i<16; i++){
        ((uint32_t*)(src+i*stride))[0]=
        ((uint32_t*)(src+i*stride))[1]=
        ((uint32_t*)(src+i*stride))[2]=
        ((uint32_t*)(src+i*stride))[3]= 0x01010101U*128U;
    }
}


//8x8
void pred8x8_vertical_test(uint8_t *src, int stride, uint8_t *top){
    int i;
    const uint32_t a= ((uint32_t*)(top))[0];
    const uint32_t b= ((uint32_t*)(top))[1];
    for(i=0; i<8; i++){
        ((uint32_t*)(src+i*stride))[0]= a;
        ((uint32_t*)(src+i*stride))[1]= b;
    }
}

void pred8x8_horizontal_test(uint8_t *src, int stride, uint8_t *left){
    int i;

    for(i=0; i<8; i++){
        ((uint32_t*)(src+i*stride))[0]=
        ((uint32_t*)(src+i*stride))[1]= left[i]*0x01010101;
    }
}

void pred8x8_128_dc_test(uint8_t *src, int stride){
    int i;

    for(i=0; i<8; i++){
        ((uint32_t*)(src+i*stride))[0]=
        ((uint32_t*)(src+i*stride))[1]= 0x01010101U*128U;
    }
}

void pred8x8_dc_rv40_test(uint8_t *src, int stride, uint8_t *top, uint8_t *left){
    int i;
    int dc0=0;
    for(i=0;i<4; i++){
        dc0+= left[i] + top[i];
        dc0+= top[4+i];
        dc0+= left[i+4];
    }

    dc0= 0x01010101*((dc0 + 8)>>4);

    for(i=0; i<4; i++){
        ((uint32_t*)(src+i*stride))[0]= dc0;
        ((uint32_t*)(src+i*stride))[1]= dc0;
    }
    for(i=4; i<8; i++){
        ((uint32_t*)(src+i*stride))[0]= dc0;
        ((uint32_t*)(src+i*stride))[1]= dc0;
    }
}


void pred8x8_plane_test(uint8_t *src, int stride, uint8_t *top, uint8_t *left){
  int j, k;
  int a;

  const uint8_t * const src0 = top+3;
  const uint8_t *src1 = left+4;
  const uint8_t *src2 = left+2;      // == src+2*stride-1;
  int H = top[4] - top[2];
  int V = left[4] - left[2];
  //k == 2
  H += 2*(top[5]-top[1]);
  V += 2*(left[5]-left[1]);
  //k == 3   
  H += 3*(top[6]-top[0]);
  V += 3*(left[6]-left[0]);
  //k == 4   
  H += 4*(top[7]-top[-1]);
  V += 4*(left[7]-left[-1]);

  H = ( 17*H+16 ) >> 5;
  V = ( 17*V+16 ) >> 5;

  a = 16*(left[7] + top[7] + 1) - 3*(V+H);
  for(j=8; j>0; --j) {
    int b = a;
    a += V;
    S32I2M(xr1,b);
    S32I2M(xr2,b+H);
    S32I2M(xr3,b+2*H);
    S32I2M(xr4,b+3*H);
    S32I2M(xr5,b+4*H);
    S32I2M(xr6,b+5*H);
    S32I2M(xr7,b+6*H);
    S32I2M(xr8,b+7*H);
    D32SARL(xr9,xr2,xr1,5);
    D32SARL(xr10,xr4,xr3,5);
    D32SARL(xr11,xr6,xr5,5);
    D32SARL(xr12,xr8,xr7,5);
    Q16SAT(xr1,xr10,xr9);
    Q16SAT(xr2,xr12,xr11);
    S32STD(xr1,src,0x0);
    S32STD(xr2,src,0x4);
    src += stride;
  }
}

void pred8x8_left_dc_rv40_test(uint8_t *src, int stride, uint8_t *left){
    int i;
    int dc0;

    dc0=0;
    for(i=0;i<8; i++)
        dc0+= left[i];
    dc0= 0x01010101*((dc0 + 4)>>3);

    for(i=0; i<8; i++){
        ((uint32_t*)(src+i*stride))[0]=
        ((uint32_t*)(src+i*stride))[1]= dc0;
    }
}

void pred8x8_top_dc_rv40_test(uint8_t *src, int stride, uint8_t *top){
    int i;
    int dc0;

    dc0=0;
    for(i=0;i<8; i++)
        dc0+= top[i];
    dc0= 0x01010101*((dc0 + 4)>>3);

    for(i=0; i<8; i++){
        ((uint32_t*)(src+i*stride))[0]=
        ((uint32_t*)(src+i*stride))[1]= dc0;
    }
}

//4x4
void pred4x4_vertical_test(uint8_t *src, int stride, uint8_t *top){
    const uint32_t a= ((uint32_t*)(top))[0];
    ((uint32_t*)(src+0*stride))[0]= a;
    ((uint32_t*)(src+1*stride))[0]= a;
    ((uint32_t*)(src+2*stride))[0]= a;
    ((uint32_t*)(src+3*stride))[0]= a;
}

void pred4x4_horizontal_test(uint8_t *src, int stride, uint8_t *left){
    ((uint32_t*)(src+0*stride))[0]= left[0]*0x01010101;
    ((uint32_t*)(src+1*stride))[0]= left[1]*0x01010101;
    ((uint32_t*)(src+2*stride))[0]= left[2]*0x01010101;
    ((uint32_t*)(src+3*stride))[0]= left[3]*0x01010101;
}

void pred4x4_dc_test(uint8_t *src, int stride, uint8_t *top, uint8_t *left){
    const int dc=(top[0]+top[1]+top[2]+top[3]+left[0]+left[1]+left[2]+left[3]+4)>>3;
    ((uint32_t*)(src+0*stride))[0]=
    ((uint32_t*)(src+1*stride))[0]=
    ((uint32_t*)(src+2*stride))[0]=
    ((uint32_t*)(src+3*stride))[0]= dc* 0x01010101;
}

void pred4x4_left_dc_test(uint8_t *src, int stride, uint8_t *left){
    const int dc= (left[0] + left[1] + left[2] + left[3] + 2) >>2;

    ((uint32_t*)(src+0*stride))[0]=
    ((uint32_t*)(src+1*stride))[0]=
    ((uint32_t*)(src+2*stride))[0]=
    ((uint32_t*)(src+3*stride))[0]= dc* 0x01010101;
}

void pred4x4_top_dc_test(uint8_t *src, int stride, uint8_t *top){
    const int dc= (  top[0] + top[1] + top[2] + top[3] + 2) >>2;

    ((uint32_t*)(src+0*stride))[0]=
    ((uint32_t*)(src+1*stride))[0]=
    ((uint32_t*)(src+2*stride))[0]=
    ((uint32_t*)(src+3*stride))[0]= dc* 0x01010101;
}

void pred4x4_128_dc_test(uint8_t *src,  int stride){
    ((uint32_t*)(src+0*stride))[0]=
    ((uint32_t*)(src+1*stride))[0]=
    ((uint32_t*)(src+2*stride))[0]=
    ((uint32_t*)(src+3*stride))[0]= 128U*0x01010101U;
}

void pred4x4_vertical_left_rv40_t(uint8_t *src, uint8_t *top, int stride,
				  const int l0, const int l1, const int l2, const int l3, const int l4, int up4){
    int t4,t5,t6,t7;
    const int av_unused t0= top[0];
    const int av_unused t1= top[1];
    const int av_unused t2= top[2];
    const int av_unused t3= top[3];

    if (!up4){
     t4= top[4];	
     t5= top[5];
     t6= top[6];
     t7= top[7];
    }else{
     t4= top[3];	
     t5= top[3];
     t6= top[3];
     t7= top[3];
    }

    src[0+0*stride]=(2*t0 + 2*t1 + l1 + 2*l2 + l3 + 4)>>3;
    src[1+0*stride]=
    src[0+2*stride]=(t1 + t2 + 1)>>1;
    src[2+0*stride]=
    src[1+2*stride]=(t2 + t3 + 1)>>1;
    src[3+0*stride]=
    src[2+2*stride]=(t3 + t4+ 1)>>1;
    src[3+2*stride]=(t4 + t5+ 1)>>1;
    src[0+1*stride]=(t0 + 2*t1 + t2 + l2 + 2*l3 + l4 + 4)>>3;
    src[1+1*stride]=
    src[0+3*stride]=(t1 + 2*t2 + t3 + 2)>>2;
    src[2+1*stride]=
    src[1+3*stride]=(t2 + 2*t3 + t4 + 2)>>2;
    src[3+1*stride]=
    src[2+3*stride]=(t3 + 2*t4 + t5 + 2)>>2;
    src[3+3*stride]=(t4 + 2*t5 + t6 + 2)>>2;
}

void pred4x4_down_left_rv40_test(uint8_t *src, int stride, uint8_t *top, uint8_t *left, int up4){
    int t4,t5,t6,t7;
    int t0= top[0];
    int t1= top[1];
    int t2= top[2];
    int t3= top[3];

    if (!up4){
     t4= top[4];	
     t5= top[5];
     t6= top[6];
     t7= top[7];
    }else{
     t4= top[3];	
     t5= top[3];
     t6= top[3];
     t7= top[3];
    }
    const int av_unused l0= left[0];	
    const int av_unused l1= left[1];
    const int av_unused l2= left[2];
    const int av_unused l3= left[3];

    const int av_unused l4= left[4];	
    const int av_unused l5= left[5];
    const int av_unused l6= left[6];
    const int av_unused l7= left[7];

    src[0+0*stride]=(t0 + t2 + 2*t1 + 2 + l0 + l2 + 2*l1 + 2)>>3;
    src[1+0*stride]=
    src[0+1*stride]=(t1 + t3 + 2*t2 + 2 + l1 + l3 + 2*l2 + 2)>>3;
    src[2+0*stride]=
    src[1+1*stride]=
    src[0+2*stride]=(t2 + t4 + 2*t3 + 2 + l2 + l4 + 2*l3 + 2)>>3;
    src[3+0*stride]=
    src[2+1*stride]=
    src[1+2*stride]=
    src[0+3*stride]=(t3 + t5 + 2*t4 + 2 + l3 + l5 + 2*l4 + 2)>>3;
    src[3+1*stride]=
    src[2+2*stride]=
    src[1+3*stride]=(t4 + t6 + 2*t5 + 2 + l4 + l6 + 2*l5 + 2)>>3;
    src[3+2*stride]=
    src[2+3*stride]=(t5 + t7 + 2*t6 + 2 + l5 + l7 + 2*l6 + 2)>>3;
    src[3+3*stride]=(t6 + t7 + 1 + l6 + l7 + 1)>>2;
}

void pred4x4_down_right_test(uint8_t *src, int stride, uint8_t *top, uint8_t *left){
    const int lt= top[-1];
    const int av_unused t0= top[0];	
    const int av_unused t1= top[1];
    const int av_unused t2= top[2];
    const int av_unused t3= top[3];

    const int av_unused l0= left[0];	
    const int av_unused l1= left[1];
    const int av_unused l2= left[2];
    const int av_unused l3= left[3];

    src[0+3*stride]=(l3 + 2*l2 + l1 + 2)>>2;
    src[0+2*stride]=
    src[1+3*stride]=(l2 + 2*l1 + l0 + 2)>>2;
    src[0+1*stride]=
    src[1+2*stride]=
    src[2+3*stride]=(l1 + 2*l0 + lt + 2)>>2;
    src[0+0*stride]=
    src[1+1*stride]=
    src[2+2*stride]=
    src[3+3*stride]=(l0 + 2*lt + t0 + 2)>>2;
    src[1+0*stride]=
    src[2+1*stride]=
    src[3+2*stride]=(lt + 2*t0 + t1 + 2)>>2;
    src[2+0*stride]=
    src[3+1*stride]=(t0 + 2*t1 + t2 + 2)>>2;
    src[3+0*stride]=(t1 + 2*t2 + t3 + 2)>>2;
}

void pred4x4_vertical_right_test(uint8_t *src, int stride, uint8_t *top, uint8_t *left){
    const int lt= top[-1];
    const int av_unused t0= top[0];	
    const int av_unused t1= top[1];
    const int av_unused t2= top[2];
    const int av_unused t3= top[3];

    const int av_unused l0= left[0];	
    const int av_unused l1= left[1];
    const int av_unused l2= left[2];
    const int av_unused l3= left[3];

    src[0+0*stride]=
    src[1+2*stride]=(lt + t0 + 1)>>1;
    src[1+0*stride]=
    src[2+2*stride]=(t0 + t1 + 1)>>1;
    src[2+0*stride]=
    src[3+2*stride]=(t1 + t2 + 1)>>1;
    src[3+0*stride]=(t2 + t3 + 1)>>1;
    src[0+1*stride]=
    src[1+3*stride]=(l0 + 2*lt + t0 + 2)>>2;
    src[1+1*stride]=
    src[2+3*stride]=(lt + 2*t0 + t1 + 2)>>2;
    src[2+1*stride]=
    src[3+3*stride]=(t0 + 2*t1 + t2 + 2)>>2;
    src[3+1*stride]=(t1 + 2*t2 + t3 + 2)>>2;
    src[0+2*stride]=(lt + 2*l0 + l1 + 2)>>2;
    src[0+3*stride]=(l0 + 2*l1 + l2 + 2)>>2;
}


void pred4x4_horizontal_down_test(uint8_t *src, int stride, uint8_t *top, uint8_t *left){
    const int lt= top[-1];
    const int av_unused t0= top[0];	
    const int av_unused t1= top[1];
    const int av_unused t2= top[2];
    const int av_unused t3= top[3];

    const int av_unused l0= left[0];	
    const int av_unused l1= left[1];
    const int av_unused l2= left[2];
    const int av_unused l3= left[3];

    src[0+0*stride]=
    src[2+1*stride]=(lt + l0 + 1)>>1;
    src[1+0*stride]=
    src[3+1*stride]=(l0 + 2*lt + t0 + 2)>>2;
    src[2+0*stride]=(lt + 2*t0 + t1 + 2)>>2;
    src[3+0*stride]=(t0 + 2*t1 + t2 + 2)>>2;
    src[0+1*stride]=
    src[2+2*stride]=(l0 + l1 + 1)>>1;
    src[1+1*stride]=
    src[3+2*stride]=(lt + 2*l0 + l1 + 2)>>2;
    src[0+2*stride]=
    src[2+3*stride]=(l1 + l2+ 1)>>1;
    src[1+2*stride]=
    src[3+3*stride]=(l0 + 2*l1 + l2 + 2)>>2;
    src[0+3*stride]=(l2 + l3 + 1)>>1;
    src[1+3*stride]=(l1 + 2*l2 + l3 + 2)>>2;
}

void pred4x4_vertical_left_rv40_test(uint8_t *src, int stride, uint8_t *top, uint8_t *left, int up4){
    const int av_unused l0= left[0];
    const int av_unused l1= left[1];
    const int av_unused l2= left[2];
    const int av_unused l3= left[3];
  
    const int av_unused l4= left[4];
    const int av_unused l5= left[5];
    const int av_unused l6= left[6];
    const int av_unused l7= left[7];
    pred4x4_vertical_left_rv40_t(src, top, stride, l0, l1, l2, l3, l4, up4);
}

void pred4x4_horizontal_up_rv40_test(uint8_t *src, int stride, uint8_t *top, uint8_t *left, int up4){
    int t4,t5,t6,t7;
    const int av_unused l0= left[0];
    const int av_unused l1= left[1];
    const int av_unused l2= left[2];
    const int av_unused l3= left[3];

    const int av_unused l4= left[4];	
    const int av_unused l5= left[5];
    const int av_unused l6= left[6];
    const int av_unused l7= left[7];

    const int av_unused t0= top[0];
    const int av_unused t1= top[1];
    const int av_unused t2= top[2];
    const int av_unused t3= top[3];

    if (!up4){
     t4= top[4];	
     t5= top[5];
     t6= top[6];
     t7= top[7];
    }else{
     t4= top[3];	
     t5= top[3];
     t6= top[3];
     t7= top[3];
    }

    src[0+0*stride]=(t1 + 2*t2 + t3 + 2*l0 + 2*l1 + 4)>>3;
    src[1+0*stride]=(t2 + 2*t3 + t4 + l0 + 2*l1 + l2 + 4)>>3;
    src[2+0*stride]=
    src[0+1*stride]=(t3 + 2*t4 + t5 + 2*l1 + 2*l2 + 4)>>3;
    src[3+0*stride]=
    src[1+1*stride]=(t4 + 2*t5 + t6 + l1 + 2*l2 + l3 + 4)>>3;
    src[2+1*stride]=
    src[0+2*stride]=(t5 + 2*t6 + t7 + 2*l2 + 2*l3 + 4)>>3;
    src[3+1*stride]=
    src[1+2*stride]=(t6 + 3*t7 + l2 + 3*l3 + 4)>>3;
    src[3+2*stride]=
    src[1+3*stride]=(l3 + 2*l4 + l5 + 2)>>2;
    src[0+3*stride]=
    src[2+2*stride]=(t6 + t7 + l3 + l4 + 2)>>2;
    src[2+3*stride]=(l4 + l5 + 1)>>1;
    src[3+3*stride]=(l4 + 2*l5 + l6 + 2)>>2;
}


void pred4x4_down_left_rv40_nodown_test(uint8_t *src, int stride, uint8_t *top, uint8_t *left, int up4){
    int t4,t5,t6,t7;
    const int av_unused t0= top[0];
    const int av_unused t1= top[1];
    const int av_unused t2= top[2];
    const int av_unused t3= top[3];

    if (!up4){
     t4= top[4];	
     t5= top[5];
     t6= top[6];
     t7= top[7];
    }else{
     t4= top[3];	
     t5= top[3];
     t6= top[3];
     t7= top[3];
    }  

    const int av_unused l0= left[0];
    const int av_unused l1= left[1];
    const int av_unused l2= left[2];
    const int av_unused l3= left[3];

    src[0+0*stride]=(t0 + t2 + 2*t1 + 2 + l0 + l2 + 2*l1 + 2)>>3;
    src[1+0*stride]=
    src[0+1*stride]=(t1 + t3 + 2*t2 + 2 + l1 + l3 + 2*l2 + 2)>>3;
    src[2+0*stride]=
    src[1+1*stride]=
    src[0+2*stride]=(t2 + t4 + 2*t3 + 2 + l2 + 3*l3 + 2)>>3;
    src[3+0*stride]=
    src[2+1*stride]=
    src[1+2*stride]=
    src[0+3*stride]=(t3 + t5 + 2*t4 + 2 + l3*4 + 2)>>3;
    src[3+1*stride]=
    src[2+2*stride]=
    src[1+3*stride]=(t4 + t6 + 2*t5 + 2 + l3*4 + 2)>>3;
    src[3+2*stride]=
    src[2+3*stride]=(t5 + t7 + 2*t6 + 2 + l3*4 + 2)>>3;
    src[3+3*stride]=(t6 + t7 + 1 + 2*l3 + 1)>>2;
}


void pred4x4_horizontal_up_rv40_nodown_test(uint8_t *src, int stride, uint8_t *top, uint8_t *left, int up4){
    int t4,t5,t6,t7;
    const int av_unused l0= left[0];
    const int av_unused l1= left[1];
    const int av_unused l2= left[2];
    const int av_unused l3= left[3];

    const int av_unused t0= top[0];	
    const int av_unused t1= top[1];
    const int av_unused t2= top[2];
    const int av_unused t3= top[3];

    if (!up4){
     t4= top[4];	
     t5= top[5];
     t6= top[6];
     t7= top[7];
    }else{
     t4= top[3];	
     t5= top[3];
     t6= top[3];
     t7= top[3];
    }  

    src[0+0*stride]=(t1 + 2*t2 + t3 + 2*l0 + 2*l1 + 4)>>3;
    src[1+0*stride]=(t2 + 2*t3 + t4 + l0 + 2*l1 + l2 + 4)>>3;
    src[2+0*stride]=
    src[0+1*stride]=(t3 + 2*t4 + t5 + 2*l1 + 2*l2 + 4)>>3;
    src[3+0*stride]=
    src[1+1*stride]=(t4 + 2*t5 + t6 + l1 + 2*l2 + l3 + 4)>>3;
    src[2+1*stride]=
    src[0+2*stride]=(t5 + 2*t6 + t7 + 2*l2 + 2*l3 + 4)>>3;
    src[3+1*stride]=
    src[1+2*stride]=(t6 + 3*t7 + l2 + 3*l3 + 4)>>3;
    src[3+2*stride]=
    src[1+3*stride]=l3;
    src[0+3*stride]=
    src[2+2*stride]=(t6 + t7 + 2*l3 + 2)>>2;
    src[2+3*stride]=
    src[3+3*stride]=l3;
}

void pred4x4_vertical_left_rv40_nodown_test(uint8_t *src, int stride, uint8_t *top, uint8_t *left, int up4){
    const int av_unused l0= left[0];
    const int av_unused l1= left[1];
    const int av_unused l2= left[2];
    const int av_unused l3= left[3];
    pred4x4_vertical_left_rv40_t(src, top, stride, l0, l1, l2, l3, l3, up4);
}

