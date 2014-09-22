# Copyright (C) 2006-2014 Ingenic Semiconductor CO.,LTD.
#
# This file is part of MPlayer.
#
# MPlayer is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# MPlayer is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with MPlayer; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#============================= Necessary Input  parameters
# dir:		T_FILE_DIR	I_TEST_DIR	CHIP_DIR	CCROOT
#		ELF_DIR		GCC_EXEC_PREFIX
# file:		MACRO_FILE
#
#============================= Optional Input parameters
# macro:	EXTRA_OBJ (for multi c file test like power test)
#		GCC_EXTRA_OPT	LD_EXTRA_OPT	MC_LIB_EXTRA	GCC_LIB_EXTRA
#		ENDIAN		GCC_OPTIMIZE_OPT

SHELL=/bin/bash		    # source command is only under bash shell, so we must define it(Kznan)

#============================= Path define
VC1_ROOT 	= ../../../
ICSYS_DIR       = /icdev/toolchain/icsys/bin/
#============================= Tools define

LD 		= mipsel-linux-ld
CC 		= mipsel-linux-gcc 
COPY		= mipsel-linux-objcopy

#CC 		= $(ICSYS_DIR)./mipsisa32-gcc-3.3
#LD		= $(ICSYS_DIR)./mipsisa32-ld
#COPY		= $(ICSYS_DIR)./mipsisa32-objcopy

P1_SRC		= ffmpeg2_p1.c 		        #@	src file
P1_BIN		= ffmpeg2_p1.bin		#@	bin file
P1_LIB		= ffmpeg2_p1.lib		#@	dst lib file
P1_OBJ		= ffmpeg2_p1.o 		        #@	obj file
P1_MID		= $(P1_OBJ:.o=.mid.s)	#@
P1_ASM		= $(P1_OBJ:.o=.s)	#@

GCC_ENDIAN_OPT	= -mel
LD_ENDIAN_OPT	= -EL
ABI_DIR		= el/mabi32

#GCC_EXTRA_OPT   = -Wall -Wno-switch -Wpointer-arith -Wredundant-decls -std=gnu99 -O2 -pipe -ffast-math -fomit-frame-pointer
GCC_EXTRA_OPT   = -Wall -Wno-switch -mips32 -mno-abicalls -fno-pic -Wpointer-arith -Wredundant-decls -O2 -pipe -ffast-math -fomit-frame-pointer

#============================= Internal macro define 
JZC_MACRO	= ffmpeg2_config.h	#@ control the mpeg2 using what 
JZC_MACRO1	=../../libjzcommon/com_config.h

ifeq ($(JZ47_LINUX),1)
else
GCC_EXTRA_OPT += -DMINIOS
endif

# des changed to have 3 keys
VC1_INC_OPT	= -I.. -I/usr/X11R6/include -I/opt/motif21/include 

GCC_OPT		= $(VC1_INC_OPT) \
		  $(GCC_ENDIAN_OPT) -imacros $(JZC_MACRO) -imacros  $(JZC_MACRO1) $(GCC_EXTRA_OPT)

#============================= Make action define
libs: $(P1_BIN)

$(P1_OBJ):%.o:%.mid.s
	$(CC) $(GCC_OPT) -c -o 	$@ 	$<

$(P1_MID):%.mid.s:%.s
	source ffmpeg2_p1_gp $< >| tmp
	mxu_as tmp >| $@
	rm -f tmp
$(P1_ASM):%.s:%.c
	$(CC) $(GCC_OPT) -S -o 	$@ 	$<

$(P1_LIB): $(P1_OBJ) 
	$(LD) -T ffmpeg2_p1.ld -static -o $@ $^ -lc
$(P1_BIN): $(P1_LIB)
	$(COPY) -O binary $< 	$@

dep depend::

clean::
	rm -f *.o *~ *.bin *.s *.lib

.PHONY: libs clean dep depend 
