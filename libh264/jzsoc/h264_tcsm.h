/*****************************************************************************
 *
 * JZ4760 TCSM Space Seperate
 *
 * $Id: h264_tcsm.h,v 1.1.1.1 2012/03/27 04:02:56 dqliu Exp $
 *
 ****************************************************************************/

#ifndef __H264_TCSM_H__
#define __H264_TCSM_H__

#define TCSM0_BANK0 0xF4000000
#define TCSM0_BANK1 0xF4001000
#define TCSM0_BANK2 0xF4002000
#define TCSM0_BANK3 0xF4003000

#define TCSM1_BANK0 0xF4000000
#define TCSM1_BANK1 0xF4001000
#define TCSM1_BANK2 0xF4002000
#define TCSM1_BANK3 0xF4003000

#define SRAM_BANK0  0x132D0000
#define SRAM_BANK1  0x132D1000
#define SRAM_BANK2  0x132D2000
#define SRAM_BANK3  0x132D3000

/*
  XXXX_PADDR:       physical address
  XXXX_VCADDR:      virtual cache-able address
  XXXX_VUCADDR:     virtual un-cache-able address 
*/
#define TCSM0_PADDR(a)        (((a) & 0xFFFF) | 0x132B0000) 
#define TCSM0_VCADDR(a)       (((a) & 0xFFFF) | 0x932B0000) 
#define TCSM0_VUCADDR(a)      (((a) & 0xFFFF) | 0xB32B0000) 

#define TCSM1_PADDR(a)        (((a) & 0xFFFF) | 0x132C0000) 
#define TCSM1_VCADDR(a)       (((a) & 0xFFFF) | 0x932C0000) 
#define TCSM1_VUCADDR(a)      (((a) & 0xFFFF) | 0xB32C0000) 

#define SRAM_PADDR(a)         (((a) & 0xFFFF) | 0x132D0000) 
#define SRAM_VCADDR(a)        (((a) & 0xFFFF) | 0x932D0000) 
#define SRAM_VUCADDR(a)       (((a) & 0xFFFF) | 0xB32D0000) 

/*------ MC Reference BUF -------*/
/*
  Since H264 MB is seperated into 16 Subblocks, 
  So each Subblock is indexed by its BN(blk ID number) and SBN(sblk ID number)
  +----------------------------+
  | BLK0.SBLK0                 |
  +----------------------------+
  | BLK0.SBLK1                 |
  +----------------------------+
  | BLK0.SBLK2                 |
  +----------------------------+
  | BLK0.SBLK3                 |
  +----------------------------+
  | BLK1.SBLK0                 |
  +----------------------------+
  | BLK1.SBLK1                 |
  +----------------------------+
  | BLK1.SBLK2                 |
  +----------------------------+
  | BLK1.SBLK3                 |
  +----------------------------+
  | BLK2.SBLK0                 |
  +----------------------------+
  | BLK2.SBLK1                 |
  +----------------------------+
  | BLK2.SBLK2                 |
  +----------------------------+
  | BLK2.SBLK3                 |
  +----------------------------+
  | BLK3.SBLK0                 |
  +----------------------------+
  | BLK3.SBLK1                 |
  +----------------------------+
  | BLK3.SBLK2                 |
  +----------------------------+
  | BLK3.SBLK3                 |
  +----------------------------+
  Note:
       For Luma, the subblock unit size is 12x9 bytes  (6-tap filter)
       For Chroma, the subblock unit size is 4x3 bytes (2-tap filter)
 */
#define H264_YUNIT             (12*9)
#define H264_CUNIT             ( 4*3)
#define H264_RYBUF_LEN         (H264_YUNIT*16)
#define H264_RCBUF_LEN         (H264_CUNIT*16)
/*Reference Buf: previous(frontward) direction*/
#define TCSM1_RYPBUF           (TCSM1_BANK0)
#define TCSM1_RUPBUF           (TCSM1_RYPBUF+H264_RYBUF_LEN)
#define TCSM1_RVPBUF           (TCSM1_RUPBUF+H264_RCBUF_LEN)
/*Reference Buf: future(backward) direction*/
#define TCSM1_RYFBUF           (TCSM1_RVPBUF+H264_RCBUF_LEN)
#define TCSM1_RUFBUF           (TCSM1_RYFBUF+H264_RYBUF_LEN)
#define TCSM1_RVFBUF           (TCSM1_RUFBUF+H264_RCBUF_LEN)

