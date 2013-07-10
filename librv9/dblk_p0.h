#ifndef __JZC_RV_DBLK_H__
#define __JZC_RV_DBLK_H__

#define TYAW 20
#define TYAH 24
#define TCAW 12
#define TCAH 16

volatile unsigned char *dblk0_base;
//#define DBLK_BASE 0x13270000
#define DBLK_BASE dblk0_base

#define OFST_CTR 0x0
#define OFST_FMT 0x4
#define OFST_CBP 0x8
#define OFST_CBP_LEFT 0xC
#define OFST_CBP_ABOVE 0x10
#define OFST_MVD 0x14
#define OFST_MVD_ADJ 0x18
#define OFST_IN_YADDR 0x1C
#define OFST_IN_UADDR 0x20
#define OFST_IN_VADDR 0x24
#define OFST_UPOUT_YADDR 0x28
#define OFST_UPOUT_UADDR 0x2C
#define OFST_UPOUT_VADDR 0x30
#define OFST_UPIN_YADDR 0x34
#define OFST_UPIN_UADDR 0x38
#define OFST_UPIN_VADDR 0x3C
#define OFST_OUT_YADDR 0x40
#define OFST_OUT_UADDR 0x44
#define OFST_OUT_VADDR 0x48
#define OFST_INXCHG_YADDR 0x4C
#define OFST_INXCHG_UADDR 0x50
#define OFST_INXCHG_VADDR 0x54
#define OFST_OUTXCHG_YADDR 0x58
#define OFST_OUTXCHG_UADDR 0x5C
#define OFST_OUTXCHG_VADDR 0x60
#define OFST_UPIN_YSTRD 0x64
#define OFST_UPIN_UVSTRD 0x68
#define OFST_UPOUT_YSTRD 0x6C
#define OFST_UPOUT_UVSTRD 0x70
#define OFST_IN_YSTRD 0x74
#define OFST_IN_UVSTRD 0x78
#define OFST_OUT_YSTRD 0x7C
#define OFST_OUT_UVSTRD 0x80
#define OFST_DDMA_DCS 0x84
#define OFST_DDMA_DHA 0x88
#define OFST_DDMA_CSA 0x8C
#define OFST_ABC_START 0xA0

#define OFST_ABC_QP0 (OFST_ABC_START+0x0)
#define OFST_ABC_QP1 (OFST_ABC_START+0x4)
#define OFST_ABC_QP2 (OFST_ABC_START+0x8)
#define OFST_ABC_QP3 (OFST_ABC_START+0xC)
#define OFST_ABC_QP4 (OFST_ABC_START+0x10)
#define OFST_ABC_QP5 (OFST_ABC_START+0x14)
#define OFST_ABC_QP6 (OFST_ABC_START+0x18)
#define OFST_ABC_QP7 (OFST_ABC_START+0x1C)
#define OFST_ABC_QP8 (OFST_ABC_START+0x20)
#define OFST_ABC_QP9 (OFST_ABC_START+0x24)
#define OFST_ABC_QP10 (OFST_ABC_START+0x28)
#define OFST_ABC_QP11 (OFST_ABC_START+0x2C)
#define OFST_ABC_QP12 (OFST_ABC_START+0x30)
#define OFST_ABC_QP13 (OFST_ABC_START+0x34)
#define OFST_ABC_QP14 (OFST_ABC_START+0x38)
#define OFST_ABC_QP15 (OFST_ABC_START+0x3C)
#define OFST_ABC_QP16 (OFST_ABC_START+0x40)
#define OFST_ABC_QP17 (OFST_ABC_START+0x44)
#define OFST_ABC_QP18 (OFST_ABC_START+0x48)
#define OFST_ABC_QP19 (OFST_ABC_START+0x4C)
#define OFST_ABC_QP20 (OFST_ABC_START+0x50)
#define OFST_ABC_QP21 (OFST_ABC_START+0x54)
#define OFST_ABC_QP22 (OFST_ABC_START+0x58)
#define OFST_ABC_QP23 (OFST_ABC_START+0x5C)
#define OFST_ABC_QP24 (OFST_ABC_START+0x60)
#define OFST_ABC_QP25 (OFST_ABC_START+0x64)
#define OFST_ABC_QP26 (OFST_ABC_START+0x68)
#define OFST_ABC_QP27 (OFST_ABC_START+0x6C)
#define OFST_ABC_QP28 (OFST_ABC_START+0x70)
#define OFST_ABC_QP29 (OFST_ABC_START+0x74)
#define OFST_ABC_QP30 (OFST_ABC_START+0x78)
#define OFST_ABC_QP31 (OFST_ABC_START+0x7C)

//#define write_reg(addr, val) ({   i_sw((val), (addr), 0);   })
//#define read_reg(reg, off) ({     i_lw((reg), (off));   })

#define SET_DBLK_CTRL(UPDATE_ADDR,TRAN_EVER,MBOUT_MNTN,MBOUT_XCHG,MBOUT_WRAP, \
		      MBIN_MNTN,MBIN_XCHG,MBIN_WRAP,UP_OUT_MNTN,UP_OUT_INCR,UP_IN_MNTN,UP_IN_INCR, \
		      QCIF_BETA2,DBLK_END,DBLK_RST,DBLK_EN)		  \
