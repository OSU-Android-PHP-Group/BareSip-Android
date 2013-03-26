#
# modules.mk
#
# Copyright (C) 2010 Creytiv.com
#
# External libraries:
#
#   USE_ALSA          ALSA audio driver
#   USE_AMR           Adaptive Multi-Rate (AMR) audio codec
#   USE_BV32          BroadVoice32 Wideband Audio codec
#   USE_CAIRO         Cairo module
#   USE_CELT          CELT audio codec
#   USE_CONS          Console input driver
#   USE_COREAUDIO     MacOSX Coreaudio audio driver
#   USE_EVDEV         Event Device module
#   USE_FFMPEG        FFmpeg video codec libraries
#   USE_G711          G.711 audio codec
#   USE_G722          G.722 audio codec
#   USE_G722_1        G.722.1 audio codec
#   USE_GSM           GSM audio codec
#   USE_GST           Gstreamer audio module
#   USE_ILBC          iLBC audio codec
#   USE_L16           L16 audio codec
#   USE_MPG123        Use mpg123
#   USE_OPUS          Opus audio codec
#   USE_OSS           OSS audio driver
#   USE_PLC           Packet Loss Concealment
#   USE_PORTAUDIO     Portaudio audio driver
#   USE_SDL           libSDL video output
#   USE_SILK          SILK (Skype) audio codec
#   USE_SNDFILE       sndfile wav dumper
#   USE_SPEEX         Speex audio codec
#   USE_SPEEX_AEC     Speex Acoustic Echo Canceller
#   USE_SPEEX_PP      Speex preprocessor
#   USE_SPEEX_RESAMP  Speex Resampler
#   USE_SRTP          Secure RTP module
#   USE_STDIO         stdio input driver
#   USE_SYSLOG        Syslog module
#   USE_UUID          UUID module
#   USE_V4L           Video4Linux module
#   USE_V4L2          Video4Linux2 module
#   USE_WINWAVE       Windows audio driver
#   USE_X11           X11 video output
#


# Default is enabled
MOD_AUTODETECT := 1

ifneq ($(MOD_AUTODETECT),)

USE_CONS  := 1
USE_G711  := 1
USE_L16   := 1

ifneq ($(OS),win32)

USE_ALSA  := $(shell [ -f $(SYSROOT)/include/alsa/asoundlib.h ] || \
	[ -f $(SYSROOT_ALT)/include/alsa/asoundlib.h ] && echo "yes")
USE_AMR   := $(shell [ -d $(SYSROOT)/include/opencore-amrnb ] || \
	[ -d $(SYSROOT_ALT)/include/opencore-amrnb ] || \
	[ -d $(SYSROOT)/local/include/amrnb ] || \
	[ -d $(SYSROOT)/include/amrnb ] && echo "yes")
USE_BV32  := $(shell [ -f $(SYSROOT)/include/bv32/bv32.h ] || \
	[ -f $(SYSROOT)/local/include/bv32/bv32.h ] && echo "yes")
USE_CAIRO  := $(shell [ -f $(SYSROOT)/include/cairo/cairo.h ] || \
	[ -f $(SYSROOT_ALT)/include/cairo/cairo.h ] && echo "yes")
USE_CELT  := $(shell [ -f $(SYSROOT)/include/celt/celt.h ] || \
	[ -f $(SYSROOT)/local/include/celt/celt.h ] || \
	[ -f $(SYSROOT_ALT)/include/celt/celt.h ] && echo "yes")
USE_FFMPEG := $(shell [ -f $(SYSROOT)/include/libavcodec/avcodec.h ] || \
	[ -f $(SYSROOT)/local/include/libavcodec/avcodec.h ] || \
	[ -f $(SYSROOT)/include/ffmpeg/libavcodec/avcodec.h ] || \
	[ -f $(SYSROOT)/include/ffmpeg/avcodec.h ] || \
	[ -f $(SYSROOT)/local/ffmpeg/avcodec.h ] || \
	[ -f $(SYSROOT_ALT)/include/libavcodec/avcodec.h ] && echo "yes")
