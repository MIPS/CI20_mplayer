/*****************************************************************************
 *
 * JZ4760 TCSM1 Space Seperate
 *
 * $Id: vp8_tcsm1.h,v 1.1.1.1 2012/03/27 04:02:56 dqliu Exp $
 *
 ****************************************************************************/

#ifndef __VP8_TCSM1_H__
#define __VP8_TCSM1_H__

#define TCSM1_BANK0 0xF4000000
#define TCSM1_BANK1 0xF4001000
#define TCSM1_BANK2 0xF4002000
#define TCSM1_BANK3 0xF4003000

#define TCSM1_BANK4 0xF4004000
#define TCSM1_BANK5 0xF4005000
#define TCSM1_BANK6 0xF4006000
#define TCSM1_BANK7 0xF4007000

#define TCSM1_PADDR(a)        ((((unsigned)(a)) & 0xFFFF) | 0x132C0000) 
extern volatile unsigned char *tcsm1_base;
#define TCSM1_VCADDR(a)       ((((unsigned)(a)) & 0xFFFF) | 0xB32C0000) 
//#define TCSM1_VCADDR(a)      (tcsm1_base + ((a) & 0xFFFF))

#define P1_MAIN_ADDR (TCSM1_BANK0)

#define TCSM1_SLICE_BUF0 (TCSM1_BANK4)

#define TASK_BUF_LEN            32

#define PREVIOUS_LUMA_STRIDE    (4+16)
#define PREVIOUS_CHROMA_STRIDE  (4+8)

#define MC_CHN_ONELEN  (19 << 2)
#define MC_CHN_DEP  6
#define MC_CHN_LEN  (MC_CHN_ONELEN*MC_CHN_DEP)
#define TCSM1_MOTION_DHA0        (TCSM1_SLICE_BUF0+TASK_BUF_LEN)
#define TCSM1_MOTION_DSA0        (TCSM1_MOTION_DHA0 + MC_CHN_LEN)
#define TCSM1_MOTION_DHA1        (TCSM1_MOTION_DSA0+TASK_BUF_LEN)
#define TCSM1_MOTION_DSA1        (TCSM1_MOTION_DHA1 + MC_CHN_LEN)
#define TCSM1_MOTION_DHA2        (TCSM1_MOTION_DSA1+TASK_BUF_LEN)
#define TCSM1_MOTION_DSA2        (TCSM1_MOTION_DHA2 + MC_CHN_LEN)

#define VMAU_YOUT_STR  PREVIOUS_LUMA_STRIDE
#define VMAU_COUT_STR  PREVIOUS_CHROMA_STRIDE

#define DDMA_GP1_DES_CHAIN_LEN  ((4*13)<<2)
#define DDMA_GP1_DES_CHAIN0      (TCSM1_MOTION_DSA2+0x4)
#define DDMA_GP1_DES_CHAIN1      (DDMA_GP1_DES_CHAIN0 + DDMA_GP1_DES_CHAIN_LEN)

#define TCSM1_GP1_POLL_END                 (DDMA_GP1_DES_CHAIN1 + DDMA_GP1_DES_CHAIN_LEN)
#define TCSM1_GP0_POLL_END                 (TCSM1_GP1_POLL_END + 4)

#define DOUT_Y_STRD             (4+16)
#define DOUT_C_STRD             (4+8)

#define VP8_DYBUF_LEN           (DOUT_Y_STRD*16)
#define VP8_DCBUF_LEN           (DOUT_C_STRD* 8)

#define ZERO_BLOCK_LEN          800
#define ZERO_BLOCK              (TCSM1_GP0_POLL_END+4)

#define TCSM1_BANK8             0xF4008000

#endif 

