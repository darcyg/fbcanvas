#include "fbcanvas.h"
#include <math.h>
#include <stdlib.h>

static void open_pixbuf(struct fbcanvas *fbc, char *filename)
{
	/* pixbuf ei tarvitse open-metodia. */
}

static void close_pixbuf(struct fbcanvas *fbc)
{
	/* pixbuf ei tarvitse close-metodia. */
}

static void update_pixbuf(struct fbcanvas *fbc)
{
	GError *err = NULL;
	fbc->gdkpixbuf = gdk_pixbuf_new_from_file(fbc->filename, &err);
	if (err)
	{
		fprintf (stderr, "%s", err->message);
		exit(1);
	}

	fbc->gdkpixbuf = gdk_pixbuf_add_alpha(fbc->gdkpixbuf, FALSE, 0, 0, 0);

	if (fbc->scale != 1.0)
	{
		GdkPixbuf *tmp = gdk_pixbuf_scale_simple(fbc->gdkpixbuf,
			ceil(fbc->scale * gdk_pixbuf_get_width(fbc->gdkpixbuf)),
			ceil(fbc->scale * gdk_pixbuf_get_height(fbc->gdkpixbuf)),
			GDK_INTERP_BILINEAR);
		g_object_unref(fbc->gdkpixbuf);
		fbc->gdkpixbuf = tmp;
	}

	fbc->width = gdk_pixbuf_get_width(fbc->gdkpixbuf);
	fbc->height = gdk_pixbuf_get_height(fbc->gdkpixbuf);
}

static struct file_ops pixbuf_ops =
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

