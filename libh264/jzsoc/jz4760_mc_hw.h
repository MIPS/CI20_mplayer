/*
 * Copyright (C) 2006-2014 Ingenic Semiconductor CO.,LTD.
 *
 * This file is part of MPlayer.
 *
 * MPlayer is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * MPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with MPlayer; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*****************************************************************************
 *
 * JZ4760 MC HardWare Core Accelerate
 *
 * $Id: jz4760_mc_hw.h,v 1.1.1.1 2012/03/27 04:02:56 dqliu Exp $
 *
 ****************************************************************************/

#ifndef __JZ4760_MC_HW_H__
#define __JZ4760_MC_HW_H__

#ifdef _RTL_SIM_
# include "instructions.h"
#else
# include "../../libjzcommon/jzasm.h"
#endif //_RTL_SIM_


/*  ------------ MC I/O Space define ----------- */
/*******************************************************
 MC Channel-1 (Main Channel)
   supports most kinds of from 2-tap to 8-tap filters
   It can be used for either Luma or Chroma interpolation
********************************************************/
// MC_REG_BASE
#ifdef P1_USE_PADDR
#define MC_V_BASE 0x13250000
#else
#define MC_V_BASE 0xB3250000
#endif

#define REG_CTRL  0x0
#define REG_STAT  0x4
#define REG_REFA  0x8
#define REG_TAP1L 0xC
#define REG_TAP2L 0x10
#define REG_CURA  0x14
#define REG_IINFO 0x18
#define REG_STRD  0x1C
#define REG_TAP1M 0x20
#define REG_TAP2M 0x24
#define REG_FDDG  0x28
#define REG_FDDC  0x2C

#ifdef MC_PMON
#define REG_DMAR  0x30
#define REG_INTPR 0x34
#define REG_PENDR 0x38
#define REG_FDDR  0x3C
#endif /*MC_PMON*/

#define RNDT_POS0  0x40
#define RNDT_POS1  0x44
#define RNDT_POS2  0x48
#define RNDT_POS3  0x4C
#define RNDT_POS4  0x50
#define RNDT_POS5  0x54
#define RNDT_POS6  0x58
#define RNDT_POS7  0x5C
#define RNDT_POS8  0x60
#define RNDT_POS9  0x64
#define RNDT_POS10 0x68
#define RNDT_POS11 0x6C
#define RNDT_POS12 0x70
#define RNDT_POS13 0x74
#define RNDT_POS14 0x78
#define RNDT_POS15 0x7C

/*
    REG_CTRL: 0x0
    +---------+------+-------+-----+------+------+------+-----+---+---+-----+
    |  N/C    | RETE |FDD_PGN|DIPE |CKG_EN|FDD_EN| INTE |DIN2D|FAE|RST|MC_EN|
    +---------+------+-------+-----+------+------+------+-----+---+---+-----+
    |31     17|  16  |15    8|  7  |   6  |  5   |  4   |  3  | 2 | 1 |  0  | 
    
     --MC_EN: 	module enable
     --RST:   	module sw reset
     --FAE:   	fix-address enable, MC wont auto-calculate the tap-offseted addr
     --DIN2D: 	DMA-input 2D mode
     --INTE:	MC-interrupt enable
     --FDD_EN:	FDD enable
     --CKG_EN:  host module clock gate enable
     --FDD_PGN: FDD pending-gap number 
*/
#define SET_CTRL(rete,pgn,dipe,fdd_en,inte,din2d,fae)	 \
({ write_reg( (MC_V_BASE + REG_CTRL),                    \
              ((rete)   & 0x1 )<<16 |                    \
              ((pgn)    & 0xFF)<<8  |                    \
              ((dipe)   & 0x1 )<<7  |                    \
              ( 0x1           )<<6  |                    \
              ((fdd_en) & 0x1 )<<5  |		         \
              ((inte)   & 0x1 )<<4  |                    \
              ((din2d)  & 0x1 )<<3  |                    \
              ((fae)    & 0x1 )<<2  |                    \
              ( 0x1           )<<0  );	                 \
})

