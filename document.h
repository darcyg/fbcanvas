#ifndef DOCUMENT_H
#define DOCUMENT_H

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
};

struct document
{
	struct fbcanvas *fbcanvas;
	char *filename;
	void *data;
	char *message;

	unsigned int pagenum;
	unsigned int pagecount;

	struct document_ops *ops;

	cairo_surface_t *cairo;

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
	void (*set_message)(struct document *doc, char *msg);
};

struct document *open_document(char *filename);

#endif /* DOCUMENT_H */
