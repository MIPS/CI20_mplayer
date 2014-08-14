
#include <stdio.h>
#include <stdlib.h>
#include "fcntl.h"
#include "unistd.h"
#include "sys/ioctl.h"
#include <linux/fb.h>
#include "libjzcommon/com_config.h"
#include <stdbool.h>

#if defined(JZ4780_IPU)

//#include <system/graphics.h>
#include "libmpcodecs/vf.h"
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
#include "jz4780_ipu.h"
#include "jz4780_ipu_hal/dmmu.h"

//#define USE_JZ4780_DMMU		/* support virtual address */

extern unsigned int get_phy_addr (unsigned int vaddr);

static struct fb_fix_screeninfo fb_fix_info;
static struct fb_var_screeninfo fb_var_info;
static int fbfd = -1;
static void* fb_vaddr = NULL;
static void* tmp_rgb_vaddr;	/* default size: 0x800000 MB */
static int fbSize = 0;
static int mIPU_inited = 0;
#ifdef USE_JZ4780_DMMU
static unsigned int tlb_base_phys;
#endif

static struct ipu_image_info *mIPUHandler = NULL;	//need to init
#undef printf


static inline int roundUpToPageSize(int x)
{
	return (x + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1);
}

static int setFGAlpha(int fg, int enable, int mode, int value)
{
	struct jzfb_fg_alpha fg_alpha;

	fg_alpha.fg = fg;
	fg_alpha.enable = enable;
	fg_alpha.mode = mode;
	fg_alpha.value = value;
	if(ioctl(fbfd, JZFB_SET_ALPHA, &fg_alpha) < 0) {
		printf("%s set foreground %d alpha: fail",  __func__, fg);
		return -1;
	}

	return 0;
}

static int setColorKeyAndAlpha(void)
{
	if (setFGAlpha(0, 1, 1, 0) < 0) { 
		printf("enable alphamd failed!\n");
		return -1;
	}    
	return 0;
}

static int openIPU(void)
{
#ifdef USE_JZ4780_DMMU
	if ( dmmu_init() != 0 ){
		printf("jz4780_ipu.c dmmu_init() Failed\n");
		return -1;
	}

	dmmu_get_page_table_base_phys(&tlb_base_phys);
#endif
	if (mIPUHandler != NULL )
		return 0;
	
	if (ipu_open(&mIPUHandler) < 0) {
		printf("ERROR: ipu_open() failed mIPUHandler=%p\n", mIPUHandler);
		ipu_close(&mIPUHandler);
		mIPUHandler = NULL;
		return -1;
	}
	return 0;
}


static int closeIPU(void)
{
	if (mIPUHandler == NULL )
		return 0;
	
	ipu_close(&mIPUHandler);
	printf("===================== jz47_ipu_exit() ===================\n");
#ifdef USE_JZ4780_DMMU
	dmmu_deinit();
#endif
	mIPUHandler = NULL;
	mIPU_inited = 0;
	printf("closeIPU mIPUHandler=%p\n", mIPUHandler);

	munmap(fb_vaddr, fbSize);
	close(fbfd);
	free(tmp_rgb_vaddr);
	tmp_rgb_vaddr = NULL;
	
	return 0;
}

#ifdef USE_JZ4780_DMMU
static int mapIPUSourceBuffer(struct vf_instance *vf, mp_image_t *mpi)
{
	struct SwsContext *c = vf->priv->ctx;
	int ysize = (c->srcW * c->srcH);
	if (mpi->ipu_line) {
		int uvsize = ysize / 2;
		if ((dmmu_match_user_mem_tlb(mpi->planes[0], ysize) == 0) && (dmmu_match_user_mem_tlb(mpi->planes[1], uvsize) == 0)) {
			if ((dmmu_map_user_mem(mpi->planes[0], ysize) == 0) && (dmmu_map_user_mem(mpi->planes[1], uvsize) == 0)) {
				//printf("mapIPUSourceBuffer successed\n");
				return 0;
			}
		}
	} else {
		int uvsize = ysize / 4;
		if ((dmmu_match_user_mem_tlb(mpi->planes[0], ysize) == 0)
			&& (dmmu_match_user_mem_tlb(mpi->planes[1], uvsize) == 0)
			&& (dmmu_match_user_mem_tlb(mpi->planes[2], uvsize) == 0)) {
			if ((dmmu_map_user_mem(mpi->planes[0], ysize) == 0)
				&& (dmmu_map_user_mem(mpi->planes[1], uvsize) == 0)
				&& (dmmu_map_user_mem(mpi->planes[2], uvsize) == 0)) {
				//printf("mapIPUSourceBuffer successed\n");
				return 0;
			}
		}
	}
	printf("mapIPUSourceBuffer failed\n");
	return -1;
}

