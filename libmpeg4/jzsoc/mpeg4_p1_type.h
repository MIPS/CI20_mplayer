/*****************************************************************************
 *
 * JZC MPEG4 Decoder Type Defines
 *
 ****************************************************************************/

#ifndef __JZC_MPEG4_AUX_TYPE_H__
#define __JZC_MPEG4_AUX_TYPE_H__

typedef short DCTELEM;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef int int32_t;
typedef short int16_t;
typedef char int8_t;

#define TRUE 1

#define I_TYPE  1 // Intra
#define P_TYPE  2 // Predicted
#define B_TYPE  3

#define CODEC_FLAG_GRAY         0x2000

int OFFTAB[4] = {0, 4, 32 , 36};
#define XCHG4(a,b,c,d,t)   t=a;a=b;b=c;c=d;d=t
#define XCHG3(a,b,c,t)   t=a;a=b;b=c;c=t
#define XCHG2(a,b,t)   t=a;a=b;b=t

#endif //__JZC_MPEG4_AUX_TYPE_H__
