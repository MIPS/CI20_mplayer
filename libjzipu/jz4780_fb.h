#include <linux/fb.h>
//#include <linux/earlysuspend.h>

#ifdef CONFIG_JZ4780_AOSD
#include "aosd.h"
#endif

#define NUM_FRAME_BUFFERS 2

#define PIXEL_ALIGN 16
#define MAX_DESC_NUM 4

/**
 * @next: physical address of next frame descriptor
 * @databuf: physical address of buffer
 * @id: frame ID
 * @cmd: DMA command and buffer length(in word)
 * @offsize: DMA off size, in word
 * @page_width: DMA page width, in word
 * @cpos: smart LCD mode is commands' number, other is bpp,
 * premulti and position of foreground 0, 1
 * @desc_size: alpha and size of foreground 0, 1
 */
struct jzfb_framedesc {
	uint32_t next;
	uint32_t databuf;
	uint32_t id;
	uint32_t cmd;
	uint32_t offsize;
	uint32_t page_width;
	uint32_t cpos;
	uint32_t desc_size;
} __packed;

struct jzfb_display_size {
	uint32_t fg0_line_size;
	uint32_t fg0_frm_size;
	uint32_t fg1_line_size;
	uint32_t fg1_frm_size;
	uint32_t panel_line_size;
	uint32_t height_width;
	uint32_t fg1_height_width;
};
#if 0
enum jzfb_format_order {
	FORMAT_X8R8G8B8 = 1,
	FORMAT_X8B8G8R8,
};
#endif
/**
 * @fg: foreground 0 or foreground 1
 * @bpp: foreground bpp
 * @x: foreground start position x
 * @y: foreground start position y
 * @w: foreground width
 * @h: foreground height
 */
struct jzfb_fg_t {
	uint32_t fg;
	uint32_t bpp;
	uint32_t x;
	uint32_t y;
	uint32_t w;
	uint32_t h;
};
#if 0
/**
 *@decompress: enable decompress function, used by FG0
 *@block: enable 16x16 block function
 *@fg0: fg0 info
 *@fg1: fg1 info
 */
struct jzfb_osd_t {
	int decompress;
	int block;
	struct jzfb_fg_t fg0;
	struct jzfb_fg_t fg1;
};
#endif
#if 0
struct jzfb {
	int id;           /* 0, lcdc0  1, lcdc1 */
	int is_enabled;   /* 0, disable  1, enable */
	int irq;          /* lcdc interrupt num */
	/* need_syspan
	 * 0: not need system pan display only hdmi (use in only hdmi)
	 * 1: need system pan display (used in lcd or(lcd and hdmi))
	 * */
	int need_syspan;
	int open_cnt;
	int irq_cnt;
	int desc_num;
	char clk_name[16];
	char pclk_name[16];
	char irq_name[16];

	struct fb_info *fb;
	struct device *dev;
	struct jzfb_platform_data *pdata;
	void __iomem *base;
	struct resource *mem;

	size_t vidmem_size;
	void *vidmem;
	int vidmem_phys;
	void *desc_cmd_vidmem;
	int desc_cmd_phys;

	int frm_size;
	int current_buffer;
	/* dma 0 descriptor base address */
	struct jzfb_framedesc *framedesc[MAX_DESC_NUM];
	struct jzfb_framedesc *fg1_framedesc; /* FG 1 dma descriptor */
	int framedesc_phys;
	struct mutex framedesc_lock;

	wait_queue_head_t vsync_wq;
	struct task_struct *vsync_thread;
	ktime_t	vsync_timestamp;
	unsigned int vsync_skip_map; /* 10 bits width */
	int vsync_skip_ratio;
	int pan_sync; /* frame update sync */

	struct mutex lock;
	struct mutex suspend_lock;

	enum jzfb_format_order fmt_order; /* frame buffer pixel format order */
	struct jzfb_osd_t osd; /* osd's config information */

	struct clk *clk;
	struct clk *pclk;
	struct clk *ipu_clk;
	struct clk *hdmi_clk;
	struct clk *hdmi_pclk;

#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
	int is_suspend;
};
#endif

