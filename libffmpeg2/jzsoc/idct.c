/******************************************************************************************
   JZ HWAC for MPEG2 -- IDCT. 
   Idct funcs are defined here, as well as some helper functions.
   Nov, 2011.
*******************************************************************************************/
#include "ffmpeg2_p1_type.h"
#include "../../libjzcommon/jzmedia.h"
#include "ffmpeg2_tcsm1.h"
#include "vmau.h"
#include "jzdsputil.h"
#include "ffmpeg2_dcore.h"

/* this table is needed when  using s->qscale to retrieve the qscale_code. */
static int non_linear_qscale_inverse [] = {
         0,  1,  2,  3,  4,  5,  6,   7,  8,  //8
         0,  9,  0, 10,  0, 11,  0,  12,  0,  //17
        13,  0, 14, 0,  15,  0, 16,   0,  0,  //26
         0, 17,  0, 0,   0, 18,  0,   0,  0,  //35
        19,  0,  0, 0,  20,  0,  0,   0, 21,  //44
	 0,  0,  0, 22,  0,  0,  0,  23,  0,  //53
         0,  0, 24,  0,  0,  0,  0,   0,  0,  //62
         0, 25,  0,  0,  0,  0,  0,   0,  0,  //71
        26,  0,  0,  0,  0,  0,  0,   0, 27,  //80
         0,  0,  0,  0,  0,  0,  0,  28,  0,  //89
         0,  0,  0,  0,  0,  0, 29,   0,  0,  //98
         0,  0,  0,  0,  0, 30,  0,   0,  0,  //107
         0,  0,  0,  0, 31,  0,  0,   0,  0,  //116         
};
static inline int get_qscale_code(MPEG2_FRAME_GloARGs* dFRM, MPEG2_MB_DecARGs * dMB )
{
    if (dFRM->q_scale_type) {
        return non_linear_qscale_inverse[dMB->qscale];
    } else {
        return dMB->qscale >> 1;  //-- not <<1
    }
}

void inline ffmpeg2_idct( MPEG2_FRAME_GloARGs* dFRM, MPEG2_MB_DecARGs * dMB)
{
    int i = ((uint32_t)dMB == TASK_DMB_BUF)? 0 : ((uint32_t)dMB == TASK_DMB_BUF1) ? 1 : 2;
    int qscale_code = get_qscale_code(dFRM, dMB);
    vmau_chn* vmau_chn_ptr =  (vmau_chn*) VMAU_CHAIN;   

    if ( dMB->mb_intra ) {
      write_reg(VMAU_P_BASE+VMAU_SLV_NCCHN_ADDR, TCSM1_PADDR( VMAU_CHAIN ) );    
      write_reg(VMAU_P_BASE+VMAU_SLV_DEC_DONE, TCSM1_PADDR( VMAU_DEC_END_FLAG) );  
      write_reg(VMAU_P_BASE+VMAU_SLV_DEC_YADDR,TCSM1_PADDR( IDCT_YOUT + i*64*6));             
      write_reg(VMAU_P_BASE+VMAU_SLV_DEC_UADDR,TCSM1_PADDR( IDCT_COUT + i*64*6));        
      write_reg(VMAU_P_BASE+VMAU_SLV_DEC_VADDR,TCSM1_PADDR( IDCT_COUT + i*64*6 + 8));     
      write_reg(VMAU_DEC_END_FLAG, -5);

      vmau_chn_ptr->main_cbp   = 0x1111111;
      vmau_chn_ptr->quant_para = qscale_code | ((dFRM->q_scale_type == 1)<<23);   
      vmau_chn_ptr->main_addr  = TCSM1_PADDR((dMB->blocks[0]) );
      vmau_chn_ptr->ncchn_addr = TCSM1_PADDR(VMAU_CHAIN);
      vmau_chn_ptr->id_mlen    = (1<<16) | 768; // 768 == 6<<7, the len of a block is 64*2(0x80)

      write_reg(VMAU_P_BASE+VMAU_SLV_GBL_RUN, 1); //run vmau
    }
    else if ( (!dMB->interlaced_dct) && (dMB->mv_type == MV_TYPE_16X16) ){ 
      write_reg(VMAU_P_BASE+VMAU_SLV_NCCHN_ADDR, TCSM1_PADDR( VMAU_CHAIN ) );    
      write_reg(VMAU_P_BASE+VMAU_SLV_DEC_DONE, TCSM1_PADDR( VMAU_DEC_END_FLAG) );  
      write_reg(VMAU_P_BASE+VMAU_SLV_DEC_YADDR,TCSM1_PADDR( IDCT_YOUT + i*64*6));             
      write_reg(VMAU_P_BASE+VMAU_SLV_DEC_UADDR,TCSM1_PADDR( IDCT_COUT + i*64*6));        
      write_reg(VMAU_P_BASE+VMAU_SLV_DEC_VADDR,TCSM1_PADDR( IDCT_COUT + i*64*6 + 8));     
      write_reg(VMAU_DEC_END_FLAG, -5);

      vmau_chn_ptr->main_cbp   = dMB->cbp;
      vmau_chn_ptr->quant_para = qscale_code | ((dFRM->q_scale_type == 1)<<23);   
      vmau_chn_ptr->main_addr  = TCSM1_PADDR((dMB->blocks[0]) );
      vmau_chn_ptr->ncchn_addr = TCSM1_PADDR(VMAU_CHAIN);
      vmau_chn_ptr->id_mlen    = (dMB->len_count<<7);
      
      write_reg(VMAU_P_BASE+VMAU_SLV_GBL_RUN, 1); //run vmau
    }
    else
      {
         //unsupported mv type, so can not do idct.
         *(volatile int *)VMAU_DEC_END_FLAG = 0;;
      }
}

