
#include <stdio.h>
#include <stdlib.h>
#include "fcntl.h"
#include "unistd.h"
#include "sys/ioctl.h"
#include <linux/fb.h>

#include "libavcodec/avcodec.h"
#include "libmpcodecs/img_format.h"
#include "stream/stream.h"
#include "libmpdemux/demuxer.h"
#include "libmpdemux/stheader.h"

#include "libmpcodecs/img_format.h"
#include "libmpcodecs/mp_image.h"
#include "libmpcodecs/vf.h"
#include "libswscale/swscale.h"
#include "libswscale/swscale_internal.h"
#include "libmpcodecs/vf_scale.h"

// cover the whole file
//#if defined(USE_JZ_IPU)
#if defined(USE_JZ_IPU) && ( defined(JZ4740_IPU) || defined(JZ4750_IPU) || defined(JZ4760_IPU) )


#ifdef JZ4750_OPT
#include "jz4750_ipu_regops.h"
#include "jz4750_lcd.h"
struct jz4750lcd_info jzlcd_info;
#else
#include "jz4740_ipu_regops.h"
#endif
#include "jz47_iputype.h"

typedef struct {
    AVCodecContext *avctx;
    AVFrame *pic;
    enum PixelFormat pix_fmt;
    int do_slices;
    int do_dr1;
    int vo_initialized;
    int best_csp;
    int b_age;
    int ip_age[2];
    int qp_stat[32];
    double qp_sum;
    double inv_qp_sum;
    int ip_count;
    int b_count;
    AVRational last_sample_aspect_ratio;
} vd_ffmpeg_ctx;

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

/* ================================================================================ */
int xpos = -1, ypos = -1;
extern int jz47_calc_resize_para ();
extern void jz47_free_alloc_mem();
/* ================================================================================ */
#define IPU_OUT_FB        0
#define IPU_OUT_LCD       1
#define IPU_OUT_PAL_TV    2
#define IPU_OUT_NTSC_TV   3
#define IPU_OUT_MEM       8

#define DEBUG_LEVEL  1  /* 1 dump resize message, 2 dump register for every frame.  */
/* ================================================================================ */

int scale_outW=0;  // -1, calculate the w,h base the panel w,h; 0, use the input w,h
int scale_outH=0;

#ifdef JZ4760_OPT
  #define CPU_TYPE 4760
  #define OUTPUT_MODE IPU_OUT_LCD
#else
#ifdef JZ4755_OPT
  #define CPU_TYPE 4755
  #define OUTPUT_MODE IPU_OUT_LCD
#else
#ifdef JZ4750_OPT
  #define CPU_TYPE 4750
  #define OUTPUT_MODE IPU_OUT_FB
#else
  #define CPU_TYPE 4740
  #define OUTPUT_MODE IPU_OUT_FB
#endif /* JZ4750_OPT  */
#endif /* JZ4755_OPT  */
#endif /* JZ4760_OPT  */


/* ipu virtual address.   */
volatile unsigned char *ipu_vbase=NULL;

/* struct IPU module to recorde some info */
struct JZ47_IPU_MOD jz47_ipu_module = {
    .output_mode = OUTPUT_MODE, /* Use the frame for the default */ 
    .rsize_algorithm = 1,      /* 0: liner, 1: bicube, 2: biliner.  */
    .rsize_bicube_level = -2,     /*-2, -1, -0.75, -0.5 */
  };

/* Flag to indicate the module init status */
int jz47_ipu_module_init = 0; 

/* CPU type: 4740, 4750, 4755, 4760 */
int jz47_cpu_type = CPU_TYPE;

/* flush the dcache.  */
unsigned int dcache[4096];

unsigned char *jz47_ipu_output_mem_ptr = NULL;
static int ipu_fbfd = 0;

int ipu_image_completed = 0;

/* ================================================================================ */

