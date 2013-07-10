#ifndef __T_MOTION_H__
#define __T_MOTION_H__

/*extern*/ volatile unsigned char *mc_base;
//#define MOTION_V_BASE       (0xB3250000)//(int)mc_base
#ifdef  MPEG4_P1_USE_PADDR
#define MOTION_V_BASE       (0x13250000)
#else
#define MOTION_V_BASE       (0xB3250000)
#endif

extern volatile unsigned char *mc_base;

#define HAS_PMON            1

#define K0_PA(va)           ((va) & 0x1FFFFFFF)

#define write_reg(reg, val) *(volatile int*)(reg) = val
#define read_reg(reg, off)  *(volatile int*)(reg+off)

/*-----------------------------------------------------------*/
#define BLK_W4              1
#define BLK_W8              2
#define BLK_W16             3

#define BLK_H4              1
#define BLK_H8              2
#define BLK_H16             3

#define IS_TAP2             0
#define IS_TAP4             1
#define IS_TAP6             2
#define IS_TAP8             3

#define IS_HDIR             0
#define IS_VDIR             1

#define IS_SKIRT            0
#define IS_MIRROR           1

#define IS_H264DGL          1
#define IS_AVSDGL           1

#define NEED_ITS            1
#define NEED_WT             1
#define NEED_BIAVG          1

#if 0
// CTRL
#define REG1_CTRL           0x0
#define CTRL_ESMS_SFT       28
#define CTRL_ESMS_MSK       0xF
#define CTRL_ESA_SFT        26
#define CTRL_ESA_MSK        0x3
#define CTRL_ESMD_SFT       24
#define CTRL_ESMD_MSK       0x3
#define CTRL_CSF_SFT        19
#define CTRL_CSF_MSK        0x1
#define CTRL_CARA_SFT       18
#define CTRL_CARA_MSK       0x1
#define CTRL_CAE_SFT        17
#define CTRL_CAE_MSK        0x1
#define CTRL_CRPM_SFT       16
#define CTRL_CRPM_MSK       0x1
#define CTRL_PGC_SFT        12
#define CTRL_PGC_MSK        0xF
#define CTRL_PRI_SFT        9
#define CTRL_PRI_MSK        0x3
#define CTRL_CKGE_SFT       8
#define CTRL_CKGE_MSK       0x1
#define CTRL_OFA_SFT        7
#define CTRL_OFA_MSK        0x1
#define CTRL_ROT_SFT        6
#define CTRL_ROT_MSK        0x1
#define CTRL_DDM_SFT        5
#define CTRL_DDM_MSK        0x1
#define CTRL_WM_SFT         4
#define CTRL_WM_MSK         0x1
#define CTRL_CCF_SFT        3
#define CTRL_CCF_MSK        0x1
#define CTRL_CSL_SFT        2   /*cancel*/
#define CTRL_CSL_MSK        0x1
#define CTRL_RST_SFT        1
#define CTRL_RST_MSK        0x1
#define CTRL_EN_SFT         0
#define CTRL_EN_MSK         0x1

#define USE_TDD             0
#define USE_CDD             1

#define SET_REG1_CTRL(esms,esa,esmd,csf,cara,cae,crpm,			\
		      pgc,pri,ckge,ofa,rot,ddm,wm,ccf,csl,rst,en)	\
  ({ write_reg( (mc_base+REG1_CTRL),				\
		(((esms) & CTRL_ESMS_MSK)<<CTRL_ESMS_SFT) |		\
		(((esa ) & CTRL_ESA_MSK )<<CTRL_ESA_SFT ) |		\
		(((esmd) & CTRL_ESMD_MSK)<<CTRL_ESMD_SFT) |		\
		(((csf ) & CTRL_CSF_MSK )<<CTRL_CSF_SFT ) |		\
		(((cara) & CTRL_CARA_MSK)<<CTRL_CARA_SFT) |		\
		(((cae ) & CTRL_CAE_MSK )<<CTRL_CAE_SFT ) |		\
		(((crpm) & CTRL_CRPM_MSK)<<CTRL_CRPM_SFT) |		\
		(((pgc ) & CTRL_PGC_MSK )<<CTRL_PGC_SFT ) |		\
		(((pri ) & CTRL_PRI_MSK )<<CTRL_PRI_SFT ) |		\
		(((ckge) & CTRL_CKGE_MSK)<<CTRL_CKGE_SFT) |		\
		(((ofa ) & CTRL_OFA_MSK )<<CTRL_OFA_SFT ) |		\
		(((rot ) & CTRL_ROT_MSK )<<CTRL_ROT_SFT ) |		\
		(((ddm ) & CTRL_DDM_MSK )<<CTRL_DDM_SFT ) |		\
		(((wm  ) & CTRL_WM_MSK  )<<CTRL_WM_SFT  ) |		\
		(((ccf ) & CTRL_CCF_MSK )<<CTRL_CCF_SFT ) |		\
		(((csl ) & CTRL_CSL_MSK )<<CTRL_CSL_SFT ) |		\
		(((rst ) & CTRL_RST_MSK )<<CTRL_RST_SFT ) |		\
		(((en  ) & CTRL_EN_MSK  )<<CTRL_EN_SFT  ) );		\
      })
#else
// CTRL 4770
#define REG1_CTRL           0x0
#define CTRL_EBMS_SFT       28
#define CTRL_EBMS_MSK       0xF
#define CTRL_ESMS_SFT       24
#define CTRL_ESMS_MSK       0xF
#define CTRL_EARM_SFT       23
#define CTRL_EARM_MSK       0x1
#define CTRL_EPMV_SFT       22
#define CTRL_EPMV_MSK       0x1
#define CTRL_ESA_SFT        20
#define CTRL_ESA_MSK        0x3
#define CTRL_EBME_SFT       19
#define CTRL_EBME_MSK       0x1
#define CTRL_CAE_SFT        17
#define CTRL_CAE_MSK        0x1
#define CTRL_PGC_SFT        12
#define CTRL_PGC_MSK        0xF
#define CTRL_CH2EN_SFT      11
#define CTRL_CH2EN_MSK      0x1
#define CTRL_PRI_SFT        9
#define CTRL_PRI_MSK        0x3
#define CTRL_CKGE_SFT       8
#define CTRL_CKGE_MSK       0x1
#define CTRL_OFA_SFT        7
#define CTRL_OFA_MSK        0x1
#define CTRL_ROT_SFT        6
#define CTRL_ROT_MSK        0x1
#define CTRL_ROTADIR_SFT        5
#define CTRL_ROTADIR_MSK        0x1
#define CTRL_WM_SFT         4
#define CTRL_WM_MSK         0x1
#define CTRL_CCF_SFT        3
#define CTRL_CCF_MSK        0x1
#define CTRL_IRQE_SFT        2   /*cancel*/
#define CTRL_IRQE_MSK        0x1
#define CTRL_RST_SFT        1
#define CTRL_RST_MSK        0x1
#define CTRL_EN_SFT         0
#define CTRL_EN_MSK         0x1

//#define USE_TDD             0
//#define USE_CDD             1

#define SET_REG1_CTRL(ebms,esms,earm,epmv,esa,ebme,cae,		\
		      pgc,ch2en,pri,ckge,ofa,rot,rotdir,wm,ccf,irqe,rst,en)	\
  ({ write_reg( (mc_base+REG1_CTRL),				\
		(((ebms) & CTRL_EBMS_MSK)<<CTRL_EBMS_SFT) |		\
		(((esms) & CTRL_ESMS_MSK)<<CTRL_ESMS_SFT) |		\
		(((earm) & CTRL_EARM_MSK)<<CTRL_EARM_SFT) |		\
		(((epmv) & CTRL_EPMV_MSK)<<CTRL_EPMV_SFT) |		\
		(((esa ) & CTRL_ESA_MSK )<<CTRL_ESA_SFT ) |		\
		(((ebme) & CTRL_EBME_MSK )<<CTRL_EBME_SFT ) |		\
		(((cae ) & CTRL_CAE_MSK )<<CTRL_CAE_SFT ) |		\
		(((pgc ) & CTRL_PGC_MSK )<<CTRL_PGC_SFT ) |		\
		(((ch2en)& CTRL_CH2EN_MSK )<<CTRL_CH2EN_SFT ) |		\
		(((pri ) & CTRL_PRI_MSK )<<CTRL_PRI_SFT ) |		\
		(((ckge) & CTRL_CKGE_MSK)<<CTRL_CKGE_SFT) |		\
		(((ofa ) & CTRL_OFA_MSK )<<CTRL_OFA_SFT ) |		\
		(((rot ) & CTRL_ROT_MSK )<<CTRL_ROT_SFT ) |		\
		(((rotdir)& CTRL_ROTADIR_MSK )<<CTRL_ROTADIR_SFT ) |	\
		(((wm  ) & CTRL_WM_MSK  )<<CTRL_WM_SFT  ) |		\
		(((ccf ) & CTRL_CCF_MSK )<<CTRL_CCF_SFT ) |		\
		(((irqe) & CTRL_IRQE_MSK )<<CTRL_IRQE_SFT ) |		\
		(((rst ) & CTRL_RST_MSK )<<CTRL_RST_SFT ) |		\
		(((en  ) & CTRL_EN_MSK  )<<CTRL_EN_SFT  ) );		\
      })
