#include "eyer_driver.h"
#include "errno.h"
#include "timer.h"
#undef printf
//#define AUX_EYER_SIM
unsigned int do_get_phy_addr(unsigned int vaddr);
int write_cnt = 0 ;
int read_cnt = 0 ;

void wait_aux_end(volatile unsigned int *end_ptr, int endvalue)
{
  unsigned begin = GetTimerMS();
  while(*end_ptr != endvalue){
    unsigned now = GetTimerMS();
    if (now - begin >= 300) break;
  }
  write_reg((end_ptr), 0);
}

#if 0  //@ cut
void load_aux_pro_bin(char * name, unsigned int addr)
{
  FILE *fp;
  //printf("load aux pro bin %s to %x-%x\n", name, (addr), do_get_phy_addr(addr));
  fp = fopen(name, "r+b");
  if (!fp)
    printf(" error while open %s \n", name);
  unsigned int but_ptr[1024];
  int len;
  int total_len;
  while ( len = fread(but_ptr, 4, 1024, fp) ){
    //printf(" len of aux task(word) = %d\n",len);
    int i;
    for ( i = 0; i<len; i++)
      {
	write_reg((addr), but_ptr[i]);
	addr = addr + 4;
      }
  }
  //printf("pro end address: %x-%x\n", (addr), do_get_phy_addr(addr));
  fclose(fp);
}
#endif //@ cut
unsigned int do_get_phy_addr(unsigned int vaddr){
  if ( vaddr >= (unsigned int)tcsm0_base && vaddr < (unsigned int)tcsm0_base + TCSM0__SIZE)
    {
      unsigned int ofst = vaddr - (unsigned int)tcsm0_base;
      return (ofst+TCSM0__OFFSET);
    }
  else if ( vaddr >= (unsigned int)tcsm1_base && vaddr < (unsigned int)tcsm1_base + TCSM1__SIZE)
    {
      unsigned int ofst = vaddr - (unsigned int)tcsm1_base;
      return (ofst+TCSM1__OFFSET);
    }
  else if ( vaddr >= (unsigned int)sram_base && vaddr < (unsigned int)sram_base + SRAM__SIZE)
    {
      unsigned int ofst = vaddr - (unsigned int)sram_base;
      return (ofst+SRAM__OFFSET);
    }
  else if ( vaddr >= (unsigned int)vpu_base && vaddr < (unsigned int)vpu_base + VPU__SIZE)
    {
      unsigned int ofst = vaddr - (unsigned int)vpu_base;
      return (ofst+VPU__OFFSET);
    }
  else if ( vaddr >= (unsigned int)gp0_base && vaddr < (unsigned int)gp0_base + GP0__SIZE)
    {
      unsigned int ofst = vaddr - (unsigned int)gp0_base;
      return (ofst+GP0__OFFSET);
    }
  else if ( vaddr >= (unsigned int)gp1_base && vaddr < (unsigned int)gp1_base + GP1__SIZE)
    {
      unsigned int ofst = vaddr - (unsigned int)gp1_base;
      return (ofst+GP1__OFFSET);
    }
  else if ( vaddr >= (unsigned int)gp2_base && vaddr < (unsigned int)gp2_base + GP2__SIZE)
    {
      unsigned int ofst = vaddr - (unsigned int)gp2_base;
      return (ofst+GP2__OFFSET);
    }
  else if ( vaddr >= (unsigned int)mc_base && vaddr < (unsigned int)mc_base + MC__SIZE)
    {
      unsigned int ofst = vaddr - (unsigned int)mc_base;
      return (ofst+MC__OFFSET);
    }
  else if ( vaddr >= (unsigned int)dblk0_base && vaddr < (unsigned int)dblk0_base + DBLK0__SIZE)
    {
      unsigned int ofst = vaddr - (unsigned int)dblk0_base;
      return (ofst+DBLK0__OFFSET);
    }
  else if ( vaddr >= (unsigned int)dblk1_base && vaddr < (unsigned int)dblk1_base + DBLK1__SIZE)
    {
      unsigned int ofst = vaddr - (unsigned int)dblk1_base;
      return (ofst+DBLK1__OFFSET);
    }
  else if ( vaddr >= (unsigned int)vmau_base && vaddr < (unsigned int)vmau_base + VMAU__SIZE)
    {
      unsigned int ofst = vaddr - (unsigned int)vmau_base;
      return (ofst+VMAU__OFFSET);
    }
  else if ( vaddr >= (unsigned int)aux_base && vaddr < (unsigned int)aux_base + AUX__SIZE)
    {
      unsigned int ofst = vaddr - (unsigned int)aux_base;
      return (ofst+AUX__OFFSET);
    }
  else if ( vaddr >= (unsigned int)sde_base && vaddr < (unsigned int)sde_base + SDE__SIZE)
    {
      unsigned int ofst = vaddr - (unsigned int)sde_base;
      return (ofst+SDE__OFFSET);
    }
  else 
    return (get_phy_addr(vaddr));
}


