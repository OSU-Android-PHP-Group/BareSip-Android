/**
 * @file oss.c  Open Sound System (OSS) driver
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re.h>
#include <rem.h>
#include <baresip.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#if defined(NETBSD) || defined(OPENBSD)
#include <soundcard.h>
#elif defined (LINUX)
#include <linux/soundcard.h>
#else
#include <sys/soundcard.h>
#endif


struct ausrc_st {
	struct ausrc *as;      /* inheritance */
	int fd;
	struct mbuf *mb;
	ausrc_read_h *rh;
	ausrc_error_h *errh;
	void *arg;
};

struct auplay_st {
	struct auplay *ap;      /* inheritance */
	pthread_t thread;
	bool run;
	int fd;
	uint8_t *buf;
	uint32_t sz;
	auplay_write_h *wh;
	void *arg;
};


static struct ausrc *ausrc;
static struct auplay *auplay;
static char oss_dev[64] = "/dev/dsp";


/**
 * Automatically calculate the fragment size depending on sampling rate
 * and number of channels. More entries can be added to the table below.
 *
 * NOTE. Powermac 8200 and linux 2.4.18 gives:
 *       SNDCTL_DSP_SETFRAGMENT: Invalid argument
 */
static int set_fragment(int fd, uint32_t frame_size)
{
	static const struct {
		uint16_t max;
		uint16_t size;
	} fragv[] = {
		{10, 7},  /* 10 x 2^7 = 1280 =  4 x 320 */
		{15, 7},  /* 15 x 2^7 = 1920 =  6 x 320 */
		{20, 7},  /* 20 x 2^7 = 2560 =  8 x 320 */
		{25, 7},  /* 25 x 2^7 = 3200 = 10 x 320 */
		{15, 8},  /* 15 x 2^8 = 3840 = 12 x 320 */
		{20, 8},  /* 20 x 2^8 = 5120 = 16 x 320 */
		{25, 8}   /* 25 x 2^8 = 6400 = 20 x 320 */
	};
	size_t i;
	const uint32_t buf_size = 2 * frame_size;

	for (i=0; i<ARRAY_SIZE(fragv); i++) {
		const uint16_t frag_max  = fragv[i].max;
		const uint16_t frag_size = fragv[i].size;
		const uint32_t fragment_size = frag_max * (1<<frag_size);

		if (0 == (fragment_size%buf_size)) {
			int fragment = (frag_max<<16) | frag_size;

			if (0 == ioctl(fd, SNDCTL_DSP_SETFRAGMENT,
				       &fragment)) {
				return 0;
			}
		}
	}

	return ENODEV;
}


static int oss_reset(int fd, uint32_t srate, uint8_t ch, int frame_size,
		     int nonblock)
{
	int format    = AFMT_S16_LE;
	int speed     = srate;
	int channels  = ch;
	int blocksize = 0;
	int err;

	err = set_fragment(fd, frame_size);
	if (err)
		return err;

	if (0 != ioctl(fd, FIONBIO, &nonblock))
		return errno;
	if (0 != ioctl(fd, SNDCTL_DSP_SETFMT, &format))
		return errno;
	if (0 != ioctl(fd, SNDCTL_DSP_CHANNELS, &channels))
		return errno;
	if (2 == channels) {
		int stereo = 1;
		if (0 != ioctl(fd, SNDCTL_DSP_STEREO, &stereo))
			return errno;
	}
	if (0 != ioctl(fd, SNDCTL_DSP_SPEED, &speed))
		return errno;

	(void)ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &blocksize);

	re_printf("oss init: %u bit %d Hz %d ch, blocksize=%d\n",
		  format, speed, channels, blocksize);

	return 0;
}


static void auplay_destructor(void *arg)
{
	struct auplay_st *st = arg;

	if (st->run) {
		st->run = false;
		pthread_join(st->thread, NULL);
	}

	if (-1 != st->fd) {
		fd_close(st->fd);
		(void)close(st->fd);
	}

	mem_deref(st->buf);
	mem_deref(st->ap);
}


