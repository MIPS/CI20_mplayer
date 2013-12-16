#include <stdio.h>
#include <stdlib.h>
#include "qt.h"
#include "ht.h"

char dht_sel[] = {0x00, 0x10, 0x01, 0x11};

typedef enum {			/* JPEG marker codes */
    M_SOF0  = 0xc0,
    M_SOF1  = 0xc1,
    M_SOF2  = 0xc2,
    M_SOF3  = 0xc3,
  
    M_SOF5  = 0xc5,
    M_SOF6  = 0xc6,
    M_SOF7  = 0xc7,
  
    M_JPG   = 0xc8,
    M_SOF9  = 0xc9,
    M_SOF10 = 0xca,
    M_SOF11 = 0xcb,
  
    M_SOF13 = 0xcd,
    M_SOF14 = 0xce,
    M_SOF15 = 0xcf,
  
    M_DHT   = 0xc4,
  
    M_DAC   = 0xcc,
  
    M_RST0  = 0xd0,
    M_RST1  = 0xd1,
    M_RST2  = 0xd2,
    M_RST3  = 0xd3,
    M_RST4  = 0xd4,
    M_RST5  = 0xd5,
    M_RST6  = 0xd6,
    M_RST7  = 0xd7,
  
    M_SOI   = 0xd8,
    M_EOI   = 0xd9,
    M_SOS   = 0xda,
    M_DQT   = 0xdb,
    M_DNL   = 0xdc,
    M_DRI   = 0xdd,
    M_DHP   = 0xde,
    M_EXP   = 0xdf,
  
    M_APP0  = 0xe0,
    M_APP1  = 0xe1,
    M_APP2  = 0xe2,
    M_APP3  = 0xe3,
    M_APP4  = 0xe4,
    M_APP5  = 0xe5,
    M_APP6  = 0xe6,
    M_APP7  = 0xe7,
    M_APP8  = 0xe8,
    M_APP9  = 0xe9,
    M_APP10 = 0xea,
    M_APP11 = 0xeb,
    M_APP12 = 0xec,
    M_APP13 = 0xed,
    M_APP14 = 0xee,
    M_APP15 = 0xef,
  
    M_JPG0  = 0xf0,
    M_JPG13 = 0xfd,
    M_COM   = 0xfe,
  
    M_TEM   = 0x01,
  
    M_ERROR = 0x100
} JPEG_MARKER;

int genhead(FILE * fp, int image_width,int image_height, int quality){
    int i, j;
    char *ptr;
    short width = (short)image_width;
    short height = (short)image_height;

    /*************** SOI *****************/
    fputc(0xFF, fp);
    fputc(M_SOI, fp);
    /************** DQT --0 **************/
    fputc(0xFF, fp);
    fputc(M_DQT, fp);
    //Lq=64*2+3
    fputc(0x0, fp);
    fputc(0x43, fp);
    //Pq=0 Tq=0
    fputc(0x00, fp);
    //Qk
    ptr = (char *)&qt[quality][0];
    for(i=0; i<64; i++)
	fputc(*ptr++, fp);

    /************** DQT --1 *************/
    fputc(0xFF, fp);
    fputc(M_DQT, fp);
    //Lq=64*2+3
    fputc(0x0, fp);
    fputc(0x43, fp);
    //Pq=0 Tq=1
    fputc(0x01, fp);
    //Qk
    for(i=0; i<64; i++)
	fputc(*ptr++, fp);
  
    /************* SOF ***************/
    fputc(0xFF, fp);
    fputc(M_SOF0, fp);
    //Lf=17
    fputc(0x0, fp);
    fputc(0x11, fp);
    //P=8 --> 8-bit sample
    fputc(0x8, fp);
    //Y=height
    fputc((height & 0xFF00)>>8, fp);
    fputc(height & 0xFF, fp);
    //X=width
    fputc((width & 0xFF00)>>8, fp);
    fputc(width & 0xFF, fp);
    //Nf=3 (number of components)
    fputc(0x3, fp);
    //C1 --> Y
    fputc(0x1, fp);
    //H=2,V=2
    fputc(0x22, fp);
    //Tq --> QT0
    fputc(0x0, fp);
    //C2 --> U
    fputc(0x2, fp);
    //H=1,V=1
    fputc(0x11, fp);
    //Tq --> QT1
    fputc(0x1, fp);
    //C3 --> V
    fputc(0x3, fp);
    //H=1,V=1
    fputc(0x11, fp);
    //Tq --> QT1
    fputc(0x1, fp);
    /**************** DHT **************/
    for(j=0; j<4; j++){
	fputc(0xFF, fp);
	fputc(M_DHT, fp);
	//Lh
	int size = ht_size[j];
	fputc((size & 0xFF00)>>8, fp);
	fputc(size & 0xFF, fp);
	//Tc Th
	fputc(dht_sel[j], fp);
	//Li
	for(i=0; i<16; i++)
	    fputc(ht_len[j][i], fp);
	//Vij
	for(i=0; i<16; i++){
	    int m;
	    for(m=0; m<ht_len[j][i]; m++)
		fputc(ht_val[j][i][m], fp);
	}
    }
    /************** SOS ****************/
    fputc(0xFF, fp);
    fputc(M_SOS, fp);
    //Ls = 12
    fputc(0x0, fp);
    fputc(0xC, fp);
    //Ns
    fputc(0x3, fp);
    //Cs1 - Y
    fputc(0x1, fp);
    //Td, Ta
    fputc(0x00, fp);
    //Cs2 - U
    fputc(0x2, fp);
    //Td, Ta
    fputc(0x11, fp);
    //Cs3 - V
    fputc(0x3, fp);
    //Td, Ta
    fputc(0x11, fp);
    //Ss
    fputc(0x00, fp);
    //Se
    fputc(0x3f, fp);
    //Ah, Al
    fputc(0x0, fp);

    return 0;
}
