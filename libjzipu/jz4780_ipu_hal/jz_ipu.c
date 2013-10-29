/* hardware/xb4770/libjzipu/android_jz4780_ipu.c
 *
 * Copyright (c) 2012 Ingenic Semiconductor Co., Ltd.
 *              http://www.ingenic.com/
 *
 * Hal file for Ingenic IPU device
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define LOG_TAG "JzIPUHardware"
#include "Log.h"
#include <linux/fb.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include "sys/stat.h"
#ifdef HAVE_ANDROID_OS
#include "fcntl.h"
#else
#include "sys/fcntl.h"
#endif
#include "sys/ioctl.h"
#include "sys/mman.h"
#include <linux/fb.h>
#include <unistd.h>

//#include <hardware/hardware.h>

#include "jz_ipu.h"
#include "jz_ipu_table.h"

#define WORDALIGN(x) (((x) + 3) & ~0x3)

//#define IPU_DBG

static int ipu0_open_cnt;
static int ipu1_open_cnt;

static void dump_ipu_img_param(struct ipu_img_param *t)
{
	ALOGD("-----------------------------\n");
	ALOGD("ipu_cmd = %x\n", t->ipu_cmd);
	ALOGD("lcdc_id = %d\n", t->lcdc_id);
	ALOGD("output_mode = %x\n", t->output_mode);
	ALOGD("in_width = %x(%d)\n", t->in_width, t->in_width);
	ALOGD("in_height = %x(%d)\n", t->in_height, t->in_height);
	ALOGD("in_bpp = %x\n", t->in_bpp);
	ALOGD("in_fmt = %x\n", t->in_fmt);
	ALOGD("out_fmt = %x\n",t->out_fmt);
	ALOGD("out_x = %x\n",t->out_x);
	ALOGD("out_y = %x\n",t->out_y);
	ALOGD("out_width = %x(%d)\n", t->out_width, t->out_width);
	ALOGD("out_height = %x(%d)\n", t->out_height, t->out_height);
	ALOGD("y_buf_v = %x\n", (unsigned int)t->y_buf_v);
	ALOGD("u_buf_v = %x\n", (unsigned int)t->u_buf_v);
	ALOGD("v_buf_v = %x\n", (unsigned int)t->v_buf_v);
	ALOGD("out_buf_v = %x\n", (unsigned int)t->out_buf_v);
	ALOGD("y_buf_p = %x\n", (unsigned int)t->y_buf_p);
	ALOGD("u_buf_p = %x\n", (unsigned int)t->u_buf_p);
	ALOGD("v_buf_p = %x\n", (unsigned int)t->v_buf_p);
	ALOGD("out_buf_p = %x\n", (unsigned int)t->out_buf_p);
	ALOGD("src_page_mapping = %x\n", (unsigned int)t->src_page_mapping);
	ALOGD("dst_page_mapping = %x\n", (unsigned int)t->dst_page_mapping);
	ALOGD("y_t_addr = %x\n", (unsigned int)t->y_t_addr);
	ALOGD("u_t_addr = %x\n", (unsigned int)t->u_t_addr);
	ALOGD("v_t_addr = %x\n", (unsigned int)t->v_t_addr);
	ALOGD("out_t_addr = %x\n", (unsigned int)t->out_t_addr);
	ALOGD("csc = %x\n", (unsigned int)t->csc);
	ALOGD("stride.y = %x\n", (unsigned int)t->stride.y);
	ALOGD("stride.u = %x\n", (unsigned int)t->stride.u);
	ALOGD("stride.v = %x\n", (unsigned int)t->stride.v);
	ALOGD("stride.out = %x\n", (unsigned int)t->stride.out);
	ALOGD("Wdiff = %x\n", (unsigned int)t->Wdiff);
	ALOGD("Hdiff = %x\n", (unsigned int)t->Hdiff);
	ALOGD("zoom_mode = %x\n", (unsigned int)t->zoom_mode);
	ALOGD("hcoef_real_heiht = %x\n", (unsigned int)t->hcoef_real_heiht);
	ALOGD("vcoef_real_heiht = %x\n", (unsigned int)t->vcoef_real_heiht);
	ALOGD("hoft_table = %x\n", (unsigned int)t->hoft_table);
	ALOGD("voft_table = %x\n", (unsigned int)t->voft_table);
	ALOGD("hcoef_table = %x\n", (unsigned int)t->hcoef_table);
	ALOGD("vcoef_table = %x\n", (unsigned int)t->vcoef_table);
	ALOGD("cube_hcoef_table = %x\n", (unsigned int)t->cube_hcoef_table);
	ALOGD("cube_vcoef_table = %x\n", (unsigned int)t->cube_vcoef_table);
	ALOGD("-----------------------------\n");
}

int isOsd2LayerMode(struct ipu_image_info *ipu_img)
{
	struct dest_data_info *dst;
	
	dst = &ipu_img->dst_info;

	if ((dst->dst_mode & IPU_OUTPUT_TO_LCD_FG1) && 
		!(dst->dst_mode & IPU_OUTPUT_MODE_FG0_OFF)) 
		return 1;
	
	return 0;
}

static int get_fbaddr_info(struct ipu_native_info *ipu_info)
{
	int ret = 0;
	int i;
	char fb_name[20];
	struct mvom_data_info **info;

	info = (struct mvom_data_info **)&ipu_info->fb_data;
	for (i = 0; i < 2; i++) {
		if (info[i] == NULL) {
			ALOGE("info not alloc mem yet!\n");
			return -1;
		}
		sprintf(fb_name, "/dev/graphics/fb%d", i);
		if ((info[i]->fbfd = open(fb_name, O_RDWR)) < 0) {
			ALOGE("ERR: can't open %s ++\n", fb_name);
			return -1;
		}
		ret = ioctl(info[i]->fbfd, FBIOGET_VSCREENINFO, &info[i]->fbvar);
		if (ret < 0) {
			ALOGE("Get %s var info failed, ret=%d\n", fb_name, ret);
			return -1;
		}
		if (i == 0) {
			ret = ioctl(info[i]->fbfd, JZFB_GET_LCDC_ID,
				    &ipu_info->fb0_lcdc_id);
			if (ret < 0) {
				ALOGE("%s get %s LCDC ID failed! fd: %d\n",
				     __func__, fb_name, info[i]->fbfd);
				return -1;
			}
		}
	}

	return 0;
}

static int mvom_lcd_init(struct ipu_native_info *ipu_info)
{
	int i;
	/* Alloc fb_data */
	for (i = 0; i < 2; i++) {
		if (NULL == ipu_info->fb_data[i]) {
			ipu_info->fb_data[i] = (struct mvom_data_info *)malloc(sizeof(struct mvom_data_info));
		}
	}

