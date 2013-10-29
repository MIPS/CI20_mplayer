#include "libjzcommon/com_config.h"
#if defined(JZ4780_X2D)
#include "libmpcodecs/vf.h"
#include "libmpcodecs/mp_image.h"
#include "jz4780_x2d.h"
#include "jz4780_ipu_hal/dmmu.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include "libmpcodecs/vf.h"
#include "libswscale/swscale_internal.h"

static int x2d_Init = 0;
static struct fb_fix_screeninfo fb_fix_info;
static struct fb_var_screeninfo fb_var_info;
static int mSyncBuffer = 0;
static int fbfd = -1;
void* fb_vaddr;
void* tmp_rgb_vaddr;	/* default size: 0x800000 MB */
static unsigned int x2d_TlbBasePhys = 0;
static struct dmmu_mem_info x2d_SrcMemInfo, x2d_DstMemInfo;
struct x2d_hal_info *x2d;
struct src_layer (*x2d_Layer)[4];
struct x2d_glb_info *x2d_GlbInfo;
struct x2d_dst_info *x2d_DstInfo;

static inline int roundUpToPageSize(int x)
{
	return (x + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1);
}

/* set src buffer colorbar, stride in bytes, bpp: bytes per pixel */
static int debug_set_buffer_color_bar(void * addr, int width, int height, int stride, int bpp)
{
	int www, hhh;

	printf("addr=%p w=%d, h=%d s=%d, bpp=%d\n", addr, width, height, stride, bpp);

	if ( bpp == 4 ) {
		unsigned int *p32;
		for (hhh=0; hhh<height; hhh++) {
			p32 = (unsigned int *)((unsigned int)addr + hhh*stride ) ;
			for (www=0; www<width; www++) {
				switch ((www/16)%4) {
				case 0:
					*p32 = 0xFF0000FF;
					break;
				case 1:
					*p32 = 0xFF00FF00;
					break;
				case 2:
					*p32 = 0xFFFF0000;
					break;
				default:
					*p32 = 0xFFFFFFFF;
				}
				p32++;
			}
		}
	}


}



//#ifdef USE_JZ4780_DMMU
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
			LOG_DBG("mapIPUDestBuffer successed\n");
			return 0;
	}
	printf("mapIPUDestBuffer failed\n");
	return -1;
}

static int unmapIPUDestBuffer(int width, int height, int bpp, void *vaddr)
{
	if (dmmu_unmap_user_mem(vaddr, width * height * bpp) == 0) {
			LOG_DBG("unmapIPUDestBuffer successed\n");
			return 0;
	}
	printf("unmapIPUDestBuffer failed\n");
	return -1;
}
//#endif

static int mapLcdFrameBuffer(void)
{
	struct fb_var_screeninfo info;
	int fbSize;

	if ( fbfd > 0 )
		return 0;
	
	/* Get lcd control info and do some setting.  */
	fbfd = open(FBDEV_NAME, O_RDWR);
	if ( fbfd < 1 ) {
		printf("open %s failed\n", FBDEV_NAME);
		return -1;
	}

	if (ioctl(fbfd, FBIOGET_FSCREENINFO, &fb_fix_info) == -1)
		return -1;

	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &fb_var_info) == -1)
		return -1;

	if (ioctl(fbfd, JZFB_GET_BUFFER, &mSyncBuffer) == -1) 
		return -1;

	info = fb_var_info;

	/* map fb address */
	fbSize = roundUpToPageSize(fb_fix_info.line_length * info.yres_virtual);
	//printf("fbSize=%d\n", fbSize);
	fb_vaddr = mmap(0, fbSize, PROT_READ|PROT_WRITE, MAP_SHARED, fbfd, 0);
	if (fb_vaddr == MAP_FAILED) {
		printf("Error mapping the framebuffer failed\n");
		return -1;
	}

	memset(fb_vaddr, 0, fbSize);

	tmp_rgb_vaddr = malloc(0x800000);

	/* dmmu x2d_init */
	if (dmmu_init() < 0) {
		printf("dmmu_x2d_init failed");
		return -1;
	}
	/* dmmu get tlb base address */
	if (dmmu_get_page_table_base_phys(&x2d_TlbBasePhys) < 0) {
		printf("dmmu get tlb base phys failed");
		return -1;
	}

	return 0;
}

static int openX2D(void)
{
	if(x2d_Init == 0){
	
		x2d_Init = 1;

		/* open x2d */
		LOG_DBG("x2d open. line: %d", __LINE__);
		if (( x2d_open(&x2d)) < 0) {
			printf("x2d open failed");
			return -1;
		}
		x2d_GlbInfo = x2d->glb_info;
		x2d_Layer = x2d->layer;
		x2d_DstInfo = x2d->dst_info;

		/* x2d_init x2d src */
		LOG_DBG("x2d_init x2d src. line: %d", __LINE__);
		if (!x2d_GlbInfo || !x2d_DstInfo || !x2d_Layer) {
			printf("parameter is NULL x2d_GlbInfo:%p, x2d_DstInfo: %p, x2d_Layer: %p",
					x2d_GlbInfo, x2d_DstInfo, x2d_Layer);
			return -1;
		}
		LOG_DBG("x2d_init x2d src. line: %d", __LINE__);
	}

	return 0;
}