static char *ipu_regs_name[] = {
    "REG_CTRL",         /* 0x0 */ 
    "REG_STATUS",       /* 0x4 */ 
    "REG_D_FMT",        /* 0x8 */ 
    "REG_Y_ADDR",       /* 0xc */ 
    "REG_U_ADDR",       /* 0x10 */ 
    "REG_V_ADDR",       /* 0x14 */ 
    "REG_IN_FM_GS",     /* 0x18 */ 
    "REG_Y_STRIDE",     /* 0x1c */ 
    "REG_UV_STRIDE",    /* 0x20 */ 
    "REG_OUT_ADDR",     /* 0x24 */ 
    "REG_OUT_GS",       /* 0x28 */ 
    "REG_OUT_STRIDE",   /* 0x2c */ 
    "RSZ_COEF_INDEX",   /* 0x30 */ 
    "REG_CSC_C0_COEF",  /* 0x34 */ 
    "REG_CSC_C1_COEF",  /* 0x38 */ 
    "REG_CSC_C2_COEF",  /* 0x3c */ 
    "REG_CSC_C3_COEF",  /* 0x40 */ 
    "REG_CSC_C4_COEF",  /* 0x44 */
    "REG_H_LUT",        /* 0x48 */ 
    "REG_V_LUT",        /* 0x4c */ 
    "REG_CSC_OFFPARA",  /* 0x50 */
};

static int jz47_dump_ipu_regs(int num)
{
  int i, total;
  if (num >= 0)
  {
    printf ("ipu_reg: %s: 0x%x\n", ipu_regs_name[num >> 2], REG32(ipu_vbase + num));
  	return 1;
  }
  if (num == -1)
  {
    total = sizeof (ipu_regs_name) / sizeof (char *);
    for (i = 0; i < total; i++)
      printf ("ipu_reg: %s: 0x%x\n", ipu_regs_name[i], REG32(ipu_vbase + (i << 2)));
  }
  return 1;
}

/* ================================================================================ */
static int init_lcd_ctrl_fbfd ()
{
  /* Get lcd control info and do some setting.  */
  if (!ipu_fbfd)
    ipu_fbfd = open ("/dev/fb0", O_RDWR);

  if (!ipu_fbfd)
  {
    printf ("%s:%d +++ ERR: can not open /dev/fb0 ++++\n", __FILE__, __LINE__);
    return 0;
  }
  return 1;
}

static void deinit_lcd_ctrl_fbfd ()
{
  /* Get lcd control info and do some setting.  */
  if (ipu_fbfd)
    close (ipu_fbfd);

  ipu_fbfd = 0;
}

static int jz47_get_output_panel_info (void)
{
  struct fb_var_screeninfo fbvar;
  struct fb_fix_screeninfo fbfix;
  int output_mode = jz47_ipu_module.output_mode;

  /* For JZ4740 cpu, We haven't TV output and IPU through */
  if (jz47_cpu_type == 4740 && output_mode != IPU_OUT_FB && output_mode != IPU_OUT_MEM)
    jz47_ipu_module.output_mode = output_mode = IPU_OUT_FB;

  switch (output_mode)
  {
    case IPU_OUT_PAL_TV:
      jz47_ipu_module.out_panel.w = 720;
      jz47_ipu_module.out_panel.h = 573;
      jz47_ipu_module.out_panel.bpp_byte = 4;
      jz47_ipu_module.out_panel.bytes_per_line = 720 * 4;
      jz47_ipu_module.out_panel.output_phy = 0;
      break;

    case IPU_OUT_NTSC_TV:
      jz47_ipu_module.out_panel.w = 720;
      jz47_ipu_module.out_panel.h = 480;
      jz47_ipu_module.out_panel.bpp_byte = 4;
      jz47_ipu_module.out_panel.bytes_per_line = 720 * 4;
      jz47_ipu_module.out_panel.output_phy = 0;
      break;

    case IPU_OUT_MEM:
      jz47_ipu_module.out_panel.w = 480;
      jz47_ipu_module.out_panel.h = 272;
      jz47_ipu_module.out_panel.bpp_byte = 4;
      jz47_ipu_module.out_panel.bytes_per_line = 480 * 4;
      jz47_ipu_module.out_panel.output_phy = get_phy_addr ((unsigned int)jz47_ipu_output_mem_ptr);
      break;

    case IPU_OUT_FB:
    default:
      /* open the frame buffer */
      if (! init_lcd_ctrl_fbfd ())
        return 0;

      /* get the frame buffer info */
      ioctl (ipu_fbfd, FBIOGET_VSCREENINFO, &fbvar);
      ioctl (ipu_fbfd, FBIOGET_FSCREENINFO, &fbfix);

      /* set the output panel info */
      jz47_ipu_module.out_panel.w = fbvar.xres;
      jz47_ipu_module.out_panel.h = fbvar.yres;
      jz47_ipu_module.out_panel.bytes_per_line = fbfix.line_length;
      jz47_ipu_module.out_panel.bpp_byte = fbfix.line_length / fbvar.xres;
      jz47_ipu_module.out_panel.output_phy = fbfix.smem_start;
      break;
  }
  return 1;
}

