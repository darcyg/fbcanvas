#ifndef FBCANVAS_H
#define FBCANVAS_H

#include <cairo/cairo.h>
#include <poppler/glib/poppler.h>
#include "document.h"

struct document;
struct fbcanvas;

struct framebuffer
{
	unsigned char *mem;	/* mmap()ed framebuffer memory	*/
	unsigned int width;	/* Hardware width		*/
	unsigned int height;	/* Hardware height		*/
	unsigned int depth;	/* Hardware color depth		*/

	void (*draw)(struct framebuffer *fb, cairo_surface_t *surface);
};

struct fbcanvas
{
	struct framebuffer *fb;
};

struct fbcanvas *fbcanvas_create(char *filename);
void fbcanvas_destroy(struct fbcanvas *fbc);

#endif /* FBCANVAS_H */
