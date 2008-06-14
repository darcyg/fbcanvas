#ifndef DOCUMENT_H
#define DOCUMENT_H

#include "fbcanvas.h"

struct document;

struct pdf_data
{
	PopplerDocument *document;
	PopplerPage *page;
};

struct document_ops
{
	void *(*open)(struct document *doc);
	void (*close)(struct document *doc);

	void (*update)(struct document *doc);

	int (*grep)(struct document *doc, char *regexp);

	unsigned int (*page_count)(struct document *doc);
};

struct document
{
	struct fbcanvas *fbcanvas;
	char *filename;
	void *data;

	unsigned int pagenum;
	unsigned int pagecount;

	struct document_ops *ops;

	GdkPixbuf *gdkpixbuf;
	double scale;

	unsigned int width;
	unsigned int height;
	signed int xoffset;
	signed int yoffset;

	/* Yleiset versiot tiedostokohtaisista metodeista. */
	void (*close)(struct document *doc);
	void (*update)(struct document *doc);
	int (*grep)(struct document *doc, char *regexp);
	unsigned int (*page_count)(struct document *doc);
};

struct document *open_document(char *filename);

#endif /* DOCUMENT_H */