({write_reg((DBLK_BASE + OFST_CTR), \
            ((UPDATE_ADDR << 31) +	    \
	     (TRAN_EVER << 30) +	    \
	     (MBOUT_MNTN << 29) +	    \
	     (MBOUT_XCHG << 28) +	    \
	     (MBOUT_WRAP << 27) +	    \
	     (MBIN_MNTN << 26) +	    \
	     (MBIN_XCHG << 25) +	    \
	     (MBIN_WRAP << 24) +	    \
	     (UP_OUT_MNTN << 23) +	    \
	     (UP_OUT_INCR << 22) +	    \
	     (UP_IN_MNTN << 21) +	    \
	     (UP_IN_INCR << 20) +	    \
	     (QCIF_BETA2 << 3) +	    \
             (DBLK_END << 2) +   \
             (DBLK_RST << 1) +	    \
             DBLK_EN));})

#define SET_DBLK_DATA_FMT(REFQP, QP,	ROW_END,				\
		     FRAME_TYPE, TOP_MBS, BOTTOM_MBS, VIDEO_FMT)		\
     ({write_reg((DBLK_BASE + OFST_FMT), \
((REFQP << 24) + \
(QP << 16) + \
(ROW_END << 15) + \
(FRAME_TYPE << 4) + \
(TOP_MBS << 3) + \
(BOTTOM_MBS << 2) + \
VIDEO_FMT)); })

#define SET_DBLK_SIMPLE_CTRL(DATA)		\
     ({write_reg((DBLK_BASE + OFST_CTR), \
		 DATA); })
#define SET_DBLK_SIMPLE_DATA_FMT(DATA)		\
     ({write_reg((DBLK_BASE + OFST_FMT), \
		 DATA); })

#define SET_DBLK_CBP(CBP)		\
  ({write_reg((DBLK_BASE + OFST_CBP),	\
	      (CBP)); })

#define SET_DBLK_CBP_LEFT(CBP_LEFT)		\
  ({write_reg((DBLK_BASE + OFST_CBP_LEFT),	\
	      (CBP_LEFT)); })

#define SET_DBLK_CBP_ABOVE(CBP_ABOVE)		\
  ({write_reg((DBLK_BASE + OFST_CBP_ABOVE),	\
	      (CBP_ABOVE)); })

#define SET_DBLK_MVD(MVD)		\
  ({write_reg((DBLK_BASE + OFST_MVD),	\
	      (MVD)); })

#define SET_DBLK_MVD_ADJ(MVD_ADJ)		\
  ({write_reg((DBLK_BASE + OFST_MVD_ADJ),	\
	      (MVD_ADJ)); })

#define SET_DBLK_UPIN_YADDR(UPIN_YADDR)		\
  ({write_reg((DBLK_BASE + OFST_UPIN_YADDR),	\
	      (UPIN_YADDR)); })

#define SET_DBLK_UPIN_UADDR(UPIN_UADDR)		\
  ({write_reg((DBLK_BASE + OFST_UPIN_UADDR),	\
	      (UPIN_UADDR)); })

#define SET_DBLK_UPIN_VADDR(UPIN_VADDR)		\
  ({write_reg((DBLK_BASE + OFST_UPIN_VADDR),	\
	      (UPIN_VADDR)); })

#define SET_DBLK_UPOUT_YADDR(UPOUT_YADDR)		\
  ({write_reg((DBLK_BASE + OFST_UPOUT_YADDR),	\
	      (UPOUT_YADDR)); })

#define SET_DBLK_UPOUT_UADDR(UPOUT_UADDR)		\
  ({write_reg((DBLK_BASE + OFST_UPOUT_UADDR),	\
	      (UPOUT_UADDR)); })

#define SET_DBLK_UPOUT_VADDR(UPOUT_VADDR)		\
  ({write_reg((DBLK_BASE + OFST_UPOUT_VADDR),	\
	      (UPOUT_VADDR)); })

#define SET_DBLK_IN_YADDR(IN_YADDR)		\
  ({write_reg((DBLK_BASE + OFST_IN_YADDR),	\
	      (IN_YADDR)); })

#define SET_DBLK_IN_UADDR(IN_UADDR)		\
  ({write_reg((DBLK_BASE + OFST_IN_UADDR),	\
	      (IN_UADDR)); })

#define SET_DBLK_IN_VADDR(IN_VADDR)		\
  ({write_reg((DBLK_BASE + OFST_IN_VADDR),	\
	      (IN_VADDR)); })

#define SET_DBLK_OUT_YADDR(OUT_YADDR)		\
  ({write_reg((DBLK_BASE + OFST_OUT_YADDR),	\
	      (OUT_YADDR)); })

#define SET_DBLK_OUT_UADDR(OUT_UADDR)		\
  ({write_reg((DBLK_BASE + OFST_OUT_UADDR),	\
	      (OUT_UADDR)); })

#define SET_DBLK_OUT_VADDR(OUT_VADDR)		\
  ({write_reg((DBLK_BASE + OFST_OUT_VADDR),	\
	      (OUT_VADDR)); })