static void ausrc_destructor(void *arg)
{
	struct ausrc_st *st = arg;

	if (-1 != st->fd) {
		fd_close(st->fd);
		(void)close(st->fd);
	}

	mem_deref(st->mb);
	mem_deref(st->as);
}


static void read_handler(int flags, void *arg)
{
	struct ausrc_st *st = arg;
	struct mbuf *mb = st->mb;
	int n;
	(void)flags;

	n = read(st->fd, mbuf_buf(mb), mbuf_get_space(mb));
	if (n <= 0)
		return;

	mb->pos += n;

	if (mb->pos < mb->size)
		return;

	st->rh(mb->buf, mb->size, st->arg);

	mb->pos = 0;
}


static void *play_thread(void *arg)
{
	struct auplay_st *st = arg;
	int n;

	while (st->run) {

		st->wh(st->buf, st->sz, st->arg);

		n = write(st->fd, st->buf, st->sz);
		if (n < 0) {
			re_printf("write: %m\n", errno);
			break;
		}
	}

	return NULL;
}


static int src_alloc(struct ausrc_st **stp, struct ausrc *as,
		     struct media_ctx **ctx,
		     struct ausrc_prm *prm, const char *device,
		     ausrc_read_h *rh, ausrc_error_h *errh, void *arg)
{
	struct ausrc_st *st;
	int err;

	(void)ctx;
	(void)errh;

	st = mem_zalloc(sizeof(*st), ausrc_destructor);
	if (!st)
		return ENOMEM;

	st->fd   = -1;
	st->rh   = rh;
	st->errh = errh;
	st->arg  = arg;

	if (!device)
		device = oss_dev;

	prm->fmt = AUFMT_S16LE;

	st->mb = mbuf_alloc(2 * prm->frame_size);
	if (!st->mb) {
		err = ENOMEM;
		goto out;
	}

	st->fd = open(device, O_RDONLY);
	if (st->fd < 0) {
		err = errno;
		goto out;
	}

	err = fd_listen(st->fd, FD_READ, read_handler, st);
	if (err)
		goto out;

	err = oss_reset(st->fd, prm->srate, prm->ch, prm->frame_size, 1);
	if (err)
		goto out;

	st->as = mem_ref(as);

 out:
	if (err)
		mem_deref(st);
	else
		*stp = st;

	return err;
}


static int play_alloc(struct auplay_st **stp, struct auplay *ap,
		      struct auplay_prm *prm, const char *device,
		      auplay_write_h *wh, void *arg)
{
	struct auplay_st *st;
	int err;

	st = mem_zalloc(sizeof(*st), auplay_destructor);
	if (!st)
		return ENOMEM;

	st->fd  = -1;
	st->wh  = wh;
	st->arg = arg;

	if (!device)
		device = oss_dev;

	prm->fmt = AUFMT_S16LE;

	st->sz  = 2 * prm->frame_size;
	st->buf = mem_alloc(st->sz, NULL);
	if (!st->buf) {
		err = ENOMEM;
		goto out;
	}

	st->fd = open(device, O_WRONLY);
	if (st->fd < 0) {
		err = errno;
		goto out;
	}

	err = oss_reset(st->fd, prm->srate, prm->ch, prm->frame_size, 0);
	if (err)
		goto out;

	st->ap = mem_ref(ap);

	st->run = true;
	err = pthread_create(&st->thread, NULL, play_thread, st);
	if (err) {
		st->run = false;
		goto out;
	}

 out:
	if (err)
		mem_deref(st);
	else
		*stp = st;

	return err;
}


static int module_init(void)
{
	int err;

	err  = ausrc_register(&ausrc, "oss", src_alloc);
	err |= auplay_register(&auplay, "oss", play_alloc);

	return err;
}


static int module_close(void)
{
	ausrc  = mem_deref(ausrc);
	auplay = mem_deref(auplay);

	return 0;
}


EXPORT_SYM const struct mod_export DECL_EXPORTS(oss) = {
	"oss",
	"audio",
	module_init,
	module_close,
};
