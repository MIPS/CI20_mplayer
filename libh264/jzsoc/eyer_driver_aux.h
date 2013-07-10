#ifndef JZ_AUX_DRIVER_H
#define JZ_AUX_DRIVER_H

#define TCSM0_BASE 0x132B0000
#define TCSM1_BASE 0x132C0000
#define SRAM_BASE 0x132F0000

#define C_TCSM0_BASE (0x132B0000)
#define C_TCSM1_BASE (0xF4000000)
#define C_SRAM_BASE (0x132F0000)

#include "instructions.h"
#define aux_write_reg write_reg
#define aux_read_reg read_reg




void aux_end(unsigned int *end_ptr, int endvalue);
unsigned int aux_do_get_phy_addr(unsigned int vaddr);
#endif
