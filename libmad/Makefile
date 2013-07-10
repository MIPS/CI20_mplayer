JZ47_LINUX := 1
JZ4725_BSP := 0
JZ4740_BSP := 0
JZ4750_BSP := 0
JZ4755_BSP := 1

AR=mipsel-linux-ar
RANLIB=mipsel-linux-ranlib

ifeq ($(JZ47_LINUX), 1)

ifeq ($(HAVE_ANDROID_OS), 1)

CC := mipsel-linux-gcc -mips32 -D__LINUX__
CXX := mipsel-linux-gcc -mips32 -D__LINUX__

ifeq ($(JZ4750_BSP),1)
CC += -DJZ4750_OPT
CXX += -DJZ4750_OPT
endif
ifeq ($(JZ4755_BSP),1)
CC += -DJZ4750_OPT
CXX += -DJZ4750_OPT
endif

include ../config.mak

OPTFLAGS := -I. -I.. -I../libavutil -Wall -Wno-switch -Wpointer-arith -Wredundant-decls -O2   -pipe -ffast-math -fomit-frame-pointer -D_REENTRANT -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -DHAVE_CONFIG_H -D__LINUX__ $(EXTRA_INC) $(ANDROID_INC)

else

CC := mipsel-linux-gcc -mips32 
CXX := mipsel-linux-gcc -mips32 

ifeq ($(JZ4750_BSP),1)
CC += -DJZ4750_OPT
CXX += -DJZ4750_OPT
endif
ifeq ($(JZ4755_BSP),1)
CC += -DJZ4750_OPT
CXX += -DJZ4750_OPT
endif

OPTFLAGS := -I. -I.. -I../libavutil -Wall -Wno-switch -Wpointer-arith -Wredundant-decls -O2   -pipe -ffast-math -fomit-frame-pointer -D_REENTRANT -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -DHAVE_CONFIG_H $(EXTRA_INC) $(ANDROID_INC)

endif

else

include ../config.mak

endif

LIBNAME_COMMON = libmad.a
SRCS_DIR = ./libmad-0.15.1b
exported_headers = version.h fixed.h bit.h timer.h stream.h frame.h  \
			synth.h decoder.h

headers = $(exported_headers)  \
			global.h layer12.h layer3.h huffman.h

data_includes = D.dat imdct_s.dat qc_table.dat rq_table.dat  \
			sf_table.dat

libmad_la_SOURCES = version.c fixed.c bit.c timer.c stream.c frame.c  \
			synth.c decoder.c layer12.c layer3.c huffman.c  \
			
SRCS_COMMON = $(addprefix $(SRCS_DIR)/,$(notdir $(libmad_la_SOURCES)))
SRCS_DAT   = $(addprefix $(SRCS_DIR)/,$(notdir $(data_includes)))

OPTFLAGS += -DFPM_MIPS -DHAVE_MADD_ASM


OBJS_COMMON    = $(addsuffix .o, $(basename $(SRCS_COMMON)) )

CFLAGS += $(CFLAGS-yes) $(OPTFLAGS)

$(LIBNAME_COMMON):$(OBJS_COMMON)
	$(AR) r $@ $^ 
	$(RANLIB) $@
	#cp $@ ../../LIBS
	
###############################################################################


MIDS_MPLAYER_COMMON = $(OBJS_COMMON:.o=.s)
ASMS_MPLAYER_COMMON = $(OBJS_COMMON:.o=.mid)

$(OBJS_COMMON):%.o:%.s
	$(CC) $(ASFLAGS) -c -o $@ $<
$(MIDS_MPLAYER_COMMON):%.s:%.mid
	mxu_as $< > $@
$(ASMS_MPLAYER_COMMON):%.mid:%.c 
	$(CC) $(CFLAGS) -S -o $@ $<

###############################################################################


clean::
	rm -f $(OBJS_COMMON) *.a *~ $(SRCS_DIR)/*.s $(SRCS_DIR)/*.mid

distclean:: clean
	rm -f .depend test test2

dep depend::
	$(CC) -MM $(CFLAGS) $(OPTFLAGS) $(SRCS_COMMON) $(SRCS_MPLAYER) 1>.depend

-include .depend

.PHONY: libs clean distclean dep depend