//	return get_fbaddr_info(ipu_info);
	return 0;
}

static int mvom_lcd_exit(struct ipu_native_info *ipu_info)
{
	int i;

	/* free fb_data */
	for (i = 0; i < 2; i++) {
		if (NULL != ipu_info->fb_data[i]) {
			/* close fbfd */
			int fd = ipu_info->fb_data[i]->fbfd;
			if (fd > 0) {
				close(fd);
			}
			free(ipu_info->fb_data[i]);
			ipu_info->fb_data[i] = NULL;
		}
	}

	return 0;
}

static int ipu_prepare_tables(struct ipu_native_info *ipu, int *outWp, int *outHp, int *Wdiff, int *Hdiff)
{
	int i;
//	int fb_index;
	int W = 0, H = 0, Hsel = 0, Wsel = 0;
	int srcN, dstM, width_up, height_up;
	int Height_lut_max, Width_lut_max;
	rsz_lut *h_lut;
	rsz_lut *v_lut;
	int *oft_table;
	int *coef_table;
	int way_sel;
	double CUBE_LEVEL;
	unsigned int rsize_w = 0, rsize_h = 0;
//	unsigned int fb_w, fb_h;
	struct ipu_img_param *img;
	struct Ration2m *ipu_ratio_table;

//	ALOGD("Enter: %s", __FUNCTION__);

	if (ipu == NULL) {
		return -1;
	}

//	fb_index = ipu->fb0_lcdc_id ? ipu->id : (!ipu->id & 0x1);
//	fb_w = ipu->fb_data[fb_index]->fbvar.xres;
//	fb_h = ipu->fb_data[fb_index]->fbvar.yres;
	ipu_ratio_table = ipu->ipu_ratio_table;
	img = &ipu->img;

	h_lut = ipu->h_lut;
	v_lut = ipu->v_lut;

	if (img->output_mode & IPU_OUTPUT_MODE_FG1_TVOUT) {
		rsize_w = img->out_width;
		rsize_h = img->out_height;
	} else if (img->output_mode & IPU_OUTPUT_TO_LCD_FG1) {
		{
			int x = img->out_x;
//			if ( x > (int)fb_w )
//				x = fb_w - 128;
			img->out_x = x;

			int w = img->out_width;
//			if ( w + x > (int)fb_w )
//				w = fb_w - x;
			img->out_width = w;

			int y = img->out_y;
//			if ( y > (int)fb_h )
//				y = fb_h - 128;
			img->out_y = y;

			int h = img->out_height;
//			if ( h + y > (int)fb_h )
//				h = fb_h - y;
			img->out_height = h;
		}
		rsize_w = img->out_width;
		rsize_h = img->out_height;
	} else {
		rsize_w = img->out_width;
		rsize_h = img->out_height;
	}

	*Wdiff = *Hdiff = 0;

	if ((img->in_width == rsize_w) && (img->in_height == rsize_h)) {
		img->out_height = *outHp = rsize_h;
		img->out_width = *outWp = rsize_w;
		return (0);
	}
	// init the resize factor table
	if (ipu->ipu_ratio_table_inited == 0) {
		init_ipu_ratio_table(ipu);
		ipu->ipu_ratio_table_inited = 1;
	}

	width_up = (rsize_w >= img->in_width);
	height_up = (rsize_h >= img->in_height);

	// get the resize factor
	if (W != (int)rsize_w) {
		Wsel = find_ipu_ratio_factor(ipu, (float)img->in_width / rsize_w, width_up);
		W = rsize_w;
	}
	if (H != (int)rsize_h) {
		Hsel = find_ipu_ratio_factor(ipu, (float)img->in_height / rsize_h, height_up);
		H = rsize_h;
	}

	// calculate out H and HLUT
	srcN = ipu_ratio_table[Wsel].n;
	dstM = ipu_ratio_table[Wsel].m;
	resize_lut_cal(srcN, dstM, width_up, h_lut);

	// calculate out W and VLUT
	srcN = ipu_ratio_table[Hsel].n;
	dstM = ipu_ratio_table[Hsel].m;
	resize_lut_cal(srcN, dstM, height_up, v_lut);

	/* calculate coef, oft, hcoef_real_heiht and vcoef_real_heiht*/
	Width_lut_max = width_up ? ipu_ratio_table[Wsel].m : ipu_ratio_table[Wsel].n;
	Height_lut_max = height_up ? ipu_ratio_table[Hsel].m : ipu_ratio_table[Hsel].n;

	caculate_h_lut(ipu, Width_lut_max);
	caculate_v_lut(ipu, Height_lut_max);

	//CUBE_LEVEL can: -2, -1, -0.75, -0.5
	CUBE_LEVEL = -1;
	if (img->zoom_mode) {
		if (img->zoom_mode == ZOOM_MODE_BICUBE) {
			way_sel = 0;
		} else
			way_sel = 1;
		
		if(ipu->sinxdivx_table_inited == 0) {
			memset((void*)ipu->sinxdivx_table_8, 0, sizeof(ipu->sinxdivx_table_8));
			_inti_sinxdivx_table(ipu, CUBE_LEVEL);
			ipu->sinxdivx_table_inited = 1;
		}

		caculate_cube_coef_table(ipu, way_sel);
	}

//	ALOGD("Exit: %s", __FUNCTION__);
	return (0);
}

