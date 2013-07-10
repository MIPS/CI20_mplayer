
#ifndef __JZC_CABAC_HW_H__
#define __JZC_CABAC_HW_H__

#include "../../libjzcommon/jzasm.h"
//#include "../../libjzcommon/jzsys.h"
#include "h264_tcsm0.h"


#define get_tcsm0_paddr(vaddr) ((((unsigned int)(vaddr)) & 0x0000FFFF) | 0x132B0000)
#define get_tcsm1_paddr(vaddr) ((((unsigned int)(vaddr)) & 0x0000FFFF) | 0x132C0000)


#define get_cabac get_cabac_hw
#define get_cabac_noinline get_cabac_hw
#define get_cabac_bypass get_cabac_bypass_hw
#define get_cabac_bypass_sign get_cabac_bypass_sign_hw
#define get_cabac_terminate get_cabac_terminate_hw
#define decode_cabac_mb_mvd decode_cabac_mb_mvd_hw


#define CABAC_HW_BS_MAXSIZE (1024*64)

#define CABAC_VBASE 0xB3290000
#define CABAC_PBASE 0x13290000

#define CABAC_CTRL_OFST		0x0
#define CABAC_BIT_VAL_OFST	0x4
#define CABAC_BS_ADDR_OFST	0x8
#define CABAC_BS_OFST_OFST	0xc
#define CABAC_MB_TYPE_OFST	0x10
#define CABAC_NNZ_CBP_OFST	0x14
#define CABAC_MB_DQ_OFST	0x18
#define CABAC_RANGE_CLOW_OFST	0x1C
#define CABAC_MB_DQ_H16_OFST	0x20
#define CABAC_EN_AND_CGG_OFST	0x24
#define CABAC_CTX_TBL_OFST	0x1400

#define CABAC_CPU_IF_CTRL	0x0
#define CABAC_CPU_IF_RSLT	0x1
#define CABAC_CPU_IF_BSADDR	0x2
#define CABAC_CPU_IF_BSOFST	0x3
#define CABAC_CPU_IF_MBTYPE	0x4
#define CABAC_CPU_IF_NNZCBP	0x5
#define CABAC_CPU_IF_RESDQ	0x6
#define CABAC_CPU_IF_RANGE	0x7


#define CPU_WR_CABAC(ofst,data) {i_mtc0_2(data, 21, ofst);}
#define CPU_RD_CABAC(ofst) i_mfc0_2(21, ofst)//if RD follow WR to the same reg, there must be two nops to seperate them

#define CPU_SET_CTRL(res_out_done, reload, reset, slice_init, residual_done, residual_en, bypass_en, decision_en, burst, ctx) \
  ({CPU_WR_CABAC(CABAC_CPU_IF_CTRL,					\
		 (((res_out_done&0x1)<<29) + ((reload&0x3f)<<23) + ((decision_en & 1) << 16) + \
		  ((bypass_en & 1) << 17) +				\
		  ((residual_en & 1) << 19) + ((residual_done & 1) << 20) + \
		  ((slice_init & 1) << 21) + ((reset & 1) << 22) + ((burst&0x3)<<9) + (ctx)));})

#define CPU_SET_MVD_EN(mvd_en, mvd_init_ctx)				\
  ({CPU_WR_CABAC(CABAC_CPU_IF_CTRL,					\
		 (((mvd_en&0x1)<<18) + (1 << 16) + ((mvd_init_ctx)<<11)));})

#define CPU_POLL_MVD()				\
  {							\
    while(!(CPU_RD_CABAC(CABAC_CPU_IF_CTRL)&0x40000000));	\
  }

#define CPU_GET_MVD()				\
  CPU_RD_CABAC(CABAC_CPU_IF_BSOFST)

#define CPU_SET_HYBRID_PARA(a)				\
  CPU_WR_CABAC(CABAC_CPU_IF_MBTYPE,(a))

/*
  |        hybrid_command==0        |        IDLE                        |
  |        hybrid_command==1        |        DECODE_REF(para)            |
  |        hybrid_command==2        |        DECODE_P_SUB                |
  |        hybrid_command==3        |        DECODE_B_SUB                |
  |        hybrid_command==4        |        DECODE_INTRA4X4_MODE(para)  |
  |        hybrid_command==5        |        DECODE_CBP(para)            |
  |        hybrid_command==6        |        DECODE_P_MBTYPE             |
  |        hybrid_command==7        |        DECODE_B_MBTYPE(para)       |
 */

