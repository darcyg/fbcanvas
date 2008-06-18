/* commands.c - 13.6.2008 - 18.6.2008 Ari & Tero Roponen */
#include <cairo/cairo.h>
#include <ncurses.h>
#undef scroll
#include "commands.h"
#include "document.h"
#include "keymap.h"

jmp_buf exit_loop;
int this_command;
int last_command;

static void cmd_unbound (struct document *doc)
{
	printf ("\a"); /* bell */
	fflush (stdout);
}

static void cmd_quit (struct document *doc)
{
	longjmp (exit_loop, 1);
}

static void cmd_redraw (struct document *doc)
{
	/* Nothing to do */
}

static void cmd_next_page (struct document *doc)
{
	if (doc->pagenum < doc->pagecount - 1)
	{
		doc->pagenum++;
		doc->update(doc);
	}
}

static void cmd_prev_page (struct document *doc)
{
	if (doc->pagenum > 0)
	{
		doc->pagenum--;
		doc->update(doc);
	}
}

static void cmd_down (struct document *doc)
{
	doc->fbcanvas->scroll(doc, 0, doc->height / 20);
}

static void cmd_up (struct document *doc)
{
	doc->fbcanvas->scroll(doc, 0, -(doc->height / 20));
}

static void cmd_left (struct document *doc)
{
	doc->fbcanvas->scroll(doc, -(doc->width / 20), 0);
}

static void cmd_right (struct document *doc)
{
	doc->fbcanvas->scroll(doc, doc->width / 20, 0);
}

static void cmd_set_zoom (struct document *doc)
{
	double scale = 1.0 + 0.1 * (this_command - '0');
	doc->scale = scale;
	doc->update(doc);
}

static void cmd_zoom_in (struct document *doc)
{
	doc->scale += 0.1;
	doc->update(doc);
}

static void cmd_zoom_out (struct document *doc)
{
	if (doc->scale >= 0.2)
	{
		doc->scale -= 0.1;
		doc->update(doc);
	}
}

static void cmd_save (struct document *doc)
{
	GError *err = NULL;
	char savename[256];

	sprintf(savename, "%s-pg-%d.png", basename(doc->filename), doc->pagenum + 1);
	if (!gdk_pixbuf_save(doc->gdkpixbuf, savename, "png", &err, NULL))
		fprintf (stderr, "%s", err->message);
}

static void cmd_dump_text (struct document *doc)
{
	char filename[256];
	sprintf(filename, "%s-pg-%d.txt", basename(doc->filename), doc->pagenum + 1);
	doc->dump_text(doc, filename);
}

static void transform_doc (struct document *doc,
			   int width, int height,
			   double x1, double y1,
			   double x2, double y2,
			   double dx, double dy)
{
	cairo_surface_t *surf = cairo_surface_create_similar (
		doc->cairo, CAIRO_CONTENT_COLOR_ALPHA, width, height);
	cairo_t *cairo = cairo_create (surf);
	cairo_pattern_t *pat = cairo_pattern_create_for_surface (doc->cairo);
	cairo_matrix_t mat;

	cairo_matrix_init (&mat, x1, y1, x2, y2, dx, dy);
	cairo_transform (cairo, &mat);
	cairo_set_source (cairo, pat);
	cairo_paint (cairo);
	cairo_pattern_destroy (pat);
	cairo_destroy (cairo);

	cairo_surface_destroy (doc->cairo);
	doc->cairo = surf;
	doc->width = width;
	doc->height = height;
}

static void cmd_flip_x (struct document *doc)
{
	if (doc->gdkpixbuf)
	{
		GdkPixbuf *tmp = gdk_pixbuf_flip(doc->gdkpixbuf, TRUE);
		g_object_unref(doc->gdkpixbuf);
		doc->gdkpixbuf = tmp;
	} else if (doc->cairo) {
		transform_doc (doc, doc->width, doc->height,
			       -1, 0, 0, 1, doc->width, 0);
	}
}

