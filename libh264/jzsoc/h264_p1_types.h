/*****************************************************************************
 *
 * JZC H264 Decoder Type Defines
 *
 ****************************************************************************/

#ifndef __H264_P1_TYPES_H__
#define __H264_P1_TYPES_H__

typedef short DCTELEM;
typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef int int32_t;
typedef short int16_t;
typedef char int8_t;

#define SDE_I_TYPE 1
#define SDE_P_TYPE 2
#define SDE_B_TYPE 4

#define DBLK_CHN_SCH_ID_IDX 14 //40

typedef struct aux_slic_info
{
  int start_mx;
  int start_my;
  int mb_width;
  int mb_height;
  int qscale;
  int list_count;
  int slice_type;
  int slice_alpha_c0_offset;
  int slice_beta_offset;
  int slice_num;
  int sbits_or_ref0cnt;
  int deblocking_filter;
  int bscnt_or_ref1cnt;
}aux_slic_info, *aux_slic_info_p;

#define XCHG2(a, b) \
({\
  unsigned int tmp; \
  tmp = a; \
  a = b; \
  b = tmp; \
})

#define MB_TYPE_INTRA4x4   0x0001
#define MB_TYPE_INTRA16x16 0x0002 //FIXME H.264-specific
#define MB_TYPE_INTRA_PCM  0x0004 //FIXME H.264-specific
#define MB_TYPE_16x16      0x0008
#define MB_TYPE_16x8       0x0010
#define MB_TYPE_8x16       0x0020
#define MB_TYPE_8x8        0x0040
#define MB_TYPE_INTERLACED 0x0080
#define MB_TYPE_DIRECT2    0x0100 //FIXME
#define MB_TYPE_ACPRED     0x0200
#define MB_TYPE_GMC        0x0400
#define MB_TYPE_SKIP       0x0800
#define MB_TYPE_P0L0       0x1000
#define MB_TYPE_P1L0       0x2000
#define MB_TYPE_P0L1       0x4000
#define MB_TYPE_P1L1       0x8000
#define MB_TYPE_L0         (MB_TYPE_P0L0 | MB_TYPE_P1L0)
#define MB_TYPE_L1         (MB_TYPE_P0L1 | MB_TYPE_P1L1)
#define MB_TYPE_L0L1       (MB_TYPE_L0   | MB_TYPE_L1)
#define MB_TYPE_QUANT      0x00010000
#define MB_TYPE_CBP        0x00020000
//Note bits 24-31 are reserved for codec specific use (h264 ref0, mpeg1 0mv, ...)

#define MB_TYPE_8x8DCT     0x01000000
#define IS_8x8DCT(a)     ((a)&MB_TYPE_8x8DCT)

#define MB_TYPE_INTRA MB_TYPE_INTRA4x4 //default mb_type if theres just one type
#define IS_INTRA4x4(a)   ((a)&MB_TYPE_INTRA4x4)
#define IS_INTRA16x16(a) ((a)&MB_TYPE_INTRA16x16)
#define IS_PCM(a)        ((a)&MB_TYPE_INTRA_PCM)
#define IS_INTRA(a)      ((a)&7)
#define IS_INTER(a)      ((a)&(MB_TYPE_16x16|MB_TYPE_16x8|MB_TYPE_8x16|MB_TYPE_8x8))
#define IS_SKIP(a)       ((a)&MB_TYPE_SKIP)
#define IS_INTRA_PCM(a)  ((a)&MB_TYPE_INTRA_PCM)
#define IS_INTERLACED(a) ((a)&MB_TYPE_INTERLACED)
#define IS_DIRECT(a)     ((a)&MB_TYPE_DIRECT2)
#define IS_GMC(a)        ((a)&MB_TYPE_GMC)
#define IS_16X16(a)      ((a)&MB_TYPE_16x16)
#define IS_16X8(a)       ((a)&MB_TYPE_16x8)
#define IS_8X16(a)       ((a)&MB_TYPE_8x16)
#define IS_8X8(a)        ((a)&MB_TYPE_8x8)
#define IS_SUB_8X8(a)    ((a)&MB_TYPE_16x16) //note reused
#define IS_SUB_8X4(a)    ((a)&MB_TYPE_16x8)  //note reused
#define IS_SUB_4X8(a)    ((a)&MB_TYPE_8x16)  //note reused
#define IS_SUB_4X4(a)    ((a)&MB_TYPE_8x8)   //note reused
#define IS_ACPRED(a)     ((a)&MB_TYPE_ACPRED)
#define IS_QUANT(a)      ((a)&MB_TYPE_QUANT)
#define IS_DIR(a, part, list) ((a) & (MB_TYPE_P0L0<<((part)+2*(list))))
#define USES_LIST(a, list) ((a) & ((MB_TYPE_P0L0|MB_TYPE_P1L0)<<(2*(list)))) ///< does this mb use listX, note does not work if subMBs
#define HAS_CBP(a)        ((a)&MB_TYPE_CBP)

