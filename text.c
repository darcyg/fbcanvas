/* text.c - 6.7.2008 - 6.7.2008 Ari & Tero Roponen */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pango/pangocairo.h>
#include <unistd.h>
#include "document.h"
#include "file_info.h"

static void *open_text (struct document *doc)
{
	FILE *fp = fopen (doc->filename,"rm");
	if (! fp)
	{
		perror ("fopen");
		abort ();
	}

	doc->pagecount = 999;	/* XXX */
	return fp;
}

static void close_text (struct document *doc)
{
	fclose ((FILE *)doc->data);
}

static char *get_text_page (FILE *fp, int page)
{
	static char lines[20][128]; /* XXX */
	char *text;

	if (text)
	{
		free (text);
		text = NULL;
	}

	/* Page is 20 lines. */
	/* Skip previous pages. */
	fseek (fp, 0, SEEK_SET);

	while (page-- > 0)
	{
		// skip 20 lines
		for (int i = 0; i < 20; i++)
		{
			if (! fgets (lines[0], 128, fp))
				goto end_no_page;
		}
	}

	/* Read this page (20 lines) */
	int linecount = 0;
	for (int i = 0; i < 20; i++)
	{
		if (fgets (lines[i], 128, fp))
			linecount++;
		else
			break;
	}

	int len = 0;
	for (int i = 0; i < linecount; i++)
		len += strlen (lines[i]);
	text = malloc (len + 1);
	int pos = 0;

	for (int i = 0; i < linecount; i++)
	{
		len = strlen (lines[i]);
		memcpy (text + pos, lines[i], len);
		pos += len;
	}
	text[pos] = '\0';
	return text;

end_no_page:
	return NULL;
}

static cairo_surface_t *update_text (struct document *doc)
{
	cairo_surface_t *surf =
		cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 800, 600);
	cairo_t *cr = cairo_create (surf);

	/* White background. */
	cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	cairo_paint(cr);

	char *text = get_text_page (doc->data, doc->pagenum);
	if (! text)
	{
		text = "<End of Text>";
		doc->pagecount = doc->pagenum + 1;
	}

	PangoLayout *layout = (PangoLayout *) pango_cairo_create_layout (cr);
	pango_layout_set_text (layout, text, -1);

	PangoFontDescription *desc = pango_font_description_from_string ("Sans 14");
	pango_layout_set_font_description (layout, desc);
	pango_font_description_free (desc);

	/* Black text. */
	cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
//	cairo_move_to (cr, 8, 0); /* after the cursor */
	pango_cairo_update_layout (cr, layout);
	pango_cairo_show_layout (cr, layout);
	g_object_unref (layout);
	cairo_destroy (cr);

	return surf;
}

static struct document_ops text_ops =
{
	.open = open_text,
	.close = close_text,
	.update = update_text,
};

struct file_info utf8_text_info = {"UTF-8 ", &text_ops};
struct file_info ascii_text_info = {"ASCII ", &text_ops};
