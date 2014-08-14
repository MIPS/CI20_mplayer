/* hardware/xb4780/hwcomposer/x2d.c
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

#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_ANDROID_OS
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#endif
#ifndef TIOCGWINSZ
#include <sys/ioctl.h>
#endif
#include <err.h>
#include <stdio.h>		/* for convenience */
#include <stdlib.h>		/* for convenience */
#include <stddef.h>		/* for offsetof */
#include <string.h>		/* for convenience */
#include <unistd.h>		/* for convenience */
#include <errno.h>		/* for definition of errno */
#include <stdarg.h>		/* ISO C variable arguments */
#include "../jz4780_ipu_hal/Log.h"
//#include <system/graphics.h>
#include "../jz4780_ipu.h"

#include "x2d.h"
#include "../jz4780_ipu_hal/dmmu.h"

#define X2D_NAME  "/dev/x2d"
#define	MAXLINE	  1024			/* max line length */
#define X2D_SCALE_FACTOR   512.0

//#define DEBUG_X2D

#define DWORD_ALIGN(a) (((a) >> 3) << 3)

static void	err_doit(int, int, const char *, va_list);

/*
 * Error normal, print a message and return.
 */
static int
err_ret(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_doit(0, 0, fmt, ap);
	va_end(ap);
	return -1;
}

/*
 * Error related to systemcall, print a message and return.
 */
static int
err_sys(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_doit(1, errno, fmt, ap);
	va_end(ap);
	return -1;
}

/*
 * Nonfatal error unrelated to a system call.
 * Print a message.
 */
static void
err_msg(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_doit(0, 0, fmt, ap);
	va_end(ap);
}

/*
 * Print a message and return to caller.
 * Caller specifies "errnoflag".
 */
static void
err_doit(int errnoflag, int error, const char *fmt, va_list ap)
{
	char	buf[MAXLINE];

	vsnprintf(buf, MAXLINE, fmt, ap);
	if (errnoflag)
		snprintf(buf+strlen(buf), MAXLINE-strlen(buf), ": %s",
		  strerror(error));
	strcat(buf, "\n");
	ALOGE("%s", buf);
}

/*
 * dump struct jz_x2d_config
 */
static void x2d_dump_cfg(struct x2d_hal_info *x2d)
{
    int i;
	struct jz_x2d_config *x2d_cfg = (struct jz_x2d_config *)x2d->x2d_deliver_data;

    ALOGE("jz_x2d_config watchdog_cnt: %d\n, tlb_base: %08x", x2d_cfg->watchdog_cnt, x2d_cfg->tlb_base);
    ALOGE("dst_address: %08x", x2d_cfg->dst_address);
    ALOGE("dst_alpha_val: %d\n dst_stride:%d\n dst_mask_val:%08x",
         x2d_cfg->dst_alpha_val, x2d_cfg->dst_stride, x2d_cfg->dst_mask_val);
    ALOGE("dst_width: %d\n dst_height:%d\n dst_bcground:%08x",
         x2d_cfg->dst_width, x2d_cfg->dst_height, x2d_cfg->dst_bcground);
    ALOGE("dst_format: %d\n dst_back_en:%08x\n dst_preRGB_en:%08x",
         x2d_cfg->dst_format, x2d_cfg->dst_back_en, x2d_cfg->dst_preRGB_en);
    ALOGE("dst_glb_alpha_en: %d\ndst_mask_en:%08x\n x2d_cfg.layer_num: %d",
         x2d_cfg->dst_glb_alpha_en, x2d_cfg->dst_mask_en, x2d_cfg->layer_num);

    for (i = 0; i < 4; i++) {
        ALOGE("layer[%d]: ======================================\n", i);
        ALOGE("format: %d\n transform: %d\n global_alpha_val: %d\n argb_order: %d",
             x2d_cfg->lay[i].format, x2d_cfg->lay[i].transform,
             x2d_cfg->lay[i].global_alpha_val, x2d_cfg->lay[i].argb_order);
        ALOGE("osd_mode: %d\n preRGB_en: %d\n glb_alpha_en: %d\n mask_en: %d",
             x2d_cfg->lay[i].osd_mode, x2d_cfg->lay[i].preRGB_en,
             x2d_cfg->lay[i].glb_alpha_en, x2d_cfg->lay[i].mask_en);
        ALOGE("color_cov_en: %d\n in_width: %d\n in_height: %d\n",
             x2d_cfg->lay[i].color_cov_en, x2d_cfg->lay[i].in_width,
             x2d_cfg->lay[i].in_height);
        ALOGE("in_w_offset: %d, in_h_offset: %d",
             x2d_cfg->lay[i].in_w_offset, x2d_cfg->lay[i].in_h_offset);
        ALOGE("out_width: %d out_height: %d\n out_w_offset: %d\n out_h_offset: %d\n v_scale_ratio: %d",
	      x2d_cfg->lay[i].out_width,
             x2d_cfg->lay[i].out_height, x2d_cfg->lay[i].out_w_offset,
             x2d_cfg->lay[i].out_h_offset, x2d_cfg->lay[i].v_scale_ratio);
        ALOGE("h_scale_ratio: %d\n yuv address addr: %08x\n u_addr: %08x\n v_addr: %08x",
             x2d_cfg->lay[i].h_scale_ratio, x2d_cfg->lay[i].addr,
             x2d_cfg->lay[i].u_addr, x2d_cfg->lay[i].v_addr);
        ALOGE("y_stride: %d\n v_stride: %d\n",
             x2d_cfg->lay[i].y_stride, x2d_cfg->lay[i].v_stride);
    }
}

