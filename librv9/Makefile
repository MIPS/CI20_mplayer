################################## PLATFORM #######################################
JZC_LINUX 	= 1
JZC_UCOS 	= 0
JZC_FPGA	= 0
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
RV9_DIR         =./

JZC_CFG 	= rv9_config.h
LIB		= librv9.a

JZSOC_HEAD 	= jzsoc/rv9_dcore.h
SRCS_VD_RV9 	= $(RV9_DIR)/rv40dsp.c\
		  $(SRCS_DIR)/mpegvideo.c\
	          $(SRCS_DIR)/error_resilience.c
OBJS_VD_RV9 	= $(addsuffix .o, $(basename $(SRCS_VD_RV9)) )

OBJS 		= $(OBJS_VD_RV9)
MIDS_VD_RV9 	= $(OBJS_VD_RV9:.o=.s)
ASMS_VD_RV9 	= $(OBJS_VD_RV9:.o=.mid)
OBJS		+=rv40.o
MIDS_VD_RV9 	+=rv40.s

CFLAGS 		+= -imacros $(JZC_CFG) -I$(RV9_DIR)/jzsoc -I. -I.. \
                   -I../libavcodec -I../libavutil

STATIC_OBJS 	:= $(OBJS)

EXT_BIN_DIR 	:= ./jzsoc/
EXT_BIN_NAME 	:= rv9_p1.bin
EXT_BIN		= $(join $(EXT_BIN_DIR),$(EXT_BIN_NAME))

all: 		$(LIB) $(EXT_BIN)

$(LIB):		$(STATIC_OBJS) $(JZSOC_HEAD)
		rm -f $@
		$(AR) rc $@ $^ 
		$(RANLIB) $@

$(OBJS)	      :%.o:%.s
		$(CC) $(CFLAGS) -c -o $@ $<
$(MIDS_VD_RV9):%.s:%.mid
		mxu_as $< > $@
rv40.mid      :rv40.c  $(JZSOC_HEAD) $(JZC_CFG)
		$(CC) $(CFLAGS) -S -o $@ $<
$(ASMS_VD_RV9):%.mid:%.c
		$(CC) $(CFLAGS) -S -o $@ $<

$(EXT_BIN):	$(EXT_BIN_DIR)
		rm -f $(EXT_BIN:.bin=.s)
		$(MAKE) -C $(EXT_BIN_DIR)
		cp -f $(EXT_BIN) ../
		@echo "Information: rv9 p1 bin generated successfully!"


clean::
		-rm -f *.o *~ *.a *.lib *.so *.so.* *.dylib *.dll *.s *.mid\
	      	*.def *.dll.a *.exp
		$(MAKE) -C $(EXT_BIN_DIR) clean
		rm -f ../$(EXT_BIN)