#define SET_DBLK_INXCHG_YADDR(INXCHG_YADDR)		\
  ({write_reg((DBLK_BASE + OFST_INXCHG_YADDR),	\
	      (INXCHG_YADDR)); })

#define SET_DBLK_INXCHG_UADDR(INXCHG_UADDR)		\
  ({write_reg((DBLK_BASE + OFST_INXCHG_UADDR),	\
	      (INXCHG_UADDR)); })

#define SET_DBLK_INXCHG_VADDR(INXCHG_VADDR)		\
  ({write_reg((DBLK_BASE + OFST_INXCHG_VADDR),	\
	      (INXCHG_VADDR)); })

#define SET_DBLK_OUTXCHG_YADDR(OUTXCHG_YADDR)		\
  ({write_reg((DBLK_BASE + OFST_OUTXCHG_YADDR),	\
	      (OUTXCHG_YADDR)); })

#define SET_DBLK_OUTXCHG_UADDR(OUTXCHG_UADDR)		\
  ({write_reg((DBLK_BASE + OFST_OUTXCHG_UADDR),	\
	      (OUTXCHG_UADDR)); })

#define SET_DBLK_OUTXCHG_VADDR(OUTXCHG_VADDR)		\
  ({write_reg((DBLK_BASE + OFST_OUTXCHG_VADDR),	\
	      (OUTXCHG_VADDR)); })

#define SET_DBLK_UPIN_YSTRD(UPIN_YSTRD)		\
  ({write_reg((DBLK_BASE + OFST_UPIN_YSTRD),	\
	      (UPIN_YSTRD)); })

#define SET_DBLK_UPIN_UVSTRD(UPIN_UVSTRD)		\
  ({write_reg((DBLK_BASE + OFST_UPIN_UVSTRD),	\
	      (UPIN_UVSTRD)); })

#define SET_DBLK_UPOUT_YSTRD(UPOUT_YSTRD)		\
  ({write_reg((DBLK_BASE + OFST_UPOUT_YSTRD),	\
	      (UPOUT_YSTRD)); })

#define SET_DBLK_UPOUT_UVSTRD(UPOUT_UVSTRD)		\
  ({write_reg((DBLK_BASE + OFST_UPOUT_UVSTRD),	\
	      (UPOUT_UVSTRD)); })

#define SET_DBLK_IN_YSTRD(IN_YSTRD)		\
  ({write_reg((DBLK_BASE + OFST_IN_YSTRD),	\
	      (IN_YSTRD)); })

#define SET_DBLK_IN_UVSTRD(IN_UVSTRD)		\
  ({write_reg((DBLK_BASE + OFST_IN_UVSTRD),	\
	      (IN_UVSTRD)); })

#define SET_DBLK_OUT_YSTRD(OUT_YSTRD)		\
  ({write_reg((DBLK_BASE + OFST_OUT_YSTRD),	\
	      (OUT_YSTRD)); })

#define SET_DBLK_OUT_UVSTRD(OUT_UVSTRD)		\
  ({write_reg((DBLK_BASE + OFST_OUT_UVSTRD),	\
	      (OUT_UVSTRD)); })

#define SET_DBLK_DDMA_DCS(DDMA_DCS)		\
  ({write_reg((DBLK_BASE + OFST_DDMA_DCS),	\
	      (DDMA_DCS)); })

#define SET_DBLK_DDMA_DHA(DDMA_DHA)		\
  ({write_reg((DBLK_BASE + OFST_DDMA_DHA),	\
	      (DDMA_DHA)); })

#define SET_DBLK_ABC_LUT(offset, abc) \
  ({write_reg((DBLK_BASE + OFST_ABC_START + (offset)), abc);})

#define FILL_DBLK_ABC_TABLE(tbl_adr) \
  {int abc_counter;						\
    for(abc_counter=0;abc_counter<32;abc_counter++)		\
      SET_DBLK_ABC_LUT((abc_counter<<2), tbl_adr[abc_counter]);	\
  }

#define POLL_DBLK_END_FLAG() \
({read_reg((DBLK_BASE + OFST_CTR), 0) & 0x4 ;})

#define SET_DBLK_DCS(val)			\
({write_reg((DBLK_BASE + OFST_DCS), val);})

#define SET_DBLK_DHA(val) \
({write_reg((DBLK_BASE + OFST_DHA), val);})


#define dblk_polling_end()                                      \
  { I32 i, md=0, tmp=-32;        				\
    do {							\
      if((md>>3) & 0x1) tmp -= 32;				\
      else tmp += 32;						\
      md++;							\
      for (i=0; i<tmp; i++);					\
    } while (!POLL_DBLK_END_FLAG());				\
}                         

#define SET_DDMA_CTRL(ddma_en) 	\
({write_reg((DBLK_BASE + OFST_DDMA_DCS), \
             ddma_en);})

#define SET_DDMA_DHA(dha) ({write_reg((DBLK_BASE + OFST_DDMA_DHA), \
                                        dha); })

#endif //__JZC_RV_DBLK_H__
