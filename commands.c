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
	cairo_matrix_t flipx;
	cairo_matrix_init (&flipx, -1, 0, 0, 1, doc->width, 0);

	/*
	 *  y        y
	 *  │   ->   │
	 * .└─x    x─┘.
	 */
	cairo_matrix_multiply (&doc->transform, &doc->transform, &flipx);
}

static void cmd_flip_y (struct document *doc)
{
	cairo_matrix_t flipy;
	cairo_matrix_init (&flipy, 1, 0, 0, -1, 0, doc->height);

	/*
	 *  y      .┌─x
	 *  │   ->  │
	 * .└─x     y
	 */
	cairo_matrix_multiply (&doc->transform, &doc->transform, &flipy);
}

static void cmd_flip_z (struct document *doc)
{
	int dir = (this_command == 'Z') ? 1 : -1;
	cairo_matrix_t flipz;
	cairo_matrix_init (&flipz, 0, dir, -dir, 0,
			   (dir == 1) ? doc->height : 0,
			   (dir == -1) ? doc->width : 0);

	/*
	 *   dir = 1          dir = -1
	 *
	 *  y        x	    y      .┌─y
	 *  │   ->   │	    │   ->  │
	 * .└─x    y─┘.	   .└─x     x
	 */
	cairo_matrix_multiply (&doc->transform, &doc->transform, &flipz);

	int tmp = doc->height;
	doc->height = doc->width;
	doc->width = tmp;
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
	cairo_matrix_t full;

	cairo_matrix_init (&full,
			   w / (double) doc->width, 0.0,
			   0.0, h / (double) doc->height,
			   0.0, 0.0);
	cairo_matrix_multiply (&doc->transform, &doc->transform, &full);
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