/*
    REG_STAT: 0x4
    +----------------+------+-------+
    |  N/C           |LK_END|BLK_END|
    +----------------+------+-------+
    |31             2|   1  |   0   | 

     --BLK_END:	each block task done, host module will set this flag
                furthermore, when CTRL.CKG_EN is set, host clock would be gated
     --LK_END:	whole FDD chain finished would set this flag 	    
*/
#define SET_FLAG()					\
({ write_reg( (MC_V_BASE + REG_STAT), 0x1);		\
})
#define MC_C1_POLLING()                                 \
({ do{}while(read_reg( MC_V_BASE, REG_STAT ) != 0x3);	\
})

/*
    REG_FDDC: 0x2C
    +-------+-----+-------+
    |  DHA  | N/C |  RUN  |
    +-------+-----+-------+
    |31    2|  1  |   0   | 

     --RUN: 	write 1 FDD will startup(if both CTRL.FDD_EN & CTRL.MC_EN)
                its write-only from BUS, and it can't be configured by FDD
     --DHA:     FDD chain hread address(word aligned)
                un-like with RUN, its R/W from BUS, and can be configured by FDD
*/
#define SET_FDDC(data)                                   \
({ write_reg( (MC_V_BASE + REG_FDDC), data);             \
})

/*
    REG_IINFO: 0x18
    +--------+--------+------+-----+-----+-----+-----+-----+---+---+
    | RND_2S | RND_1S | IRND | N/C | EC  | YC  | FMT | POS | W | H | 
    +--------+--------+------+-----+-----+-----+-----+-----+---+---+
    |31    24|23    16|  15  |  14 | 13  | 12  |11  8|7   4|3 2|1 0| 

     --RND_2S:	2nd stage rounding data (0~255)
     --RND_1S:	1st stage rounding data (0~255)
     --IRND:    interpolate AVG rounding
     --EC:      EPEL coeff select: 
                1-choose REG_FDDG.YPOS/XPOS; 0-choose TAP1L/TAP2L
     --YC:      RAW format: 0-Y; 1-Chroma
     --FMT:     tap-filter format
                0000 - MPEG_HPEL
                0001 - MPEG_QPEL
                0010 - H264_QPEL
                0011 - H264_EPEL
                0100 - VP6_EPEL
                0101 - WMV2_QPEL
                0110 - VC1_QPEL
                0111 - RV8_TPEL
                1000 - RV8_CHROM
                1001 - RV9_QPEL
                1010 - RV9_CHROM
                1011~1111 - N/C
     --POS:     sub-pixel position (except physical meaning of 1/8-pixel)
     --W:       block width
     --H:       block height
*/
#define SET_IINFO(rnd2s,rnd1s,irnd,ec,yc,fmt,pos,w,h)    \
({ write_reg( (MC_V_BASE + REG_IINFO),                   \
              ((rnd2s) & 0xFF)<<24 |                     \
              ((rnd1s) & 0xFF)<<16 |                     \
              ((irnd)  & 0x1 )<<15 |		         \
              ((ec)    & 0x1 )<<13 |                     \
              ((yc)    & 0x1 )<<12 |                     \
              ((fmt)   & 0xF )<<8  |                     \
              ((pos)   & 0xF )<<4  |                     \
              ((w)     & 0x3 )<<2  |                     \
              ((h)     & 0x3 )<<0  );                    \
})

