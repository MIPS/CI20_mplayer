#ifndef __VP8_VMAU_H__
#define __VP8_VMAU_H__

extern volatile unsigned char *vmau_base;
#define VMAU_V_BASE 0xB3280000
//#define VMAU_V_BASE vmau_base
#define VMAU_P_BASE 0x13280000
//#define VMAU_P_BASE vmau_base

#define write_reg(reg, val) \
({ i_sw((val), (reg), 0); \
})

#define read_reg(reg, off) \
({ i_lw((reg), (off)); \
})

#define HAMA 0
#define H264 1
#define REAL 2
#define VC1  3
#define MPEG2 4
#define MPEG4 5
#define VPX  6

#define IMB 1
#define BMB 2
#define SKIP 3

#define VMAU_SLV_MCBP (0*4)
#define VMAU_SLV_QTPARA (1*4)
#define VMAU_SLV_MAIN_ADDR (2*4)
#define VMAU_SLV_NCCHN_ADDR (3*4)
#define VMAU_SLV_CHN_LEN (4*4)

#define VMAU_SLV_ACBP (5*4)
#define VMAU_SLV_CPREDM_TLV (6*4)
#define VMAU_SLV_YPREDM0 (7*4)
#define VMAU_SLV_YPREDM1 (8*4)

#define VMAU_SLV_GBL_RUN (0x10*4)
#define VMAU_SLV_GBL_CTR (0x11*4)
#define VMAU_SLV_STATUS (0x12*4)
#define VMAU_SLV_CCHN_ADDR (0x13*4)
#define VMAU_SLV_VIDEO_TYPE (0x14*4)

#define VMAU_SLV_Y_GS (0x15*4)

#define VMAU_SLV_DEC_DONE (0x16*4)
#define VMAU_SLV_ENC_DONE (0x17*4)

#define VMAU_SLV_POS (0x18*4)
#define VMAU_SLV_MCF_STA (0x19*4)

#define VMAU_SLV_DEC_YADDR (0x1a*4)
#define VMAU_SLV_DEC_UADDR (0x1b*4)
#define VMAU_SLV_DEC_VADDR (0x1c*4)
#define VMAU_SLV_DEC_STR (0x1d*4)

#define MAU_MTX_MSK 0x3
#define MAU_MTX_WID 2
#define MAU_MTX_SFT (24)

#define MAU_Y_ERR_MSK 1
#define MAU_Y_ERR_SFT (MAU_MTX_WID+MAU_MTX_SFT) //26

#define MAU_Y_PREDE_MSK 1
#define MAU_Y_PREDE_SFT (MAU_Y_ERR_SFT+MAU_Y_ERR_MSK) //27

#define MAU_Y_DC_DCT_MSK 1
#define MAU_Y_DC_DCT_SFT (MAU_Y_PREDE_SFT+MAU_Y_PREDE_MSK) //28

#define MAU_C_ERR_MSK 1
#define MAU_C_ERR_SFT (MAU_Y_DC_DCT_MSK+MAU_Y_DC_DCT_SFT) //29

#define MAU_C_PREDE_MSK 1
#define MAU_C_PREDE_SFT (MAU_C_ERR_SFT+MAU_C_ERR_MSK) //30

#define MAU_C_DC_DCT_MSK 1
#define MAU_C_DC_DCT_SFT (MAU_C_PREDE_MSK+MAU_C_PREDE_SFT) //31

#define MAU_U_DC_DCT_MSK 1
#define MAU_U_DC_DCT_SFT (MAU_C_ERR_SFT+MAU_C_ERR_MSK) //30

#define MAU_V_DC_DCT_MSK 1
#define MAU_V_DC_DCT_SFT (MAU_C_PREDE_MSK+MAU_C_PREDE_SFT) //31

#define MAU_ENC_RESULT_U_DC_MSK 24
#define MAU_ENC_RESULT_V_DC_MSK 25

#define MAU_VIDEO_MSK 0x7
#define MAU_VIDEO_WID 3
#define MAU_VIDEO_SFT 0 //0