#endif

#define GET_REG1_CTRL()				\
  ({ read_reg(  MOTION_V_BASE , REG1_CTRL  );	\
  })

// STATUS
#define REG1_STAT           0x4
#define REG2_STAT           0x804
#define STAT_PFE_SFT        2
#define STAT_PFE_MSK        0x1
#define STAT_LKE_SFT        1
#define STAT_LKE_MSK        0x1
#define STAT_TKE_SFT        0
#define STAT_TKE_MSK        0x1

#define SET_REG1_STAT(pfe,lke,tke)					\
  ({ write_reg( (mc_base+REG1_STAT),				\
		(((pfe ) & STAT_PFE_MSK )<<STAT_PFE_SFT ) |		\
		(((lke ) & STAT_LKE_MSK )<<STAT_LKE_SFT ) |		\
		(((tke ) & STAT_TKE_MSK )<<STAT_TKE_SFT ) );		\
  })
#define SET_REG2_STAT(pfe,lke,tke)					\
  ({ write_reg( (mc_base+REG2_STAT),				\
		(((pfe ) & STAT_PFE_MSK )<<STAT_PFE_SFT ) |		\
		(((lke ) & STAT_LKE_MSK )<<STAT_LKE_SFT ) |		\
		(((tke ) & STAT_TKE_MSK )<<STAT_TKE_SFT ) );		\
  })

#define GET_REG1_STAT()				\
  ({ read_reg(  MOTION_V_BASE , REG1_STAT  );	\
  })
#define GET_REG2_STAT()				\
  ({ read_reg(  MOTION_V_BASE , REG2_STAT  );	\
  })

// MBPOS
#define REG1_MBPOS          0x8
#define REG2_MBPOS          0x808
#define MBPOS_MBY_SFT       8
#define MBPOS_MBY_MSK       0xFF
#define MBPOS_MBX_SFT       0
#define MBPOS_MBX_MSK       0xFF

#define SET_REG1_MBPOS(mby,mbx)						\
  ({ write_reg( (MOTION_V_BASE+REG1_MBPOS),				\
		(((mby) & MBPOS_MBY_MSK)<<MBPOS_MBY_SFT) |		\
		(((mbx) & MBPOS_MBX_MSK)<<MBPOS_MBX_SFT) );		\
  })
#define SET_REG2_MBPOS(mby,mbx)						\
  ({ write_reg( (MOTION_V_BASE+REG2_MBPOS),				\
		(((mby) & MBPOS_MBY_MSK)<<MBPOS_MBY_SFT) |		\
		(((mbx) & MBPOS_MBX_MSK)<<MBPOS_MBX_SFT) );		\
  })

#define GET_REG1_MBPOS()			\
  ({ read_reg(  MOTION_V_BASE , REG1_MBPOS  );	\
  })
#define GET_REG2_MBPOS()			\
  ({ read_reg(  MOTION_V_BASE , REG2_MBPOS  );	\
  })

// MVPA
#define REG1_MVPA           0xC

#define SET_REG1_MVPA(mvpa)						\
  ({ write_reg( (MOTION_V_BASE+REG1_MVPA), (mvpa) );			\
  })

#define GET_REG1_MVPA()				\
  ({ read_reg(  MOTION_V_BASE , REG1_MVPA  );	\
  })

#define SET_REG1_IWTA       SET_REG1_MVPA
#define GET_REG1_IWTA       GET_REG1_IWTA

// RAWA
#define REG1_RAWA           0x10

#define SET_REG1_RAWA(rawa)						\
  ({ write_reg( (MOTION_V_BASE+REG1_RAWA), (rawa) );			\
  })

#define GET_REG1_RAWA()				\
  ({ read_reg(  MOTION_V_BASE , REG1_RAWA  );	\
  })

// MVINFO
#define REG1_MVINFO         0x14
#define REG2_MVINFO         0x814
#define MVINFO_INTY_SFT     16
#define MVINFO_INTY_MSK     0xFFFF
#define MVINFO_INTX_SFT     0
#define MVINFO_INTX_MSK     0xFFFF
#define MVINFO_MVDY_SFT     24
#define MVINFO_MVDY_MSK     0xFF
#define MVINFO_MVPY_SFT     16
#define MVINFO_MVPY_MSK     0xFF
#define MVINFO_MVDX_SFT     8
#define MVINFO_MVDX_MSK     0xFF
#define MVINFO_MVPX_SFT     0
#define MVINFO_MVPX_MSK     0xFF

#define SET_REG1_CMV(inty,intx)						\
  ({ write_reg( (MOTION_V_BASE+REG1_MVINFO),				\
		(((inty) & MVINFO_INTY_MSK)<<MVINFO_INTY_SFT) |		\
		(((intx) & MVINFO_INTX_MSK)<<MVINFO_INTX_SFT) );	\
  })
#define SET_REG2_CMV(inty,intx)						\
  ({ write_reg( (MOTION_V_BASE+REG2_MVINFO),				\
		(((inty) & MVINFO_INTY_MSK)<<MVINFO_INTY_SFT) |		\
		(((intx) & MVINFO_INTX_MSK)<<MVINFO_INTX_SFT) );	\
  })
#define SET_REG1_EMV(mvdy,mvpy,mvdx,mvpx)				\
  ({ write_reg( (MOTION_V_BASE+REG1_MVINFO),				\
		(((mvdy) & MVINFO_MVDY_MSK)<<MVINFO_MVDY_SFT) |		\
		(((mvpy) & MVINFO_MVPY_MSK)<<MVINFO_MVPY_SFT) |		\
		(((mvdx) & MVINFO_MVDX_MSK)<<MVINFO_MVDX_SFT) |		\
		(((mvpx) & MVINFO_MVPX_MSK)<<MVINFO_MVPX_SFT) );	\
  })

#define GET_REG1_CMV()				\
  ({ read_reg(  MOTION_V_BASE , REG1_MVINFO  );	\
  })
#define GET_REG2_CMV()				\
  ({ read_reg(  MOTION_V_BASE , REG2_MVINFO  );	\
  })
#define GET_REG1_EMV        GET_REG1_CMV

// REFA
#define REG1_REFA           0x18
#define REG2_REFA           0x818

#define SET_REG1_REFA(refa)						\
  ({ write_reg( (MOTION_V_BASE+REG1_REFA), (refa) );			\
  })
#define SET_REG2_REFA(refa)						\
  ({ write_reg( (MOTION_V_BASE+REG2_REFA), (refa) );			\
  })

#define GET_REG1_REFA()				\
  ({ read_reg(  MOTION_V_BASE , REG1_REFA  );	\
  })
#define GET_REG2_REFA()				\
  ({ read_reg(  MOTION_V_BASE , REG2_REFA  );	\
  })

// DSTA
#define REG1_DSTA           0x1C
#define REG2_DSTA           0x81C

#define SET_REG1_DSTA(dsta)						\
  ({ write_reg( (MOTION_V_BASE+REG1_DSTA), (dsta) );			\
  })
#define SET_REG2_DSTA(dsta)						\
  ({ write_reg( (MOTION_V_BASE+REG2_DSTA), (dsta) );			\
  })

#define GET_REG1_DSTA()				\
  ({ read_reg(  MOTION_V_BASE , REG1_DSTA  );	\
  })
#define GET_REG2_DSTA()				\
  ({ read_reg(  MOTION_V_BASE , REG2_DSTA  );	\
  })