/* structures for frame buffer ioctl */
struct jzfb_fg_pos {
	__uint32_t fg; /* 0:fg0, 1:fg1 */
	__uint32_t x;
	__uint32_t y;
};

struct jzfb_fg_size {
	__uint32_t fg;
	__uint32_t w;
	__uint32_t h;
};

struct jzfb_fg_alpha {
	__uint32_t fg; /* 0:fg0, 1:fg1 */
	__uint32_t enable;
	__uint32_t mode; /* 0:global alpha, 1:pixel alpha */
	__uint32_t value; /* 0x00-0xFF */
};

struct jzfb_bg {
	__uint32_t fg; /* 0:fg0, 1:fg1 */
	__uint32_t red;
	__uint32_t green;
	__uint32_t blue;
};

struct jzfb_color_key {
	__uint32_t fg; /* 0:fg0, 1:fg1 */
	__uint32_t enable;
	__uint32_t mode; /* 0:color key, 1:mask color key */
	__uint32_t red;
	__uint32_t green;
	__uint32_t blue;
};

struct jzfb_mode_res {
	__uint32_t index; /* 1-64 */
	__uint32_t w;
	__uint32_t h;
};

struct jzfb_aosd {
	__uint32_t aosd_enable;
	__uint32_t with_alpha;
	__uint32_t buf_phys_addr;
	__uint32_t buf_virt_addr;
	__uint32_t buf_size;
};

/* ioctl commands base fb.h FBIO_XXX */
/* image_enh.h: 0x142 -- 0x162 */
#define JZFB_GET_MODENUM		_IOR('F', 0x100, int)
#define JZFB_GET_MODELIST		_IOR('F', 0x101, int)
#define JZFB_SET_VIDMEM			_IOW('F', 0x102, unsigned int *)
#define JZFB_SET_MODE			_IOW('F', 0x103, int)
#define JZFB_ENABLE			_IOW('F', 0x104, int)
#define JZFB_GET_RESOLUTION		_IOWR('F', 0x105, struct jzfb_mode_res)

/* Reserved for future extend */
#define JZFB_SET_FG_SIZE		_IOW('F', 0x116, struct jzfb_fg_size)
#define JZFB_GET_FG_SIZE		_IOWR('F', 0x117, struct jzfb_fg_size)
#define JZFB_SET_FG_POS			_IOW('F', 0x118, struct jzfb_fg_pos)
#define JZFB_GET_FG_POS			_IOWR('F', 0x119, struct jzfb_fg_pos)
#define JZFB_GET_BUFFER			_IOR('F', 0x120, int)
#define JZFB_GET_LCDC_ID	       	_IOR('F', 0x121, int)
/* Reserved for future extend */
#define JZFB_SET_ALPHA			_IOW('F', 0x123, struct jzfb_fg_alpha)
#define JZFB_SET_BACKGROUND		_IOW('F', 0x124, struct jzfb_bg)
#define JZFB_SET_COLORKEY		_IOW('F', 0x125, struct jzfb_color_key)
#define JZFB_AOSD_EN			_IOW('F', 0x126, struct jzfb_aosd)
#define JZFB_16X16_BLOCK_EN		_IOW('F', 0x127, int)
//#define JZFB_IPU0_TO_BUF		_IOW('F', 0x128, int)
//#define JZFB_ENABLE_IPU_CLK		_IOW('F', 0x129, int)
#define JZFB_ENABLE_LCDC_CLK		_IOW('F', 0x130, int)
/* Reserved for future extend */
#define JZFB_ENABLE_FG0			_IOW('F', 0x139, int)
#define JZFB_ENABLE_FG1			_IOW('F', 0x140, int)

/* Reserved for future extend */
#define JZFB_SET_VSYNCINT		_IOW('F', 0x210, int)

#define JZFB_SET_PAN_SYNC		_IOW('F', 0x220, int)

#define JZFB_SET_NEED_SYSPAN	_IOR('F', 0x310, int)
#define NOGPU_PAN				_IOR('F', 0x311, int)
