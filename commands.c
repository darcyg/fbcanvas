/* commands.c - 13.6.2008 - 14.7.2008 Ari & Tero Roponen */
#include <linux/input.h>
#include <cairo/cairo.h>
#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "commands.h"
#include "document.h"
#include "extcmd.h"
#include "keymap.h"
#include "util.h"

extern char *fb_read_line (struct document *doc, char *prompt); /* In readline.c */

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
	int delta = doc->backend->height / 20;
	doc->scroll(doc, 0, delta);
}

static void cmd_up (struct document *doc)
{
	int delta = -(doc->backend->height / 20);
	doc->scroll(doc, 0, delta);
}

static void cmd_left (struct document *doc)
{
	int delta = -(doc->backend->width / 20);
	doc->scroll(doc, delta, 0);
}

static void cmd_right (struct document *doc)
{
	int delta = doc->backend->width / 20;
	doc->scroll(doc, delta, 0);
}

static void cmd_set_zoom (struct document *doc)
{
	double scale = 1.0;
	if (this_command != KEY_0)
		scale += 0.1 * (this_command - KEY_1 + 1);
	execute_extended_command (doc, "scale %2f", scale);
}

static void cmd_zoom_in (struct document *doc)
{
	execute_extended_command (doc, "scale more");
}

static void cmd_zoom_out (struct document *doc)
{
	execute_extended_command (doc, "scale less");
}

static void ecmd_save (struct document *doc, int argc, char *argv[])
{
	if (argc != 2)
	{
		doc->set_message (doc, "Usage: save file.png");
		return;
	}

	char *ext = strrchr (argv[1], '.');
	if (! ext || strcmp (ext, ".png"))
	{
		doc->set_message (doc, "File name must end with \".png\".");
		return;
	}
	
	convert_surface_argb_to_abgr (doc->cairo);
	int ret = cairo_surface_write_to_png(doc->cairo, argv[1]);
	if (ret)
		fprintf (stderr, "%s", cairo_status_to_string(ret));
	convert_surface_argb_to_abgr (doc->cairo);
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

	int dir = (this_command == (KEY_Z | SHIFT)) ? 1 : -1;
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
		this_command = KEY_L | CONTROL;
		return;
	}

	doc->set_message(doc, "%d/%d", doc->pagenum + 1, doc->pagecount);
}

static void cmd_display_info(struct document *doc)
{
	if (last_command == this_command)
	{
		this_command = KEY_L | CONTROL;
		return;
	}

	doc->set_message (doc, "File: %s\nPage: %d/%d",
			  basename(doc->filename),
			  doc->pagenum + 1,
			  doc->pagecount);
}

static void cmd_reset (struct document *doc)
{
	execute_extended_command (doc, "scale reset");
}

static void cmd_full_screen (struct document *doc)
{
	execute_extended_command (doc, "scale full");
}

static void cmd_full_width (struct document *doc)
{
	execute_extended_command (doc, "scale width");
}

static void cmd_full_height (struct document *doc)
{
	execute_extended_command (doc, "scale height");
}

void cmd_extended_command (struct document *doc)
{
	char *prompt = "C:\\> ";
	char *cmd = fb_read_line (doc, prompt);
	if (cmd)
		execute_extended_command (doc, cmd);
	free (cmd);
}

void setup_keys (void)
{
	struct
	{
		unsigned int code;
		void (*cmd)(struct document *doc);
	} keys[] = {
		{KEY_ESC, cmd_quit},
		{KEY_F, cmd_full_screen},
		{KEY_G, cmd_reset},
		{KEY_H, cmd_full_height},
		{KEY_I, cmd_display_info},
		{KEY_L | CONTROL, cmd_redraw},
		{KEY_N | CONTROL, cmd_down},
		{KEY_P, cmd_display_current_page},
		{KEY_P | CONTROL, cmd_up},
		{KEY_Q, cmd_quit},
		{KEY_ENTER, cmd_extended_command}, /* RET */
		{KEY_T, cmd_dump_text},
		{KEY_W, cmd_full_width},
		{KEY_X, cmd_flip_x},
		{KEY_Y, cmd_flip_y},
		{KEY_Z, cmd_flip_z},
		{KEY_Z | SHIFT, cmd_flip_z},

		{KEY_HOME, cmd_goto_top},
		{KEY_HOME | CONTROL, cmd_first_page},
		{KEY_END, cmd_goto_bottom},
		{KEY_END | CONTROL, cmd_last_page},
		{KEY_PAGEDOWN, cmd_next_page},
		{KEY_PAGEUP, cmd_prev_page},
		{KEY_DOWN, cmd_down},
		{KEY_UP, cmd_up},
		{KEY_LEFT, cmd_left},
		{KEY_RIGHT, cmd_right},

		{KEY_0, cmd_set_zoom},
		{KEY_1, cmd_set_zoom},
		{KEY_2, cmd_set_zoom},
		{KEY_3, cmd_set_zoom},
		{KEY_4, cmd_set_zoom},
		{KEY_5, cmd_set_zoom},
		{KEY_6, cmd_set_zoom},
		{KEY_7, cmd_set_zoom},
		{KEY_8, cmd_set_zoom},
		{KEY_9, cmd_set_zoom},
		{KEY_MINUS, cmd_zoom_in}, /* +/- tulevat englanninkielisen    */
		{KEY_SLASH, cmd_zoom_out}, /* näppäismistöasettelun mukaisesti */

		{0, NULL}
	};

	use_keymap (NULL);	/* Global keymap */
	for (int i = 0; keys[i].cmd; i++)
		set_key(keys[i].code, keys[i].cmd);

	register_extended_commands ();

	set_extcmd ("save", ecmd_save);
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
