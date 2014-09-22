/*
 * Copyright (C) 2006-2014 Ingenic Semiconductor CO.,LTD.
 *
 * This file is part of MPlayer.
 *
 * MPlayer is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * MPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with MPlayer; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

//#define LOG_NDEBUG 0
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "JzIPUTable"

#include "Log.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include "sys/stat.h"
#ifdef HAVE_ANDROID_OS
#include "fcntl.h"
#else
#include "sys/fcntl.h"
#endif	/* BUILD_WITH_ANDROID */
#include "sys/ioctl.h"
#include "sys/mman.h"
#include "fcntl.h"
#include "jz_ipu.h"

int jz47_dump_ipu_resize_table(struct ipu_native_info *ipu, int num)
{
	int i, total;

	rsz_lut *h_lut;
	rsz_lut *v_lut;

	int *hoft_table; /*  */
	int *hcoef_table;
	int hcoef_real_heiht;
	/* jz4760's ipu HRSZ_COEF_LUT*/
	int *voft_table;
	int *vcoef_table;
	int vcoef_real_heiht;
	struct ipu_img_param *img;

	if (ipu == NULL) {
		return -1;
	}
	img = &ipu->img;

	h_lut = ipu->h_lut;
	v_lut = ipu->v_lut;

	hoft_table = ipu->hoft_table; /*  */
	hcoef_table= ipu->hcoef_table;
	hcoef_real_heiht = img->hcoef_real_heiht;
	voft_table = ipu->voft_table; /*  */
	vcoef_table= ipu->vcoef_table;
	vcoef_real_heiht = img->vcoef_real_heiht;


	if (num >= 0) {
		//ALOGV ("ipu_reg: %s: 0x%x\n", ipu_regs_name[num >> 2], REG32(IPU_V_BASE + num));
		return (1);
	}
	else if (num == -1) {
		ALOGV(" //bi-cube resize\nint cube_hcoef_table[H_OFT_LUT][4] = {");
		for ( i = 0 ; i < img->hcoef_real_heiht ; i++) 
			ALOGV("\t\t\t{0x%02x,0x%02x, 0x%02x, 0x%02x}, \n", ipu->cube_hcoef_table[i][0], ipu->cube_hcoef_table[i][1], 
			     ipu->cube_hcoef_table[i][2], ipu->cube_hcoef_table[i][3] ); 
		ALOGV("};");
		
		ALOGV(" int cube_vcoef_table[V_OFT_LUT][4] = {");
		for ( i = 0 ; i < img->vcoef_real_heiht ; i++) 
			ALOGV("\t\t\t{0x%02x,0x%02x, 0x%02x, 0x%02x}, \n", ipu->cube_vcoef_table[i][0], ipu->cube_vcoef_table[i][1], 
			     ipu->cube_vcoef_table[i][2], ipu->cube_vcoef_table[i][3] ); 
		ALOGV("};");
	}
	else if (num == -2) {
		for (i = 0; i < IPU_LUT_LEN; i++) {
			ALOGV("ipu_H_LUT(%02d): in:%d, out:%d, coef: 0x%08x\n",
			       i, h_lut[i].in_n, h_lut[i].out_n, h_lut[i].coef);
		}

		for (i = 0; i < IPU_LUT_LEN; i++) {
			ALOGV("ipu_V_LUT(%02d): in:%d, out:%d, coef: 0x%08x\n",
			       i, v_lut[i].in_n, v_lut[i].out_n, v_lut[i].coef);
		}
	}
	else if (num == -3) {
		ALOGV("hcoef_real_heiht=%d\n", hcoef_real_heiht);
		for (i = 0; i < IPU_LUT_LEN; i++) {
			ALOGV("ipu_H_LUT(%02d): hcoef, hoft: %05d %02d\n",
			       i, hcoef_table[i], hoft_table[i]);
		}
		ALOGV("vcoef_real_veiht=%d\n", vcoef_real_heiht);
		for (i = 0; i < IPU_LUT_LEN; i++) {
			ALOGV("ipu_V_LUT(%02d): vcoef, voft: %05d %02d\n",
			       i, vcoef_table[i], voft_table[i]);
		}
	}

	return 1;
}


