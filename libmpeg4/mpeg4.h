#ifndef _MPEG4_H_
#define _MPEG4_H_

#define I_TYPE 1
#define P_TYPE 2
#define B_TYPE 3

av_cold int mpeg4_decode_end(AVCodecContext *avctx);
int mpeg4_decode_frame(AVCodecContext *avctx, void *data, int *data_size, AVPacket *avpkt);
av_cold int mpeg4_decode_init(AVCodecContext *avctx);
int mpeg4_decode_mb_p0(MpegEncContext *s, DCTELEM block[6][64]);


#endif
