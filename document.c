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
		if (doc->cairo)
		{
			cairo_surface_destroy(doc->cairo);
			doc->cairo = NULL;
		}


		doc->ops->update(doc);

		if (doc->cairo)
		{
			doc->width = cairo_image_surface_get_width(doc->cairo);
			doc->height = cairo_image_surface_get_height(doc->cairo);
		}
	}
}

static void merge_surfaces (struct document *doc)
{
	cairo_pattern_t *img = cairo_pattern_create_for_surface (doc->cairo);
	cairo_pattern_t *msg = cairo_pattern_create_for_surface (doc->message);

	cairo_surface_t *surf = cairo_surface_create_similar (
		doc->cairo, CAIRO_CONTENT_COLOR_ALPHA,
		doc->fbcanvas->fb->width, doc->fbcanvas->fb->height);

	cairo_t *cr = cairo_create (surf);

	/* Use current image as a background. */
	cairo_save (cr);
	cairo_translate (cr, -doc->xoffset, -doc->yoffset);
	cairo_set_source (cr, img);
	cairo_paint_with_alpha (cr, 0.7);
	cairo_pattern_destroy (img);

	cairo_restore (cr);
	cairo_set_source (cr, msg);
	cairo_paint (cr);
	cairo_pattern_destroy (msg);
	cairo_destroy (cr);

	/* Replace doc->message with merged surface. */
	cairo_surface_destroy (doc->message);
	doc->message = surf;
}

static void draw_document(struct document *doc)
{
	if (doc->message)
		merge_surfaces (doc);

	doc->fbcanvas->fb->draw(doc);

	if (doc->message)
	{
		cairo_surface_destroy (doc->message);
		doc->message = NULL;
	}
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

		doc->cairo = NULL;
		doc->message = NULL;
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