int dump_ipu_ratio_table(struct ipu_native_info *ipu)
{
	unsigned int i, j, cnt;
	int diff;
	struct Ration2m *ipu_ratio_table;

	if (ipu == NULL) {
		return -1;
	}

	ipu_ratio_table = ipu->ipu_ratio_table;

	if (ipu_ratio_table == 0) {
		ALOGV("no find ratio table!\n");
		return (-1);
	}

	ALOGV("%s():\n", __FUNCTION__);

	for (i = 1; i <= (IPU_LUT_LEN); i++) {
		for (j = 1; j <= (IPU_LUT_LEN); j++) {
			ALOGV("%02dx%02d: (ratio,n,m)= (%06f, %02d, %02d)\n", 
			       i, j,
			       ipu_ratio_table[(i - 1) * IPU_LUT_LEN + j - 1].ratio,
			       ipu_ratio_table[(i - 1) * IPU_LUT_LEN + j - 1].n,
			       ipu_ratio_table[(i - 1) * IPU_LUT_LEN + j - 1].m);

		}
	}

	ALOGV("%s(), DONE...\n", __FUNCTION__);
	return 0;
}


int init_ipu_ratio_table(struct ipu_native_info *ipu)
{
	unsigned int i, j, cnt;
	float diff;
	struct Ration2m *ipu_ratio_table;


	if (ipu == NULL) {
		return -1;
	}
	extern struct host_ipu_table hostiputable;
	memcpy(ipu->ipu_ratio_table,hostiputable.ipu_ratio_table,sizeof(hostiputable.ipu_ratio_table));
	ipu->rtable_len = hostiputable.cnt;
	
	ALOGV("ipu->rtable_len = %d\n", ipu->rtable_len);

	dump_ipu_ratio_table(ipu);

	return (0);
}

int find_ipu_ratio_factor(struct ipu_native_info *ipu, float ratio, unsigned int up)
{
	int i, sel;
	float diff, min = 100;
	struct Ration2m *ipu_ratio_table;

	if (ipu == NULL) {
		return -1;
	}

	ipu_ratio_table = ipu->ipu_ratio_table;

	sel = ipu->rtable_len;
	ALOGV("sel=%d", sel);
	ALOGV("ratio = %06f", ratio);

	for (i = 0; i < ipu->rtable_len; i++) {
/*
		if ((up == 0) && ((ipu_ratio_table[i].n & 1) != 0)) {
			continue;
		}
*/
		if (ratio > ipu_ratio_table[i].ratio) {
			diff = ratio - ipu_ratio_table[i].ratio;
		} else {
			diff = ipu_ratio_table[i].ratio - ratio;
		}

		if (diff < min || i == 0) {
			min = diff;
			sel = i;
		}
	}

	return (sel);
}


int resize_out_cal(int insize, int outsize, int srcN, int dstM,
			  int upScale, int *diff)
{
	int calsize, delta;
	float tmp, tmp2;

	delta = 1;

	do {
		tmp = (int)((insize - delta) * dstM / (float)srcN);
		tmp2 = tmp * srcN / dstM;
		if (upScale) {
			if (tmp2 == insize - delta) {
				calsize = tmp + 1;
			} else {
				calsize = tmp + 2;
			}
		} else		// down Scale
		{
			if (tmp2 == insize - delta) {
				calsize = tmp;
			} else {
				calsize = tmp + 1;
			}
		}

		delta ++;

	} while (calsize > outsize);

	*diff = delta - 2;

	return (calsize);
}