/* ================================================================================ */
/*
  x = -1, y = -1 is center display
  w = -1, h = -1 is orignal w,h
  w = -2, h = -2 is auto fit
  other: specify  by user
*/

static int jz47_calc_ipu_outsize_and_position (int x, int y, int w, int h)
{
  int dispscr_w, dispscr_h;
  int orignal_w = jz47_ipu_module.srcW;
  int orignal_h = jz47_ipu_module.srcH;

  /* record the orignal setting */
  jz47_ipu_module.out_x = x;
  jz47_ipu_module.out_y = y;
  jz47_ipu_module.out_w = w;
  jz47_ipu_module.out_h = h;

  // The MAX display area which can be used by ipu 
  dispscr_w = (x < 0) ? jz47_ipu_module.out_panel.w : (jz47_ipu_module.out_panel.w - x);
  dispscr_h = (y < 0) ? jz47_ipu_module.out_panel.h : (jz47_ipu_module.out_panel.h - y);

  // Orignal size playing or auto fit screen playing mode
  if ((w == -1 && h == -1 && (orignal_w > dispscr_w ||  orignal_h > dispscr_h)) || (w == -2 || h == -2))
  {
    float scale_h = (float)orignal_h / dispscr_h;
    float scale_w = (float)orignal_w / dispscr_w;
    if (scale_w > scale_h)
    {
      w = dispscr_w;
      h = (dispscr_w * orignal_h) / orignal_w;
    }
    else
    {
      h = dispscr_h;
      w = (dispscr_h * orignal_w) / orignal_h;
    }
  }
  
  // w,h is orignal w,h
  w = (w == -1)? orignal_w : w;
  h = (h == -1)? orignal_h : h;

  // w,h must < dispscr_w,dispscr_h
  w = (w > dispscr_w)? dispscr_w : w;
  h = (h > dispscr_h)? dispscr_h : h;

  // w,h must <= 2*(orignal_w, orignal_h)
  w = (w > 2 * orignal_w) ? (2 * orignal_w) : w;
  h = (h > 2 * orignal_h) ? (2 * orignal_h) : h;

  // calc output position out_x, out_y
  jz47_ipu_module.act_x = (x == -1) ? ((jz47_ipu_module.out_panel.w - w) / 2) : x;
  jz47_ipu_module.act_y = (y == -1) ? ((jz47_ipu_module.out_panel.h - h) / 2) : y;

  // set the resize_w, resize_h
  jz47_ipu_module.act_w = w;
  jz47_ipu_module.act_h = h;

  jz47_ipu_module.need_config_resize = 1;
  jz47_ipu_module.need_config_outputpara = 1;
  return 1;
}

