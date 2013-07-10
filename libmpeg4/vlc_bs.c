#include "vlc_hw.h"

#define NOP_N(){\
    i_nop;				\
    i_nop;				\
    i_nop;				\
}
#define NOP_N_4(){\
    i_nop;				\
    i_nop;				\
    i_nop;				\
    i_nop;				\
}
#define likely(x)  __builtin_expect((x),1)
#define unlikely(x)  __builtin_expect((x),0)
#define SET_GET_BITS(n){\
    unsigned int get_bit_len, vlc_len, vlc_mode, cfg_mode, vlc_en, ctrl_val; \
    vlc_en = 1; vlc_mode = 2; cfg_mode = 1; get_bit_len = (n-1) & 0x7;	\
    ctrl_val = ((get_bit_len << 4) | (cfg_mode << 3) | (vlc_mode << 1) | (vlc_en << 0) ); \
    CPU_SET_CTRL(ctrl_val);						\
  }
#define SET_CFG_BITS(n){\
    unsigned int get_bit_len, reset, cfg_val; \
    get_bit_len = (n-1) & 0x7;reset=0;					\
    cfg_val = ((get_bit_len << 4) | (reset << 0));			\
    CPU_SET_CFG(cfg_val);						\
  }
#define SET_EN_CFG(){\
    CPU_SET_CTRL(0x5);				\
  }
#define GET_S_BITS(n)({				\
    SET_GET_BITS(n);				\
    NOP_N();					\
    CPU_GET_RSLT();				\
    })
#define GET_L_BITS(n)({				\
    int h_rslt,l_rslt;\
    SET_GET_BITS(8);				\
    NOP_N();\
    h_rslt=CPU_GET_RSLT();\
    SET_GET_BITS(n);				\
    NOP_N();\
    l_rslt=CPU_GET_RSLT();   \
    (l_rslt | (h_rslt << (n - 8)));		\
    })
#define GET_BITS(n)({				\
    int h_rslt,l_rslt;\
    if(unlikely(n>8)){				\
    SET_GET_BITS(8);				\
    NOP_N();\
    h_rslt=CPU_GET_RSLT();\
    SET_GET_BITS(n);				\
    NOP_N();\
    l_rslt=CPU_GET_RSLT();   \
    l_rslt |= (h_rslt << (n - 8));\
    }\
    else{\
    SET_GET_BITS(n);				\
    NOP_N();\
    l_rslt=CPU_GET_RSLT();   \
    }\
    l_rslt;					\
    })
#define CALC_VLC2_CTRL(codingset) ({		\
  unsigned int cfg_mode, vlc_en;\
  unsigned int tbl_mode, bit_len, table_ofst;\
\
  tbl_mode = cfg_all_table[codingset].table_mode;\
  bit_len = cfg_all_table[codingset].lvl0_len - 1; table_ofst = (codingset == vc1_hw_codingset1) ? 0 : 0x400;\
\
  vlc_en = 1; cfg_mode = 1;\
  ((table_ofst << 9) | (tbl_mode<<8) | (bit_len << 4) | (cfg_mode << 3) | (vlc_en << 0) );\
})

static av_always_inline void skip_bits_hw(int n){
  unsigned int bit_len, vlc_len, vlc_mode, cfg_mode, vlc_en, ctrl_val;
  vlc_en = 1; vlc_mode = 2; cfg_mode = 1; bit_len = n-1;
  ctrl_val = ((bit_len << 4) | (cfg_mode << 3) | (vlc_mode << 1) | (vlc_en << 0) );
  CPU_SET_CTRL(ctrl_val);
  CPU_GET_RSLT();
};

static av_always_inline void align_get_bits_hw(GetBitContext *s)
{
    int n= (s->index) & 7;
    if(n) skip_bits_hw(n);
}

static av_always_inline unsigned int get_bits_hw(GetBitContext *s, int n){
  unsigned int get_bit_len, vlc_len, vlc_mode, cfg_mode, vlc_en, ctrl_val;
  int hw_rslt;

  unsigned int hh_val = 0;
  vlc_en = 1; vlc_mode = 2; cfg_mode = 1;;
  if(unlikely(n > 8)){
    get_bit_len = 7;
    ctrl_val = ((get_bit_len << 4) | (cfg_mode << 3) | (vlc_mode << 1) | (vlc_en << 0) );
    CPU_SET_CTRL(ctrl_val);
    CPU_GET_RSLT();
    hw_rslt = CPU_GET_RSLT();
    hh_val = hw_rslt;
    get_bit_len = (n-1) & 0x7;
    ctrl_val = ((get_bit_len << 4) | (cfg_mode << 3) | (vlc_mode << 1) | (vlc_en << 0) );
    CPU_SET_CTRL(ctrl_val);
    CPU_GET_RSLT();
    hw_rslt = CPU_GET_RSLT();
    hw_rslt |= hh_val << (n - 8);
  }
  else{
  get_bit_len = (n-1) & 0x7;
  ctrl_val = ((get_bit_len << 4) | (cfg_mode << 3) | (vlc_mode << 1) | (vlc_en << 0) );
  CPU_SET_CTRL(ctrl_val);
  CPU_GET_RSLT();
  hw_rslt = CPU_GET_RSLT();
  }
  return (unsigned int)hw_rslt;
}