/*------ MC Destination BUF -------*/
/*
  +-------+-------+-------+-------+
  | BLK0  | BLK0  | BLK1  | BLK1  |
  | SBLK0 | SBLK1 | SBLK0 | SBLK1 |
  |       |       |       |       |
  +-------+-------+-------+-------+
  | BLK0  | BLK0  | BLK1  | BLK1  |
  | SBLK2 | SBLK3 | SBLK2 | SBLK3 |
  |       |       |       |       |
  +-------+-------+-------+-------+
  | BLK2  | BLK2  | BLK3  | BLK3  |
  | SBLK0 | SBLK1 | SBLK0 | SBLK1 |
  |       |       |       |       |
  +-------+-------+-------+-------+
  | BLK2  | BLK2  | BLK3  | BLK3  |
  | SBLK2 | SBLK3 | SBLK2 | SBLK3 |
  |       |       |       |       |
  +-------+-------+-------+-------+
 */
#define H264_YCSTRD            (20)
#define H264_CCSTRD            (12)
#define H264_DYBUF_LEN         (H264_YCSTRD*16)
#define H264_DCBUF_LEN         (H264_CCSTRD* 8)
/*Destination Buf: previous(frontward) direction*/
/*Destination Buf: future(backward) direction*/
#define TCSM1_DYPBUF           (TCSM1_RVFBUF+H264_RCBUF_LEN)
#define TCSM1_DYFBUF           (TCSM1_DYPBUF+H264_DYBUF_LEN)
#define TCSM1_DUPBUF           (TCSM1_DYFBUF+H264_DYBUF_LEN)
#define TCSM1_DUFBUF           (TCSM1_DUPBUF+H264_DCBUF_LEN)
#define TCSM1_DVPBUF           (TCSM1_DUFBUF+H264_DCBUF_LEN)
#define TCSM1_DVFBUF           (TCSM1_DVPBUF+H264_DCBUF_LEN)

/*------ MC Descriptor Chain -------*/
/*
  +--------------------------------+<-- DHA - 12
  | FID: MC processed SBLK ID(Fdir)|
  +--------------------------------+<-- DHA - 8
  | PID: MC processed SBLK ID(Pdir)|
  +--------------------------------+<-- DHA - 4
  | SF: SYNC Status Flag           |
  +--------------------------------+<-- DHA
  | Y MB Start Addr (3 wrod)       |
  |   -- FDD Data Node Head        |
  |   -- Y Reference Start Addr    |
  |   -- Y Destination Start Addr  |
  +--------------------------------+
  | SBLK CMD (1 wrod)              |
  |   -- FDD Auto mode configure   |
  +--------------------------------+
  | SBLK CMD (1 wrod)              |
  |   -- FDD Auto mode configure   |
  +--------------------------------+
   .
   .
   .
  +--------------------------------+
  | JP: JUMP Node (1 wrod)         |
  +--------------------------------+
  | U MB Start Addr (3 wrod)       |
  |   -- FDD Data Node Head        |
  |   -- U Reference Start Addr    |
  |   -- U Destination Start Addr  |
  +--------------------------------+
  | SBLK CMD (1 wrod)              |
  |   -- FDD Auto mode configure   |
  +--------------------------------+
   .
   .
   .
  +--------------------------------+
  | Last SBLK CMD (1 wrod)         |
  |   -- FDD Auto mode configure   |
  +--------------------------------+
  | JP: JUMP Node (1 wrod)         |
  +--------------------------------+
  | FDD SYNC Node (1 wrod)         |
  +--------------------------------+
  Note:
       1. PID: MC FDD auto-write MC's current processed ID into this Address
               CPU can read it to know current MC's progress.
       2. SF:  In FDD SYNC node, FDD auto write the flag into this place.
  H264_NLEN:   Single(Y/U/V) Chain length(3word Start Addr + 16SBLK Node)  
 */
