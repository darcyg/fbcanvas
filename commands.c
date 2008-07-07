/* commands.c - 13.6.2008 - 7.7.2008 Ari & Tero Roponen */
#include <cairo/cairo.h>
#include <math.h>
#include <ncurses.h>
#include <string.h>
#undef scroll
#include "commands.h"
#include "document.h"
#include "keymap.h"

extern void execute_extended_command (struct document *doc, char *cmd);

jmp_buf exit_loop;
static int this_command;
static int last_command;
static int in_command_mode;

static fb_keymap_t *cmd_read_keymap;
static char cmdbuf[128];	/* XXX */
static int cmdpos;

static void cmd_unbound (struct document *doc)
{
	printf ("\a"); /* bell */
	fflush (stdout);

	if (in_command_mode)
		doc->set_message (doc, "C:\\> %s_", cmdbuf);
}

static void cmd_quit (struct document *doc)
{
	longjmp (exit_loop, 1);
}

static void cmd_redraw (struct document *doc)
{
	/* Nothing to do */
}

static void cmd_first_page (struct document *doc)
{
	if (doc->pagenum > 0)
	{
		doc->pagenum = 0;
		doc->update(doc);
	}
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

static void cmd_last_page (struct document *doc)
{
	if (doc->pagenum < doc->pagecount - 1)
	{
		doc->pagenum = doc->pagecount - 1;
		doc->update(doc);
	}
}

static void cmd_down (struct document *doc)
{
	doc->scroll(doc, 0, doc->backend->height / 20);
}

static void cmd_up (struct document *doc)
{
	doc->scroll(doc, 0, -(doc->backend->height / 20));
}

static void cmd_left (struct document *doc)
{
	doc->scroll(doc, -(doc->backend->width / 20), 0);
}

static void cmd_right (struct document *doc)
{
	doc->scroll(doc, doc->backend->width / 20, 0);
}

static void cmd_set_zoom (struct document *doc)
{
	double scale = 1.0 + 0.1 * (this_command - '0');
	doc->scale = scale;

	if (doc->flags & NO_GENERIC_SCALE)
		doc->update(doc);
}

static void cmd_zoom_in (struct document *doc)
{
	doc->scale += 0.1;

	if (doc->flags & NO_GENERIC_SCALE)
		doc->update(doc);
}

static void cmd_zoom_out (struct document *doc)
{
	if (doc->scale >= 0.2)
	{
		doc->scale -= 0.1;

		if (doc->flags & NO_GENERIC_SCALE)
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
	int width = (doc->flags & NO_GENERIC_SCALE) ?
		doc->width : ceil(doc->scale * doc->width);
	cairo_matrix_t flipx;
	cairo_matrix_init (&flipx, -1, 0, 0, 1, width, 0);

	/*
	 * .y        y.
	 *  │   ->   │
	 *  └─x    x─┘
	 */
	cairo_matrix_multiply (&doc->transform, &doc->transform, &flipx);
}

static void cmd_flip_y (struct document *doc)
{
	int height = (doc->flags & NO_GENERIC_SCALE) ?
		doc->height : ceil(doc->scale * doc->height);
	cairo_matrix_t flipy;
	cairo_matrix_init (&flipy, 1, 0, 0, -1, 0, height);

	/* .y      .
	 *  │  
	 *  └─x ->
	 *          ┌─x
	 *          │
	 *          y
	 */
	cairo_matrix_multiply (&doc->transform, &doc->transform, &flipy);
}

static void cmd_flip_z (struct document *doc)
{
	int width = (doc->flags & NO_GENERIC_SCALE) ?
		doc->width : ceil(doc->scale * doc->width);
	int height = (doc->flags & NO_GENERIC_SCALE) ?
		doc->height : ceil(doc->scale * doc->height);

	int dir = (this_command == ('z' | SHIFT)) ? 1 : -1;
	cairo_matrix_t flipz;
	cairo_matrix_init (&flipz, 0, dir, -dir, 0,
			   (dir == 1) ? height : 0,
			   (dir == -1) ? width : 0);

	/* TODO: fix images
	 *   dir = 1          dir = -1
	 *
	 * .y         x	   .y      .┌─y
	 *  │   ->    │	    │   ->  │
	 *  └─x    .y─┘	    └─x     x
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
	static int last_y;
	if (last_command == this_command)
	{
		int tmp = doc->yoffset;
		doc->yoffset = last_y;
		last_y = tmp;
	} else {
		last_y = doc->yoffset;
		doc->yoffset = doc->height - doc->backend->height;
	}
}

static void cmd_display_current_page (struct document *doc)
{
	if (last_command == this_command)
	{
		this_command = 'l' | CONTROL;
		return;
	}

	doc->set_message(doc, "%d/%d", doc->pagenum + 1, doc->pagecount);
}

static void cmd_display_info(struct document *doc)
{
	if (last_command == this_command)
	{
		this_command = 'l' | CONTROL;
		return;
	}

	doc->set_message (doc, "File: %s\nPage: %d/%d",
			  basename(doc->filename),
			  doc->pagenum + 1,
			  doc->pagecount);
}

static void scale_doc_full (struct document *doc, double xs, double ys)
{
	double w = doc->backend->width;
	double h = doc->backend->height;
	cairo_matrix_t scale;

	cairo_matrix_init (&scale, w / xs, 0.0, 0.0, h / ys, 0.0, 0.0);
	cairo_matrix_multiply (&doc->transform, &doc->transform, &scale);
}

static void cmd_reset (struct document *doc)
{
	doc->scale = 1.0;
	doc->xoffset = 0;
	doc->yoffset = 0;
	cairo_matrix_init_identity (&doc->transform);

	/* Update is needed to reset to the original image. */
	if (doc->flags & NO_GENERIC_SCALE)
		doc->update(doc);
}

static void cmd_full_screen (struct document *doc)
{
	cmd_reset (doc);
	scale_doc_full (doc, doc->width, doc->height);
}

static void cmd_full_width (struct document *doc)
{
	cmd_reset (doc);

	if (doc->flags & NO_GENERIC_SCALE)
	{
		doc->scale = (double)doc->backend->width / (double)doc->width;
		doc->update(doc);
	} else {
		scale_doc_full (doc, doc->width, doc->width);
	}
}

static void cmd_full_height (struct document *doc)
{
	cmd_reset (doc);

	if (doc->flags & NO_GENERIC_SCALE)
	{
		doc->scale = (double)doc->backend->height / (double)doc->height;
		doc->update(doc);
	} else {
		scale_doc_full (doc, doc->height, doc->height);
	}
}

static void cmd_read_finish (struct document *doc)
{
	cmdbuf[cmdpos] = '\0';

	/* Back to normal mode. */
	use_keymap (NULL);
	in_command_mode = 0;

	execute_extended_command (doc, cmdbuf);
}

static void cmd_read_insert (struct document *doc)
{
	cmdbuf[cmdpos++] = this_command;
	cmdbuf[cmdpos] = '\0';
	doc->set_message (doc, "C:\\> %s_", cmdbuf);
}

static void cmd_read_backspace (struct document *doc)
{
	if (cmdpos > 0)
		cmdbuf[--cmdpos] = '\0';
	doc->set_message (doc, "C:\\> %s_", cmdbuf);
}

static void cmd_read_quit (struct document *doc)
{
	use_keymap (NULL);
	in_command_mode = 0;
}

static void cmd_read_mode (struct document *doc)
{
	if (! cmd_read_keymap)
	{
		cmd_read_keymap = create_keymap ();
		use_keymap (cmd_read_keymap);
		for (int ch = 'a'; ch <= 'z'; ch++)
			set_key (ch, cmd_read_insert);
		for (int ch = '0'; ch <= '9'; ch++)
			set_key (ch, cmd_read_insert);
		set_key (' ', cmd_read_insert);
		set_key (106, cmd_read_finish); /* RET */
		set_key (263, cmd_read_backspace); /* Backspace */
		set_key (27, cmd_read_quit);	   /* ESC */
		use_keymap (NULL);
	}

	cmdbuf[cmdpos = 0] = '\0';

	use_keymap (cmd_read_keymap);
	in_command_mode = 1;
	doc->set_message (doc, "C:\\> %s_", cmdbuf);
}

void setup_keys (void)
{
	struct
	{
		unsigned int code;
		void (*cmd)(struct document *doc);
	} keys[] = {
		{27, cmd_quit},
		{'f', cmd_full_screen},
		{'g', cmd_reset},
		{'h', cmd_full_height},
		{'i', cmd_display_info},
		{'l' | CONTROL, cmd_redraw},
		{'n' | CONTROL, cmd_down},
		{'p', cmd_display_current_page},
		{'p' | CONTROL, cmd_up},
		{'q', cmd_quit},
		{106, cmd_read_mode}, /* RET */
		{'s', cmd_save},
		{'t', cmd_dump_text},
		{'w', cmd_full_width},
		{'x', cmd_flip_x},
		{'y', cmd_flip_y},
		{'z', cmd_flip_z},
		{'z' | SHIFT, cmd_flip_z},

		{KEY_HOME, cmd_goto_top},
		{KEY_HOME | CONTROL, cmd_first_page},
		{KEY_END, cmd_goto_bottom},
		{KEY_END | CONTROL, cmd_last_page},
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

	use_keymap (NULL);	/* Global keymap */
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
