#include "eyer_driver_aux.h"

unsigned int aux_do_get_phy_addr(unsigned int vaddr){
  if ( (vaddr & 0xff000000 ) == 0xf4000000 )
    vaddr = ((vaddr & 0xffff) | 0x132c0000);
  return vaddr; 
}


unsigned int do_get_phy_addr(unsigned int vaddr){
  return vaddr; 
}


void aux_end(unsigned int *end_ptr, int endvalue)
{
  *(volatile unsigned int *)end_ptr = endvalue;
  //write_reg(aux_do_get_phy_addr(end_ptr), endvalue);
  i_nop;
  i_nop;
  i_nop;
  i_nop;
  i_nop;
  __asm__ __volatile__ ("wait");    
}