static int updateVideoGUI(struct vf_instance *vf, mp_image_t *mpi, double pts)
{
#if 0
	unsigned int src_addr, src_w, src_h, src_stride, dst_addr, dst_w, dst_h, dst_stride, dstRect_x, dstRect_y, dstRect_w,dstRect_h;
	src_addr= mpi->planes[0];
	src_w = vf->priv->w;
	src_h = vf->priv->h;
	src_stride = src_w<<4;
	dst_addr = fb_vaddr;
	dst_w = fb_var_info.xres;
	dst_h = fb_var_info.yres;
	//dst_stride = fb_var_info.xres * fb_var_info.bits_per_pixel / 8;
	dst_stride = fb_fix_info.line_length;
	dstRect_x = 0;
	dstRect_y = 0;
	dstRect_w = fb_var_info.xres;
	dstRect_h = fb_var_info.yres;
#endif
	int ret = 0;

	/* debug set colorbar */
	//memset (mpi->planes[0], 0, vf->priv->w*vf->priv->h);
	//debug_set_buffer_color_bar(mpi->planes[0], vf->priv->w, vf->priv->h/4, vf->priv->w*4, 4);
	SwsContext *c = vf->priv->ctx;

	/* map address */
	if (mapIPUDestBuffer(fb_var_info.xres, fb_var_info.yres, fb_var_info.bits_per_pixel, fb_vaddr) < 0) {
		return -1;
	}
	if (mapIPUSourceBuffer(vf, mpi) < 0) {
		return -1;
	}

	LOG_DBG("x2d_init x2d src. line: %d", __LINE__);
	x2d_GlbInfo->layer_num = 1;
	if (mpi->ipu_line) {
		x2d_Layer[0]->format = HAL_PIXEL_FORMAT_JZ_YUV_420_B;
		x2d_Layer[0]->in_width = c->srcW;
		x2d_Layer[0]->in_height = c->srcH;
	} else {
		x2d_Layer[0]->format = HAL_PIXEL_FORMAT_JZ_YUV_420_P;
		x2d_Layer[0]->in_width = c->srcW;
		x2d_Layer[0]->in_height = c->srcH & ~(0xf - 0x1);
	}
	x2d_Layer[0]->out_width = fb_var_info.xres;
	x2d_Layer[0]->out_height = fb_var_info.yres;

	x2d_Layer[0]->addr = mpi->planes[0];
	x2d_Layer[0]->u_addr = mpi->planes[1];
	if (mpi->ipu_line) {
		x2d_Layer[0]->v_addr = mpi->planes[1];
	} else {
		x2d_Layer[0]->v_addr = mpi->planes[2];
	}
#if 0
	x2d_Layer[0]->y_stride = vf->priv->w; /* X2D recaculater YUV420_TILE stride_new = stride_orig*16 */
	x2d_Layer[0]->v_stride = vf->priv->w/2;
#else
	x2d_Layer[0]->y_stride = mpi->stride[0]; /* X2D recaculater YUV420_TILE stride_new = stride_orig*16 */
	x2d_Layer[0]->v_stride = mpi->stride[1];
	if (mpi->ipu_line) {
		x2d_Layer[0]->v_stride = mpi->stride[1];
	} else {
		x2d_Layer[0]->v_stride = mpi->stride[2];
	}
#endif
	
	x2d_Layer[0]->glb_alpha_en = 1;
	x2d_Layer[0]->global_alpha_val = 0xff;
	x2d_Layer[0]->preRGB_en = 0;
	x2d_Layer[0]->out_w_offset = 0;
	x2d_Layer[0]->out_h_offset = 0;
	x2d_Layer[0]->mask_en = 0;
	x2d_Layer[0]->msk_val = 0xffff0000;

	if (x2d_src_init(x2d) < 0)
		printf("x2d src hal x2d_init failed");

	/* x2d_init x2d dst */
	LOG_DBG("x2d_init x2d dst. line: %d", __LINE__);
	x2d_GlbInfo->tlb_base = x2d_TlbBasePhys;
	x2d_DstInfo->dst_address = fb_vaddr;
	x2d_DstInfo->dst_format = HAL_PIXEL_FORMAT_RGBX_8888;
	//x2d_DstInfo->dst_format = HAL_PIXEL_FORMAT_BGRA_8888;
	x2d_DstInfo->dst_width = fb_var_info.xres;
	x2d_DstInfo->dst_height = fb_var_info.yres;
	x2d_DstInfo->dst_stride = fb_fix_info.line_length;
	x2d_DstInfo->dst_alpha_val = 0xff;
	x2d_DstInfo->dst_mask_val = 0;
	x2d_DstInfo->dst_bcground = 0x00000000;
	x2d_DstInfo->dst_back_en = 0;
	x2d_DstInfo->dst_preRGB_en = 0;
	x2d_DstInfo->dst_glb_alpha_en = 1;
	x2d_DstInfo->dst_mask_en = 0;


	if (x2d_dst_init(x2d) < 0)
		printf("x2d dst x2d_init failed");

	/* start x2d */
	LOG_DBG("x2d start. line: %d", __LINE__);
	if ((ret = x2d_start(x2d)) < 0)
		printf("x2d_start failed");

	return 0;
}

int jz4780_put_image_x2d(struct vf_instance *vf, mp_image_t *mpi, double pts)
{
	if ( openX2D() ) {
		return 0;
	}
	
	if ( mapLcdFrameBuffer() ) {
		return 0;
	}

	if (updateVideoGUI(vf, mpi, pts) ) {
		return 0;
	}

	return 1;
}




int jz4780_x2d_exit(void)
{

	LOG_DBG("==================jz4780_x2d_exit================\n");
	/* close x2d */
	x2d_release(&x2d);
	x2d_Init = 0;

	/* close dmmu */
	dmmu_deinit();

	/* close fb */
	close(fbfd);
	fbfd = -1;

	return 0;
}

#endif //defined(JZ4780_X2D)