// PINFO
#define REG1_PINFO          0x20
#define REG2_PINFO          0x820
#define PINFO_RGR_SFT       31
#define PINFO_RGR_MSK       0x1
#define PINFO_ITS_SFT       28
#define PINFO_ITS_MSK       0x1
#define PINFO_ITS_SFT_SFT   24
#define PINFO_ITS_SFT_MSK   0x7
#define PINFO_ITS_SCALE_SFT 16
#define PINFO_ITS_SCALE_MSK 0xFF
#define PINFO_ITS_RND_SFT   0
#define PINFO_ITS_RND_MSK   0xFFFF

#define SET_REG1_PINFO(rgr,its,its_sft,its_scale,its_rnd)		\
  ({ write_reg( (mc_base+REG1_PINFO),				\
		(((rgr) & PINFO_RGR_MSK)<<PINFO_RGR_SFT) |		\
		(((its) & PINFO_ITS_MSK)<<PINFO_ITS_SFT) |		\
		(((its_sft) & PINFO_ITS_SFT_MSK)<<PINFO_ITS_SFT_SFT) |	\
		(((its_scale) & PINFO_ITS_SCALE_MSK)<<PINFO_ITS_SCALE_SFT) |	\
		(((its_rnd) & PINFO_ITS_RND_MSK)<<PINFO_ITS_RND_SFT) );	\
  })
#define SET_REG2_PINFO(rgr,its,its_sft,its_scale,its_rnd)		\
  ({ write_reg( (mc_base+REG2_PINFO),				\
		(((rgr) & PINFO_RGR_MSK)<<PINFO_RGR_SFT) |		\
		(((its) & PINFO_ITS_MSK)<<PINFO_ITS_SFT) |		\
		(((its_sft) & PINFO_ITS_SFT_MSK)<<PINFO_ITS_SFT_SFT) |	\
		(((its_scale) & PINFO_ITS_SCALE_MSK)<<PINFO_ITS_SCALE_SFT) |	\
		(((its_rnd) & PINFO_ITS_RND_MSK)<<PINFO_ITS_RND_SFT) );	\
  })

#define GET_REG1_PINFO()			\
  ({ read_reg(  MOTION_V_BASE , REG1_PINFO  );	\
  })
#define GET_REG2_PINFO()			\
  ({ read_reg(  MOTION_V_BASE , REG2_PINFO  );	\
  })

// WINFO
#define REG1_WINFO          0x24
#define REG2_WINFO1         0x824
#define REG2_WINFO2         0x828
#define WINFO_WT_SFT        31
#define WINFO_WT_MSK        0x1
#define WINFO_WTPD_SFT      30
#define WINFO_WTPD_MSK      0x1
#define WINFO_WTMD_SFT      28
#define WINFO_WTMD_MSK      0x3
#define WINFO_BIAVG_RND_SFT 27
#define WINFO_BIAVG_RND_MSK 0x1
#define WINFO_WT_DENOM_SFT  24
#define WINFO_WT_DENOM_MSK  0x7
#define WINFO_WT_SFT_SFT    16
#define WINFO_WT_SFT_MSK    0xF
#define WINFO_WT_LCOEF_SFT  8
#define WINFO_WT_LCOEF_MSK  0xFF
#define WINFO_WT_RCOEF_SFT  0
#define WINFO_WT_RCOEF_MSK  0xFF

#define SET_REG1_WINFO(wt,wtpd,wtmd,biavg_rnd,wt_denom,			\
		       wt_sft,wt_lcoef,wt_rcoef)			\
  ({ write_reg( (mc_base+REG1_WINFO),				\
		(((wt) & WINFO_WT_MSK)<<WINFO_WT_SFT) |			\
		(((wtmd) & WINFO_WTMD_MSK)<<WINFO_WTMD_SFT) |		\
		(((wtpd) & WINFO_WTPD_MSK)<<WINFO_WTPD_SFT) |		\
		(((biavg_rnd) & WINFO_BIAVG_RND_MSK)<<WINFO_BIAVG_RND_SFT) |	\
		(((wt_denom) & WINFO_WT_DENOM_MSK)<<WINFO_WT_DENOM_SFT) |	\
		(((wt_sft) & WINFO_WT_SFT_MSK)<<WINFO_WT_SFT_SFT) |	\
		(((wt_lcoef) & WINFO_WT_LCOEF_MSK)<<WINFO_WT_LCOEF_SFT) |	\
		(((wt_rcoef) & WINFO_WT_RCOEF_MSK)<<WINFO_WT_RCOEF_SFT) );      \
  })
#define SET_REG2_WINFO1(wt,wtpd,wtmd,biavg_rnd,wt_denom,		\
			wt_sft,wt_lcoef,wt_rcoef)			\
  ({ write_reg( (mc_base+REG2_WINFO1),				\
		(((wt) & WINFO_WT_MSK)<<WINFO_WT_SFT) |			\
		(((wtpd) & WINFO_WTPD_MSK)<<WINFO_WTPD_SFT) |		\
		(((wtmd) & WINFO_WTMD_MSK)<<WINFO_WTMD_SFT) |		\
		(((biavg_rnd) & WINFO_BIAVG_RND_MSK)<<WINFO_BIAVG_RND_SFT) |	\
		(((wt_denom) & WINFO_WT_DENOM_MSK)<<WINFO_WT_DENOM_SFT) |	\
		(((wt_sft) & WINFO_WT_SFT_MSK)<<WINFO_WT_SFT_SFT) |	\
		(((wt_lcoef) & WINFO_WT_LCOEF_MSK)<<WINFO_WT_LCOEF_SFT) |	\
		(((wt_rcoef) & WINFO_WT_RCOEF_MSK)<<WINFO_WT_RCOEF_SFT) );      \
  })
#define SET_REG2_WINFO2(wt_sft,wt_lcoef,wt_rcoef)			\
  ({ write_reg( (mc_base+REG2_WINFO2),				\
		(((wt_sft) & WINFO_WT_SFT_MSK)<<WINFO_WT_SFT_SFT) |	\
		(((wt_lcoef) & WINFO_WT_LCOEF_MSK)<<WINFO_WT_LCOEF_SFT) |	\
		(((wt_rcoef) & WINFO_WT_RCOEF_MSK)<<WINFO_WT_RCOEF_SFT) );      \
  })

#define GET_REG1_WINFO()			\
  ({ read_reg(  MOTION_V_BASE , REG1_WINFO  );	\
  })
#define GET_REG2_WINFO1()			\
  ({ read_reg(  MOTION_V_BASE , REG2_WINFO1  );	\
  })
#define GET_REG2_WINFO2()			\
  ({ read_reg(  MOTION_V_BASE , REG2_WINFO2  );	\
  })

// WTRND
#define REG1_WTRND          0x2C
#define REG2_WTRND          0x82C
#define WTRND_WT2_RND_SFT   16
#define WTRND_WT2_RND_MSK   0xFFFF
#define WTRND_WT1_RND_SFT   0
#define WTRND_WT1_RND_MSK   0xFFFF

#define SET_REG1_WTRND(wt_rnd)						\
  ({ write_reg( (mc_base+REG1_WTRND),				\
		(((wt_rnd) & WTRND_WT1_RND_MSK)<<WTRND_WT1_RND_SFT) );	\
  })
#define SET_REG2_WTRND(wt2_rnd,wt1_rnd)					\
  ({ write_reg( (mc_base+REG2_WTRND),				\
		(((wt2_rnd) & WTRND_WT2_RND_MSK)<<WTRND_WT2_RND_SFT) |	\
		(((wt1_rnd) & WTRND_WT1_RND_MSK)<<WTRND_WT1_RND_SFT) );	\
  })

#define GET_REG1_WTRND()			\
  ({ read_reg(  MOTION_V_BASE , REG1_WTRND  );	\
  })
#define GET_REG2_WTRND()			\
  ({ read_reg(  MOTION_V_BASE , REG2_WTRND  );	\
  })

// BINFO
#define REG1_BINFO          0x30
#define REG2_BINFO          0x830
#define BINFO_ARY_SFT       31
#define BINFO_ARY_MSK       0x1
#define BINFO_DOE_SFT       30
#define BINFO_DOE_MSK       0x1
#define BINFO_EXPDY_SFT     24
#define BINFO_EXPDY_MSK     0xF
#define BINFO_EXPDX_SFT     20
#define BINFO_EXPDX_MSK     0xF
#define BINFO_ILMD_SFT      16
#define BINFO_ILMD_MSK      0x3
#define BINFO_PEL_SFT       14
#define BINFO_PEL_MSK       0x3
#define BINFO_FLD_SFT       13
#define BINFO_FLD_MSK       0x1
#define BINFO_FLDSEL_SFT    12
#define BINFO_FLDSEL_MSK    0x1
#define BINFO_BOY_SFT       10
#define BINFO_BOY_MSK       0x3
#define BINFO_BOX_SFT       8
#define BINFO_BOX_MSK       0x3
#define BINFO_BH_SFT        6
#define BINFO_BH_MSK        0x3
#define BINFO_BW_SFT        4
#define BINFO_BW_MSK        0x3
#define BINFO_POS_SFT       0
#define BINFO_POS_MSK       0xF

