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