#define H264_NLEN              (3+16+1)
#define TCSM1_MCC_FID          (TCSM1_DVFBUF+H264_DCBUF_LEN) 
#define TCSM1_MCC_PID          (TCSM1_MCC_FID+4)
#define TCSM1_MCC_SF           (TCSM1_MCC_PID+4)
/*MC chain: previous(frontward) direction start address*/
/*MC chain: future(backward) direction start address*/
#define TCSM1_MCC_YPS          (TCSM1_MCC_SF +4)
#define TCSM1_MCC_YFS          (TCSM1_MCC_YPS+H264_NLEN*4)
#define TCSM1_MCC_UPS          (TCSM1_MCC_YFS+H264_NLEN*4)
#define TCSM1_MCC_UFS          (TCSM1_MCC_UPS+H264_NLEN*4)
#define TCSM1_MCC_VPS          (TCSM1_MCC_UFS+H264_NLEN*4)
#define TCSM1_MCC_VFS          (TCSM1_MCC_VPS+H264_NLEN*4)
#define TCSM1_MCC_SN           (TCSM1_MCC_VFS+H264_NLEN*4) 


/*------ IDCT source  buf- lhuang------*/
/*   4x4 
  +-------+-------+-------+-------+----
  | BLK0  | BLK1  | BLK2  | BLK3  |
  +-------+-------+-------+-------+
  | BLK4  | BLK5  | BLK6  | BLK7  |
  +-------+-------+-------+-------+ lunma
  | BLK8  | BLK9  | BLK10 | BLK11 |
  +-------+-------+-------+-------+
  | BLK12 | BLK13 | BLK14 | BLK15 |
  +-------+-------+-------+-------+-----
  | BLK16 | BLK17 | BLK18 | BLK19 |
  +-------+-------+-------+-------+ chroma
  | BLK20 | BLK21 | BLK22 | BLK23 |
  +-------+-------+-------+-------+----
 */
/*   8x8 
  +-------+-------+-------+-------+----
  |            BLK8x8_0           |
  +-------+-------+-------+-------+
  |            BLK8x8_1           |
  +-------+-------+-------+-------+ lunma
  |            BLK8x8_2           |
  +-------+-------+-------+-------+
  |            BLK8x8_3           |
  +-------+-------+-------+-------+-----
  | BLK16 | BLK17 | BLK18 | BLK19 |
  +-------+-------+-------+-------+ chroma
  | BLK20 | BLK21 | BLK22 | BLK23 |
  +-------+-------+-------+-------+----
 */


#define TCSM0_PHY_ADDR(addr) (((unsigned int)addr & 0xFFFF) | 0x132b0000)
#define TCSM1_PHY_ADDR(addr) (((unsigned int)addr & 0xFFFF) | 0x132c0000)
/*------ IDCT description chain  buf- lhuang------*/
/*   INTER chain
  +-------+-------+-------+-------+
  |            0x80000000         |
  +-------+-------+-------+-------+
  |            HEAD               |
  +-------+-------+-------+-------+
  |            CBP                |
  +-------+-------+-------+-------+
  |            DEST_ADDR          |
  +-------+-------+-------+-------+
  |            SRC_ADDR           |
  +-------+-------+-------+-------+  
 */
/*   INTRA chain (4x4:23 w  8x8: 24w)
  +-------+-------+-------+-------+-----------
  |            0x80000000         |
  +-------+-------+-------+-------+
  |            HEAD               |
  +-------+-------+-------+-------+
  |      TOP and LEFT Pixel       |
  |      4x4:9 W   8x8: 10w       | 
  +-------+-------+-------+-------+
  |        PREDICTION MODE        |
  |            2 W                |
  +-------+-------+-------+-------+   LUMA prediction
  |         HAS_TOP_LEFT          |
  +-------+-------+-------+-------+  
  |             DC_CBP            |
  +-------+-------+-------+-------+  
  |              CBP              |
  +-------+-------+-------+-------+  
  |            DEST_ADDR          |
  +-------+-------+-------+-------+
  |            SRC_ADDR           |
  +-------+-------+-------+-------+---------
  |            0x80000000         |
  +-------+-------+-------+-------+
  |            HEAD               |
  +-------+-------+-------+-------+   CHROMA IDCT
  |            CBP                |
  +-------+-------+-------+-------+
  |            DEST_ADDR          |
  +-------+-------+-------+-------+
  |            SRC_ADDR           |
  +-------+-------+-------+-------+========
 */

