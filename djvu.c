#include <libdjvu/ddjvuapi.h>
#include "fbcanvas.h"
#include "file_info.h"

static void open_djvu(struct fbcanvas *fbc, char *filename)
{
	const ddjvu_message_t *msg;

	fbc->context = ddjvu_context_create("fbcanvas");
	fbc->document = ddjvu_document_create_by_filename(fbc->context, filename, 0);

	/* open-metodin pitää asettaa oikea sivumäärä... */
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
				break;
			}
		}

		ddjvu_message_pop(fbc->context);
	}
}

static void close_djvu(struct fbcanvas *fbc)
{
	if (fbc->document)
		ddjvu_document_release(fbc->document);
	if (fbc->context)
		ddjvu_context_release(fbc->context);
}

static void update_djvu(struct fbcanvas *fbc)
{
	struct framebuffer *fb = fbc->fb;
	unsigned int rgb[] =
	{
		0xFF << 0,	/* R */
		0xFF << 8,	/* G */
		0xFF << 16,	/* B */
	};

	const ddjvu_message_t *msg;

	if (fbc->page)
		ddjvu_page_release(fbc->page);

	fbc->page = ddjvu_page_create_by_pageno(fbc->document, fbc->pagenum);

	for (;;)
	{
		if (!ddjvu_page_decoding_done(fbc->page))
			ddjvu_message_wait(fbc->context);

		msg = ddjvu_message_peek(fbc->context);
		if (!msg)
			break;

		switch (msg->m_any.tag)
		{
		case DDJVU_NEWSTREAM:
			//fprintf(stderr, "DDJVU_NEWSTREAM\n");
			break;
		case DDJVU_PAGEINFO:
			//fprintf(stderr, "DDJVU_PAGEINFO\n");
			break;
		case DDJVU_ERROR:
			//fprintf(stderr, "DDJVU_ERROR\n");
			break;
		case DDJVU_INFO:
			//fprintf(stderr, "DDJVU_INFO\n");
			break;
		case DDJVU_CHUNK:
			//fprintf(stderr, "DDJVU_CHUNK\n");
			break;
		case DDJVU_THUMBNAIL:
			//fprintf(stderr, "DDJVU_THUMBNAIL\n");
			break;
		case DDJVU_PROGRESS:
			//fprintf(stderr, "DDJVU_PROGRESS\n");
			break;
		case DDJVU_RELAYOUT:
			//fprintf(stderr, "DDJVU_RELAYOUT\n");
			break;
		case DDJVU_REDISPLAY:
			//fprintf(stderr, "DDJVU_REDISPLAY\n");
			break;
		case DDJVU_DOCINFO:
			//fprintf(stderr, "DDJVU_DOCINFO\n");
			break;
		}

		ddjvu_message_pop(fbc->context);
	}

	int width = ddjvu_page_get_width(fbc->page);
	int height = ddjvu_page_get_height(fbc->page);

	if (width > fb->width)
		width = fb->width;
	if (height > fb->height)
		height = fb->height;

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


