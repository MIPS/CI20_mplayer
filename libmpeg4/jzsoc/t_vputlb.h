#ifndef __T_VPUTLB_H__
#define __T_VPUTLB_H__

extern volatile unsigned char *vpu_base;
#define VPUTLB_V_BASE       (int)vpu_base
//#define VPUTLB_V_BASE       (0xB3240000)

#define PSIZE_4M            0
#define PSIZE_8M            1
#define PSIZE_16M           2
#define PSIZE_32M           3

#define VALID_SFT           0
#define VALID_MSK           0x1
#define PSIZE_SFT           1
#define PSIZE_MSK           0x3
#define PTAG_SFT            12
#define PTAG_MSK            0x3FF
#define VTAG_SFT            22
#define VTAG_MSK            0x3FF

#define SET_VPU_TLB(entry, valid, psize, vtag, ptag)	 \
  ({ *(volatile int *)(VPUTLB_V_BASE + (entry)*4) = 	 \
      ((valid) & VALID_MSK)<<VALID_SFT |		 \
      ((psize) & PSIZE_MSK)<<PSIZE_SFT |		 \
      ((vtag ) & VTAG_MSK )<<VTAG_SFT  |                 \
      ((ptag ) & PTAG_MSK )<<PTAG_SFT  ;                 \
  })

#define GET_VPU_TLB(entry) (*(volatile int *)(VPUTLB_V_BASE + (entry)*4))

#define VPUCTRL_V_BASE      0xB3408000

#define SET_VPU_CTRL(ack, stop, rst)			 \
  ({ *(volatile int *)(VPUCTRL_V_BASE) =		 \
      ((ack ) & 0x1)<<2 |				 \
      ((stop) & 0x1)<<1 |				 \
      ((rst ) & 0x1)<<0 ;				 \
  })

#define GET_VPU_CTRL()     (*(volatile int *)(VPUCTRL_V_BASE))

#endif /*__T_MOTION_H__*/
