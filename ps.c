#include <ghostscript/gdevdsp.h>
#include <ghostscript/iapi.h>
#include <ghostscript/ierrors.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <math.h>
#include <stdlib.h>
#include "document.h"
#include "file_info.h"

struct postscript_data
{
	GdkPixbuf *pixbuf;
	void *instance;
};

static void *open_postscript(struct document *doc)
{
	int err;

	static char *args[] =
	{
		"fb", /* argv[0] */
		"-q",
		"-sDEVICE=png48",
		"-sOutputFile=/tmp/fb-%d.png",
		"-dBATCH",
		"-dNOPAUSE",
		NULL
	};

	struct postscript_data *data = malloc(sizeof(*data));
	if (data)
	{
		data->pixbuf = NULL;
		data->instance = NULL;
	}

	gsapi_new_instance(&data->instance, NULL);

	err = gsapi_init_with_args(data->instance, 6, args);
	if (err == e_Quit)
	{
		gsapi_exit(data->instance);
		goto out_free;
	} else if (err == e_Info) {
		goto out_free;
	}

	gsapi_run_file(data->instance, doc->filename, 0, &err);
	gsapi_exit(data->instance);
	gsapi_delete_instance(data->instance);
out:
	return data;
out_free:
	free(data);
	return NULL;
}

static void close_postscript(struct document *doc)
{
	struct postscript_data *data = doc->data;
	if (data)
	{
		if (data->pixbuf)
			g_object_unref(data->pixbuf);
		free(data);
	}
}

static cairo_surface_t *update_postscript(struct document *doc)
{
	struct postscript_data *data = doc->data;

	GError *err = NULL;
	GdkPixbuf *tmp = gdk_pixbuf_new_from_file("/tmp/fb-1.png", &err);
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

static struct document_ops postscript_ops =
{
	.open = open_postscript,
	.close = close_postscript,
	.update = update_postscript,
};

struct file_info ps_info = {"PostScript", NULL, &postscript_ops};
