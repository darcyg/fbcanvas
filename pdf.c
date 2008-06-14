#include <poppler/glib/poppler.h>
#include <math.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h>
#include "document.h"
#include "fbcanvas.h"
#include "file_info.h"

static void open_pdf(struct document *doc)
{
	GError *err = NULL;
	char fullname[256];
	char *canon_name = canonicalize_file_name(doc->filename);

	/* PDF vaatii absoluuttisen "file:///tiedostonimen". */
	sprintf(fullname, "file://%s", canon_name);

	doc->document = poppler_document_new_from_file(fullname, NULL, &err);
	if (!doc->document)
	{
		fprintf(stderr, "open_pdf: %s\n", err->message);
		exit(1);
	}

	free(canon_name);
	doc->pagecount = poppler_document_get_n_pages(doc->document);
}

static void close_pdf(struct document *doc)
{
	if (doc->page)
		g_object_unref(doc->page);
	if (doc->document)
		g_object_unref(doc->document);
	doc->page = NULL;
	doc->document = NULL;
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

static int grep_pdf(struct document *doc, char *regexp)
{
	/* TODO: use real regexps. */
	int i, ret = 1;
	char *str;

	/* Set up methods & canvas size. */
	doc->ops->update(doc);

	for (i = 0; i < doc->pagecount; i++)
	{
		PopplerRectangle rec = {0, 0, doc->width, doc->height};
		doc->page = poppler_document_get_page (doc->document, i);
		str = poppler_page_get_text (doc->page, POPPLER_SELECTION_LINE, &rec);

		if (grep_from_str (regexp, str, doc->filename, i + 1) == 0)
			ret = 0;
	}

	return ret;
}

static void update_pdf(struct document *doc)
{
	GError *err = NULL;
	static double width, height;

	if (doc->page)
		g_object_unref(doc->page);
	if (doc->gdkpixbuf)
		g_object_unref(doc->gdkpixbuf);

	doc->page = poppler_document_get_page(doc->document, doc->pagenum);
	if (!doc->page)
	{
		/* TODO: käsittele virhe */
	}

	poppler_page_get_size(doc->page, &width, &height);
	//fprintf(stderr, "Size: %lfx%lf\n", width, height);

	doc->gdkpixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
		TRUE, 8, ceil(width * doc->scale), ceil(height * doc->scale));
	poppler_page_render_to_pixbuf(doc->page, 0, 0,
		ceil(width), ceil(height), doc->scale, 0, doc->gdkpixbuf);

	doc->width = gdk_pixbuf_get_width(doc->gdkpixbuf);
	doc->height = gdk_pixbuf_get_height(doc->gdkpixbuf);
}

static struct document_ops pdf_ops =
{
	.open = open_pdf,
	.close = close_pdf,
	.update = update_pdf,
	.grep = grep_pdf,
};

struct file_info pdf_info = {"PDF", &pdf_ops};