/* ================================================================================ */
static void jz47_config_ipu_input_para (SwsContext *c, mp_image_t *mpi)
{
  unsigned int in_fmt;
  unsigned int srcFormat = c->srcFormat;

  in_fmt = INFMT_YCbCr420; // default value
  if (jz47_ipu_module.need_config_inputpara)
  {
    /* Set input Data format.  */
    switch (srcFormat)
    {
      case PIX_FMT_YUV420P:
        in_fmt = INFMT_YCbCr420;
      break;

      case PIX_FMT_YUV422P:
        in_fmt = INFMT_YCbCr422;
      break;

      case PIX_FMT_YUV444P:
        in_fmt = INFMT_YCbCr444;
      break;

      case PIX_FMT_YUV411P:
        in_fmt = INFMT_YCbCr411;
      break;
    }
    REG32 (ipu_vbase + REG_D_FMT) &= ~(IN_FMT_MASK);
    REG32 (ipu_vbase + REG_D_FMT) |= in_fmt;

    /* Set input width and height.  */
    REG32(ipu_vbase + REG_IN_FM_GS) = IN_FM_W(jz47_ipu_module.srcW) | IN_FM_H (jz47_ipu_module.srcH & ~1);

    /* Set the CSC COEF */
#ifdef JZ4765_OPT
    REG32(ipu_vbase + REG_CSC_C0_COEF) = YUV_CSC_C0;
    REG32(ipu_vbase + REG_CSC_C4_COEF) = YUV_CSC_C1;
    REG32(ipu_vbase + REG_CSC_C3_COEF) = YUV_CSC_C2;
    REG32(ipu_vbase + REG_CSC_C2_COEF) = YUV_CSC_C3;
    REG32(ipu_vbase + REG_CSC_C1_COEF) = YUV_CSC_C4;
#else
    REG32(ipu_vbase + REG_CSC_C0_COEF) = YUV_CSC_C0;
    REG32(ipu_vbase + REG_CSC_C1_COEF) = YUV_CSC_C1;
    REG32(ipu_vbase + REG_CSC_C2_COEF) = YUV_CSC_C2;
    REG32(ipu_vbase + REG_CSC_C3_COEF) = YUV_CSC_C3;
    REG32(ipu_vbase + REG_CSC_C4_COEF) = YUV_CSC_C4;
#endif

    if (jz47_cpu_type != 4740)
      REG32(ipu_vbase + REG_CSC_OFFPARA) = YUV_CSC_OFFPARA;

    /* Configure the stride for YUV.  */

    REG32(ipu_vbase + REG_Y_STRIDE) = mpi->stride[0];
#ifdef JZ4765_OPT
    if (mpi->ipu_line){
      REG32(ipu_vbase + REG_UV_STRIDE) = U_STRIDE(mpi->stride[1]) | V_STRIDE(mpi->stride[1]);
    }else{
      REG32(ipu_vbase + REG_UV_STRIDE) = U_STRIDE(mpi->stride[1]) | V_STRIDE(mpi->stride[2]);
    }
#else
    REG32(ipu_vbase + REG_UV_STRIDE) = U_STRIDE(mpi->stride[1]) | V_STRIDE(mpi->stride[2]);
#endif
  }

  /* Set the YUV addr.  */
#ifdef JZ4765_OPT
  if (mpi->ipu_line){
    REG32(ipu_vbase + REG_Y_ADDR) = get_phy_addr ((unsigned int)mpi->planes[0]);
    REG32(ipu_vbase + REG_U_ADDR) = get_phy_addr ((unsigned int)mpi->planes[1]);
    REG32(ipu_vbase + REG_V_ADDR) = get_phy_addr ((unsigned int)mpi->planes[1]);
  }else{
    REG32(ipu_vbase + REG_Y_ADDR) = get_phy_addr ((unsigned int)mpi->planes[0]);
    REG32(ipu_vbase + REG_U_ADDR) = get_phy_addr ((unsigned int)mpi->planes[1]);
    REG32(ipu_vbase + REG_V_ADDR) = get_phy_addr ((unsigned int)mpi->planes[2]);
  }
#else
  REG32(ipu_vbase + REG_Y_ADDR) = get_phy_addr ((unsigned int)mpi->planes[0]);
  REG32(ipu_vbase + REG_U_ADDR) = get_phy_addr ((unsigned int)mpi->planes[1]);
  REG32(ipu_vbase + REG_V_ADDR) = get_phy_addr ((unsigned int)mpi->planes[2]);
#endif

}

