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
H264_ROOT 	= ../../../
#ICSYS_DIR       = /icdev/toolchain/linux/mipseltools-gcc412-lnx26/bin
ICSYS_DIR       = /icdev/toolchain/icsys/bin/
#============================= Tools define

LD 		= mipsel-linux-ld
CC 		= mipsel-linux-gcc 
COPY		= mipsel-linux-objcopy

#CC 		= $(ICSYS_DIR)./mipsisa32-gcc-3.3
#LD		= $(ICSYS_DIR)./mipsisa32-ld
#COPY		= $(ICSYS_DIR)./mipsisa32-objcopy

P1_SRC		= h264_p1.c
P1_BIN		= h264_p1.bin
P1_LIB		= h264_p1.lib
P1_OBJ		= h264_p1.o 
P1_MID		= $(P1_OBJ:.o=.mid.s)
P1_ASM		= $(P1_OBJ:.o=.s)

CV_P1_SRC		= h264_cavlc_p1.c
CV_P1_BIN		= h264_cavlc_p1.bin
CV_P1_LIB               = h264_cavlc_p1.lib
CV_P1_OBJ               = h264_cavlc_p1.o
CV_P1_MID		= $(CV_P1_OBJ:.o=.mid.s)
CV_P1_ASM		= $(CV_P1_OBJ:.o=.s)

GCC_ENDIAN_OPT	= -mel
LD_ENDIAN_OPT	= -EL
ABI_DIR		= el/mabi32

#GCC_EXTRA_OPT   = -Wall -Wno-switch -mips32 -Wpointer-arith -Wredundant-decls -O2 -pipe -ffast-math -fomit-frame-pointer
GCC_EXTRA_OPT   = -Wall -Wno-switch -mips32 -mno-abicalls -fno-pic -Wpointer-arith -Wredundant-decls -O2 -pipe -ffast-math -fomit-frame-pointer

#============================= Internal macro define 
JZC_MACRO	= h264_config.h

ifeq ($(JZ47_LINUX),1)
else
GCC_EXTRA_OPT += -DMINIOS
endif

# des changed to have 3 keys
H264_INC_OPT	= -I.. -I/usr/X11R6/include -I/opt/motif21/include 

GCC_OPT		= $(H264_INC_OPT) \
		  $(GCC_ENDIAN_OPT) -imacros $(JZC_MACRO) $(GCC_EXTRA_OPT) -fno-schedule-insns -fno-schedule-insns2

#============================= Make action define
libs: $(P1_BIN) $(CV_P1_BIN)

$(P1_OBJ):%.o:%.mid.s
	$(CC) $(GCC_OPT) -c -o 	$@ 	$<
$(P1_MID):%.mid.s:%.s
	source ../../libjzcommon/jz4760e_p1_gp $< >| tmp
	mxu_as tmp >| $@
	rm -f tmp
$(P1_ASM):%.s:%.c ../h264_config.h
	$(CC) $(GCC_OPT) -S -o 	$@ 	$<

$(P1_LIB): $(P1_OBJ) 
#	$(LD) -T h264_p1.ld -EL -nostdlib -o $@ $^
	$(LD) -T ../../libjzcommon/jz4760e_p1.ld -static -o $@ $^ -lc
$(P1_BIN): $(P1_LIB)
	$(COPY) -O binary $< 	$@

$(CV_P1_OBJ):%.o:%.mid.s
	$(CC) $(GCC_OPT) -c -o 	$@ 	$<
$(CV_P1_MID):%.mid.s:%.s
	source ../../libjzcommon/jz4760e_p1_gp $< >| tmp
	mxu_as tmp >| $@
	rm -f tmp
$(CV_P1_ASM):%.s:%.c ../h264_config.h
	$(CC) $(GCC_OPT) -S -o 	$@ 	$<

$(CV_P1_LIB): $(CV_P1_OBJ) 
#	$(LD) -T h264_p1.ld -EL -nostdlib -o $@ $^
	$(LD) -T ../../libjzcommon/jz4760e_p1.ld -static -o $@ $^ -lc
$(CV_P1_BIN): $(CV_P1_LIB)
	$(COPY) -O binary $< 	$@

dep depend::

clean::
	rm -f *.o *~ *.s *.bin *.lib

.PHONY: libs clean dep depend 
