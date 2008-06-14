#include <libdjvu/ddjvuapi.h>
#include "document.h"
#include "fbcanvas.h"
#include "file_info.h"

static void open_djvu(struct document *doc)
{
	struct fbcanvas *fbc = doc->fbcanvas;
	const ddjvu_message_t *msg;

	doc->context = ddjvu_context_create("fbcanvas");
	doc->document = ddjvu_document_create_by_filename(doc->context, doc->filename, 0);

	/* open-metodin pitää asettaa oikea sivumäärä... */
	for (;;)
	{
		if (!ddjvu_page_decoding_done(doc->page))
			ddjvu_message_wait(doc->context);

		msg = ddjvu_message_peek(doc->context);
		if (!msg)
			break;

		if (msg->m_any.tag == DDJVU_DOCINFO)
		{
			if (ddjvu_document_decoding_status(doc->document) == DDJVU_JOB_OK)
			{
				doc->pagecount = ddjvu_document_get_pagenum(doc->document);
				break;
			}
		}

		ddjvu_message_pop(doc->context);
	}
}

static void close_djvu(struct document *doc)
{
	if (doc->document)
		ddjvu_document_release(doc->document);
	if (doc->context)
		ddjvu_context_release(doc->context);
}

static void update_djvu(struct document *doc)
{
	struct framebuffer *fb = doc->fbcanvas->fb;
	unsigned int rgb[] =
	{
		0xFF << 0,	/* R */
		0xFF << 8,	/* G */
		0xFF << 16,	/* B */
	};

	const ddjvu_message_t *msg;

	if (doc->page)
		ddjvu_page_release(doc->page);

	doc->page = ddjvu_page_create_by_pageno(doc->document, doc->pagenum);

	for (;;)
	{
		if (!ddjvu_page_decoding_done(doc->page))
			ddjvu_message_wait(doc->context);

		msg = ddjvu_message_peek(doc->context);
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

		ddjvu_message_pop(doc->context);
	}

	int width = ddjvu_page_get_width(doc->page);
	int height = ddjvu_page_get_height(doc->page);

	if (width > fb->width)
		width = fb->width;
	if (height > fb->height)
		height = fb->height;

	ddjvu_rect_t pagerec = {0, 0, width*doc->scale, height*doc->scale};
	ddjvu_rect_t renderrec = pagerec;

	ddjvu_format_t *pixelformat = ddjvu_format_create(DDJVU_FORMAT_RGBMASK32, 3, rgb);
	ddjvu_format_set_row_order(pixelformat, 1);
	ddjvu_format_set_y_direction(pixelformat, 1);
	//ddjvu_format_set_ditherbits(pixelformat, 16);

	if (doc->gdkpixbuf)
		g_object_unref(doc->gdkpixbuf);
	doc->gdkpixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
		TRUE, 8, width*doc->scale, height*doc->scale);

	if (!doc->gdkpixbuf)
		exit(1);

	doc->width = gdk_pixbuf_get_width(doc->gdkpixbuf);
	doc->height = gdk_pixbuf_get_height(doc->gdkpixbuf);

	ddjvu_page_render(doc->page, DDJVU_RENDER_COLOR, &pagerec, &renderrec,
		pixelformat, doc->scale * width * 4, gdk_pixbuf_get_pixels(doc->gdkpixbuf));

	ddjvu_format_release (pixelformat);
}

static struct document_ops djvu_ops =
{
	.open = open_djvu,
	.close = close_djvu,
	.update = update_djvu,
//	.grep = grep_djvu,
};

struct file_info djvu_info = {"DjVu Image file", &djvu_ops};


