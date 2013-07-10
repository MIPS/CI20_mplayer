#include "vlc_hw.h"
#define VLC_TYPE_8B int8_t

#define NOP_N(){\
    CPU_GET_RSLT();				\
}
#define likely(x)  __builtin_expect((x),1)
#define unlikely(x)  __builtin_expect((x),0)
#define SET_GET_BITS(n){\
    unsigned int get_bit_len, vlc_len, vlc_mode, cfg_mode, vlc_en, ctrl_val; \
    vlc_en = 1; vlc_mode = 2; cfg_mode = 1; get_bit_len = (n-1) & 0x7;	\
    ctrl_val = ((get_bit_len << 4) | (cfg_mode << 3) | (vlc_mode << 1) | (vlc_en << 0) ); \
    CPU_SET_CTRL(ctrl_val);						\
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
#define CALC_VLC2_CTRL_INTRA() ({		\
  unsigned int cfg_mode, vlc_en;\
  unsigned int tbl_mode, bit_len, table_ofst;\
\
  tbl_mode = 0;\
  bit_len = 7 - 1; table_ofst = 0; \
\
  vlc_en = 1; cfg_mode = 1;\
  ((table_ofst << 9) | (tbl_mode<<8) | (bit_len << 4) | (cfg_mode << 3) | (vlc_en << 0) );\
})
#define CALC_VLC2_CTRL_INTER() ({		\
  unsigned int cfg_mode, vlc_en;\
  unsigned int tbl_mode, bit_len, table_ofst;\
\
  tbl_mode = 0;\
  bit_len = 7 - 1; table_ofst = 0x100; \
\
  vlc_en = 1; cfg_mode = 1;\
  ((table_ofst << 9) | (tbl_mode<<8) | (bit_len << 4) | (cfg_mode << 3) | (vlc_en << 0) );\
})
#define CALC_SEC_PAT_CTRL(lvl0_len,table_ofst) ({		\
  unsigned int cfg_mode, vlc_en;\
  unsigned int tbl_mode, bit_len;\
\
  tbl_mode = 0;\
  bit_len = lvl0_len - 1; \
\
  vlc_en = 1; cfg_mode = 1;\
  ((table_ofst << 9) | (tbl_mode<<8) | (bit_len << 4) | (cfg_mode << 3) | (vlc_en << 0) );\
})

#define init_hw_bits(bs_len){\
int ii;\
uint8_t * bs_src = s->gb.buffer;\
uint8_t * bs_dst = rv9_sw_bs_buffer;\
for(ii=0; ii<bs_len; ii++)\
  bs_dst[ii] = bs_src[ii];\
}

#define sync_hw_bits(gb){\
unsigned int sw_addr, sw_ofst, sw_acc_pos;\
rv9_hw_bs_buffer=get_phy_addr(gb->buffer);\
sw_addr = rv9_hw_bs_buffer;	  \
sw_ofst = gb->index;\
unsigned int bs_addr, bs_ofst;\
bs_addr = (sw_addr + (sw_ofst >> 3)) & 0xFFFFFFFC;\
sw_ofst+=(sw_addr&3)<<3;\
bs_ofst = sw_ofst & 0x1F;\
\
CPU_SET_BS_ADDR(bs_addr);\
\
unsigned int bs_ofst_val = (1 << 16) | (bs_ofst << 5);\
CPU_SET_BS_OFST(bs_ofst_val);\
unsigned int bs_init_done;\
do {\
  bs_init_done = CPU_GET_BS_OFST() & (1<<16);\
 } while (bs_init_done);\
  }

#define sync_sw_bits(gb){\
unsigned int bs_addr, bs_ofst;\
bs_ofst = (CPU_GET_BS_OFST() >> 5) & 0x1F;\
bs_addr = CPU_GET_BS_ADDR() + (bs_ofst >> 3);\
bs_ofst &= 0x7;\
unsigned int sw_addr, sw_ofst, sw_acc_pos;\
\
sw_addr = rv9_hw_bs_buffer;\
sw_ofst = ((bs_addr - sw_addr) << 3) + bs_ofst ;\
\
 gb->index = sw_ofst;		\
  }

