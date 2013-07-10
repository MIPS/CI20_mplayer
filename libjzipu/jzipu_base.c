
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


//#define DEBUG_IPU_EXCUTE_TIME 1



#if 0 // ipu_mmap, jz47_soc_mem.c

#ifdef JZ4750_OPT
#include "jz4750_ipu_regops.h"
#else
#include "jz4740_ipu_regops.h"
#endif

extern unsigned char *ipu_vbase;
void ipu_mmap (void)
{
	int mmapfd = 0;
	/* open /dev/mem for map the memory.  */
	mmapfd = open ("/dev/mem", O_RDWR);
	if (mmapfd <= 0)
	{
		printf ("+++ Jz47 alloc frame failed: can not mmap the memory +++\n");
		return;
	}
 
	ipu_vbase = mmap((void *)0, IPU__SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
			 mmapfd, IPU__OFFSET);
	/* enable the IPU clock.  */
	*(volatile unsigned int *)(0xb0000020) &= ~(1 << 29);

//  close (mmapfd);
}

#endif


void jz47_ipu_exit(void)
{
	printf("jz4780_exit\n");
#if defined(JZ4780_IPU)
	jz4780_ipu_exit();
#endif
	
#if defined(JZ4780_X2D)
	jz4780_x2d_exit();
#endif
	
	return ;
}


#ifdef USE_JZ_IPU

static int jz47_put_image_internal (struct vf_instance *vf, mp_image_t *mpi, double pts)
{
	int ret_val;
	//printf("jz47_put_image() %d\n", __LINE__);
#if defined(USE_SW_COLOR_CONVERT_RENDER)
//#if 1
	//printf("jz47_put_image() %d\n", __LINE__);
	extern int jz47_put_image_sw (struct vf_instance *vf, mp_image_t *mpi, double pts);
	ret_val = jz47_put_image_sw (vf, mpi, pts);
	//printf("jz47_put_image() %d\n", __LINE__);
	return ret_val;
#endif	
	//printf("jz47_put_image() %d\n", __LINE__);
#if ( defined(JZ4740_IPU) || defined(JZ4750_IPU) || defined(JZ4760_IPU) )
	//printf("jz47_put_image() %d\n", __LINE__);
	extern int jz47xx_put_image (struct vf_instance *vf, mp_image_t *mpi, double pts);
	ret_val = jz47xx_put_image (vf, mpi, pts);
	//printf("jz47_put_image() %d\n", __LINE__);
#endif
#if defined(JZ4780_IPU)
	//printf("jz47_put_image() %d\n", __LINE__);
	extern int jz4780_put_image (struct vf_instance *vf, mp_image_t *mpi, double pts);
	ret_val = jz4780_put_image (vf, mpi, pts);
#endif
#if defined(JZ4780_X2D)
	//printf("jz47_put_image() %d\n", __LINE__);
	extern int jz4780_put_image_x2d (struct vf_instance *vf, mp_image_t *mpi, double pts);
	ret_val = jz4780_put_image_x2d (vf, mpi, pts);
#endif

	//printf("jz47_put_image() %d\n", __LINE__);
	return ret_val;
}


int jz47_put_image (struct vf_instance *vf, mp_image_t *mpi, double pts)
{
	int ret_val;
	/* trace jz47_put_image_internal excute time */
#if DEBUG_IPU_EXCUTE_TIME
	struct timeval tv_s, tv_e;
	gettimeofday(&tv_s, NULL);
#endif
	ret_val = jz47_put_image_internal(vf, mpi, pts);

#if DEBUG_IPU_EXCUTE_TIME
	gettimeofday(&tv_e, NULL);
	printf("jz47_put_image_internal() excute time: %d us\n", (int)((tv_e.tv_sec - tv_s.tv_sec)*1000000 + (tv_e.tv_usec - tv_s.tv_usec)));
#endif	
	return ret_val;
}

#endif //USE_JZ_IPU


