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
RV9_ROOT 	= ../../../
ICSYS_DIR       = /icdev/toolchain/icsys/bin/
#============================= Tools define

LD 		= ld
CC 		= gcc 
COPY		= objcopy

#CC 		= $(ICSYS_DIR)./mipsisa32-gcc-3.3
#LD		= $(ICSYS_DIR)./mipsisa32-ld
#COPY		= $(ICSYS_DIR)./mipsisa32-objcopy

P1_BIN		= rv9_p1.bin
P1_LIB		= rv9_p1.lib
P1_SRC		= rv9_p1.c
P1_OBJ		= rv9_p1.o

P1_MID		= $(P1_OBJ:.o=.mid.s)
P1_ASM		= $(P1_OBJ:.o=.s)

GCC_ENDIAN_OPT	= -mel
LD_ENDIAN_OPT	= -EL
ABI_DIR		= el/mabi32

#GCC_EXTRA_OPT   = -Wall -Wno-switch -Wpointer-arith -Wredundant-decls -O2 -pipe -ffast-math -fomit-frame-pointer
GCC_EXTRA_OPT   = -Wall -Wno-switch -mips32 -mno-abicalls -fno-pic -Wpointer-arith -Wredundant-decls -O2 -pipe -ffast-math -fomit-frame-pointer

#============================= Internal macro define 
JZC_MACRO	= rv9_config.h

ifeq ($(JZ47_LINUX),1)
else
GCC_EXTRA_OPT += -DMINIOS
endif

# des changed to have 3 keys
RV9_INC_OPT	= -I.. -I/usr/X11R6/include -I/opt/motif21/include 

GCC_OPT		= $(RV9_INC_OPT) \
		  $(GCC_ENDIAN_OPT) -imacros $(JZC_MACRO) $(GCC_EXTRA_OPT) -fno-schedule-insns -fno-schedule-insns2

#============================= Make action define
libs: $(P1_BIN)

$(P1_BIN): $(P1_LIB)
	$(COPY) -O binary $< 	$@
$(P1_LIB): $(P1_OBJ) 
	$(LD) -T ../../libjzcommon/jz4760e_p1.ld -static -o $@ $^ -L/usr/lib/mipsel-linux-gnu -L/usr/lib -lc
#	$(LD) -T rv9_p1.ld -EL -nostdlib -o $@ $^

$(P1_OBJ):%.o:%.mid.s
	$(CC) $(GCC_OPT) -c -o 	$@ 	$<
$(P1_MID):%.mid.s:%.s
	source ../../libjzcommon/jz4760e_p1_gp $< >| tmp
	mxu_as tmp >| $@
	rm -f tmp
$(P1_ASM):%.s:%.c
	$(CC) $(GCC_OPT) -S -o 	$@ 	$<

dep depend::

clean::
	rm -f *.o *~ *.s *.bin *.lib

.PHONY: libs clean dep depend 