/*
    REG_TAP1L: 0xC
    REG_TAP2L: 0x10
    REG_TAP1M: 0x20
    REG_TAP2M: 0x24
    +--------+--------+-------+-------+
    | COEF3  | COEF2  | COEF1 | COEF0 | 
    +--------+--------+-------+-------+
    |31    24|23    16|15    8|7     0| 
*/
#define SET_TAP1L(coef3,coef2,coef1,coef0)              \
({ write_reg( (MC_V_BASE + REG_TAP1L),                  \
              ((coef3) & 0xFF)<<24 |                    \
              ((coef2) & 0xFF)<<16 |                    \
              ((coef1) & 0xFF)<<8  |                    \
              ((coef0) & 0xFF)<<0  );		        \
})
#define SET_TAP2L(coef3,coef2,coef1,coef0)              \
({ write_reg( (MC_V_BASE + REG_TAP2L),                  \
              ((coef3) & 0xFF)<<24 |                    \
              ((coef2) & 0xFF)<<16 |                    \
              ((coef1) & 0xFF)<<8  |                    \
              ((coef0) & 0xFF)<<0  );		        \
})
#define SET_TAP1M(coef7,coef6,coef5,coef4)              \
({ write_reg( (MC_V_BASE + REG_TAP1M),                  \
              ((coef7) & 0xFF)<<24 |                    \
              ((coef6) & 0xFF)<<16 |                    \
              ((coef5) & 0xFF)<<8  |                    \
              ((coef4) & 0xFF)<<0  );		        \
})
#define SET_TAP2M(coef7,coef6,coef5,coef4)              \
({ write_reg( (MC_V_BASE + REG_TAP2M),                  \
              ((coef7) & 0xFF)<<24 |                    \
              ((coef6) & 0xFF)<<16 |                    \
              ((coef5) & 0xFF)<<8  |                    \
              ((coef4) & 0xFF)<<0  );		        \
})
#define TAPALN(coef1,coef2,coef3,coef4)                 \
( ((coef1) & COEF1_MSK)<<COEF1_SFT                      \
| ((coef2) & COEF2_MSK)<<COEF2_SFT                      \
| ((coef3) & COEF3_MSK)<<COEF3_SFT                      \
| ((coef4) & COEF4_MSK)<<COEF4_SFT)

/*
    REG_STRD: 0x1C
    +-------+------+-----+-------+
    |CCSTRD |YCSTRD| N/C | RSTRD |
    +-------+------+-----+-------+
    |31   24|23  16|15 12|11    0| 

     --RSTRD:  	reference stride
     --YCSTRD:  Y current stride
     --CCSTRD:  Chroma current stride
*/
#define SET_STRD(ccstrd,ycstrd,rstrd)                    \
({ write_reg( (MC_V_BASE + REG_STRD),                    \
              ((ccstrd) & 0xFF )<<24 |                   \
              ((ycstrd) & 0xFF )<<16 |                   \
              ((rstrd)  & 0xFFF)<<0  );			 \
})

/*
    RAM_RNDT(rounding table): 0x40~0x7C
    +--------+--------+--------+--------+
    | CRND2S | CRND1S | YRND2S | YRND1S | 
    +--------+--------+--------+--------+
    |31    24|23    16|15     8|7      0| 
    
     --YRND1S: 	Y 1st stage rounding data
     --YRND2S: 	Y 2nd stage rounding data
     --CRND1S: 	Chroma 1st stage rounding data
     --CRND2S: 	Chroma 2nd stage rounding data
     Note: RAM_RNDT is forbidden for FDD data mode's self-configuration
*/
#define SET_RNDT(RNDT_POS,c2s,c1s,y2s,y1s)               \
({ write_reg( (MC_V_BASE + (RNDT_POS)),                  \
              ((c2s) & 0xFF)<<24 |		         \
              ((c1s) & 0xFF)<<16 |		         \
              ((y2s) & 0xFF)<<8  |		         \
              ((y1s) & 0xFF)<<0  );		         \
})

/*
    REG_FDDG: 0x28
    +-----+-------+-----+-------+-----+-----+-----+-------+
    | N/C | CUNIT | N/C | YUNIT | N/C |YPOS | N/C | XPOS  |
    +-----+-------+-----+-------+-----+-----+-----+-------+
    |31 29|28   24|23 21|20   16|15 12|11  8|7   4|3     0|

     --CUNIT:   Auto mode, chroma min-blk's reference rectangle size
     --YUNIT:   Auto mode, luma min-blk's reference rectangle size
     --YPOS:  	1/8-pixel subpixel Y-direction positin
     --XPOS:  	1/8-pixel subpixel X-direction positin
*/
#define SET_FDDG(cunit,yunit,ypos,xpos)			\
({ write_reg( (MC_V_BASE + (REG_FDDG)), 		\
              ((cunit) & 0x1F)<<24 |			\
              ((yunit) & 0x1F)<<16 |			\
              ((ypos)  & 0xF )<<8  |			\
              ((xpos)  & 0xF )<<0  );			\
})

