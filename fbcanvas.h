#ifndef FBCANVAS_H
#define FBCANVAS_H

#include <poppler/glib/poppler.h>

struct fbcanvas
{
	unsigned int width;
	unsigned int height;
	unsigned int xoffset;
	unsigned int yoffset;
	unsigned char *buffer;
	GdkPixbuf *gdkpixbuf;

	void (*draw)(struct fbcanvas *fbc);
	void (*data_from_pixbuf)(struct fbcanvas *fbc, GdkPixbuf *gdkpixbuf, int w, int h);
};

struct fbcanvas *fbcanvas_create(int width, int height);
void fbcanvas_destroy(struct fbcanvas *fbc);

#endif /* FBCANVAS_H */