int resize_lut_cal(int srcN, int dstM, int upScale, rsz_lut lut[])
{
	int i, t, x;
	float w_coef, factor, factor2;

	if (upScale) {
		for (i = 0, t = 0; i < dstM; i++) {
			factor = (float)(i * srcN) / (float)dstM;
			factor2 = factor - (int)factor;
			w_coef = 1.0 - factor2;
			lut[i].coef = (unsigned int) (512.0 * w_coef);
			// calculate in & out
			lut[i].out_n = 1;
			if (t <= factor) {
				lut[i].in_n = 1;
				t ++;
			} else {
				lut[i].in_n = 0;
			}
		}		// end for
	} else {
		for (i = 0, t = 0, x = 0; i < srcN; i++) {
			factor = (float)(t * srcN + 1) / (float)dstM;
			if (dstM == 1) {
				// calculate in & out
				lut[i].coef = (i == 0) ? 256 : 0;
				lut[i].out_n = (i == 0) ? 1 : 0;
			} else {
				if (((t * srcN + 1) / dstM - i) >= 1) {
					lut[i].coef = 0;
				} else {
					if (factor - i == 0) {
						lut[i].coef = 512;
						t++;
					} else {
						factor2 = (float)(t * srcN) / (float)dstM;
						factor = factor - (int)factor2;
						w_coef = 1.0 - factor;
						lut[i].coef = (unsigned int) (512 * w_coef);
						t++;
					}
				}
			}
			// calculate in & out
			lut[i].in_n = 1;
			if (dstM != 1) {
				if (((x * srcN + 1) / dstM - i) >= 1) {
					lut[i].out_n = 0;
				} else {
					lut[i].out_n = 1;
					x++;
				}
			}

		}		// for end down Scale
	}			// else upScale

	return 0;
}


void caculate_h_lut(struct ipu_native_info *ipu, int H_MAX_LUT)
{ 
	int j ; 
	int i; 
	int in_oft_tmp = 0;
	int coef_tmp = 0;  
	rsz_lut *h_lut;
	struct ipu_img_param *img;
	int *hoft_table = &ipu->hoft_table[1];
	int *hcoef_table = &ipu->hcoef_table[1];
	ALOGV("Enter: %s", __FUNCTION__);

	if (ipu == NULL) {
		return;
	}

	img = &ipu->img;
	h_lut = ipu->h_lut;

//	j = 0 ;
	j = -1;
	for (i=0; i<H_MAX_LUT; i++)
	{
		if ( h_lut[i].out_n ) {
			hoft_table[j] = (h_lut[i].in_n == 0)? 0: in_oft_tmp;	    
			hcoef_table[j] = coef_tmp;
			coef_tmp = h_lut[i].coef;
			in_oft_tmp = h_lut[i].in_n==0? in_oft_tmp : h_lut[i].in_n ;
			j++;
		}
		else 
			in_oft_tmp = h_lut[i].in_n + in_oft_tmp;
	}
	if ( h_lut[0].out_n ){
		hoft_table[j] = (h_lut[0].in_n == 0)? 0: in_oft_tmp;	    
		hcoef_table[j] = coef_tmp;
	}
	j++;
	img->hcoef_real_heiht = j;
	ALOGV("img->hcoef_real_heiht = %d", img->hcoef_real_heiht);
	ALOGV("Exit: %s", __FUNCTION__);
}