#define SET_REG1_BINFO(ary,doe,expdy,expdx,ilmd,pel,fld,fldsel,		\
		       boy,box,bh,bw,pos)				\
  ({ write_reg( (MOTION_V_BASE+REG1_BINFO),				\
		(((ary)   & BINFO_ARY_MSK)<<BINFO_ARY_SFT) |		\
		(((doe)   & BINFO_DOE_MSK)<<BINFO_DOE_SFT) |		\
		(((expdy) & BINFO_EXPDY_MSK)<<BINFO_EXPDY_SFT) |	\
		(((expdx) & BINFO_EXPDX_MSK)<<BINFO_EXPDX_SFT) |	\
		(((ilmd)  & BINFO_ILMD_MSK)<<BINFO_ILMD_SFT) |		\
		(((pel)   & BINFO_PEL_MSK)<<BINFO_PEL_SFT) |		\
		(((fld)   & BINFO_FLD_MSK)<<BINFO_FLD_SFT) |		\
		(((fldsel)& BINFO_FLDSEL_MSK)<<BINFO_FLDSEL_SFT) |	\
		(((boy)   & BINFO_BOY_MSK)<<BINFO_BOY_SFT) |		\
		(((box)   & BINFO_BOX_MSK)<<BINFO_BOX_SFT) |		\
		(((bh)    & BINFO_BH_MSK)<<BINFO_BH_SFT) |		\
		(((bw)    & BINFO_BW_MSK)<<BINFO_BW_SFT) |		\
		(((pos)   & BINFO_POS_MSK)<<BINFO_POS_SFT) );		\
  })
#define SET_REG1_BINFO_P0(ary,doe,expdy,expdx,ilmd,pel,fld,fldsel,		\
		       boy,box,bh,bw,pos)				\
  ({ write_reg( (mc_base+REG1_BINFO),				\
		(((ary)   & BINFO_ARY_MSK)<<BINFO_ARY_SFT) |		\
		(((doe)   & BINFO_DOE_MSK)<<BINFO_DOE_SFT) |		\
		(((expdy) & BINFO_EXPDY_MSK)<<BINFO_EXPDY_SFT) |	\
		(((expdx) & BINFO_EXPDX_MSK)<<BINFO_EXPDX_SFT) |	\
		(((ilmd)  & BINFO_ILMD_MSK)<<BINFO_ILMD_SFT) |		\
		(((pel)   & BINFO_PEL_MSK)<<BINFO_PEL_SFT) |		\
		(((fld)   & BINFO_FLD_MSK)<<BINFO_FLD_SFT) |		\
		(((fldsel)& BINFO_FLDSEL_MSK)<<BINFO_FLDSEL_SFT) |	\
		(((boy)   & BINFO_BOY_MSK)<<BINFO_BOY_SFT) |		\
		(((box)   & BINFO_BOX_MSK)<<BINFO_BOX_SFT) |		\
		(((bh)    & BINFO_BH_MSK)<<BINFO_BH_SFT) |		\
		(((bw)    & BINFO_BW_MSK)<<BINFO_BW_SFT) |		\
		(((pos)   & BINFO_POS_MSK)<<BINFO_POS_SFT) );		\
  })

#define SET_REG2_BINFO(ary,doe,expdy,expdx,ilmd,pel,fld,fldsel,		\
		       boy,box,bh,bw,pos)				\
  ({ write_reg( (MOTION_V_BASE+REG2_BINFO),				\
		(((ary)   & BINFO_ARY_MSK)<<BINFO_ARY_SFT) |		\
		(((doe)   & BINFO_DOE_MSK)<<BINFO_DOE_SFT) |		\
		(((expdy) & BINFO_EXPDY_MSK)<<BINFO_EXPDY_SFT) |	\
		(((expdx) & BINFO_EXPDX_MSK)<<BINFO_EXPDX_SFT) |	\
		(((ilmd)  & BINFO_ILMD_MSK)<<BINFO_ILMD_SFT) |		\
		(((pel)   & BINFO_PEL_MSK)<<BINFO_PEL_SFT) |		\
		(((fld)   & BINFO_FLD_MSK)<<BINFO_FLD_SFT) |		\
		(((fldsel)& BINFO_FLDSEL_MSK)<<BINFO_FLDSEL_SFT) |	\
		(((boy)   & BINFO_BOY_MSK)<<BINFO_BOY_SFT) |		\
		(((box)   & BINFO_BOX_MSK)<<BINFO_BOX_SFT) |		\
		(((bh)    & BINFO_BH_MSK)<<BINFO_BH_SFT) |		\
		(((bw)    & BINFO_BW_MSK)<<BINFO_BW_SFT) |		\
		(((pos)   & BINFO_POS_MSK)<<BINFO_POS_SFT) );		\
  })
#define SET_REG2_BINFO_P0(ary,doe,expdy,expdx,ilmd,pel,fld,fldsel,		\
		       boy,box,bh,bw,pos)				\
  ({ write_reg( (mc_base+REG2_BINFO),				\
		(((ary)   & BINFO_ARY_MSK)<<BINFO_ARY_SFT) |		\
		(((doe)   & BINFO_DOE_MSK)<<BINFO_DOE_SFT) |		\
		(((expdy) & BINFO_EXPDY_MSK)<<BINFO_EXPDY_SFT) |	\
		(((expdx) & BINFO_EXPDX_MSK)<<BINFO_EXPDX_SFT) |	\
		(((ilmd)  & BINFO_ILMD_MSK)<<BINFO_ILMD_SFT) |		\
		(((pel)   & BINFO_PEL_MSK)<<BINFO_PEL_SFT) |		\
		(((fld)   & BINFO_FLD_MSK)<<BINFO_FLD_SFT) |		\
		(((fldsel)& BINFO_FLDSEL_MSK)<<BINFO_FLDSEL_SFT) |	\
		(((boy)   & BINFO_BOY_MSK)<<BINFO_BOY_SFT) |		\
		(((box)   & BINFO_BOX_MSK)<<BINFO_BOX_SFT) |		\
		(((bh)    & BINFO_BH_MSK)<<BINFO_BH_SFT) |		\
		(((bw)    & BINFO_BW_MSK)<<BINFO_BW_SFT) |		\
		(((pos)   & BINFO_POS_MSK)<<BINFO_POS_SFT) );		\
  })

#define GET_REG1_BINFO()			\
  ({ read_reg(  MOTION_V_BASE , REG1_BINFO  );	\
  })
#define GET_REG2_BINFO()			\
  ({ read_reg(  MOTION_V_BASE , REG2_BINFO  );	\
  })

// IINFO
#define REG1_IINFO1          0x34
#define REG1_IINFO2          0x38
#define IINFO_INTP_SFT       31
#define IINFO_INTP_MSK       0x1
#define IINFO_TAP_SFT        28
#define IINFO_TAP_MSK        0x3
#define IINFO_INTP_PKG_SFT   27
#define IINFO_INTP_PKG_MSK   0x1
#define IINFO_HLDGL_SFT      26
#define IINFO_HLDGL_MSK      0x1
#define IINFO_AVSDGL_SFT     25
#define IINFO_AVSDGL_MSK     0x1
#define IINFO_INTP_DIR_SFT   24
#define IINFO_INTP_DIR_MSK   0x1
#define IINFO_INTP_RND_SFT   16
#define IINFO_INTP_RND_MSK   0xFF
#define IINFO_INTP_SFT_SFT   8
#define IINFO_INTP_SFT_MSK   0xF
#define IINFO_SINTP_SFT      2
#define IINFO_SINTP_MSK      0x1
#define IINFO_SINTP_RND_SFT  1
#define IINFO_SINTP_RND_MSK  0x1
#define IINFO_SINTP_BIAS_SFT 0
#define IINFO_SINTP_BIAS_MSK 0x1

