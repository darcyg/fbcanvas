#include <stdlib.h>
#include <string.h>
#include "document.h"
#include "file_info.h"

static void close_document(struct document *doc)
{
	if (doc->ops->close)
		doc->ops->close(doc);
	fbcanvas_destroy(doc->fbcanvas);
	free(doc->filename);
	free(doc);
}

static void update_document(struct document *doc)
{
	if (doc->ops->update)
	{
		if (doc->gdkpixbuf)
		{
			g_object_unref(doc->gdkpixbuf);
			doc->gdkpixbuf = NULL;
		}

		if (doc->cairo)
		{
			cairo_surface_destroy(doc->cairo);
			doc->cairo = NULL;
		}


		doc->ops->update(doc);

		if (doc->gdkpixbuf)
		{
			doc->width = gdk_pixbuf_get_width(doc->gdkpixbuf);
			doc->height = gdk_pixbuf_get_height(doc->gdkpixbuf);
		} else if (doc->cairo) {
			doc->width = cairo_image_surface_get_width(doc->cairo);
			doc->height = cairo_image_surface_get_height(doc->cairo);
		}
	}
}

static void draw_document(struct document *doc)
{
	unsigned char *data;
	unsigned int width;
	unsigned int height;

	if (doc->gdkpixbuf)
	{
		data = gdk_pixbuf_get_pixels(doc->gdkpixbuf);
		width = gdk_pixbuf_get_width(doc->gdkpixbuf);
		height = gdk_pixbuf_get_height(doc->gdkpixbuf);
	} else if (doc->cairo) {
		data = cairo_image_surface_get_data(doc->cairo);
		width = cairo_image_surface_get_width(doc->cairo);
		height = cairo_image_surface_get_height(doc->cairo);
	}

	doc->fbcanvas->fb->draw(doc->fbcanvas->fb,
		data, width, height, doc->xoffset, doc->yoffset);
}

static int grep_document(struct document *doc, char *regexp)
{
	if (doc->ops->grep)
		return doc->ops->grep(doc, regexp);

	fprintf (stderr, "%s", "Sorry, grepping is not implemented for this file type.\n");
	return 1;
}

static unsigned int document_page_count(struct document *doc)
{
	if (doc->ops->page_count)
		return doc->ops->page_count(doc);
	return 1;
}

static void document_dump_text(struct document *doc, char *filename)
{
	if (doc->ops->dump_text)
		doc->ops->dump_text(doc, filename);
}

struct document *open_document(char *filename)
{
	struct document *doc = malloc(sizeof(*doc));
	if (doc)
	{
		struct file_info *fi;

		doc->fbcanvas = fbcanvas_create(filename);

		doc->gdkpixbuf = NULL;
		doc->cairo = NULL;
		doc->xoffset = 0;
		doc->yoffset = 0;
		doc->scale = 1.0;
		doc->pagenum = 0;
		doc->pagecount = 1;

		/* Asetetaan yleiset tiedot */
		doc->filename = strdup(filename);
		doc->close = close_document;

		/*
		 * Tunnistetaan tiedostotyyppi ja asetetaan
		 * sitä vastaavat metodit.
		 */
		fi = get_file_info(filename);
		if (!fi)
		{
			free(doc);
			doc = NULL;
			goto out;
		}

		doc->ops = fi->ops;
		doc->update = update_document;
		doc->draw = draw_document;
		doc->grep = grep_document;
		doc->page_count = document_page_count;
		doc->dump_text = document_dump_text;

		doc->data = doc->ops->open(doc);
	}
out:
	return doc;
}
