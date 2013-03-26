/**
 * @file opengles.c Video driver for OpenGLES
 *
 * Copyright (C) 2010 Creytiv.com
 */

#include <re.h>
#include <rem.h>
#include <baresip.h>
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include "opengles.h"


#define DEBUG_MODULE "opengles"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


static struct vidisp *vid;


static int texture_init(struct vidisp_st *st)
{
	glGenTextures(1, &st->texture_id);
	if (!st->texture_id)
		return ENOMEM;

	glBindTexture(GL_TEXTURE_2D, st->texture_id);
	glTexParameterf(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
		     st->vf->size.w, st->vf->size.h, 0,
		     GL_RGB, GL_UNSIGNED_SHORT_5_6_5, st->vf->data[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	return 0;
}


static void texture_render(struct vidisp_st *st)
{
	static const GLfloat coords[4 * 2] = {
		0.0, 1.0,
		1.0, 1.0,
		0.0, 0.0,
		1.0, 0.0
	};

	glBindTexture(GL_TEXTURE_2D, st->texture_id);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
		     st->vf->size.w, st->vf->size.h, 0,
		     GL_RGB, GL_UNSIGNED_SHORT_5_6_5, st->vf->data[0]);

	/* Setup the vertices */
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, st->vertices);

	/* Setup the texture coordinates */
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, coords);

	glBindTexture(GL_TEXTURE_2D, st->texture_id);

	glEnable(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisable(GL_TEXTURE_2D);
}


static void setup_layout(struct vidisp_st *st, const struct vidsz *screensz,
			 struct vidrect *ortho, struct vidrect *vp)
{
	struct vidpt origin;
	int w, h, i = 0;

	w = st->vf->size.w;
	h = st->vf->size.h;

	st->vertices[i++] = 0;
	st->vertices[i++] = 0;
	st->vertices[i++] = 0;
	st->vertices[i++] = w;
	st->vertices[i++] = 0;
	st->vertices[i++] = 0;
	st->vertices[i++] = 0;
	st->vertices[i++] = h;
	st->vertices[i++] = 0;
	st->vertices[i++] = w;
	st->vertices[i++] = h;
	st->vertices[i++] = 0;

	origin.x = (screensz->w - w) / 2;
	origin.y = (screensz->h - h) / 2;

	if (origin.x < 0) {
		vp->x    = 0;
		ortho->x = -origin.x;
	}
	else {
		vp->x    = origin.x;
		ortho->x = 0;
	}

	if (origin.y < 0) {
		vp->y    = 0;
		ortho->y = -origin.y;
	}
	else {
		vp->y    = origin.y;
		ortho->y = 0;
	}

	vp->w = screensz->w - 2 * vp->x;
	vp->h = screensz->h - 2 * vp->y;

	ortho->w = w - ortho->x;
	ortho->h = h - ortho->y;
}


void opengles_addbuffers(struct vidisp_st *st)
{
	glGenFramebuffersOES(1, &st->framebuffer);
	glGenRenderbuffersOES(1, &st->renderbuffer);
	glBindFramebufferOES(GL_FRAMEBUFFER_OES, st->framebuffer);
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, st->renderbuffer);
}


void opengles_render(struct vidisp_st *st)
{
	if (!st->texture_id) {

		struct vidrect ortho, vp;
		struct vidsz bufsz;
		int err = 0;

		glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES,
						GL_RENDERBUFFER_WIDTH_OES,
						&bufsz.w);
		glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES,
						GL_RENDERBUFFER_HEIGHT_OES,
						&bufsz.h);

		glBindFramebufferOES(GL_FRAMEBUFFER_OES, st->framebuffer);
		glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES,
					     GL_COLOR_ATTACHMENT0_OES,
					     GL_RENDERBUFFER_OES,
					     st->renderbuffer);

		err = texture_init(st);
		if (err)
			return;

		glBindRenderbufferOES(GL_FRAMEBUFFER_OES, st->renderbuffer);

		setup_layout(st, &bufsz, &ortho, &vp);


		/* Set up Viewports etc. */

		glBindFramebufferOES(GL_FRAMEBUFFER_OES, st->framebuffer);

		glViewport(vp.x, vp.y, vp.w, vp.h);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		glOrthof(ortho.x, ortho.w, ortho.y, ortho.h, 0.0f, 1.0f);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glDisable(GL_DEPTH_TEST);
		glDisableClientState(GL_COLOR_ARRAY);
	}

	texture_render(st);

	glDisable(GL_TEXTURE_2D);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindTexture(GL_TEXTURE_2D, 0);
	glEnable(GL_DEPTH_TEST);

	glBindRenderbufferOES(GL_RENDERBUFFER_OES, st->renderbuffer);
}


static void destructor(void *arg)
{
	struct vidisp_st *st = arg;

	glDeleteTextures(1, &st->texture_id);
	glDeleteFramebuffersOES(1, &st->framebuffer);
	glDeleteRenderbuffersOES(1, &st->renderbuffer);

	context_destroy(st);

	mem_deref(st->vf);
	mem_deref(st->vd);
}


static int opengles_alloc(struct vidisp_st **stp, struct vidisp_st *parent,
			  struct vidisp *vd,
			  struct vidisp_prm *prm, const char *dev,
			  vidisp_input_h *inputh, vidisp_resize_h *resizeh,
			  void *arg)
{
	struct vidisp_st *st;
	int err = 0;

	(void)parent;
	(void)prm;
	(void)dev;
	(void)inputh;
	(void)resizeh;
	(void)arg;

	st = mem_zalloc(sizeof(*st), destructor);
	if (!st)
		return ENOMEM;

	st->vd = mem_ref(vd);

	err = context_init(st);
	if (err)
		goto out;

 out:
	if (err)
		mem_deref(st);
	else
		*stp = st;

	return err;
}


static int opengles_display(struct vidisp_st *st, const char *title,
			    const struct vidframe *frame)
{
	int err;

	(void)title;

	if (!st->vf) {
		if (frame->size.w & 3) {
			DEBUG_WARNING("width must be multiple of 4\n");
			return EINVAL;
		}

		err = vidframe_alloc(&st->vf, VID_FMT_RGB565, &frame->size);
		if (err)
			return err;
	}

	vidconv(st->vf, frame, NULL);

	context_render(st);

	return 0;
}


static int module_init(void)
{
	return vidisp_register(&vid, "opengles", opengles_alloc, NULL,
			       opengles_display, NULL);
}


static int module_close(void)
{
	vid = mem_deref(vid);

	return 0;
}


EXPORT_SYM const struct mod_export DECL_EXPORTS(opengles) = {
	"opengles",
	"vidisp",
	module_init,
	module_close,
};