#define SET_REG1_IINFO1(intp,tap,intp_pkg,intp_dir,intp_rnd,intp_sft,	\
			sintp,sintp_rnd,sintp_bias)			\
  ({ write_reg( (MOTION_V_BASE+REG1_IINFO1),				\
		(((intp) & IINFO_INTP_MSK)<<IINFO_INTP_SFT) |		\
		(((tap) & IINFO_TAP_MSK)<<IINFO_TAP_SFT) |		\
		(((intp_pkg) & IINFO_INTP_PKG_MSK)<<IINFO_INTP_PKG_SFT) |	\
		(((intp_dir) & IINFO_INTP_DIR_MSK)<<IINFO_INTP_DIR_SFT) |	\
		(((intp_rnd) & IINFO_INTP_RND_MSK)<<IINFO_INTP_RND_SFT) |	\
		(((intp_sft) & IINFO_INTP_SFT_MSK)<<IINFO_INTP_SFT_SFT) |	\
		(((sintp) & IINFO_SINTP_MSK)<<IINFO_SINTP_SFT) |	\
		(((sintp_rnd) & IINFO_SINTP_RND_MSK)<<IINFO_SINTP_RND_SFT) |	\
		(((sintp_bias)& IINFO_SINTP_BIAS_MSK)<<IINFO_SINTP_BIAS_SFT) ); \
  })
#define SET_REG1_IINFO2(intp,intp_pkg,hldgl,avsdgl,intp_dir,intp_rnd,intp_sft,	\
			sintp,sintp_rnd,sintp_bias)			\
  ({ write_reg( (MOTION_V_BASE+REG1_IINFO2),				\
		(((intp) & IINFO_INTP_MSK)<<IINFO_INTP_SFT) |		\
		(((intp_pkg) & IINFO_INTP_PKG_MSK)<<IINFO_INTP_PKG_SFT) |	\
		(((hldgl) & IINFO_HLDGL_MSK)<<IINFO_HLDGL_SFT) |	\
		(((avsdgl) & IINFO_AVSDGL_MSK)<<IINFO_AVSDGL_SFT) |	\
		(((intp_dir) & IINFO_INTP_DIR_MSK)<<IINFO_INTP_DIR_SFT) |	\
		(((intp_rnd) & IINFO_INTP_RND_MSK)<<IINFO_INTP_RND_SFT) |	\
		(((intp_sft) & IINFO_INTP_SFT_MSK)<<IINFO_INTP_SFT_SFT) |	\
		(((sintp) & IINFO_SINTP_MSK)<<IINFO_SINTP_SFT) |	\
		(((sintp_rnd) & IINFO_SINTP_RND_MSK)<<IINFO_SINTP_RND_SFT) |	\
		(((sintp_bias)& IINFO_SINTP_BIAS_MSK)<<IINFO_SINTP_BIAS_SFT) ); \
  })

#define GET_REG1_IINFO1()				\
  ({ read_reg(  MOTION_V_BASE , REG1_IINFO1  );		\
  })
#define GET_REG1_IINFO2()				\
  ({ read_reg(  MOTION_V_BASE , REG1_IINFO2  );		\
  })

#define REG2_IINFO1           0x834
#define REG2_IINFO2           0x838
#define CH2_IINFO_INTP_SFT       31
#define CH2_IINFO_INTP_MSK       0x1
#define CH2_IINFO_INTP_DIR_SFT   15
#define CH2_IINFO_INTP_DIR_MSK   0x1
#define CH2_IINFO_INTP_SFT_SFT   12
#define CH2_IINFO_INTP_SFT_MSK   0x7
#define CH2_IINFO_INTP_LCOEF_SFT 9
#define CH2_IINFO_INTP_LCOEF_MSK 0x7
#define CH2_IINFO_INTP_RCOEF_SFT 6
#define CH2_IINFO_INTP_RCOEF_MSK 0x7
#define CH2_IINFO_INTP_RND_SFT   0
#define CH2_IINFO_INTP_RND_MSK   0x3F

#define SET_REG2_IINFO1(intp,intp_dir,intp_sft,intp_lcoef,intp_rcoef,intp_rnd)  \
  ({ write_reg( (MOTION_V_BASE+REG2_IINFO1),				        \
		(((intp) & CH2_IINFO_INTP_MSK)<<CH2_IINFO_INTP_SFT) |	        \
		(((intp_dir) & CH2_IINFO_INTP_DIR_MSK)<<CH2_IINFO_INTP_DIR_SFT) |	\
		(((intp_sft) & CH2_IINFO_INTP_SFT_MSK)<<CH2_IINFO_INTP_SFT_SFT) |	\
		(((intp_lcoef) & CH2_IINFO_INTP_LCOEF_MSK)<<CH2_IINFO_INTP_LCOEF_SFT) |	\
		(((intp_rcoef) & CH2_IINFO_INTP_RCOEF_MSK)<<CH2_IINFO_INTP_RCOEF_SFT) |	\
		(((intp_rnd) & CH2_IINFO_INTP_RND_MSK)<<CH2_IINFO_INTP_RND_SFT) ); \
  })
#define SET_REG2_IINFO2(intp,intp_dir,intp_sft,intp_lcoef,intp_rcoef,intp_rnd)  \
  ({ write_reg( (MOTION_V_BASE+REG2_IINFO2),				        \
		(((intp) & CH2_IINFO_INTP_MSK)<<CH2_IINFO_INTP_SFT) |	        \
		(((intp_dir) & CH2_IINFO_INTP_DIR_MSK)<<CH2_IINFO_INTP_DIR_SFT) |	\
		(((intp_sft) & CH2_IINFO_INTP_SFT_MSK)<<CH2_IINFO_INTP_SFT_SFT) |	\
		(((intp_lcoef) & CH2_IINFO_INTP_LCOEF_MSK)<<CH2_IINFO_INTP_LCOEF_SFT) |	\
		(((intp_rcoef) & CH2_IINFO_INTP_RCOEF_MSK)<<CH2_IINFO_INTP_RCOEF_SFT) |	\
		(((intp_rnd) & CH2_IINFO_INTP_RND_MSK)<<CH2_IINFO_INTP_RND_SFT) ); \
  })
#define GET_REG2_IINFO1()				\
  ({ read_reg(  MOTION_V_BASE , REG2_IINFO1  );		\
  })
#define GET_REG2_IINFO2()				\
  ({ read_reg(  MOTION_V_BASE , REG2_IINFO2  );		\
  })

// TAP_COEF
#define REG1_TAP1L           0x3C
#define REG1_TAP2L           0x40
#define REG1_TAP1M           0x44
#define REG1_TAP2M           0x48
#define COEF8_SFT            24
#define COEF8_MSK            0xFF
#define COEF7_SFT            16
#define COEF7_MSK            0xFF
#define COEF6_SFT            8
#define COEF6_MSK            0xFF
#define COEF5_SFT            0
#define COEF5_MSK            0xFF
#define COEF4_SFT            24
#define COEF4_MSK            0xFF
#define COEF3_SFT            16
#define COEF3_MSK            0xFF
#define COEF2_SFT            8
#define COEF2_MSK            0xFF
#define COEF1_SFT            0
#define COEF1_MSK            0xFF

#define SET_REG1_TAP1(coef8,coef7,coef6,coef5,coef4,coef3,coef2,coef1)	\
  ({ write_reg( (MOTION_V_BASE+REG1_TAP1M),				\
		(((coef8) & COEF8_MSK)<<COEF8_SFT) |			\
		(((coef7) & COEF7_MSK)<<COEF7_SFT) |			\
		(((coef6) & COEF6_MSK)<<COEF6_SFT) |			\
		(((coef5) & COEF5_MSK)<<COEF5_SFT) );			\
     write_reg( (MOTION_V_BASE+REG1_TAP1L),				\
		(((coef4) & COEF4_MSK)<<COEF4_SFT) |			\
		(((coef3) & COEF3_MSK)<<COEF3_SFT) |			\
		(((coef2) & COEF2_MSK)<<COEF2_SFT) |			\
		(((coef1) & COEF1_MSK)<<COEF1_SFT) );			\
  })
#define SET_REG1_TAP2(coef8,coef7,coef6,coef5,coef4,coef3,coef2,coef1)	\
  ({ write_reg( (MOTION_V_BASE+REG1_TAP2M),				\
		(((coef8) & COEF8_MSK)<<COEF8_SFT) |			\
		(((coef7) & COEF7_MSK)<<COEF7_SFT) |			\
		(((coef6) & COEF6_MSK)<<COEF6_SFT) |			\
		(((coef5) & COEF5_MSK)<<COEF5_SFT) );			\
     write_reg( (MOTION_V_BASE+REG1_TAP2L),				\
		(((coef4) & COEF4_MSK)<<COEF4_SFT) |			\
		(((coef3) & COEF3_MSK)<<COEF3_SFT) |			\
		(((coef2) & COEF2_MSK)<<COEF2_SFT) |			\
		(((coef1) & COEF1_MSK)<<COEF1_SFT) );			\
  })

