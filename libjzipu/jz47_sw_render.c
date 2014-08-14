
#include <libjzcommon/com_config.h>

//#if defined(USE_SW_COLOR_CONVERT_RENDER)
#if 1

#include <stdio.h>
#include <stdlib.h>
#include "fcntl.h"
#include "unistd.h"
#include "sys/ioctl.h"
#include <linux/fb.h>

#include "libmpcodecs/vf.h"
//libmpcodecs/vf.h
#include "libavcodec/avcodec.h"
#include "libmpcodecs/img_format.h"
#include "stream/stream.h"
#include "libmpdemux/demuxer.h"
#include "libmpdemux/stheader.h"

#include "libmpcodecs/img_format.h"
#include "libmpcodecs/mp_image.h"
#include "libswscale/swscale.h"
#include "libswscale/swscale_internal.h"
#include "libmpcodecs/vf_scale.h"
#include "jz47_iputype.h"

#include <sys/mman.h>

#if 0
#include <dlfcn.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include <linux/fb.h>
#endif


static struct vf_priv_s {
    int w,h;
    int v_chr_drop;
    double param[2];
    unsigned int fmt;
    struct SwsContext *ctx;
    struct SwsContext *ctx2; //for interlaced slices only
    unsigned char* palette;
    int interlaced;
    int noup;
    int accurate_rnd;
};

//#define FBDEV_NAME "/dev/graphics/fb0"
#define FBDEV_NAME "/dev/fb0"
static struct fb_fix_screeninfo fb_fix_info;
static struct fb_var_screeninfo fb_var_info;

static void* fb_vaddr;
static void* tmp_rgb_vaddr;	/* default size: 0x800000 MB */

extern struct JZ47_IPU_MOD jz47_ipu_module;

#define PAGE_SIZE (0x1000)

static inline size_t roundUpToPageSize(size_t x) {
	return (x + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1);
}


static int mapLcdFrameBuffer()
{
	static int fbfd = 0;
	if ( fbfd > 0 )
		return 0;
	/* Get lcd control info and do some setting.  */
	fbfd = open (FBDEV_NAME, O_RDWR);
	if ( fbfd < 1 ) {
		printf("open %s failed\n", FBDEV_NAME);
		return -1;
	}


	if (ioctl(fbfd, FBIOGET_FSCREENINFO, &fb_fix_info) == -1)
		return -errno;

	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &fb_var_info) == -1)
		return -errno;

	struct fb_var_screeninfo info = fb_var_info;

	/* map fb address */
	size_t fbSize = roundUpToPageSize(fb_fix_info.line_length * info.yres_virtual);
	printf("fbSize=%d\n", fbSize);
	fb_vaddr = mmap(0, fbSize, PROT_READ|PROT_WRITE, MAP_SHARED, fbfd, 0);
	if (fb_vaddr == MAP_FAILED) {
		printf("Error mapping the framebuffer (%s)\n", strerror(errno));
		return -errno;
	}


	memset(fb_vaddr, 0, fbSize);
#if 0
	printf( "using (fd=%d)\n"
		"id           = %s\n"
		"fb_vaddr     = %#x\n"
		"line_length  = %d\n"
		"xres         = %d px\n"
		"yres         = %d px\n"
		"xres_virtual = %d px\n"
		"yres_virtual = %d px\n"
		"bpp          = %d\n"
		"r            = %2u:%u\n"
		"g            = %2u:%u\n"
		"b            = %2u:%u\n",
		fbfd,
		fb_fix_info.id,
		fb_vaddr,
		fb_fix_info.line_length,
		info.xres,
		info.yres,
		info.xres_virtual,
		info.yres_virtual,
		info.bits_per_pixel,
		info.red.offset, info.red.length,
		info.green.offset, info.green.length,
		info.blue.offset, info.blue.length
		);
#endif
    
	tmp_rgb_vaddr = malloc(0x800000);
	
	return 0;
}

