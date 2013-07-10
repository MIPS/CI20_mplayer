#ifndef __T_DBLK_H__
#define __T_DBLK_H__

#ifdef  P1_USE_PADDR
#define DBLK_V_BASE 0x13270000
#define DBLK_P_BASE 0x13270000
#else
extern volatile unsigned char * dblk0_base;
#define DBLK_V_BASE dblk0_base
#define DBLK_P_BASE 0x13270000
#endif

#ifndef write_reg
#define write_reg(reg, val) \
({ i_sw((val), (reg), 0); \
})
#endif

#ifndef read_reg
#define read_reg(reg, off) \
({ i_lw(((reg)+(off)), (0)); \
})
#endif

#if 0
#ifdef EYER_EVA
#include "eyer_driver.h"
#else
#include "instructions.h"
#endif
#endif

#define DEBLK_REG_DHA (0x0)
#define DEBLK_REG_DHA_MSK (0xffffff)


/*--------TRI-------------*/
#define DBLK_RUN 1
#define DBLK_STOP 2
#define DBLK_RESET 4
#define DBLK_SLICE_RUN 8

#define DEBLK_REG_TRIG (0x60)
#define DEBLK_REG_CTRL (0x64)
#define DEBLK_REG_VTR (0x68)
#define DEBLK_REG_FSTA (0x6C)
#define DEBLK_REG_GSTA (0x70)
#define DEBLK_REG_GSIZE (0x74)
#define DEBLK_REG_GENDA (0x78)
#define DEBLK_REG_GPOS (0x7C)
#define DEBLK_REG_GPIC_STR (0x80)
#define DEBLK_REG_GPIC_YA (0x84)
#define DEBLK_REG_GPIC_CA (0x88)
#define DEBLK_REG_GP_ENDA (0x8C)
#define DEBLK_REG_GP_SLICE_ENDA (0x90)

#define DEBLK_REG_BLK_CTRL (0x94)
#define DEBLK_REG_BLK_FIFO (0x98)

#define DEBLK_VTR_VIDEO_H264 0x1
#define DEBLK_VTR_VIDEO_REAL 0x2
#define DEBLK_VTR_VIDEO_VC1 0x3
#define DEBLK_VTR_VIDEO_MPEG2 0x4
#define DEBLK_VTR_VIDEO_MPEG4 0x5
#define DEBLK_VTR_VIDEO_VPX 0x6

#define DEBLK_VTR_FMT_I 0
#define DEBLK_VTR_FMT_P (1<<3)
#define DEBLK_VTR_FMT_B (2<<3)

#define DEBLK_VTR_KEYF (1<<5)
#define DEBLK_VTR_VP8_SIMPLEF (1<<9)

#define DEBLK_VTR_ENC (1<<11)

#define DEBLK_VTR_BETA_SFT (16)
#define DEBLK_VTR_BETA_MSK (0xff)
#define DEBLK_VTR_ALPHA_SFT (24)
#define DEBLK_VTR_ALPHA_MSK (0xff)


#define H264_ALPHA_BATA_REG_BASE (0xA0)

#define BLK_CTRL_START 1
#define BLK_CTRL_DONE 2

#define H264_8x8 1
#define H264_16x16 2
#define H264_16x8 4
#define H264_8x16 8
#define H264_SKIP 16
#define H264_8x8DCT 32

#define H264_INTER_MB (1<<8)

#define H264_QP_MSK 0x3f
#define H264_YQP_UP_SFT 0
#define H264_YQP_CUR_SFT 6
#define H264_UQP_UP_SFT 12
#define H264_UQP_CUR_SFT 18
#define H264_VQP_UP_SFT 24
#define H264_VQP_CUR_SFT 24

#define H264_NNZ_MSK 0xffff
#define H264_NNZ_SFT 0
#define H264_TNNZ_MSK 0xf
#define H264_TNNZ_SFT 16

#define H264_MVP_MODE_MSK 0xfff
#define H264_MVP_MODE_SFT 0
#define H264_MVP_LEN_MSK 0xf
#define H264_MVP_LEN_SFT 12

#define H264_CACHREF_MSK 0x1f

#define H264_CACHREF0_B0_REF0_SFT 0
#define H264_CACHREF0_B0_REF1_SFT 5
#define H264_CACHREF0_B1_REF0_SFT 10
#define H264_CACHREF0_B1_REF1_SFT 15
#define H264_CACHREF0_B2_REF0_SFT 20
#define H264_CACHREF0_B2_REF1_SFT 25

#define H264_CACHREF1_B3_REF0_SFT 0
#define H264_CACHREF1_B3_REF1_SFT 5
#define H264_CACHREF1_T0_REF0_SFT 10
#define H264_CACHREF1_T0_REF1_SFT 15
#define H264_CACHREF1_T1_REF0_SFT 20
#define H264_CACHREF1_T1_REF1_SFT 25

