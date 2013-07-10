
#ifndef _IPU_H_
#define _IPU_H_

#define REG32(val)  (*((volatile unsigned int *)(val)))

// IPU_REG_BASE
#define IPU_P_BASE  0x13080000
#define IPU__OFFSET 0x13080000
#define IPU__SIZE   0x00001000

struct ipu_module
{
  unsigned int reg_ctrl;      // 0x0
  unsigned int reg_status;    // 0x4
  unsigned int reg_d_fmt;     // 0x8
  unsigned int reg_y_addr;    // 0xc
  unsigned int reg_u_addr;    // 0x10
  unsigned int reg_v_addr;    // 0x14
  unsigned int reg_in_fm_gs;  // 0x18
  unsigned int reg_y_stride;  // 0x1c
  unsigned int reg_uv_stride; // 0x20
  unsigned int reg_out_addr;  // 0x24
  unsigned int reg_out_gs;    // 0x28
  unsigned int reg_out_stride;// 0x2c
  unsigned int rsz_coef_index;// 0x30
  unsigned int reg_csc_c0_coef;      // 0x34
  unsigned int reg_csc_c1_coef;      // 0x38
  unsigned int reg_csc_c2_coef;      // 0x3c
  unsigned int reg_csc_c3_coef;      // 0x40
  unsigned int reg_csc_c4_coef;      // 0x44
  unsigned int hrsz_coef_lut[20];    // 0x48
  unsigned int vrsz_coef_lut[20];    // 0x98
};

// Register offset
#define  REG_CTRL           0x0
#define  REG_STATUS         0x4
#define  REG_D_FMT          0x8
#define  REG_Y_ADDR         0xc
#define  REG_U_ADDR         0x10
#define  REG_V_ADDR         0x14
#define  REG_IN_FM_GS       0x18
#define  REG_Y_STRIDE       0x1c
#define  REG_UV_STRIDE      0x20
#define  REG_OUT_ADDR       0x24
#define  REG_OUT_GS         0x28
#define  REG_OUT_STRIDE     0x2c
#define  REG_RSZ_COEF_INDEX 0x30
#define  REG_CSC_C0_COEF    0x34
#define  REG_CSC_C1_COEF    0x38
#define  REG_CSC_C2_COEF    0x3c
#define  REG_CSC_C3_COEF    0x40
#define  REG_CSC_C4_COEF    0x44
#define  HRSZ_LUT_BASE      0x48
#define  VRSZ_LUT_BASE      0x98

// Not used but can avoid compling errors
#define  REG_CSC_OFFPARA    0x50
#define YUV_CSC_OFFPARA         0x800010  /* chroma,luma */
#define START_N_SFT       12
#define CSC_EN          (1 << 4)
#define enable_yuv_ready(IPU_V_BASE) 
#define enable_ipu_addrsel(IPU_V_BASE) 
#define disable_ipu_addrsel(IPU_V_BASE) 
#define enable_ipu_direct(IPU_V_BASE)
#define disable_ipu_direct(IPU_V_BASE)

// REG_CTRL field define
#define IPU_EN          (1 << 0)
#define RSZ_EN          (1 << 1)
#define FM_IRQ_EN       (1 << 2)
#define IPU_RESET       (1 << 3)
#define H_UP_SCALE      (1 << 8)
#define V_UP_SCALE      (1 << 9)
#define H_SCALE_SHIFT      (8)
#define V_SCALE_SHIFT      (9)

// REG_STATUS field define
#define OUT_END         (1 << 0)

// REG_D_FMT field define
#define INFMT_YUV420    (0 << 0)
#define INFMT_YUV422    (1 << 0)
#define INFMT_YUV444    (2 << 0)
#define INFMT_YUV411    (3 << 0)
#define INFMT_YCbCr420  (4 << 0)
#define INFMT_YCbCr422  (5 << 0)
#define INFMT_YCbCr444  (6 << 0)
#define INFMT_YCbCr411  (7 << 0)
#define IN_FMT_MASK     (7 << 0)

#define OUTFMT_RGB555   (0 << 16)
#define OUTFMT_RGB565   (1 << 16)
#define OUTFMT_RGB888   (2 << 16)
#define OUT_FMT_MASK    (3 << 16)

// REG_IN_FM_GS field define
#define IN_FM_W(val)    ((val) << 16)
#define IN_FM_H(val)    ((val) << 0)

// REG_IN_FM_GS field define
#define OUT_FM_W(val)    ((val) << 16)
#define OUT_FM_H(val)    ((val) << 0)

// REG_UV_STRIDE field define
#define U_STRIDE(val)     ((val) << 16)
#define V_STRIDE(val)     ((val) << 0)


#define VE_IDX_SFT        0
#define HE_IDX_SFT        16

// RSZ_LUT_FIELD
#define OUT_N_SFT         0
#define OUT_N_MSK         0x1
#define IN_N_SFT          1
#define IN_N_MSK          0x1
#define W_COEF_SFT        2
#define W_COEF_MSK        0xFF

// function about REG_CTRL
#define stop_ipu(IPU_V_BASE) \
REG32(IPU_V_BASE + REG_CTRL) &= ~IPU_EN;

#define run_ipu(IPU_V_BASE) \
REG32(IPU_V_BASE + REG_CTRL) |= IPU_EN;

#define reset_ipu(IPU_V_BASE) \
REG32(IPU_V_BASE + REG_CTRL) |= IPU_RESET;

#define disable_irq(IPU_V_BASE) \
REG32(IPU_V_BASE + REG_CTRL) &= ~FM_IRQ_EN;

#define disable_rsize(IPU_V_BASE) \
REG32(IPU_V_BASE + REG_CTRL) &= ~RSZ_EN;

#define enable_hrsize(IPU_V_BASE) \
REG32(IPU_V_BASE + REG_CTRL) |= RSZ_EN;

#define enable_vrsize(IPU_V_BASE) \
REG32(IPU_V_BASE + REG_CTRL) |= RSZ_EN;

#define ipu_is_enable(IPU_V_BASE) \
  (REG32(IPU_V_BASE + REG_CTRL) & IPU_EN)

// function about REG_STATUS
#define clear_end_flag(IPU_V_BASE) \
REG32(IPU_V_BASE +  REG_STATUS) &= ~OUT_END;

#define polling_end_flag(IPU_V_BASE) \
 (REG32(IPU_V_BASE +  REG_STATUS) & OUT_END)

// parameter
// R = 1.164 * (Y - 16) + 1.596 * (cr - 128)    {C0, C1}
// G = 1.164 * (Y - 16) - 0.392 * (cb -128) - 0.813 * (cr - 128)  {C0, C2, C3}
// B = 1.164 * (Y - 16) + 2.017 * (cb - 128)    {C0, C4}

#if 1
#define YUV_CSC_C0 0x4A8        /* 1.164 * 1024 */
#define YUV_CSC_C1 0x662        /* 1.596 * 1024 */
#define YUV_CSC_C2 0x191        /* 0.392 * 1024 */
#define YUV_CSC_C3 0x341        /* 0.813 * 1024 */
#define YUV_CSC_C4 0x811        /* 2.017 * 1024 */
#else
#define YUV_CSC_C0 0x400
#define YUV_CSC_C1 0x59C
#define YUV_CSC_C2 0x161
#define YUV_CSC_C3 0x2DC
#define YUV_CSC_C4 0x718
#endif

#endif