#define CPU_SET_HYBRID_CMD(a)				\
  {CPU_WR_CABAC(CABAC_CPU_IF_CTRL,(((a)&3)<<14) | (((a)&4)<<27) | (1<<16)); i_nop; i_nop;}

#define CPU_POLL_HYBRID()			\
  {								\
    while(!(CPU_RD_CABAC(CABAC_CPU_IF_CTRL)&0x80000000));	\
  }

#define CPU_GET_HYBRID()			\
  CPU_RD_CABAC(CABAC_CPU_IF_MBTYPE)

#define CPU_POLL_RES_DEC_DONE()				\
  {							\
    while(!(CPU_RD_CABAC(CABAC_CPU_IF_CTRL)&0x100000));	\
  }

#define CPU_POLL_RES_OUT_DONE()				\
  {							\
    while(!(CPU_RD_CABAC(CABAC_CPU_IF_NNZCBP)&0x8000));	\
  }

#define CPU_GET_RSLT() CPU_RD_CABAC(CABAC_CPU_IF_RSLT)

#define CPU_GET_RANGE() CPU_RD_CABAC(CABAC_CPU_IF_RANGE)

#define CPU_POLL_SLICE_INIT() \
  {\
    while(CPU_RD_CABAC(CABAC_CPU_IF_CTRL)&0x200000);	\
  }

#define CPU_POLL_BIT_DONE() \
  ({	int tmp;						\
    do {tmp = i_mfc0_2(21, (CABAC_CPU_IF_RSLT));		\
      i_nop; i_nop; i_nop;					\
    }while (tmp < 0);						\
    })

#define CPU_SET_BSADDR(ADDR)			\
  ({CPU_WR_CABAC(CABAC_CPU_IF_BSADDR,ADDR);})

#define CPU_SET_BSOFST(OFST)			\
  ({CPU_WR_CABAC(CABAC_CPU_IF_BSOFST,OFST);})


#define POLL_SLICE_INIT() \
  {\
    while(read_reg(CABAC_VBASE, CABAC_CTRL_OFST)&0x200000);	\
  }

#define SET_CTRL_CTX(decision_en, bypass_en, terminate_en, residual_en, slice_init, reset, ctx) \
  ({write_reg((CABAC_VBASE + CABAC_CTRL_OFST),				\
	      (((decision_en & 1) << 16) + ((bypass_en & 1) << 17) +	\
	       ((terminate_en & 1) << 18) + ((residual_en & 1) << 19) +	\
	       ((slice_init & 1) << 21) + ((reset & 1) << 22) + (ctx & 0x1ff)));})

#define SET_OVERALL_EN_AND_DISABLE_CKG(en,disable_ckg) ({write_reg((CABAC_VBASE + CABAC_EN_AND_CGG_OFST), \
								   ((disable_ckg&0x1)<<1)|(en&0x1));})

#define SET_BS_ADDR(bs_addr) ({write_reg((CABAC_VBASE + CABAC_BS_ADDR_OFST), bs_addr);})

#define SET_BS_OFST(bs_ofst) ({write_reg((CABAC_VBASE + CABAC_BS_OFST_OFST), bs_ofst);})

#define SET_MB_TYPE(mb_type) ({write_reg((CABAC_VBASE + CABAC_MB_TYPE_OFST), mb_type);})

#define SET_CBP(cbp) ({write_reg((CABAC_VBASE + CABAC_CBP_OFST), cbp);})

#define SET_NNZ_ADDR(nnz_addr) ({write_reg((CABAC_VBASE + CABAC_NNZ_ADDR_OFST), nnz_addr);})

#define SET_DQ_ADDR(dq_addr) ({write_reg((CABAC_VBASE + CABAC_DQ_ADDR_OFST), dq_addr);})

#define SET_MB_ADDR(mb_addr) ({write_reg((CABAC_VBASE + CABAC_MB_ADDR_OFST), mb_addr);})

#define SET_RANGE(range) ({write_reg((CABAC_VBASE + CABAC_RANGE_OFST), range);})