static int jz47_ipu_fg1_init(struct ipu_native_info * ipu)
{
	int ret;
	struct ipu_img_param *img;
	
	if (ipu == NULL) {
		ALOGD("ipu is NULL!\n");
		return -1;
	}
	
	img = &ipu->img;

	if (img->output_mode & IPU_OUTPUT_MODE_FG1_TVOUT) {
		ipu->fb_data[ipu->id]->fb_width = ipu->fb_data[ipu->id]->fb_osd.fg1.w; 
		ipu->fb_data[ipu->id]->fb_height = ipu->fb_data[ipu->id]->fb_osd.fg1.h;
		img->out_x = ipu->fb_data[ipu->id]->fb_osd.fg1.x;
		img->out_y = ipu->fb_data[ipu->id]->fb_osd.fg1.y;
		img->out_width = ipu->fb_data[ipu->id]->fb_osd.fg1.w;
		img->out_height = ipu->fb_data[ipu->id]->fb_osd.fg1.h;
	} else {
		ipu->fb_data[ipu->id]->fb_width = ipu->fb_data[ipu->id]->fb_osd.fg1.w; 
		if (ipu->fb_data[ipu->id]->fb_osd.fg1.h <= ipu->fb_data[ipu->id]->fb_osd.fg0.h) {
			ipu->fb_data[ipu->id]->fb_width = ipu->fb_data[ipu->id]->fb_osd.fg1.h;
		} else { 
			ipu->fb_data[ipu->id]->fb_width = ipu->fb_data[ipu->id]->fb_osd.fg0.h;
		}
		
		if (ipu->fb_data[ipu->id]->fb_osd.fg0.x > 0) {
			img->out_x = img->out_x + ipu->fb_data[ipu->id]->fb_osd.fg0.x;
		}
		if (ipu->fb_data[ipu->id]->fb_osd.fg0.y > 0) {
			img->out_y = img->out_y + ipu->fb_data[ipu->id]->fb_osd.fg0.y;
		}

		if (img->out_x > ipu->fb_data[ipu->id]->fb_width || 
			img->out_y > ipu->fb_data[ipu->id]->fb_height) {
			ALOGD("*** jz47xx ipu dest rect out range error.\n");
			return -1;
		}
		
		if (img->out_x + img->out_width > ipu->fb_data[ipu->id]->fb_width)
			img->out_width = ipu->fb_data[ipu->id]->fb_width - img->out_x;
		if (img->out_y + img->out_height > ipu->fb_data[ipu->id]->fb_height)
			img->out_height = ipu->fb_data[ipu->id]->fb_height - img->out_y;		
	}
		
	return 0;
}

