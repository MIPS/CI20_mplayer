#ifndef __JZ4780_X2D_H__
#define __JZ4780_X2D_H__

#include "jz4780_x2d_hal/x2d.h"

#if 0
#define LOG_DBG(sss, aaa...)                    \
    do {                                        \
        ALOGD("%s " sss, __func__, ##aaa);   \
    } while (0)
#else
#define LOG_DBG(sss, aaa...)                    \
    do {                                        \
    } while (0)
#endif

#define FBDEV_NAME "/dev/fb0"
#define PAGE_SIZE (4096)

#define JZFB_GET_BUFFER         _IOR('F', 0x120, int)
#define NOGPU_PAN               _IOR('F', 0x311, int)

/*  Support for multiple buffer, can be switched. */
#define JZFB_ACQUIRE_BUFFER		_IOR('F', 0x441, int)	//acquire new buffer and to display
#define JZFB_RELEASE_BUFFER		_IOR('F', 0x442, int)	//release the acquire buffer
#define JZFB_CHANGE_BUFFER		_IOR('F', 0x443, int)	//change the acquire buffer*/

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

extern int jz4780_put_image_x2d(struct vf_instance *vf, mp_image_t *mpi, double pts);

#endif //__JZ4780_X2D_H__
