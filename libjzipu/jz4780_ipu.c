
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

static int openIPU(void)
{
#ifdef USE_JZ4780_DMMU
	if (dmmu_init() != 0) {
		printf("jz4780_ipu.c dmmu_init() Failed\n");
		return -1;
	}

	dmmu_get_page_table_base_phys(&tlb_base_phys);
#endif
	if (mIPUHandler != NULL)
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
	if (mIPUHandler == NULL)
		return 0;
	
	ipu_close(&mIPUHandler);
	printf("===================== jz47_ipu_exit() ===================\n");
#ifdef USE_JZ4780_DMMU
	dmmu_deinit();
#endif
	mIPUHandler = NULL;
	mIPU_inited = 0;
	printf("closeIPU mIPUHandler=%p\n", mIPUHandler);

	return 0;
}

#ifdef USE_JZ4780_DMMU
static int mapIPUSourceBuffer(struct vf_instance *vf, mp_image_t *mpi)
{
	struct SwsContext *c = vf->priv->ctx;
	int ysize = (c->srcW * c->srcH);
	if (mpi->ipu_line) {
		int uvsize = ysize / 2;
		if ((dmmu_match_user_mem_tlb(mpi->planes[0], ysize) == 0)
			&& (dmmu_match_user_mem_tlb(mpi->planes[1], uvsize) == 0)) {
			if ((dmmu_map_user_mem(mpi->planes[0], ysize) == 0)
				&& (dmmu_map_user_mem(mpi->planes[1], uvsize) == 0)) {
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
		if ((dmmu_unmap_user_mem(mpi->planes[0], ysize) == 0)
			&& (dmmu_unmap_user_mem(mpi->planes[1], uvsize) == 0)) {
			return 0;
		}
	} else {
		int uvsize = ysize / 4;
		if ((dmmu_unmap_user_mem(mpi->planes[0], ysize) == 0)
			&& (dmmu_unmap_user_mem(mpi->planes[1], uvsize) == 0)
			&& (dmmu_unmap_user_mem(mpi->planes[2], uvsize) == 0)) {
			return 0;
		}
	}

	printf("unmapIPUSourceBuffer failed\n");
	return -1;
}

static int mapIPUDestBuffer(struct vf_instance *vf, mp_image_t *dmpi)
{
	int size = vf->priv->w * vf->priv->h * (dmpi->bpp / 8);

	if ((dmmu_match_user_mem_tlb(dmpi->planes[0], size) == 0)
		&& (dmmu_map_user_mem(dmpi->planes[0], size) == 0)) {
		return 0;
	}

	printf("mapIPUDestBuffer failed\n");
	return -1;
}

static int unmapIPUDestBuffer(struct vf_instance *vf, mp_image_t *dmpi)
{
	int size = vf->priv->w * vf->priv->h * (dmpi->bpp / 8);

	if (dmmu_unmap_user_mem(dmpi->planes[0], size) == 0) {
		return 0;
	}

	printf("unmapIPUDestBuffer failed\n");
	return -1;
}
#endif

static int initIPUSourceBuffer(struct vf_instance *vf, mp_image_t *mpi)
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
	} else {
		srcBuf->y_stride = src->width<<4; // src->width*16
		srcBuf->u_stride = src->width<<3; // src->width*8
		srcBuf->v_stride = srcBuf->u_stride;
	}

	return 0;
}