static int jz47_ipu_init(struct ipu_image_info *ipu_img)
{
	int ret = 0;

//	ALOGD("Enter: %s", __FUNCTION__);
	if (ipu_img == NULL) {
		ALOGD("ipu_img is NULL\n");
		return -1;
	}

	struct source_data_info * src = &ipu_img->src_info;
	struct dest_data_info *dst = &ipu_img->dst_info;
	struct ipu_native_info * ipu = (struct ipu_native_info*)ipu_img->native_data;
	struct ipu_img_param *img = &ipu->img;

	img->in_width = src->width;
	img->in_height = src->height;

	img->out_x = dst->left;
	img->out_y = dst->top;
	img->out_width = dst->width;
	img->out_height = dst->height;
	img->in_fmt = src->fmt;
	img->out_fmt = dst->fmt;
	//img->lcdc_id = dst->lcdc_id; /* no used */

	img->in_bpp = 16;
	img->stride.y = src->srcBuf.y_stride;
	img->stride.u = src->srcBuf.u_stride;
	img->stride.v = src->srcBuf.v_stride;
	img->stride.out = dst->dstBuf.y_stride;

	img->output_mode = dst->dst_mode;
	img->alpha = dst->alpha;
	img->colorkey = dst->colorkey;
	img->src_page_mapping = 1;
	img->dst_page_mapping = 1;
	img->stlb_base = src->stlb_base;
	img->dtlb_base = dst->dtlb_base;
	if (img->in_height/img->out_height > 3) {
		img->zoom_mode = ZOOM_MODE_BILINEAR; /* 0 bilinear, 1 bicube, 2 enhance bilinear*/
	} else {	
		img->zoom_mode = ZOOM_MODE_BICUBE; /* 0 bilinear, 1 bicube, 2 enhance bilinear*/
	}
	ipu_prepare_tables(ipu, (int *)&img->out_width, (int *)&img->out_height, &img->Wdiff, &img->Hdiff);
	gen_pic_enhance_table(ipu, 128);

	jz47_dump_ipu_resize_table(ipu, -1); /* cube coef*/
	jz47_dump_ipu_resize_table(ipu, -3); /* linear oft coed */
	
	img->hoft_table = ipu->hoft_table;
	img->voft_table = ipu->voft_table;

	img->cube_hcoef_table = (void*)ipu->cube_hcoef_table;
	img->cube_vcoef_table = (void*)ipu->cube_vcoef_table;

	img->hcoef_table = ipu->hcoef_table;
	img->vcoef_table = ipu->vcoef_table;

	img->pic_enhance_table = ipu->pic_enhance_table;

//	dump_ipu_img_param(img);

	if (img->output_mode & IPU_OUTPUT_TO_LCD_FG1) {
//		ALOGD("mg->output_mode & IPU_OUTPUT_TO_LCD_FG1\n");
		if (!(img->output_mode & IPU_OUTPUT_MODE_FG0_OFF)) {
			/* enable compress decompress mode */
			//ret = ioctl();
		}
		//ret = jz47_ipu_fg1_init(ipu);
		if (ret < 0) {
			ALOGD("jz47_ipu_fg1_init failed\n");
			return -1;
		}
	} else {
		/*
		 * IPU write back to buffer need to get the ipu bus first. And the bus is 
		 *  control by register LCDC_DUAL_CTRL (bit 8) of LCDC 1.
		 *  Default: IPU controller 0 get the bus.
		 */
		int ipu_to_buf = ipu->id ? 1 : 0;
//		int fb_index;
		//ALOGE("++++ipu->id: %d, ipu_to_buf: %d, fb0_lcdc_id=%d", ipu->id, ipu_to_buf,ipu->fb0_lcdc_id);

//		fb_index = ipu->fb0_lcdc_id ? 0 : 1;
//		ret = ioctl(ipu->fb_data[fb_index]->fbfd, JZFB_IPU0_TO_BUF, &ipu_to_buf);
		ret = ioctl(ipu->ipu_fd,IOCTL_IPU0_TO_BUF, &ipu_to_buf);
		if (ret < 0) {
			ALOGD("enable IPU %d to buf failed\n", ipu->id);
			return -1;
		}
	}

	ret = ioctl(ipu->ipu_fd, IOCTL_IPU_INIT, (void *)img);
	if (ret < 0) {
		ALOGD("ipu first init failed\n");
		return -1;
	}

	ipu->state &= ~IPU_STATE_STARTED;

//	ALOGD("Exit: %s", __FUNCTION__);

	return ret;
}

