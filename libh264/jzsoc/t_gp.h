#ifndef __TC_DDMA_GP_H__
#define __TC_DDMA_GP_H__

#ifdef P1_USE_PADDR
#define DDMA_GP0_VBASE 0x13210000
#define DDMA_GP1_VBASE 0x13220000
#define DDMA_GP2_VBASE 0x13230000
#else
#define DDMA_GP0_VBASE gp0_base
#define DDMA_GP1_VBASE gp1_base
#define DDMA_GP2_VBASE gp2_base
#endif

#define OFST_DHA 0x0
#define OFST_DCS 0x4
#define OFST_GAT 0x8

#define DDR_STRIDE_VBASE 0x13020034
//ddr configure
#define WRITE_DDR_STRIDE(STRIDE) \
(write_reg((DDR_STRIDE_VBASE), STRIDE))

//ddma_gp 0
#define POLLING_GP0_END() \
  ((read_reg((volatile int *)DDMA_GP0_VBASE, OFST_DCS)) & 0x4)

#define RUN_GP0() \
(write_reg((DDMA_GP0_VBASE + OFST_DCS), 0x1))

#define WRITE_GP0_DHA(DHA) \
(write_reg((DDMA_GP0_VBASE + OFST_DHA), DHA))

#define READ_GP0_DHA() \
( read_reg(DDMA_GP0_VBASE, OFST_DHA) )

#define READ_GP0_DCS() \
( read_reg(DDMA_GP0_VBASE, OFST_DCS) )

#define WRITE_GP0_GAT(GAT) \
(write_reg((DDMA_GP0_VBASE + OFST_GAT), GAT))

#define READ_GP0_GAT() \
(read_reg(DDMA_GP0_VBASE , OFST_GAT))
//ddma_gp 1
#define POLLING_GP1_END() \
((read_reg((volatile int *)DDMA_GP1_VBASE, OFST_DCS)) & 0x4)

#define RUN_GP1() \
(write_reg((DDMA_GP1_VBASE + OFST_DCS), 0x1))

#define WRITE_GP1_DHA(DHA) \
(write_reg((DDMA_GP1_VBASE + OFST_DHA), DHA))

#define READ_GP1_DHA() \
( read_reg(DDMA_GP1_VBASE, OFST_DHA) )

#define READ_GP1_DCS() \
( read_reg(DDMA_GP1_VBASE, OFST_DCS) )

#define WRITE_GP1_GAT(GAT) \
(write_reg((DDMA_GP1_VBASE + OFST_GAT), GAT))

#define READ_GP1_GAT() \
(read_reg(DDMA_GP1_VBASE , OFST_GAT))
//ddma_gp 2
#define POLLING_GP2_END() \
((read_reg((volatile int *)DDMA_GP2_VBASE, OFST_DCS)) & 0x4)

#define RUN_GP2() \
(write_reg((DDMA_GP2_VBASE + OFST_DCS), 0x1))

#define WRITE_GP2_DHA(DHA) \
(write_reg((DDMA_GP2_VBASE + OFST_DHA), DHA))

#define READ_GP2_DHA() \
( read_reg(DDMA_GP2_VBASE, OFST_DHA) )

#define READ_GP2_DCS() \
( read_reg(DDMA_GP2_VBASE, OFST_DCS) )

#define WRITE_GP2_GAT(GAT) \
(write_reg((DDMA_GP2_VBASE + OFST_GAT), GAT))

#define READ_GP2_GAT() \
(read_reg(DDMA_GP2_VBASE , OFST_GAT))


/*GP-DMA Descriptor-Node Structure*/
/*
     | bit31 | bit30......bit16 | bit15......bit0 |
     +--------------------------------------------+
   0 |<--------          TSA       -------------->|
     +--------------------------------------------+
   4 |<--------          TDA       -------------->|
     +--------------------------------------------+
   8 |<-   TYP    ->|<- TST   ->|<-FRM->|<- TDT ->|
     +--------------------------------------------+
  12 |<-TAG->|<-    TRN       ->|<-  NUM        ->|
     +--------------------------------------------+         
  
  Note:
    TSA: source address
    TDA: destination address
    TYP: transfer type: 00-word, 01-byte, 10-halfword, 11-illegal
    TST: source stride
    FRM: DDR optimize page size setting: 00-N/C, 01-1Kbyte, 10-2Kbyte, 11-4Kbyte
         only souce or destination is DDR can open FRM for optimizing.
    TDT: destination stride
    TAG: Current node Link or Not
    TRN: ROW width for DMA(unit: byte)
    NUM: Total for DMA(unit: byte)   
 */
#define GP_FRM_NML 0x0
#define GP_FRM_1K  0x1
#define GP_FRM_2K  0x2
#define GP_FRM_4K  0x3

#define GP_IRQ_EN 0x8
#define GP_END 0x4
#define GP_RST 0x2
#define GP_SUP 0x1

#define GP_LK  0x0
#define GP_UL  0x1

#define GP_STR(size, tst, frm, tdt) ((((size)&0x3)<<30) | (((tst)&0x3fff) << 16) | (((frm)&0x3)<<14) | (((tdt)&0x3fff)))

#define GP_GSC(link, w, num) ((((link)&0x1)<<31) | (((w)&0x7fff) << 16) | (((num)&0xffff)))

#endif