#ifdef MC_PMON  /*Only Used in RTL SIM/FPGA Test*/
/*
    REG_DMAR: 	Stat. DMA input/output running cycles (HCLK) 
    REG_INTPR:	Stat. MC kernel Tap-Filter interpolation cycles (HCLK)
    REG_PENDR:  Stat. FDD pending cycles (HCLK)
    REG_FDDR:   Stat. FDD normal working(not-include pending) cycles (HCLK)
*/
#define CLR_DMAR()                                       \
({ write_reg( (MC_V_BASE + REG_DMAR), 0);	         \
})
#define CLR_INTPR()                                      \
({ write_reg( (MC_V_BASE + REG_INTPR), 0);	         \
})
#define CLR_PENDR()                                      \
({ write_reg( (MC_V_BASE + REG_PENDR), 0);	         \
})
#define CLR_FDDR()                                       \
({ write_reg( (MC_V_BASE + REG_FDDR), 0);	         \
})

#define GET_DMAR()                                       \
({ read_reg(  MC_V_BASE , REG_DMAR  );	         \
})
#define GET_INTPR()                                      \
({ read_reg(  MC_V_BASE , REG_INTPR  );	         \
})
#define GET_PENDR()                                      \
({ read_reg(  MC_V_BASE , REG_PENDR  );	         \
})
#define GET_FDDR()                                       \
({ read_reg(  MC_V_BASE , REG_FDDR  );	         \
})
#endif /*MC_PMON*/

/////////////////////////////////////////////////////////