static int jz47_put_image(struct ipu_image_info *ipu_img, struct ipu_data_buffer *data_buf)
{
	int ret;

//	ALOGD("Enter: %s", __FUNCTION__);
	if (ipu_img == NULL) {
		ALOGD("ipu_img is NULL\n");
		return -1;
	}

	struct source_data_info * src = &ipu_img->src_info;
	struct dest_data_info *dst = &ipu_img->dst_info;
	struct ipu_native_info * ipu = (struct ipu_native_info*)ipu_img->native_data;
	struct ipu_img_param * img = &ipu->img;


	if (data_buf) {
		img->y_buf_p = WORDALIGN(data_buf->y_buf_phys);
		img->u_buf_p = WORDALIGN(data_buf->u_buf_phys);
		img->v_buf_p = WORDALIGN(data_buf->v_buf_phys);
		img->stride.y = WORDALIGN(data_buf->y_stride);
		img->stride.u = WORDALIGN(data_buf->u_stride);
		img->stride.v = WORDALIGN(data_buf->v_stride);
	} else {
		img->y_buf_p = WORDALIGN(src->srcBuf.y_buf_phys);
		img->u_buf_p = WORDALIGN(src->srcBuf.u_buf_phys);
		img->v_buf_p = WORDALIGN(src->srcBuf.v_buf_phys);

		img->y_buf_v = (unsigned char *)WORDALIGN((unsigned int)src->srcBuf.y_buf_v);
		img->u_buf_v = (unsigned char *)WORDALIGN((unsigned int)src->srcBuf.u_buf_v);
		img->v_buf_v = (unsigned char *)WORDALIGN((unsigned int)src->srcBuf.v_buf_v);

		img->stride.y = WORDALIGN(src->srcBuf.y_stride);
		img->stride.u = WORDALIGN(src->srcBuf.u_stride);
		img->stride.v = WORDALIGN(src->srcBuf.v_stride);
		if (!(img->output_mode & IPU_OUTPUT_TO_LCD_FG1)) {
			img->out_buf_p = WORDALIGN(dst->dstBuf.y_buf_phys);
			img->out_buf_v = (unsigned char *)WORDALIGN((unsigned int)dst->out_buf_v);
			img->stride.out = (dst->dstBuf.y_stride);
		}
	}

	ALOGV("img->y_buf_v: %08x, img->u_buf_v: %08x, img->v_buf_v: %08x, img->stride.y: %d, img->stride.u: %d, img->stride.v: %d", (unsigned int)img->y_buf_v, (unsigned int)img->u_buf_v, (unsigned int)img->v_buf_v, img->stride.y, img->stride.u, img->stride.v);

	ret = ioctl(ipu->ipu_fd, IOCTL_IPU_SET_BUFF, img);
	if (ret < 0) {
		ALOGD("jz47_put_image driver_ioctl(ipu, IOCTL_IPU_SET_BUFF) ret err=%d", ret);
		return ret;
	}

//	ALOGD("Exit: %s", __FUNCTION__);
	return 0;
}