#define H264_IDCT_CHAIN_BASE (TCSM1_MCC_SN+H264_NLEN+4)
#define H264_IDCT_CHAIN_LEN (24*4)

#define H264_IDCT_CHAIN_HEAD (H264_IDCT_CHAIN_BASE)
#define H264_IDCT_CHAIN_HEAD_LEN (2*4)

#define H264_IDCT_SRC_BASE (H264_IDCT_CHAIN_BASE + H264_IDCT_CHAIN_LEN)
#define H264_IDCT_DATA_BUF_LEN (16*24*2)

/*------ IDCT destinatiion  buf- lhuang------*/
/*   IDCT 4x4 
  +-------+-------+-------+-------+----
  | BLK0  | BLK1  | BLK2  | BLK3  |
  +-------+-------+-------+-------+
  | BLK4  | BLK5  | BLK6  | BLK7  |
  +-------+-------+-------+-------+ lunma
  | BLK8  | BLK9  | BLK10 | BLK11 |
  +-------+-------+-------+-------+
  | BLK12 | BLK13 | BLK14 | BLK15 |
  +-------+-------+-------+-------+-----
  | BLK16 | BLK17 | BLK18 | BLK19 |
  +-------+-------+-------+-------+ chroma
  | BLK20 | BLK21 | BLK22 | BLK23 |
  +-------+-------+-------+-------+----
 */
/*   IDCT 8x8 
  +-------+-------+-------+-------+----
  |            BLK8x8_0           |
  +-------+-------+-------+-------+
  |            BLK8x8_1           |
  +-------+-------+-------+-------+ lunma
  |            BLK8x8_2           |
  +-------+-------+-------+-------+
  |            BLK8x8_3           |
  +-------+-------+-------+-------+-----
  | BLK16 | BLK17 | BLK18 | BLK19 |
  +-------+-------+-------+-------+ chroma
  | BLK20 | BLK21 | BLK22 | BLK23 |
  +-------+-------+-------+-------+----
 */
/*   intra 4x4 
     +-------+-------+-------+-------+----
 BLK | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
     +-------+-------+-------+-------+
     | 8 | 9 | 10| 11| 12| 13| 14| 15|
     +-------+-------+-------+-------+ lunma
     |           NULL                |
     +-------+-------+-------+-------+
     |           NULL                |
     +-------+-------+-------+-------+-----
     | BLK16 | BLK17 | BLK18 | BLK19 |
     +-------+-------+-------+-------+ chroma IDCT result
     | BLK20 | BLK21 | BLK22 | BLK23 |
     +-------+-------+-------+-------+----
 */
/*   IDCT 8x8 
     +-------+-------+-------+-------+----
 BLK | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
     +-------+-------+-------+-------+
     | 8 | 9 | 10| 11| 12| 13| 14| 15|
     +-------+-------+-------+-------+ lunma
     |            NULL               |
     +-------+-------+-------+-------+
     |            NULL               |
     +-------+-------+-------+-------+-----
     | BLK16 | BLK17 | BLK18 | BLK19 |
     +-------+-------+-------+-------+ chroma
     | BLK20 | BLK21 | BLK22 | BLK23 |
     +-------+-------+-------+-------+----
 */
#define H264_IDCT_RES_BASE (H264_IDCT_SRC_BASE + 0) //+ H264_IDCT_SRC_LEN)

#define H264_IDCT_CHAIN1_BASE (H264_IDCT_RES_BASE + H264_IDCT_DATA_BUF_LEN)
#define H264_IDCT_SRC1_BASE (H264_IDCT_CHAIN1_BASE + H264_IDCT_CHAIN_LEN )
#define H264_IDCT_RES1_BASE (H264_IDCT_SRC1_BASE + 0)

#endif /*__H264_TCSM_H__*/
