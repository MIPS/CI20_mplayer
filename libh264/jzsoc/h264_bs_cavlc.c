#ifdef HAVE_BUILTIN_EXPECT
#define likely(x) __builtin_expect ((x) != 0, 1)
#define unlikely(x) __builtin_expect ((x) != 0, 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

#include "../../libjzcommon/jzmedia.h"
#include "../../libjzcommon/jzasm.h"
//#include "../bitstream.h"
#include "../get_bits.h"
#include "../h264_cavlc.c" //FIXME: move variables declared in h264_cavlc.c to h264_cavlc.h, gjwang

#if 0
typedef struct H264_VLC_RES{
  int     total_coeff;
  uint8_t *index;
  short   *coeff;
}H264_VLC_RES;
#endif

//static H264_VLC_RES vlc_res_comp;

typedef struct VLC_BS_Context{
  unsigned int buffer;
  unsigned int index;
}VLC_BS_Context;

static VLC_BS_Context vlc_bs_mxu;

#define VLC_BS_OPEN(gb)\
({\
  unsigned int gb_index  = gb->index;\
  unsigned int gb_buffer = gb->buffer;\
  vlc_bs_mxu.index  = ((gb_buffer << 3) + gb_index) & 31;\
  vlc_bs_mxu.buffer = (gb_buffer + (gb_index >> 3)) & (~3);\
  S32LDDR(xr2, (vlc_bs_mxu.buffer), 0x0);\
  S32LDDR(xr1, (vlc_bs_mxu.buffer), 0x4);\
})

#define VLC_BS_UPDATE()\
({\
  if(vlc_bs_mxu.index >= 32){\
    vlc_bs_mxu.index -= 32;\
    vlc_bs_mxu.buffer += 4;\
    S32OR(xr2, xr1, xr0);\
    S32LDDR(xr1, vlc_bs_mxu.buffer, 4);\
  }\
})

#define VLC_BS_CLOSE(gb)\
({\
  unsigned int vlc_buffer = (unsigned int)vlc_bs_mxu.buffer + (vlc_bs_mxu.index >> 3);\
  gb->index = ((vlc_buffer - (unsigned int)gb->buffer) << 3) + (vlc_bs_mxu.index & 7);\
})


static av_always_inline int get_level_prefix_bs(){
  int val, count;

  S32OR(xr3, xr2, xr0);
  S32EXTR(xr2, xr1, (vlc_bs_mxu.index), 31);
  val = S32M2I(xr2);
  S32OR(xr2, xr3, xr0);

  val <<= 1;
  count = i_clz(val);

  vlc_bs_mxu.index += count + 1;
  VLC_BS_UPDATE();

  return count;
}

static av_always_inline int get_ue_golomb_bs(){
  int val, count;
  S32OR(xr3, xr2, xr0);
  S32EXTR(xr2, xr1, (vlc_bs_mxu.index), 31);
  val = S32M2I(xr2);
  S32OR(xr2, xr3, xr0);

  count = i_clz(val);
  count <<= 1;
  val >>= 32 - count;
  vlc_bs_mxu.index += count - 1;
  VLC_BS_UPDATE();

  return val - 1;
}

static av_always_inline int get_se_golomb_bs(){
  int val, count;
  S32OR(xr3, xr2, xr0);
  S32EXTR(xr2, xr1, (vlc_bs_mxu.index), 31);
  val = S32M2I(xr2);
  S32OR(xr2, xr3, xr0);

  count = i_clz(val);
  count <<= 1;
  val >>= 32 - count;
  vlc_bs_mxu.index += count - 1;
  VLC_BS_UPDATE();

  int val_sign = val & 1;
  val >>= 1;
  if (val_sign) val = -val;

  return val;
}

