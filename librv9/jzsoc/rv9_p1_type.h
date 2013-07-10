/*****************************************************************************
 *
 * JZC RV9 Decoder Type Defines
 *
 ****************************************************************************/

#ifndef __JZC_RV9_P1_TYPE_H__
#define __JZC_RV9_P1_TYPE_H__

typedef short DCTELEM;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef int int32_t;
typedef short int16_t;
typedef char int8_t;

#define FF_I_TYPE  1 ///< Intra
#define FF_P_TYPE  2 ///< Predicted
#define FF_B_TYPE  3 ///< Bi-dir predicted
#define PICT_FRAME         3
#define MB_TYPE_SEPARATE_DC 0x01000000
#define IS_INTRA(a)      ((a)&7)
#define IS_SEPARATE_DC(a)   ((a) & MB_TYPE_SEPARATE_DC)
#define LUMA_CBP_BLOCK_MASK 0x33

#define MB_TYPE_16x16      0x0008
#define MB_TYPE_16x8       0x0010
#define MB_TYPE_8x16       0x0020
#define MB_TYPE_8x8        0x0040

#define U_CBP_MASK 0x0F0000
#define V_CBP_MASK 0xF00000

#define IS_INTRA_MBTYPE(mbtype) ((mbtype==0) || (mbtype==1))

#define XCHG2(a,b,t)   t=a;a=b;b=t
#define XCHG3(a,b,c,t)   t=a;a=b;b=c;c=t
#define XCHG4(a,b,c,d,t)   t=a;a=b;b=c;c=d;d=t

enum RV40BlockTypes{
    RV34_MB_TYPE_INTRA,      ///< Intra macroblock
    RV34_MB_TYPE_INTRA16x16, ///< Intra macroblock with DCs in a separate 4x4 block
    RV34_MB_P_16x16,         ///< P-frame macroblock, one motion frame
    RV34_MB_P_8x8,           ///< P-frame macroblock, 8x8 motion compensation partitions
    RV34_MB_B_FORWARD,       ///< B-frame macroblock, forward prediction
    RV34_MB_B_BACKWARD,      ///< B-frame macroblock, backward prediction
    RV34_MB_SKIP,            ///< Skipped block
    RV34_MB_B_DIRECT,        ///< Bidirectionally predicted B-frame macroblock, no motion vectors
    RV34_MB_P_16x8,          ///< P-frame macroblock, 16x8 motion compensation partitions
    RV34_MB_P_8x16,          ///< P-frame macroblock, 8x16 motion compensation partitions
    RV34_MB_B_BIDIR,         ///< Bidirectionally predicted B-frame macroblock, two motion vectors
    RV34_MB_P_MIX16x16,      ///< P-frame macroblock with DCs in a separate 4x4 block, one motion vector
    RV34_MB_TYPES
};

#define IS_16X16(a)      ((a)&MB_TYPE_16x16)
#define IS_16X8(a)       ((a)&MB_TYPE_16x8)
#define IS_8X16(a)       ((a)&MB_TYPE_8x16)
#define IS_8X8(a)        ((a)&MB_TYPE_8x8)

#endif //__JZC_RV9_P1_TYPE_H__
