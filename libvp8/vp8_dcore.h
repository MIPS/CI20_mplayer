#ifndef __VP8_DCORE_H__
#define __VP8_DCORE_H__

#define XCHG3(a,b,c,t)   t=a;a=b;b=c;c=t
#define XCHG2(a,b,t)   t=a;a=b;b=t

typedef struct {
    uint8_t skip;
    // todo: make it possible to check for at least (i4x4 or split_mv)
    // in one op. are others needed?
    uint8_t mode;
    uint8_t ref_frame;
    uint8_t partitioning;
    VP56mv mv;
    VP56mv bmv[16];
} VP8Macroblock;

typedef struct {
    uint8_t filter_level;
    uint8_t inner_limit;
    uint8_t inner_filter;
} VP8FilterStrength;

typedef struct _VP8_MB_DecARGs{
  int mb_y;
  int mb_x;
  VP8Macroblock mb;
  int segment;
}VP8_MB_DecARGs;
#endif
