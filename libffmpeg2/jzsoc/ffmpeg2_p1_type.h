/*****************************************************************************
 *
 * JZC H264 Decoder Type Defines
 *
 ****************************************************************************/

#ifndef __JZC_VC1_AUX_TYPE_H__
#define __JZC_VC1_AUX_TYPE_H__

#define CODEC_FLAG_GRAY 0x2000
#define CODEC_FLAG_EMU_EDGE 0x4000

/* picture type */
#define PICT_TOP_FIELD     1
#define PICT_BOTTOM_FIELD  2
#define PICT_FRAME         3

#define FF_I_TYPE  1 ///< Intra
#define FF_P_TYPE  2 ///< Predicted
#define FF_B_TYPE  3 ///< Bi-dir predicted

//mv_dir
#define MV_DIR_FORWARD   1
#define MV_DIR_BACKWARD  2
#define MV_DIRECT        4 ///< bidirectional mode where the difference equals the MV of the last P/S/I-Frame (mpeg4)

//mv_type
#define MV_TYPE_16X16       0   ///< 1 vector for the whole mb
#define MV_TYPE_8X8         1   ///< 4 vectors (h263, mpeg4 4MV)
#define MV_TYPE_16X8        2   ///< 2 vectors, one per 16x8 block
#define MV_TYPE_FIELD       3   ///< 2 vectors, one per field
#define MV_TYPE_DMV         4   ///< 2 vectors, special mpeg2 Dual Prime Vectors

#define FFMIN(a,b) ((a) > (b) ? (b) : (a))
#define FFMAX(a,b) ((a) < (b) ? (b) : (a))

typedef short DCTELEM;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef int int32_t;
typedef short int16_t;
typedef char int8_t;

typedef void (*op_pixels_func)(uint8_t *block, const uint8_t *pixels, int line_size, int h);
typedef void (*tpel_mc_func)(uint8_t *block, const uint8_t *pixels, int line_size, int w, int h);
typedef void (*qpel_mc_func)(uint8_t *dst, uint8_t *src, int stride);
typedef void (*h264_chroma_mc_func)(uint8_t *dst, uint8_t *src, int srcStride, int h, int x, int y);
typedef void (*op_fill_func)(uint8_t *block, uint8_t value, int line_size, int h);

static inline int ff_h263_round_chroma(int x){
    static const uint8_t h263_chroma_roundtab[16] = {
    //  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
        0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1,
    };
    return h263_chroma_roundtab[x & 0xf] + (x >> 3);
}

/**
 * clip a signed integer value into the amin-amax range
 * @param a value to clip
 * @param amin minimum value of the clip range
 * @param amax maximum value of the clip range
 * @return clipped value
 */
static inline int av_clip(int a, int amin, int amax)
{
    if (a < amin)      return amin;
    else if (a > amax) return amax;
    else               return a;
}

enum AVDiscard{
    /* We leave some space between them for extensions (drop some
     * keyframes for intra-only or drop just some bidir frames). */
    AVDISCARD_NONE   =-16, ///< discard nothing
    AVDISCARD_DEFAULT=  0, ///< discard useless packets like 0 size packets in avi
    AVDISCARD_NONREF =  8, ///< discard all non reference
    AVDISCARD_BIDIR  = 16, ///< discard all bidirectional frames
    AVDISCARD_NONKEY = 32, ///< discard all frames except keyframes
    AVDISCARD_ALL    = 48, ///< discard all
};

#endif //__JZC_VC1_AUX_TYPE_H__
