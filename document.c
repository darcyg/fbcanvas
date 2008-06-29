#include <pango/pangocairo.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "document.h"
#include "file_info.h"

void x11_main_loop(struct document *doc);
void ncurses_main_loop(struct document *doc);

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
	cairo_surface_destroy(doc->cairo);
	doc->cairo = doc->ops->update(doc);
	doc->width = cairo_image_surface_get_width(doc->cairo);
	doc->height = cairo_image_surface_get_height(doc->cairo);
}

static void merge_surfaces (struct document *doc, cairo_surface_t *surf)
{
	cairo_pattern_t *img = cairo_pattern_create_for_surface (doc->cairo);
	cairo_t *cr = cairo_create (surf);
	cairo_matrix_t mat = doc->transform;

	/* Insert current image. */
	cairo_save (cr);
	cairo_translate (cr, -doc->xoffset, -doc->yoffset);

	/* Some files do their own scaling. */
	if (!(doc->flags & NO_GENERIC_SCALE))
		cairo_matrix_scale (&mat, doc->scale, doc->scale);

	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_paint(cr);

	cairo_transform (cr, &mat);
	cairo_set_source (cr, img);
	cairo_paint_with_alpha (cr, doc->message ? 0.8: 1.0);
	cairo_pattern_destroy (img);
#if 0
	/* For debugging: draw circle to (0,0) */
	cairo_set_source_rgb (cr, 1.0, 1.0, 0.0);
	cairo_arc (cr, 0, 0, 10, 0, 2 * M_PI);
	cairo_fill (cr);
#endif
	cairo_restore (cr);

	if (doc->message)
	{
		PangoLayout *layout;
		PangoFontDescription *desc;
		int msg_lines = 1;

		for (int i = 0; doc->message[i]; i++)
		{
			if (doc->message[i] == '\n')
				msg_lines++;
		}

		/* Restrict operations to message area. */
		cairo_rectangle (cr, 0, 0, doc->fbcanvas->fb->width, msg_lines * 23);
		cairo_clip (cr);

		/* Black background. */
		cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
		cairo_paint_with_alpha (cr, 0.5);

		layout = (PangoLayout *) pango_cairo_create_layout (cr);
		pango_layout_set_text (layout, doc->message, -1);
		desc = pango_font_description_from_string ("Sans 14");
		pango_layout_set_font_description (layout, desc);
		pango_font_description_free (desc);

		/* White text. */
		cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
		cairo_move_to (cr, 8, 0); /* after the cursor */
		pango_cairo_update_layout (cr, layout);
		pango_cairo_show_layout (cr, layout);
		g_object_unref (layout);

		free (doc->message);
		doc->message = NULL;
	}

	cairo_destroy (cr);
}

static void draw_document(struct document *doc)
{
	struct fbcanvas *canvas = doc->fbcanvas;
	merge_surfaces (doc, canvas->surface);
	canvas->fb->draw(canvas->fb, canvas->surface);
}

static int grep_document(struct document *doc, char *regexp)
{
	if (doc->ops->grep)
		return doc->ops->grep(doc, regexp);

	fprintf (stderr, "%s", "Sorry, grepping is not implemented for this file type.\n");
	return 1;
}

static void document_dump_text(struct document *doc, char *filename)
{
	if (doc->ops->dump_text)
		doc->ops->dump_text(doc, filename);
}

static void document_set_message(struct document *doc, char *msg)
{
	free (doc->message);
	doc->message = strdup (msg);
}

static void document_scroll(struct document *doc, int dx, int dy)
{
	struct framebuffer *fb = doc->fbcanvas->fb;

	doc->xoffset += dx;
	doc->yoffset += dy;

	if (doc->xoffset >= 0)
	{
		if (doc->xoffset >= (int)doc->width)
			doc->xoffset = doc->width - 1;
	} else {
		if (-doc->xoffset >= fb->width)
			doc->xoffset = -(fb->width - 1);
	}

	if (doc->yoffset >= 0)
	{
		if (doc->yoffset >= (int)doc->height)
			doc->yoffset = doc->height - 1;
	} else {
		if (-doc->yoffset >= fb->height)
			doc->yoffset = -(fb->height - 1);
	}
}

struct document *open_document(char *filename)
{
	struct document *doc = malloc(sizeof(*doc));
	if (doc)
	{
		struct file_info *fi;

		g_type_init();

		/* First try X11, then framebuffer. */
		doc->fbcanvas = x11canvas_create(filename);
		if (doc->fbcanvas)
		{
			doc->main_loop = x11_main_loop;
		} else {
			doc->fbcanvas = fbcanvas_create(filename);
			if (!doc->fbcanvas)
			{
				free (doc);
				doc = NULL;
				goto out;
			}

			doc->main_loop = ncurses_main_loop;
		}

		cairo_matrix_init_identity (&doc->transform);
		doc->flags = 0;
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
		doc->dump_text = document_dump_text;
		doc->set_message = document_set_message;
		doc->scroll = document_scroll;

		doc->data = doc->ops->open(doc);
	}
out:
	return doc;
}
