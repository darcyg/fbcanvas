/* commands.c - 13.6.2008 - 14.6.2008 Ari & Tero Roponen */
#include <ncurses.h>
#undef scroll
#include "commands.h"
#include "fbcanvas.h"
#include "keymap.h"

jmp_buf exit_loop;
int this_command;
int last_command;

static void cmd_unbound (struct fbcanvas *fbc)
{
	printf ("\a"); /* bell */
	fflush (stdout);
}

static void cmd_quit (struct fbcanvas *fbc)
{
	longjmp (exit_loop, 1);
}

static void cmd_redraw (struct fbcanvas *fbc)
{
	/* Nothing to do */
}

static void cmd_next_page (struct fbcanvas *fbc)
{
	if (fbc->pagenum < fbc->pagecount - 1)
	{
		fbc->pagenum++;
		fbc->ops->update(fbc);
	}
}

static void cmd_prev_page (struct fbcanvas *fbc)
{
	if (fbc->pagenum > 0)
	{
		fbc->pagenum--;
		fbc->ops->update(fbc);
	}
}

static void cmd_down (struct fbcanvas *fbc)
{
	fbc->scroll(fbc, 0, fbc->height / 20);
}

static void cmd_up (struct fbcanvas *fbc)
{
	fbc->scroll(fbc, 0, -(fbc->height / 20));
}

static void cmd_left (struct fbcanvas *fbc)
{
	fbc->scroll(fbc, -(fbc->width / 20), 0);
}

static void cmd_right (struct fbcanvas *fbc)
{
	fbc->scroll(fbc, fbc->width / 20, 0);
}

static void cmd_set_zoom (struct fbcanvas *fbc)
{
	double scale = 1.0 + 0.1 * (this_command - '0');
	fbc->scale = scale;
	fbc->ops->update (fbc);
}

static void cmd_zoom_in (struct fbcanvas *fbc)
{
	fbc->scale += 0.1;
	fbc->ops->update(fbc);
}

static void cmd_zoom_out (struct fbcanvas *fbc)
{
	if (fbc->scale >= 0.2)
	{
		fbc->scale -= 0.1;
		fbc->ops->update(fbc);
	}
}

static void cmd_save (struct fbcanvas *fbc)
{
	GError *err = NULL;
	char savename[256];

	sprintf(savename, "%s-pg-%d.png", basename(fbc->filename), fbc->pagenum + 1);
	if (!gdk_pixbuf_save(fbc->gdkpixbuf, savename, "png", &err, NULL))
		fprintf (stderr, "%s", err->message);
}

static void cmd_dump_text (struct fbcanvas *fbc)
{
	PopplerRectangle rec = {0, 0, fbc->width, fbc->height};
	char *str;

	/* fbc->page is currently only used with PDF-files. */
	if (!fbc->page)
		return;

	str = poppler_page_get_text(fbc->page, POPPLER_SELECTION_LINE, &rec);
	if (str)
	{
		FILE *fp;
		char savename[256];
		sprintf(savename, "%s-pg-%d.txt", basename(fbc->filename), fbc->pagenum + 1);
		fp = fopen(savename, "w+");
		if (fp)
		{
			fprintf(fp, "%s", str);
			fclose (fp);
		}
	}
}

static void cmd_flip_x (struct fbcanvas *fbc)
{
	GdkPixbuf *tmp = gdk_pixbuf_flip(fbc->gdkpixbuf, TRUE);
	g_object_unref(fbc->gdkpixbuf);
	fbc->gdkpixbuf = tmp;
}

static void cmd_flip_y (struct fbcanvas *fbc)
{
	GdkPixbuf *tmp = gdk_pixbuf_flip(fbc->gdkpixbuf, FALSE);
	g_object_unref(fbc->gdkpixbuf);
	fbc->gdkpixbuf = tmp;
}

static void cmd_flip_z (struct fbcanvas *fbc)
{
	int angle = (this_command == 'z' ? 90 : 270);
	GdkPixbuf *tmp = gdk_pixbuf_rotate_simple(fbc->gdkpixbuf, angle);
	g_object_unref(fbc->gdkpixbuf);
	fbc->gdkpixbuf = tmp;
	fbc->width = gdk_pixbuf_get_width(fbc->gdkpixbuf);
	fbc->height = gdk_pixbuf_get_height(fbc->gdkpixbuf);
	fbc->scroll(fbc, 0, 0); /* Update offsets */
}

static void cmd_goto_top (struct fbcanvas *fbc)
{
	static int last_y;
	if (last_command == this_command)
	{
		int tmp = fbc->yoffset;
		fbc->yoffset = last_y;
		last_y = tmp;
	} else {
		last_y = fbc->yoffset;
		fbc->yoffset = 0;
	}
}

static void cmd_goto_bottom (struct fbcanvas *fbc)
{
	struct framebuffer *fb = fbc->fb;
	static int last_y;
	if (last_command == this_command)
	{
		int tmp = fbc->yoffset;
		fbc->yoffset = last_y;
		last_y = tmp;
	} else {
		last_y = fbc->yoffset;
		fbc->yoffset = fbc->height - fb->height;
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
