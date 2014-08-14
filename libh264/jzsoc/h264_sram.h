/*****************************************************************************
 *
 * JZ4760 SRAM Space Seperate
 *
 * $Id: h264_sram.h,v 1.1.1.1 2012/03/27 04:02:56 dqliu Exp $
 *
 ****************************************************************************/

#ifndef __H264_SRAM_H__
#define __H264_SRAM_H__


#define SRAM_BANK0  0x132D0000
#define SRAM_BANK1  0x132D1000
#define SRAM_BANK2  0x132D2000
#define SRAM_BANK3  0x132D3000
//#define SRAM_BANK4  0x132D4000
//#define SRAM_BANK5  0x132D5000
//#define SRAM_BANK6  0x132D6000
//#define SRAM_BANK7  0x132D7000

/*
  XXXX_PADDR:       physical address
  XXXX_VCADDR:      virtual cache-able address
  XXXX_VUCADDR:     virtual un-cache-able address 
*/
#define SRAM_PADDR(a)         ((((unsigned)(a)) & 0xFFFF) | 0x132D0000) 
#define SRAM_VCADDR(a)        ((((unsigned)(a)) & 0xFFFF) | 0x932D0000) 
#define SRAM_VUCADDR(a)       ((((unsigned)(a)) & 0xFFFF) | 0xB32D0000) 


#define SRAM_DBLKUP_STRD_Y               (1280+4)
#define SRAM_DBLKUP_STRD_C               (640+4)
#define SRAM_DBLKUP_Y                    (SRAM_BANK0 + 4)
#define SRAM_DBLKUP_U                    (SRAM_DBLKUP_Y+(SRAM_DBLKUP_STRD_Y<<2) + 4)
#define SRAM_DBLKUP_V                    (SRAM_DBLKUP_U+(SRAM_DBLKUP_STRD_C<<2) + 4)

#define SRAM_INTRAUP_STRD_Y              (1284)
#define SRAM_INTRAUP_Y                   (SRAM_DBLKUP_V + (SRAM_DBLKUP_STRD_C<<2) + 4)

#if 0
//#define MC_H_YDIS_THRED 32
#define MC_H_YDIS_THRED 48
#define MC_H_YDIS_STR (MC_H_YDIS_THRED*2)

#define MC_H_CDIS_WIDTH 24//24
#define MC_H_CDIS_THRED (MC_H_CDIS_WIDTH*2)//?? maybe not right

#define MC_V_YDIS_THRED 24
#define MC_V_CDIS_THRED 13

#define MC_YBUF_SIZE (MC_H_YDIS_THRED*MC_V_YDIS_THRED)
#define MC_CBUF_SIZE (MC_H_CDIS_WIDTH*MC_V_CDIS_THRED)

#define SRAM_MC_PRE_BUF (SRAM_DBLKUP_V+(SRAM_DBLKUP_STRD_C<<2))
#define SRAM_MC_PRE_BUF_SIZE ((3*MC_YBUF_SIZE+2*3*MC_CBUF_SIZE)*2)


#define H264_YUNIT             (12*9)
#define H264_CUNIT             ( 8*3)
#define H264_RYBUF_LEN         (H264_YUNIT*16)
#define H264_RCBUF_LEN         (H264_CUNIT*16)

/*Reference Buf: previous(frontward) direction*/
#define SRAM_RY1PBUF           (SRAM_MC_PRE_BUF+SRAM_MC_PRE_BUF_SIZE)
#define SRAM_RV1PBUF           (SRAM_RY1PBUF+H264_RYBUF_LEN)
#define SRAM_RU1PBUF           (SRAM_RV1PBUF+H264_RCBUF_LEN)
/*Reference Buf: future(backward) direction*/
#define SRAM_RY1FBUF           (SRAM_RU1PBUF+H264_RCBUF_LEN)
#define SRAM_RV1FBUF           (SRAM_RY1FBUF+H264_RYBUF_LEN)
#define SRAM_RU1FBUF           (SRAM_RV1FBUF+H264_RCBUF_LEN)


/*Reference Buf: previous(frontward) direction*/
#define SRAM_RYPBUF           (SRAM_RU1FBUF+H264_RCBUF_LEN)
#define SRAM_RVPBUF           (SRAM_RYPBUF+H264_RYBUF_LEN)
#define SRAM_RUPBUF           (SRAM_RVPBUF+H264_RCBUF_LEN)
/*Reference Buf: future(backward) direction*/
#define SRAM_RYFBUF           (SRAM_RUPBUF+H264_RCBUF_LEN)
#define SRAM_RVFBUF           (SRAM_RYFBUF+H264_RYBUF_LEN)
#define SRAM_RUFBUF           (SRAM_RVFBUF+H264_RCBUF_LEN)

#define SRAM_DIS_BUF_END      (SRAM_RUFBUF+H264_RCBUF_LEN)

#define SRAM_GP2_CHAIN_LEN  (((SRAM_GP2_CHN_NOD_NUM)*4)<<2)

#define SRAM_GP2_CHN2       (SRAM_DIS_BUF_END)
#define SRAM_GP2_CHN1       (SRAM_GP2_CHN2+SRAM_GP2_CHAIN_LEN)
#define SRAM_GP2_CHN        (SRAM_GP2_CHN1+SRAM_GP2_CHAIN_LEN)

#define SRAM_GP2_DHA2       (SRAM_GP2_CHN + SRAM_GP2_CHAIN_LEN )
#define SRAM_GP2_DHA1       (SRAM_GP2_DHA2 + 4 )
#define SRAM_GP2_DHA        (SRAM_GP2_DHA1 + 4)

#define SRAM_GP2_DCS         (SRAM_GP2_DHA + 4)

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


#define MC_TMP_BUF (SRAM_GP2_DCS + 4)

#define MC_C_INDEX 0
#endif
#endif /*__H264_SRAM_H__*/
