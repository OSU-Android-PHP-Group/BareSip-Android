/**
 * @file opensles/recorder.c  OpenSLES audio driver -- recording
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re.h>
#include <baresip.h>
#include <SLES/OpenSLES.h>
#include "SLES/OpenSLES_Android.h"
#include "opensles.h"


#define DEBUG_MODULE "opensles"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


struct ausrc_st {
	struct ausrc *as;      /* inheritance */
	uint8_t buf[320];
	ausrc_read_h *rh;
	void *arg;

	SLObjectItf recObject;
	SLRecordItf recRecord;
	SLAndroidSimpleBufferQueueItf recBufferQueue;
};


static void ausrc_destructor(void *arg)
{
	struct ausrc_st *st = arg;

	if (st->recObject != NULL)
		(*st->recObject)->Destroy(st->recObject);

	mem_deref(st->as);
}


static void bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
	struct ausrc_st *st = context;

	(void)bq;

	st->rh(st->buf, sizeof(st->buf), st->arg);

	(*st->recBufferQueue)->Enqueue(st->recBufferQueue,
				       st->buf, sizeof(st->buf));
}


static int createAudioRecorder(struct ausrc_st *st, struct ausrc_prm *prm)
{
	SLDataLocator_IODevice loc_dev = {SL_DATALOCATOR_IODEVICE,
					  SL_IODEVICE_AUDIOINPUT,
					  SL_DEFAULTDEVICEID_AUDIOINPUT,
					  NULL};
	SLDataSource audioSrc = {&loc_dev, NULL};

	SLDataLocator_AndroidSimpleBufferQueue loc_bq = {
		SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2
	};
	SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, prm->ch,
				       prm->srate * 1000,
				       SL_PCMSAMPLEFORMAT_FIXED_16,
				       SL_PCMSAMPLEFORMAT_FIXED_16,
				       SL_SPEAKER_FRONT_CENTER,
				       SL_BYTEORDER_LITTLEENDIAN};
	SLDataSink audioSnk = {&loc_bq, &format_pcm};
	const SLInterfaceID id[1] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
	const SLboolean req[1] = {SL_BOOLEAN_TRUE};
	SLresult r;

	r = (*engineEngine)->CreateAudioRecorder(engineEngine,
						 &st->recObject,
						 &audioSrc,
						 &audioSnk, 1, id, req);
	if (SL_RESULT_SUCCESS != r)
		return ENODEV;

	r = (*st->recObject)->Realize(st->recObject, SL_BOOLEAN_FALSE);
	if (SL_RESULT_SUCCESS != r)
		return ENODEV;

	r = (*st->recObject)->GetInterface(st->recObject, SL_IID_RECORD,
					   &st->recRecord);
	if (SL_RESULT_SUCCESS != r)
		return ENODEV;

	r = (*st->recObject)->GetInterface(st->recObject,
					   SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
					   &st->recBufferQueue);
	if (SL_RESULT_SUCCESS != r)
		return ENODEV;

	r = (*st->recBufferQueue)->RegisterCallback(st->recBufferQueue,
						    bqRecorderCallback,
						    st);
	if (SL_RESULT_SUCCESS != r)
		return ENODEV;

	return 0;
}


static int startRecording(struct ausrc_st *st)
{
	SLresult r;

	(*st->recRecord)->SetRecordState(st->recRecord,
					 SL_RECORDSTATE_STOPPED);
	(*st->recBufferQueue)->Clear(st->recBufferQueue);

	r = (*st->recBufferQueue)->Enqueue(st->recBufferQueue,
					   st->buf, sizeof(st->buf));
	if (SL_RESULT_SUCCESS != r)
		return ENODEV;

	r = (*st->recRecord)->SetRecordState(st->recRecord,
					     SL_RECORDSTATE_RECORDING);
	if (SL_RESULT_SUCCESS != r)
		return ENODEV;

	return 0;
}


int opensles_recorder_alloc(struct ausrc_st **stp, struct ausrc *as,
			    struct media_ctx **ctx,
			    struct ausrc_prm *prm, const char *device,
			    ausrc_read_h *rh, ausrc_error_h *errh, void *arg)
{
	struct ausrc_st *st;
	int err;

	(void)ctx;
	(void)device;
	(void)errh;

	st = mem_zalloc(sizeof(*st), ausrc_destructor);
	if (!st)
		return ENOMEM;

	st->as  = mem_ref(as);
	st->rh  = rh;
	st->arg = arg;

	err = createAudioRecorder(st, prm);
	if (err)
		goto out;

	err = startRecording(st);
	if (err)
		goto out;

 out:

	if (err)
		mem_deref(st);
	else
		*stp = st;

	return err;
}