int alloc_img_info(struct ipu_image_info **ipu_img_p)
{
	struct ipu_image_info *ipu_img;
	struct ipu_native_info *ipu;
	struct ipu_img_param *img;

	ipu_img = *ipu_img_p;
	if (ipu_img == NULL) {
		ipu_img = (struct ipu_image_info *)calloc(sizeof(struct ipu_image_info) + 
												  sizeof(struct ipu_native_info), sizeof(char));
		if (ipu_img == NULL) {
			ALOGE("ipu_open() no mem.\n");
			return -1;
		}
		ipu_img->native_data = (void*)((struct ipu_image_info *)ipu_img + 1);
		*ipu_img_p = ipu_img;
	}

	ipu = (struct ipu_native_info *)ipu_img->native_data;
	img = &ipu->img;
	img->version = sizeof(struct ipu_img_param);
	mvom_lcd_init(ipu);
	
	return 0;
}

static void free_img_info(struct ipu_image_info **ipu_img_p)
{
	struct ipu_image_info *ipu_img;
	struct ipu_native_info *ipu;

	ipu_img = *ipu_img_p;
	if (ipu_img == NULL) {
		ALOGE("ipu_img is NULL+++++");
		return;
	}

	ipu = (struct ipu_native_info *)ipu_img->native_data;
	mvom_lcd_exit(ipu);
	free(ipu_img);
	ipu_img = NULL;
	*ipu_img_p = ipu_img;

	return;
}

void set_open_state(struct ipu_native_info *ipu, int fd)
{
	ipu->ipu_fd = fd;
	ipu->state = IPU_STATE_OPENED;
}

int open_ipu1(struct ipu_image_info **ipu_img_p)
{
	int fd, ret;
//	int fb_index;
	int clk_en = 0;
	struct ipu_native_info *ipu;
	struct dest_data_info *dst;

	if (ipu_img_p == NULL) {
		ALOGE("%s ipu_img_p is NULL\n", __func__);
		return -1;
	}
	ret = alloc_img_info(ipu_img_p);
	if (ret < 0) {
		ALOGE("%s alloc_img_info failed!\n", __func__);
		return -1;
	}
	ipu = (struct ipu_native_info *)(* ipu_img_p)->native_data;
	dst = &((*ipu_img_p)->dst_info);
	
	if ((fd = open(IPU1_DEVNAME, O_RDONLY)) < 0) {
		ALOGE("%s open ipu1 failed! fd: %d\n", __func__, fd);
		return fd;
	}

	if ((ret = ioctl(fd, IOCTL_IPU_SET_BYPASS)) < 0) {
		ALOGE("%s IOCTL_IPU_SET_BYPASS failed", __func__);
		fd = ret;
		return ret;
	}
	set_open_state(ipu, fd);
	//android_atomic_inc(&ipu1_open_cnt);
	ipu1_open_cnt++;
	ipu->id = 1;
//	fb_index = ipu->fb0_lcdc_id ? 1 : 0;

	clk_en = 1;
	//ret = ioctl(ipu->fb_data[fb_index]->fbfd, JZFB_ENABLE_IPU_CLK, &clk_en);
	ret = ioctl(ipu->ipu_fd, IOCTL_IPU_ENABLE_CLK, &clk_en);
	if (ret < 0) {
		ALOGD("enable IPU %d clock failed\n", ipu->id);
		return -1;
	}

	return fd;
}

/* alloc a ipu_image_info structure */
/*
 * Note:
 * "/dev/ipu0" == IPU 1; "/dev/ipu1" == IPU 0
 *
 * fb0_lcdc_id store the LCDC ID of "/dev/graphics/fb0".
 * fb0_lcdc_id == 1:
 * "/dev/graphics/fb0" == LCDC 1; "/dev/graphics/fb1" == LCDC 0
 *
 * fb0_lcdc_id == 0:
 * "/dev/graphics/fb0" == LCDC 0; "/dev/graphics/fb1" == LCDC 1
 */