static void cmd_flip_y (struct document *doc)
{
	if (doc->gdkpixbuf)
	{
		GdkPixbuf *tmp = gdk_pixbuf_flip(doc->gdkpixbuf, FALSE);
		g_object_unref(doc->gdkpixbuf);
		doc->gdkpixbuf = tmp;
	} else if (doc->cairo) {
		transform_doc (doc, doc->width, doc->height,
			       1, 0, 0, -1, 0, doc->height);
	}
}

static void cmd_flip_z (struct document *doc)
{
	int dir = (this_command == 'z' ? 1 : -1);

	if (doc->gdkpixbuf)
	{
		GdkPixbuf *tmp = gdk_pixbuf_rotate_simple(doc->gdkpixbuf, dir = 1?90:270);
		g_object_unref(doc->gdkpixbuf);
		doc->gdkpixbuf = tmp;
		doc->width = gdk_pixbuf_get_width(doc->gdkpixbuf);
		doc->height = gdk_pixbuf_get_height(doc->gdkpixbuf);
		doc->fbcanvas->scroll(doc, 0, 0); /* Update offsets */
	} else if (doc->cairo) {
		if (dir == 1)
		{
			transform_doc (doc, doc->height, doc->width,
				       0, 1, -1, 0, doc->height, 0);
		} else {
			transform_doc (doc, doc->height, doc->width,
				       0, -1, 1, 0, 0, doc->width);
		}
	}
}

static void cmd_goto_top (struct document *doc)
{
	static int last_y;
	if (last_command == this_command)
	{
		int tmp = doc->yoffset;
		doc->yoffset = last_y;
		last_y = tmp;
	} else {
		last_y = doc->yoffset;
		doc->yoffset = 0;
	}
}

static void cmd_goto_bottom (struct document *doc)
{
	struct framebuffer *fb = doc->fbcanvas->fb;
	static int last_y;
	if (last_command == this_command)
	{
		int tmp = doc->yoffset;
		doc->yoffset = last_y;
		last_y = tmp;
	} else {
		last_y = doc->yoffset;
		doc->yoffset = doc->height - fb->height;
	}
}

static void cmd_message (struct document *doc)
{
	cairo_t *cr = cairo_create(doc->cairo);
	if (cr)
	{
		cairo_select_font_face(cr, "monospace",
			CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
		cairo_set_font_size(cr, 14);

		cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
		cairo_move_to (cr, 10, 14);

		cairo_show_text (cr, "Unregistered fb");
		cairo_destroy (cr);
	}
}

void setup_keys (void)
{
#define SET(key,command) set_key (key, (void *) cmd_ ##command)

	SET (12, redraw); /* CTRL-L */
	SET (27, quit);	 /* ESC */
	SET ('q', quit); SET ('s', save); SET ('t', dump_text); SET ('x', flip_x);
	SET ('y', flip_y); SET ('z', flip_z); SET ('Z', flip_z);
	SET (KEY_HOME, goto_top); SET (KEY_END, goto_bottom);
	SET (KEY_NPAGE, next_page); SET (KEY_PPAGE, prev_page);
	SET (KEY_DOWN, down); SET (KEY_UP, up);
	SET (KEY_LEFT, left); SET (KEY_RIGHT, right);
	SET ('0', set_zoom); SET ('1', set_zoom); SET ('2', set_zoom);
	SET ('3', set_zoom); SET ('4', set_zoom); SET ('5', set_zoom);
	SET ('6', set_zoom); SET ('7', set_zoom); SET ('8', set_zoom);
	SET ('9', set_zoom); SET ('+', zoom_in); SET ('-', zoom_out);
	SET ('m', message);

#undef SET
}

command_t lookup_command (int character)
{
	void *cmd = lookup_key (character);
	if (! cmd)
		cmd = cmd_unbound;

	last_command = this_command;
	this_command = character;

	return (command_t) cmd;
}