static av_always_inline void get_se_golomb_mvd(){
  int mvx_d, x_count;
  int mvy_d, y_count;

  S32OR(xr3, xr2, xr0);
  S32EXTR(xr2, xr1, (vlc_bs_mxu.index), 31);
  mvx_d = S32M2I(xr2);
  S32OR(xr2, xr3, xr0);

  x_count = i_clz(mvx_d);
  x_count <<= 1;
  mvx_d >>= 32 - x_count;
  vlc_bs_mxu.index += x_count - 1;
  VLC_BS_UPDATE();

  S32OR(xr3, xr2, xr0);
  S32EXTR(xr2, xr1, (vlc_bs_mxu.index), 31);
  mvy_d = S32M2I(xr2);
  S32OR(xr2, xr3, xr0);

  y_count = i_clz(mvy_d);
  y_count <<= 1;
  mvy_d >>= 32 - y_count;
  vlc_bs_mxu.index += y_count - 1;
  VLC_BS_UPDATE();

  S32I2M(xr5,mvx_d);
  S32I2M(xr6,mvy_d);
  S32I2M(xr9,0x10001);
  S32SFL(xr0,xr6,xr5,xr5,ptn3);
  S32AND(xr6,xr5,xr9);
  D32SLL(xr6,xr6,xr0,xr0,15);
  Q16SLR(xr5,xr5,xr0,xr0,1);
  D16CPS(xr8,xr5,xr6);
}

static av_always_inline unsigned int get_bits_bs(int n){
  register tmp;

  S32OR(xr3, xr2, xr0);
  S32EXTRV(xr2, xr1, (vlc_bs_mxu.index), n);
  tmp = S32M2I(xr2);
  S32OR(xr2, xr3, xr0);

  vlc_bs_mxu.index += n;
  VLC_BS_UPDATE();

  return tmp;
}

static av_always_inline unsigned int get_bits1_bs(){
  return get_bits_bs(1);
}

static av_always_inline int get_te0_golomb_bs(int range){
  if(range==1)      return 0;
  else if(range==2) return get_bits1_bs()^1;
  else              return get_ue_golomb_bs();
}

static av_always_inline int get_bits_count_bs(GetBitContext *s){
  uint32_t vlc_buffer = (uint32_t)vlc_bs_mxu.buffer + (vlc_bs_mxu.index >> 3);
  int index = ((vlc_buffer - (uint32_t)s->buffer) << 3) + (vlc_bs_mxu.index & 7);
  return index;
}

/* un used */
static av_always_inline void align_get_bits_bs(GetBitContext *s)
{
  s->index += ((-(s->index))&7);
}

static av_always_inline int get_vlc2_bs(VLC_TYPE (*table)[2], int bits, int max_depth)
{
  unsigned int code;
  int n, val, nb_bits;

  S32OR(xr3, xr2, xr0);
  S32EXTRV(xr2, xr1, (vlc_bs_mxu.index), bits);
  val = S32M2I(xr2);
  S32OR(xr2, xr3, xr0);

  code = table[val][0];
  n    = table[val][1];

  if(max_depth > 1 && n < 0){
    vlc_bs_mxu.index += bits;
    VLC_BS_UPDATE();

    nb_bits = -n;
    S32OR(xr3, xr2, xr0);
    S32EXTRV(xr2, xr1, (vlc_bs_mxu.index), nb_bits);
    val = S32M2I(xr2);
    S32OR(xr2, xr3, xr0);
    val += code;

    code = table[val][0];
    n    = table[val][1];
  }
  vlc_bs_mxu.index += n;
  VLC_BS_UPDATE();

  return code;
}

static av_always_inline int get_vlc2_bs_max1(VLC_TYPE (*table)[2], int bits)
{
  unsigned int code;
  int n, val;

  S32OR(xr3, xr2, xr0);
  S32EXTRV(xr2, xr1, (vlc_bs_mxu.index), bits);
  val = S32M2I(xr2);
  S32OR(xr2, xr3, xr0);

  code = table[val][0];
  n    = table[val][1];

  vlc_bs_mxu.index += n;
  VLC_BS_UPDATE();

  return code;
}

static av_always_inline int get_vlc2_bs_max2(VLC_TYPE (*table)[2], int bits)
{
  unsigned int code;
  int n, val, nb_bits;

  S32OR(xr3, xr2, xr0);
  S32EXTRV(xr2, xr1, (vlc_bs_mxu.index), bits);
  val = S32M2I(xr2);
  S32OR(xr2, xr3, xr0);

  code = table[val][0];
  n    = table[val][1];

  if(n < 0){
    vlc_bs_mxu.index += bits;
    VLC_BS_UPDATE();

    nb_bits = -n;
    S32OR(xr3, xr2, xr0);
    S32EXTRV(xr2, xr1, (vlc_bs_mxu.index), nb_bits);
    val = S32M2I(xr2);
    S32OR(xr2, xr3, xr0);
    val += code;

    code = table[val][0];
    n    = table[val][1];
  }
  vlc_bs_mxu.index += n;
  VLC_BS_UPDATE();

  return code;
}