int ipu_open(struct ipu_image_info **ipu_img_p)
{
	int ret, fd;
//	int fb_index;
	int clk_en = 0;
	struct ipu_native_info *ipu;
	struct dest_data_info *dst;

	ALOGV("Enter: %s", __FUNCTION__);
	if (ipu_img_p == NULL) {
		ALOGD("ipu_img_p is NULL\n");
		return -1;
	}

	ret = alloc_img_info(ipu_img_p);
	if (ret < 0) {
		ALOGD("alloc_img_info failed!\n");
		return -1;
	}
	ipu = (struct ipu_native_info *)(* ipu_img_p)->native_data;
	dst = &((*ipu_img_p)->dst_info);
	if ((fd = open(IPU0_DEVNAME, O_RDONLY)) < 0) {
		ALOGD("open ipu0 failed!\n");
		free_img_info(ipu_img_p);
		return -1;
	}

	if ((ret = ioctl(fd, IOCTL_IPU_SET_BYPASS)) < 0) {
		close(fd);
		if ((fd = open(IPU1_DEVNAME, O_RDONLY)) < 0) {
			ALOGD("open ipu1 failed!\n");
			free_img_info(ipu_img_p);
			return -1;
		}
		if ((ret = ioctl(fd, IOCTL_IPU_SET_BYPASS)) < 0) {
			ALOGD("IOCTL_IPU_SET_BYPASS failed! no free ipu\n");
			free_img_info(ipu_img_p);
			return -1;
		}

//		LOGE("--------open---ipu1");
		ipu->id = 1;
//		fb_index = ipu->fb0_lcdc_id ? 1 : 0;
		//android_atomic_inc(&ipu1_open_cnt);
		ipu1_open_cnt++;
	} else {
//		LOGE("--------open---ipu0");
		ipu->id = 0;
//		fb_index = ipu->fb0_lcdc_id ? 0 : 1;
		//android_atomic_inc(&ipu0_open_cnt);
		ipu0_open_cnt++;
	}
	
	set_open_state(ipu, fd);

	clk_en = 1;
	//ret = ioctl(ipu->fb_data[fb_index]->fbfd, JZFB_ENABLE_IPU_CLK, &clk_en);
	ret = ioctl(ipu->ipu_fd, IOCTL_IPU_ENABLE_CLK, &clk_en);
	if (ret < 0) {
		ALOGD("enable IPU %d clock failed\n", ipu->id);
		free_img_info(ipu_img_p);
		return -1;
	}

//	ALOGD("ipu_img addr= %p", ipu_img);
	ALOGV("Exit: %s", __FUNCTION__);

	return fd;
}

int ipu_init(struct ipu_image_info * ipu_img)
{
	int ret;
	struct ipu_native_info *ipu;
	struct ipu_img_param *img;

	ALOGV("Enter: %s", __FUNCTION__);
	if (ipu_img == NULL)
		return -1;

	ipu = (struct ipu_native_info *)ipu_img->native_data;
	img = &ipu->img;

	if (ipu->state & IPU_STATE_STARTED) {
		//ALOGD("RESET IPU, STOP IPU FIRST.");
		//ipu_stop(ipu_img);
	}

	ret = jz47_ipu_init(ipu_img);
	if (ret < 0) {
		ALOGD("jz47_ipu_init() failed. ");
		return -1;
	}

	ipu->state |= IPU_STATE_INITED;

	ALOGV("Exit: %s", __FUNCTION__);
	return ret;
}

int ipu_start(struct ipu_image_info * ipu_img)
{
	int ret;
	ALOGV("Enter: %s", __FUNCTION__);

	if (ipu_img == NULL) {
		ALOGD("ipu_img is NULL\n");
		return -1;
	}

	struct ipu_native_info * ipu = (struct ipu_native_info *)ipu_img->native_data;
	struct ipu_img_param * img = &ipu->img;

//	dump_ipu_img_param(img);
#ifdef IPU_DBG
	ret = ioctl(ipu->ipu_fd, IOCTL_IPU_DUMP_REGS, img);
	if (ret < 0) {
		ALOGD("IOCTL_IPU_DUMP_REGS failed\n");
		return -1;
	}
#endif

	ret = ioctl(ipu->ipu_fd, IOCTL_IPU_START, img);
	if (ret < 0) {
		ALOGD("IOCTL_IPU_START failed\n");
		return -1;
	}

	ipu->state &= ~IPU_STATE_STOPED;
	ipu->state |= IPU_STATE_STARTED;
	ALOGV("Exit: %s", __FUNCTION__);

	return ret;
}

int ipu_stop(struct ipu_image_info * ipu_img)
{
	int ret;

	if (ipu_img == NULL) {
		ALOGD("ipu_img is NULL!\n");
		return -1;
	}

	struct ipu_native_info *ipu = (struct ipu_native_info *)ipu_img->native_data;
	struct ipu_img_param *img = &ipu->img;

	if (!(ipu->state & IPU_STATE_STARTED)) {
		ALOGD("ipu not started\n");
		return 0;
	}

	ret = ioctl(ipu->ipu_fd, IOCTL_IPU_STOP, img);
	if (ret < 0) {
		ALOGD("IOCTL_IPU_STOP failed!\n");
		return -1;
	}

	ipu->state &= ~IPU_STATE_STARTED;
	ipu->state |= IPU_STATE_STOPED;

	return ret;
}

