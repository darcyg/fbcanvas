/* commands.c - 13.6.2008 - 8.7.2008 Ari & Tero Roponen */
#include <linux/input.h>
#include <cairo/cairo.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "commands.h"
#include "document.h"
#include "extcmd.h"
#include "keymap.h"

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
	int delta = doc->backend->height / 20;
	if (this_command & SHIFT)
		delta = 1;
	doc->scroll(doc, 0, delta);
}

static void cmd_up (struct document *doc)
{
	int delta = -(doc->backend->height / 20);
	if (this_command & SHIFT)
		delta = -1;
	doc->scroll(doc, 0, delta);
}

static void cmd_left (struct document *doc)
{
	int delta = -(doc->backend->width / 20);
	if (this_command & SHIFT)
		delta = -1;
	doc->scroll(doc, delta, 0);
}

static void cmd_right (struct document *doc)
{
	int delta = doc->backend->width / 20;
	if (this_command & SHIFT)
		delta = 1;
	doc->scroll(doc, delta, 0);
}

static void cmd_set_zoom (struct document *doc)
{
	double scale = 1.0;
	if (this_command != KEY_0)
		scale += 0.1 * (this_command - KEY_1);

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
	
	int ret = cairo_surface_write_to_png(doc->cairo, argv[1]);
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
	reset_transformations (doc);

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
	int ind = this_command & ~SHIFT;
	int key = 0;
	switch (ind)
	{
		case KEY_Q ... KEY_P:
			key = "qwertyuiop"[ind - KEY_Q];
			break;
		case KEY_A ... KEY_L:
			key = "asdfghjkl"[ind - KEY_A];
			break;
		case KEY_Z ... KEY_M:
			key = "zxcvbnm"[ind - KEY_Z];
			break;
		case KEY_1 ... KEY_0:
			key = "1234567890"[ind - KEY_1];
			break;
		case KEY_SPACE:
			key = ' ';
			break;
		case KEY_DOT:
			key = '.';
	}

	if (this_command & SHIFT)
		key = toupper(key);

	cmdbuf[cmdpos++] = key;
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
		for (int ch = KEY_Q; ch <= KEY_P; ch++)
		{
			set_key (ch, cmd_read_insert);
			set_key (ch | SHIFT, cmd_read_insert);
		}
		for (int ch = KEY_A; ch <= KEY_L; ch++)
		{
			set_key (ch, cmd_read_insert);
			set_key (ch | SHIFT, cmd_read_insert);
		}
		for (int ch = KEY_Z; ch <= KEY_DOT; ch++)
		{
			set_key (ch, cmd_read_insert);
			set_key (ch | SHIFT, cmd_read_insert);
		}
		for (int ch = KEY_1; ch <= KEY_0; ch++)
			set_key (ch, cmd_read_insert);
		set_key (KEY_SPACE, cmd_read_insert);
		set_key (KEY_ENTER, cmd_read_finish);
		set_key (KEY_BACKSPACE, cmd_read_backspace);
		set_key (KEY_ESC, cmd_read_quit);
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
		{KEY_ENTER, cmd_read_mode}, /* RET */
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
		{KEY_DOWN | SHIFT, cmd_down},
		{KEY_UP, cmd_up},
		{KEY_UP | SHIFT, cmd_up},
		{KEY_LEFT, cmd_left},
		{KEY_LEFT | SHIFT, cmd_left},
		{KEY_RIGHT, cmd_right},
		{KEY_RIGHT | SHIFT, cmd_right},

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