#if 0
static av_always_inline clear_blocks_mxu(int addr, int len){
  int32_t mxu_i;               
  int32_t local = (int32_t)(addr)-4;  
  int cline = ((len)>>5);		   
  for (mxu_i=0; mxu_i < cline; mxu_i++) 
    {                                     
      S32SDI(xr0,local,4);              
      S32SDI(xr0,local,4);              
      S32SDI(xr0,local,4);              
      S32SDI(xr0,local,4);              
      S32SDI(xr0,local,4);              
      S32SDI(xr0,local,4);              
      S32SDI(xr0,local,4);                  
      S32SDI(xr0,local,4);              
    }                                     
}
#endif

static av_always_inline
int pred_non_zero_count_cavlc(H264Context *h, int index8){
  //const int index8= scan8[n];
  const int left= h->non_zero_count_cache[index8 - 1];
  const int top = h->non_zero_count_cache[index8 - 8];
  int i= left + top;
  if(i<64) i= (i+1)>>1;
  return i&31;
}

__cache_text1__
//static int decode_residual_bs(H264_VLC_RES *vlc_res, H264Context *h, int n, uint8_t *scantable, uint32_t *qmul, int max_coeff, DCTELEM *block){
static int decode_residual_bs(H264Context *h, /*GetBitContext *gb,*/ DCTELEM *block, int n, const uint8_t *scantable, const uint32_t *qmul, int max_coeff){
  static const int coeff_token_table_index[17]= {0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3};
  int level[16];
  int zeros_left, coeff_num, coeff_token, total_coeff, i, j, trailing_ones, run_before;

  if(n >= CHROMA_DC_BLOCK_INDEX){
    coeff_token= get_vlc2_bs_max1(chroma_dc_coeff_token_vlc.table, CHROMA_DC_COEFF_TOKEN_VLC_BITS);
    total_coeff= coeff_token>>2;
  }else{
    if(n == LUMA_DC_BLOCK_INDEX){
      total_coeff= pred_non_zero_count_cavlc(h, scan8[0]);
      coeff_token= get_vlc2_bs_max2(coeff_token_vlc[ coeff_token_table_index[total_coeff] ].table, COEFF_TOKEN_VLC_BITS);
      total_coeff= coeff_token>>2;
    }else{
      int index8 = scan8[n];
      total_coeff= pred_non_zero_count_cavlc(h, index8);
      coeff_token= get_vlc2_bs_max2(coeff_token_vlc[ coeff_token_table_index[total_coeff] ].table, COEFF_TOKEN_VLC_BITS);
      total_coeff= coeff_token>>2;
      h->non_zero_count_cache[ index8 ]= total_coeff;
    }
  }

  if(total_coeff==0)
    return 0;
  if(total_coeff > (unsigned)max_coeff) {
    return -1;
  }

  trailing_ones= coeff_token&3;
  int bs_index = vlc_bs_mxu.index;

#if 0
  for(i=0; i<trailing_ones; i++){
    level[i]= 1 - (get_bits1_bs() << 1);
  }
#else

  S32OR(xr3, xr2, xr0);
  S32EXTR(xr2, xr1, bs_index, 3);
  D32SLL(xr2,xr3,xr2,xr3,0);
  i = S32M2I(xr3);

  //vlc_bs_mxu.index += trailing_ones;
  //VLC_BS_UPDATE();
  bs_index += trailing_ones;
  if(bs_index >= 32){
    bs_index -= 32;
    //vlc_bs_mxu.index -= 32;  
    vlc_bs_mxu.buffer += 4;	 
    S32OR(xr2, xr1, xr0);			
    S32LDDR(xr1, vlc_bs_mxu.buffer, 4);
  }

  level[0] = 1-((i&4)>>1);
  level[1] = 1-((i&2)   );
  level[2] = 1-((i&1)<<1);  

  //i = trailing_ones;
#endif
  
  
  if(trailing_ones<total_coeff) {
    int level_code, mask;
    int suffix_length = total_coeff > 10 && trailing_ones < 3;
    int val, count;
    S32OR(xr3, xr2, xr0);
    S32EXTR(xr2, xr1, (bs_index), 31);
    D32SLL(xr2,xr3,xr2,xr3,0);
    val = S32M2I(xr3);
    count = i_clz(val);
    int prefix= count - 1;
    bs_index += count;

    if(prefix<14){
      if(suffix_length){
	S32EXTR(xr3, xr0, count+1, 1);
	level_code= (prefix<<1) + S32M2I(xr3);
	bs_index += 1;
      }else
	level_code= (prefix); //part
    }else if(prefix==14){
      if(suffix_length){
	S32EXTR(xr3, xr0, count+1, 1);
	level_code= (prefix<<1) + S32M2I(xr3);
	bs_index += 1;
      }else{
	S32EXTR(xr3, xr0, count+1, 4);
	level_code= prefix + S32M2I(xr3);
	bs_index += 4;
      }
    }else if(prefix==15){
      S32EXTR(xr3, xr0, count+1, 12);
      level_code= (prefix<<suffix_length) + S32M2I(xr3);
      bs_index += 12;
      if(suffix_length==0) level_code+=15; //FIXME doesn't make (much)sense
    }else{
      return -1;
    }
    //vlc_bs_mxu.index += bs_index;
    //VLC_BS_UPDATE();
    if(bs_index >= 32){
	bs_index -= 32;
	//vlc_bs_mxu.index -= 32;  
	vlc_bs_mxu.buffer += 4;	 
	S32OR(xr2, xr1, xr0);			
	S32LDDR(xr1, vlc_bs_mxu.buffer, 4);
    }


    if(trailing_ones < 3) level_code += 2;

    suffix_length = 1;
    if(level_code > 5)
      suffix_length++;

#if 0
    mask = level_code&1;
    level_code = (level_code >> 1) + 1;
    if(mask) level_code = -level_code;
    level[trailing_ones]= level_code;
#else
    mask= -(level_code&1);
    level[trailing_ones]= (((level_code>>1) +1 ) ^ mask) - mask;
#endif
    //i++;

    //bs_index = vlc_bs_mxu.index;
    for(i = trailing_ones + 1;i<total_coeff;i++) {
      static const int suffix_limit[7] = {0,5,11,23,47,95,INT_MAX };
      int val, count;
      S32OR(xr3, xr2, xr0);
      //S32EXTR(xr2, xr1, (vlc_bs_mxu.index), 31);
      S32EXTR(xr2, xr1, bs_index, 31);
      D32SLL(xr2,xr3,xr2,xr3,0);
      val = S32M2I(xr3);
      count = i_clz(val);
      bs_index += count;
      prefix =  count - 1;

      if(prefix<15){
	S32EXTRV(xr3, xr0, count+1, suffix_length);
	level_code= (prefix<<suffix_length) + S32M2I(xr3);
	bs_index += suffix_length;
      }else if(prefix==15){
        S32EXTR(xr3, xr0, count+1, 12);
	level_code= (prefix<<suffix_length) + S32M2I(xr3);
	bs_index += 12;
      }else{
	return -1;
      }
    
      //vlc_bs_mxu.index += bs_index;
      //VLC_BS_UPDATE();
      //if(vlc_bs_mxu.index >= 32){
      if(bs_index >= 32){
	bs_index -= 32;
	//vlc_bs_mxu.index -= 32;  
	vlc_bs_mxu.buffer += 4;	 
	S32OR(xr2, xr1, xr0);			
	S32LDDR(xr1, vlc_bs_mxu.buffer, 4);
      }

      suffix_length += level_code > suffix_limit[suffix_length];

      mask= -(level_code&1);
      level[i]= (((level_code>>1) +1 ) ^ mask) - mask;
    }
    //vlc_bs_mxu.index = bs_index;  
  }
  vlc_bs_mxu.index = bs_index;  

  if(total_coeff == max_coeff)
    zeros_left=0;
  else{
    if(n >= CHROMA_DC_BLOCK_INDEX)
      zeros_left= get_vlc2_bs_max1(chroma_dc_total_zeros_vlc[ total_coeff-1 ].table, CHROMA_DC_TOTAL_ZEROS_VLC_BITS);
    else
      zeros_left= get_vlc2_bs_max1(total_zeros_vlc[ total_coeff-1 ].table, TOTAL_ZEROS_VLC_BITS);
  }

  coeff_num = zeros_left + total_coeff - 1;
  j = scantable[coeff_num];

  if(n > 24){
    block[j] = level[0];
    for(i=1;i<total_coeff;i++) {
      if(zeros_left <= 0)
	run_before = 0;
      else if(zeros_left < 7){
	run_before= get_vlc2_bs_max1(run_vlc[zeros_left-1].table, RUN_VLC_BITS);
      }else{
	run_before= get_vlc2_bs_max2(run7_vlc.table, RUN7_VLC_BITS);
      }
      zeros_left -= run_before;
      coeff_num -= 1 + run_before;
      j= scantable[ coeff_num ];
      block[j]= level[i];
    }
  }else{
    VLC_TYPE (*table)[2];
    VLC_TYPE (*run7_vlc_table)[2];
    int32_t run_before_n = 0;

    run7_vlc_table = run7_vlc.table;
    
    int n, val;    

    bs_index = vlc_bs_mxu.index;    
    block[j] = (level[0] * qmul[j] + 32)>>6;    
    for(i=1;i<total_coeff;i++) {
      if(zeros_left <= 0)
	run_before = 0;
      else if(likely(zeros_left < 7)){
#if 1
        table = run_vlc[zeros_left-1].table;
	S32OR(xr3, xr2, xr0);
	S32EXTRV(xr2, xr1, bs_index, RUN_VLC_BITS);
	val = S32M2I(xr2);
	S32OR(xr2, xr3, xr0);

	//run_before = table[val][0];
	//bs_index += table[val][1];
	run_before_n = *(uint32_t*)&table[val][0];//assumed VLC_TYPE is int16_t
	run_before = run_before_n & 0x0000FFFF;
	bs_index += run_before_n >> 16;

	if(bs_index >= 32){
	  bs_index -= 32;
	  vlc_bs_mxu.buffer += 4;	 
	  S32OR(xr2, xr1, xr0);			
	  S32LDDR(xr1, vlc_bs_mxu.buffer, 4);
	}
#else
	run_before= get_vlc2_bs_max1(run_vlc[zeros_left-1].table, RUN_VLC_BITS);
#endif
      }else{
#if 1
	int nb_bits;

	S32OR(xr3, xr2, xr0);
	S32EXTRV(xr2, xr1, bs_index, RUN7_VLC_BITS);
	val = S32M2I(xr2);
	S32OR(xr2, xr3, xr0);

	//run_before = run7_vlc_table[val][0];
	//n    = run7_vlc_table[val][1];
	run_before_n = *(uint32_t*)&run7_vlc_table[val][0];//assumed VLC_TYPE is int16_t
	run_before = run_before_n & 0x0000FFFF;
	n = run_before_n >> 16;

	if(n < 0){
	  bs_index += RUN7_VLC_BITS;
	  if(bs_index >= 32){
	    bs_index -= 32;
	    vlc_bs_mxu.buffer += 4;	 
	    S32OR(xr2, xr1, xr0);			
	    S32LDDR(xr1, vlc_bs_mxu.buffer, 4);
	  }

	  nb_bits = -n;
	  S32OR(xr3, xr2, xr0);
	  S32EXTRV(xr2, xr1, bs_index, nb_bits);
	  val = S32M2I(xr2);
	  S32OR(xr2, xr3, xr0);
	  val += run_before;

	  //run_before = run7_vlc_table[val][0];
	  //n    = run7_vlc_table[val][1];
	  run_before_n = *(uint32_t*)&run7_vlc_table[val][0];//assumed VLC_TYPE is int16_t
	  run_before = run_before_n & 0x0000FFFF;
	  n = run_before_n >> 16;
	}

	bs_index += n;
	if(bs_index >= 32){
	  bs_index -= 32;
          vlc_bs_mxu.buffer += 4;	 
          S32OR(xr2, xr1, xr0);			
          S32LDDR(xr1, vlc_bs_mxu.buffer, 4);
        }

#else
	run_before= get_vlc2_bs_max2(run7_vlc.table, RUN7_VLC_BITS);
#endif
      }
      zeros_left -= run_before;
      coeff_num -= 1 + run_before;
      j= scantable[ coeff_num ];
      block[j]= (level[i] * qmul[j] + 32)>>6;
    }

    vlc_bs_mxu.index = bs_index;  
  }

  if(zeros_left<0){
    return -1;
  }

  return 0;
}