USE_G722 := $(shell [ -f $(SYSROOT)/include/spandsp/g722.h ] || \
	[ -f $(SYSROOT_ALT)/include/spandsp/g722.h ] || \
	[ -f $(SYSROOT)/local/include/spandsp/g722.h ] && echo "yes")
USE_G722_1 := $(shell [ -f $(SYSROOT)/include/g722_1.h ] || \
	[ -f $(SYSROOT_ALT)/include/g722_1.h ] || \
	[ -f $(SYSROOT)/local/include/g722_1.h ] && echo "yes")
USE_GSM := $(shell [ -f $(SYSROOT)/include/gsm.h ] || \
	[ -f $(SYSROOT_ALT)/include/gsm.h ] || \
	[ -f $(SYSROOT)/include/gsm/gsm.h ] || \
	[ -f $(SYSROOT)/local/include/gsm.h ] || \
	[ -f $(SYSROOT)/local/include/gsm/gsm.h ] && echo "yes")
USE_GST := $(shell [ -f $(SYSROOT)/include/gstreamer-0.10/gst/gst.h ] || \
	[ -f $(SYSROOT_ALT)/include/gstreamer-0.10/gst/gst.h ] && echo "yes")
USE_ILBC := $(shell [ -f $(SYSROOT)/include/iLBC_define.h ] || \
	[ -f $(SYSROOT)/local/include/iLBC_define.h ] && echo "yes")
USE_MPG123  := $(shell [ -f $(SYSROOT)/include/mpg123.h ] || \
	[ -f $(SYSROOT_ALT)/include/mpg123.h ] && echo "yes")
USE_OPUS := $(shell [ -f $(SYSROOT)/include/opus/opus.h ] || \
	[ -f $(SYSROOT_ALT)/include/opus/opus.h ] || \
	[ -f $(SYSROOT)/local/include/opus/opus.h ] && echo "yes")
USE_OSS := $(shell [ -f $(SYSROOT)/include/soundcard.h ] || \
	[ -f $(SYSROOT)/include/linux/soundcard.h ] || \
	[ -f $(SYSROOT)/include/sys/soundcard.h ] && echo "yes")
USE_PLC := $(shell [ -f $(SYSROOT)/include/spandsp/plc.h ] || \
	[ -f $(SYSROOT_ALT)/include/spandsp/plc.h ] || \
	[ -f $(SYSROOT)/local/include/spandsp/plc.h ] && echo "yes")
USE_PORTAUDIO := $(shell [ -f $(SYSROOT)/local/include/portaudio.h ] || \
		[ -f $(SYSROOT)/include/portaudio.h ] || \
		[ -f $(SYSROOT_ALT)/include/portaudio.h ] && echo "yes")
USE_SDL  := $(shell [ -f $(SYSROOT)/include/SDL/SDL.h ] || \
	[ -f $(SYSROOT)/local/include/SDL/SDL.h ] || \
	[ -f $(SYSROOT_ALT)/include/SDL/SDl.h ] && echo "yes")
USE_SILK := $(shell [ -f $(SYSROOT)/include/silk/SKP_Silk_SDK_API.h ] || \
	[ -f $(SYSROOT_ALT)/include/silk/SKP_Silk_SDK_API.h ] || \
	[ -f $(SYSROOT)/local/include/silk/SKP_Silk_SDK_API.h ] && echo "yes")
USE_SNDFILE := $(shell [ -f $(SYSROOT)/include/sndfile.h ] || \
	[ -f $(SYSROOT_ALT)/include/sndfile.h ] && echo "yes")
USE_STDIO := $(shell [ -f $(SYSROOT)/include/termios.h ] && echo "yes")
HAVE_SPEEXDSP := $(shell \
	[ -f $(SYSROOT)/local/lib/libspeexdsp$(LIB_SUFFIX) ] || \
	[ -f $(SYSROOT)/lib/libspeexdsp$(LIB_SUFFIX) ] || \
	[ -f $(SYSROOT_ALT)/lib/libspeexdsp$(LIB_SUFFIX) ] && echo "yes")
