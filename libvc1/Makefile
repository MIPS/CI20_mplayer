################################## PLATFORM #######################################
JZC_LINUX 	= 0
JZC_UCOS 	= 0
JZC_FPGA	= 1
JZC_RTL		= 0

################################## TOOL CHAIN #####################################
AR 		= mipsel-linux-ar
LD 		= mipsel-linux-ld
CC 		= mipsel-linux-gcc 
RANLIB 		= mipsel-linux-ranlib

CFLAGS 		= -mips32 -std=gnu99 -O2 -fno-builtin \
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
CFLAGS		+= -mno-abicalls -fno-pic
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
SRCS_DIR 	= ../libavcodec
VC1_DIR         =./

JZC_CFG 	= vc1_config.h
LIB		= libvc1.a

SRCS_VD_VC1 	= $(VC1_DIR)/vc1.c $(VC1_DIR)/vc1dec.c \
	          $(SRCS_DIR)/msmpeg4data.c   $(VC1_DIR)/vc1data.c\
	          $(VC1_DIR)/vc1dsp.c $(VC1_DIR)/vc1_parser.c\
	          $(SRCS_DIR)/msmpeg4.c 	\
	          $(SRCS_DIR)/intrax8.c       $(SRCS_DIR)/intrax8dsp.c\
		  $(SRCS_DIR)/mpeg4video.c    $(SRCS_DIR)/mpegvideo.c\
	          $(SRCS_DIR)/h263.c          $(SRCS_DIR)/error_resilience.c
OBJS_VD_VC1 	= $(addsuffix .o, $(basename $(SRCS_VD_VC1)) )

OBJS 		= $(OBJS_VD_VC1)
MIDS_VD_VC1 	= $(OBJS_VD_VC1:.o=.s)
ASMS_VD_VC1 	= $(OBJS_VD_VC1:.o=.mid)

CFLAGS 		+= -imacros $(JZC_CFG) -I. -I.. \
                   -I../libavcodec -I../libavutil

STATIC_OBJS 	:= $(OBJS)

all: 		$(LIB)

$(LIB):		$(STATIC_OBJS)
		rm -f $@
		$(AR) rc $@ $^ 
		$(RANLIB) $@

$(OBJS)	      :%.o:%.s
		$(CC) $(CFLAGS) -c -o $@ $<
$(MIDS_VD_VC1):%.s:%.mid
		mxu_as $< > $@
# vc1dec.mid    :vc1dec.c      # if we use mxu, then we make this file in another way
# 		$(CC) $(CFLAGS) -S -o $@ $<
$(ASMS_VD_VC1):%.mid:%.c
		$(CC) $(CFLAGS) -S -o $@ $<

clean::
		-rm -f *.o *~ *.a *.lib *.so *.so.* *.dylib *.dll *.s *.mid\
	      	*.def *.dll.a *.exp