/*
 * Open x2d and alloc struct space
 */
int x2d_open(struct x2d_hal_info **x2d)
{
	int fd = -1;
	struct x2d_hal_info *x2d_info = NULL;

	if (!x2d)
		err_ret("%s parameter is NULL!", __func__);

	x2d_info = *x2d;

	if (!x2d_info) {
		x2d_info = (struct x2d_hal_info *)calloc(sizeof(struct x2d_hal_info) +
												 sizeof(struct jz_x2d_config), sizeof(char));
		if (!x2d_info)
			err_sys("alloc struct x2d_hal_info error");
		x2d_info->x2d_deliver_data = (void *)((struct x2d_hal_info *)x2d_info + 1);

		x2d_info->glb_info = (struct x2d_glb_info *)calloc(sizeof(struct x2d_glb_info),
														   sizeof(char));
		if (!x2d_info->glb_info)
			err_sys("alloc struct x2d_glb_info error");
		x2d_info->dst_info = (struct x2d_dst_info *)calloc(sizeof(struct x2d_dst_info),
														   sizeof(char));
		if (!x2d_info->dst_info)
			err_sys("alloc struct x2d_dst_info error");
		x2d_info->layer = (struct src_layer (*)[4])calloc(sizeof(struct src_layer)*4,
														  sizeof(char));
		if (!x2d_info->layer)
			err_sys("alloc struct src_layer error");

		*x2d = x2d_info;
	}

	if ((fd = open(X2D_NAME, O_RDWR)) < 0)
		err_sys("open x2d error");

	x2d_info->glb_info->x2d_fd = fd;

	return fd;
}

static int
get_bpp(struct src_layer *layer)
{
    int bpp = 0;
	int fmt = layer->format;

    switch (fmt) {
	case HAL_PIXEL_FORMAT_RGB_565:
            bpp = 2;
            break;
	case HAL_PIXEL_FORMAT_RGBA_8888:
	case HAL_PIXEL_FORMAT_BGRA_8888:
	case HAL_PIXEL_FORMAT_RGBX_8888:
	case HAL_PIXEL_FORMAT_BGRX_8888:
            bpp = 4;
            break;
	case HAL_PIXEL_FORMAT_YV12:
	case HAL_PIXEL_FORMAT_YCrCb_420_SP:
            bpp = 16;
            break;
	case HAL_PIXEL_FORMAT_JZ_YUV_420_P:
	case HAL_PIXEL_FORMAT_JZ_YUV_420_B:
            bpp = 16;
            break;
	default:
		err_msg("%s unknown data format 0x%08x", __func__, fmt);
		bpp = 4;
		break;
    }

    return bpp;
}

static int
cal_deliver_addr(struct src_layer *lay)
{
	int bpp = 0, ret = 0, offset = 0;
    struct dmmu_mem_info mem;

	bpp = get_bpp(lay);

	if (lay->in_w_offset || lay->in_h_offset) {
		offset = (lay->in_h_offset*lay->y_stride + lay->in_w_offset) * bpp;
		lay->addr += offset;
		lay->addr = DWORD_ALIGN(lay->addr);
	}

	return 0;
}

/*
 * Micro HWC* can be defined as you wish
 */
