#ifndef __VP8_TCSM0_H__
#define __VP8_TCSM0_H__

#define TCSM0_BANK0 0xF4000000
#define TCSM0_BANK1 0xF4001000
#define TCSM0_BANK2 0xF4002000
#define TCSM0_BANK3 0xF4003000
#define TCSM0_END   0xF4004000

#define TCSM0_PADDR(a)        (((unsigned)(a) & 0xFFFF) | 0x132B0000) 
#define TCSM0_VCADDR(a)       (((unsigned)(a) & 0xFFFF) | 0xF4000000) 

#define VP8_BLOCK_LEN         (800)
#define VP8_BLOCK0            (TCSM0_BANK1)
#define VP8_BLOCK1            (VP8_BLOCK0+VP8_BLOCK_LEN)
#define VP8_BLOCK2            (VP8_BLOCK1+VP8_BLOCK_LEN)

#define VP8_MB_LEN            80
#define VP8_MB0               (VP8_BLOCK2+VP8_BLOCK_LEN)
#define VP8_MB1               (VP8_MB0+80)
#define VP8_MB2               (VP8_MB1+80)

#define TCSM0_GP1_POLL_END    (VP8_MB2+80)

#define TCSM0_RECONY          (TCSM0_GP1_POLL_END+4)

#define T0_RECON_YBUF0             (TCSM0_RECONY+256)
#define T0_RECON_CBUF0             (T0_RECON_YBUF0 + 512)

#define T0_RECON_YBUF1             (T0_RECON_CBUF0 + 256)
#define T0_RECON_CBUF1             (T0_RECON_YBUF1 + 512)

#define T0_RECON_YBUF2             (T0_RECON_CBUF1 + 256)
#define T0_RECON_CBUF2             (T0_RECON_YBUF2 + 512)

#define DDMA_GP0_DES_CHAIN_LEN  (8<<4)
#define DDMA_GP0_DES_CHAIN      (T0_RECON_CBUF2+256)

#define VMAU_CHN_ONELEN  (9<<2)
#define VMAU_CHN_DEP  6
#define VMAU_CHN_LEN  (VMAU_CHN_ONELEN*VMAU_CHN_DEP)
#define VMAU_CHN_BASE0           (DDMA_GP0_DES_CHAIN+DDMA_GP0_DES_CHAIN_LEN)
#define VMAU_END_FLAG            (VMAU_CHN_BASE0+VMAU_CHN_LEN)
#define VMAU_CHN_BASE1           (VMAU_END_FLAG+4)
#define VMAU_END_FLAG1           (VMAU_CHN_BASE1+VMAU_CHN_LEN)
#define VMAU_CHN_BASE2           (VMAU_END_FLAG1+4)
#define VMAU_END_FLAG2           (VMAU_CHN_BASE2+VMAU_CHN_LEN)

#endif
