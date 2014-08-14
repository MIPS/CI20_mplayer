#ifndef __VP8_SRAM_H__
#define __VP8_SRAM_H__

#define SRAM_BANK0  0x132F0000
#define SRAM_BANK1  0x132F1000
#define SRAM_BANK2  0x132F2000
#define SRAM_BANK3  0x132F3000

#define SRAM_PADDR(a)         ((((unsigned)(a)) & 0xFFFF) | 0x132F0000)
#define SRAM_VCADDR(a)        ((((unsigned)(a)) & 0xFFFF) | 0xB32F0000)

#define VP8_BLOCK_LEN         (800)
#define VP8_BLOCK0            (SRAM_BANK1)
#define VP8_BLOCK1            (VP8_BLOCK0+VP8_BLOCK_LEN)
#define VP8_BLOCK2            (VP8_BLOCK1+VP8_BLOCK_LEN)

#define VP8_MB_LEN            80
#define VP8_MB0               (VP8_BLOCK2+VP8_BLOCK_LEN)
#define VP8_MB1               (VP8_MB0+80)
#define VP8_MB2               (VP8_MB1+80)

#endif
