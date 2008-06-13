/* file_info.c - 13.6.2008 - 13.6.2008 Ari & Tero Roponen */
#include <magic.h>
#include <string.h>
#include "file_info.h"

extern struct file_info bmp_info;
extern struct file_info djvu_info;
extern struct file_info gif_info;
extern struct file_info jpg_info;
extern struct file_info pcx_info;
extern struct file_info pdf_info;
extern struct file_info png_info;
extern struct file_info ppm_info;
extern struct file_info tiff_info;
extern struct file_info xpm_info;

static struct file_info *file_info[] =
{
	&bmp_info,
	&djvu_info,
	&gif_info,
	&jpg_info,
	&pcx_info,
	&pdf_info,
	&png_info,
	&ppm_info,
	&tiff_info,
	&xpm_info,
	NULL
};

struct file_info *get_file_info (char *filename)
{
	struct file_info **fi = NULL;
	magic_t magic = magic_open (MAGIC_NONE);
	int ret = magic_load (magic, NULL);
	const char *type = magic_file (magic, filename);

	if (! type)
	{
		fprintf (stderr, "Could not determine file type: %s\n", filename);
		return NULL;
	}

	for (fi = file_info; *fi; fi++)
	{
		if (! strncmp (type, (*fi)->type, strlen ((*fi)->type)))
			break;
	}
	if (! *fi) /* TODO: Try to identify file by its extension */
		fprintf (stderr, "Unsupported file type: %s\n", type);

	magic_close (magic);
	return *fi;
}