static int unmapIPUSourceBuffer(struct vf_instance *vf, mp_image_t *mpi)
{
	struct SwsContext *c = vf->priv->ctx;
	int ysize = (c->srcW * c->srcH);
	if (mpi->ipu_line) {
		int uvsize = ysize / 2;
		if ((dmmu_unmap_user_mem(mpi->planes[0], ysize) == 0) && (dmmu_unmap_user_mem(mpi->planes[1], uvsize) == 0)) {
			//printf("unmapIPUSourceBuffer successed\n");
			return 0;
		}
	} else {
		int uvsize = ysize / 4;
		if ((dmmu_unmap_user_mem(mpi->planes[0], ysize) == 0)
			&& (dmmu_unmap_user_mem(mpi->planes[1], uvsize) == 0)
			&& (dmmu_unmap_user_mem(mpi->planes[2], uvsize) == 0)) {
			//printf("unmapIPUSourceBuffer successed\n");
			return 0;
		}
	}
	printf("unmapIPUSourceBuffer failed\n");
	return -1;
}

static int mapIPUDestBuffer(int width, int height, int bpp, void *vaddr)
{
	if ((dmmu_match_user_mem_tlb(vaddr, width * height * bpp) == 0) && (dmmu_map_user_mem(vaddr, width * height * bpp) == 0)) {
			//printf("mapIPUDestBuffer successed\n");
			return 0;
	}
	printf("mapIPUDestBuffer failed\n");
	return -1;
}

static int unmapIPUDestBuffer(int width, int height, int bpp, void *vaddr)
{
	if (dmmu_unmap_user_mem(vaddr, width * height * bpp) == 0) {
			//printf("unmapIPUDestBuffer successed\n");
			return 0;
	}
	printf("unmapIPUDestBuffer failed\n");
	return -1;
}
#endif

static int mapLcdFrameBuffer(void)
{
	struct fb_var_screeninfo info;

	if ( fbfd > 0 )
		return 0;
	
	/* Get lcd control info and do some setting.  */
	fbfd = open(FBDEV_NAME, O_RDWR);
	if ( fbfd < 1 ) {
		printf("open %s failed\n", FBDEV_NAME);
		return -1;
	}

	if (ioctl(fbfd, FBIOGET_FSCREENINFO, &fb_fix_info) == -1)
		return -errno;

	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &fb_var_info) == -1)
		return -errno;

	info = fb_var_info;

	/* map fb address */
	fbSize = roundUpToPageSize(fb_fix_info.line_length * info.yres_virtual);
	//printf("fbSize=%d\n", fbSize);
	fb_vaddr = mmap(0, fbSize, PROT_READ|PROT_WRITE, MAP_SHARED, fbfd, 0);
	if (fb_vaddr == MAP_FAILED) {
		printf("Error mapping the framebuffer (%s)\n", strerror(errno));
		return -errno;
	}

	/* this should move into initIPUDestBuffer func to get proper size */
	//memset(fb_vaddr, 0, fbSize);

	tmp_rgb_vaddr = malloc(0x800000);

	return 0;
}