#define READ_INIT_END() ({read_reg((CABAC_VBASE + CABAC_CTRL_OFST), 0) & (1<<21);})

#define READ_RESIDUAL_END() ({read_reg((CABAC_VBASE + CABAC_CTRL_OFST), 0) & (1<<20);})

#define READ_CABAC_RZLT() ({read_reg((CABAC_VBASE + CABAC_BIT_VAL_OFST), 0);})

#define READ_CTRL() ({read_reg((CABAC_VBASE + CABAC_CTRL_OFST), 0);})

#define READ_BIT_VAL() ({read_reg((CABAC_VBASE + CABAC_BIT_VAL_OFST), 0);})

#define READ_BS_ADDR() ({read_reg((CABAC_VBASE + CABAC_BS_ADDR_OFST), 0);})


static const unsigned int lps_comb[128]={
  0xefcfaffe,0xefcfafff,0xe2c4a6fe,0xe2c4a6ff,0xd7ba9dfe,0xd7ba9dff,0xccb195f4,0xccb195f5,
  0xc2a88de6,0xc2a88de7,0xb89f86dc,0xb89f86dd,0xae977fd0,0xae977fd1,0xa58f79c6,0xa58f79c7,
  0x9d8873bc,0x9d8873bd,0x95816db2,0x95816db3,0x8d7a67a8,0x8d7a67a9,0x867462a0,0x867462a1,
  0x7f6e5d98,0x7f6e5d99,0x79685890,0x79685891,0x73635488,0x73635489,0x6d5e4f82,0x6d5e4f83,
  0x67594b7a,0x67594b7b,0x62554774,0x62554775,0x5d50446e,0x5d50446f,0x584c4068,0x584c4069,
  0x54483d64,0x54483d65,0x4f443a5e,0x4f443a5f,0x4b41375a,0x4b41375b,0x473e3454,0x473e3455,
  0x443a3150,0x443a3151,0x40372f4c,0x40372f4d,0x3d352c48,0x3d352c49,0x3a322a44,0x3a322a45,
  0x372f2840,0x372f2841,0x342d263e,0x342d263f,0x312a243a,0x312a243b,0x2f282238,0x2f282239,
  0x2c262034,0x2c262035,0x2a241e32,0x2a241e33,0x28221d2e,0x28221d2f,0x26201b2c,0x26201b2d,
  0x241f1a2a,0x241f1a2b,0x221d1928,0x221d1929,0x201c1726,0x201c1727,0x1e1a1624,0x1e1a1625,
  0x1d191522,0x1d191523,0x1b181420,0x1b181421,0x1a16131e,0x1a16131f,0x1815121c,0x1815121d,
  0x1714111a,0x1714111b,0x1613101a,0x1613101b,0x15120f18,0x15120f19,0x14110e16,0x14110e17,
  0x13100d16,0x13100d17,0x120f0d14,0x120f0d15,0x110e0c14,0x110e0c15,0x100e0b12,0x100e0b13,
  0xf0d0b12,0xf0d0b13,0xe0c0a10,0xe0c0a11,0xd0b0a10,0xd0b0a11,0xd0b090e,0xd0b090f,
  0xc0a080e,0xc0a080f,0xb0a080c,0xb0a080d,0xb09080c,0xb09080d,0xa09070c,0xa09070d,
  0xa08070a,0xa08070b,0x908060a,0x908060b,0x807060a,0x807060b,0x1010102,0x1010103,
};

static const char scan4x4[16] = {0,1,4,5,2,3,6,7,8,9,12,13,10,11,14,15};

static const char cache_to_bits[16] = {4,5,6,7,1,2,25,26,11,19,27,35,8,16,32,40};

unsigned char * cabac_state_base;

uint8_t * bitstream_bufer_hw;

#define BS_SLICE_BUF_MAXSIZE 0x80000

extern unsigned char *frame_buffer;
extern unsigned int phy_fb;


static void copy_dq_table(H264Context *h, int is_intra, int is_8x8dct, int * dq_table_base);

static void reproduce_blk(DCTELEM *block, DCTELEM *coeff, uint8_t *index,
			  int coeff_count, uint8_t *scantable);

static void cabac_reproduce_mb(H264Context *h);


#endif // __JZC_CABAC_HW_H__