typedef struct h264_mv_info
{
  unsigned short mx;
  unsigned short my;
}h264_mv_info;

typedef struct h264_mb_chn
{
  unsigned int id_dha;
  unsigned short top_mb_type;
  unsigned short cur_mb_type;
  unsigned int qp0;
  unsigned int nnz;  
  unsigned int mv_para_addr;  

  unsigned int cach_ref0;
  unsigned int cach_ref1;

  h264_mv_info top_mv0[4];
  h264_mv_info top_mv1[4];
  h264_mv_info cur_mv[32];
}h264_mb_chn, *h264_mb_chn_p;


#define H264_MB_DHA_MSK 0xffffff
#define H264_MB_ID_MSK 0x7f
#define H264_MB_ID_SFT 24

#define H264_MB_SEND_MSK 0x1
#define H264_MB_SEND_SFT 31

#define H264_MB_DHA_MSK 0xffffff
#define H264_MB_ID_MSK 0x7f
#define H264_MB_ID_SFT 24


typedef struct vp8_mb_chn
{
  unsigned int id_dha;
  unsigned int fl_level;
}vp8_mb_chn, *vp8_mb_chn_p;

#define MB_DHA_MSK 0xffffff
#define MB_ID_MSK 0x7f
#define MB_ID_SFT 24

#define MB_SEND_MSK 0x1
#define MB_SEND_SFT 31
#define HWDBLK_SLICE_END (MB_SEND_MSK<<MB_SEND_SFT)

#define MB_DHA_MSK 0xffffff
#define MB_ID_MSK 0x7f
#define MB_ID_SFT 24

#define VP8_FLEVE_MSK 0x3F
#define VP8_FLEVE_SFT 0
#define VP8_DCDIF_MSK 0x1
#define VP8_DCDIF_SFT 16

#define VP8_BYPASS_MSK 0x1
#define VP8_BYPASS_SFT 20


#define VP8_KEY_FM_MSK 0x1
#define VP8_KEY_FM_SFT 5


#define VP8_FL_TYPE_MSK 0x1
#define VP8_FL_TYPE_SFT 9

#define VP8_SHARP_MSK 0xFF
#define VP8_SHARP_SFT 16

typedef struct vc1_dblk_mb_chn
{
  unsigned int id_dha;
  unsigned char fl_intra;
  unsigned char fl_hen;
  unsigned char fl_ven;
  unsigned char tmp;
}vc1_dblk_mb_chn, *vc1_dblk_mb_chn_p;
#define VC1_FL_INTRA 0x1

typedef struct rv4_dblk_mb_chn
{
  unsigned int id_dha;
  unsigned int coef_nei_info;
  unsigned int mb_info;
}rv4_dblk_mb_chn, *rv4_dblk_mb_chn_p;

#define RV4_TOP_YCBP_MSK 0xf
#define RV4_TOP_YCBP_SFT 0

#define RV4_TOP_UCBP_MSK 0x3
#define RV4_TOP_UCBP_SFT 4

#define RV4_TOP_VCBP_MSK 0x3
#define RV4_TOP_VCBP_SFT 6

#define RV4_TOP_COEF_MSK 0xf
#define RV4_TOP_COEF_SFT 8

#define RV4_TOP_STRONG_MSK 0x1
#define RV4_TOP_STRONG_SFT 13
#define RV4_TOP_STRONG (RV4_TOP_STRONG_MSK<<RV4_TOP_STRONG_SFT)

#define RV4_LEFTEN_MSK 0x1
#define RV4_LEFTEN_SFT 14
#define RV4_LEFTEN (RV4_LEFTEN_MSK<<RV4_LEFTEN_SFT)

#define RV4_TOPEN_MSK 0x1
#define RV4_TOPEN_SFT 15
#define RV4_TOPEN (RV4_TOPEN_MSK<<RV4_TOPEN_SFT)

#define RV4_QP_MSK 0x1f
#define RV4_QP_SFT 24

#define RV4_STRONG_MSK 0x1
#define RV4_STRONG_SFT 29
#define RV4_STRONG (RV4_STRONG_MSK<<RV4_STRONG_SFT)




#define write_dblk_reg(off, value) \
({  write_reg(DBLK_V_BASE+(off), (value));\
})

#define read_dblk_reg(off) \
({  read_reg(DBLK_V_BASE+(off), 0);\
})

#define DBLK_UP_MB_LINE_BASE 0x8000
#define DBLK_LEFT_MB_BASE 0x4000
#define DBLK_MB_FL_BYPASS 0x1000

