#include <math.h>
#include <stdlib.h>
#include "document.h"
#include "fbcanvas.h"
#include "file_info.h"

static void *open_png(struct document *doc)
{
	return NULL;
}

static void close_png(struct document *doc)
{
}

static void update_png(struct document *doc)
{
	doc->cairo = cairo_image_surface_create_from_png(doc->filename);
}

static struct document_ops png_ops =
{
	.open = open_png,
	.close = close_png,
	.update = update_png,
};

struct file_info png_info = {"PNG", &png_ops};

