/*****************************************************************************
 *
 * JZ4760 TCSM0 Space Seperate
 *
 ****************************************************************************/

#ifndef __VP6_TCSM0_H__
#define __VP6_TCSM0_H__


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
#define TCSM0_VCADDR(a)       (((a) & 0xFFFF) | 0x932B0000) 
#define TCSM0_VUCADDR(a)      (((a) & 0xFFFF) | 0xB32B0000) 

#define TCSM1_PADDR(a)        ((((unsigned)a) & 0xFFFF) | 0x132C0000) 
#define TCSM1_VUCADDR(a)      ((((unsigned)a) & 0xFFFF) | 0xB32C0000) 

#define DDMA_GP0_DES_CHAIN_LEN   ((4*6)<<2)
#define DDMA_GP0_DES_CHAIN       (TCSM0_BANK2)
#define TCSM0_MC_BUF             (DDMA_GP0_DES_CHAIN + DDMA_GP0_DES_CHAIN_LEN)       
#endif /*__VP6_TCSM_H0__*/
