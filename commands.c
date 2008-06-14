/* commands.c - 13.6.2008 - 13.6.2008 Ari & Tero Roponen */
#include <ncurses.h>
#undef scroll
#include "commands.h"
#include "document.h"
#include "keymap.h"

jmp_buf exit_loop;

#define DEFUN(name) static void cmd_ ##name (struct document *doc, int command, int last)

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
	if (doc->pagenum < doc->pagecount - 1)
	{
		doc->pagenum++;
		doc->ops->update(doc);
	}
}

DEFUN (prev_page)
{
	if (doc->pagenum > 0)
	{
		doc->pagenum--;
		doc->ops->update(doc);
	}
}

DEFUN (down)
{
	doc->fbcanvas->scroll(doc, 0, doc->height / 20);
}

DEFUN (up)
{
	doc->fbcanvas->scroll(doc, 0, -(doc->height / 20));
}

DEFUN (left)
{
	doc->fbcanvas->scroll(doc, -(doc->width / 20), 0);
}

DEFUN (right)
{
	doc->fbcanvas->scroll(doc, doc->width / 20, 0);
}

DEFUN (set_zoom)
{
	double scale = 1.0 + 0.1 * (command - '0');
	doc->scale = scale;
	doc->ops->update(doc);
}

DEFUN (zoom_in)
{
	doc->scale += 0.1;
	doc->ops->update(doc);
}

DEFUN (zoom_out)
{
	if (doc->scale >= 0.2)
	{
		doc->scale -= 0.1;
		doc->ops->update(doc);
	}
}

DEFUN (save)
{
	GError *err = NULL;
	char savename[256];

	sprintf(savename, "%s-pg-%d.png", basename(doc->filename), doc->pagenum + 1);
	if (!gdk_pixbuf_save(doc->gdkpixbuf, savename, "png", &err, NULL))
		fprintf (stderr, "%s", err->message);
}

DEFUN (dump_text)
{
	PopplerRectangle rec = {0, 0, doc->width, doc->height};
	char *str;

	/* fbc->page is currently only used with PDF-files. */
	if (!doc->page)
		return;

	str = poppler_page_get_text(doc->page, POPPLER_SELECTION_LINE, &rec);
	if (str)
	{
		FILE *fp;
		char savename[256];
		sprintf(savename, "%s-pg-%d.txt", basename(doc->filename), doc->pagenum + 1);
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
	GdkPixbuf *tmp = gdk_pixbuf_flip(doc->gdkpixbuf, TRUE);
	g_object_unref(doc->gdkpixbuf);
	doc->gdkpixbuf = tmp;
}

DEFUN (flip_y)
{
	GdkPixbuf *tmp = gdk_pixbuf_flip(doc->gdkpixbuf, FALSE);
	g_object_unref(doc->gdkpixbuf);
	doc->gdkpixbuf = tmp;
}

DEFUN (flip_z)
{
	int angle = (command == 'z' ? 90 : 270);
	GdkPixbuf *tmp = gdk_pixbuf_rotate_simple(doc->gdkpixbuf, angle);
	g_object_unref(doc->gdkpixbuf);
	doc->gdkpixbuf = tmp;
	doc->width = gdk_pixbuf_get_width(doc->gdkpixbuf);
	doc->height = gdk_pixbuf_get_height(doc->gdkpixbuf);
	doc->fbcanvas->scroll(doc, 0, 0); /* Update offsets */
}

DEFUN (goto_top)
{
	static int last_y;
	if (last == command)
	{
		int tmp = doc->yoffset;
		doc->yoffset = last_y;
		last_y = tmp;
	} else {
		last_y = doc->yoffset;
		doc->yoffset = 0;
	}
}

DEFUN (goto_bottom)
{
	struct framebuffer *fb = doc->fbcanvas->fb;
	static int last_y;
	if (last == command)
	{
		int tmp = doc->yoffset;
		doc->yoffset = last_y;
		last_y = tmp;
	} else {
		last_y = doc->yoffset;
		doc->yoffset = doc->height - fb->height;
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
	return (command_t) cmd;
}