/*********************** MC Channel-1 FDD *************************************
    MC Fast Descriptor Node structure
    +---+-----+-----+---------------------------------------------------------+
    |PD | MD  | LK  |                      XXX                                |
    +---+-----+-----+---------------------------------------------------------+
    | 31|30 28| 27  |26                                                      0|

    PD:   pending, if 0, MC fast-descriptor-dma will pending until it turn to 1 
    MD:   mode:
             000-capture: MC FDD capture relative infomation and then start host
             001-data:    similar with MC-DDMA data mode
             010-run:     only inform host to run, it configs nothing
             011-sync:    only write sync information into {DHA-4}
             100-skip:    do nothing, but next node address is (current + offset)
             101-jump:    do nothing, but next node address is (TGT)
             110-snap:    same with capture mode except it would not start-up host
             111-auto:    same with capture mode except it would auto cal address
    LK:   node direction, 00-linking; 01-ending;
    XXX:  different usage in different mode


    1)capture mode/snap mode/auto mode
    +---+-----+-----+--+-----+-----+--+-----+-----+-----+----+----+-----+-----+
    |PD | MD  | LK  |PR| RAW | FMT |PF| RS  |YPOS | BW  | BH | BN | SBN | POS |
    +---+-----+-----+--+-----+-----+--+-----+-----+-----+----+----+-----+-----+
    | 31|30 28| 27  |26|25 24|23 20|19|18 16|15 12|11 10|9  8|7  6|5   4|3   0|
    
    PR:   progress record, when 1 FDD will write curr blk num to {DHA - 8}
    RAW:  00-Y; 01-U; 10-V; 11-illegal (only used in progress record case)
    FMT:  filter format, same as BINFO.FMT
    PF:   reference direction, 0-previous, 1-future    
    RS:   reference stride,(0~7, its unit is 4word) 
    YPOS: Y-direction sub-pixel position (only used in h264 1/8-pixel case)
    BW:   block width, same as BINFO.BW in Y
    BH:   block height, same as BINFO.BH in Y
    BN:   block number, 0~4
    SBN:  sub-block number, 0~4
    POS:  combo-use, in h264 1/8-pixel as X-dir sub-pxl, others as 2-D sub-pixel
    
    
    2)data mode
    +---+-----+-----+--+--+---+-----+--+-----+-----+-----+----+----+-----+-----+
    |PD | MD  | LK  |PR|AT|D1D| CSA |PF| RS  | DTC | BW  | BH | BN | SBN | POS |
    +---+-----+-----+--+--+---+-----+--+-----+-----+-----+----+----+-----+-----+
    | 31|30 28| 27  |26|25|24 |23 20|19|18 16|15 12|11 10|9  8|7  6|5   4|3   0|

    AT:   DATA AUTO flag, similar with auto mode when it was set
    D1D:  DMA 1-line, when set, reference stride is Not-Cared
    CSA:  Transfer start index, MC FDD config slave-reg from this index register
    DTC:  Transfer number, MC FDD from CSA and down config DTC number registers
    [Notes]
      1. DATA AUTO mode is only suit for Luma, otherwise error would occur
      2. in DATA AUTO mode, host would be auto-startup when DTC count-down to 0
      3. in DATA AUTO mode, only CURRA would be auto-CALed, REFA needs be CFGed
    
    
    3)run mode
    +---+-----+-----+---------------------------------------------------------+
    |PD | MD  | LK  |                      N/C                                |
    +---+-----+-----+---------------------------------------------------------+
    | 31|30 28| 27  |26                                                      0|

    
    4)sync mode
    +---+-----+-----+--------------------------------------------+------------+
    |PD | MD  | LK  |                      N/C                   |   SFD      |
    +---+-----+-----+--------------------------------------------+------------+
    | 31|30 28| 27  |26                                        16|15         0|

    SFD:  sync flag data, {16'b0,SFD} will be written into {DHA-4}
          in video decoing, SFD can be used as MB ID number or just a sync flag
    
    
    5)skip mode
    +---+-----+-----+----------------------------------------------------+----+
    |PD | MD  | LK  |                      N/C                           |OFST|
    +---+-----+-----+----------------------------------------------------+----+
    | 31|30 28| 27  |26                                                  |7  0|
    
    OFST:  next node address is (curr + {OFST,2'b0}), its unit is word

    
    6)jump mode
    +---+-----+-----+-----+---------------------------------------------------+
    |PD | MD  | LK  | N/C |                     TGT                           |
    +---+-----+-----+-----+---------------------------------------------------+
    | 31|30 28| 27  |26 24|23                                                0|
    
    TGT:  next node address is {curr[31:24],TGT[23:2],2'b0})
***********************************************************/
/*---------- Filter Format -------*/
#define	MPEG_HPEL           0
#define	MPEG_QPEL           1
#define	H264_QPEL           2
#define	H264_EPEL           3
#define	VP6_EPEL            4
#define	WMV2_QPEL           5
#define	VC1_QPEL            6
#define	RV8_TPEL            7
#define	RV8_CHROM           8
#define	RV9_QPEL            9
#define	RV9_CHROM           10

/*--- FDD mode ---*/
#define FDD_MD_CAP       0x8<<28
#define FDD_MD_DATA      0x9<<28
#define FDD_MD_RUN       0xA<<28
#define FDD_MD_SYNC      0xB<<28
#define FDD_MD_SKIP      0xC<<28
#define FDD_MD_JUMP      0xD<<28
#define FDD_MD_SNAP      0xE<<28
#define FDD_MD_AUTO      0xF<<28

#define FDD_LK           0x1<<27
#define FDD_UL           0x0<<27

#define FDD_PR           0x1<<26

#define FDD_AT           0x1<<25
#define FDD_D1D          0x1<<24
#define FDD_D2D          0x0<<24

#define FDD_RAW_Y        0x0<<24
#define FDD_RAW_U        0x1<<24
#define FDD_RAW_V        0x2<<24

#define FDD_MPEG_HPEL    MPEG_HPEL<<20
#define FDD_MPEG_QPEL    MPEG_QPEL<<20
#define FDD_H264_QPEL    H264_QPEL<<20
#define FDD_H264_EPEL    H264_EPEL<<20
#define FDD_VP6_EPEL     VP6_EPEL<<20
#define FDD_WMV2_QPEL    WMV2_QPEL<<20
#define FDD_VC1_QPEL     VC1_QPEL<<20
#define FDD_RV8_TPEL     RV8_TPEL<<20
#define FDD_RV8_CHROM    RV8_CHROM<<20
#define FDD_RV9_QPEL     RV9_QPEL<<20
#define FDD_RV9_CHROM    RV9_CHROM<<20