void caculate_v_lut(struct ipu_native_info *ipu, int V_MAX_LUT)
{ 
	int j;
	int i; 
	int in_oft_tmp = 0;
	int coef_tmp = 0;  
	rsz_lut *v_lut;
	struct ipu_img_param *img;

	int *voft_table = &ipu->voft_table[1];
	int *vcoef_table = &ipu->vcoef_table[1];

	ALOGV("Enter: %s", __FUNCTION__);
	if (ipu == NULL) {
		return;
	}
	img = &ipu->img;

	v_lut = ipu->v_lut;

//	j = 0 ;
	j = -1;
	for (i=0; i<V_MAX_LUT; i++)
	{
		if ( v_lut[i].out_n ){
			voft_table[j] = (v_lut[i].in_n == 0)? 0: in_oft_tmp;	    
			vcoef_table[j] = coef_tmp;
			coef_tmp = v_lut[i].coef;
			in_oft_tmp = v_lut[i].in_n==0? in_oft_tmp : v_lut[i].in_n ;
			j++;
		}
		else 
			in_oft_tmp = v_lut[i].in_n + in_oft_tmp;
	}
	if ( v_lut[0].out_n ){
		voft_table[j] = (v_lut[0].in_n == 0)? 0: in_oft_tmp;	    
		vcoef_table[j] = coef_tmp;
	}
	j++;
	img->vcoef_real_heiht = j;
	ALOGV("img->vcoef_real_heiht = %d", img->vcoef_real_heiht);
	ALOGV("Exit: %s", __FUNCTION__);
}


static double sinxdivx(double x, double CUBE_LEVEL)
{
  //calculate sin(x*PI)/(x*PI)
  const float a = CUBE_LEVEL;//a can be selected : -2, -1, -0.75, -0.5
  double B = 0.0;
  double C =  0.6;
  if ( x < 0) x = -x;
  double x2 = x*x;
  double x3= x*x*x;
  if (x<=1)
    //return ((12-9*B-6*C)*x*x*x + (-18+12*B+6*C)*x*x + 6-2*B);
    return (a+2)*x3 - (a+3)*x2+1;
  else if ( x<=2 )
    //return ((-B-6*C)*x*x*x + (6*B+30*C)*x*x + (-12*B-48*C)*x +8*B+24*C);
    return a*x3 - (5*a)*x2 + (8*a)*x - 4*a;
  else
    return 0;   
  
}

//int sinxdivx_table_8[(2<<9)+1];
void _inti_sinxdivx_table(struct ipu_native_info * ipu, double CUBE_LEVEL)
{
  ALOGV("Enter: %s", __FUNCTION__);
  int i = 0 ; 
  for ( i = 0 ; i < (2<<9); ++i)
    {
      ipu->sinxdivx_table_8[i] = (int)(0.5 + 512*sinxdivx(i*(1.0/512),CUBE_LEVEL));
    }
  ALOGV("Exit: %s", __FUNCTION__);
}


