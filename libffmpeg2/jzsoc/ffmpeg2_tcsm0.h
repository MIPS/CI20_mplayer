/*****************************************************************************
 *
 * JZ4760 TCSM0 Space Seperate
 *
 ****************************************************************************/

#ifndef __FFMPEG2_TCSM0_H__
#define __FFMPEG2_TCSM0_H__


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


#define SPACE_TYPE_CHAR  0x0
#define SPACE_TYPE_SHORT 0x1
#define SPACE_TYPE_WORD  0x2

#define TCSM_BANK0_OFST	0x0
#define TCSM_BANK1_OFST	0x1000
#define TCSM_BANK2_OFST	0x2000
#define TCSM_BANK3_OFST	0x3000

#define SPACE_HALF_MILLION_BYTE 0x80000

#define JZC_CACHE_LINE 32

/*--------------------------------------------------
 * P1 to P0 interaction signals,
 * P0 only read, P1 should only write
 --------------------------------------------------*/
#define TCSM0_COMD_BUF_LEN 32
#define TCSM0_P1_TASK_DONE TCSM0_BANK0
#define TCSM0_P1_FIFO_RP   (TCSM0_P1_TASK_DONE + 4) //start ADDRESS of using task

#define TCSM0_P1_DEBUG     (TCSM0_P1_FIFO_RP + 4)

/*--------------------------------------------------
 * P1 TASK_FIFO
 --------------------------------------------------*/
#define TCSM0_TASK_FIFO TCSM0_BANK1

/*--------------------------------------------------
 * P1 Boot Codes
 --------------------------------------------------*/
#define TCSM1_BOOT_CODE (TCSM1_VCADDR(TCSM1_CPUBASE))
#define P1_BOOT_CODE_LEN (64 << 4)

#endif /*__FFMPEG2_TCSM_H0__*/