static void
transform_deliver_rot(struct src_layer *layer)
{
    switch (layer->transform) {
	case HAL_TRANSFORM_FLIP_H:
		layer->transform = X2D_H_MIRROR;
		break;
	case HAL_TRANSFORM_FLIP_V:
		layer->transform = X2D_V_MIRROR;
		break;
	case HAL_TRANSFORM_ROT_90:
		layer->transform = X2D_ROTATE_90;
		break;
	case HAL_TRANSFORM_ROT_180:
		layer->transform = X2D_ROTATE_180;
		break;
	case HAL_TRANSFORM_ROT_270:
		layer->transform = X2D_ROTATE_270;
		break;
	default:
		layer->transform = X2D_ROTATE_0;
		break;
    }
}

static void
transform_tox2d_format(int *format, int *argb_order)
{
	int fmt = *format;

    switch (fmt) {
	case HAL_PIXEL_FORMAT_RGB_565:
		*argb_order = X2D_RGBORDER_RGB;
		*format = X2D_INFORMAT_RGB565;
		break;
	case HAL_PIXEL_FORMAT_RGBA_8888:
		*argb_order = X2D_RGBORDER_BGR;
		*format = X2D_INFORMAT_ARGB888;
		break;
	case HAL_PIXEL_FORMAT_RGBX_8888:
		*argb_order = X2D_RGBORDER_BGR;
		*format = X2D_INFORMAT_ARGB888;
		break;
	case HAL_PIXEL_FORMAT_BGRX_8888:
		*argb_order = X2D_RGBORDER_RGB;
		*format = X2D_INFORMAT_ARGB888;
		break;
	case HAL_PIXEL_FORMAT_BGRA_8888:
		*argb_order = X2D_RGBORDER_RGB;
		*format = X2D_INFORMAT_ARGB888;
		break;
	case HAL_PIXEL_FORMAT_YCrCb_420_SP:
		*format = X2D_INFORMAT_NV12;
		break;
	case HAL_PIXEL_FORMAT_YV12:
	case HAL_PIXEL_FORMAT_JZ_YUV_420_P:
		*format = X2D_INFORMAT_YUV420SP;
		break;
	case HAL_PIXEL_FORMAT_JZ_YUV_420_B:
		*format = X2D_INFORMAT_TILE420;
		break;
	default:
		err_msg("%s %s %d: unknown layer data format 0x%08x",
				__FILE__, __func__, __LINE__, fmt);
		*format = X2D_INFORMAT_NOTSUPPORT;
		break;
    }
}

static void
cal_deliver_size(struct src_layer *layer)
{
	int bpp = 0;
	float h_scale = 0.0, v_scale = 0.0;

	bpp = get_bpp(layer);

	if (layer->in_w_offset) {
		layer->addr += layer->in_w_offset * bpp;
		layer->addr = DWORD_ALIGN(layer->addr);
	}
	if (layer->in_h_offset) {
		layer->addr += layer->in_h_offset * layer->y_stride;
		layer->addr = DWORD_ALIGN(layer->addr);
	}

	if (layer->out_width && layer->out_height) {
		switch (layer->transform) {
		case X2D_H_MIRROR:
		case X2D_V_MIRROR:
		case X2D_ROTATE_0:
		case X2D_ROTATE_180:
			h_scale = (float)layer->in_width / (float)layer->out_width;
			v_scale = (float)layer->in_height / (float)layer->out_height;
			layer->h_scale_ratio = (int)(h_scale * X2D_SCALE_FACTOR);
			layer->v_scale_ratio = (int)(v_scale * X2D_SCALE_FACTOR);
			break;
		case X2D_ROTATE_90:
		case X2D_ROTATE_270:
			h_scale = (float)layer->in_width / (float)layer->out_height;
			v_scale = (float)layer->in_height / (float)layer->out_width;
			layer->h_scale_ratio = (int)(h_scale * X2D_SCALE_FACTOR);
			layer->v_scale_ratio = (int)(v_scale * X2D_SCALE_FACTOR);
			break;
		default:
			err_msg("%s %s %d undefined rotating degree!",
					__FILE__, __func__, __LINE__);
        }
	} else {
		err_msg("out_width: %d or out_height: %d warning!",
				layer->out_width, layer->out_height);
	}
}

/*
 * Prepare src data for x2d
 */
