/**
 * @file coreaudio.c  Apple Coreaudio sound driver
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <AudioToolbox/AudioToolbox.h>
#include <re.h>
#include <rem.h>
#include <baresip.h>
#include "coreaudio.h"


static struct auplay *auplay;
static struct ausrc *ausrc;


int audio_fmt(enum aufmt fmt)
{
	switch (fmt) {

	case AUFMT_S16LE: return kAudioFormatLinearPCM;
	case AUFMT_PCMA:  return kAudioFormatALaw;
	case AUFMT_PCMU:  return kAudioFormatULaw;
	default:
		re_fprintf(stderr, "coreaudio: unknown format %d\n", fmt);
		return -1;
	}
}


int bytesps(enum aufmt fmt)
{
	switch (fmt) {

	case AUFMT_S16LE: return 2;
	case AUFMT_PCMA:  return 1;
	case AUFMT_PCMU:  return 1;
	default: return 0;
	}
}


#if TARGET_OS_IPHONE && __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_2_0
static void interruptionListener(void *data, UInt32 inInterruptionState)
{
	(void)data;

	/* TODO: implement this properly */

	if (inInterruptionState == kAudioSessionBeginInterruption) {
		re_printf("coreaudio player interrupt: Begin\n");
	}
	else if (inInterruptionState == kAudioSessionEndInterruption) {
		re_printf("coreaudio player interrupt: End\n");
	}
}


int audio_session_enable(void)
{
	OSStatus res;
	UInt32 category;

	res = AudioSessionInitialize(NULL, NULL, interruptionListener, 0);
	if (res && res != 1768843636)
		return ENODEV;

	category = kAudioSessionCategory_PlayAndRecord;
	res = AudioSessionSetProperty(kAudioSessionProperty_AudioCategory,
				      sizeof(category), &category);
	if (res) {
		re_fprintf(stderr, "Audio Category: %d\n", res);
		return ENODEV;
	}

	res = AudioSessionSetActive(true);
	if (res) {
		re_fprintf(stderr, "AudioSessionSetActive: %d\n", res);
		return ENODEV;
	}

	return 0;
}


void audio_session_disable(void)
{
	AudioSessionSetActive(false);
}
#else
int audio_session_enable(void)
{
	return 0;
}


void audio_session_disable(void)
{
}
#endif


static int module_init(void)
{
	int err;

	err  = auplay_register(&auplay, "coreaudio", coreaudio_player_alloc);
	err |= ausrc_register(&ausrc, "coreaudio", coreaudio_recorder_alloc);

	return err;
}


static int module_close(void)
{
	auplay = mem_deref(auplay);
	ausrc  = mem_deref(ausrc);

	return 0;
}


EXPORT_SYM const struct mod_export DECL_EXPORTS(coreaudio) = {
	"coreaudio",
	"audio",
	module_init,
	module_close,
};