#define MAU_TYPE_MSK 0xF
#define MAU_TYPE_WID 4
#define MAU_TYPE_SFT (MAU_VIDEO_WID + MAU_VIDEO_SFT) //3

#define MAU_SFT_MSK 0xF
#define MAU_SFT_WID 4
#define MAU_SFT_SFT (MAU_TYPE_WID + MAU_TYPE_SFT) //7

#define MAU_ENC_MSK 0x1
#define MAU_ENC_WID 1
#define MAU_ENC_SFT (MAU_SFT_SFT + MAU_SFT_WID) //11

//#define MAU_MCBP_REG (4*6)
#define QUANT_QP_MSK 0x3F
#define QUANT_QP_WID 6
#define QUANT_QP_SFT 0

#define QUANT_CQP_MSK 0x3F
#define QUANT_CQP_WID 6
#define QUANT_CQP_SFT (QUANT_QP_SFT + QUANT_QP_WID)

#define QUANT_C1QP_MSK 0x3F
#define QUANT_C1QP_WID 6
#define QUANT_C1QP_SFT (QUANT_CQP_SFT + QUANT_CQP_WID)

#define MAU_MP2_QTYPE_MSK 0x1
#define MAU_MP2_QTYPE_WID 1
#define MAU_MP2_QTYPE_SFT (23)

#define MAU_MP2_MMOD_MSK 0x1
#define MAU_MP2_MMOD_WID 1
#define MAU_MP2_MMOD_SFT (24)

#define MAU_MP2_SCAN_MSK 0x1
#define MAU_MP2_SCAN_WID 1
#define MAU_MP2_SCAN_SFT (25)

typedef struct VMAU_CHN_REG{
  unsigned int main_cbp ;
  unsigned int quant_para;
  unsigned int main_addr;
  unsigned int ncchn_addr;
  unsigned int id_mlen;
  //unsigned short int main_len;
  //unsigned char id;
  //unsigned char dec_incr;

  unsigned int aux_cbp ;
  unsigned int tlr_cpmd;
  //unsigned short c_pred_mode[1];
  //unsigned short top_lr_avalid;
  unsigned int y_pred_mode[2];
}VMAU_CHN_REG;

#define QUANT_DEQEN_SFT (31)
#define QUANT_DEQEN_MSK (1)

#define VMAU_MEML_BASE (0x4000)
#define VMAU_QT_BASE (0x8000)

#define VMAU_RUN 1
#define VMAU_STOP 2
#define VMAU_RESET 4

#define VPX_QP_SFT (0)
#define VPX_QP_MSK (0x7f)

#define VPX_Y1DC_SFT (7)
#define VPX_Y1DC_MSK (0x1f)

#define VPX_Y2DC_SFT (12)
#define VPX_Y2DC_MSK (0x1f)

#define VPX_Y2AC_SFT (17)
#define VPX_Y2AC_MSK (0x1f)

#define VPX_UVDC_SFT (22)
#define VPX_UVDC_MSK (0x1f)

#define VPX_UVAC_SFT (27)
#define VPX_UVAC_MSK (0x1f)

/*-------CTRL------------*/
#define VMAU_CTRL_FIFO_M 0x1
#define VMAU_CTRL_IRQ_EN 0x10
#define VMAU_CTRL_SLPOW 0x10000
/*--------TRI-------------*/
#define VMAU_TRI_IRQ_CLR 0x8
#define VMAU_TRI_MCYFW_WR 0x10000
#define VMAU_TRI_MCCFW_WR 0x30000
#define VMAU_TRI_MCF_CLR 0x40000

#define VMAU_MC_Y0 (VMAU_V_BASE+0x4000)
#define VMAU_MC_C0 (VMAU_V_BASE+0x4500)

#define VMAU_MC_Y1 (VMAU_MC_Y0+0x100)
#define VMAU_MC_C1 (VMAU_MC_C0+0x80)
#endif//__VP8_VMAU_H__
