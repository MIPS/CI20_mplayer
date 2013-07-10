#ifndef __JZC_VLC_HW_H__
#define __JZC_VLC_HW_H__

#define i_mtc0_2(src0,src1,src2)	\
({ __asm__ __volatile__ ("mtc0\t%0,$%1,%2"::"r"(src0),"i"(src1),"i"(src2));})

#define i_mfc0_2(src0,src1)								\
(	{unsigned long _dst_; 								\
 	__asm__ __volatile__ ("mfc0\t%0,$%1,%2":"=r"(_dst_):"i"(src0),"i"(src1));	\
	_dst_;})	

#define i_nop  ({__asm__ __volatile__("nop\t#i_nop":::"memory");})

#define i_wait()  ({__asm__ __volatile__("wait\t#i_wait":::"memory");})

extern volatile unsigned char *sde_base;
#define vlc_base (sde_base)

#define CPU_CTRL_OFST		0x0
#define CPU_RSLT_OFST		0x1
#define CPU_BS_ADDR_OFST	0x2
#define CPU_BS_FOST_OFST	0x3
#define CPU_CFG_OFST		0x4

#define VLC_GLOBAL_OFST		0x30
#define VLC_TBL_OFST		0x2000

#define CPU_WR_VLC(ofst,data) ({i_mtc0_2(data, 21, ofst);})
#define CPU_RD_VLC(ofst)      ({i_mfc0_2(21, ofst);})

//#define SET_GLOBAL_CTRL(val)	({write_reg((VLC_VBASE + VLC_GLOBAL_OFST), (val));})
#define SET_GLOBAL_CTRL(val)	({*((unsigned int *)((unsigned int)vlc_base + VLC_GLOBAL_OFST)) = (val);})
#define GET_GLOBAL_CTRL()	({*((unsigned int*)((unsigned int)vlc_base + VLC_GLOBAL_OFST))})

//#define CPU_SET_CTRL(val)       ({CPU_WR_VLC((CPU_CTRL_OFST), (val)); i_nop;i_nop;i_nop;})
#define CPU_SET_CTRL(val)       CPU_WR_VLC((CPU_CTRL_OFST), (val))
#define CPU_SET_BS_ADDR(val)	CPU_WR_VLC((CPU_BS_ADDR_OFST), (val))
#define CPU_SET_BS_OFST(val)	CPU_WR_VLC((CPU_BS_FOST_OFST), (val))
#define CPU_SET_CFG(val)	CPU_WR_VLC((CPU_CFG_OFST), (val))

//#define CPU_GET_RSLT()		CPU_RD_VLC(CPU_RSLT_OFST)
#define CPU_GET_CFG()		CPU_RD_VLC(CPU_CFG_OFST)
#define CPU_GET_BS_ADDR()	CPU_RD_VLC(CPU_BS_ADDR_OFST)
#define CPU_GET_BS_OFST()	CPU_RD_VLC(CPU_BS_FOST_OFST)
#define CPU_GET_CTRL()		CPU_RD_VLC(CPU_CTRL_OFST)
//#define CPU_GET_RSLT()		({CPU_RD_VLC(CPU_CTRL_OFST); CPU_RD_VLC(CPU_RSLT_OFST);})
#define CPU_GET_RSLT()		({CPU_RD_VLC(CPU_RSLT_OFST);})

#define GET_PHY_ADDR(addr) (((unsigned int)addr) & 0x1FFFFFFF)


#endif // __JZC_VLC_HW_H__
