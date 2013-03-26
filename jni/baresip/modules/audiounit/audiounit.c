/**
 * @file audiounit.c  AudioUnit sound driver
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#include <re.h>
#include <baresip.h>
#include "audiounit.h"


AudioComponent output_comp = NULL;

static struct auplay *auplay;
static struct ausrc *ausrc;


#if TARGET_OS_IPHONE
static void interruptionListener(void *data, UInt32 inInterruptionState)
{
	(void)data;

	if (inInterruptionState == kAudioSessionBeginInterruption) {
		re_printf("audiounit interrupt: Begin\n");
		audiosess_interrupt(true);
	}
	else if (inInterruptionState == kAudioSessionEndInterruption) {
		re_printf("audiounit interrupt: End\n");
		audiosess_interrupt(false);
	}
}
#endif


static int module_init(void)
{
	AudioComponentDescription desc;
	int err;

#if TARGET_OS_IPHONE
	OSStatus ret;

	ret = AudioSessionInitialize(NULL, NULL, interruptionListener, 0);
	if (ret && ret != kAudioSessionAlreadyInitialized) {
		re_fprintf(stderr, "AudioSessionInitialize: %d\n", ret);
		return ENODEV;
	}
#endif

	desc.componentType = kAudioUnitType_Output;
#if TARGET_OS_IPHONE
	desc.componentSubType = kAudioUnitSubType_VoiceProcessingIO;
#else
	desc.componentSubType = kAudioUnitSubType_HALOutput;
#endif
	desc.componentManufacturer = kAudioUnitManufacturer_Apple;
	desc.componentFlags = 0;
	desc.componentFlagsMask = 0;

	output_comp = AudioComponentFindNext(NULL, &desc);
	if (!output_comp) {
		re_printf("audiounit: Voice Processing I/O not found\n");
		return ENOENT;
	}

	err  = auplay_register(&auplay, "audiounit", audiounit_player_alloc);
	err |= ausrc_register(&ausrc, "audiounit", audiounit_recorder_alloc);

	return 0;
}


static int module_close(void)
{
	ausrc  = mem_deref(ausrc);
	auplay = mem_deref(auplay);

	return 0;
}


EXPORT_SYM const struct mod_export DECL_EXPORTS(audiounit) = {
	"audiounit",
	"audio",
	module_init,
	module_close,
};
