#ifndef FBCANVAS_H
#define FBCANVAS_H

#include <poppler/glib/poppler.h>

struct fbcanvas
{
	char *filename;
	unsigned int xoffset;
	unsigned int yoffset;
	double scale;
	unsigned int pagenum;
	unsigned int pagecount;
	GdkPixbuf *gdkpixbuf;

	void (*draw)(struct fbcanvas *fbc);
};

struct fbcanvas *fbcanvas_create(char *filename);
void fbcanvas_destroy(struct fbcanvas *fbc);

#endif /* FBCANVAS_H */
