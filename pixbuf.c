#include <math.h>
#include <stdlib.h>
#include "document.h"
#include "fbcanvas.h"
#include "file_info.h"

struct pixbuf_data
{
	GdkPixbuf *pixbuf;
};

static void *open_pixbuf(struct document *doc)
{
	struct pixbuf_data *data = malloc(sizeof(*data));
	return data;
}

static void close_pixbuf(struct document *doc)
{
	struct pixbuf_data *data = doc->data;
	if (data)
	{
		if (data->pixbuf)
			g_object_unref(data->pixbuf);
		free(data);
	}
}

static void update_pixbuf(struct document *doc)
{
	struct pixbuf_data *data = doc->data;

	GError *err = NULL;
	GdkPixbuf *tmp = gdk_pixbuf_new_from_file(doc->filename, &err);
	if (err)
	{
		fprintf (stderr, "%s", err->message);
		exit(1);
	}

	data->pixbuf = gdk_pixbuf_add_alpha(tmp, FALSE, 0, 0, 0);
	g_object_unref(tmp);

	if (doc->scale != 1.0)
	{
		tmp = gdk_pixbuf_scale_simple(data->pixbuf,
			ceil(doc->scale * gdk_pixbuf_get_width(data->pixbuf)),
			ceil(doc->scale * gdk_pixbuf_get_height(data->pixbuf)),
			GDK_INTERP_BILINEAR);
		g_object_unref(data->pixbuf);
		data->pixbuf = tmp;
	}

	doc->cairo = cairo_image_surface_create_for_data (gdk_pixbuf_get_pixels(data->pixbuf),
		CAIRO_FORMAT_ARGB32,
		gdk_pixbuf_get_width(data->pixbuf),
		gdk_pixbuf_get_height(data->pixbuf),
		gdk_pixbuf_get_rowstride(data->pixbuf));
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
struct file_info png_info = {"PNG", &pixbuf_ops};
struct file_info pcx_info = {"PCX", &pixbuf_ops};
struct file_info ppm_info = {"Netpbm PPM", &pixbuf_ops};
struct file_info tiff_info = {"TIFF image data", &pixbuf_ops};
struct file_info xpm_info = {"X pixmap image text", &pixbuf_ops};

