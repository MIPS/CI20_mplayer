#define P1_USE_PADDR
#include <string.h>
#include "../../libjzcommon/jzasm.h"
#include "../../libjzcommon/jzmedia.h"
#include "../../libjzcommon/jz4760e_2ddma_hw.h"
#include "ffmpeg2_p1_type.h"
#include "ffmpeg2_dcore.h"
#include "ffmpeg2_tcsm0.h"
#include "ffmpeg2_tcsm1.h"
#include "jzdsputil.h"

#define MPEG2_STOP_P1()						\
  ({								\
    ((volatile int *)TCSM0_PADDR(TCSM0_P1_FIFO_RP))[0]=0;	\
    *((volatile int *)(TCSM0_PADDR(TCSM0_P1_TASK_DONE))) = 0x1;	\
    i_nop;							\
    i_nop;							\
    i_wait();							\
  })

static void inline gp1_move_back( MPEG2_FRAME_GloARGs * dFRM, MPEG2_MB_DecARGs * dMB );

volatile uint32_t * debug = (volatile uint32_t *)(TCSM1_BANK7 + 4);
volatile uint32_t * unknown_block = (volatile uint32_t *)UNKNOWN_BLOCK;
#include "idct.c"
#include "mc.c"

#define XCH2(a, b, t)     t=a;a=b;b=t
#define XCH3(a, b, c, t)  t=a;a=b;b=c;c=t
//#include "debug1.h"
#define __p1_text __attribute__ ((__section__ (".p1_text")))
#define __p1_main __attribute__ ((__section__ (".p1_main")))
#define __p1_data __attribute__ ((__section__ (".p1_data")))


