#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <cairo/cairo.h>

struct document;

struct document_ops
{
	void *(*open)(struct document *doc);
	void (*close)(struct document *doc);

	cairo_surface_t *(*update)(struct document *doc);
	void (*draw)(struct document *doc);

	int (*grep)(struct document *doc, char *regexp);
	void (*dump_text)(struct document *doc, char *filename);
};

struct backend
{
	struct backend *(*open)(char *filename);
	void (*close)(struct backend *be);

	void (*idle_callback)(struct document *doc);

	cairo_surface_t *surface;
	void (*draw)(struct backend *be, cairo_surface_t *surface);

	unsigned int width;
	unsigned int height;
	void *data;
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

	/* Kaikille yhteiset metodit */
	void (*set_message)(struct document *doc, char *fmt, ...);
	void (*scroll)(struct document *doc, int dx, int dy);
};

enum
{
	NO_GENERIC_SCALE = 1,
	DOCUMENT_IDLE = 2,
};

struct document *open_document(char *filename);

#endif /* DOCUMENT_H */
