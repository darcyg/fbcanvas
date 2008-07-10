/* util.c - 10.7.2008 - 10.7.2008 Ari & Tero Roponen */
#include <cairo/cairo.h>
#include <regex.h>
#include <stdio.h>
#include "util.h"

/* Print lines from STR that match REGEXP.
   WHERE and PAGE specify the current filename and page number.
   If WHERE is NULL, don't display matches.
   Return 0 if some matches were found, else 1. */
int grep_from_str (char *regexp, char *str, char *where, unsigned int page)
{
	regex_t re;
	regmatch_t match;
	int ret = 1;
	char *beg, *end;

	if (regcomp (&re, regexp, REG_EXTENDED))
	{
		perror ("regcomp");
		return 1;
	}

	while (! regexec (&re, str, 1, &match, 0))
	{
		ret = 0;	/* Found match. */
		if (! where)
			break;

		beg = str + match.rm_so;
		end = str +  match.rm_eo;

		/* try to find line beginning and end. */
		while (beg > str && beg[-1] != '\n')
			beg--;
		while (*end && *end != '\n')
			end++;

		printf ("%s:%d: %.*s\n", where, page, end - beg, beg);
		str = end;
	}

	regfree (&re);
	return ret;
}

void convert_surface_argb_to_abgr (cairo_surface_t *surface)
{
	int width = cairo_image_surface_get_width (surface);
	int height = cairo_image_surface_get_height (surface);
	int stride = cairo_image_surface_get_stride (surface);
	char *data = cairo_image_surface_get_data (surface);

	// ARGB -> ABGR?
	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			unsigned int *p = (unsigned int *)(char *)(data + j * stride) + i;
			*p = ((((*p & 0xff) >> 0) << 16)
			      | (*p & 0xff00)
			      | (((*p & 0xff0000) >> 16) << 0)
			      | (*p & 0xff000000));
		}
	}
}
