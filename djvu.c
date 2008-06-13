#include <libdjvu/ddjvuapi.h>
#include "fbcanvas.h"

static void open_djvu(struct fbcanvas *fbc, char *filename)
{
	const ddjvu_message_t *msg;

	fbc->context = ddjvu_context_create("fbcanvas");
	fbc->document = ddjvu_document_create_by_filename(fbc->context, filename, 0);

	/* Odotellaan DDJVU_DOCINFO-viestin saapumista. */
	for (;;)
	{
		if (!ddjvu_page_decoding_done(fbc->page))
			ddjvu_message_wait(fbc->context);

		msg = ddjvu_message_peek(fbc->context);
		if (!msg)
			break;

		if (msg->m_any.tag == DDJVU_DOCINFO)
		{
			if (ddjvu_document_decoding_status(fbc->document) == DDJVU_JOB_OK)
			{
				fbc->pagecount = ddjvu_document_get_pagenum(fbc->document);
				fbc->page = ddjvu_page_create_by_pageno(fbc->document,
					fbc->pagenum);
				break;
			}
		}

		ddjvu_message_pop(fbc->context);
	}

	while (!ddjvu_page_decoding_done(fbc->page))
		sleep(1);

	fprintf(stderr, "Decoding stopped\n");
}

static void close_djvu(struct fbcanvas *fbc)
{
	/* TODO: toteuta */
}

static void update_djvu(struct fbcanvas *fbc)
{
	unsigned int rgb[] =
	{
		0b00000000000000001111100000000000, /* R */
		0b00000000000000000000011111100000, /* G */
		0b00000000000000000000000000011111, /* B */
	};

	int width = ddjvu_page_get_width(fbc->page);
	int height = ddjvu_page_get_height(fbc->page);

	ddjvu_rect_t pagerec = {0, 0, width*fbc->scale, height*fbc->scale};
	ddjvu_rect_t renderrec = pagerec;

	ddjvu_format_t *pixelformat = ddjvu_format_create(DDJVU_FORMAT_RGBMASK32, 3, rgb);
	ddjvu_format_set_row_order(pixelformat, 1);
	ddjvu_format_set_y_direction(pixelformat, 1);
	//ddjvu_format_set_ditherbits(pixelformat, 16);

	if (fbc->gdkpixbuf)
		g_object_unref(fbc->gdkpixbuf);
	fbc->gdkpixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
		TRUE, 8, width*fbc->scale, height*fbc->scale);

	if (!fbc->gdkpixbuf)
		exit(1);

	fbc->width = gdk_pixbuf_get_width(fbc->gdkpixbuf);
	fbc->height = gdk_pixbuf_get_height(fbc->gdkpixbuf);


	ddjvu_page_render(fbc->page, DDJVU_RENDER_COLOR, &pagerec, &renderrec,
		pixelformat, fbc->scale * width * 4, gdk_pixbuf_get_pixels(fbc->gdkpixbuf));

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


