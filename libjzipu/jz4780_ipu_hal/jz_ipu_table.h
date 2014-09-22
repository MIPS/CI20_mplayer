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

#ifndef __ANDROID_JZ_IPU_TABLE_H__
#define __ANDROID_JZ_IPU_TABLE_H__

#include <linux/fb.h>
//#include "jz4750_android_ipu.h"

#ifdef __cplusplus
extern "C" {
#endif
	int dump_ipu_ratio_table(struct ipu_native_info *ipu);
	int init_ipu_ratio_table(struct ipu_native_info *ipu);
	int find_ipu_ratio_factor(struct ipu_native_info *ipu, float ratio, unsigned int up);
	int resize_out_cal(int insize, int outsize, int srcN, int dstM, int upScale, int *diff);
	int resize_lut_cal(int srcN, int dstM, int upScale, rsz_lut lut[]);
	void caculate_h_lut(struct ipu_native_info *ipu, int H_MAX_LUT);
	void caculate_v_lut(struct ipu_native_info *ipu, int V_MAX_LUT);
	void _inti_sinxdivx_table(struct ipu_native_info * ipu, double CUBE_LEVEL);
	int caculate_cube_coef_table(struct ipu_native_info *ipu, int way_sel);
	int jz47_dump_ipu_resize_table(struct ipu_native_info *ipu, int num);
	void gen_pic_enhance_table(struct ipu_native_info *ipu, int mid);

#ifdef __cplusplus
}
#endif

#endif /* __ANDROID_JZ_IPU_TABLE_H__ */ 
