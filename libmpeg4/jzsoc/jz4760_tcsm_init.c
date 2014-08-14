/*TCSM init is done by P0*/
#define TCSM0_BASE 0xF4000000
#define TCSM1_BASE 0xB32C0000
#define SRAM_BASE  0xB32D0000

#define TCSM0_SIZE 0x1000
#define TCSM1_SIZE 0x3000
#define SRAM_SIZE  0x1c00

#if defined(_FPGA_TEST_) || defined(_RTL_SIM_)
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#undef time
#undef rand
#undef srand
static void tcsm_init(){
  int i, *tcsm_ptr;
  srand(time(0));

  tcsm_ptr = (int *)TCSM0_BASE;
  for(i=0;i<TCSM0_SIZE;i++)
    tcsm_ptr[i] = (int)rand();

  tcsm_ptr = (int *)TCSM1_BASE;
  for(i=0;i<TCSM1_SIZE;i++)
    tcsm_ptr[i] = (int)rand();

  tcsm_ptr = (int *)SRAM_BASE;
  for(i=0;i<SRAM_SIZE;i++)
    tcsm_ptr[i] = (int)rand();
}

#else  /*application OS*/
//volatile unsigned char *tcsm0_base;
volatile unsigned char *tcsm1_base;
volatile unsigned char *sram_base;
static void tcsm_init(){
  int i, *tcsm_ptr;

  tcsm_ptr = (int *)TCSM0_BASE;
  for(i=0;i<TCSM0_SIZE;i++)
    tcsm_ptr[i] = 0x0;

  tcsm_ptr = (int *)tcsm1_base;
  for(i=0;i<TCSM1_SIZE;i++)
    tcsm_ptr[i] = 0x0;

  tcsm_ptr = (int *)sram_base;
  for(i=0;i<SRAM_SIZE;i++)
    tcsm_ptr[i] = 0x0;
}

#endif /*(_FPGA_TEST_) || (_RTL_SIM_)*/
