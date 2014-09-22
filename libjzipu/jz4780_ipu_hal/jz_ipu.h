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

#ifndef __ANDROID_JZ4780_IPU_H__
#define __ANDROID_JZ4780_IPU_H__

#include <linux/fb.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(ANDROID)
#define ALOGE(sss, aaa...) do {         \
        printf(sss"\n", ##aaa);     \
    } while (0)
    
#define ALOGD(sss, aaa...) do {         \
        printf(sss"\n", ##aaa);     \
    } while (0)
   
#define ALOGV(sss, aaa...) do {         \
        /*printf(sss"\n", ##aaa);*/ \
    } while (0)
   
#endif

#define IPU_FB0DEV_NAME "/dev/graphics/fb0"
#define IPU_FB1DEV_NAME "/dev/graphics/fb1"
#define IPU0_DEVNAME    "/dev/ipu0"
#define IPU1_DEVNAME    "/dev/ipu1"

#define IPU_STATE_OPENED           0x00000001
#define IPU_STATE_CLOSED           0x00000002
#define IPU_STATE_INITED           0x00000004
#define IPU_STATE_STARTED          0x00000008
#define IPU_STATE_STOPED           0x00000010

/* ipu output mode */
#define IPU_OUTPUT_TO_LCD_FG1           0x00000002
#define IPU_OUTPUT_TO_LCD_FB0           0x00000004
#define IPU_OUTPUT_TO_LCD_FB1           0x00000008
#define IPU_OUTPUT_TO_FRAMEBUFFER       0x00000010 /* output to user defined buffer */
#define IPU_OUTPUT_MODE_MASK            0x000000FF
#define IPU_DST_USE_COLOR_KEY           0x00000100
#define IPU_DST_USE_ALPHA               0x00000200
#define IPU_OUTPUT_BLOCK_MODE           0x00000400
#define IPU_OUTPUT_MODE_FG0_OFF         0x00000800
#define IPU_OUTPUT_MODE_FG1_TVOUT       0x00001000
#define IPU_DST_USE_PERPIXEL_ALPHA      0x00010000

enum {
	ZOOM_MODE_BILINEAR = 0,
	ZOOM_MODE_BICUBE,
	ZOOM_MODE_BILINEAR_ENHANCE,
};

#define IPU_LUT_LEN                      (32)

struct YuvCsc
{									// YUV(default)	or	YCbCr
	unsigned int csc0;				//	0x400			0x4A8
	unsigned int csc1;              //	0x59C   		0x662
	unsigned int csc2;              //	0x161   		0x191
	unsigned int csc3;              //	0x2DC   		0x341
	unsigned int csc4;              //	0x718   		0x811
	unsigned int chrom;             //	128				128
	unsigned int luma;              //	0				16
};

struct YuvStride
{
	unsigned int y;
	unsigned int u;
	unsigned int v;
	unsigned int out;
};

struct ipu_img_param
{
	unsigned int 		fb_w;
	unsigned int 		fb_h;
	unsigned int 		version;			/* sizeof(struct ipu_img_param) */
	int			        ipu_cmd;			/* IPU command by ctang 2012/6/25 */
	unsigned int        stlb_base;      /* IPU src tlb table base phys */
	unsigned int        dtlb_base;      /* IPU dst tlb table base phys */
	int                 lcdc_id;        /* no used */
	unsigned int		output_mode;		/* IPU output mode: fb0, fb1, fg1, alpha, colorkey and so on */
	unsigned int		alpha;
	unsigned int		colorkey;
	unsigned int		in_width;
	unsigned int		in_height;
	unsigned int		in_bpp;
	unsigned int		out_x;
	unsigned int		out_y;
	unsigned int		in_fmt;
	unsigned int		out_fmt;
	unsigned int		out_width;
	unsigned int		out_height;
	unsigned char*		y_buf_v;            /* Y buffer virtual address */
	unsigned char*		u_buf_v;
	unsigned char*		v_buf_v;
	unsigned int		y_buf_p;            /* Y buffer physical address */
	unsigned int		u_buf_p;
	unsigned int		v_buf_p;
	unsigned char*		out_buf_v;
	unsigned int		out_buf_p;
	unsigned int 		src_page_mapping;
	unsigned int 		dst_page_mapping;
	unsigned char*		y_t_addr;				// table address
	unsigned char*		u_t_addr;
	unsigned char*		v_t_addr;
	unsigned char*		out_t_addr;
	struct YuvCsc*		csc;
	struct YuvStride	stride;
	int 			    Wdiff;
	int 			    Hdiff;
	unsigned int		zoom_mode;
	int 			    hcoef_real_heiht;
	int 			    vcoef_real_heiht;
	int*			    hoft_table;
	int* 			    voft_table;
	int*			    hcoef_table;
	int*			    vcoef_table;
	void*			    cube_hcoef_table;
	void*			    cube_vcoef_table;
	int*                pic_enhance_table;
};