/* ================================================================================ */
static void jz47_config_ipu_resize_para ()
{
  int i, width_resize_enable, height_resize_enable;
  int width_up, height_up, width_lut_size, height_lut_size;

  width_resize_enable  = jz47_ipu_module.resize_para.width_resize_enable;
  height_resize_enable = jz47_ipu_module.resize_para.height_resize_enable;
  width_up  =  jz47_ipu_module.resize_para.width_up;
  height_up =  jz47_ipu_module.resize_para.height_up;
  width_lut_size  = jz47_ipu_module.resize_para.width_lut_size;
  height_lut_size = jz47_ipu_module.resize_para.height_lut_size;

  /* Enable the rsize configure.  */
  disable_rsize (ipu_vbase);

  if (width_resize_enable)
    enable_hrsize (ipu_vbase);

  if (height_resize_enable)
    enable_vrsize (ipu_vbase);

  /* select the resize algorithm.  */
#ifdef JZ4750_OPT
  if (jz47_cpu_type == 4760 && jz47_ipu_module.rsize_algorithm)     /* 0: liner, 1: bicube, 2: biliner.  */
    enable_ipu_bicubic(ipu_vbase);
  else
    disable_ipu_bicubic(ipu_vbase);
#endif

  /* Enable the scale configure.  */
  REG32 (ipu_vbase + REG_CTRL) &= ~((1 << V_SCALE_SHIFT) | (1 << H_SCALE_SHIFT));
  REG32 (ipu_vbase + REG_CTRL) |= (height_up << V_SCALE_SHIFT) | (width_up << H_SCALE_SHIFT);

  /* Set the LUT index.  */
  REG32 (ipu_vbase + REG_RSZ_COEF_INDEX) = (((height_lut_size - 1) << VE_IDX_SFT) 
                                            | ((width_lut_size - 1) << HE_IDX_SFT));

 /* set lut. */
  if (jz47_cpu_type == 4740)
  {
    if (height_resize_enable)
    {
      for (i = 0; i < height_lut_size; i++)
        REG32 (ipu_vbase + VRSZ_LUT_BASE + i * 4) = jz47_ipu_module.resize_para.height_lut_coef[i];
    }
    else
      REG32 (ipu_vbase + VRSZ_LUT_BASE) = ((128 << 2)| 0x3);

    if (width_resize_enable)
    {
      for (i = 0; i < width_lut_size; i++)
        REG32 (ipu_vbase + HRSZ_LUT_BASE + i * 4) = jz47_ipu_module.resize_para.width_lut_coef[i];
    }
    else
      REG32 (ipu_vbase + HRSZ_LUT_BASE) = ((128 << 2)| 0x3);
  }
  else
  {
    REG32 (ipu_vbase + VRSZ_LUT_BASE) = (1 << START_N_SFT);
    for (i = 0; i < height_lut_size; i++)
      if (jz47_cpu_type == 4760 && jz47_ipu_module.rsize_algorithm)     /* 0: liner, 1: bicube, 2: biliner.  */
      {
        REG32 (ipu_vbase + VRSZ_LUT_BASE) = jz47_ipu_module.resize_para.height_bicube_lut_coef[2*i + 0];
        REG32 (ipu_vbase + VRSZ_LUT_BASE) = jz47_ipu_module.resize_para.height_bicube_lut_coef[2*i + 1];
      }
      else
        REG32 (ipu_vbase + VRSZ_LUT_BASE) = jz47_ipu_module.resize_para.height_lut_coef[i];

    REG32 (ipu_vbase + HRSZ_LUT_BASE) = (1 << START_N_SFT);
    for (i = 0; i < width_lut_size; i++)
      if (jz47_cpu_type == 4760 && jz47_ipu_module.rsize_algorithm)     /* 0: liner, 1: bicube, 2: biliner.  */
      {
        REG32 (ipu_vbase + HRSZ_LUT_BASE) = jz47_ipu_module.resize_para.width_bicube_lut_coef[2*i + 0];
        REG32 (ipu_vbase + HRSZ_LUT_BASE) = jz47_ipu_module.resize_para.width_bicube_lut_coef[2*i + 1];
      }
      else
        REG32 (ipu_vbase + HRSZ_LUT_BASE) = jz47_ipu_module.resize_para.width_lut_coef[i];
  }
 
  /* dump the resize messages.  */
  if (DEBUG_LEVEL > 0)
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

}

/* ================================================================================ */

static void disable_lcd_ctrl ()
{
#ifdef JZ4750_OPT
  int ret;
  struct jz4750lcd_info jzlcd_info;
  if (! init_lcd_ctrl_fbfd ())
    return;
  ret = ioctl(ipu_fbfd, FBIO_GET_MODE, &jzlcd_info);
  ret = ioctl (ipu_fbfd, FBIODISPOFF, &jzlcd_info);
#endif
}

static void enable_lcd_ctrl ()
{
#ifdef JZ4750_OPT
  int ret;
  struct jz4750lcd_info jzlcd_info;
  if (! init_lcd_ctrl_fbfd ())
    return;
  ret = ioctl(ipu_fbfd, FBIO_GET_MODE, &jzlcd_info);
  ret = ioctl (ipu_fbfd, FBIODISPON, &jzlcd_info);
#endif
}