#define FDD_DIR_P        0x0<<19
#define FDD_DIR_F        0x1<<19

#define FDD_DATA_CMD(csa,dtc)                     \
( FDD_MD_DATA |                                   \
  FDD_LK |                                        \
  (csa)<<20 |                                     \
  (dtc)<<12                                       \
)

/*----------- INTP Pos -----------*/
#define H0V0             0
#define H1V0             1
#define H2V0             2
#define H3V0             3
#define H0V1             4
#define H1V1             5
#define H2V1             6
#define H3V1             7
#define H0V2             8
#define H1V2             9
#define H2V2             10
#define H3V2             11
#define H0V3             12
#define H1V3             13
#define H2V3             14
#define H3V3             15

/*----------- BLK Size -----------*/
#define BLK_16X16        0
#define BLK_16X8         1
#define BLK_8X16         2
#define BLK_8X8          3

#define SBLK_8X8         0
#define SBLK_8X4         1
#define SBLK_4X8         2
#define SBLK_4X4         3


/************************************************************
 MC Channel-2 (Simple Channel)
   supports only 2-tap filters
   It can only be used for all kinds of Chroma interpolation
*************************************************************/
#ifdef P1_USE_PADDR
#define MC2_V_BASE  0x13258000
#else
#define MC2_V_BASE  0xB3258000
#endif
#define MC2_P_BASE  0x13258000

#define REG2_CTRL   0x0
#define REG2_STAT   0x4
#define REG2_REFA   0x8
#define REG2_CURA   0xC
#define REG2_IINFO  0x10
#define REG2_TAP    0x14
#define REG2_STRD   0x18
#define REG2_FDDC   0x1C

#define RNDT2_POS0  0x40
#define RNDT2_POS1  0x44
#define RNDT2_POS2  0x48
#define RNDT2_POS3  0x4C
#define RNDT2_POS4  0x50
#define RNDT2_POS5  0x54
#define RNDT2_POS6  0x58
#define RNDT2_POS7  0x5C
#define RNDT2_POS8  0x60
#define RNDT2_POS9  0x64
#define RNDT2_POS10 0x68
#define RNDT2_POS11 0x6C
#define RNDT2_POS12 0x70
#define RNDT2_POS13 0x74
#define RNDT2_POS14 0x78
#define RNDT2_POS15 0x7C

/*
    REG2_CTRL: 0x0
    +-----------+-----+-------+-----+------+------+------+-----+---+---+-----+
    |  N/C      |RETE |FDD_PGN| N/C |CKG_EN|FDD_EN| INTE |DIN2D|PR |RST|MC_EN|
    +-----------+-----+-------+-----+------+------+------+-----+---+---+-----+
    |31       17| 16  |15    8|  7  |   6  |   5  |  4   |  3  | 2 | 1 |  0  | 
*/
#if 1
#define SET_C2_CTRL(rete,pgn,fdd_en,inte,din2d,pr)	 \
({ write_reg( (MC2_V_BASE + REG2_CTRL),                  \
              ((rete)   & 0x1 )<<16 |		         \
              ((pgn)    & 0xFF)<<8  |                    \
              ( 0x1 /*CKG_EN*/)<<6  |			 \
              ((fdd_en) & 0x1 )<<5  |		         \
              ((inte)   & 0x1 )<<4  |                    \
              ((din2d)  & 0x1 )<<3  |                    \
              ((pr)     & 0x1 )<<2  |                    \
              ( 0x1 /*MC_EN*/ )<<0  );			 \
})
#else
#define SET_C2_CTRL(pgn,fdd_en,inte,din2d)               \
({ write_reg( (MC2_V_BASE + REG2_CTRL),                  \
              ((pgn)    & 0xFF)<<8  |                    \
              ( 0x1           )<<5  |                    \
              ((fdd_en) & 0x1 )<<4  |		         \
              ((inte)   & 0x1 )<<3  |                    \
              ((din2d)  & 0x1 )<<2  |                    \
              ( 0x1           )<<0  );			 \
})
#endif

