#ifndef FBCANVAS_H
#define FBCANVAS_H

#include <poppler/glib/poppler.h>

struct fbcanvas
{
	unsigned int xoffset;
	unsigned int yoffset;
	GdkPixbuf *gdkpixbuf;

	void (*draw)(struct fbcanvas *fbc);
};

struct fbcanvas *fbcanvas_create(void);
void fbcanvas_destroy(struct fbcanvas *fbc);

#endif /* FBCANVAS_H */