__p1_main int main() {

    int i, count;
    /* prepare variables -- why must be first defined than others?*/
    MPEG2_FRAME_GloARGs * dFRM = (MPEG2_FRAME_GloARGs *)TCSM1_DFRM_BUF;
    MPEG2_MB_DecARGs * dMB  = (MPEG2_MB_DecARGs *)TASK_DMB_BUF;
    MPEG2_MB_DecARGs * dMB_L  = (MPEG2_MB_DecARGs *)TASK_DMB_BUF1;
    MPEG2_MB_DecARGs * dMB_N  = (MPEG2_MB_DecARGs *)TASK_DMB_BUF2;
    MPEG2_MB_DecARGs * tmp;

    //vmau_chn * vmau_chain_ptr = (vmau_chn *)VMAU_CHAIN;
    //vmau_chn * vmau_chain_ptr1 = (vmau_chn *)VMAU_CHAIN1;

    unsigned int * mc_chain_ptr = (unsigned int *)MOTION_DHA;
    //unsigned int * mc_chain_ptr1 = (unsigned int *)MOTION_DHA1;
    //unsigned int * mc_tmp;
    /* volatile must be add when we access a physic address */
    volatile int * mbnum_wp = (volatile int *)TCSM1_MBNUM_WP;
    volatile int * mbnum_rp = (volatile int *)TCSM1_MBNUM_RP;
    volatile unsigned int * tcsm1_fifo_rp = (volatile unsigned int *)TCSM1_FIFO_RP;
    *tcsm1_fifo_rp = TCSM0_TASK_FIFO;
    int mbnumber = ((dFRM->height + 15)/16) * ((dFRM->width + 15)/16);
    volatile int reach_tcsm1_end = 0;

    /* prepare for circle */
    *(volatile int *)MOTION_DSA = 0x8000FFFF;
    *(volatile int *)VMAU_DEC_END_FLAG = 0;;
    *(volatile int*)(DDMA_GP1_V_BASE+DDMA_GP_DCS_OFST) = 0x4;
    while(*mbnum_wp <= *mbnum_rp + 2){
      if( *unknown_block )
        goto end;
    }

    /* first mb ready */
    set_gp0_dha( TCSM1_PADDR(DDMA_GP0_SET));
    *((volatile int *)(DDMA_GP0_SET + TSA))  = TCSM0_PADDR(*tcsm1_fifo_rp);
    *((volatile int *)(DDMA_GP0_SET + TDA))  = TCSM1_PADDR(dMB);
    *((volatile int *)(DDMA_GP0_SET + STRD)) = GP_STRD(TASK_BUF_LEN, 0, TASK_BUF_LEN); //make it better
    *((volatile int *)(DDMA_GP0_SET + UNIT)) = GP_UNIT(1, TASK_BUF_LEN, sizeof(struct MPEG2_MB_DecARGs));
    set_gp0_dcs();
    poll_gp0_end();
    (*mbnum_rp)++; 
    (*tcsm1_fifo_rp) += (dMB->block_size + 3) & 0x0000FFFC;

    /* start secound mb */
    *((volatile int *)(DDMA_GP0_SET + TSA))  = TCSM0_PADDR(*tcsm1_fifo_rp);
    *((volatile int *)(DDMA_GP0_SET + TDA))  = TCSM1_PADDR(dMB_N);
    set_gp0_dcs();

    count = 0;
    for(i = 0; i < mbnumber; i++){
      /* mc */
      debug[0] = count;
      if( !dMB->mb_intra && dMB->mv_type == MV_TYPE_16X16 ){
        while(*(volatile int *)MOTION_DSA != 0x8000FFFF); 
        frame_mc_16x16(dFRM, dMB, mc_chain_ptr);
        
        SET_REG1_DSA (TCSM1_PADDR(MOTION_DSA)); 
        *(volatile int *)MOTION_DSA = 0;
        SET_REG1_DDC( TCSM1_PADDR(mc_chain_ptr) | 0x1 );  //start 
      }
      
      /* idct */
      debug[1] = count;
      if(count > 0){
        while( read_reg(VMAU_DEC_END_FLAG, 0) == -5);
        ffmpeg2_idct(dFRM, dMB_L);
      }

      /* gp1 move back */
      debug[2] = count;
      if(count > 1){
        poll_gp1_end();
        gp1_move_back(dFRM, dMB_N);
        dFRM->cp_data0 += 256;
        dFRM->cp_data1 += 128;
      }

      /* gp0 */
      debug[3] = count;
      if(count < mbnumber - 1){
        poll_gp0_end();
        (*tcsm1_fifo_rp) += (dMB->block_size + 3) & 0x0000FFFC;
        
        reach_tcsm1_end = (((*tcsm1_fifo_rp) & 0x0000FFFF) + TASK_BUF_LEN) > 0x4000;
        if(reach_tcsm1_end)
          *tcsm1_fifo_rp = TCSM0_TASK_FIFO;
        
        (*mbnum_rp)++;
      }
      
      if( *unknown_block )
        goto err;

      debug[4] = count;
      if(count < mbnumber - 2){
        while( *mbnum_wp <= *mbnum_rp);

        while( read_reg(VMAU_DEC_END_FLAG, 0) == -5);
        *((volatile int *)(DDMA_GP0_SET + TSA))  = TCSM0_PADDR(*tcsm1_fifo_rp);
        *((volatile int *)(DDMA_GP0_SET + TDA))  = TCSM1_PADDR(dMB_L);
        set_gp0_dcs();
      }

      debug[5] = count;

      //XCH2(mc_chain_ptr, mc_chain_ptr1, mc_tmp);
      XCH3(dMB_L, dMB, dMB_N, tmp);
      count++;
    }
    
    while(*(volatile int *)MOTION_DSA != 0x8000FFFF); 
    while( read_reg(VMAU_DEC_END_FLAG, 0) == -5);
    ffmpeg2_idct(dFRM, dMB_L);

    poll_gp1_end();
    gp1_move_back(dFRM, dMB_N);
    dFRM->cp_data0 += 256;
    dFRM->cp_data1 += 128;

    while( read_reg(VMAU_DEC_END_FLAG, 0) == -5);
    poll_gp1_end();
    gp1_move_back(dFRM, dMB_L);
    poll_gp1_end();
    
 err:
    if( *unknown_block ){
      *unknown_block = 0;
      while(*(volatile int *)MOTION_DSA != 0x8000FFFF); 
      poll_gp1_end();
      poll_gp0_end();
    }
 end:
    *((volatile int *)TCSM0_PADDR(TCSM0_P1_TASK_DONE)) = 0x1;
    i_nop;
    i_nop;
    i_wait();

    return 0;
}

