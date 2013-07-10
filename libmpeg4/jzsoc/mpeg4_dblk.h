#ifndef __MPEG4_DBLK_H__
#define __MPEG4_DBLK_H__

#define DBLK_V_BASE 0x13270000

#define write_dblk_reg(off, value) \
({  write_reg(DBLK_V_BASE+(off), (value));\
})

#define read_dblk_reg(off) \
({  read_reg(DBLK_V_BASE+(off), 0);\
})

#define DBLK_RUN 1
#define DBLK_STOP 2
#define DBLK_RESET 4
#define DBLK_SLICE_RUN 8

#define BLK_CTRL_START 1
#define BLK_CTRL_DONE 2

#define DEBLK_REG_DHA  (0)
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

#define hw_dblk_mpeg4_init(chin_base, chn_end_base, mb_width, mb_height, mb_x, mb_y, dst_y, dst_uv, dma_end_base, slice_end_base, dst_y_str, dst_uv_str, mid_mode)\
({\
write_dblk_reg(DEBLK_REG_TRIG,DBLK_RESET);\ 
write_dblk_reg(DEBLK_REG_DHA, chin_base&0xFFFFFF);\ 
write_dblk_reg(DEBLK_REG_GENDA, chn_end_base); \
write_dblk_reg(DEBLK_REG_GSIZE, mb_width | (mb_height << 16)); \
write_dblk_reg(DEBLK_REG_GPOS, (mb_x & 0x3ff) | ((mb_y & 0x3ff)<<16)); \
write_dblk_reg(DEBLK_REG_GPIC_YA, dst_y); \
write_dblk_reg(DEBLK_REG_GPIC_CA, dst_uv); \
write_dblk_reg(DEBLK_REG_GP_ENDA, dma_end_base); \
write_dblk_reg(DEBLK_REG_GP_SLICE_ENDA, slice_end_base); \
write_dblk_reg(DEBLK_REG_CTRL, 0);\
unsigned int vtr =0; \
vtr = DEBLK_VTR_VIDEO_MPEG4; \
write_dblk_reg(DEBLK_REG_VTR, vtr); \     
write_dblk_reg(DEBLK_REG_GPIC_STR, dst_y_str | (dst_uv_str)<<16); \
write_dblk_reg(DEBLK_REG_TRIG, DBLK_SLICE_RUN);\
})

#endif