#define GET_REG1_TAP1L()			\
  ({ read_reg(  MOTION_V_BASE , REG1_TAP1L  );	\
  })
#define GET_REG1_TAP1M()			\
  ({ read_reg(  MOTION_V_BASE , REG1_TAP1M  );	\
  })
#define GET_REG1_TAP2L()			\
  ({ read_reg(  MOTION_V_BASE , REG1_TAP2L  );	\
  })
#define GET_REG1_TAP2M()			\
  ({ read_reg(  MOTION_V_BASE , REG1_TAP2M  );	\
  })

// STRD
#define REG1_STRD             0x4C
#define REG2_STRD             0x84C
#define REF_STRD_SFT          16
#define REF_STRD_MSK          0xFFF
#define RAW_STRD_SFT          8
#define RAW_STRD_MSK          0xFF
#define DST_STRD_SFT          0
#define DST_STRD_MSK          0xFF

#define SET_REG1_STRD(ref_strd,raw_strd,dst_strd)			\
  ({ write_reg( (mc_base+REG1_STRD),				\
		(((ref_strd) & REF_STRD_MSK)<<REF_STRD_SFT) |		\
		(((raw_strd) & RAW_STRD_MSK)<<RAW_STRD_SFT) |		\
		(((dst_strd) & DST_STRD_MSK)<<DST_STRD_SFT) );		\
  })
#define SET_REG2_STRD(ref_strd,raw_strd,dst_strd)			\
  ({ write_reg( (mc_base+REG2_STRD),				\
		(((ref_strd) & REF_STRD_MSK)<<REF_STRD_SFT) |		\
		(((raw_strd) & RAW_STRD_MSK)<<RAW_STRD_SFT) |		\
		(((dst_strd) & DST_STRD_MSK)<<DST_STRD_SFT) );		\
  })

#define GET_REG1_STRD()				\
  ({ read_reg(  MOTION_V_BASE , REG1_STRD  );	\
  })
#define GET_REG2_STRD()				\
  ({ read_reg(  MOTION_V_BASE , REG2_STRD  );	\
  })

// GEOM
#define REG1_GEOM             0x50
#define GEOM_FH_SFT           16
#define GEOM_FH_MSK           0xFFF
#define GEOM_FW_SFT           0
#define GEOM_FW_MSK           0xFFF

#define SET_REG1_GEOM(fh,fw)						\
  ({ write_reg( (mc_base+REG1_GEOM),				\
		(((fh) & GEOM_FH_MSK)<<GEOM_FH_SFT) |			\
		(((fw) & GEOM_FW_MSK)<<GEOM_FW_SFT) );			\
  })

#define GET_REG1_GEOM()				\
  ({ read_reg(  MOTION_V_BASE , REG1_GEOM  );	\
  })

// DDC
#define REG1_DDC           0x54
#define REG2_DDC           0x854
#define REG1_DSA           0x58
#define REG2_DSA           0x858

#define SET_REG1_DDC(ddc)						\
  ({ write_reg( (MOTION_V_BASE+REG1_DDC), (ddc) );			\
  })
#define SET_REG2_DDC(ddc)						\
  ({ write_reg( (MOTION_V_BASE+REG2_DDC), (ddc) );			\
  })

#define GET_REG1_DDC()				\
  ({ read_reg(  MOTION_V_BASE , REG1_DDC  );	\
  })
#define GET_REG2_DDC()				\
  ({ read_reg(  MOTION_V_BASE , REG2_DDC  );	\
  })

#define SET_REG1_DSA(dsa)						\
  ({ write_reg( (MOTION_V_BASE+REG1_DSA), (dsa) );			\
  })
#define SET_REG2_DSA(dsa)						\
  ({ write_reg( (MOTION_V_BASE+REG2_DSA), (dsa) );			\
  })

#define GET_REG1_DSA()				\
  ({ read_reg(  MOTION_V_BASE , REG1_DSA  );	\
  })
#define GET_REG2_DSA()				\
  ({ read_reg(  MOTION_V_BASE , REG2_DSA  );	\
  })

// PMON
#define PMON1_INTP         0x5C
#define PMON2_INTP         0x85C
#define PMON1_PREF         0x60
#define PMON2_PREF         0x860
#define PMON1_IMISS        0x64
#define PMON2_IMISS        0x864
#define PMON1_PMISS        0x68
#define PMON2_PMISS        0x868
#define PMON1_TRAN         0x6C
#define PMON2_TRAN         0x86C
#define PMON1_WORK         0x70
#define PMON2_WORK         0x870

#define CLR_PMON1_INTP()						\
  ({ write_reg( (MOTION_V_BASE+PMON1_INTP), 0 );			\
  })
#define CLR_PMON2_INTP()						\
  ({ write_reg( (MOTION_V_BASE+PMON2_INTP), 0 );			\
  })

#define GET_PMON1_INTP()			\
  ({ read_reg(  MOTION_V_BASE , PMON1_INTP  );	\
  })
#define GET_PMON2_INTP()			\
  ({ read_reg(  MOTION_V_BASE , PMON2_INTP  );	\
  })

#define CLR_PMON1_PREF()						\
  ({ write_reg( (MOTION_V_BASE+PMON1_PREF), 0 );			\
  })
#define CLR_PMON2_PREF()						\
  ({ write_reg( (MOTION_V_BASE+PMON2_PREF), 0 );			\
  })

#define GET_PMON1_PREF()			\
  ({ read_reg(  MOTION_V_BASE , PMON1_PREF  );	\
  })
#define GET_PMON2_PREF()			\
  ({ read_reg(  MOTION_V_BASE , PMON2_PREF  );	\
  })

#define CLR_PMON1_IMISS()						\
  ({ write_reg( (MOTION_V_BASE+PMON1_IMISS), 0 );			\
  })
#define CLR_PMON2_IMISS()						\
  ({ write_reg( (MOTION_V_BASE+PMON2_IMISS), 0 );			\
  })

#define GET_PMON1_IMISS()			\
  ({ read_reg(  MOTION_V_BASE , PMON1_IMISS  );	\
  })
#define GET_PMON2_IMISS()			\
  ({ read_reg(  MOTION_V_BASE , PMON2_IMISS  );	\
  })

#define CLR_PMON1_PMISS()						\
  ({ write_reg( (MOTION_V_BASE+PMON1_PMISS), 0 );			\
  })
#define CLR_PMON2_PMISS()						\
  ({ write_reg( (MOTION_V_BASE+PMON2_PMISS), 0 );			\
  })

#define GET_PMON1_PMISS()			\
  ({ read_reg(  MOTION_V_BASE , PMON1_PMISS  );	\
  })
#define GET_PMON2_PMISS()			\
  ({ read_reg(  MOTION_V_BASE , PMON2_PMISS  );	\
  })

#define CLR_PMON1_TRAN()						\
  ({ write_reg( (MOTION_V_BASE+PMON1_TRAN), 0 );			\
  })
#define CLR_PMON2_TRAN()						\
  ({ write_reg( (MOTION_V_BASE+PMON2_TRAN), 0 );			\
  })

#define GET_PMON1_TRAN()			\
  ({ read_reg(  MOTION_V_BASE , PMON1_TRAN  );	\
  })
#define GET_PMON2_TRAN()			\
  ({ read_reg(  MOTION_V_BASE , PMON2_TRAN  );	\
  })

#define CLR_PMON1_WORK()						\
  ({ write_reg( (MOTION_V_BASE+PMON1_WORK), 0 );			\
  })
#define CLR_PMON2_WORK()						\
  ({ write_reg( (MOTION_V_BASE+PMON2_WORK), 0 );			\
  })

#define GET_PMON1_WORK()			\
  ({ read_reg(  MOTION_V_BASE , PMON1_WORK  );	\
  })
#define GET_PMON2_WORK()			\
  ({ read_reg(  MOTION_V_BASE , PMON2_WORK  );	\
  })


#define CDD_VLD_SFT        31
#define CDD_VLD_MSK        0x1
#define CDD_LK_SFT         30
#define CDD_LK_MSK         0x1
#define CDD_SYNC_SFT       29
#define CDD_SYNC_MSK       0x1
#define CDD_RUN_SFT        28
#define CDD_RUN_MSK        0x1
#define CDD_CSA_SFT        16
#define CDD_CSA_MSK        0x3F
#define CDD_CTC_SFT        0
#define CDD_CTC_MSK        0x3F

