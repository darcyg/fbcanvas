/* file_info.c - 13.6.2008 - 15.6.2008 Ari & Tero Roponen */
#include <glib.h>
#include <magic.h>
#include <stdio.h>
#include <string.h>
#include "file_info.h"

static GArray *file_infos;

void register_plugins (void)
{
	extern struct file_info bmp_info;
	extern struct file_info chm_info;
	extern struct file_info djvu_info;
	extern struct file_info gif_info;
	extern struct file_info jpg_info;
	extern struct file_info pcx_info;
	extern struct file_info pdf_info;
	extern struct file_info png_info;
	extern struct file_info ppm_info;
	extern struct file_info tiff_info;
	extern struct file_info xpm_info;

	struct file_info *fi[] =
	{
		&bmp_info,
		&chm_info,
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

	if (!file_infos)
		file_infos = g_array_new (TRUE, FALSE, sizeof (struct file_info *));

	for (int i = 0; fi[i]; i++)
		g_array_append_val (file_infos, fi[i]);
}

struct file_info *get_file_info (char *filename)
{
	struct file_info *fi = NULL;
	magic_t magic;
	int ret, i;
	const char *type;

	if (! file_infos)
		return NULL;

	magic = magic_open (MAGIC_NONE);
	ret = magic_load (magic, NULL);
	type = magic_file (magic, filename);

	if (! type)
	{
		fprintf (stderr, "Could not determine file type: %s\n", filename);
		return NULL;
	}

	for (i = 0;
	     (fi = g_array_index (file_infos, struct file_info *, i)) != NULL;
	     i++)
	{
		if (! strncmp (type, fi->type, strlen (fi->type)))
			break;
	}

	if (!fi) /* TODO: Try to identify file by its extension */
		fprintf (stderr, "Unsupported file type: %s\n", type);

	magic_close (magic);
	return fi;
}
