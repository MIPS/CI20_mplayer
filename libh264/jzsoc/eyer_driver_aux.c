/*
 * Copyright (C) 2006-2014 Ingenic Semiconductor CO.,LTD.
 *
 * This file is part of MPlayer.
 *
 * MPlayer is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * MPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with MPlayer; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

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