#define CDD_HEAD(vld, lk, sync, run, csa, ctc)  \
  ( (((vld) & CDD_VLD_MSK)<<CDD_VLD_SFT) |	\
    (((lk) & CDD_LK_MSK)<<CDD_LK_SFT) |		\
    (((sync) & CDD_SYNC_MSK)<<CDD_SYNC_SFT) |	\
    (((run) & CDD_RUN_MSK)<<CDD_RUN_SFT) |	\
    (((csa) & CDD_CSA_MSK)<<CDD_CSA_SFT) |	\
    (((ctc) & CDD_CTC_MSK)<<CDD_CTC_SFT) 	\
    )

#define TDD_VLD_SFT        31
#define TDD_VLD_MSK        0x1
#define TDD_LK_SFT         30
#define TDD_LK_MSK         0x1
#define TDD_OP_SFT         28
#define TDD_OP_MSK         0x3
#define TDD_CH1PEL_SFT     27
#define TDD_CH1PEL_MSK     0x1
#define TDD_CH2PEL_SFT     25
#define TDD_CH2PEL_MSK     0x3
#define TDD_POSMD_SFT      24
#define TDD_POSMD_MSK      0x1
#define TDD_MVMD_SFT       23
#define TDD_MVMD_MSK       0x1
#define TDD_TKN_SFT        16
#define TDD_TKN_MSK        0x7F
#define TDD_MBY_SFT        8
#define TDD_MBY_MSK        0xFF
#define TDD_MBX_SFT        0
#define TDD_MBX_MSK        0xFF
#define TDD_HEAD(vld, lk, op, ch1pel, ch2pel, posmd, mvmd,	 \
		 tkn, mby, mbx)					 \
  ( (((vld) & TDD_VLD_MSK)<<TDD_VLD_SFT) |			 \
    (((lk) & TDD_LK_MSK)<<TDD_LK_SFT) |				 \
    (((op) & TDD_OP_MSK)<<TDD_OP_SFT) |			 \
    (((ch1pel) & TDD_CH1PEL_MSK)<<TDD_CH1PEL_SFT) |		 \
    (((ch2pel) & TDD_CH2PEL_MSK)<<TDD_CH2PEL_SFT) |		 \
    (((posmd) & TDD_POSMD_MSK)<<TDD_POSMD_SFT) |		 \
    (((mvmd) & TDD_MVMD_MSK)<<TDD_MVMD_SFT) |			 \
    (((tkn) & TDD_TKN_MSK)<<TDD_TKN_SFT) |			 \
    (((mby) & TDD_MBY_MSK)<<TDD_MBY_SFT) |			 \
    (((mbx) & TDD_MBX_MSK)<<TDD_MBX_SFT)			 \
    )

#define SYN_VLD_SFT        31
#define SYN_VLD_MSK        0x1
#define SYN_LK_SFT         30
#define SYN_LK_MSK         0x1
#define SYN_OP_SFT         28
#define SYN_OP_MSK         0x3
#define SYN_CRST_SFT       27
#define SYN_CRST_MSK       1
#define SYN_VAL_SFT        0
#define SYN_VAL_MSK        0xFFFF
#define SYN_HEAD(vld, lk, op, crst, val)			 \
  ( (((vld) & SYN_VLD_MSK)<<SYN_VLD_SFT) |			 \
    (((lk) & SYN_LK_MSK)<<SYN_LK_SFT) |				 \
    (((op) & SYN_OP_MSK)<<SYN_OP_SFT) |				 \
    (((crst) & SYN_CRST_MSK)<<SYN_CRST_SFT) |			 \
    (((val) & SYN_VAL_MSK)<<SYN_VAL_SFT)			 \
    )

#define TDD_POS_AUTO       0
#define TDD_POS_SPEC       1
#define TDD_MV_AUTO        0
#define TDD_MV_SPEC        1

#define TDD_MVY_SFT        16
#define TDD_MVY_MSK        0xFFFF
#define TDD_MVX_SFT        0
#define TDD_MVX_MSK        0xFFFF
#define TDD_MV(mvy, mvx)                                         \
  ( (((mvy) & TDD_MVY_MSK)<<TDD_MVY_SFT) |			 \
    (((mvx) & TDD_MVX_MSK)<<TDD_MVX_SFT)			 \
    )

#define TDD_BIDIR_SFT      31
#define TDD_BIDIR_MSK      0x1
#define TDD_REFDIR_SFT     30
#define TDD_REFDIR_MSK     0x1
#define TDD_FLD_SFT        29
#define TDD_FLD_MSK        0x1
#define TDD_FLDSEL_SFT     28
#define TDD_FLDSEL_MSK     0x1
#define TDD_RGR_SFT        27
#define TDD_RGR_MSK        0x1
#define TDD_ITS_SFT        26
#define TDD_ITS_MSK        0x1
#define TDD_DOE_SFT        25
#define TDD_DOE_MSK        0x1
#define TDD_CFLO_SFT       24
#define TDD_CFLO_MSK       0x1
#define TDD_YPOS_SFT       20
#define TDD_YPOS_MSK       0xF
#define TDD_LILMD_SFT      18
#define TDD_LILMD_MSK      0x3
#define TDD_CILMD_SFT      16
#define TDD_CILMD_MSK      0x3
#define TDD_LIST_SFT       12
#define TDD_LIST_MSK       0xF
#define TDD_BOY_SFT        10
#define TDD_BOY_MSK        0x3
#define TDD_BOX_SFT        8
#define TDD_BOX_MSK        0x3
#define TDD_BH_SFT         6
#define TDD_BH_MSK         0x3
#define TDD_BW_SFT         4
#define TDD_BW_MSK         0x3
#define TDD_POS_SFT        0
#define TDD_POS_MSK        0xF
#define TDD_CMD(bidir, refdir, fld, fldsel, rgr, its, doe,	 \
		cflo, ypos, lilmd, cilmd, list,			 \
		boy, box, bh, bw, pos)				 \
  ( (((bidir) & TDD_BIDIR_MSK)<<TDD_BIDIR_SFT) |		 \
    (((refdir) & TDD_REFDIR_MSK)<<TDD_REFDIR_SFT) |		 \
    (((fld) & TDD_FLD_MSK)<<TDD_FLD_SFT) |			 \
    (((fldsel) & TDD_FLDSEL_MSK)<<TDD_FLDSEL_SFT) |		 \
    (((rgr) & TDD_RGR_MSK)<<TDD_RGR_SFT) |			 \
    (((its) & TDD_ITS_MSK)<<TDD_ITS_SFT) |			 \
    (((doe) & TDD_DOE_MSK)<<TDD_DOE_SFT) |			 \
    (((cflo) & TDD_CFLO_MSK)<<TDD_CFLO_SFT) |			 \
    (((ypos) & TDD_YPOS_MSK)<<TDD_YPOS_SFT) |			 \
    (((lilmd) & TDD_LILMD_MSK)<<TDD_LILMD_SFT) |		 \
    (((cilmd) & TDD_CILMD_MSK)<<TDD_CILMD_SFT) |		 \
    (((list) & TDD_LIST_MSK)<<TDD_LIST_SFT) |			 \
    (((boy) & TDD_BOY_MSK)<<TDD_BOY_SFT) |			 \
    (((box) & TDD_BOX_MSK)<<TDD_BOX_SFT) |			 \
    (((bh) & TDD_BH_MSK)<<TDD_BH_SFT) |				 \
    (((bw) & TDD_BW_MSK)<<TDD_BW_SFT) |				 \
    (((pos) & TDD_POS_MSK)<<TDD_POS_SFT)			 \
    )

// TLUT
#define TAB1_TLUT          0x100
#define TAB2_TLUT          0x900
#define GET_TAB1_TLUT(idx)				\
  ({ read_reg(  MOTION_V_BASE , TAB1_TLUT+(idx)*4  );	\
  })
#define GET_TAB2_TLUT(idx)				\
  ({ read_reg(  MOTION_V_BASE , TAB2_TLUT+(idx)*4  );	\
  })

// RLUT
#define TAB1_RLUT          0x300
#define TAB2_RLUT          0xB00
#define RLUT_WCOEF2_SFT    24
#define RLUT_WCOEF2_MSK    0xFF
#define RLUT_WOFST2_SFT    16
#define RLUT_WOFST2_MSK    0xFF
#define RLUT_WCOEF1_SFT    8
#define RLUT_WCOEF1_MSK    0xFF
#define RLUT_WOFST1_SFT    0
#define RLUT_WOFST1_MSK    0xFF