static int initIPUDestBuffer(struct vf_instance *vf, mp_image_t *dmpi)
{
	struct dest_data_info *dst;
	struct ipu_data_buffer *dstBuf;

	if (mIPUHandler == NULL) {
		return -1;
	}

	if (dmpi->flags & MP_IMGFLAG_PLANAR) {
		fprintf(stderr, "Cannot handle planar destination image!\n");
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

	switch(vf->priv->fmt) {
	case IMGFMT_RGB24:
		dst->fmt = HAL_PIXEL_FORMAT_RGB_888;
		break;
	case IMGFMT_RGB16:
		dst->fmt = HAL_PIXEL_FORMAT_RGB_565;
		break;
	case IMGFMT_ARGB:
		dst->fmt = HAL_PIXEL_FORMAT_RGBX_8888;
		break;
	case IMGFMT_BGRA:
	default:
		dst->fmt = HAL_PIXEL_FORMAT_BGRX_8888;
		break;
	}

	dst->width = vf->priv->w;
	dst->height = vf->priv->h;
	dst->left = 0;
	dst->top = 0;

#ifdef USE_JZ4780_DMMU
	/* virtual address */
	dst->dtlb_base = tlb_base_phys;
	dst->out_buf_v = dmpi->planes[0];
	dstBuf->y_buf_phys = 0;
#else
	/* physical address */
	dst->out_buf_v = 0;
	dstBuf->y_buf_phys = get_phy_addr(dmpi->planes[0]);
#endif

	dstBuf->y_stride = dmpi->stride[0];
	return 0;
}

static int updateVideo(struct vf_instance *vf, mp_image_t *mpi, mp_image_t *dmpi)
{
	if (mIPUHandler == NULL) {
		return -1;
	}

	// if buffer sizes changed, reset(re_init) ipu
	if ((mIPUHandler->src_info.width != vf->priv->ctx->srcW)
		|| (mIPUHandler->src_info.height != vf->priv->ctx->srcH)
		|| (mIPUHandler->dst_info.width != vf->priv->w)
		|| (mIPUHandler->dst_info.height != vf->priv->h)) {
		mIPU_inited = 0;
	}

	if (mIPU_inited == 0) {
		initIPUDestBuffer(vf, dmpi);
		initIPUSourceBuffer(vf, mpi);
#ifdef USE_JZ4780_DMMU
		if (mapIPUSourceBuffer(vf, mpi) < 0) {
			return -1;
		}
#ifndef JZ4780_IPU_LCDC_OVERLAY
		if (mapIPUDestBuffer(vf, dmpi) < 0) {
			return -1;
		}
#endif
#endif
		
#ifdef JZ4780_IPU_LCDC_OVERLAY
		if (setColorKeyAndAlpha() < 0) {
			printf("setColorKeyAndAlpha fail");
			return -1;
		}
#endif
		if (ipu_init(mIPUHandler) < 0) { 
			return -1;
		} else {
			mIPU_inited = 1;
		}
	} else {
		initIPUDestBuffer(vf, dmpi);
		initIPUSourceBuffer(vf, mpi);
#ifdef USE_JZ4780_DMMU
		if (mapIPUSourceBuffer(vf, mpi) < 0) {
			return -1;
		}
#ifndef JZ4780_IPU_LCDC_OVERLAY
		if (mapIPUDestBuffer(vf, dmpi) < 0) {
			return -1;
		}
#endif
#endif
	}

	ipu_postBuffer(mIPUHandler);

#ifdef USE_JZ4780_DMMU
	unmapIPUDestBuffer(vf, mpi);
#ifndef JZ4780_IPU_LCDC_OVERLAY
	/* if overlay, we don't need to umap the sourcebuffer */
	unmapIPUSourceBuffer(vf, mpi);
#endif
#endif
	return 0;

}

int jz4780_put_image(struct vf_instance *vf, mp_image_t *mpi, double pts)
{
	mp_image_t *dmpi = mpi->priv;

	//if (mpi->ipu_line) {
	//	vf->priv->ctx->srcH &= ~(0xf - 0x1);
	//	vf->priv->h &= ~(0xf - 0x1);
	//}

	if (openIPU()) {
		return 0;
	}

	if (!(mpi->flags&MP_IMGFLAG_DRAW_CALLBACK && dmpi)) {
		dmpi = vf_get_image(vf->next, vf->priv->fmt, MP_IMGTYPE_STATIC,
			MP_IMGFLAG_ACCEPT_STRIDE | MP_IMGFLAG_PREFER_ALIGNED_STRIDE,
			vf->priv->w, vf->priv->h);

		updateVideo(vf, mpi, dmpi);
	}

	if (vf->priv->w == mpi->w && vf->priv->h == mpi->h) {
		// just conversion, no scaling -> keep postprocessing data
		// this way we can apply pp filter to non-yv12 source using scaler
		vf_clone_mpi_attributes(dmpi, mpi);
	}

	return vf_next_put_image(vf, dmpi, pts);
}


void jz4780_ipu_exit(void)
{
	closeIPU();
}

#endif // defined(JZ4780_IPU)
