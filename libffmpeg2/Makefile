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
MPEG2_DIR       =./

JZC_CFG 	= ffmpeg2_config.h
LIB		= libffmpeg2.a

JZSOC_HEAD 	= jzsoc/ffmpeg2_dcore.h jzsoc/ffmpeg2_tcsm0.h
SRCS_VD_VC1 	= $(SRCS_DIR)/mpegvideo.c $(SRCS_DIR)/error_resilience.c
OBJS_VD_VC1 	= $(addsuffix .o, $(basename $(SRCS_VD_VC1)) )

OBJS 		= $(OBJS_VD_VC1)
MIDS_VD_VC1 	= $(OBJS_VD_VC1:.o=.s)
ASMS_VD_VC1 	= $(OBJS_VD_VC1:.o=.mid)
OBJS		+=ffmpeg2dec.o
MIDS_VD_VC1 	+=ffmpeg2dec.s

CFLAGS 		+= -imacros $(JZC_CFG) -I$(VC1_DIR)/jzsoc -I. -I.. \
                   -I../libavcodec -I../libavutil

STATIC_OBJS 	:= $(OBJS)

EXT_BIN_DIR 	:= ./jzsoc/
EXT_BIN_NAME 	:= ffmpeg2_p1.bin
EXT_BIN		= $(join $(EXT_BIN_DIR),$(EXT_BIN_NAME))

all: 		$(LIB) $(EXT_BIN)

$(LIB):		$(STATIC_OBJS) $(JZSOC_HEAD)
		rm -f $@
		$(AR) rc $@ $^ 
		$(RANLIB) $@

$(OBJS)	      :%.o:%.s
		$(CC) $(CFLAGS) -c -o $@ $<
$(MIDS_VD_VC1):%.s:%.mid
		mxu_as $< > $@
ffmpeg2dec.mid	:ffmpeg2dec.c $(JZSOC_HEAD)
		$(CC) $(CFLAGS) -S -o $@ $<
$(ASMS_VD_VC1):%.mid:%.c
		$(CC) $(CFLAGS) -S -o $@ $<

$(EXT_BIN):	$(EXT_BIN_DIR)
		rm -f $(EXT_BIN:.bin=.s)
		$(MAKE) -C $(EXT_BIN_DIR)
		cp -f $(EXT_BIN) ../
		@echo "Information: ffmpeg2 p1 bin generated successfully!"

clean::
		-rm -f *.o *~ *.a *.lib *.so *.so.* *.dylib *.dll *.s *.mid\
	      	*.def *.dll.a *.exp
		$(MAKE) -C $(EXT_BIN_DIR) clean
