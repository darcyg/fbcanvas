#ifndef FBCANVAS_H
#define FBCANVAS_H

#include <poppler/glib/poppler.h>

struct fbcanvas;

struct framebuffer
{
	unsigned char *mem;	/* mmap()ed framebuffer memory	*/
	unsigned int width;	/* Hardware width		*/
	unsigned int height;	/* Hardware height		*/
	unsigned int depth;	/* Hardware color depth		*/

	void (*draw)(struct fbcanvas *fbc);
};

struct file_ops
{
	char *type;
	void (*open)(struct fbcanvas *fbc, char *filename);
	void (*close)(struct fbcanvas *fbc);
	void (*update)(struct fbcanvas *fbc);

	int (*grep)(struct fbcanvas *fbc, char *regexp);
};

struct fbcanvas
{
	struct framebuffer *fb;
	char *filename;
	unsigned int width;	/* Virtual width		*/
	unsigned int height;	/* Virtual height		*/
	signed int xoffset;
	signed int yoffset;
	double scale;
	unsigned int pagenum;
	unsigned int pagecount;

	void *context;
	void *document;
	void *page;

	GdkPixbuf *gdkpixbuf;

	struct file_ops *ops;
	void (*scroll)(struct fbcanvas *fbc, int dx, int dy);
};

struct fbcanvas *fbcanvas_create(char *filename);
void fbcanvas_destroy(struct fbcanvas *fbc);

#endif /* FBCANVAS_H */
