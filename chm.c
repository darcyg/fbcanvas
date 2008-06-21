#include <stdlib.h>
#include "document.h"
#include "file_info.h"

struct chm_data
{
};

static void *open_chm(struct document *doc)
{
	struct chm_data *data = malloc(sizeof(*data));
	if (data)
	{
		/* do something... */
		fprintf(stderr, "open_chm called\n");
	}

	return data;
}

static void close_chm(struct document *doc)
{
	struct chm_data *data = doc->data;
	if (data)
	{
		free (data);
	}
}

static cairo_surface_t *update_chm(struct document *doc)
{
	struct chm_data *data = doc->data;

	fprintf(stderr, "update_chm called\n");

	exit(1);
	return NULL;
}

static struct document_ops chm_ops =
{
	.open = open_chm,
	.close = close_chm,
	.update = update_chm,
};

struct file_info chm_info = {"MS Windows HtmlHelp Data", &chm_ops};