/* log  ./mplayer -nosound RV40_test2011.rmvb */
/* [swscaler @ 0xdb24f0]No accelerated colorspace conversion found from yuv420p to bgra.
   [swscaler @ 0xdb24f0]using unscaled yuv420p -> bgra special converter
   VO: [fbdev] 640x480 => 640x480 BGRA 
   PIX_FMT_YUV420P=0, srcFormat=0, mpi->planes[0]=0x24244500, mpi->planes[1]=0x2428f700, mpi->planes[2]=0x242b4f00mpi->stride[0]=10240, mpi->st0
   panel_w = 0, panel_h = 0, srcW = 0, srcH = 0
   out_x = 0, out_y = 0, out_w = 0, out_h = 0
   act_x = 0, act_y = 0, act_w = 0, act_h = 0, outW = 0, outH = 0
   PIX_FMT_YUV420P=0, srcFormat=0, mpi->planes[0]=0x24470d00, mpi->planes[1]=0x244bbf00, mpi->planes[2]=0x244e1700mpi->stride[0]=10240, mpi->st0
   panel_w = 0, panel_h = 0, srcW = 0, srcH = 0
   out_x = 0, out_y = 0, out_w = 0, out_h = 0
   act_x = 0, act_y = 0, act_w = 0, act_h = 0, outW = 0, outH = 0
   PIX_FMT_YUV420P=0, srcFormat=0, mpi->planes[0]=0x24470d00, mpi->planes[1]=0x244bbf00, mpi->planes[2]=0x244e1700mpi->stride[0]=10240, mpi->st0
   panel_w = 0, panel_h = 0, srcW = 0, srcH = 0
   out_x = 0, out_y = 0, out_w = 0, out_h = 0
   act_x = 0, act_y = 0, act_w = 0, act_h = 0, outW = 0, outH = 0
   PIX_FMT_YUV420P=0, srcFormat=0, mpi->planes[0]=0x24470d00, mpi->planes[1]=0x244bbf00, mpi->planes[2]=0x244e1700mpi->stride[0]=10240, mpi->st0
   panel_w = 0, panel_h = 0, srcW = 0, srcH = 0
   out_x = 0, out_y = 0, out_w = 0, out_h = 0
   act_x = 0, act_y = 0, act_w = 0, act_h = 0, outW = 0, outH = 0
   PIX_FMT_YUV420P=0, srcFormat=0, mpi->planes[0]=0x24400100, mpi->planes[1]=0x2444b300, mpi->planes[2]=0x24470b00mpi->stride[0]=10240, mpi->st0
   panel_w = 0, panel_h = 0, srcW = 0, srcH = 0
   out_x = 0, out_y = 0, out_w = 0, out_h = 0
   act_x = 0, act_y = 0, act_w = 0, act_h = 0, outW = 0, outH = 0
   PIX_FMT_YUV420P=0, srcFormat=0, mpi->planes[0]=0x24470d00, mpi->planes[1]=0x244bbf00, mpi->planes[2]=0x244e1700mpi->stride[0]=10240, mpi->st0
   panel_w = 0, panel_h = 0, srcW = 0, srcH = 0
   out_x = 0, out_y = 0, out_w = 0, out_h = 0
   act_x = 0, act_y = 0, act_w = 0, act_h = 0, outW = 0, outH = 0
   PIX_FMT_YUV420P=0, srcFormat=0, mpi->planes[0]=0x24470d00, mpi->planes[1]=0x244bbf00, mpi->planes[2]=0x244e1700mpi->stride[0]=10240, mpi->st0
   panel_w = 0, panel_h = 0, srcW = 0, srcH = 0
   out_x = 0, out_y = 0, out_w = 0, out_h = 0
   act_x = 0, act_y = 0, act_w = 0, act_h = 0, outW = 0, outH = 0
   PIX_FMT_YUV420P=0, srcFormat=0, mpi->planes[0]=0x24470d00, mpi->planes[1]=0x244bbf00, mpi->planes[2]=0x244e1700mpi->stride[0]=10240, mpi->st0
   panel_w = 0, panel_h = 0, srcW = 0, srcH = 0
   out_x = 0, out_y = 0, out_w = 0, out_h = 0
   act_x = 0, act_y = 0, act_w = 0, act_h = 0, outW = 0, outH = 0
   PIX_FMT_YUV420P=0, srcFormat=0, mpi->planes[0]=0x24244500, mpi->planes[1]=0x2428f700, mpi->planes[2]=0x242b4f00mpi->stride[0]=10240, mpi->st0
   panel_w = 0, panel_h = 0, srcW = 0, srcH = 0
   out_x = 0, out_y = 0, out_w = 0, out_h = 0
   act_x = 0, act_y = 0, act_w = 0, act_h = 0, outW = 0, outH = 0
   PIX_FMT_YUV420P=0, srcFormat=0, mpi->planes[0]=0x24470d00, mpi->planes[1]=0x244bbf00, mpi->planes[2]=0x244e1700mpi->stride[0]=10240, mpi->st0
   panel_w = 0, panel_h = 0, srcW = 0, srcH = 0
   out_x = 0, out_y = 0, out_w = 0, out_h = 0
   act_x = 0, act_y = 0, act_w = 0, act_h = 0, outW = 0, outH = 0
   PIX_FMT_YUV420P=0, srcFormat=0, mpi->planes[0]=0x24470d00, mpi->planes[1]=0x244bbf00, mpi->planes[2]=0x244e1700mpi->stride[0]=10240, mpi->st0
   panel_w = 0, panel_h = 0, srcW = 0, srcH = 0
   out_x = 0, out_y = 0, out_w = 0, out_h = 0
   act_x = 0, act_y = 0, act_w = 0, act_h = 0, outW = 0, outH = 0
   PIX_FMT_YUV420P=0, srcFormat=0, mpi->planes[0]=0x24470d00, mpi->planes[1]=0x244bbf00, mpi->planes[2]=0x244e1700mpi->stride[0]=10240, mpi->st0

   MPlayer interrupted by signal 2 in module: decode_video
   PIX_FMT_YUV420P=0, srcFormat=0, mpi->planes[0]=0x24470d00, mpi->planes[1]=0x244bbf00, mpi->planes[2]=0x244e1700mpi->stride[0]=10240, mpi->st0
   panel_w = 0, panel_h = 0, srcW = 0, srcH = 0
   out_x = 0, out_y = 0, out_w = 0, out_h = 0
   act_x = 0, act_y = 0, act_w = 0, act_h = 0, outW = 0, outH = 0

*/
int color_convert_video_to_rgba8888(int srcW, int srcH, void *rgb_vaddr, void *planes0, void* planes1, void *planes2, int stride0, int stride1, int stride2)
{
	//printf("ENTER %s\n", __FUNCTION__);

	int width = srcW;
	int height = srcH;
	
	int line, col, linewidth;
	int y, u, v, yy, vr, ug, vg, ub;
	int r, g, b;
	const unsigned char *py, *pu, *pv;
	
	py = (unsigned char *)planes0;
	pu = (unsigned char *)planes1;
	pv = pu + 8;	
	//pv = (unsigned char *)planes2;
	//printf("w : %d, h : %d, py : 0x%08x, pu : 0x%08x, pv : 0x%08x\n",
	//		width, height, py, pu, pv);


	unsigned int *dst = (unsigned int *) rgb_vaddr;

		
#if 1
	int iLoop = 0, jLoop = 0, kLoop = 0, dxy = 0;
	int stride_y = width*16;
	int stride_uv = width*8;
	
	for (line = 0; line < height; line++) {
		for (col = 0; col < width; col++) {
			if ( iLoop > 0 && iLoop % 16 == 0 ) {
				jLoop++;
				iLoop = 0;
				dxy = jLoop*256;
				iLoop++;
			} else {
				dxy = iLoop + jLoop * 256;
				iLoop++;
			}

			y = *(py+dxy);
			yy = y << 8;
			u = *(pu+dxy/2) - 128;
			ug = 88 * u;
			ub = 454 * u;
			v = *(pv+dxy/2) - 128;
			vg = 183 * v;
			vr = 359 * v;

			r = (yy + vr) >> 8;
			g = (yy - ug - vg) >> 8;
			b = (yy + ub ) >> 8;
			
			if (r < 0) r = 0;
			if (r > 255) r = 255;
			if (g < 0) g = 0;
			if (g > 255) g = 255;
			if (b < 0) b = 0;
			if (b > 255) b = 255;
			//*dst++ = (((unsigned short)r>>3)<<11) | (((unsigned short)g>>2)<<5) | (((unsigned short)b>>3)<<0);     			
			*dst++ = 0xFF000000 | (r<<16) | (g<<8) | b;
		} 

		if ( kLoop > 0 && kLoop % 15 == 0 ) {
			py += stride_y + 16 - 256;
			pu += stride_uv + 16 - 128;
			pv = pu + 8;
			iLoop = 0; jLoop = 0; kLoop = 0;
		}
		else if ( kLoop & 1 ) {
			py += 16;
			pu += 16;
			pv = pu + 8;
			iLoop = 0; jLoop = 0; kLoop++;
		}
		else {
			py += 16;
			iLoop = 0; jLoop = 0;
			kLoop++;
		}		
	} 
#else
	int kLoop = 0;
	for (line = 0; line < height; line++) {
		for (col = 0; col < width; col++) {
			y = *(py+col);
			yy = y << 8;
			u = *(pu+col/2) - 128;
			ug = 88 * u;
			ub = 454 * u;
			v = *(pv+col/2) - 128;
			vg = 183 * v;
			vr = 359 * v;

			r = (yy + vr) >> 8;
			g = (yy - ug - vg) >> 8;
			b = (yy + ub ) >> 8;
			
			if (r < 0) r = 0;
			if (r > 255) r = 255;
			if (g < 0) g = 0;
			if (g > 255) g = 255;
			if (b < 0) b = 0;
			if (b > 255) b = 255;
			//*dst++ = (((unsigned short)r>>3)<<11) | (((unsigned short)g>>2)<<5) | (((unsigned short)b>>3)<<0);     			
			*dst++ = 0xFF000000 | (r<<16) | (g<<8) | b;
		} 
		py += width;
		if ( kLoop & 1 ) {
			pu += (width / 2);
			pv += (width / 2);
		}
		kLoop++;
	} 
#endif

	return 0;
}