int x2d_src_init(struct x2d_hal_info *x2d)
{
	int i = 0;
	struct x2d_glb_info *glb_info = NULL;
	struct src_layer (*layer)[4] = NULL;
	struct jz_x2d_config *x2d_cfg = NULL;

	if (!x2d)
		err_ret("%s parameter is NULL!", __func__);

	layer = x2d->layer;
	glb_info = x2d->glb_info;
	x2d_cfg = (struct jz_x2d_config *)x2d->x2d_deliver_data;
	if (!layer || !x2d_cfg || !glb_info)
		err_ret("glb_info: %p layer: %p x2d_cfg: %p is NULL!", glb_info, layer, x2d_cfg);

	x2d_cfg->layer_num = glb_info->layer_num;

	/* deliver src data */
	for (i = 0; i < x2d_cfg->layer_num; i++) {
		memcpy((char *)&x2d_cfg->lay[i], (char *)layer[i], sizeof(struct src_layer));
		int *fmt = &x2d_cfg->lay[i].format;
		int *argb_order = &x2d_cfg->lay[i].argb_order;

		/* calculate all kinds of parameters */
		if (cal_deliver_addr(&x2d_cfg->lay[i]) < 0)
			err_ret("i: %d calculate addr error!", i);

		transform_deliver_rot(&x2d_cfg->lay[i]);

		transform_tox2d_format(fmt, argb_order);

		if (X2D_INFORMAT_NOTSUPPORT == *fmt)
			err_ret("i: %d input format error!", i);

		cal_deliver_size(&x2d_cfg->lay[i]);
	}

	return 0;
}

/*
 * Prepare dst data for x2d
 */
int x2d_dst_init(struct x2d_hal_info *x2d)
{
	int *fmt = NULL, argb_order = 0;
	struct x2d_glb_info *glb_info = NULL;
	struct x2d_dst_info *dst_info = NULL;
	struct jz_x2d_config *x2d_cfg = NULL;

	if (!x2d)
		err_ret("%s parameter is NULL!", __func__);

	glb_info = x2d->glb_info;
	dst_info = x2d->dst_info;
	x2d_cfg = (struct jz_x2d_config *)x2d->x2d_deliver_data;
	if (!glb_info || !dst_info || !x2d_cfg)
		err_ret("glb_info: %p dst_info: %p x2d_cfg: %p error!", 
				glb_info, dst_info, x2d_cfg);

	/* deliver global data */
	x2d_cfg->watchdog_cnt = glb_info->watchdog_cnt;
	x2d_cfg->tlb_base = glb_info->tlb_base;

	memcpy((char *)x2d_cfg+8, (char *)dst_info, sizeof(struct x2d_dst_info));
	fmt = &x2d_cfg->dst_format;

	/* As there is no dst argb order para, we just ignore it or we can add it if neccesary */
	transform_tox2d_format(fmt, &argb_order);

	return 0;
}

/*
 * Start x2d
 */
int x2d_start(struct x2d_hal_info *x2d)
{
	int ret = 0;
	struct jz_x2d_config *x2d_cfg = NULL;

	if (!x2d)
		err_ret("%s parameter is NULL!", __func__);

	x2d_cfg = (struct jz_x2d_config *)x2d->x2d_deliver_data;
	if (!x2d_cfg)
		err_ret("%s x2d_cfg is NULL!", __func__);

#ifdef DEBUG_X2D
	x2d_dump_cfg(x2d);
#endif
	if (ioctl(x2d->glb_info->x2d_fd, IOCTL_X2D_SET_CONFIG, x2d_cfg) < 0)
		err_sys("%s %d: IOCTL_X2D_SET_CONFIG error!", __func__, __LINE__);

	if (ioctl(x2d->glb_info->x2d_fd, IOCTL_X2D_START_COMPOSE) < 0)
		err_sys("%s %d: IOCTL_X2D_START_COMPOSE error!", __func__, __LINE__);

	return 0;
}

/*
 * Stop x2d
 */
int x2d_stop(struct x2d_hal_info *x2d)
{
	struct x2d_glb_info *glb_info = NULL;

	if (!x2d)
		err_ret("%s parameter is NULL!", __func__);
	glb_info = x2d->glb_info;
	if (!glb_info)
		err_ret("%s glb_info is NULL!", __func__);

	if (ioctl(glb_info->x2d_fd, IOCTL_X2D_STOP) < 0)
		err_sys("%s %d IOCTL_X2D_STOP error!", __func__, __LINE__);

	return 0;
}

/*
 * Release x2d
 */
int x2d_release(struct x2d_hal_info **x2d)
{
	struct x2d_hal_info *x2d_info = *x2d;

	close(x2d_info->glb_info->x2d_fd);
	free(x2d_info->glb_info);
	free(x2d_info->dst_info);
	free(x2d_info->layer);
	free(x2d_info);

	return 0;
}