int caculate_cube_coef_table(struct ipu_native_info *ipu, int way_sel)
{
  int i;
  int u_8;
  struct ipu_img_param *img;
  int *hcoef_table = &ipu->hcoef_table[1];
  int *vcoef_table = &ipu->vcoef_table[1];
  ALOGV("Enter: %s", __FUNCTION__);
  if (ipu == NULL) {
	  return -1;
  }

  img = &ipu->img;

  if ( way_sel == 0)
  {
  for (i = 0 ; i <img->hcoef_real_heiht; i++ )
    {
      int au_8[4];
      u_8 = 512 - hcoef_table[i];
      ipu->cube_hcoef_table[i][0] = ipu->sinxdivx_table_8[(1<<9)+u_8];
      ipu->cube_hcoef_table[i][1] = ipu->sinxdivx_table_8[u_8];
      ipu->cube_hcoef_table[i][2] = ipu->sinxdivx_table_8[(1<<9)-u_8];
      ipu->cube_hcoef_table[i][3] = ipu->sinxdivx_table_8[(2<<9)-u_8];
    }
  for (i = 0 ; i <img->vcoef_real_heiht; i++ )
    {
      int au_8[4];
      u_8 = 512 - vcoef_table[i];
      ipu->cube_vcoef_table[i][0] = ipu->sinxdivx_table_8[(1<<9)+u_8];
      ipu->cube_vcoef_table[i][1] = ipu->sinxdivx_table_8[u_8];
      ipu->cube_vcoef_table[i][2] = ipu->sinxdivx_table_8[(1<<9)-u_8];
      ipu->cube_vcoef_table[i][3] = ipu->sinxdivx_table_8[(2<<9)-u_8];
    }
  }
  else
    {
      for ( i = 0 ; i <img->hcoef_real_heiht; i++ ){
      int av_8[4];
      int sum = 0;
      double ratio = 0 ;
      u_8 = 512 - hcoef_table[i];
      av_8[0] = (1<<9)+u_8;
      av_8[1] = u_8;
      av_8[2] = (1<<9)-u_8;
      av_8[3] = (2<<9)-u_8;
      
      av_8[0] = (1<<10) - av_8[0];
      av_8[1] = (1<<10) - av_8[1];
      av_8[2] = (1<<10) - av_8[2];
      av_8[3] = (1<<10) - av_8[3];

      sum = av_8[0] + av_8[1] + av_8[2] + av_8[3];
      ratio = (1<<9)/(double)sum ;
      ipu->cube_hcoef_table[i][0] = (int)(av_8[0] * ratio);
      ipu->cube_hcoef_table[i][1] = (int)(av_8[1] * ratio);
      ipu->cube_hcoef_table[i][2] = (int)(av_8[2] * ratio);
      ipu->cube_hcoef_table[i][3] = (int)(av_8[3] * ratio);
      }

  for (i = 0 ; i <img->hcoef_real_heiht; i++ )
    {
      int au_8[4];
      u_8 = 512 - hcoef_table[i];
      ipu->cube_hcoef_table[i][0] = ipu->sinxdivx_table_8[(1<<9)+u_8];
      ipu->cube_hcoef_table[i][1] = ipu->sinxdivx_table_8[u_8];
      ipu->cube_hcoef_table[i][2] = ipu->sinxdivx_table_8[(1<<9)-u_8];
      ipu->cube_hcoef_table[i][3] = ipu->sinxdivx_table_8[(2<<9)-u_8];
    }

      for ( i = 0 ; i <img->vcoef_real_heiht; i++ ){
      int av_8[4];
      int sum = 0;
      double ratio = 0 ;
      u_8 = 512 - vcoef_table[i];
      av_8[0] = (1<<9)+u_8;
      av_8[1] = u_8;
      av_8[2] = (1<<9)-u_8;
      av_8[3] = (2<<9)-u_8;
      
      av_8[0] = (1<<10) - av_8[0];
      av_8[1] = (1<<10) - av_8[1];
      av_8[2] = (1<<10) - av_8[2];
      av_8[3] = (1<<10) - av_8[3];

      sum = av_8[0] + av_8[1] + av_8[2] + av_8[3];
      ratio = (1<<9)/(double)sum ;
      ipu->cube_vcoef_table[i][0] = (int)(av_8[0] * ratio);
      ipu->cube_vcoef_table[i][1] = (int)(av_8[1] * ratio);
      ipu->cube_vcoef_table[i][2] = (int)(av_8[2] * ratio);
      ipu->cube_vcoef_table[i][3] = (int)(av_8[3] * ratio);
      }
    }

  ALOGV("Exit: %s", __FUNCTION__);
      return 0;
}

void gen_pic_enhance_table(struct ipu_native_info *ipu, int mid)
{
    int i;
    float a, b;
    int tmp;
    char *ptr = ipu->pic_enhance_table;

    mid = 4 * mid;
    for ( i = 0 ; i < mid; i++)
    {
	tmp = ( 512 / (float)mid) * i;
	tmp = (tmp >> 2);
	if ( tmp > 255 )
		tmp = 255;
	ptr[i] = tmp;
    }
    a = ( 1024 - 512 ) / (float)(1024 - mid);
    b = 512 - a * mid;
    for ( ; i < 1024; i++)
    {
	tmp = (int)(a * i + b);
	if ( tmp > 1023 )
		tmp = 1023;
	tmp = tmp >> 2;
	if ( tmp > 255 )
		tmp = 255;
	ptr[i] = tmp;
    }
}
