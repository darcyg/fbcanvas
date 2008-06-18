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

	void (*draw)(struct document *doc);
};

struct fbcanvas
{
	struct framebuffer *fb;
	void (*scroll)(struct document *doc, int dx, int dy);
};

struct fbcanvas *fbcanvas_create(char *filename);
void fbcanvas_destroy(struct fbcanvas *fbc);

#endif /* FBCANVAS_H */