ifeq ($(HAVE_SPEEXDSP),)
HAVE_SPEEXDSP := \
	$(shell find $(SYSROOT)/lib -name libspeexdsp$(LIB_SUFFIX) 2>/dev/null)
endif
USE_SPEEX := $(shell [ -f $(SYSROOT)/include/speex.h ] || \
	[ -f $(SYSROOT)/include/speex/speex.h ] || \
	[ -f $(SYSROOT)/local/include/speex.h ] || \
	[ -f $(SYSROOT)/local/include/speex/speex.h ] || \
	[ -f $(SYSROOT_ALT)/include/speex/speex.h ] && echo "yes")
USE_SPEEX_AEC := $(shell [ -f $(SYSROOT)/include/speex/speex_echo.h ] || \
	[ -f $(SYSROOT)/local/include/speex/speex_echo.h ] || \
	[ -f $(SYSROOT_ALT)/include/speex/speex_echo.h ] && echo "yes")
USE_SPEEX_PP := $(shell [ -f $(SYSROOT)/include/speex_preprocess.h ] || \
	[ -f $(SYSROOT)/local/include/speex_preprocess.h ] || \
	[ -f $(SYSROOT)/local/include/speex/speex_preprocess.h ] || \
	[ -f $(SYSROOT_ALT)/include/speex/speex_preprocess.h ] || \
	[ -f $(SYSROOT)/include/speex/speex_preprocess.h ] && echo "yes")
USE_SPEEX_RESAMP := $(shell [ -f $(SYSROOT)/include/speex/speex_resampler.h ] \
	|| [ -f $(SYSROOT)/local/include/speex/speex_resampler.h ] \
	|| [ -f $(SYSROOT_ALT)/include/speex/speex_resampler.h ] \
	&& echo "yes")
USE_SRTP := $(shell [ -f $(SYSROOT)/include/srtp/srtp.h ] || \
	[ -f $(SYSROOT_ALT)/include/srtp/srtp.h ] || \
	[ -f $(SYSROOT)/local/include/srtp/srtp.h ] && echo "yes")
USE_SYSLOG := $(shell [ -f $(SYSROOT)/include/syslog.h ] || \
	[ -f $(SYSROOT_ALT)/include/syslog.h ] || \
	[ -f $(SYSROOT)/local/include/syslog.h ] && echo "yes")
USE_UUID  := $(shell [ -f $(SYSROOT)/include/uuid/uuid.h ] && echo "yes")
USE_V4L  := $(shell [ -f $(SYSROOT)/include/linux/videodev.h ] || \
	[ -f $(SYSROOT)/local/include/linux/videodev.h ] \
	&& echo "yes")
USE_V4L2  := $(shell [ -f $(SYSROOT)/include/libv4l2.h ] || \
	[ -f $(SYSROOT)/local/include/libv4l2.h ] \
	&& echo "yes")
USE_X11 := $(shell [ -f $(SYSROOT)/include/X11/Xlib.h ] || \
	[ -f $(SYSROOT)/local/include/X11/Xlib.h ] || \
	[ -f $(SYSROOT_ALT)/include/X11/Xlib.h ] && echo "yes")
USE_VPX  := $(shell [ -f $(SYSROOT)/include/vpx/vp8.h ] \
	|| [ -f $(SYSROOT)/local/include/vpx/vp8.h ] \
	|| [ -f $(SYSROOT_ALT)/include/vpx/vp8.h ] \
	&& echo "yes")
endif

# Platform specific modules
ifeq ($(OS),darwin)
USE_COREAUDIO := yes
USE_OPENGL    := yes

ifneq ($(USE_FFMPEG),)
ifneq ($(shell echo | $(CC) -E -dM - | grep '__LP64__'), )
LP64      := 1
endif

ifndef LP64
USE_QUICKTIME := yes
endif

endif