static int initIPUSourceBuffer(struct vf_instance *vf, mp_image_t *mpi, double pts)
{
	struct source_data_info *src;
	struct ipu_data_buffer *srcBuf;
	SwsContext *c = vf->priv->ctx;

	if (mIPUHandler == NULL) {
		return -1;
	}
	src = &mIPUHandler->src_info;
	srcBuf = &mIPUHandler->src_info.srcBuf;
	memset(src, 0, sizeof(struct source_data_info));

	if (mpi->ipu_line) {
		src->fmt = HAL_PIXEL_FORMAT_JZ_YUV_420_B;
		src->width = c->srcW;
		src->height = c->srcH;
	} else {
		src->fmt = HAL_PIXEL_FORMAT_JZ_YUV_420_P;
		src->width = c->srcW;
		src->height = c->srcH & ~(0xf - 0x1);
	}

#ifdef USE_JZ4780_DMMU
	src->is_virt_buf = 1;
	src->stlb_base = tlb_base_phys;

	/* YUV420_TILE do not use mpi->planes[2] and mpi->stride[2]; */
	if (mpi->ipu_line) {
		srcBuf->y_buf_v = mpi->planes[0];
		srcBuf->u_buf_v = mpi->planes[1];
		srcBuf->v_buf_v = mpi->planes[1];
	} else {
		srcBuf->y_buf_v = mpi->planes[0];
		srcBuf->u_buf_v = mpi->planes[1];
		srcBuf->v_buf_v = mpi->planes[2];
	}
	srcBuf->y_buf_phys = 0;
	srcBuf->u_buf_phys = 0;
	srcBuf->v_buf_phys = 0;
#else
	src->is_virt_buf = 0;
	src->stlb_base = 0;

	/* YUV420_TILE do not use mpi->planes[2] and mpi->stride[2]; */
	srcBuf->y_buf_v = 0;
	srcBuf->u_buf_v = 0;
	srcBuf->v_buf_v = 0;
	if (mpi->ipu_line) {
		srcBuf->y_buf_phys = get_phy_addr(mpi->planes[0]);
		srcBuf->u_buf_phys = get_phy_addr(mpi->planes[1]);
		srcBuf->v_buf_phys = get_phy_addr(mpi->planes[1]);
	} else {
		srcBuf->y_buf_phys = get_phy_addr(mpi->planes[0]);
		srcBuf->u_buf_phys = get_phy_addr(mpi->planes[1]);
		srcBuf->v_buf_phys = get_phy_addr(mpi->planes[2]);
	}
#endif


	if (1) {		/* H264_3366kb-720-ac3-7.mkv(1280x720), y_stride should be 0x5000 but decoder set y_stride=0x5400 */
		if (mpi->ipu_line) {
			srcBuf->y_stride = mpi->stride[0];
			srcBuf->u_stride = mpi->stride[1];
			srcBuf->v_stride = mpi->stride[1];
		} else {
			srcBuf->y_stride = mpi->stride[0];
			srcBuf->u_stride = mpi->stride[1];
			srcBuf->v_stride = mpi->stride[2];
		}
		//printf("mpi->stride[0]=%d, mpi->stride[1]=%d, mpi->stride[2]=%d\n",
		//		mpi->stride[0], mpi->stride[1], mpi->stride[2]);
	}
	else {
		srcBuf->y_stride = src->width<<4; // src->width*16
		srcBuf->u_stride = src->width<<3; // src->width*8
		srcBuf->v_stride = srcBuf->u_stride;
	}

	//printf("--srcBuf:w=%d,h=%d,y_stride=%d,u_stride=%d,v_stride=%d, mpi->stride[0]=%d, mpi->stride[1]=%d\n",
			//src->width,src->height,srcBuf->y_stride,srcBuf->u_stride,srcBuf->v_stride, mpi->stride[0], mpi->stride[1]);
	return 0;
}

int clear_the_dst_area(struct dest_data_info *dst)
{
	int i = 0, j = 0;
	char *pos = NULL;
	for (i = dst->left; i < dst->left + dst->width; i++) {
		for (j = dst->top; j < dst->top + dst->height; j++) {
			pos = (char *)fb_vaddr + fb_var_info.xres * j + i;  
			*pos = 0;
		}
	}

	return 0;
}