/*
    REG2_STAT: 0x4
    +----------------+------+-------+
    |  N/C           |LK_END|BLK_END|
    +----------------+------+-------+
    |31             2|   1  |   0   | 
*/
#define SET_C2_FLAG()					\
({ write_reg( (MC2_V_BASE + REG2_STAT), 0x1);		\
})
#define MC_C2_POLLING()                                 \
({ do{}while(read_reg( MC2_V_BASE, REG2_STAT ) != 0x3);	\
})

/*
    REG2_IINFO: 0x10
    +-----+--------+-----+--------+----+-----+--------+--------+-----+---+---+
    | N/C | RND_2S | N/C | RND_1S | EC | N/C | SFT_2S | SFT_1S | POS | W | H | 
    +-----+--------+-----+--------+----+-----+--------+--------+-----+---+---+
    |31 30|29    24|23 22|21    16| 15 |  14 |13    11|10     8|7   4|3 2|1 0| 
     --SFT_2S:	2nd stage shift bits (0~7)
     --SFT_1S:	1st stage shift bits (0~7)
*/
#define SET_C2_IINFO(rnd2s,rnd1s,ec,sft2s,sft1s,pos,w,h) \
({ write_reg( (MC2_V_BASE + REG2_IINFO),                 \
              ((rnd2s) & 0x3F)<<24 |                     \
              ((rnd1s) & 0x3F)<<16 |                     \
              ((ec)    & 0x1 )<<15 |                     \
              ((sft2s) & 0x7 )<<11 |                     \
              ((sft1s) & 0x7 )<<8  |                     \
              ((pos)   & 0xF )<<4  |                     \
              ((w)     & 0x3 )<<2  |                     \
              ((h)     & 0x3 )<<0  );                    \
})

/*
    REG2_TAP: 0x14
    +-----+--------+-----+--------+-----+--------+-----+--------+
    | N/C | TAP2M  | N/C | TAP2L  | N/C | TAP1M  | N/C | TAP1L  | 
    +-----+--------+-----+--------+-----+--------+-----+--------+
    |31 28|27    24|23 20|19    16|15 12|11     8|7   4|3      0| 
*/
#define SET_C2_TAP(tap2m,tap2l,tap1m,tap1l)             \
({ write_reg( (MC2_V_BASE + REG2_TAP),                  \
              ((tap2m) & 0xF)<<24 |                     \
              ((tap2l) & 0xF)<<16 |                     \
              ((tap1m) & 0xF)<<8  |                     \
              ((tap1l) & 0xF)<<0  );		        \
})

/*
    REG2_STRD: 0x18
    +-----+-------+------+-----+-------+
    | N/C | CUNIT |CSTRD | N/C | RSTRD |
    +-----+-------+------+-----+-------+
    |31 26|25   24|23  16|15 12|11    0| 
*/
#define SET_C2_STRD(cunit,cstrd,rstrd)			\
({ write_reg( (MC2_V_BASE + REG2_STRD),                 \
              ((cunit) & 0x3  )<<24 |			\
              ((cstrd) & 0xFF )<<16 |			\
              ((rstrd) & 0xFFF)<<0  );			\
})

/*
    REG2_FDDC: 0x1C
    +-------+-----+-------+
    |  DHA  | N/C |  RUN  |
    +-------+-----+-------+
    |31    2|  1  |   0   | 
*/
#define SET_C2_FDDC(data)                              \
({ write_reg( (MC2_V_BASE + REG2_FDDC), data);         \
})

