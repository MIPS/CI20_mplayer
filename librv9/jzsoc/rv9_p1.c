#define P1_USE_PADDR

#include "../../libjzcommon/jzsys.h"
#include "../../libjzcommon/jzasm.h"
#include "../../libjzcommon/jzmedia.h"
#include "rv9_p1_type.h"
#include "rv9_dcore.h"
#include "rv9_tcsm.h"
#include "rv9_sram.h"
#include "t_dblk.h"
#include "../../libjzcommon/jz4760e_dcsc.h"
#include "t_vmau.h"

#if 1
#define RV9_STOP_P1()						\
  ({								\
    ((volatile int *)TCSM1_P1_FIFO_RP)[0]=0;	                \
    *((volatile int *)TCSM1_P1_TASK_DONE) = 0x1;                \
    i_nop;							\
    i_nop;							\
    i_wait();							\
  })
#endif

int mc_flag=0,last_mc_flag=0;
#include "rv9_p1_mc.c"

#define __p1_text __attribute__ ((__section__ (".p1_text")))
#define __p1_main __attribute__ ((__section__ (".p1_main")))
#define __p1_data __attribute__ ((__section__ (".p1_data")))

__p1_main int main() {
    S32I2M(xr16, 0x3);

    int count;
    RV9_MB_DecARGs * dMB, * dMB_L, * dMB_N, * dMB_NN;
    RV9_Slice_GlbARGs * dSlice;
    VMAU_CHN_REG * mau_reg_ptr = (VMAU_CHN_REG *) VMAU_CHN_BASE;

    dMB    = (RV9_MB_DecARGs *) TASK_BUF1;//curr mb
    dMB_L  = (RV9_MB_DecARGs *) TASK_BUF0;
    dMB_N  = (RV9_MB_DecARGs *) TASK_BUF2;
    dMB_NN = (RV9_MB_DecARGs *) TASK_BUF3;

    dSlice    = (RV9_Slice_GlbARGs *) TCSM1_SLICE_BUF;
    volatile int * debug = (volatile int *)DEBUG;
    debug[0] = debug [1] = debug [2] = debug [3] = debug [4] =
	debug [5] = debug [6] = debug [7] = debug [8] = -1;

    int xchg_tmp; 
    volatile int * mbnum_wp = (volatile int *) TCSM1_MBNUM_WP;
    int mbnum_rp;
    unsigned int addr_rp;
    uint8_t  mb_x = 0, mb_x_d1=0, mb_x_d2=0;
    uint8_t  mb_y = 0, mb_y_d1=0, mb_y_d2=0;
    volatile int * task_fifo_ready = (volatile int *)FIFO_ADDR_READY;
    //volatile unsigned int * p1_cur_dmb = (volatile unsigned int *)P1_CUR_DMB;

    volatile unsigned int * vdma_chain_ptr  = (volatile unsigned int *)VDMA_CHAIN;
    vdma_chain_ptr[3] = (TASK_BUF_LEN / 4) - 1; // tran size

    *(volatile int*)TCSM1_MOTION_DSA = (0x80000000 | 0xFFFF);
    uint8_t * motion_dha = (uint8_t *) TCSM1_MOTION_DHA;
    uint8_t * next_motion_dha = (uint8_t *) TCSM1_MOTION_DHA+MC_CHN_ONELEN;
    uint8_t * motion_dsa = (uint8_t *) TCSM1_MOTION_DSA;
    last_mc_flag = 0;
    mc_flag = 0;

    unsigned char * vmau_yout0 = (unsigned char *) VMAU_YOUT_ADDR;
    unsigned char * vmau_yout1 = (unsigned char *) VMAU_YOUT_ADDR1;

#ifdef JZC_DBLK_OPT
    rv4_dblk_mb_chn * dblk_chain_ptr  = DBLK_CHAIN;
    int dblk_slice_end = 0;
#endif

    mbnum_rp=0;
    while(*task_fifo_ready);
    debug[0] = -2;
    addr_rp=*((volatile unsigned int *)TASK_FIFO_ADDR);

    /* ------------------         start          --------------------- */
    while(*mbnum_wp <= mbnum_rp+1);//wait until the first two mb is ready
    debug[0] = -3;
    vdma_chain_ptr[0] = (addr_rp & 0xFFFFFFFC) | 0x1;
    vdma_chain_ptr[1] = TCSM1_PADDR(dMB) & 0xFFFFFFFC;
    write_reg(0x1321000C, 0x0);
    write_reg(0x13210008, ((TCSM1_PADDR(VDMA_CHAIN) & 0xFFFFFFF0) | 0x3) );//run

#ifdef JZC_VMAU_OPT
    unsigned int tmp = ((REAL & MAU_VIDEO_MSK) << MAU_VIDEO_SFT);
    write_reg(VMAU_V_BASE+VMAU_SLV_GBL_RUN, VMAU_RESET);
    write_reg(VMAU_V_BASE+VMAU_SLV_VIDEO_TYPE, tmp);

    write_reg(VMAU_V_BASE+VMAU_SLV_DEC_STR,(VMAU_COUT_STR<<16)|VMAU_YOUT_STR);
    write_reg(VMAU_V_BASE+VMAU_SLV_Y_GS, dSlice->mb_width*16);
    /* we must set DEC_END_ADDR registor, or it will write end flag to anywhere */
    write_reg(VMAU_V_BASE+VMAU_SLV_DEC_DONE, TCSM1_PADDR(VMAU_DEC_END));

#ifdef JZC_DBLK_OPT
    write_reg(VMAU_V_BASE+VMAU_SLV_GBL_CTR, VMAU_CTRL_FIFO_M | 0x1<<24);
#else  
    write_reg(VMAU_V_BASE+VMAU_SLV_GBL_CTR, VMAU_CTRL_FIFO_M);
#endif

    *((volatile uint32_t *)VMAU_END_FLAG) = 1;
#endif

    while( (read_reg(0x1321000C, 0) & 0x2) == 0);//vdma_end
    debug[0] = -4;
    mbnum_rp++;
    addr_rp += CIRCLE_BUF_LEN;

#ifdef JZC_VMAU_OPT
/*  set slice start MB POS, then VMAU will store MB info from that pos
 *  or it will store MB info from start pos of line store space(0x4780 in spec) and go wrong
 */
    write_reg(VMAU_V_BASE+VMAU_SLV_POS, ((dMB->mb_y<<16) | dMB->mb_x));
#endif

    vdma_chain_ptr[0] = (addr_rp & 0xFFFFFFFC) | 0x1;
    vdma_chain_ptr[1] = TCSM1_PADDR(dMB_N) & 0xFFFFFFFC;
    write_reg(0x1321000C, 0x0);
    write_reg(0x13210008, ((TCSM1_PADDR(VDMA_CHAIN) & 0xFFFFFFF0) | 0x3) );//run

    SET_REG1_DSA(TCSM1_PADDR((int)motion_dsa));
    if(dSlice->si_type){
	if(dMB->mbtype > RV34_MB_TYPE_INTRA16x16)
	    {
		rv40_decode_mv_aux(dSlice, dMB, motion_dha);
		motion_dsa[0] = 0x0;
	    }
    }

    if(mc_flag){
	SET_REG1_DDC(TCSM1_PADDR((int)motion_dha) | 0x1);
	last_mc_flag=mc_flag;
    }

    while( (read_reg(0x1321000C, 0) & 0x2) == 0);//vdma end
    debug[0] = -5;
    mbnum_rp++;
    addr_rp += CIRCLE_BUF_LEN;
    while(*mbnum_wp<=mbnum_rp);//wait until the third mb is ready
    debug[0] = -6;
    vdma_chain_ptr[0] = (addr_rp & 0xFFFFFFFC) | 0x1;
    vdma_chain_ptr[1] = TCSM1_PADDR(dMB_NN) & 0xFFFFFFFC;
    write_reg(0x1321000C, 0x0);
    write_reg(0x13210008, ((TCSM1_PADDR(VDMA_CHAIN) & 0xFFFFFFF0) | 0x3) );//run

    count = 0;
    debug[1] = dMB->new_slice;
    debug[2] = dMB->er_slice;
    while((dMB->new_slice >= 0) && (dMB->er_slice >= 0)) {
	debug[0]=count;
	mc_flag = 0;
	mb_x_d2=mb_x_d1;
	mb_x_d1=mb_x;
	mb_x= dMB->mb_x;
	mb_y_d2=mb_y_d1;
	mb_y_d1=mb_y;
	mb_y= dMB->mb_y;

	if(dSlice->si_type){
	    if(dMB_N->mbtype > RV34_MB_TYPE_INTRA16x16)
		{
		    rv40_decode_mv_aux(dSlice, dMB_N, next_motion_dha);
		}
	}

#ifdef DEBUG_MC_CHAIN
	// we can mv dates we need to some free space define on TCSM1( suit for all)
	if( dMB->mb_y == DEBUG_MB_Y && dMB->mb_x == DEBUG_MB_X ) { // get MC DHA
	    int i;
	    for(i = 0; i < MC_CHN_ONELEN; i++)
		*(volatile unsigned int *)(VDMA_CHECK + i) = *(volatile unsigned int *)(motion_dha + i); 
	}
#endif

	if(last_mc_flag){
	    while( *(volatile int *)motion_dsa != (0x80000000 | 0xFFFF) );
	    last_mc_flag = 0;
	}

	if(mc_flag){
	    motion_dsa[0] = 0x0;
	    SET_REG1_DDC(TCSM1_PADDR(next_motion_dha) | 0x1);
	    last_mc_flag = mc_flag;
	}

	debug[1]=count;
	
#ifdef JZC_VMAU_OPT
	{
	    while(*(volatile uint32_t *)VMAU_END_FLAG == 0);
	    mau_reg_ptr->main_cbp = (dMB->mau_mcbp<<24)|dMB->cbp;
	    mau_reg_ptr->quant_para=(dMB->qscale & QUANT_QP_MSK) << QUANT_QP_SFT;
	    mau_reg_ptr->main_addr=TCSM1_PADDR(dMB->block16);
	    mau_reg_ptr->ncchn_addr=TCSM1_PADDR(mau_reg_ptr);
	    mau_reg_ptr->main_len = dMB->mau_mlen<<1;
	    mau_reg_ptr->id=1;
	    mau_reg_ptr->dec_incr=0;
	    mau_reg_ptr->aux_cbp=0;
	    mau_reg_ptr->y_pred_mode[0]=dMB->mau_ypm[0];
	    mau_reg_ptr->y_pred_mode[1]=dMB->mau_ypm[1];
	    mau_reg_ptr->c_pred_mode[0]=dMB->mau_cpm;
	    mau_reg_ptr->top_lr_avalid = dMB->mau_na;

	    write_reg(VMAU_V_BASE+VMAU_SLV_DEC_YADDR,TCSM1_PADDR(vmau_yout0));
	    write_reg(VMAU_V_BASE+VMAU_SLV_DEC_UADDR,TCSM1_PADDR(vmau_yout0+16*16));
	    write_reg(VMAU_V_BASE+VMAU_SLV_DEC_VADDR,TCSM1_PADDR(vmau_yout0+16*16 + 8));
	    write_reg(VMAU_V_BASE+VMAU_SLV_NCCHN_ADDR,TCSM1_PADDR(mau_reg_ptr));
	    write_reg(VMAU_V_BASE+VMAU_SLV_DEC_DONE, TCSM1_PADDR(VMAU_END_FLAG));
	    *(volatile uint32_t *)VMAU_END_FLAG = 0;
	    write_reg(VMAU_V_BASE+VMAU_SLV_GBL_RUN, 1); //run
	}
	debug[2]=count;
#endif

#ifdef JZC_DBLK_OPT
	if(count){
	    /* ------------------  config dblk  --------------------- */
	    dblk_slice_end = (dMB->new_slice != 0);
	    dblk_chain_ptr->id_dha = 
		(dblk_slice_end? HWDBLK_SLICE_END: 0) | (TCSM1_PADDR(dblk_chain_ptr) & MB_DHA_MSK);
	    dblk_chain_ptr->coef_nei_info = 
		(((dMB_L->up_cbp>>12) & RV4_TOP_YCBP_MSK) \
		 | (((dMB_L->up_cbp>>16) >> 2) & RV4_TOP_UCBP_MSK) << RV4_TOP_UCBP_SFT \
		 | ((((dMB_L->up_cbp>>16) >> 6) & RV4_TOP_VCBP_MSK) << RV4_TOP_VCBP_SFT) \
		 | (((dMB_L->up_coef>>12) & RV4_TOP_COEF_MSK ) << RV4_TOP_COEF_SFT) \
		 | ((IS_INTRA(dMB_L->mbtype_above) || IS_SEPARATE_DC(dMB_L->mbtype_above))? RV4_TOP_STRONG: 0) \
		 | ((dMB_L->mb_x != 0) ? RV4_LEFTEN: 0) \
		 | ((dMB_L->mb_y != 0) ? RV4_TOPEN: 0) \
		 | ((dMB_L->cur_coef & 0xffff) << 16));
	    dblk_chain_ptr->mb_info = 
		((dMB_L->cur_cbp & 0xffff) \
		 | ((dMB_L->cur_cbp>>16 & 0xff )<<16) \
		 | ((dMB_L->qscale & RV4_QP_MSK )<< RV4_QP_SFT) \
		 | ((IS_INTRA(dMB_L->mbtype_cur) || IS_SEPARATE_DC(dMB_L->mbtype_cur))? RV4_STRONG: 0));

#ifdef DEBUG_DBLK_CHAIN
	    if( dMB_L->mb_y == DEBUG_MB_Y /* && dMB_L->mb_x == DEBUG_MB_X */ ) {
		volatile unsigned int * store_chain = (volatile unsigned int *) VDMA_CHECK;
		store_chain += 3 * dMB_L->mb_x;
		*store_chain++ = dblk_chain_ptr->id_dha;
		*store_chain++ = dblk_chain_ptr->coef_nei_info;
		*store_chain++ = dblk_chain_ptr->mb_info;
	    }
#endif
	    write_dblk_reg(DEBLK_REG_TRIG, DBLK_RUN); //run
	    while(*((volatile int*)DBLK_ENDA) == -1); //waiting read chain task end
	    *((volatile int*)DBLK_ENDA) = -1;
	}
    
	debug[3] = count;
	if(count > 1){
	    /* waiting dblk end, using sync the location of last mb 
	     * FIXME : we can waiting for current mb end only while we start next mb dblk
	     */
	    int pre_mb_xy = mb_x_d2 | (mb_y_d2 << 16);
	    do{
		debug[6] = pre_mb_xy;
		debug[7] = *((volatile int *)DBLK_DMA_ENDA);
	    }while( *((volatile int *)DBLK_DMA_ENDA) != pre_mb_xy );
	}
#endif

	while( (read_reg(0x1321000C, 0) & 0x2) == 0x0 );
	mbnum_rp++;
	addr_rp += CIRCLE_BUF_LEN;

	debug[4] = count;

	while(*mbnum_wp<=mbnum_rp);//wait until next mb is ready
	vdma_chain_ptr[0] = (addr_rp & 0xFFFFFFFC) | 0x1;
	vdma_chain_ptr[1] = TCSM1_PADDR(dMB_L) & 0xFFFFFFFC;
	write_reg(0x1321000C, 0x0);
	write_reg(0x13210008, ( (TCSM1_PADDR(VDMA_CHAIN) & 0xFFFFFFF0) | 0x3) );//run

	debug[5] = count;

	count++;
	XCHG2(motion_dha,next_motion_dha,xchg_tmp);
	XCHG2(vmau_yout0,vmau_yout1,xchg_tmp);
	XCHG4(dMB_L,dMB,dMB_N,dMB_NN,xchg_tmp);
    }

    while( (read_reg(0x1321000C, 0) & 0x2) == 0x0);

#ifdef JZC_DBLK_OPT
    /* -----------  waiting copy the last 4 lines end  ----------- */
    // FIXME : maybe when we waiting for last mb end, we start a new dblk
    //         so the last 4 line has been copied end
    debug[8] = 34345656;
    while((read_reg(0x13270070, 0) & 0x1) == 0x0 );  
#endif

    *((volatile int *)TCSM1_P1_TASK_DONE) = 0x1;
    i_nop;
    i_nop;
    i_nop;
    i_nop;
    i_wait();

}
