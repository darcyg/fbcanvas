/* commands.c - 13.6.2008 - 21.6.2008 Ari & Tero Roponen */
#include <cairo/cairo.h>
#include <ncurses.h>
#include <string.h>
#undef scroll
#include "commands.h"
#include "document.h"
#include "keymap.h"

jmp_buf exit_loop;
static int this_command;
static int last_command;

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

/*
 * Update DOC to contain a new surface of size WIDTH * HEIGHT.
 *
 * X1,Y1 is a new value for X-axis
 * X2,Y2 is a new value for Y-axis
 *
 * DX and DY are used to move the image so that the base point is at
 * the bottom left corner.
 *
 * EXAMPLE:
 * Dot (.) marks the base point. It doesn't move.
 * To x-flip image, we change the axis like this:
 *
 *  y        y
 *  │   ->   │
 * .└─x    x─┘.
 *
 * As can be seen, the picture will be drawn to the left from base
 * point, making it invisible. DX = doc->width solves the problem.
 */
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
	int ret;
	char savename[256];

	sprintf(savename, "%s-pg-%d.png", basename(doc->filename), doc->pagenum + 1);
	ret = cairo_surface_write_to_png(doc->cairo, savename);
	if (ret)
		fprintf (stderr, "%s", cairo_status_to_string(ret));
}

static void cmd_dump_text (struct document *doc)
{
	char filename[256];
	sprintf(filename, "%s-pg-%d.txt", basename(doc->filename), doc->pagenum + 1);
	doc->dump_text(doc, filename);
}

static void cmd_flip_x (struct document *doc)
{
	/* See comment in transform_doc.
	 *
	 *  y        y
	 *  │   ->   │
	 * .└─x    x─┘.
	 */
	transform_doc (doc, doc->width, doc->height, -1, 0, 0, 1, doc->width, 0);
}

static void cmd_flip_y (struct document *doc)
{
	/* See comment in transform_doc.
	 *
	 *  y      .┌─x
	 *  │   ->  │
	 * .└─x     y
	 */
	transform_doc (doc, doc->width, doc->height, 1, 0, 0, -1, 0, doc->height);
}

static void cmd_flip_z (struct document *doc)
{
	int dir = (this_command == 'Z') ? 1 : -1;

	/* See comment in transform_doc.
	 *   dir = 1          dir = -1
	 *
	 *  y        x	    y      .┌─y
	 *  │   ->   │	    │   ->  │
	 * .└─x    y─┘.	   .└─x     x
	 */
	transform_doc (doc, doc->height, doc->width,
		       0, dir, -dir, 0,
		       (dir == 1) ? doc->height : 0,
		       (dir == -1) ? doc->width : 0);
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

static void cmd_display_current_page (struct document *doc)
{
	char buf[10];

	if (last_command == this_command)
	{
		this_command = 12; /* CTRL-L */
		return;
	}

	sprintf (buf, "%d/%d", doc->pagenum + 1, doc->pagecount);
	doc->set_message(doc, buf);
}

static void cmd_full_screen (struct document *doc)
{
	double w = doc->fbcanvas->fb->width;
	double h = doc->fbcanvas->fb->height;

	transform_doc (doc, w, h,
		       w / (double) doc->width, 0.0, 0.0,
		       h / (double) doc->height, 0.0, 0.0);
}

void setup_keys (void)
{
	struct
	{
		unsigned int code;
		void (*cmd)(struct document *doc);
	} keys[] = {
		{12, cmd_redraw},
		{27, cmd_quit},
		{'f', cmd_full_screen},
		{'p', cmd_display_current_page},
		{'q', cmd_quit},
		{'s', cmd_save},
		{'t', cmd_dump_text},
		{'x', cmd_flip_x},
		{'y', cmd_flip_y},
		{'z', cmd_flip_z},
		{'Z', cmd_flip_z},

		{KEY_HOME, cmd_goto_top},
		{KEY_END, cmd_goto_bottom},
		{KEY_NPAGE, cmd_next_page},
		{KEY_PPAGE, cmd_prev_page},
		{KEY_DOWN, cmd_down},
		{KEY_UP, cmd_up},
		{KEY_LEFT, cmd_left},
		{KEY_RIGHT, cmd_right},

		{'0', cmd_set_zoom},
		{'1', cmd_set_zoom},
		{'2', cmd_set_zoom},
		{'3', cmd_set_zoom},
		{'4', cmd_set_zoom},
		{'5', cmd_set_zoom},
		{'6', cmd_set_zoom},
		{'7', cmd_set_zoom},
		{'8', cmd_set_zoom},
		{'9', cmd_set_zoom},
		{'+', cmd_zoom_in},
		{'-', cmd_zoom_out},
		{0, NULL}
	};

	for (int i = 0; keys[i].cmd; i++)
		set_key(keys[i].code, keys[i].cmd);
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
