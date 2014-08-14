#ifndef JZDSPUTIL_H_P1
#define JZDSPUTIL_H_P1

#include "ffmpeg2_dcore.h"

void inline ffmpeg2_inter_idct( MPEG2_FRAME_GloARGs* dFRM, MPEG2_MB_DecARGs * dMB);

void inline ffmpeg2_intra_idct( MPEG2_FRAME_GloARGs* dFRM, MPEG2_MB_DecARGs * dMB);

void inline idct_add_mxu(uint8_t *dest, int line_size, short *block);

void inline ffmpeg2_frame_mc(MPEG2_FRAME_GloARGs* dFRM, MPEG2_MB_DecARGs * dMB);

#endif //JZDSPUTIL_H_P1
