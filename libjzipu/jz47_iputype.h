
#define IPU_LUT_LEN 32

/* struct IPU module to recorde some info */
struct JZ47_IPU_MOD {
  int out_x, out_y, out_w, out_h;   // require the output position and w,h by user
  int act_x, act_y, act_w, act_h;   // IPU actual output w,h and position
  int srcW, srcH;
  int ipu_working_mode; 
  int need_config_resize; // ipu need re-configure the resize registers
  int need_config_inputpara; // ipu need re-configure the input parameter registers
  int need_config_outputpara; // ipu need re-configure the output parameter registers
  int output_mode;  // output mode: 0 FrameBuf, 1: LCD (ipu through), 2: PAL TV, 3: NTSC TV, 8: memory
  int rsize_algorithm;  /* 0: liner, 1: bicube, 2: biliner.  */
  float rsize_bicube_level; //-2, -1, -0.75, -0.5
  struct IPU_output_panel
  {
    int w, h, bytes_per_line;
    int bpp_byte;
    unsigned int output_phy; // output physical addr
  } out_panel;
 struct IPU_rsize_para
 {
   int width_up, height_up, width_resize_enable, height_resize_enable;
   int width_lut_size, height_lut_size;
   int outW, outH, Wsel, Hsel; 
   unsigned int width_lut_coef [IPU_LUT_LEN];
   unsigned int height_lut_coef [IPU_LUT_LEN];
   unsigned int width_bicube_lut_coef  [IPU_LUT_LEN * 2];
   unsigned int height_bicube_lut_coef [IPU_LUT_LEN * 2];
 } resize_para;
};

struct Ratio_n2m
{
  float ratio;
  int n, m;
};

