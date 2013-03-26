/**
 * @file opensles/player.c  OpenSLES audio driver -- playback
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re.h>
#include <baresip.h>
#include <SLES/OpenSLES.h>
#include "SLES/OpenSLES_Android.h"
#include "opensles.h"


struct auplay_st {
	struct auplay *ap;      /* inheritance */
	auplay_write_h *wh;
	void *arg;

	SLObjectItf outputMixObject;
	SLObjectItf bqPlayerObject;
	SLPlayItf bqPlayerPlay;
	SLAndroidSimpleBufferQueueItf BufferQueue;
};


static void auplay_destructor(void *arg)
{
	struct auplay_st *st = arg;

	if (st->bqPlayerObject != NULL)
		(*st->bqPlayerObject)->Destroy(st->bqPlayerObject);

	if (st->outputMixObject != NULL)
		(*st->outputMixObject)->Destroy(st->outputMixObject);

	mem_deref(st->ap);
}


static void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
	struct auplay_st *st = context;
	uint8_t buf[320 * 8];

	(void)bq;

	st->wh(buf, sizeof(buf), st->arg);

	(*st->BufferQueue)->Enqueue(st->BufferQueue, buf, sizeof(buf));
}


static int createOutput(struct auplay_st *st)
{
	const SLInterfaceID ids[1] = {SL_IID_ENVIRONMENTALREVERB};
	const SLboolean req[1] = {SL_BOOLEAN_FALSE};
	SLresult r;

	r = (*engineEngine)->CreateOutputMix(engineEngine,
					    &st->outputMixObject, 1, ids, req);
	if (SL_RESULT_SUCCESS != r)
		return ENODEV;

	r = (*st->outputMixObject)->Realize(st->outputMixObject,
					    SL_BOOLEAN_FALSE);
	if (SL_RESULT_SUCCESS != r)
		return ENODEV;

	return 0;
}


static int createPlayer(struct auplay_st *st, struct auplay_prm *prm)
{
	SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {
		SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2
	};
	SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, prm->ch,
				       prm->srate * 1000,
				       SL_PCMSAMPLEFORMAT_FIXED_16,
				       SL_PCMSAMPLEFORMAT_FIXED_16,
				       SL_SPEAKER_FRONT_CENTER,
				       SL_BYTEORDER_LITTLEENDIAN};
	SLDataSource audioSrc = {&loc_bufq, &format_pcm};
	SLDataLocator_OutputMix loc_outmix = {
		SL_DATALOCATOR_OUTPUTMIX, st->outputMixObject
	};
	SLDataSink audioSnk = {&loc_outmix, NULL};
	const SLInterfaceID ids[2] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND};
	const SLboolean req[2] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
	SLresult r;

	r = (*engineEngine)->CreateAudioPlayer(engineEngine,
					       &st->bqPlayerObject,
					       &audioSrc, &audioSnk,
					       2, ids, req);
	if (SL_RESULT_SUCCESS != r)
		return ENODEV;

	r = (*st->bqPlayerObject)->Realize(st->bqPlayerObject,
					   SL_BOOLEAN_FALSE);
	if (SL_RESULT_SUCCESS != r)
		return ENODEV;

	r = (*st->bqPlayerObject)->GetInterface(st->bqPlayerObject,
						SL_IID_PLAY,
						&st->bqPlayerPlay);
	if (SL_RESULT_SUCCESS != r)
		return ENODEV;

	r = (*st->bqPlayerObject)->GetInterface(st->bqPlayerObject,
						SL_IID_BUFFERQUEUE,
						&st->BufferQueue);
	if (SL_RESULT_SUCCESS != r)
		return ENODEV;

	r = (*st->BufferQueue)->RegisterCallback(st->BufferQueue,
						 bqPlayerCallback, st);
	if (SL_RESULT_SUCCESS != r)
		return ENODEV;

	r = (*st->bqPlayerPlay)->SetPlayState(st->bqPlayerPlay,
					      SL_PLAYSTATE_PLAYING);
	if (SL_RESULT_SUCCESS != r)
		return ENODEV;

	return 0;
}


int opensles_player_alloc(struct auplay_st **stp, struct auplay *ap,
			  struct auplay_prm *prm, const char *device,
			  auplay_write_h *wh, void *arg)
{
	struct auplay_st *st;
	int err;

	(void)device;

	st = mem_zalloc(sizeof(*st), auplay_destructor);
	if (!st)
		return ENOMEM;

	st->ap  = mem_ref(ap);
	st->wh  = wh;
	st->arg = arg;

	err = createOutput(st);
	if (err)
		goto out;

	err = createPlayer(st, prm);
	if (err)
		goto out;

	bqPlayerCallback(st->BufferQueue, st);

 out:
	if (err)
		mem_deref(st);
	else
		*stp = st;

	return err;
}
