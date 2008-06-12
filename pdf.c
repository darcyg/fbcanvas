#include <poppler/glib/poppler.h>
#include "fbcanvas.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static void open_pdf(struct fbcanvas *fbc, char *filename)
{
	GError *err = NULL;
	char fullname[256];
	char *canon_name = canonicalize_file_name(filename);

	/* PDF vaatii absoluuttisen "file:///tiedostonimen". */
	sprintf(fullname, "file://%s", canon_name);

	fbc->page = NULL;
	fbc->document = poppler_document_new_from_file(fullname, NULL, &err);
	if (!fbc->document)
	{
		fprintf(stderr, "open_pdf: %s\n", err->message);
		exit(1);
	}

	free(canon_name);
	fbc->pagecount = poppler_document_get_n_pages(fbc->document);
}

static void close_pdf(struct fbcanvas *fbc)
{
	if (fbc->page)
		g_object_unref(fbc->page);
	if (fbc->document)
		g_object_unref(fbc->document);
	if (fbc->gdkpixbuf)
		g_object_unref(fbc->gdkpixbuf);
	fbc->page = NULL;
	fbc->document = NULL;
	fbc->gdkpixbuf = NULL;
}

static int grep_pdf(struct fbcanvas *fbc, char *regexp)
{
	/* TODO: use real regexps. */
	int i, ret = 1, len = strlen (regexp);
	char *str, *beg, *end;

	/* Set up methods & canvas size. */
	fbc->ops->update (fbc);

	for (i = 0; i < fbc->pagecount; i++)
	{
		PopplerRectangle rec = {0, 0, fbc->width, fbc->height};
		fbc->page = poppler_document_get_page (fbc->document, i);
		str = poppler_page_get_text (fbc->page, POPPLER_SELECTION_LINE, &rec);

		while (str)
		{
			beg = strstr (str, regexp);
			end = beg + len;

			if (beg)
			{
				ret = 0; /* found match */

				/* try to find line beginning and end. */
				while (beg > str && beg[-1] != '\n')
					beg--;
				while (*end && *end != '\n')
					end++;

				printf ("%s:%d: %.*s\n", fbc->filename, i + 1, end - beg, beg);
				str = end;
			} else str = NULL;
		}
	}

	return ret;
}
static void update_pdf(struct fbcanvas *fbc)
{
	GError *err = NULL;
	static double width, height;

	if (fbc->page)
		g_object_unref(fbc->page);
	if (fbc->gdkpixbuf)
		g_object_unref(fbc->gdkpixbuf);

	fbc->page = poppler_document_get_page(fbc->document, fbc->pagenum);
	if (!fbc->page)
	{
		/* TODO: käsittele virhe */
	}

	poppler_page_get_size(fbc->page, &width, &height);
	//fprintf(stderr, "Size: %lfx%lf\n", width, height);

	fbc->gdkpixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
		TRUE, 8, ceil(width * fbc->scale), ceil(height * fbc->scale));
	poppler_page_render_to_pixbuf(fbc->page, 0, 0,
		ceil(width), ceil(height), fbc->scale, 0, fbc->gdkpixbuf);

	fbc->width = gdk_pixbuf_get_width(fbc->gdkpixbuf);
	fbc->height = gdk_pixbuf_get_height(fbc->gdkpixbuf);
}

static struct file_ops pdf_ops =
{
	.open = open_pdf,
	.close = close_pdf,
	.update = update_pdf,
	.grep = grep_pdf,
};

struct file_info pdf_info = {"PDF", &pdf_ops};