/*
    RAM2_RNDT(rounding table): 0x40~0x7C
    +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
    |TAP2M|TAP2L|TAP1M|TAP1L|RND2S|RND1S| N/C |SFT2S| N/C |SFT1S| 
    +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
    |31 29|28 26|25 23|22 20|19 14|13  8|  7  |6   4|  3  |2   0| 
     Note: RAM2_RNDT is forbidden for C2FDD data mode's self-configuration
*/
#define SET_C2_RNDT(RNDT2_POS,tap2m,tap2l,tap1m,tap1l, \
		    rnd2s,rnd1s,sft2s,sft1s)	       \
({ write_reg( (MC2_V_BASE + (RNDT2_POS)),              \
              ((tap2m) & 0x7 )<<29 |		       \
              ((tap2l) & 0x7 )<<26 |		       \
              ((tap1m) & 0x7 )<<23 |		       \
              ((tap1l) & 0x7 )<<20 |		       \
              ((rnd2s) & 0x3F)<<14 |		       \
              ((rnd1s) & 0x3F)<<8  |		       \
              ((sft2s) & 0x7 )<<4  |		       \
              ((sft1s) & 0x7 )<<0  );		       \
})

/*********************** MC Channel-2 FDD *************************************
    C2FDD is similiar with C1FDD except snap/capture/auto/data mode:
    1)snap/capture/auto mode
    +---+-----+----+---+-----+-----+-----+--+-----+-----+----+----+-----+-----+
    |PD | MD  | LK |D1D| RAW |YPOS |XPOS |EC| N/C | BW  | BH | BN | SBN | POS |
    +---+-----+----+---+-----+-----+-----+--+-----+-----+----+----+-----+-----+
    | 31|30 28| 27 |26 |25 24|23 20|19 16|15|14 12|11 10|9  8|7  6|5   4|3   0|
    
    D1D:  DMA 1-line, when set, reference stride is Not-Cared
    RAW:  00-UP; 01-VP; 10-UF; 11-VF (only used in progress record case)
    YPOS: Y-direction sub-pixel position (only used in h264 1/8-pixel case)
    XPOS: X-direction sub-pixel position (only used in h264 1/8-pixel case)
    EC:   h264 eighth-pixel mode
    BW:   block width, same with BINFO.BW
    BH:   block height, same with BINFO.BH
    BN:   block number, 0~4
    SBN:  sub-block number, 0~4
    POS:  except in h264 1/8-pixel case, used as 2-D sub-pixel position

    2)data mode
    +---+-----+-----+------------------------------------------+----+----+----+
    |PD | MD  | LK  |                      N/C                 |CSA |N/C |DTC |
    +---+-----+-----+------------------------------------------+----+----+----+
    | 31|30 28| 27  |26                                      13|12 8|7  5|4  0|

    CSA:  Transfer start index, MC FDD config slave-reg from this index register
    DTC:  Transfer number, MC FDD from CSA and down config DTC number registers
    [Notes]
        1. un-like C1FDD DATA AUTO mode, C2FDD data mode would never startup 
           host except by clear REG2_STAT.BLK_END
******************************************************************************/
#define FDD2_MD_CAP       0x8<<28
#define FDD2_MD_DATA      0x9<<28
#define FDD2_MD_RUN       0xA<<28
#define FDD2_MD_SYNC      0xB<<28
#define FDD2_MD_SKIP      0xC<<28
#define FDD2_MD_JUMP      0xD<<28
#define FDD2_MD_SNAP      0xE<<28
#define FDD2_MD_AUTO      0xF<<28

#define FDD2_LK           0x1<<27
#define FDD2_UL           0x0<<27

#define FDD2_PR           0x1<<26
#define FDD2_D1D          0x1<<26
#define FDD2_D2D          0x0<<26

#define FDD2_RAW_UP       0x0<<24
#define FDD2_RAW_VP       0x1<<24
#define FDD2_RAW_UF       0x2<<24
#define FDD2_RAW_VF       0x3<<24

#define FDD2_EC           0x1<<15
#define FDD2_AT           0x1<<15

#define FDD2_DATA_CMD(csa,dtc)                    \
( FDD2_MD_DATA |                                  \
  FDD2_LK |                                       \
  (csa)<<23 |					  \
  (dtc)<<20					  \
)

#endif /*__JZ4760_MC_HW_H__*/