static av_always_inline unsigned int get_bits1_hw(GetBitContext *s){
  CPU_SET_CTRL(0xd);
  CPU_GET_RSLT();

  return (unsigned int)CPU_GET_RSLT();
}

static av_always_inline int decode210_hw(GetBitContext *gb){
  if (get_bits1_hw(gb))
    return 0;
  else
    return 2 - get_bits1_hw(gb);
}

static av_always_inline int decode012_hw(GetBitContext *gb){
    int n;
    n = get_bits1_hw(gb);
    if (n == 0)
        return 0;
    else
        return get_bits1_hw(gb) + 1;
}

static av_always_inline int get_unary_hw(GetBitContext *gb, int stop, int len)
{
  int i;
  for(i = 0; i < len && get_bits1_hw(gb) != stop; i++);
  return i;
}

static av_always_inline unsigned int show_ubits_hw(int n){
  int hw_rslt;
  unsigned int bit_len, vlc_len, vlc_mode, cfg_mode, vlc_en, ctrl_val;
  vlc_en = 1; vlc_mode = 3; cfg_mode = 1; bit_len = n-1;
  ctrl_val = ((bit_len << 4) | (cfg_mode << 3) | (vlc_mode << 1) | (vlc_en << 0) );
  CPU_SET_CTRL(ctrl_val);
  CPU_GET_RSLT();
  hw_rslt = CPU_GET_RSLT();
  return (unsigned int)hw_rslt;
}

static av_always_inline unsigned int get_xbits_hw(GetBitContext *s, int n){
#if 1
    register int sign;
    register int32_t cache;
    cache =  get_bits_hw(&s->index,n);  
    //skip_bits_hw(n);
#if 0
    cache=(cache<<(32-n)); 
    sign=(~cache)>>(31); 
    return (((unsigned int)(sign ^ cache) >> (32 - n)) ^ sign) - sign;
#else
    if(cache>>(n-1))
        return cache;
    else
        return ((0xffffffff<<n)|cache)+1;     
#endif
#endif
}

static av_always_inline int get_vlc2_hw(GetBitContext *s, VLC_TYPE (*table)[2], int bits, int max_depth) {
  int code;
  int n, index, nb_bits;
  index= show_ubits_hw(bits);
  code = table[index][0];
  n    = table[index][1];

  if(max_depth > 1 && n < 0){
    skip_bits_hw(bits);
    nb_bits = -n;
    index= show_ubits_hw(nb_bits) + code;
    code = table[index][0];
    n    = table[index][1];

    if(max_depth > 2 && n < 0){
      skip_bits_hw(nb_bits);
      nb_bits = -n;
      index= show_ubits_hw(nb_bits) + code;
      code = table[index][0];
      n    = table[index][1];

      if(max_depth > 3 && n < 0){
	skip_bits_hw(nb_bits);
	nb_bits = -n;
	index= show_ubits_hw(nb_bits) + code;
	code = table[index][0];
	n    = table[index][1];
      }
    }
  }

  skip_bits_hw(n);
  return code;
}

static av_always_inline int get_vlc2_hw_m4(VLC_TYPE (*table)[2]) {
  int code;
  int n, index, nb_bits;
  index= show_ubits_hw(8);
  code = table[index][0];
  n    = table[index][1];

  if( n < 0){
    CPU_SET_CTRL(0x7d);
    NOP_N();
    //skip_bits_hw(8);
    nb_bits = -n;
    index= show_ubits_hw(nb_bits) + code;
    code = table[index][0];
    n    = table[index][1];

    if( n < 0){
      CPU_SET_CTRL(0x7d);
      NOP_N();
      //skip_bits_hw(8);
      nb_bits = -n;
      index= show_ubits_hw(nb_bits) + code;
      code = table[index][0];
      n    = table[index][1];

      if(n < 0){
	skip_bits_hw(nb_bits);
	nb_bits = -n;
	index= show_ubits_hw(nb_bits) + code;
	code = table[index][0];
	n    = table[index][1];
      }
    }
  }

  skip_bits_hw(n);
  return code;
}

static av_always_inline int get_vlc2_tbl_hw(void) {
  int hw_rslt;
  //unsigned int vlc_len, vlc_mode, cfg_mode, vlc_en, ctrl_val;
  //unsigned int tbl_mode, bit_len, table_ofst, cfg_val;

  //tbl_mode = 0;
  //bit_len = 5; table_ofst = 0;

  //vlc_en = 1; vlc_mode = 0; cfg_mode = 1;
  //ctrl_val = ((table_ofst << 9) | (tbl_mode<<8) | (bit_len << 4) |
  //(cfg_mode << 3) | (vlc_mode << 1) | (vlc_en << 0) );
  CPU_SET_CTRL(0x59);
  CPU_GET_RSLT();
  hw_rslt = CPU_GET_RSLT();

  return hw_rslt;
}
#undef printf
static av_always_inline int get_vlc2_tbl_hw_cbpy(void) {
  int hw_rslt;
  //unsigned int vlc_len, vlc_mode, cfg_mode, vlc_en, ctrl_val;
  //unsigned int tbl_mode, bit_len, table_ofst, cfg_val;

  //tbl_mode = 0;
  //bit_len = 5; table_ofst = 256;

  //vlc_en = 1; vlc_mode = 0; cfg_mode = 1;
  //ctrl_val = ((table_ofst << 9) | (tbl_mode<<8) | (bit_len << 4) |
  //(cfg_mode << 3) | (vlc_mode << 1) | (vlc_en << 0) );
  CPU_SET_CTRL(0x20059);
  CPU_GET_RSLT();
  hw_rslt = CPU_GET_RSLT();

  return hw_rslt;
}

