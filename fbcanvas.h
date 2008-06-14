#ifndef FBCANVAS_H
#define FBCANVAS_H

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

	void (*draw)(struct framebuffer *fb, GdkPixbuf *gdkpixbuf,
		signed int xoffset, signed int yoffset);
};

struct fbcanvas
{
	struct framebuffer *fb;
	void (*scroll)(struct document *doc, int dx, int dy);
};

struct fbcanvas *fbcanvas_create(char *filename);
void fbcanvas_destroy(struct fbcanvas *fbc);

#endif /* FBCANVAS_H */
