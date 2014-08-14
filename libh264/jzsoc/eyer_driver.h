#ifndef EYER_DRIVER_H
#define EYER_DRIVER_H
#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <string.h> 
#include <signal.h>
#include <pthread.h>
//#include "EYE_LIB_DEF.h"
#include "stdarg.h"
#include "instructions.h"

#define TCSM0_BASE 0x132B0000
#define TCSM1_BASE 0x132C0000
#define SRAM_BASE 0x132F0000

#define C_TCSM0_BASE ((unsigned int)tcsm0_base)
#define C_TCSM1_BASE ((unsigned int)tcsm1_base)
#define C_SRAM_BASE ((unsigned int)sram_base)

#define __place_k0_data__ 

#define TCSM0_SIZE (16*1024/4)
#define TCSM1_SIZE (48*1024/4)
#define SRAM_SIZE (28*1024/4)

#define MC__OFFSET 0x13250000
#define VPU__OFFSET 0x13200000
#define CPM__OFFSET 0x10000000
#define AUX__OFFSET 0x132A0000
#define TCSM0__OFFSET 0x132B0000
#define TCSM1__OFFSET 0x132C0000
#define SRAM__OFFSET 0x132F0000
#define GP0__OFFSET 0x13210000
#define GP1__OFFSET 0x13220000
#define GP2__OFFSET 0x13230000
#define DBLK0__OFFSET 0x13270000
#define DBLK1__OFFSET 0x132D0000
#define SDE__OFFSET 0x13290000
#define VMAU__OFFSET 0x13280000


#define MC__SIZE   0x00001000
#define VPU__SIZE 0x00001000
#define CPM__SIZE 0x00001000
#define TCSM0__SIZE 0x00004000
#define TCSM1__SIZE 0x0000C000
#define SRAM__SIZE 0x00007000
#define AUX__SIZE 0x00001000

#define GP0__SIZE   0x00001000
#define GP1__SIZE   0x00001000
#define GP2__SIZE   0x00001000
#define DBLK0__SIZE   0x00001000
#define DBLK1__SIZE   0x00001000
#define SDE__SIZE   0x00001000
#define VMAU__SIZE 0x0000F000

extern volatile unsigned char *ipu_base;
extern volatile unsigned char *mc_base;
extern volatile unsigned char *vpu_base;
extern volatile unsigned char *cpm_base;
extern volatile unsigned char *lcd_base;
extern volatile unsigned char *gp0_base;
extern volatile unsigned char *gp1_base;
extern volatile unsigned char *gp2_base;
extern volatile unsigned char *vmau_base;
extern volatile unsigned char *dblk0_base;
extern volatile unsigned char *dblk1_base;
extern volatile unsigned char *sde_base;

extern volatile unsigned char *tcsm0_base;
extern volatile unsigned char *tcsm1_base;
extern volatile unsigned char *sram_base;
extern volatile unsigned char *ahb1_base;
extern volatile unsigned char *ddr_base;
extern volatile unsigned char *aux_base;


//#define get_phy_addr(a) (a)
unsigned int do_get_phy_addr(unsigned int vaddr);

//system(". ncsim_cmd&") ;						


#define AUX_START() 
void aux_clr();

#define SCORE_RST 0x1001
#define SCORE_RUN 0x1002
#define SCORE_STOP 0x1003
#define SCORE_DISP 0x1004
#define SCORE_CFG_T 0x1005
#define SCORE_DIS_M 0x1006

#define SCORE_SHOW_BUS 0x1
#define SCORE_SHOW_MODULE 0x2
#define SCORE_SHOW_ALL 0x3
#define SCORE_SHOW_BUSW 0x4

#define score_clr()\
({\
})\

#define score_enable()\
({\
})\

#define score_disable()\
({\
})\

#define score_disp()\
({\
})\

#define score_cfg_sample_T(T)\
({\
})\

#define score_dips_mod(MODE)\
({\
})\

#define AUX_VBASE        aux_base
#define aux_hard_start() \
({ write_reg(AUX_VBASE, 1);\
write_reg(AUX_VBASE, 2);\
})

#define aux_hard_reset() \
({ write_reg(AUX_VBASE, 1);\
})
#endif

void eyer_stop();