#define hw_dblk_vpx_init(chin_base, chn_end_base, mb_width, mb_height, mb_x, mb_y, dst_y, dst_uv, dma_end_base, slice_end_base, fm_type, dst_y_str, dst_uv_str, key_fm, sharp, simple_sel) \
({\
write_dblk_reg(DEBLK_REG_TRIG,DBLK_RESET);\
write_dblk_reg(DEBLK_REG_DHA, chin_base);\
write_dblk_reg(DEBLK_REG_GENDA, chn_end_base); \
write_dblk_reg(DEBLK_REG_GSIZE, mb_width | (mb_height << 16)); \
write_dblk_reg(DEBLK_REG_GPOS, (mb_x & 0x3ff) | ((mb_y & 0x3ff)<<16)); \
write_dblk_reg(DEBLK_REG_GPIC_YA, dst_y); \
write_dblk_reg(DEBLK_REG_GPIC_CA, dst_uv); \
write_dblk_reg(DEBLK_REG_GP_ENDA, dma_end_base); \
write_dblk_reg(DEBLK_REG_GP_SLICE_ENDA, slice_end_base); \
unsigned int vtr =0; \
vtr = DEBLK_VTR_VIDEO_VPX | fm_type | (key_fm & VP8_KEY_FM_MSK) << VP8_KEY_FM_SFT | (sharp & VP8_SHARP_MSK) << VP8_SHARP_SFT | (simple_sel & VP8_FL_TYPE_MSK) << VP8_FL_TYPE_SFT; \
fprintf(stderr, "vtr: %x\n", vtr); \
write_dblk_reg(DEBLK_REG_VTR, vtr); \
write_dblk_reg(DEBLK_REG_GPIC_STR, dst_y_str | (dst_uv_str)<<16); \
write_dblk_reg(DEBLK_REG_TRIG, DBLK_SLICE_RUN); \
})


#define hw_dblk_vc1_init(chin_base, chn_end_base, mb_width, mb_height, mb_x, mb_y, dst_y, dst_uv, dma_end_base, slice_end_base, dst_y_str, dst_uv_str) \
({\
write_dblk_reg(DEBLK_REG_TRIG,DBLK_RESET);\
write_dblk_reg(DEBLK_REG_DHA, chin_base);\
write_dblk_reg(DEBLK_REG_GENDA, chn_end_base); \
write_dblk_reg(DEBLK_REG_GSIZE, mb_width | (mb_height << 16)); \
write_dblk_reg(DEBLK_REG_GPOS, (mb_x & 0x3ff) | ((mb_y & 0x3ff)<<16)); \
write_dblk_reg(DEBLK_REG_GPIC_YA, dst_y); \
write_dblk_reg(DEBLK_REG_GPIC_CA, dst_uv); \
write_dblk_reg(DEBLK_REG_GP_ENDA, dma_end_base); \
write_dblk_reg(DEBLK_REG_GP_SLICE_ENDA, slice_end_base); \
unsigned int vtr =0; \
vtr = DEBLK_VTR_VIDEO_VC1; \
write_dblk_reg(DEBLK_REG_VTR, vtr); \
write_dblk_reg(DEBLK_REG_GPIC_STR, dst_y_str | (dst_uv_str)<<16); \
write_dblk_reg(DEBLK_REG_TRIG, DBLK_SLICE_RUN); \
})
#define RV4_MIN_MODE (1<<3)
#define hw_dblk_real_init(chin_base, chn_end_base, mb_width, mb_height, mb_x, mb_y, dst_y, dst_uv, dma_end_base, slice_end_base, dst_y_str, dst_uv_str, mid_mode)\
({\
write_dblk_reg(DEBLK_REG_TRIG,DBLK_RESET);\
write_dblk_reg(DEBLK_REG_DHA, chin_base);\
write_dblk_reg(DEBLK_REG_GENDA, chn_end_base); \
write_dblk_reg(DEBLK_REG_GSIZE, mb_width | (mb_height << 16)); \
write_dblk_reg(DEBLK_REG_GPOS, (mb_x & 0x3ff) | ((mb_y & 0x3ff)<<16)); \
write_dblk_reg(DEBLK_REG_GPIC_YA, dst_y); \
write_dblk_reg(DEBLK_REG_GPIC_CA, dst_uv); \
write_dblk_reg(DEBLK_REG_GP_ENDA, dma_end_base); \
write_dblk_reg(DEBLK_REG_GP_SLICE_ENDA, slice_end_base); \
write_dblk_reg(DEBLK_REG_CTRL, (mid_mode? RV4_MIN_MODE: 0) | 0x1); \
unsigned int vtr =0; \
vtr = DEBLK_VTR_VIDEO_REAL; \
write_dblk_reg(DEBLK_REG_VTR, vtr); \
write_dblk_reg(DEBLK_REG_GPIC_STR, dst_y_str | (dst_uv_str)<<16); \
write_dblk_reg(DEBLK_REG_TRIG, DBLK_SLICE_RUN); \
})

#endif