int ipu_close(struct ipu_image_info ** ipu_img_p)
{
	int ret;
//	int fb_index;
	int clk_en = 0;
	int ipu_to_buf = 0;
	struct ipu_image_info *ipu_img;
	struct ipu_native_info *ipu;
	struct ipu_img_param *img;

	ALOGV("Enter: %s", __FUNCTION__);

	if (ipu_img_p == NULL) {
		ALOGD("ipu_img_p is NULL\n");
		return -1;
	}

	ipu_img = *ipu_img_p;
	if (ipu_img == NULL) {
		ALOGD("ipu_img is NULL\n");
		return -1;
	}

	ipu = (struct ipu_native_info *)ipu_img->native_data;
	img = &ipu->img;

	if (ipu->state == IPU_STATE_CLOSED) {
		ALOGD("ipu already closed!\n");
		free_img_info(ipu_img_p);
		return -1;
	}

	/* close IPU and LCD controller */
	ipu_stop(ipu_img);

	ret = ioctl(ipu->ipu_fd, IOCTL_IPU_CLR_BYPASS);
	if (ret < 0) {
		ALOGE("IOCTL_IPU_CLR_BYPASS failed!!!");
		free_img_info(ipu_img_p);
		return -1;
	}

	if (1 == ipu->id) {
		if (ipu1_open_cnt) {
			//android_atomic_dec(&ipu1_open_cnt);
			ipu1_open_cnt++;
		}
//		fb_index = ipu->fb0_lcdc_id ? 0 : 1;
		//ret = ioctl(ipu->fb_data[fb_index]->fbfd, JZFB_IPU0_TO_BUF, &ipu_to_buf);
		ret = ioctl(ipu->ipu_fd,IOCTL_IPU0_TO_BUF, &ipu_to_buf);
		if (ret < 0) {
			ALOGD("enable IPU %d to buf failed\n", ipu->id);
			free_img_info(ipu_img_p);
			return -1;
		}
//		fb_index = ipu->fb0_lcdc_id ? 1 : 0;
		/* If IPU1 is idle then Close the IPU clock */
 		if (!ipu1_open_cnt) {
		//	ret = ioctl(ipu->fb_data[fb_index]->fbfd, JZFB_ENABLE_IPU_CLK, &clk_en);
			ret = ioctl(ipu->ipu_fd, IOCTL_IPU_ENABLE_CLK, &clk_en);
			if (ret < 0) {
				ALOGD("Disable IPU %d clock failed\n", ipu->id);
				free_img_info(ipu_img_p);
				return -1;
			}
		}
	} else {
		if (ipu0_open_cnt) {
			//android_atomic_dec(&ipu0_open_cnt);
			ipu0_open_cnt++;
		}
//		fb_index = ipu->fb0_lcdc_id ? 0 : 1;
		/* If IPU0 is idle then Close the IPU clock */
 		if (!ipu0_open_cnt) {
//			ret = ioctl(ipu->fb_data[fb_index]->fbfd, JZFB_ENABLE_IPU_CLK, &clk_en);
			ret = ioctl(ipu->ipu_fd, IOCTL_IPU_ENABLE_CLK, &clk_en);
			if (ret < 0) {
				ALOGD("Disable IPU %d clock failed\n", ipu->id);
				free_img_info(ipu_img_p);
				return -1;
			}
		}
	}

	ipu->state = IPU_STATE_CLOSED;
	free_img_info(ipu_img_p);

	if (ipu->ipu_fd)
		close(ipu->ipu_fd);

	ALOGV("Exit: %s", __FUNCTION__);
	return ret;
}

/* post video frame(YUV buffer) to ipu */
int ipu_postBuffer(struct ipu_image_info* ipu_img)
{
	int ret;

	if (ipu_img == NULL) {
		ALOGD("ipu_img is NULL\n");
		return -1;
	}

	ret = jz47_put_image(ipu_img, NULL);
	if (ret < 0) {
		ALOGE("jz47_put_image ret error=%d, return", ret);
		return ret;
	}

	struct ipu_native_info * ipu = (struct ipu_native_info*)ipu_img->native_data;
	struct ipu_img_param * img = &ipu->img;

//	ALOGD("%d ---- %d\n", img->output_mode&IPU_OUTPUT_TO_LCD_FG1, ipu->state&IPU_STATE_STARTED);
	if (!(img->output_mode & IPU_OUTPUT_TO_LCD_FG1) || 
		!(ipu->state & IPU_STATE_STARTED)) {
		ret = ipu_start(ipu_img);
		if (ret < 0) {
			ALOGD("ipu_start failed!");
			return ret;
		}
	}

	return ret;
}