static av_always_inline int get_dc_vlc_lum(void) {
  int hw_rslt;
  //unsigned int vlc_len, vlc_mode, cfg_mode, vlc_en, ctrl_val;
  //unsigned int tbl_mode, bit_len, table_ofst, cfg_val;

  //tbl_mode = 0;
  //bit_len = 7; table_ofst = 512;

  //vlc_en = 1; vlc_mode = 0; cfg_mode = 1;
  //ctrl_val = ((table_ofst << 9) | (tbl_mode<<8) | (bit_len << 4) |
  //(cfg_mode << 3) | (vlc_mode << 1) | (vlc_en << 0) );
  //printf("lum %d\n", ctrl_val);
  //CPU_SET_CTRL(ctrl_val);
  CPU_SET_CTRL(0x40079);
  CPU_GET_RSLT();
  //NOP_N();
  hw_rslt = CPU_GET_RSLT();

  return hw_rslt;
}

static av_always_inline int get_dc_vlc_chrom(void) {
  int hw_rslt;
  //unsigned int vlc_len, vlc_mode, cfg_mode, vlc_en, ctrl_val;
  //unsigned int tbl_mode, bit_len, table_ofst, cfg_val;

  //tbl_mode = 0;
  //bit_len = 7; table_ofst = 768;

  //vlc_en = 1; vlc_mode = 0; cfg_mode = 1;
  //ctrl_val = ((table_ofst << 9) | (tbl_mode<<8) | (bit_len << 4) |
  //(cfg_mode << 3) | (vlc_mode << 1) | (vlc_en << 0) );
  //printf("lum %d\n", ctrl_val);
  //CPU_SET_CTRL(ctrl_val);
  CPU_SET_CTRL(0x60079);
  //NOP_N();
  CPU_GET_RSLT();
  hw_rslt = CPU_GET_RSLT();

  return hw_rslt;
}

static av_always_inline int get_rl_tbl_hw_intra(void) {
  int hw_rslt;
  //unsigned int vlc_len, vlc_mode, cfg_mode, vlc_en, ctrl_val;
  //unsigned int tbl_mode, bit_len, table_ofst, cfg_val;

  //tbl_mode = 0;
  //bit_len = 7; table_ofst = 1024;

  //vlc_en = 1; vlc_mode = 0; cfg_mode = 1;
  //ctrl_val = ((table_ofst << 9) | (tbl_mode<<8) | (bit_len << 4) |
  //(cfg_mode << 3) | (vlc_mode << 1) | (vlc_en << 0) );
  CPU_SET_CTRL(0x80079);
  CPU_GET_RSLT();
  hw_rslt = CPU_GET_RSLT();
	if (hw_rslt>102)
		hw_rslt=102;

  return hw_rslt;
}

static av_always_inline int get_rl_tbl_hw_inter(void) {
  int hw_rslt;
  unsigned int vlc_len, vlc_mode, cfg_mode, vlc_en, ctrl_val;
  unsigned int tbl_mode, bit_len, table_ofst, cfg_val;

  tbl_mode = 0;
  bit_len = 7; table_ofst = 2048;

  vlc_en = 1; vlc_mode = 0; cfg_mode = 1;
  ctrl_val = ((table_ofst << 9) | (tbl_mode<<8) | (bit_len << 4) |
	      (cfg_mode << 3) | (vlc_mode << 1) | (vlc_en << 0) );
  CPU_SET_CTRL(ctrl_val);
  NOP_N();
  hw_rslt = CPU_GET_RSLT();

  return hw_rslt;
}

#define GET_RL_VLC_HW(level, run, name, gb, table, bits, max_depth, need_update) \
  {									\
    int n, nb_bits;							\
    unsigned int index;							\
									\
    index= show_ubits_hw(bits);						\
    level = table[index].level;						\
    n     = table[index].len;						\
    if(max_depth > 1 && n < 0){						\
      skip_bits_hw(bits);						\
      nb_bits = -n;							\
      index= show_ubits_hw(nb_bits)+ level;				\
      level = table[index].level;					\
      n     = table[index].len;						\
      if(max_depth > 2 && n < 0){					\
        skip_bits_hw(nb_bits);						\
        nb_bits = -n;							\
        index= show_ubits_hw(nb_bits)+level;				\
        level = table[index].level;					\
        n  = table[index].len;						\        
      }                                                                 \
    }									\
    run= table[index].run;						\
    skip_bits_hw(n);							\
  }