enum format_order {
	FORMAT_X8R8G8B8  =  1,
	FORMAT_X8B8G8R8  =  2,
};

struct jzlcd_fg_t {
	int fg;            /* 0, fg0  1, fg1 */
	int bpp;	/* foreground bpp */
	int x;		/* foreground start position x */
	int y;		/* foreground start position y */
	int w;		/* foreground width */
	int h;		/* foreground height */
	unsigned int alpha;     /* ALPHAEN, alpha value */
	unsigned int bgcolor;   /* background color value */
};

struct jzfb_osd_t {
	enum format_order fmt_order;  /* pixel format order */
	int decompress;			      /* enable decompress function, used by FG0 */
	int useIPUasInput;			  /* useIPUasInput, used by FG1 */

	unsigned int osd_cfg;	      /* OSDEN, ALHPAEN, F0EN, F1EN, etc */
	unsigned int osd_ctrl;	      /* IPUEN, OSDBPP, etc */
	unsigned int rgb_ctrl;	      /* RGB Dummy, RGB sequence, RGB to YUV */
	unsigned int colorkey0;	      /* foreground0's Colorkey enable, Colorkey value */
	unsigned int colorkey1;       /* foreground1's Colorkey enable, Colorkey value */
	unsigned int ipu_restart;     /* IPU Restart enable, ipu restart interval time */

	int line_length;              /* line_length, stride, in byte */

	struct jzlcd_fg_t fg0;
	struct jzlcd_fg_t fg1;	
};

struct mvom_data_info {
	int fbfd;
	unsigned int fb_width;
	unsigned int fb_height;
//	void *fb_v_base;			    /* virtual base address of frame buffer */
//	unsigned int fb_p_base;			/* physical base address of frame buffer */
	struct jzfb_osd_t fb_osd;
	struct fb_var_screeninfo fbvar;
//	struct fb_fix_screeninfo fbfix;
};

struct ipu_data_buffer {
	int size;				        /* buffer size */
	void *y_buf_v;				    /* virtual address of y buffer or base address */
	void *u_buf_v;
	void *v_buf_v;
	unsigned int y_buf_phys;		/* physical address of y buffer */
	unsigned int u_buf_phys;
	unsigned int v_buf_phys;
	int y_stride;
	int u_stride;
	int v_stride;
};

typedef struct {
   unsigned int coef;
   unsigned short int in_n;
   unsigned short int out_n;
} rsz_lut;

struct Ration2m
{
  float ratio;
  int n, m;
};

struct host_ipu_table
{
	int cnt;
	struct Ration2m ipu_ratio_table[IPU_LUT_LEN*IPU_LUT_LEN];
};

struct ipu_native_info {
	void *ipu_buf_base;
	int ipu_fd;
	int id;
	int fb0_lcdc_id;
	struct ipu_img_param img; /* Interface to ipu driver */
	struct mvom_data_info *fb_data[2];
	int ipu_inited;
	unsigned int state;

	/* ipu frame buffers */
	int data_buf_num;	/* 3 buffers in most case */
	struct ipu_data_buffer data_buf[3];

	int rtable_len;
	struct Ration2m ipu_ratio_table[IPU_LUT_LEN*IPU_LUT_LEN];
	rsz_lut h_lut[IPU_LUT_LEN];
	rsz_lut v_lut[IPU_LUT_LEN];

	/* jz4760's ipu HRSZ_COEF_LUT*/
	int hoft_table[IPU_LUT_LEN+1]; 
	int hcoef_table[IPU_LUT_LEN+1];