/* ================================================================================ */
static void set_lcd_osd_for_ipu_thr ()
{
#ifdef JZ4750_OPT
  int ret;
  int rsize_w, rsize_h, outW, outH;
  struct jz4750lcd_info jzlcd_info;
   
  /* Get the output parameter from struct.  */
  outW = jz47_ipu_module.resize_para.outW;
  outH = jz47_ipu_module.resize_para.outH;
  rsize_w = jz47_ipu_module.act_w;
  rsize_h = jz47_ipu_module.act_h;

  /* outW must < resize_w and outH must < resize_h.  */
  outW = (outW <= rsize_w) ? outW : rsize_w;
  outH = (outH <= rsize_h) ? outH : rsize_h;

  if (! init_lcd_ctrl_fbfd ())
    return;
  
  /* set the fg1 osd size.  */
  ret = ioctl(ipu_fbfd, FBIO_GET_MODE, &jzlcd_info);
  jzlcd_info.osd.fg_change = FG1_CHANGE_SIZE; /* change f1 size */
  jzlcd_info.osd.fg1.w = outW;
  jzlcd_info.osd.fg1.h = outH;
  ret = ioctl(ipu_fbfd, FBIO_SET_MODE, &jzlcd_info);

  /* set the fg1 osd position.  */
  ret = ioctl(ipu_fbfd, FBIO_GET_MODE, &jzlcd_info);
  jzlcd_info.osd.fg_change = FG1_CHANGE_POSITION;
  jzlcd_info.osd.fg1.x = jz47_ipu_module.act_x;
  jzlcd_info.osd.fg1.y = jz47_ipu_module.act_y;
  ret = ioctl(ipu_fbfd, FBIO_SET_MODE, &jzlcd_info);
  
  /* switch to fg1 */
  ret = ioctl(ipu_fbfd, FBIO_GET_MODE, &jzlcd_info);
  jzlcd_info.osd.bgcolor = 0x0;
  __jzlcd_enable_fg1(jzlcd_info);
  __jzlcd_disable_fg0(jzlcd_info);

  if ( ! __jzlcd_is_ipu_enable(jzlcd_info) )
    __jzlcd_enable_ipu(jzlcd_info);

  if ( !__jzlcd_is_ipu_restart_enable(jzlcd_info))
    __jzlcd_enable_ipu_restart(jzlcd_info);

   __jzlcd_set_ipu_restart_val(jzlcd_info, 0x900);
   ret = ioctl(ipu_fbfd, FBIO_SET_MODE, &jzlcd_info);
#endif
}

int set_lcd_fg0(void)
{
#ifdef JZ4750_OPT
  int ret, fbfd;

  if ((fbfd = open("/dev/fb0", O_RDWR)) == -1) {
    printf("++ ERR: can't open /dev/fb0 ++\n");
    return 0;
  }

  ret = ioctl(fbfd, FBIO_GET_MODE, &jzlcd_info);
   __jzlcd_disable_fg1(jzlcd_info);
  __jzlcd_enable_fg0(jzlcd_info);

  if (__jzlcd_is_ipu_enable(jzlcd_info))
    __jzlcd_disable_ipu(jzlcd_info);

  if (__jzlcd_is_ipu_restart_enable(jzlcd_info))
    __jzlcd_disable_ipu_restart(jzlcd_info);

  ret = ioctl(fbfd, FBIODISPOFF, &jzlcd_info);
  ret = ioctl(fbfd, FBIO_DEEP_SET_MODE, &jzlcd_info);
  ret = ioctl(fbfd, FBIODISPON, &jzlcd_info);
  close(fbfd);
#endif

  return 0;
}

/* ================================================================================ */
static void  jz47_config_ipu_output_para (SwsContext *c, int ipu_line)
{
  int frame_offset, out_x, out_y;
  int rsize_w, rsize_h, outW, outH;
  int output_mode = jz47_ipu_module.output_mode;
  unsigned int out_fmt, dstFormat = c->dstFormat;
   
  /* Get the output parameter from struct.  */
  outW = jz47_ipu_module.resize_para.outW;
  outH = jz47_ipu_module.resize_para.outH;
  out_x = jz47_ipu_module.act_x;
  out_y = jz47_ipu_module.act_y;
  rsize_w = jz47_ipu_module.act_w;
  rsize_h = jz47_ipu_module.act_h;

  /* outW must < resize_w and outH must < resize_h.  */
  outW = (outW <= rsize_w) ? outW : rsize_w;
  outH = (outH <= rsize_h) ? outH : rsize_h;

  /* calc the offset for output.  */
  frame_offset = (out_x + out_y * jz47_ipu_module.out_panel.w) * jz47_ipu_module.out_panel.bpp_byte;
  out_fmt = OUTFMT_RGB565;  // default value

  /* clear some output control bits.  */
  disable_ipu_direct(ipu_vbase);

  switch (dstFormat)
  {
    case PIX_FMT_RGB32:
    case PIX_FMT_RGB32_1:
      out_fmt = OUTFMT_RGB888;
      outW = outW << 2;
      break;

    case PIX_FMT_RGB555:
      out_fmt = OUTFMT_RGB555;
      outW = outW << 1;
      break;

    case PIX_FMT_RGB565:
    case PIX_FMT_BGR565:
      out_fmt = OUTFMT_RGB565;
      outW = outW << 1;
      break;
  }

  /* clear the OUT_DATA_FMT control bits.  */
  REG32 (ipu_vbase + REG_D_FMT) &= ~(OUT_FMT_MASK);

  switch (output_mode)
  {
    case IPU_OUT_LCD:
      enable_ipu_direct(ipu_vbase);
      set_lcd_osd_for_ipu_thr ();
      break;

    case IPU_OUT_FB:
    default:
      set_lcd_fg0();
      REG32(ipu_vbase + REG_OUT_ADDR) = jz47_ipu_module.out_panel.output_phy + frame_offset;
      REG32(ipu_vbase + REG_OUT_STRIDE) = jz47_ipu_module.out_panel.bytes_per_line;
      break;
  }

  REG32(ipu_vbase + REG_OUT_GS) = OUT_FM_W (outW) | OUT_FM_H (outH);
  REG32 (ipu_vbase + REG_D_FMT) |= out_fmt;
  REG32 (ipu_vbase + REG_CTRL) |= CSC_EN;

  if (ipu_line){
    REG32 (ipu_vbase + REG_D_FMT) |= BLK_SEL;
    REG32 (ipu_vbase + REG_D_FMT) |= OUTFMT_OFTBGR;
  }
}


