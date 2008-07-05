#ifndef FBCANVAS_H
#define FBCANVAS_H

#include <cairo/cairo.h>

struct backend;

struct framebuffer
{
	unsigned char *mem;	/* mmap()ed framebuffer memory	*/
	unsigned int depth;	/* Hardware color depth		*/

	void (*draw)(struct backend *be, cairo_surface_t *surface);
};

#endif /* FBCANVAS_H */
