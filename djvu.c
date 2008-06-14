#include <libdjvu/ddjvuapi.h>
#include "document.h"
#include "fbcanvas.h"
#include "file_info.h"

struct djvu_data
{
	ddjvu_context_t *context;
	ddjvu_document_t *document;
	ddjvu_page_t *page;
};

static void *open_djvu(struct document *doc)
{
	struct djvu_data *data = malloc(sizeof(*data));
	if (data)
	{
		struct fbcanvas *fbc = doc->fbcanvas;
		const ddjvu_message_t *msg;

		data->context = ddjvu_context_create("fbcanvas");
		data->document = ddjvu_document_create_by_filename(data->context,
			doc->filename, 0);
		data->page = NULL;

		/* open-metodin pitää asettaa oikea sivumäärä... */
		for (;;)
		{
			if (!ddjvu_page_decoding_done(data->page))
				ddjvu_message_wait(data->context);

			msg = ddjvu_message_peek(data->context);
			if (!msg)
				break;

			if (msg->m_any.tag == DDJVU_DOCINFO)
			{
				if (ddjvu_document_decoding_status(data->document) == DDJVU_JOB_OK)
				{
					doc->pagecount = ddjvu_document_get_pagenum(data->document);
					break;
				}
			}

			ddjvu_message_pop(data->context);
		}
	}

	return data;
}

static void close_djvu(struct document *doc)
{
	struct djvu_data *data = doc->data;
	if (data)
	{
		if (data->document)
			ddjvu_document_release(data->document);
		if (data->context)
			ddjvu_context_release(data->context);
		free (data);
	}
}

static void update_djvu(struct document *doc)
{
	struct djvu_data *data = doc->data;
	struct framebuffer *fb = doc->fbcanvas->fb;
	unsigned int rgb[] =
	{
		0xFF << 0,	/* R */
		0xFF << 8,	/* G */
		0xFF << 16,	/* B */
	};

	const ddjvu_message_t *msg;

	if (data->page)
		ddjvu_page_release(data->page);

	data->page = ddjvu_page_create_by_pageno(data->document, doc->pagenum);

	for (;;)
	{
		if (!ddjvu_page_decoding_done(data->page))
			ddjvu_message_wait(data->context);

		msg = ddjvu_message_peek(data->context);
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

		ddjvu_message_pop(data->context);
	}

	int width = ddjvu_page_get_width(data->page);
	int height = ddjvu_page_get_height(data->page);

	if (width > fb->width)
		width = fb->width;
	if (height > fb->height)
		height = fb->height;

	ddjvu_rect_t pagerec = {0, 0, width * doc->scale, height * doc->scale};
	ddjvu_rect_t renderrec = pagerec;

	ddjvu_format_t *pixelformat = ddjvu_format_create(DDJVU_FORMAT_RGBMASK32, 3, rgb);
	ddjvu_format_set_row_order(pixelformat, 1);
	ddjvu_format_set_y_direction(pixelformat, 1);
	//ddjvu_format_set_ditherbits(pixelformat, 16);

	if (doc->gdkpixbuf)
		g_object_unref(doc->gdkpixbuf);
	doc->gdkpixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
		TRUE, 8, width * doc->scale, height * doc->scale);

	if (!doc->gdkpixbuf)
		exit(1);

	ddjvu_page_render(data->page, DDJVU_RENDER_COLOR, &pagerec, &renderrec,
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