/* mode class A:  OUT_FB, OUT_MEM
   mode class B:  OUT_LCD
*/
/* ================================================================================ */
static int jz47_config_stop_ipu ()
{
  int switch_mode, runing_mode;

  /* Get the runing mode class.  */
  if (jz47_cpu_type == 4740
      || jz47_ipu_module.ipu_working_mode == IPU_OUT_FB
      || jz47_ipu_module.ipu_working_mode == IPU_OUT_MEM)
    runing_mode = 'A';
  else 
    runing_mode = 'B';
  
  /* Get the switch mode class.  */
  if (jz47_cpu_type == 4740
      || jz47_ipu_module.output_mode == IPU_OUT_FB
      || jz47_ipu_module.output_mode == IPU_OUT_MEM)
    switch_mode = 'A';
  else 
    switch_mode = 'B';

  /* Base on the runing_mode and switch_mode, disable lcd or stop ipu.  */
  if (runing_mode == 'A' && switch_mode == 'A')
  {
    while (ipu_is_enable(ipu_vbase) && (!polling_end_flag(ipu_vbase)))
      ;
    stop_ipu(ipu_vbase);
    clear_end_flag(ipu_vbase);
  }
  else
  {
    if (jz47_ipu_module.need_config_resize
        || jz47_ipu_module.need_config_inputpara
        || jz47_ipu_module.need_config_outputpara)
    {
      disable_lcd_ctrl ();
      while (ipu_is_enable(ipu_vbase) && (!polling_end_flag(ipu_vbase)))
        ;
      stop_ipu(ipu_vbase);
      clear_end_flag(ipu_vbase);
    } 

    /* Can change YUV address while IPU running (jz4755).  */
    if (switch_mode == 'A')
      disable_ipu_addrsel(ipu_vbase);
    else
      enable_ipu_addrsel(ipu_vbase);
  }
  return 1;
}

/* ================================================================================ */

static int  jz47_config_run_ipu ()
{
  int output_mode = jz47_ipu_module.output_mode;

  /* set the ipu working mode.  */
  jz47_ipu_module.ipu_working_mode = output_mode;

  if (jz47_cpu_type == 4740)
  {
    run_ipu (ipu_vbase);
    return 1;
  }

  /* run ipu for different output mode.  */
  switch (output_mode)
  {
    case IPU_OUT_FB:
    case IPU_OUT_MEM:
    default:
      run_ipu (ipu_vbase);
      break;

    case IPU_OUT_LCD:
      enable_yuv_ready(ipu_vbase);
      run_ipu (ipu_vbase);
      if (jz47_ipu_module.need_config_resize
          || jz47_ipu_module.need_config_inputpara
          || jz47_ipu_module.need_config_outputpara)
      {
        enable_lcd_ctrl ();
      } 
      break;
  }

  return 1;
}

