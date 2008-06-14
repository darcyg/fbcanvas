#include <math.h>
#include <stdlib.h>
#include "document.h"
#include "fbcanvas.h"
#include "file_info.h"

static void *open_pixbuf(struct document *doc)
{
	/* pixbuf ei tarvitse open-metodia. */
	return NULL;
}

static void close_pixbuf(struct document *doc)
{
	/* pixbuf ei tarvitse close-metodia. */
}

static void update_pixbuf(struct document *doc)
{
	GError *err = NULL;
	doc->gdkpixbuf = gdk_pixbuf_new_from_file(doc->filename, &err);
	if (err)
	{
		fprintf (stderr, "%s", err->message);
		exit(1);
	}

	doc->gdkpixbuf = gdk_pixbuf_add_alpha(doc->gdkpixbuf, FALSE, 0, 0, 0);

	if (doc->scale != 1.0)
	{
		GdkPixbuf *tmp = gdk_pixbuf_scale_simple(doc->gdkpixbuf,
			ceil(doc->scale * gdk_pixbuf_get_width(doc->gdkpixbuf)),
			ceil(doc->scale * gdk_pixbuf_get_height(doc->gdkpixbuf)),
			GDK_INTERP_BILINEAR);
		g_object_unref(doc->gdkpixbuf);
		doc->gdkpixbuf = tmp;
	}

	doc->width = gdk_pixbuf_get_width(doc->gdkpixbuf);
	doc->height = gdk_pixbuf_get_height(doc->gdkpixbuf);
}

static struct document_ops pixbuf_ops =
{
	.open = open_pixbuf,
	.close = close_pixbuf,
	.update = update_pixbuf,
};

/* Nämä toimivat suoraan pixbufin avulla */
struct file_info bmp_info = {"PC bitmap", &pixbuf_ops};
struct file_info gif_info = {"GIF image data", &pixbuf_ops};
struct file_info jpg_info = {"JPEG", &pixbuf_ops};
struct file_info pcx_info = {"PCX", &pixbuf_ops};
struct file_info png_info = {"PNG", &pixbuf_ops};
struct file_info ppm_info = {"Netpbm PPM", &pixbuf_ops};
struct file_info tiff_info = {"TIFF image data", &pixbuf_ops};
struct file_info xpm_info = {"X pixmap image text", &pixbuf_ops};

