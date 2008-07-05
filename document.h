#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <cairo/cairo.h>
#include "fbcanvas.h"

struct document;

struct document_ops
{
	void *(*open)(struct document *doc);
	void (*close)(struct document *doc);

	cairo_surface_t *(*update)(struct document *doc);
	void (*draw)(struct document *doc);

	int (*grep)(struct document *doc, char *regexp);
	void (*dump_text)(struct document *doc, char *filename);

	void (*main_loop)(struct document *doc);
};

struct backend
{
	struct backend *(*open)(char *filename);
	void (*close)(struct backend *fbc);
	void (*main_loop)(struct document *doc);

	cairo_surface_t *surface;
	void (*draw)(struct backend *be, cairo_surface_t *surface);

	unsigned int width;
	unsigned int height;
	void *data;

	struct framebuffer *fb;
};

struct document
{
	struct backend *backend;
	char *filename;
	void *data;
	char *message;

	unsigned int pagenum;
	unsigned int pagecount;

	struct document_ops *ops;

	cairo_surface_t *cairo;
	cairo_matrix_t transform;

	unsigned int flags;

	double scale;

	unsigned int width;
	unsigned int height;
	signed int xoffset;
	signed int yoffset;

	/* Yleiset versiot tiedostokohtaisista metodeista. */
	void (*close)(struct document *doc);
	void (*update)(struct document *doc);
	void (*draw)(struct document *doc);
	int (*grep)(struct document *doc, char *regexp);
	void (*dump_text)(struct document *doc, char *filename);
	void (*main_loop)(struct document *doc);

	/* Kaikille yhteiset metodit */
	void (*set_message)(struct document *doc, char *msg);
	void (*scroll)(struct document *doc, int dx, int dy);
};

enum
{
	NO_GENERIC_SCALE = 1,
};

struct document *open_document(char *filename);

#endif /* DOCUMENT_H */