static int initIPUDestBuffer(int dstW, int dstH)
{
	int bytes_per_pixel;
	struct dest_data_info *dst;
	struct ipu_data_buffer *dstBuf;

	if (mIPUHandler == NULL) {
		return -1;
	}

	dst = &mIPUHandler->dst_info;
	dstBuf = &dst->dstBuf;

	memset(dst, 0, sizeof(struct dest_data_info));
	memset(dstBuf, 0, sizeof(struct ipu_data_buffer));

#ifdef JZ4780_IPU_LCDC_OVERLAY
	dst->dst_mode = IPU_OUTPUT_TO_LCD_FG1;
	dst->dst_mode |= IPU_DST_USE_COLOR_KEY;
	dst->dst_mode |= IPU_DST_USE_PERPIXEL_ALPHA;
#else

	dst->dst_mode = IPU_OUTPUT_TO_FRAMEBUFFER | IPU_OUTPUT_BLOCK_MODE;
#endif

	dst->fmt = HAL_PIXEL_FORMAT_BGRX_8888;
	//dst->fmt = HAL_PIXEL_FORMAT_RGBX_8888;
	dst->width= dstW ? dstW : fb_var_info.xres;
	dst->height = dstH ? dstH : fb_var_info.yres;
	dst->left = (fb_var_info.xres - dst->width) / 2;
	dst->top = (fb_var_info.yres - dst->height) / 2;
	bytes_per_pixel = fb_fix_info.line_length / fb_var_info.xres;

	//clear_the_dst_area(dst);

#ifdef USE_JZ4780_DMMU
	/* virtual address */
	dst->dtlb_base = tlb_base_phys;
	dst->out_buf_v = fb_vaddr + dst->top * fb_fix_info.line_length + dst->left * bytes_per_pixel;
	dstBuf->y_buf_phys = 0;
	/* physical address */
#else
	dst->out_buf_v = 0;
	dstBuf->y_buf_phys = fb_fix_info.smem_start;
#endif

	//dstBuf->y_stride = dst->width * bytes_per_pixel;
	dstBuf->y_stride = fb_fix_info.line_length;
	//printf("--dstBuf:l=%d,t=%d,w=%d,h=%d,stride=%d,",dst->left,dst->top,dst->width,dst->height,dstBuf->y_stride);
	return 0;
}

static int updateVideo(struct vf_instance *vf, mp_image_t *mpi, double pts)
{
	//printf("vf->priv->ctx->srcW = %d, vf->priv->ctx->srcH = %d\n", vf->priv->ctx->srcW, vf->priv->ctx->srcH);
	if (mIPUHandler == NULL) {
		return -1;
	}
	// if src buffer size changed, reset(re_init) ipu
	if ((mIPUHandler->src_info.width != vf->priv->ctx->srcW) || ( mIPUHandler->src_info.height != vf->priv->ctx->srcH)) {
		mIPU_inited = 0;
	}

	if (mIPU_inited == 0) {
		initIPUDestBuffer(vf->priv->ctx->dstW, vf->priv->ctx->dstH);
		initIPUSourceBuffer(vf, mpi, pts);
#ifdef USE_JZ4780_DMMU
		mapIPUSourceBuffer(vf, mpi);
#ifndef JZ4780_IPU_LCDC_OVERLAY
		mapIPUDestBuffer(fb_var_info.xres, fb_var_info.yres, 4, fb_vaddr);
#endif
#endif
		
#ifdef JZ4780_IPU_LCDC_OVERLAY
		if (setColorKeyAndAlpha() < 0) {
			printf("setColorKeyAndAlpha fail");
			return -1;
		}
#endif
		//printf("ipu_init");
		if (ipu_init(mIPUHandler) < 0) { 
			//printf("%s: ipu_init() failed mIPUHandler=%p",__FUNCTION__, mIPUHandler);
			return -1;
		} else {
			mIPU_inited = 1;
		}
	} else {
		//printf("per frame initIPUSourceBuffer"); 
		initIPUSourceBuffer(vf, mpi, pts);
#ifdef USE_JZ4780_DMMU
		mapIPUSourceBuffer(vf, mpi);
#endif
	}

	//printf("ipu_postBuffer");
	ipu_postBuffer(mIPUHandler);

#ifdef USE_JZ4780_DMMU
	//unmapIPUDestBuffer(vf, mpi);
#ifndef JZ4780_IPU_LCDC_OVERLAY
	/* if overlay, we don't need to umap the sourcebuffer */
	//unmapIPUSourceBuffer(vf, mpi);
#endif
#endif
	//printf("ok!"); 
	return 0;

}

int jz4780_put_image (struct vf_instance *vf, mp_image_t *mpi, double pts)
{
	
	if (mpi->ipu_line)
	{
		vf->priv->ctx->srcH &= ~(0xf - 0x1);
		vf->priv->h &= ~(0xf - 0x1);
	}

	if ( mapLcdFrameBuffer() ) {
		return 0;
	}

	if (openIPU()) {
		return 0;
	}

	updateVideo(vf, mpi, pts);

	return 0;	
}


void jz4780_ipu_exit(void)
{

	closeIPU();

	/* close fb */

	return;
}

#endif //defined(JZ4780_IPU)
