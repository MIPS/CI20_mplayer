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
MPEG4_ROOT 	= ../../../
ICSYS_DIR       = /icdev/toolchain/icsys/bin/
#============================= Tools define

LD 		= mipsel-linux-ld
CC 		= mipsel-linux-gcc 
COPY		= mipsel-linux-objcopy

#CC 		= $(ICSYS_DIR)./mipsisa32-gcc-3.3
#LD		= $(ICSYS_DIR)./mipsisa32-ld
#COPY		= $(ICSYS_DIR)./mipsisa32-objcopy

P1_SRC		= mpeg4_p1.c
P1_BIN		= mpeg4_p1.bin
P1_LIB		= mpeg4_p1.lib
#P1_OBJ		= vp6_p1.o 
P1_OBJ		= mpeg4_p1.o
P1_MID		= $(P1_OBJ:.o=.mid.s)
P1_ASM		= $(P1_OBJ:.o=.s)

P1_MS_SRC	= msmpeg4_p1.c
P1_MS_BIN	= msmpeg4_p1.bin
P1_MS_LIB	= msmpeg4_p1.lib
P1_MS_OBJ	= msmpeg4_p1.o
P1_MS_MID	= $(P1_MS_OBJ:.o=.mid.s)
P1_MS_ASM	= $(P1_MS_OBJ:.o=.s)

GCC_ENDIAN_OPT	= -mel
LD_ENDIAN_OPT	= -EL
ABI_DIR		= el/mabi32

#GCC_EXTRA_OPT   = -Wall -Wno-switch -Wpointer-arith -Wredundant-decls -O2 -pipe -ffast-math -fomit-frame-pointer
GCC_EXTRA_OPT   = -Wall -Wno-switch -mips32 -mno-abicalls -fno-pic -Wpointer-arith -Wredundant-decls -O2 -pipe -ffast-math -fomit-frame-pointer

#============================= Internal macro define 
JZC_MACRO	= mpeg4_config.h

ifeq ($(JZ47_LINUX),1)
else
GCC_EXTRA_OPT += -DMINIOS
endif

# des changed to have 3 keys
MPEG4_INC_OPT	= -I.. -I/usr/X11R6/include -I/opt/motif21/include 

GCC_OPT		= $(MPEG4_INC_OPT) \
		  $(GCC_ENDIAN_OPT) -imacros $(JZC_MACRO) $(GCC_EXTRA_OPT)

#============================= Make action define
libs: $(P1_BIN) $(P1_MS_BIN)

$(P1_OBJ):%.o:%.mid.s
	$(CC) $(GCC_OPT) -c -o 	$@ 	$<
$(P1_MID):%.mid.s:%.s
	source mpeg4_p1_gp $< >| tmp
	mxu_as tmp >| $@
	rm -f tmp
$(P1_ASM):%.s:%.c
	$(CC) $(GCC_OPT) -S -o 	$@ 	$<

$(P1_LIB): $(P1_OBJ) 
	$(LD) -T mpeg4_p1.ld -static -o $@ $^ -L/usr/lib/mipsel-linux-gnu -L/usr/lib -lc
$(P1_BIN): $(P1_LIB)
	$(COPY) -O binary $< 	$@

$(P1_MS_OBJ):%.o:%.mid.s
	$(CC) $(GCC_OPT) -c -o 	$@ 	$<
$(P1_MS_MID):%.mid.s:%.s
	source mpeg4_p1_gp $< >| tmp
	mxu_as tmp >| $@
	rm -f tmp
$(P1_MS_ASM):%.s:%.c
	$(CC) $(GCC_OPT) -S -o 	$@ 	$<

$(P1_MS_LIB): $(P1_MS_OBJ) 
	$(LD) -T mpeg4_p1.ld -static -o $@ $^ -L/usr/lib/mipsel-linux-gnu -L/usr/lib -lc
$(P1_MS_BIN): $(P1_MS_LIB)
	$(COPY) -O binary $< 	$@

dep depend::

clean::
	rm -f *.o *~ *.s *.bin *.lib

.PHONY: libs clean dep depend 