int copy_tmp_rgb_buffer_to_fb_buffer(int srcW, int srcH)
{
	//printf("ENTER %s\n", __FUNCTION__);
#define BPP (4)
	
	int height, width;
	int src_stide, dst_stride;

	height = srcH < fb_var_info.yres ? srcH : fb_var_info.yres;
	width = srcW < fb_var_info.xres ? srcW : fb_var_info.xres;
	src_stide = srcW*BPP;
	dst_stride = fb_fix_info.line_length;
	int line_size = src_stide < dst_stride ? src_stide : dst_stride;
#if 0
	printf("width=%d, height=%d, src_stide=%d, dst_stride=%d, line_size=%d, fb_vaddr=%p, tmp_rgb_vaddr=%p",
	     width, height, src_stide, dst_stride, line_size, fb_vaddr, tmp_rgb_vaddr);
#endif
	int hhh, www;
	for (hhh=0; hhh< height; hhh++) {
		void * src = (void *)((unsigned int) tmp_rgb_vaddr + src_stide*hhh);
		void * dst = (void *)((unsigned int) fb_vaddr + dst_stride*hhh);
		memcpy(dst, src, line_size);
	}
	
	return 0;
}


int jz47_put_image_sw (struct vf_instance *vf, mp_image_t *mpi, double pts)
{
	
	/* src format */
	SwsContext *c = vf->priv->ctx;
	//SwsContext *c = (SwsContext *)vf->priv->ctx;
	unsigned int srcFormat = c->srcFormat;
	//printf("PIX_FMT_YUV420P=%#x, srcFormat=%#x, \n", PIX_FMT_YUV420P, srcFormat);

	/* input size */
	//printf("c->srcW=%d,  c->srcH=%d\n", c->srcW,  c->srcH);
	int srcW = c->srcW;
	int srcH = c->srcH;
	/* output size */
	/* scale_outW = (!scale_outW) ? c->dstW : scale_outW; */
	/* scale_outH = (!scale_outH) ? c->dstH : scale_outH; */

	/* src yuv buffer */
#if 0
	if (mpi) {
		printf("mpi->planes[0]=%#x, mpi->planes[1]=%#x, mpi->planes[2]=%#x \n", mpi->planes[0], mpi->planes[1], mpi->planes[2]);
		printf("mpi->stride[0]=%d, mpi->stride[1]=%d, mpi->stride[2]%d\n", mpi->stride[0], mpi->stride[1], mpi->stride[2]);
	}

	//if (DEBUG_LEVEL > 0)
	if ( 1 )
	{
		printf ("panel_w = %d, panel_h = %d, srcW = %d, srcH = %d\n",
			jz47_ipu_module.out_panel.w, jz47_ipu_module.out_panel.h, 
			jz47_ipu_module.srcW, jz47_ipu_module.srcH);
		printf ("out_x = %d, out_y = %d, out_w = %d, out_h = %d\n", 
			jz47_ipu_module.out_x, jz47_ipu_module.out_y, 
			jz47_ipu_module.out_w, jz47_ipu_module.out_h);
		printf ("act_x = %d, act_y = %d, act_w = %d, act_h = %d, outW = %d, outH = %d\n", 
			jz47_ipu_module.act_x, jz47_ipu_module.act_y, 
			jz47_ipu_module.act_w, jz47_ipu_module.act_h,
			jz47_ipu_module.resize_para.outW, jz47_ipu_module.resize_para.outH);
	}
#endif


	/* map LCD framebuffer */
	if ( mapLcdFrameBuffer() ) {
		return 0;
	}

#if 0
	/* copy to framebuffer */
	int VideoSize = (srcW * srcH * 3)/2;
	//memcpy((void*)fb_vaddr, mpi->planes[0], VideoSize );
#endif
	/* convert YUV buffer to tmp rgb buffer */

	color_convert_video_to_rgba8888(srcW, srcH, tmp_rgb_vaddr, mpi->planes[0], mpi->planes[1], mpi->planes[2], mpi->stride[0], mpi->stride[1], mpi->stride[2]);


	/* copy tmp rgb buffer to lcd fb buffer */
	copy_tmp_rgb_buffer_to_fb_buffer(srcW, srcH);
	
	
	return 0;	
}

#endif //defined(USE_SW_COLOR_CONVERT_RENDER)