#define SET_TAB1_RLUT(idx,roa,wcoef,wofst)				\
  ({ write_reg( (mc_base+TAB1_RLUT+(idx)*8),			\
		(((wcoef ) & RLUT_WCOEF1_MSK)<<RLUT_WCOEF1_SFT) |	\
		(((wofst ) & RLUT_WOFST1_MSK)<<RLUT_WOFST1_SFT) );	\
     write_reg( (mc_base+TAB1_RLUT+(idx)*8+4), (roa) );		\
  })
#define SET_TAB2_RLUT(idx,roa,wcoef2,wofst2,wcoef1,wofst1)		\
  ({ write_reg( (mc_base+TAB2_RLUT+(idx)*8),			\
		(((wcoef2) & RLUT_WCOEF2_MSK)<<RLUT_WCOEF2_SFT) |	\
		(((wofst2) & RLUT_WOFST2_MSK)<<RLUT_WOFST2_SFT) |	\
		(((wcoef1) & RLUT_WCOEF1_MSK)<<RLUT_WCOEF1_SFT) |	\
		(((wofst1) & RLUT_WOFST1_MSK)<<RLUT_WOFST1_SFT) );	\
     write_reg( (mc_base+TAB2_RLUT+(idx)*8+4), (roa) );		\
  })

#define GET_TAB1_RLUT(idx)				\
  ({ read_reg(  MOTION_V_BASE , TAB1_RLUT+(idx)*4  );	\
  })
#define GET_TAB2_RLUT(idx)				\
  ({ read_reg(  MOTION_V_BASE , TAB2_RLUT+(idx)*4  );	\
  })

// CLUT
#define TAB1_CLUT          0x400
#define SET_TAB1_CLUT(idx,coef8,coef7,coef6,coef5,coef4,coef3,coef2,coef1) \
  ({ write_reg( (mc_base+TAB1_CLUT+(idx)*8+4),			\
		(((coef8) & COEF8_MSK)<<COEF8_SFT) |			\
		(((coef7) & COEF7_MSK)<<COEF7_SFT) |			\
		(((coef6) & COEF6_MSK)<<COEF6_SFT) |			\
		(((coef5) & COEF5_MSK)<<COEF5_SFT) );			\
     write_reg( (mc_base+TAB1_CLUT+(idx)*8),			\
		(((coef4) & COEF4_MSK)<<COEF4_SFT) |			\
		(((coef3) & COEF3_MSK)<<COEF3_SFT) |			\
		(((coef2) & COEF2_MSK)<<COEF2_SFT) |			\
		(((coef1) & COEF1_MSK)<<COEF1_SFT) );			\
  })

#define GET_TAB1_CLUT(idx)				\
  ({ read_reg(  MOTION_V_BASE , TAB1_CLUT+(idx)*4  );	\
  })

// ILUT
#define TAB1_ILUT          0x500
#define TAB2_ILUT          0xD00
#define SET_TAB1_ILUT(idx,intp2,intp2_pkg,hldgl,avsdgl,intp2_dir,		\
		      intp2_rnd,intp2_sft,sintp2,sintp2_rnd,sintp2_bias,        \
		      intp1,tap,intp1_pkg,intp1_dir,intp1_rnd,intp1_sft,        \
		      sintp1, sintp1_rnd, sintp1_bias)				\
  ({ write_reg( (mc_base+TAB1_ILUT+(idx)*8),			        \
		(((intp1) & IINFO_INTP_MSK)<<IINFO_INTP_SFT) |		        \
		(((tap) & IINFO_TAP_MSK)<<IINFO_TAP_SFT) |		        \
		(((intp1_pkg) & IINFO_INTP_PKG_MSK)<<IINFO_INTP_PKG_SFT) |      \
		(((intp1_dir) & IINFO_INTP_DIR_MSK)<<IINFO_INTP_DIR_SFT) |	\
		(((intp1_rnd) & IINFO_INTP_RND_MSK)<<IINFO_INTP_RND_SFT) |	\
		(((intp1_sft) & IINFO_INTP_SFT_MSK)<<IINFO_INTP_SFT_SFT) |      \
		(((sintp1) & IINFO_SINTP_MSK)<<IINFO_SINTP_SFT) |        	\
		(((sintp1_rnd) & IINFO_SINTP_RND_MSK)<<IINFO_SINTP_RND_SFT) |   \
		(((sintp1_bias) & IINFO_SINTP_BIAS_MSK)<<IINFO_SINTP_BIAS_SFT));\
     write_reg( (mc_base+TAB1_ILUT+(idx)*8+4),			        \
		(((intp2) & IINFO_INTP_MSK)<<IINFO_INTP_SFT) |		        \
		(((intp2_pkg) & IINFO_INTP_PKG_MSK)<<IINFO_INTP_PKG_SFT) |      \
		(((hldgl) & IINFO_HLDGL_MSK)<<IINFO_HLDGL_SFT) |	        \
		(((avsdgl) & IINFO_AVSDGL_MSK)<<IINFO_AVSDGL_SFT) |	        \
		(((intp2_dir) & IINFO_INTP_DIR_MSK)<<IINFO_INTP_DIR_SFT) |	\
		(((intp2_rnd) & IINFO_INTP_RND_MSK)<<IINFO_INTP_RND_SFT) |	\
		(((intp2_sft) & IINFO_INTP_SFT_MSK)<<IINFO_INTP_SFT_SFT) |      \
		(((sintp2) & IINFO_SINTP_MSK)<<IINFO_SINTP_SFT) |        	\
		(((sintp2_rnd) & IINFO_SINTP_RND_MSK)<<IINFO_SINTP_RND_SFT) |   \
		(((sintp2_bias) & IINFO_SINTP_BIAS_MSK)<<IINFO_SINTP_BIAS_SFT));\
  })
#define GET_TAB1_ILUT(idx)				\
  ({ read_reg(  MOTION_V_BASE , TAB1_ILUT+(idx)*4  );	\
  })

#define SET_TAB2_ILUT(idx,intp2,intp2_dir,intp2_sft,intp2_lcoef,intp2_rcoef,intp2_rnd,  \
		      intp1,intp1_dir,intp1_sft,intp1_lcoef,intp1_rcoef,intp1_rnd)      \
  ({ write_reg( (mc_base+TAB2_ILUT+(idx)*8),			                \
		(((intp1) & CH2_IINFO_INTP_MSK)<<CH2_IINFO_INTP_SFT) |	        \
		(((intp1_dir) & CH2_IINFO_INTP_DIR_MSK)<<CH2_IINFO_INTP_DIR_SFT) |	\
		(((intp1_sft) & CH2_IINFO_INTP_SFT_MSK)<<CH2_IINFO_INTP_SFT_SFT) |	\
		(((intp1_lcoef) & CH2_IINFO_INTP_LCOEF_MSK)<<CH2_IINFO_INTP_LCOEF_SFT) |\
		(((intp1_rcoef) & CH2_IINFO_INTP_RCOEF_MSK)<<CH2_IINFO_INTP_RCOEF_SFT) |\
		(((intp1_rnd) & CH2_IINFO_INTP_RND_MSK)<<CH2_IINFO_INTP_RND_SFT) );     \
     write_reg( (mc_base+TAB2_ILUT+(idx)*8+4),			                \
		(((intp2) & CH2_IINFO_INTP_MSK)<<CH2_IINFO_INTP_SFT) |	        \
		(((intp2_dir) & CH2_IINFO_INTP_DIR_MSK)<<CH2_IINFO_INTP_DIR_SFT) |	\
		(((intp2_sft) & CH2_IINFO_INTP_SFT_MSK)<<CH2_IINFO_INTP_SFT_SFT) |	\
		(((intp2_lcoef) & CH2_IINFO_INTP_LCOEF_MSK)<<CH2_IINFO_INTP_LCOEF_SFT) |\
		(((intp2_rcoef) & CH2_IINFO_INTP_RCOEF_MSK)<<CH2_IINFO_INTP_RCOEF_SFT) |\
		(((intp2_rnd) & CH2_IINFO_INTP_RND_MSK)<<CH2_IINFO_INTP_RND_SFT) );     \
  })
#define GET_TAB2_ILUT(idx)				\
  ({ read_reg(  MOTION_V_BASE , TAB2_ILUT+(idx)*4  );	\
  })

#endif /*__T_MOTION_H__*/