static void inline gp1_move_back( MPEG2_FRAME_GloARGs * dFRM, MPEG2_MB_DecARGs * dMB )
{
    int i = ((uint32_t)dMB == TASK_DMB_BUF)? 0 : ((uint32_t)dMB == TASK_DMB_BUF1)? 1 : 2;
    if( dMB->interlaced_dct)
    {
        set_gp1_dha( TCSM1_PADDR(DDMA_GP1_SET) );
        // Y blocks: even lines (start with line 0)
        *((volatile uint32_t *)(DDMA_GP1_SET + 0x0))   = TCSM1_PADDR( IDCT_YOUT + i*64*6 );
	*((volatile uint32_t *)(DDMA_GP1_SET + 0x4))   = (uint32_t)dFRM->cp_data0;
	*((volatile uint32_t *)(DDMA_GP1_SET + 0x8))   = GP_STRD(16, 0, 32);
	*((volatile uint32_t *)(DDMA_GP1_SET + 0xc))   = GP_UNIT(0, 16, 128); 
        // Odd lines
        *((volatile uint32_t *)(DDMA_GP1_SET + 0x10))  = TCSM1_PADDR( IDCT_YOUT + i*64*6 + 128);
	*((volatile uint32_t *)(DDMA_GP1_SET + 0x14))  = (uint32_t)dFRM->cp_data0 + 16; 
	*((volatile uint32_t *)(DDMA_GP1_SET + 0x18))  = GP_STRD(16, 0, 32);
	*((volatile uint32_t *)(DDMA_GP1_SET + 0x1c))  = GP_UNIT(0, 16, 128);  
        // C blocks
        *((volatile uint32_t *)(DDMA_GP1_SET + 0x20))  = TCSM1_PADDR( IDCT_COUT + i*64*6 );
        *((volatile uint32_t *)(DDMA_GP1_SET + 0x24))  = (uint32_t)dFRM->cp_data0;
        *((volatile uint32_t *)(DDMA_GP1_SET + 0x28))  = GP_STRD(128, 0, 128);
        *((volatile uint32_t *)(DDMA_GP1_SET + 0x2c))  = GP_UNIT(1, 128, 128); 
    }
    else
    {
        set_gp1_dha( TCSM1_PADDR(DDMA_GP1_SET) );
        // Y blocks
        *((volatile uint32_t *)(DDMA_GP1_SET + 0x0))   = TCSM1_PADDR( IDCT_YOUT + i*64*6 );
 	*((volatile uint32_t *)(DDMA_GP1_SET + 0x4))   = (uint32_t)dFRM->cp_data0;
	*((volatile uint32_t *)(DDMA_GP1_SET + 0x8))   = GP_STRD(256, 0, 256);
	*((volatile uint32_t *)(DDMA_GP1_SET + 0xc))   = GP_UNIT(0, 256, 256); 
        // C blocks
        *((volatile uint32_t *)(DDMA_GP1_SET + 0x10))  = TCSM1_PADDR( IDCT_COUT + i*64*6 );
        *((volatile uint32_t *)(DDMA_GP1_SET + 0x14))  = (uint32_t)dFRM->cp_data1;
        *((volatile uint32_t *)(DDMA_GP1_SET + 0x18))  = GP_STRD(128, 0, 128);
        *((volatile uint32_t *)(DDMA_GP1_SET + 0x1c))  = GP_UNIT(1, 128, 128); 
    }
    set_gp1_dcs();
}