	/* jz4760's ipu VRSZ_COEF_LUT*/
	int voft_table[IPU_LUT_LEN+1];
	int vcoef_table[IPU_LUT_LEN+1];
	int pic_enhance_table[256];

	int cube_hcoef_table[32][4];
	int cube_vcoef_table[32][4];
	int sinxdivx_table_8[(2<<9)+1];
	int ipu_ratio_table_inited;
	int sinxdivx_table_inited;
};

/* Source element */
struct source_data_info
{
	/* pixel format definitions in hardware/libhardware/include/hardware/hardware.h */
	int fmt;
	int width;
	int height;
	int is_virt_buf; /* virtual memory or continuous memory */
	unsigned int stlb_base;
	struct ipu_data_buffer srcBuf; /* data, stride */
};


/* Destination element */
struct dest_data_info
{
	unsigned int dst_mode;		/* ipu output to lcd's fg0 or fg1, ipu output to fb0 or fb1 */
	int fmt;			/* pixel format definitions in hardware/libhardware/include/hardware/hardware.h */
	unsigned int colorkey;		/* JZLCD FG0 colorkey */
	unsigned int alpha;		/* JZLCD FG0 alpha */
	int left;
	int top;
	int width;
	int height;
	int lcdc_id; /* no need to config it */
	unsigned int dtlb_base;
	void *out_buf_v;
	struct ipu_data_buffer dstBuf; /* data, stride */
};

struct ipu_image_info
{
	struct source_data_info src_info;
	struct dest_data_info dst_info;
	void * native_data;
};

#define JZIPU_IOC_MAGIC  'I'

/* ioctl command */
#define	IOCTL_IPU_SHUT               _IO(JZIPU_IOC_MAGIC, 102)
#define IOCTL_IPU_INIT               _IOW(JZIPU_IOC_MAGIC, 103, struct ipu_img_param)
#define IOCTL_IPU_SET_BUFF           _IOW(JZIPU_IOC_MAGIC, 105, struct ipu_img_param)
#define IOCTL_IPU_START              _IO(JZIPU_IOC_MAGIC, 106)
#define IOCTL_IPU_STOP               _IO(JZIPU_IOC_MAGIC, 107)
#define IOCTL_IPU_DUMP_REGS          _IO(JZIPU_IOC_MAGIC, 108)
#define IOCTL_IPU_SET_BYPASS         _IO(JZIPU_IOC_MAGIC, 109)
#define IOCTL_IPU_GET_BYPASS_STATE   _IOR(JZIPU_IOC_MAGIC, 110, int)
#define IOCTL_IPU_CLR_BYPASS         _IO(JZIPU_IOC_MAGIC, 111)
#define IOCTL_IPU_ENABLE_CLK         _IO(JZIPU_IOC_MAGIC, 112)
#define IOCTL_IPU0_TO_BUF            _IO(JZIPU_IOC_MAGIC, 113)
//#define IOCTL_GET_FREE_IPU       _IOR(JZIPU_IOC_MAGIC, 109, int)

/* ioctl commands to control LCD controller registers */
#define JZFB_GET_LCDC_ID	       	_IOR('F', 0x121, int)
#define JZFB_IPU0_TO_BUF		_IOW('F', 0x128, int)
#define JZFB_ENABLE_IPU_CLK		_IOW('F', 0x129, int)
#define JZFB_ENABLE_LCDC_CLK		_IOW('F', 0x130, int)

int ipu_open(struct ipu_image_info ** ipu_img); /* get a "struct ipu_image_info" handler*/
int open_ipu1(struct ipu_image_info ** ipu_img); /* get a "struct ipu_image_info" handler*/
int ipu_close(struct ipu_image_info ** ipu_img);
int ipu_init(struct ipu_image_info * ipu_img);
int ipu_start(struct ipu_image_info * ipu_img); /* Block if IPU_OUTPUT_BLOCK_MODE set */
int ipu_stop(struct ipu_image_info * ipu_img);
/* post video frame(YUV buffer) to ipu and start ipu, Block if IPU_OUTPUT_BLOCK_MODE set */
int ipu_postBuffer(struct ipu_image_info* ipu_img);
int isOsd2LayerMode(struct ipu_image_info * ipu_img);

#ifdef __cplusplus
}
#endif

#endif  //__ANDROID_JZ4780_IPU_H__