/* ================================================================================ */
int jz47_put_image_with_ipu (struct vf_instance *vf, mp_image_t *mpi, double pts)
{
  SwsContext *c = vf->priv->ctx;

  /* Get the output panel information, calc the output position and shapes */
  if (!jz47_ipu_module_init)
  {
    if (! jz47_get_output_panel_info ())
      return 0;
    /* input size */
    jz47_ipu_module.srcW = c->srcW;
    jz47_ipu_module.srcH = c->srcH;
    /* output size */
    scale_outW = (!scale_outW) ? c->dstW : scale_outW;
    scale_outH = (!scale_outH) ? c->dstH : scale_outH;
    /* calculate input/output size. */
    jz47_calc_ipu_outsize_and_position (xpos, ypos, scale_outW, scale_outH);

    jz47_ipu_module.need_config_resize = 1;
    jz47_ipu_module.need_config_inputpara = 1;
    jz47_ipu_module.need_config_outputpara = 1;
    jz47_ipu_module_init = 1;

    while(!polling_end_flag(ipu_vbase));
    reset_ipu (ipu_vbase);
  }
  /* calculate resize parameter.  */
  if (jz47_ipu_module.need_config_resize)
    jz47_calc_resize_para ();

  /* Following codes is used to configure IPU, so we need stop the ipu.  */
  jz47_config_stop_ipu ();

  /* configure the input parameter.  */
  jz47_config_ipu_input_para (c, mpi);

  /* configure the resize parameter.  */
  if (jz47_ipu_module.need_config_resize)
    jz47_config_ipu_resize_para ();

  /* configure the output parameter.  */
  if (jz47_ipu_module.need_config_outputpara)
    jz47_config_ipu_output_para (c, mpi->ipu_line);

  /* flush dcache if need.  */
  {
    unsigned int img_area = c->srcW * c->srcH;
    if (img_area <= 320 * 240)
      memset (dcache, 0x2, 0x4000);
  }

  /* run ipu */
  jz47_config_run_ipu ();

  if (DEBUG_LEVEL > 1)
    jz47_dump_ipu_regs(-1);

  /* set some flag for normal.  */
  deinit_lcd_ctrl_fbfd ();
  jz47_ipu_module.need_config_resize = 0;
  jz47_ipu_module.need_config_inputpara = 0;
  jz47_ipu_module.need_config_outputpara = 0;
  ipu_image_completed = 1;
  return 1;
}

/* Following function is the interface.  */
/* ================================================================================ */

void free_jz47_tlb ()
{
  return;
}

void  ipu_image_start()
{
  jz47_free_alloc_mem();
  ipu_image_completed = 0;
  jz47_ipu_module_init = 0;
}

static int visual = 0;
void drop_image ()
{
  visual = 0;
}


#if ( defined(JZ4740_IPU) || defined(JZ4750_IPU) || defined(JZ4760_IPU) )
extern void ipu_mmap (void);
int jz47xx_put_image (struct vf_instance *vf, mp_image_t *mpi, double pts)
{
  mp_image_t *dmpi=mpi->priv;
  if (ipu_vbase == NULL)
    ipu_mmap ();

  if (!(mpi->flags & MP_IMGFLAG_DRAW_CALLBACK && dmpi))
  {
    if (mpi->pict_type == 1)  /* P_type */
      visual = 1;
    if (visual || (mpi->pict_type == 0)) /* I_type */
      jz47_put_image_with_ipu (vf, mpi, pts);
  }

  return 1;
}
#endif // ( defined(JZ4740_IPU) || defined(JZ4750_IPU) || defined(JZ4760_IPU) )

/* ================================================================================ */

void  ipu_outsize_changed(int x,int y,int w, int h)
{
  x = (x < 0) ? -1 : 0;
  y = (y < 0) ? -1 : 0;

  jz47_calc_ipu_outsize_and_position (x,  y, w, h);
}

void  ipu_outmode_changed(int x,int y,int w, int h, int mode)
{
  /* Set the output mode. */
  mode = (mode < 0) ? 0 : mode;
  jz47_ipu_module.output_mode = mode;

  /* get the output information.  */
  if (! jz47_get_output_panel_info ())
    return;

  /* caculate the x, y, w, h */
  ipu_outsize_changed(x, y,  w, h);
  jz47_ipu_module.need_config_resize = 1;
  jz47_ipu_module.need_config_outputpara = 1;
}


#endif /* defined(USE_JZ_IPU) && ( defined(JZ4740_IPU) || defined(JZ4750_IPU) || defined(JZ4760_IPU) ) */

