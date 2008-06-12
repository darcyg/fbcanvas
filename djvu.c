#include <libdjvu/ddjvuapi.h>
#include "fbcanvas.h"

/* TODO: siirrä nämä oikeaan paikkaan */
static ddjvu_context_t *context;
static ddjvu_document_t *document;
static const ddjvu_message_t *msg;
static ddjvu_page_t *page;

static void open_djvu(struct fbcanvas *fbc, char *filename)
{
	fbc->page = NULL; // TODO: tyhjennä tämä yleisessä koodissa
	fbc->pagenum = 0;
	fbc->pagecount = 1; //TODO: aseta oikein
	page = NULL;

	context = ddjvu_context_create("fbcanvas");
	document = ddjvu_document_create_by_filename(context, filename, 0);

	/* Odotellaan DDJVU_DOCINFO-viestin saapumista. */
	for (;;)
	{
		if (!ddjvu_page_decoding_done(page))
			ddjvu_message_wait(context);

		msg = ddjvu_message_peek(context);
		if (!msg)
			break;

		if (msg->m_any.tag == DDJVU_DOCINFO)
		{
			if (ddjvu_document_decoding_status(document) == DDJVU_JOB_OK)
			{
				fbc->pagecount = ddjvu_document_get_pagenum(document);
				page = ddjvu_page_create_by_pageno(document, fbc->pagenum);
				break;
			}
		}

		ddjvu_message_pop(context);
	}

	while (!ddjvu_page_decoding_done(page))
		sleep(1);

	fprintf(stderr, "Decoding stopped\n");
}

static void close_djvu(struct fbcanvas *fbc)
{
}

static void update_djvu(struct fbcanvas *fbc)
{
	int width = ddjvu_page_get_width(page);
	int height = ddjvu_page_get_height(page);

	ddjvu_rect_t pagerec = {0, 0, width*fbc->scale, height*fbc->scale};
	ddjvu_rect_t renderrec = pagerec;

	ddjvu_format_t *pixelformat = ddjvu_format_create(DDJVU_FORMAT_BGR24, 0, NULL);
	ddjvu_format_set_row_order(pixelformat, 1);
	ddjvu_format_set_y_direction(pixelformat, 1);

	if (fbc->gdkpixbuf)
		g_object_unref(fbc->gdkpixbuf);
	fbc->gdkpixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
		TRUE, 8, width*fbc->scale, height*fbc->scale);

	if (!fbc->gdkpixbuf)
		exit(1);

	fbc->width = gdk_pixbuf_get_width(fbc->gdkpixbuf);
	fbc->height = gdk_pixbuf_get_height(fbc->gdkpixbuf);


	ddjvu_page_render(page, DDJVU_RENDER_COLOR, &pagerec, &renderrec,
		pixelformat, fbc->width * 4, gdk_pixbuf_get_pixels(fbc->gdkpixbuf));

	ddjvu_format_release (pixelformat);
}

static struct file_ops djvu_ops =
{
	.open = open_djvu,
	.close = close_djvu,
	.update = update_djvu,
//	.grep = grep_djvu,
};

struct file_info djvu_info = {"DjVu Image file", &djvu_ops};


