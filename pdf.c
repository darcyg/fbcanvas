#include "config.h"
#ifdef ENABLE_PDF

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <poppler/glib/poppler.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "document.h"
#include "file_info.h"
#include "util.h"

struct pdf_data
{
	PopplerDocument *document;
	PopplerPage *page;
	GdkPixbuf *pixbuf;
};

static void *open_pdf(struct document *doc)
{
	struct pdf_data *data = malloc(sizeof(*data));
	if (data)
	{
		GError *err = NULL;
		char fullname[256];
		char *canon_name = canonicalize_file_name(doc->filename);

		/* PDF vaatii absoluuttisen "file:///tiedostonimen". */
		sprintf(fullname, "file://%s", canon_name);

		data->pixbuf = NULL;
		data->page = NULL;
		data->document = poppler_document_new_from_file(fullname, NULL, &err);
		if (!data->document)
		{
			fprintf(stderr, "open_pdf: %s\n", err->message);
			exit(1);
		}

		free(canon_name);

		doc->pagecount = poppler_document_get_n_pages(data->document);
	}

	return data;
}

static void close_pdf(struct document *doc)
{
	struct pdf_data *data = doc->data;
	if (data)
	{
		if (data->page)
			g_object_unref(data->page);
		if (data->document)
			g_object_unref(data->document);
		free (data);
	}
}

static int grep_pdf(struct document *doc, char *regexp)
{
	struct pdf_data *data = doc->data;
	int i, ret = 1;
	char *str;

	/* Set up methods & canvas size. */
	doc->update(doc);

	for (i = 0; i < doc->pagecount; i++)
	{
		data->page = poppler_document_get_page (data->document, i);
		str = poppler_page_get_text(data->page);

		if (grep_from_str (regexp, str, doc->filename, i + 1) == 0)
			ret = 0;
	}

	return ret;
}

static cairo_surface_t *update_pdf(struct document *doc)
{
	cairo_surface_t *ret;
	struct pdf_data *data = doc->data;
	GError *err = NULL;
	static double width, height;

	if (data->page)
		g_object_unref(data->page);
	if (data->pixbuf)
		g_object_unref(data->pixbuf);

	data->page = poppler_document_get_page(data->document, doc->pagenum);
	if (!data->page)
	{
		/* TODO: käsittele virhe */
	}

	poppler_page_get_size(data->page, &width, &height);
	//fprintf(stderr, "Size: %lfx%lf\n", width, height);

#if 0
	doc->flags |= NO_GENERIC_SCALE;

	data->pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
		TRUE, 8, ceil(width * doc->scale), ceil(height * doc->scale));
	poppler_page_render_to_pixbuf(data->page, 0, 0,
		ceil(width), ceil(height), doc->scale, 0, data->pixbuf);

	ret = cairo_image_surface_create_for_data (gdk_pixbuf_get_pixels (data->pixbuf),
			CAIRO_FORMAT_ARGB32,
			gdk_pixbuf_get_width(data->pixbuf),
			gdk_pixbuf_get_height(data->pixbuf),
			gdk_pixbuf_get_rowstride(data->pixbuf));
#else
	ret = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, ceil(width), ceil(height));
	cairo_t *cr = cairo_create(ret);
	poppler_page_render(data->page, cr);
	cairo_destroy(cr);
#endif

	return ret;
}

static void dump_text_pdf(struct document *doc, char *filename)
{
	struct pdf_data *data = doc->data;
	char *str = poppler_page_get_text(data->page);
	if (str)
	{
		FILE *fp = fopen(filename, "w+");
		if (fp)
		{
			fprintf(fp, "%s", str);
			fclose (fp);
		}
	}
}

static struct document_ops pdf_ops =
{
	.open = open_pdf,
	.close = close_pdf,
	.update = update_pdf,
	.grep = grep_pdf,
	.dump_text = dump_text_pdf,
};

struct file_info pdf_info = {"PDF", NULL, &pdf_ops};

#endif /* ENABLE_PDF */
