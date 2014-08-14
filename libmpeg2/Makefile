# This Makefile is not necessary.
# $Id: Makefile,v 1.3 2012/09/10 03:49:03 kznan Exp $

################################## PLATFORM #######################################
JZC_LINUX 	= 1
JZC_UCOS 	= 0
JZC_FPGA        = 0
JZC_RTL		= 0

################################## TOOL CHAIN #####################################
AR 		= mipsel-linux-ar
LD 		= mipsel-linux-ld
CC 		= mipsel-linux-gcc 
RANLIB 		= mipsel-linux-ranlib

CFLAGS 		= -mips32 -O2 -fno-builtin \
		  -fno-exceptions -ffunction-sections -falign-functions=32 \
	   	  -fshort-wchar -fomit-frame-pointer -ffast-math -msoft-float -G 0 -Wall \
                     -mlong-calls -Wdisabled-optimization -Wno-switch \
		  -Wpointer-arith -Wredundant-decls -pipe -fomit-frame-pointer \
                     -D_REENTRANT -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 \
                     -D_LARGEFILE64_SOURCE -DHAVE_CONFIG_H -DHAVE_AV_CONFIG_H \
                     -D_ISOC9X_SOURCE

ifeq 		($(JZC_LINUX),1)
CFLAGS		+= -fPIC
CFLAGS		+= -D_LINUX_
endif

ifeq 		($(JZC_UCOS),1)
CFLAGS          += -mno-abicalls -fno-pic
CFLAGS		+= -D_UCOS_ 
endif

ifeq 		($(JZC_FPGA),1)
CFLAGS		+= -fPIC 
CFLAGS		+= -D_FPGA_TEST_
endif

ifeq 		($(JZC_RTL),1)
CFLAGS		+= -fPIC 
CFLAGS		+= -D_RTL_SIM_
endif

################################## COMPILE  DEF #####################################
SRCS_DIR 	= .

LIB		= libmpeg2.a

SRCS_VD_MPEG2 	= $(SRCS_DIR)/slice.c $(SRCS_DIR)/alloc.c  $(SRCS_DIR)/idct.c $(SRCS_DIR)/motion_comp.c\
		  $(SRCS_DIR)/cpu_accel.c $(SRCS_DIR)/cpu_state.c $(SRCS_DIR)/decode.c  $(SRCS_DIR)/header.c\

OBJS_VD_MPEG2 	= $(addsuffix .o, $(basename $(SRCS_VD_MPEG2)) )
MIDS_VD_MPEG2 	= $(OBJS_VD_MPEG2:.o=.s)
ASMS_VD_MPEG2 	= $(OBJS_VD_MPEG2:.o=.mid)

CFLAGS 		+= -I. -I.. -I../libavcodec -I../libavutil

all: 		$(LIB) Makefile

$(LIB): 	$(OBJS_VD_MPEG2)
		rm -f $@
		$(AR) rc $@ $^ 
		$(RANLIB) $@

$(OBJS_VD_MPEG2):%.o:%.s
		$(CC) $(CFLAGS) -c -o $@ $<
$(MIDS_VD_MPEG2):%.s:%.mid
		mxu_as $< > $@
$(ASMS_VD_MPEG2):%.mid:%.c
		$(CC) $(CFLAGS) -S -o $@ $<

clean::
		rm -f *.o *~ *.a *.lib *.so *.so.* *.dylib *.dll *.s *.mid *.def *.dll.a *.exp *.d
