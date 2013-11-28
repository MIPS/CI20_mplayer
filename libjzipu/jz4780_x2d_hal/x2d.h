/* hardware/xb4780/hwcomposer/x2d.h
 *
 * Copyright (c) 2013 Ingenic Semiconductor Co., Ltd.
 *              http://www.ingenic.com/
 *
 * General hal file for Ingenic X2D device
 *    Sean Tang <ctang@ingenic.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __X2D_H__
#define __X2D_H__

__BEGIN_DECLS

#define X2D_IOCTL_MAGIC 'X'

#define IOCTL_X2D_SET_CONFIG		_IOW(X2D_IOCTL_MAGIC, 0x01, struct jz_x2d_config)
#define IOCTL_X2D_START_COMPOSE		_IO(X2D_IOCTL_MAGIC, 0x02)
#define IOCTL_X2D_GET_SYSINFO		_IOR(X2D_IOCTL_MAGIC, 0x03, struct jz_x2d_config)
#define IOCTL_X2D_STOP			    _IO(X2D_IOCTL_MAGIC, 0x04)

#define X2D_INFORMAT_NOTSUPPORT		-1
#define X2D_RGBORDER_RGB 		 0
#define X2D_RGBORDER_BGR		 1
#define X2D_RGBORDER_GRB		 2
#define X2D_RGBORDER_BRG		 3
#define X2D_RGBORDER_RBG		 4
#define X2D_RGBORDER_GBR		 5

#define X2D_ALPHA_POSLOW		 1
#define X2D_ALPHA_POSHIGH		 0

#define X2D_H_MIRROR			 4
#define X2D_V_MIRROR			 8
#define X2D_ROTATE_0			 0
#define	X2D_ROTATE_90 			 1
#define X2D_ROTATE_180			 2
#define X2D_ROTATE_270			 3

#define X2D_INFORMAT_ARGB888		0
#define X2D_INFORMAT_RGB555		    1
#define X2D_INFORMAT_RGB565		    2
#define X2D_INFORMAT_YUV420SP		3
#define X2D_INFORMAT_TILE420		4
#define X2D_INFORMAT_NV12		    5
#define X2D_INFORMAT_NV21		    6

#define X2D_OUTFORMAT_ARGB888		0
#define X2D_OUTFORMAT_XRGB888		1
#define X2D_OUTFORMAT_RGB565		2
#define X2D_OUTFORMAT_RGB555		3

#define X2D_OSD_MOD_CLEAR		    3
#define X2D_OSD_MOD_SOURCE		    1
#define X2D_OSD_MOD_DST			    2
#define X2D_OSD_MOD_SRC_OVER		0
#define X2D_OSD_MOD_DST_OVER		4
#define X2D_OSD_MOD_SRC_IN		    5
#define X2D_OSD_MOD_DST_IN		    6
#define X2D_OSD_MOD_SRC_OUT		    7
#define X2D_OSD_MOD_DST_OUT		    8
#define X2D_OSD_MOD_SRC_ATOP		9
#define X2D_OSD_MOD_DST_ATOP		0xa
#define X2D_OSD_MOD_XOR			    0xb

/*
 * This struct should not be changed
 * It related to the driver
 */
struct src_layer {
	int format;
	int transform;
	int global_alpha_val;
	int argb_order;
	int osd_mode;
	int preRGB_en;
	int glb_alpha_en;
	int mask_en;
	int color_cov_en;

	/* input output size */
	int in_width;
	int in_height;
	int in_w_offset;
	int in_h_offset;
	int out_width;
	int out_height;
	int out_w_offset;
	int out_h_offset;

	int v_scale_ratio;
	int h_scale_ratio;
	int msk_val;

	/* yuv address */
	int addr;
	int u_addr;
	int v_addr;
	int y_stride;
	int v_stride;
};

/*
 * This struct should not be changed
 * It related to the driver
 */
struct jz_x2d_config {
	int watchdog_cnt;
	unsigned int tlb_base;

	int dst_address;
	int dst_alpha_val;
	int dst_stride;
	int dst_mask_val;
	int dst_width;
	int dst_height;
	int dst_bcground;
	int dst_format;
	int dst_back_en;
	int dst_preRGB_en;
	int dst_glb_alpha_en;
	int dst_mask_en;

	int layer_num;
	struct src_layer lay[4];
};

struct x2d_glb_info {
	int x2d_fd;
	int layer_num;
	int watchdog_cnt;
	unsigned int tlb_base;
};

struct x2d_dst_info {
	int dst_address;
	int dst_alpha_val;
	int dst_stride;
	int dst_mask_val;
	int dst_width;
	int dst_height;
	int dst_bcground;
	int dst_format;
	int dst_back_en;
	int dst_preRGB_en;
	int dst_glb_alpha_en;
	int dst_mask_en;
};

struct x2d_hal_info {
	struct x2d_glb_info *glb_info;
	struct x2d_dst_info *dst_info;
	//struct src_layer (*layer)[4];
	struct src_layer *layer;
	void *x2d_deliver_data;
};

int x2d_open(struct x2d_hal_info **x2d);
int x2d_src_init(struct x2d_hal_info *x2d);
int x2d_dst_init(struct x2d_hal_info *x2d);
int x2d_start(struct x2d_hal_info *x2d);
int x2d_stop(struct x2d_hal_info *x2d);
int x2d_release(struct x2d_hal_info **x2d);

__END_DECLS

#endif //#define __x2d_h__