USE_QTCAPTURE := yes

endif
ifeq ($(OS),linux)
USE_EVDEV := $(shell [ -f $(SYSROOT)/include/linux/input.h ] && echo "yes")
endif
ifeq ($(OS),win32)
USE_WINWAVE := yes
endif

endif

# ------------------------------------------------------------------------- #

MODULES   += $(EXTRA_MODULES) stun turn ice natbd auloop vidloop presence
MODULES   += menu contact vumeter selfview

ifneq ($(USE_ALSA),)
MODULES   += alsa
endif
ifneq ($(USE_AMR),)
MODULES   += amr
endif
ifneq ($(USE_BV32),)
MODULES   += bv32
endif
ifneq ($(USE_CAIRO),)
MODULES   += cairo
ifneq ($(USE_MPG123),)
MODULES   += rst
endif
endif
ifneq ($(USE_CELT),)
MODULES   += celt
endif
ifneq ($(USE_CONS),)
MODULES   += cons
endif
ifneq ($(USE_COREAUDIO),)
MODULES   += coreaudio
endif
ifneq ($(USE_QUICKTIME),)
MODULES   += quicktime
endif
ifneq ($(USE_QTCAPTURE),)
MODULES   += qtcapture
CFLAGS    += -DQTCAPTURE_RUNLOOP
endif
ifneq ($(USE_EVDEV),)
MODULES   += evdev
endif
ifneq ($(USE_FFMPEG),)
USE_FFMPEG_AVFORMAT := 1
CFLAGS    += -I/usr/include/ffmpeg
CFLAGS    += -Wno-shadow -DUSE_FFMPEG
MODULES   += avcodec
ifneq ($(USE_FFMPEG_AVFORMAT),)
MODULES   += avformat
endif
endif
ifneq ($(USE_G711),)
MODULES   += g711
endif
ifneq ($(USE_G722),)
MODULES   += g722
endif
ifneq ($(USE_G722_1),)
MODULES   += g7221
endif
ifneq ($(USE_GSM),)
MODULES   += gsm
endif
ifneq ($(USE_GST),)
MODULES   += gst
endif
ifneq ($(USE_ILBC),)
MODULES   += ilbc
endif
ifneq ($(USE_L16),)
MODULES   += l16
endif
ifneq ($(USE_OPENGL),)
MODULES   += opengl
endif
ifneq ($(USE_OPENGLES),)
MODULES   += opengles
endif
ifneq ($(USE_OPUS),)
MODULES   += opus
endif
ifneq ($(USE_OSS),)
MODULES   += oss
endif
ifneq ($(USE_PLC),)
MODULES   += plc
endif
ifneq ($(USE_PORTAUDIO),)
MODULES   += portaudio
endif
ifneq ($(USE_SDL),)
MODULES   += sdl
endif
ifneq ($(USE_SILK),)
MODULES   += silk
endif
ifneq ($(USE_SNDFILE),)
MODULES   += sndfile
endif
ifneq ($(USE_SPEEX),)
MODULES   += speex
endif
ifneq ($(USE_SPEEX_AEC),)
MODULES   += speex_aec
endif
ifneq ($(USE_SPEEX_PP),)
MODULES   += speex_pp
endif
ifneq ($(USE_SPEEX_RESAMP),)
MODULES   += speex_resamp
endif
ifneq ($(USE_SRTP),)
MODULES   += srtp
endif
ifneq ($(USE_STDIO),)
MODULES   += stdio
endif
ifneq ($(USE_SYSLOG),)
MODULES   += syslog
endif
ifneq ($(USE_UUID),)
MODULES   += uuid
endif
ifneq ($(USE_V4L),)
MODULES   += v4l
endif
ifneq ($(USE_V4L2),)
MODULES   += v4l2
endif
ifneq ($(USE_VPX),)
MODULES   += vpx
endif
ifneq ($(USE_WINWAVE),)
MODULES   += winwave
endif
ifneq ($(USE_X11),)
MODULES   += x11 x11grab
endif