static const char scan4x4[16] = {0,1,4,5,2,3,6,7,8,9,12,13,10,11,14,15};

static const uint8_t zigzag_scan[16]={
 0+0*4, 1+0*4, 0+1*4, 0+2*4,
 1+1*4, 2+0*4, 3+0*4, 2+1*4,
 1+2*4, 0+3*4, 1+3*4, 2+2*4,
 3+1*4, 3+2*4, 2+3*4, 3+3*4,
};

static const uint8_t zigzag_scan8x8_cavlc[64]={
 0+0*8, 1+1*8, 1+2*8, 2+2*8,
 4+1*8, 0+5*8, 3+3*8, 7+0*8,
 3+4*8, 1+7*8, 5+3*8, 6+3*8,
 2+7*8, 6+4*8, 5+6*8, 7+5*8,
 1+0*8, 2+0*8, 0+3*8, 3+1*8,
 3+2*8, 0+6*8, 4+2*8, 6+1*8,
 2+5*8, 2+6*8, 6+2*8, 5+4*8,
 3+7*8, 7+3*8, 4+7*8, 7+6*8,
 0+1*8, 3+0*8, 0+4*8, 4+0*8,
 2+3*8, 1+5*8, 5+1*8, 5+2*8,
 1+6*8, 3+5*8, 7+1*8, 4+5*8,
 4+6*8, 7+4*8, 5+7*8, 6+7*8,
 0+2*8, 2+1*8, 1+3*8, 5+0*8,
 1+4*8, 2+4*8, 6+0*8, 4+3*8,
 0+7*8, 4+4*8, 7+2*8, 3+6*8,
 5+5*8, 6+5*8, 6+6*8, 7+7*8,
};

static const uint8_t luma_dc_zigzag_scan[16]={
 0*16 + 0*64, 1*16 + 0*64, 2*16 + 0*64, 0*16 + 2*64,
 3*16 + 0*64, 0*16 + 1*64, 1*16 + 1*64, 2*16 + 1*64,
 1*16 + 2*64, 2*16 + 2*64, 3*16 + 2*64, 0*16 + 3*64,
 3*16 + 1*64, 1*16 + 3*64, 2*16 + 3*64, 3*16 + 3*64,
};

static const uint8_t chroma_dc_scan_ori[4]={
 (0+0*2)*16, (1+0*2)*16,
 (0+1*2)*16, (1+1*2)*16,  //FIXME
};

static const uint8_t chroma_dc_scan[4]={
 (0+0*2)*1, (1+0*2)*1,
 (0+1*2)*1, (1+1*2)*1,
};

static const uint8_t zigzag_scan8x8[64]={
 0+0*8, 1+0*8, 0+1*8, 0+2*8,
 1+1*8, 2+0*8, 3+0*8, 2+1*8,
 1+2*8, 0+3*8, 0+4*8, 1+3*8,
 2+2*8, 3+1*8, 4+0*8, 5+0*8,
 4+1*8, 3+2*8, 2+3*8, 1+4*8,
 0+5*8, 0+6*8, 1+5*8, 2+4*8,
 3+3*8, 4+2*8, 5+1*8, 6+0*8,
 7+0*8, 6+1*8, 5+2*8, 4+3*8,
 3+4*8, 2+5*8, 1+6*8, 0+7*8,
 1+7*8, 2+6*8, 3+5*8, 4+4*8,
 5+3*8, 6+2*8, 7+1*8, 7+2*8,
 6+3*8, 5+4*8, 4+5*8, 3+6*8,
 2+7*8, 3+7*8, 4+6*8, 5+5*8,
 6+4*8, 7+3*8, 7+4*8, 6+5*8,
 5+6*8, 4+7*8, 5+7*8, 6+6*8,
 7+5*8, 7+6*8, 6+7*8, 7+7*8,
};

#endif //__H264_P1_TYPES_H__