#define check_sync(gb)({			\
unsigned int bs_addr, bs_ofst;\
bs_ofst = (CPU_GET_BS_OFST() >> 5) & 0x1F;		\
bs_addr = CPU_GET_BS_ADDR() + (bs_ofst >> 3);		\
bs_ofst &= 0x7;						\
unsigned int sw_addr, sw_ofst, sw_acc_pos;		\
sw_addr = rv9_hw_bs_buffer;				\
sw_ofst = ((bs_addr - sw_addr) << 3) + bs_ofst ;	\
 if(sw_ofst != gb->index) printf("%s line %d  -------- check bs error !! sw_ofst: %d; s->index: %d \n",__FILE__,__LINE__,sw_ofst,gb->index); \
\
(sw_ofst==gb->index);\
    })

#define get_bs_index(gb)({\
unsigned int bs_addr, bs_ofst;\
bs_ofst = (CPU_GET_BS_OFST() >> 5) & 0x1F;\
bs_addr = CPU_GET_BS_ADDR() + (bs_ofst >> 3);\
bs_ofst &= 0x7;\
unsigned int sw_addr, sw_ofst, sw_acc_pos;\
sw_addr = rv9_hw_bs_buffer;\
sw_ofst = ((bs_addr - sw_addr) << 3) + bs_ofst ;\
\
sw_ofst;\
    })

static av_always_inline unsigned int get_bits_hw(int n){
  unsigned int get_bit_len, vlc_len, vlc_mode, cfg_mode, vlc_en, ctrl_val;
  int hw_rslt;

  unsigned int hh_val = 0;
  vlc_en = 1; vlc_mode = 2; cfg_mode = 1;;
  if(unlikely(n > 8)){
    get_bit_len = 7;
    ctrl_val = ((get_bit_len << 4) | (cfg_mode << 3) | (vlc_mode << 1) | (vlc_en << 0) );
    CPU_SET_CTRL(ctrl_val);
    NOP_N();
    hw_rslt = CPU_GET_RSLT();
    hh_val = hw_rslt;
    get_bit_len = (n-1) & 0x7;
    ctrl_val = ((get_bit_len << 4) | (cfg_mode << 3) | (vlc_mode << 1) | (vlc_en << 0) );
    CPU_SET_CTRL(ctrl_val);
    NOP_N();
    hw_rslt = CPU_GET_RSLT();
    hw_rslt |= hh_val << (n - 8);
  }
  else{
  get_bit_len = (n-1) & 0x7;
  ctrl_val = ((get_bit_len << 4) | (cfg_mode << 3) | (vlc_mode << 1) | (vlc_en << 0) );
  CPU_SET_CTRL(ctrl_val);
  NOP_N();
  hw_rslt = CPU_GET_RSLT();
  }
  return (unsigned int)hw_rslt;
}

static av_always_inline unsigned int get_bits1_hw(){
  unsigned int ctrl_val;
  int hw_rslt;

  ctrl_val = ((0 << 4) | (1 << 3) | (2 << 1) | (1 << 0) );
  CPU_SET_CTRL(ctrl_val);
  NOP_N();
  hw_rslt = CPU_GET_RSLT();

  return (unsigned int)hw_rslt;
}

static av_always_inline unsigned int show_ubits_hw(int n){
  int hw_rslt;
  unsigned int bit_len, vlc_len, vlc_mode, cfg_mode, vlc_en, ctrl_val;
  vlc_en = 1; vlc_mode = 3; cfg_mode = 1; bit_len = n-1;
  ctrl_val = ((bit_len << 4) | (cfg_mode << 3) | (vlc_mode << 1) | (vlc_en << 0) );
  CPU_SET_CTRL(ctrl_val);
  NOP_N();
  hw_rslt = CPU_GET_RSLT();
  return (unsigned int)hw_rslt;
};

static av_always_inline void skip_bits_hw(int n){
  unsigned int bit_len, vlc_len, vlc_mode, cfg_mode, vlc_en, ctrl_val;
  vlc_en = 1; vlc_mode = 2; cfg_mode = 1; bit_len = n-1;
  ctrl_val = ((bit_len << 4) | (cfg_mode << 3) | (vlc_mode << 1) | (vlc_en << 0) );
  CPU_SET_CTRL(ctrl_val);
  NOP_N();
  NOP_N();
};

static av_always_inline int get_vlc2_hw(VLC_TYPE (*table)[2], int bits, int max_depth) {
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

  }

  skip_bits_hw(n);
  return code;
}
