#include "jzm_jpeg_enc.h"


void JPEGE_SliceInit(_JPEGE_SliceInfo *s)
{
  unsigned int i;
  volatile unsigned int *chn = (volatile unsigned int *)s->des_va;

  GEN_VDMA_ACFG(chn,REG_SCH_GLBC, 0, SCH_GLBC_HIAXI	
#ifdef JZM_VPU_TLB_OPT				
	    | SCH_GLBC_TLBE		
#endif
#ifdef JZM_VPU_INT_OPT					
	    | SCH_INTE_ACFGERR					
	    | SCH_INTE_BSERR					
	    | SCH_INTE_ENDF   
	    | SCH_INTE_TLBERR
#endif
);	

  /* Open clock configuration */
  GEN_VDMA_ACFG(chn, REG_JPGC_GLBI, 0, OPEN_CLOCK);

  /**************************************************
   Huffman Encode Table configuration
  *************************************************/
  for(i=0; i<HUFFENC_LEN; i++)
    GEN_VDMA_ACFG(chn,REG_JPGC_HUFE+i*4, 0, huffenc[s->huffenc_sel][i]);
  
  /**************************************************
   Quantization Table configuration
  *************************************************/
  for(i=0; i<QMEM_LEN; i++)
    GEN_VDMA_ACFG(chn,REG_JPGC_QMEM+i*4, 0, qmem[s->ql_sel][i]);
  
  /**************************************************
   REGs configuration
  *************************************************/
  GEN_VDMA_ACFG(chn,REG_JPGC_STAT, 0,STAT_CLEAN);
  
  GEN_VDMA_ACFG(chn,REG_JPGC_BSA, 0, s->bsa);
  GEN_VDMA_ACFG(chn,REG_JPGC_P0A, 0, s->p0a);
  GEN_VDMA_ACFG(chn,REG_JPGC_P1A, 0, s->p1a);
  
  GEN_VDMA_ACFG(chn,REG_JPGC_NMCU, 0,s->nmcu);
  GEN_VDMA_ACFG(chn,REG_JPGC_NRSM, 0,s->nrsm);

  GEN_VDMA_ACFG(chn,REG_JPGC_P0C, 0,YUV420P0C);
  GEN_VDMA_ACFG(chn,REG_JPGC_P1C, 0,YUV420P1C);
  GEN_VDMA_ACFG(chn,REG_JPGC_P2C, 0,YUV420P2C);
  
  GEN_VDMA_ACFG(chn,REG_JPGC_GLBI, 0,( YUV420PVH                 |
				       JPGC_NCOL                 |
				       JPGC_SPEC     /* MODE */  |
				       JPGC_EN  ));

  GEN_VDMA_ACFG(chn,REG_JPGC_TRIG, VDMA_ACFG_TERM,JPGC_BS_TRIG | JPGC_PP_TRIG | JPGC_CORE_OPEN);
}
