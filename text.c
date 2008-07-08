/* text.c - 6.7.2008 - 8.7.2008 Ari & Tero Roponen */

#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pango/pangocairo.h>
#include <unistd.h>
#include "document.h"
#include "extcmd.h"
#include "file_info.h"
#include "keymap.h"

static void close_text (struct document *doc);
static void *open_text (struct document *doc);
static char *get_text_page (struct document *doc, int page);
extern int grep_from_str (char *regexp, char *str, char *where, unsigned int page); /* pdf.c */
extern char *fb_read_line (struct document *doc, char *prompt); /* In readline.c */

#define LINE_COUNT  30

static struct text_info
{
	FILE *fp;
	int base_line;
	int xoffset;
} text_info;

static void cmd_text_key_down (struct document *doc)
{
	struct text_info *ti = doc->data;
	int line = doc->pagenum * LINE_COUNT + ti->base_line;

	line++;
	ti->base_line = line % LINE_COUNT;
	doc->pagenum = line / LINE_COUNT;
	doc->update (doc);
}

static void cmd_text_key_up (struct document *doc)
{
	struct text_info *ti = doc->data;
	int line = doc->pagenum * LINE_COUNT + ti->base_line;

	if (line > 0)
	{
		line--;
		ti->base_line = line % LINE_COUNT;
		doc->pagenum = line / LINE_COUNT;
		doc->update (doc);
	}
}

static void cmd_text_key_right (struct document *doc)
{
	struct text_info *ti = doc->data;
	ti->xoffset++;
	doc->update (doc);
}

static void cmd_text_key_left (struct document *doc)
{
	struct text_info *ti = doc->data;
	if (ti->xoffset > 0)
	{
		ti->xoffset--;
		doc->update (doc);
	}
}

static void cmd_text_revert (struct document *doc)
{
	close_text (doc);
	open_text (doc);

	reset_transformations (doc);
	doc->update (doc);
}

static void ecmd_text_find (struct document *doc, int argc, char *argv[])
{
	if (argc != 2)
		doc->set_message (doc, "Usage: find string");
	else
	{
		for (int i = doc->pagenum; i < doc->pagecount; i++)
		{
			char *str = get_text_page (doc, i);
			if (grep_from_str (argv[1], str, doc->filename, i) == 0)
			{
				doc->pagenum = i;
				doc->update (doc);
				return;
			}
		}
		doc->set_message (doc, "No matches");
	}
}

static void cmd_find (struct document *doc)
{
	char *buf = fb_read_line (doc, "Find: ");
	if (buf)
	{
		char *cmd;
		asprintf (&cmd, "%s%s", "find ", buf);
		free (buf);
		execute_extended_command (doc, cmd);
		free (cmd);
	}
}

static void setup_text_keys (void)
{
	/* Set scroll commands. */
	set_key (KEY_DOWN, cmd_text_key_down);
	set_key (KEY_UP, cmd_text_key_up);
	set_key (KEY_RIGHT, cmd_text_key_right);
	set_key (KEY_LEFT, cmd_text_key_left);

	set_key (KEY_G, cmd_text_revert);
	set_key (KEY_7 | SHIFT, cmd_find); /* "/" */

	set_extcmd ("find", ecmd_text_find);
}

static char *next_line (struct document *doc)
{
	struct text_info *ti = doc->data;
	char *buf = NULL;
	size_t size = 0;
	if (getline (&buf, &size, ti->fp) > 0)
		return buf;
	return NULL;
}

static int skip_line (struct document *doc, int count)
{
	char *tmp;
	while (count-- > 0)
	{
		tmp = next_line (doc);
		free (tmp);
		if (! tmp)
			return 0;
	}
	return 1;
}

static void *open_text (struct document *doc)
{
	FILE *fp = fopen (doc->filename,"rm");
	if (! fp)
	{
		perror ("fopen");
		abort ();
	}

	text_info.fp = fp;
	text_info.base_line = 0;
	text_info.xoffset = 0;
	doc->data = &text_info;		/* skip_line needs this */
	doc->pagecount = 1;

	while (skip_line (doc, 1))
		doc->pagecount++;

	doc->pagecount = (doc->pagecount + LINE_COUNT - 1) / LINE_COUNT;

	return &text_info;
}

static void close_text (struct document *doc)
{
	struct text_info *ti = doc->data;
	fclose (ti->fp);
}

static char *get_text_page (struct document *doc, int page)
{
	static char *text;
	struct text_info *ti = doc->data;
	char *txtbuf[LINE_COUNT];

	if (text)
	{
		free (text);
		text = NULL;
	}

	/* Page is LINE_COUNT lines. */
	/* Skip previous pages. */
	fseek (ti->fp, 0, SEEK_SET);
	skip_line (doc, ti->base_line);

	while (page-- > 0)
	{
		if (! skip_line (doc, LINE_COUNT))
			goto end_no_page;
	}

	/* Read this page (LINE_COUNT lines) */
	int linecount = 0;
	for (int i = 0; i < LINE_COUNT; i++)
	{
		txtbuf[i] = next_line (doc);
		if (txtbuf[i])
			linecount++;
		else
			break;
	}

	int len = 0;
	for (int i = 0; i < linecount; i++)
		len += strlen (txtbuf[i]);
	text = malloc (len + 1);

	int pos = 0;
	for (int i = 0; i < linecount; i++)
	{
		len = strlen (txtbuf[i]);
		memcpy (text + pos, txtbuf[i], len);
		pos += len;
		free (txtbuf[i]);
		txtbuf[i] = NULL;
	}
	text[pos] = '\0';

	return text;

end_no_page:
	return NULL;
}

static cairo_surface_t *update_text (struct document *doc)
{
	struct text_info *ti = doc->data;
	cairo_surface_t *surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
		doc->backend->width, doc->backend->height);
	cairo_t *cr = cairo_create (surf);

	/* White background. */
	cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	cairo_paint(cr);

	char *text = get_text_page (doc, doc->pagenum);
	if (! text)
	{
		text = "<End of Text>";
		doc->pagecount = doc->pagenum + 1;
	}

	PangoLayout *layout = (PangoLayout *) pango_cairo_create_layout (cr);
	pango_layout_set_text (layout, text, -1);

	PangoFontDescription *desc = pango_font_description_from_string ("Serif 12");
	pango_layout_set_font_description (layout, desc);
	pango_font_description_free (desc);

	/* Black text. */
	cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
	cairo_move_to (cr, -ti->xoffset * 14, 0);
	pango_cairo_update_layout (cr, layout);
	pango_cairo_show_layout (cr, layout);
	g_object_unref (layout);
	cairo_destroy (cr);

	return surf;
}

static struct document_ops text_ops =
{
	.open = open_text,
	.close = close_text,
	.update = update_text,
};

struct file_info utf8_text_info = {"UTF-8 ", NULL, &text_ops, setup_text_keys};
struct file_info ascii_text_info = {"ASCII ", NULL, &text_ops, setup_text_keys};
struct file_info txt_text_info = {NULL, ".txt", &text_ops, setup_text_keys};
