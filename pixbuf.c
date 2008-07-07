#include <gdk-pixbuf/gdk-pixbuf.h>
#include <math.h>
#include <stdlib.h>
#include "document.h"
#include "file_info.h"

struct pixbuf_data
{
	GdkPixbuf *pixbuf;
};

static void *open_pixbuf(struct document *doc)
{
	struct pixbuf_data *data = malloc(sizeof(*data));
	if (data)
		data->pixbuf = NULL;
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

static cairo_surface_t *update_pixbuf(struct document *doc)
{
	struct pixbuf_data *data = doc->data;

	GError *err = NULL;
	GdkPixbuf *tmp = gdk_pixbuf_new_from_file(doc->filename, &err);
	if (err)
	{
		fprintf (stderr, "%s", err->message);
		exit(1);
	}

	if (data->pixbuf)
		g_object_unref(data->pixbuf);

	data->pixbuf = gdk_pixbuf_add_alpha(tmp, FALSE, 0, 0, 0);
	g_object_unref(tmp);

	return cairo_image_surface_create_for_data (gdk_pixbuf_get_pixels(data->pixbuf),
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
struct file_info bmp_info = {"PC bitmap", NULL, &pixbuf_ops};
struct file_info gif_info = {"GIF image data", NULL, &pixbuf_ops};
struct file_info jpg_info = {"JPEG", NULL, &pixbuf_ops};
struct file_info png_info = {"PNG", NULL, &pixbuf_ops};
struct file_info pcx_info = {"PCX", NULL, &pixbuf_ops};
struct file_info ppm_info = {"Netpbm PPM", NULL, &pixbuf_ops};
struct file_info tiff_info = {"TIFF image data", NULL, &pixbuf_ops};
struct file_info xbm_info = {NULL, ".xbm", &pixbuf_ops};
struct file_info xpm_info = {"X pixmap image text", NULL, &pixbuf_ops};
