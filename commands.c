/* commands.c - 13.6.2008 - 14.6.2008 Ari & Tero Roponen */
#include <ncurses.h>
#undef scroll
#include "commands.h"
#include "fbcanvas.h"
#include "keymap.h"

jmp_buf exit_loop;
int this_command;
int last_command;

#define DEFUN(name) static void cmd_ ##name (struct fbcanvas *fbc)

DEFUN (unbound)
{
	printf ("\a"); /* bell */
	fflush (stdout);
}

DEFUN (quit)
{
	longjmp (exit_loop, 1);
}

DEFUN (redraw)
{
	/* Nothing to do */
}

DEFUN (next_page)
{
	if (fbc->pagenum < fbc->pagecount - 1)
	{
		fbc->pagenum++;
		fbc->ops->update(fbc);
	}
}

DEFUN (prev_page)
{
	if (fbc->pagenum > 0)
	{
		fbc->pagenum--;
		fbc->ops->update(fbc);
	}
}

DEFUN (down)
{
	fbc->scroll(fbc, 0, fbc->height / 20);
}

DEFUN (up)
{
	fbc->scroll(fbc, 0, -(fbc->height / 20));
}

DEFUN (left)
{
	fbc->scroll(fbc, -(fbc->width / 20), 0);
}

DEFUN (right)
{
	fbc->scroll(fbc, fbc->width / 20, 0);
}

DEFUN (set_zoom)
{
	double scale = 1.0 + 0.1 * (this_command - '0');
	fbc->scale = scale;
	fbc->ops->update (fbc);
}

DEFUN (zoom_in)
{
	fbc->scale += 0.1;
	fbc->ops->update(fbc);
}

DEFUN (zoom_out)
{
	if (fbc->scale >= 0.2)
	{
		fbc->scale -= 0.1;
		fbc->ops->update(fbc);
	}
}

DEFUN (save)
{
	GError *err = NULL;
	char savename[256];

	sprintf(savename, "%s-pg-%d.png", basename(fbc->filename), fbc->pagenum + 1);
	if (!gdk_pixbuf_save(fbc->gdkpixbuf, savename, "png", &err, NULL))
		fprintf (stderr, "%s", err->message);
}

DEFUN (dump_text)
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

DEFUN (flip_x)
{
	GdkPixbuf *tmp = gdk_pixbuf_flip(fbc->gdkpixbuf, TRUE);
	g_object_unref(fbc->gdkpixbuf);
	fbc->gdkpixbuf = tmp;
}

DEFUN (flip_y)
{
	GdkPixbuf *tmp = gdk_pixbuf_flip(fbc->gdkpixbuf, FALSE);
	g_object_unref(fbc->gdkpixbuf);
	fbc->gdkpixbuf = tmp;
}

DEFUN (flip_z)
{
	int angle = (this_command == 'z' ? 90 : 270);
	GdkPixbuf *tmp = gdk_pixbuf_rotate_simple(fbc->gdkpixbuf, angle);
	g_object_unref(fbc->gdkpixbuf);
	fbc->gdkpixbuf = tmp;
	fbc->width = gdk_pixbuf_get_width(fbc->gdkpixbuf);
	fbc->height = gdk_pixbuf_get_height(fbc->gdkpixbuf);
	fbc->scroll(fbc, 0, 0); /* Update offsets */
}

DEFUN (goto_top)
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

DEFUN (goto_bottom)
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

#define SET(key,command) set_key (key, (void *) cmd_ ##command)

void setup_keys (void)
{
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
};

command_t lookup_command (int character)
{
	void *cmd = lookup_key (character);
	if (! cmd)
		cmd = cmd_unbound;

	last_command = this_command;
	this_command = character;

	return (command_t) cmd;
}
