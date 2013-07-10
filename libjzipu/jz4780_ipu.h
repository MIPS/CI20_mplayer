#ifndef __JZ4780_IPU_H__
#define __JZ4780_IPU_H__

#include "jz4780_ipu_hal/jz_ipu.h"

enum {
    HAL_PIXEL_FORMAT_RGBA_8888          = 1,
    HAL_PIXEL_FORMAT_RGBX_8888          = 2,
    HAL_PIXEL_FORMAT_RGB_888            = 3,
    HAL_PIXEL_FORMAT_RGB_565            = 4,
    HAL_PIXEL_FORMAT_BGRA_8888          = 5,
    HAL_PIXEL_FORMAT_RGBA_5551          = 6,
    HAL_PIXEL_FORMAT_RGBA_4444          = 7,

//    HAL_PIXEL_FORMAT_BGRX_8888  	= 0x8000,
    HAL_PIXEL_FORMAT_BGRX_8888  	= 0x1ff,

    /* 0x8 - 0xFF range unavailable */

    /*
     * 0x100 - 0x1FF
     *
     * This range is reserved for pixel formats that are specific to the HAL
     * implementation.  Implementations can use any value in this range to
     * communicate video pixel formats between their HAL modules.  These formats
     * must not have an alpha channel.  Additionally, an EGLimage created from a
     * gralloc buffer of one of these formats must be supported for use with the
     * GL_OES_EGL_image_external OpenGL ES extension.
     */

    /*
     * Android YUV format:
     *
     * This format is exposed outside of the HAL to software decoders and
     * applications.  EGLImageKHR must support it in conjunction with the
     * OES_EGL_image_external extension.
     *
     * YV12 is a 4:2:0 YCrCb planar format comprised of a WxH Y plane followed
     * by (W/2) x (H/2) Cr and Cb planes.
     *
     * This format assumes
     * - an even width
     * - an even height
     * - a horizontal stride multiple of 16 pixels
     * - a vertical stride equal to the height
     *
     *   y_size = stride * height
     *   c_stride = ALIGN(stride/2, 16)
     *   c_size = c_stride * height/2
     *   size = y_size + c_size * 2
     *   cr_offset = y_size
     *   cb_offset = y_size + c_size
     *
     */
    HAL_PIXEL_FORMAT_YV12   = 0x32315659, // YCrCb 4:2:0 Planar

    /*
     * Android RAW sensor format:
     *
     * This format is exposed outside of the HAL to applications.
     *
     * RAW_SENSOR is a single-channel 16-bit format, typically representing raw
     * Bayer-pattern images from an image sensor, with minimal processing.
     *
     * The exact pixel layout of the data in the buffer is sensor-dependent, and
     * needs to be queried from the camera device.
     *
     * Generally, not all 16 bits are used; more common values are 10 or 12
     * bits. All parameters to interpret the raw data (black and white points,
     * color space, etc) must be queried from the camera device.
     *
     * This format assumes
     * - an even width
     * - an even height
     * - a horizontal stride multiple of 16 pixels (32 bytes).
     */
    HAL_PIXEL_FORMAT_RAW_SENSOR = 0x20,

    /* Legacy formats (deprecated), used by ImageFormat.java */
    HAL_PIXEL_FORMAT_YCbCr_422_SP       = 0x10, // NV16
    HAL_PIXEL_FORMAT_YCrCb_420_SP       = 0x11, // NV21
    HAL_PIXEL_FORMAT_YCbCr_422_I        = 0x14, // YUY2

    /* suport for YUV420 */
    HAL_PIXEL_FORMAT_JZ_YUV_420_P       = 0x47700001, // YUV_420_P
    HAL_PIXEL_FORMAT_JZ_YUV_420_B       = 0x47700002, // YUV_420_P BLOCK MODE
};

enum {
	/* flip source image horizontally (around the vertical axis) */
	HAL_TRANSFORM_FLIP_H    = 0x01,
	/* flip source image vertically (around the horizontal axis)*/
	HAL_TRANSFORM_FLIP_V    = 0x02,
	/* rotate source image 90 degrees clockwise */
	HAL_TRANSFORM_ROT_90    = 0x04,
	/* rotate source image 180 degrees */
	HAL_TRANSFORM_ROT_180   = 0x03,
	/* rotate source image 270 degrees clockwise */
	HAL_TRANSFORM_ROT_270   = 0x07,
};



struct vf_priv_s {
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


typedef struct IpuSrcInfo {
    int srcFmt;
    int srcWidth;
    int srcHeight;
    int src_is_virt_buf;          /* virtual memory or physical memory */
    unsigned int src_tlb_base;

    void *src_y_v;                /* virtual address of y buffer or base address */
    void *src_u_v;
    void *src_v_v;
    int src_y_stride;
    int src_u_stride;
    int src_v_stride;

    unsigned int src_y_addr;      /* physical address of y buffer */
    unsigned int src_u_addr;
    unsigned int src_v_addr;
}IpuSrcInfo_t;

typedef struct {
	int out_width;
	int out_height;
	int out_w_offset;
	int out_h_offset;
}IpuDstInfo_t;

struct jzfb_fg_alpha {
	unsigned int fg; /* 0:fg0, 1:fg1 */
	unsigned int enable;
	unsigned int mode; /* 0:global alpha, 1:pixel alpha */
	unsigned int value; /* 0x00-0xFF */
};

#if 0
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
#endif

#define CPU_TYPE 4780
#define OUTPUT_MODE IPU_OUT_FB

#define PAGE_SIZE (0x1000)
#define FBDEV_NAME "/dev/fb0"

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

/* #define HAL_PIXEL_FORMAT_BGRX_8888      0x1FF */
/* #define HAL_PIXEL_FORMAT_JZ_YUV_420_P	0x47700001 */
#define JZFB_SET_ALPHA			_IOW('F', 0x123, struct jzfb_fg_alpha)

#if 0
enum {
	ZOOM_MODE_BILINEAR = 0,
	ZOOM_MODE_BICUBE,
	ZOOM_MODE_BILINEAR_ENHANCE,
};
#endif

#endif //__JZ4780_IPU_H__
