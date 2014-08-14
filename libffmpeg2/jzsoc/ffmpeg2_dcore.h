/*****************************************************************************
 *
 * JZC MPEG2 Decoder Architecture
 *
 ****************************************************************************/
/* reference words : libvc1->jzsoc->vc1_dcore.h */

#ifndef  __JZC_MPEG2_DCORE_H__
#define  __JZC_MPEG2_DCORE_H__

typedef struct FFMPEG2_PICT{
  uint8_t * data[4];
}FFMPEG2_PICT;

/* these variables we need but not change in a frame */
typedef struct MPEG2_FRAME_GloARGs{
  int32_t width;
  int32_t height;
  int32_t pict_type;
  int32_t picture_structure;   // for each frame, they maybe have different pict struct
  int32_t chroma_x_shift; //depend on pix_format, that depend on chroma_format
  int32_t chroma_y_shift;
  int32_t q_scale_type;
  int32_t first_field;         ///< is 1 for the first field of a field picture 0 otherwise
  int32_t no_rounding;         // need further polishing
  int32_t linesize;
  int32_t uvlinesize;
  uint8_t * cp_data0;
  uint8_t * cp_data1;
}MPEG2_FRAME_GloARGs;

/* these variables maybe change in MPV decoded */
typedef struct MPEG2_MB_DecARGs{
  int32_t block_size;   //using for this block struct to increase the space
  int32_t mb_x;
  int32_t mb_y;
  int32_t qscale;   // if we have new qscale, we get it, if not, we use the old one, so it is global
  int32_t mb_intra;
  int32_t mv_dir;
  int32_t mv_type;
  int32_t interlaced_dct;  
  int32_t cbp;
  int32_t len_count;

  int32_t mv[2][4][2];
  int32_t field_select[2][2];

  DCTELEM blocks[6][64];
}MPEG2_MB_DecARGs;

#define MV_TYPE_16X16       0   ///< 1 vector for the whole mb
#define MV_TYPE_8X8         1   ///< 4 vectors (h263, mpeg4 4MV)
#define MV_TYPE_16X8        2   ///< 2 vectors, one per 16x8 block
#define MV_TYPE_FIELD       3   ///< 2 vectors, one per field
#define MV_TYPE_DMV         4   ///< 2 vectors, special mpeg2 Dual Prime Vectors

#endif   /* __JZC_MPEG2_ARCH_H__ */
