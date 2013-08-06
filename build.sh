#!/bin/bash

./configure --enable-sdl --enable-x11 --enable-gl --disable-vm --disable-termcap \
	--disable-freetype --enable-fbdev --enable-alsa --enable-ossaudio \
	--disable-esd --disable-ivtv --enable-hardcoded-tables --disable-mencoder \
	--disable-mp3lib --enable-mad --disable-speex --disable-iconv --disable-faac \
	--disable-faac-lavc --enable-pvr --disable-tv-v4l1 --enable-tv-v4l2 \
	--extra-ldflags="-L./libmad" \
	--extra-cflags="-mips32 -I./libmad/libmad-0.15.1b/ -imacros ./libjzcommon/com_config.h" \
	--enable-debug --enable-gui
