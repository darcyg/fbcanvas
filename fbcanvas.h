#ifndef FBCANVAS_H
#define FBCANVAS_H

#include <poppler/glib/poppler.h>

struct fbcanvas
{
	char *filename;
	unsigned char *hwmem;	/* mmap()ed framebuffer memory	*/
	unsigned int width;	/* Virtual width		*/
	unsigned int height;	/* Virtual height		*/
	unsigned int hwwidth;	/* Hardware width		*/
	unsigned int hwheight;	/* Hardware height		*/
	unsigned int hwdepth;	/* Hardware color depth		*/
	signed int xoffset;
	signed int yoffset;
	double scale;
	unsigned int pagenum;
	unsigned int pagecount;
	PopplerDocument *document;
	PopplerPage *page;
	GdkPixbuf *gdkpixbuf;

	void (*open)(struct fbcanvas *fbc, char *filename);
	void (*close)(struct fbcanvas *fbc);
	void (*draw)(struct fbcanvas *fbc);
	void (*update)(struct fbcanvas *fbc);
	void (*scroll)(struct fbcanvas *fbc, int dx, int dy);

	int (*grep)(struct fbcanvas *fbc, char *regexp);
};

struct file_ops
{
	char *type;
	void (*open)(struct fbcanvas *fbc, char *filename);
	void (*close)(struct fbcanvas *fbc);
	void (*update)(struct fbcanvas *fbc);

	int (*grep)(struct fbcanvas *fbc, char *regexp);
};

struct fbcanvas *fbcanvas_create(char *filename);
void fbcanvas_destroy(struct fbcanvas *fbc);

#endif /* FBCANVAS_H */
