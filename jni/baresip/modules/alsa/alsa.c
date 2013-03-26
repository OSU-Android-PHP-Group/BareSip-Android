/**
 * @file alsa.c  ALSA sound driver
 *
 * Copyright (C) 2010 Creytiv.com
 */
#define _POSIX_SOURCE 1
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include <alsa/asoundlib.h>
#include <re.h>
#include <rem.h>
#include <baresip.h>
#include "alsa.h"


#define DEBUG_MODULE "alsa"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


char alsa_dev[64] = "default";

static struct ausrc *ausrc;
static struct auplay *auplay;


static inline snd_pcm_format_t audio_fmt(enum aufmt fmt)
{
	switch (fmt) {

	default:
	case AUFMT_S16LE: return SND_PCM_FORMAT_S16_LE;
	case AUFMT_PCMU:  return SND_PCM_FORMAT_MU_LAW;
	case AUFMT_PCMA:  return SND_PCM_FORMAT_A_LAW;
	}
}


int alsa_reset(snd_pcm_t *pcm, uint32_t srate, uint32_t ch, enum aufmt fmt,
	       uint32_t frame_size)
{
	snd_pcm_hw_params_t *hw_params = NULL;
	const snd_pcm_format_t pcmfmt = audio_fmt(fmt);
	snd_pcm_uframes_t period = frame_size, bufsize = frame_size * 10;
	int err;

	err = snd_pcm_hw_params_malloc(&hw_params);
	if (err < 0) {
		DEBUG_WARNING("cannot allocate hw params (%s)\n",
			      snd_strerror(err));
		goto out;
	}

	err = snd_pcm_hw_params_any(pcm, hw_params);
	if (err < 0) {
		DEBUG_WARNING("cannot initialize hw params (%s)\n",
			      snd_strerror(err));
		goto out;
	}

	err = snd_pcm_hw_params_set_access(pcm, hw_params,
					   SND_PCM_ACCESS_RW_INTERLEAVED);
	if (err < 0) {
		DEBUG_WARNING("cannot set access type (%s)\n",
			      snd_strerror(err));
		goto out;
	}

	err = snd_pcm_hw_params_set_format(pcm, hw_params, pcmfmt);
	if (err < 0) {
		DEBUG_WARNING("cannot set sample format %d (%s)\n",
			      pcmfmt, snd_strerror(err));
		goto out;
	}

	err = snd_pcm_hw_params_set_rate(pcm, hw_params, srate, 0);
	if (err < 0) {
		DEBUG_WARNING("cannot set sample rate (%s)\n",
			      snd_strerror(err));
		goto out;
	}

	err = snd_pcm_hw_params_set_channels(pcm, hw_params, ch);
	if (err < 0) {
		DEBUG_WARNING("cannot set channel count to %d (%s)\n",
			      ch, snd_strerror(err));
		goto out;
	}

	err = snd_pcm_hw_params_set_period_size_near(pcm, hw_params,
						     &period, 0);
	if (err < 0) {
		DEBUG_WARNING("cannot set period size to %d (%s)\n",
			      period, snd_strerror(err));
	}

	err = snd_pcm_hw_params_set_buffer_size_near(pcm, hw_params, &bufsize);
	if (err < 0) {
		DEBUG_WARNING("cannot set buffer size to %d (%s)\n",
			      bufsize, snd_strerror(err));
	}

	err = snd_pcm_hw_params(pcm, hw_params);
	if (err < 0) {
		DEBUG_WARNING("cannot set parameters (%s)\n",
			      snd_strerror(err));
		goto out;
	}

	err = snd_pcm_prepare(pcm);
	if (err < 0) {
		DEBUG_WARNING("cannot prepare audio interface for use (%s)\n",
			      snd_strerror(err));
		goto out;
	}

	err = 0;

 out:
	snd_pcm_hw_params_free(hw_params);

	if (err) {
		DEBUG_WARNING("init failed: err=%d\n", err);
	}

	return err;
}


static int alsa_init(void)
{
	int err;

	err  = ausrc_register(&ausrc, "alsa", alsa_src_alloc);
	err |= auplay_register(&auplay, "alsa", alsa_play_alloc);

	return err;
}


static int alsa_close(void)
{
	ausrc  = mem_deref(ausrc);
	auplay = mem_deref(auplay);

	return 0;
}


const struct mod_export DECL_EXPORTS(alsa) = {
	"alsa",
	"sound",
	alsa_init,
	alsa_close
};
