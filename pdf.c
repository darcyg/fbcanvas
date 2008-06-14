#include <poppler/glib/poppler.h>
#include <math.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h>
#include "fbcanvas.h"
#include "file_info.h"

static void open_pdf(struct fbcanvas *fbc, char *filename)
{
	GError *err = NULL;
	char fullname[256];
	char *canon_name = canonicalize_file_name(filename);

	/* PDF vaatii absoluuttisen "file:///tiedostonimen". */
	sprintf(fullname, "file://%s", canon_name);

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
	fbc->page = NULL;
	fbc->document = NULL;
}

/* Print lines from STR that match REGEXP.
   WHERE and PAGE specify the current filename and page number.
   Return 0 if some matches were found, else 1. */
static int grep_from_str (char *regexp, char *str, char *where, unsigned int page)
{
	regex_t re;
	regmatch_t match;
	int ret = 1;
	char *beg, *end;

	if (regcomp (&re, regexp, REG_EXTENDED))
	{
		perror ("regcomp");
		return 1;
	}

	while (! regexec (&re, str, 1, &match, 0))
	{
		ret = 0;	/* Found match. */

		beg = str + match.rm_so;
		end = str +  match.rm_eo;

		/* try to find line beginning and end. */
		while (beg > str && beg[-1] != '\n')
			beg--;
		while (*end && *end != '\n')
			end++;

		printf ("%s:%d: %.*s\n", where, page, end - beg, beg);
		str = end;
	}

	regfree (&re);
	return ret;
}

static int grep_pdf(struct fbcanvas *fbc, char *regexp)
{
	/* TODO: use real regexps. */
	int i, ret = 1;
	char *str;

	/* Set up methods & canvas size. */
	fbc->ops->update (fbc);

	for (i = 0; i < fbc->pagecount; i++)
	{
		PopplerRectangle rec = {0, 0, fbc->width, fbc->height};
		fbc->page = poppler_document_get_page (fbc->document, i);
		str = poppler_page_get_text (fbc->page, POPPLER_SELECTION_LINE, &rec);

		if (grep_from_str (regexp, str, fbc->filename, i + 1) == 0)
			ret = 0;
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

