/*****************************************************************************
 *
 * JZ4760 TCSM0 Space Seperate
 *
 ****************************************************************************/

#ifndef __H264_TCSM0_H__
#define __H264_TCSM0_H__


#define TCSM0_BANK0 0xF4000000
#define TCSM0_BANK1 0xF4001000
#define TCSM0_BANK2 0xF4002000
#define TCSM0_BANK3 0xF4003000

/*
  XXXX_PADDR:       physical address
  XXXX_VCADDR:      virtual cache-able address
  XXXX_VUCADDR:     virtual un-cache-able address 
*/
#define TCSM0_PADDR(a)        (((a) & 0xFFFF) | 0x132B0000) 
#define TCSM0_VCADDR(a)       (((a) & 0xFFFF) | 0xB32B0000) 
#define TCSM0_VUCADDR(a)      (((a) & 0xFFFF) | 0xB32B0000) 


#define SPACE_TYPE_CHAR 0x0
#define SPACE_TYPE_SHORT 0x1
#define SPACE_TYPE_WORD 0x2


#define TCSM0_CPUBASE	0xF4000000
#define TCSM1_CPUBASE	0xF4000000
#define TCSM0_PBASE	0x132B0000
#define TCSM0_VBASE_UC	0xB32B0000
#define TCSM0_VBASE_CC	0x932B0000
#define TCSM1_PBASE	0x132C0000
#define TCSM1_VBASE_UC	0xB32C0000
#define TCSM1_VBASE_CC	0x932C0000
#define TCSM_BANK0_OFST	0x0
#define TCSM_BANK1_OFST	0x1000
#define TCSM_BANK2_OFST	0x2000
#define TCSM_BANK3_OFST	0x3000

#define SPACE_HALF_MILLION_BYTE 0x80000

#define JZC_CACHE_LINE 32
#define	ALIGN(p,n)     ((uint8_t*)(p)+((((uint8_t*)(p)-(uint8_t*)(0)) & ((n)-1)) ? ((n)-(((uint8_t*)(p)-(uint8_t*)(0)) & ((n)-1))):0))
#define TCSM1_ALN4(x)  ((x & 0x3)? (x + 4 - (x & 0x3)) : x)


#define TCSM0_IWTA_CHAIN_LEN      (260)
#define TCSM0_IWTA_CABAC_CHAIN    TCSM0_CPUBASE
#define TCSM0_IWTA_CAVLC_CHAIN    TCSM0_CPUBASE


/*--------------------------------------------------
 * P1 to P0 interaction signals,
 * P0 only read, P1 should only write
 --------------------------------------------------*/
#define TCSM0_COMD_BUF_LEN 32
#define TCSM0_P1_TASK_DONE (TCSM0_IWTA_CABAC_CHAIN + TCSM0_IWTA_CHAIN_LEN) //must be the same with cavlc
#define TCSM0_P1_FIFO_RP (TCSM0_P1_TASK_DONE + 4) //start ADDRESS of using task


/*--------------------------------------------------
 * CABAC HW space
 --------------------------------------------------*/
#define TCSM0_DQ_TABLE_BASE (TCSM0_P1_TASK_DONE + TCSM0_COMD_BUF_LEN)
#define DQ_TABLE_LEN ((64 + 16*2) << SPACE_TYPE_WORD)


/*--------------------------------------------------
 * TCSM0_GP_END_FLAG; to indicate gp transmission finished; use to replace polling gp end
 --------------------------------------------------*/
#define TCSM0_GP0_END_FLAG      (TCSM0_DQ_TABLE_BASE+DQ_TABLE_LEN)
#define TCSM0_GP0_END_FLAG_LEN   ( 1 << SPACE_TYPE_WORD )
#define TCSM0_GP1_END_FLAG      (TCSM0_GP0_END_FLAG + TCSM0_GP0_END_FLAG_LEN)
#define TCSM0_GP1_END_FLAG_LEN   ( 1 << SPACE_TYPE_WORD )


/*--------------------------------------------------
 * HAT_PARA_ATM space
 --------------------------------------------------*/
#define REF_HAT_PARA_ATM (TCSM0_GP1_END_FLAG + TCSM0_GP1_END_FLAG_LEN)
#define REF_HAT_PARA_LEN (H264_MAX_ROW_MBNUM*4*2)//2 word per MB
#define MV_HAT_PARA_ATM (REF_HAT_PARA_ATM+REF_HAT_PARA_LEN)//not in tcsm0 yet
#define MV_HAT_PARA_LEN ((H264_MAX_ROW_MBNUM*4*2) << SPACE_TYPE_WORD)//8 words per MB

//cavlc NOT used the two below
#define DIRECT_HAT_PARA_ATM (MV_HAT_PARA_ATM + MV_HAT_PARA_LEN)
#define DIRECT_HAT_PARA_LEN (H264_MAX_ROW_MBNUM*2)//1 short per MB
#define MVD_HAT_PARA_ATM (DIRECT_HAT_PARA_ATM + DIRECT_HAT_PARA_LEN)//not in tcsm0 yet
#define MVD_HAT_PARA_LEN ((H264_MAX_ROW_MBNUM*4*2) << SPACE_TYPE_WORD)//8 words per MB


/*--------------------------------------------------
 * P1 TASK_FIFO
 --------------------------------------------------*/
//#define TCSM0_TASK_FIFO (TCSM0_BANK2)
#define TCSM0_TASK_FIFO (MVD_HAT_PARA_ATM + MVD_HAT_PARA_LEN)
#define MAX_TASK_LEN (sizeof(H264_MB_Ctrl_DecARGs)	\
		       +sizeof(H264_MB_InterB_DecARGs)	\
		      +(296<<2))
//296=(384(luma res) + 192(chroma res) + 16)/2

#define CV_MAX_TASK_LEN (sizeof(H264_MB_Ctrl_DecARGs)	   \
		         + sizeof(H264_MB_InterB_DecARGs)  \
		         + sizeof(H264_MB_Res))

#endif /*__H264_TCSM_H0__*/
