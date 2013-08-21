#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "config.h"

#define LINUX_MULTICORE
#ifdef LINUX_MULTICORE
// In mipsel tools, macro _GNU_SOURCE can not enable __USE_GNU
// So we enable __USE_GNU immediatly, not like man
#define __USE_GNU
#include <sys/syscall.h>   /* For SYS_xxx definitions */
#include <sched.h>
#include <unistd.h>
#endif

#ifdef TIME_OUT_TEST
static unsigned int GetTimer(void)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}
#endif

#define MC__OFFSET 0x13250000
#define VPU__OFFSET 0x13200000
#define CPM__OFFSET 0x10000000
#define AUX__OFFSET 0x132A0000
#define TCSM0__OFFSET 0x132B0000
#define TCSM1__OFFSET 0x132C0000
#define SRAM__OFFSET 0x132F0000
#define GP0__OFFSET 0x13210000
#define DBLK0__OFFSET 0x13270000
#define DBLK1__OFFSET 0x132D0000
#define SDE__OFFSET 0x13290000
#define VMAU__OFFSET 0x13280000


#define MC__SIZE   0x00001000
#define VPU__SIZE 0x00001000
#define CPM__SIZE 0x00001000
#define AUX__SIZE 0x00004000
#define TCSM0__SIZE 0x00004000
#define TCSM1__SIZE 0x0000C000
#define SRAM__SIZE 0x00007000

#define GP0__SIZE   0x00001000
#define DBLK0__SIZE   0x00001000
#define DBLK1__SIZE   0x00001000
#define SDE__SIZE   0x00010000
#define VMAU__SIZE 0x0000F000

int vae_fd;
int tcsm_fd;
volatile unsigned char *ipu_base;
volatile unsigned char *mc_base;
volatile unsigned char *vpu_base;
volatile unsigned char *cpm_base;
volatile unsigned char *lcd_base;
volatile unsigned char *gp0_base;
volatile unsigned char *vmau_base;
volatile unsigned char *dblk0_base;
volatile unsigned char *dblk1_base;
volatile unsigned char *sde_base;

volatile unsigned char *tcsm0_base;
volatile unsigned char *tcsm1_base;
volatile unsigned char *sram_base;
volatile unsigned char *ahb1_base;
volatile unsigned char *ddr_base;
volatile unsigned char *aux_base;

#define CPM_VPU_SWRST    (cpm_base + 0xC4)
#define CPM_VPU_SR     	 (0x1<<31)
#define CPM_VPU_STP    	 (0x1<<30)
#define CPM_VPU_ACK    	 (0x1<<29)

#define write_cpm_reg(a)    (*(volatile unsigned int *)(CPM_VPU_SWRST) = a)
#define read_cpm_reg()      (*(volatile unsigned int *)(CPM_VPU_SWRST))

#define RST_VPU()							\
    {									\
	write_cpm_reg(read_cpm_reg() | CPM_VPU_STP);			\
	while( !(read_cpm_reg() & CPM_VPU_ACK) );			\
	write_cpm_reg( (read_cpm_reg() | CPM_VPU_SR) & (~CPM_VPU_STP) ); \
	write_cpm_reg( read_cpm_reg() & (~CPM_VPU_SR) & (~CPM_VPU_STP) ); \
    }

void reset_vpu(void)
{
	RST_VPU();
}

static void * safe_map(int fd, unsigned int offset, unsigned int size)
{
    void * vaddr = mmap((void *)0, size, PROT_READ | PROT_WRITE, MAP_SHARED, vae_fd, offset);
    if (vaddr == MAP_FAILED) {
	printf("Map 0x%08x addr with 0x%x size failed.\n", offset, size);
	return NULL;
    }
    return vaddr;
}

#ifdef JZC_HW_MEDIA
void VAE_map() {

#ifdef LINUX_MULTICORE
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask);
    if ( sched_setaffinity(syscall(SYS_gettid), sizeof(mask), &mask) == -1 ){
	printf("[CPU] sched_setaffinity fail");
	return -1;
    }
#endif

      /* open and map flash device */
    vae_fd = open("/dev/mem", O_RDWR | O_SYNC);
      //asm("break %0"::"i"(PMON_ON));
      // tricky appoach to use TCSM
    tcsm_fd = open("/dev/jz-vpu", O_RDWR);
    if (vae_fd < 0) {
	printf("open /dev/mem error.\n");
	exit(1);
    }

    if (tcsm_fd < 0) {
	printf("open /dev/jz-vpu error.\n");
	exit(1);
    }

    mc_base     = safe_map(vae_fd, MC__OFFSET    , MC__SIZE);
    sde_base    = safe_map(vae_fd, SDE__OFFSET   , SDE__SIZE);
    dblk1_base  = safe_map(vae_fd, DBLK1__OFFSET , DBLK1__SIZE);
    dblk0_base  = safe_map(vae_fd, DBLK0__OFFSET , DBLK0__SIZE);
    vmau_base   = safe_map(vae_fd, VMAU__OFFSET  , VMAU__SIZE);
    vpu_base    = safe_map(vae_fd, VPU__OFFSET   , VPU__SIZE);
    aux_base    = safe_map(vae_fd, AUX__OFFSET   , AUX__SIZE);
    cpm_base    = safe_map(vae_fd, CPM__OFFSET   , CPM__SIZE);
    gp0_base    = safe_map(vae_fd, GP0__OFFSET   , GP0__SIZE); 
    tcsm0_base  = safe_map(vae_fd, TCSM0__OFFSET , TCSM0__SIZE); 
    tcsm1_base  = safe_map(vae_fd, TCSM1__OFFSET , TCSM1__SIZE); 
    sram_base   = safe_map(vae_fd, SRAM__OFFSET  , SRAM__SIZE);

#ifdef TIME_OUT_TEST
    unsigned int start_time = GetTimer();
    printf("Start time test\n");
    int time_out = 0;
    while( (*(volatile unsigned int *)(vpu_base + 0x34) & 0x1) == 0x0 ) {
	time_out++;
	if(time_out > 10000000){
	    printf("- Waiting AUX for too long times, maybe it deads.\n");
	    break;
	}
    }
    start_time = GetTimer() - start_time;
    printf("circle 100000000 used %d us\n", start_time);
#endif

    RST_VPU();
    printf("VAE mmap successfully done!\n");
}

void VAE_unmap() {
    munmap(mc_base, MC__SIZE);
    munmap(vpu_base, VPU__SIZE);
    munmap(cpm_base, CPM__SIZE);
    munmap(sram_base, SRAM__SIZE);
    sram_base = 0 ;  
      //asm("break %0"::"i"(PMON_OFF));
    printf("VAE unmap successfully done!\n");
    close(vae_fd);
    close(tcsm_fd);
}
#endif
