/*****************************************************************************
 *
 * JZ4760E TCSM0 Space Seperate
 *
 ****************************************************************************/

#ifndef __RV30_TCSM0_H__
#define __RV30_TCSM0_H__


#define TCSM0_BANK0 0xF4000000
#define TCSM0_BANK1 0xF4001000
#define TCSM0_BANK2 0xF4002000
#define TCSM0_BANK3 0xF4003000
#define TCSM0_END   0xF4004000
/*
  XXXX_PADDR:       physical address
  XXXX_VCADDR:      virtual cache-able address
  XXXX_VUCADDR:     virtual un-cache-able address 
*/
#define TCSM0_PADDR(a)        (((a) & 0xFFFF) | 0x132B0000) 
#define TCSM0_VCADDR(a)       (((a) & 0xFFFF) | 0xB32B0000) 
#define TCSM0_VUCADDR(a)      (((a) & 0xFFFF) | 0xB32B0000) 

/*--------------------------------------------------
 * P1 to P0 interaction signals,
 * P0 only read, P1 should only write
 --------------------------------------------------*/
#define BLK16_BUF  TCSM0_BANK0
#define COEF_BUF1  BLK16_BUF+32
#define COEF_BUF2  COEF_BUF1+64
#define IDCT_OUT1  COEF_BUF2+64
#define IDCT_OUT2  IDCT_OUT1+32
#define BLK_BUF    IDCT_OUT2+32
#define IDCT_DES_CHAIN BLK_BUF+768
#define GP0_DES_CHAIN  IDCT_DES_CHAIN+20
#define RECON_YBUF     GP0_DES_CHAIN+16
#define RECON_UBUF     RECON_YBUF+256
#define RECON_VBUF     RECON_UBUF+64

#define MC_YBUF        RECON_VBUF+256
#define MC_UBUF        MC_YBUF+256
#define MC_VBUF        MC_UBUF+64
#endif /*__RV30_TCSM_H0__*/
