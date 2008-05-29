/*
 * main.c - 17.5.2008 - 27.5.2008 Ari & Tero Roponen
 */

#include <magic.h>
#include <ncurses.h>
#undef scroll
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fbcanvas.h"

static struct
{
	int page;
	signed int x;
	signed int y;
	double scale;
} prefs = {0, 0, 0, 1.0};

static int just_pagecount;

static int parse_arguments (int argc, char *argv[])
{
	extern char *optarg;
	char *opts = "cp:s:x:y:";
	int i;
	
	while ((i = getopt (argc, argv, opts)) != -1)
	{
		switch (i)
		{
		default:
			fprintf (stderr, "Invalid option: %c", i);
			return 1;
		case 'c':
			just_pagecount = 1;
			break;
		case 'p':
			prefs.page = atoi (optarg) - 1;
			break;
		case 's':
			prefs.scale = strtod (optarg, NULL);
			fprintf (stderr, "%f\n", prefs.scale);
			break;
		case 'x':
			prefs.x = atoi (optarg);
			break;
		case 'y':
			prefs.y = atoi (optarg);
			break;
		}
	}
	return 0;
}

#define DEFUN(name) static int cmd_ ##name (struct fbcanvas *fbc, int command, int last)
#define SET(key,command) [key] = cmd_ ##command

DEFUN (unbound)
{
	printf ("\a"); /* bell */
	fflush (stdout);
	return 0;
}

DEFUN (quit)
{
	return 1;		/* exit */
}

DEFUN (redraw)
{
	/* Nothing to do */
	return 0;
}

DEFUN (next_page)
{
	if (fbc->pagenum < fbc->pagecount - 1)
	{
		fbc->pagenum++;
		fbc->update(fbc);
	}
	return 0;
}

DEFUN (prev_page)
{
	if (fbc->pagenum > 0)
	{
		fbc->pagenum--;
		fbc->update(fbc);
	}
	return 0;
}

DEFUN (down)
{
	fbc->scroll(fbc, 0, fbc->height / 20);
	return 0;
}

DEFUN (up)
{
	fbc->scroll(fbc, 0, -(fbc->height / 20));
	return 0;
}

DEFUN (left)
{
	fbc->scroll(fbc, -(fbc->width / 20), 0);
	return 0;
}

DEFUN (right)
{
	fbc->scroll(fbc, fbc->width / 20, 0);
	return 0;
}

DEFUN (set_zoom)
{
	double scale = 1.0 + 0.1 * (command - '0');
	fbc->scale = scale;
	fbc->update (fbc);
	return 0;
}

DEFUN (zoom_in)
{
	fbc->scale += 0.1;
	fbc->update(fbc);
	return 0;
}

DEFUN (zoom_out)
{
	if (fbc->scale >= 0.2)
	{
		fbc->scale -= 0.1;
		fbc->update(fbc);
	}
	return 0;
}

DEFUN (save)
{
	GError *err = NULL;
	char savename[256];

	sprintf(savename, "%s-pg-%d.png", basename(fbc->filename), fbc->pagenum + 1);
	if (!gdk_pixbuf_save(fbc->gdkpixbuf, savename, "png", &err, NULL))
		fprintf (stderr, "%s", err->message);
	return 0;
}

DEFUN (dump_text)
{
	PopplerRectangle rec = {0, 0, fbc->width, fbc->height};
	char *str;

	/* fbc->page is currently only used with PDF-files. */
	if (!fbc->page)
		return 0;

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

	return 0;
}

DEFUN (flip_x)
{
	GdkPixbuf *tmp = gdk_pixbuf_flip(fbc->gdkpixbuf, TRUE);
	g_object_unref(fbc->gdkpixbuf);
	fbc->gdkpixbuf = tmp;
	return 0;
}

DEFUN (flip_y)
{
	GdkPixbuf *tmp = gdk_pixbuf_flip(fbc->gdkpixbuf, FALSE);
	g_object_unref(fbc->gdkpixbuf);
	fbc->gdkpixbuf = tmp;
	return 0;
}

DEFUN (flip_z)
{
	int angle = (command == 'z' ? 90 : 270);
	GdkPixbuf *tmp = gdk_pixbuf_rotate_simple(fbc->gdkpixbuf, angle);
	g_object_unref(fbc->gdkpixbuf);
	fbc->gdkpixbuf = tmp;
	fbc->width = gdk_pixbuf_get_width(fbc->gdkpixbuf);
	fbc->height = gdk_pixbuf_get_height(fbc->gdkpixbuf);
	fbc->scroll(fbc, 0, 0); /* Update offsets */
	return 0;
}

DEFUN (goto_top)
{
	static int last_y;
	if (last == command)
	{
		int tmp = fbc->yoffset;
		fbc->yoffset = last_y;
		last_y = tmp;
	} else {
		last_y = fbc->yoffset;
		fbc->yoffset = 0;
	}
	return 0;
}

DEFUN (goto_bottom)
{
	static int last_y;
	if (last == command)
	{
		int tmp = fbc->yoffset;
		fbc->yoffset = last_y;
		last_y = tmp;
	} else {
		last_y = fbc->yoffset;
		// XXX: 600 = framebuffer.height
		fbc->yoffset = fbc->height - 600;
	}
	return 0;
}

typedef int (*command_t) (struct fbcanvas *, int command, int last);
command_t keymap[] = {
	SET (12, redraw), /* CTRL-L */
	SET (27, quit),	 /* ESC */
	SET ('q', quit), SET ('s', save), SET ('t', dump_text), SET ('x', flip_x),
	SET ('y', flip_y), SET ('z', flip_z), SET ('Z', flip_z),
	SET (KEY_HOME, goto_top), SET (KEY_END, goto_bottom),
	SET (KEY_NPAGE, next_page), SET (KEY_PPAGE, prev_page),
	SET (KEY_DOWN, down), SET (KEY_UP, up),
	SET (KEY_LEFT, left), SET (KEY_RIGHT, right),
	SET ('0', set_zoom), SET ('1', set_zoom), SET ('2', set_zoom),
	SET ('3', set_zoom), SET ('4', set_zoom), SET ('5', set_zoom),
	SET ('6', set_zoom), SET ('7', set_zoom), SET ('8', set_zoom),
	SET ('9', set_zoom), SET ('+', zoom_in), SET ('-', zoom_out),
};

static command_t get_command (int ch)
{
	if (ch < sizeof (keymap))
		return keymap[ch] ?: cmd_unbound;
	return cmd_unbound;
}

static void main_loop (struct fbcanvas *fbc)
{
	int ch, last = 0;
	command_t cmd;
	WINDOW *win = initscr();

	refresh();
	noecho();
	cbreak();
	keypad(win, 1); /* Handle KEY_xxx */

	/* Main loop */
	for (;;)
	{
		fbc->draw (fbc);

		ch = getch ();
		cmd = get_command (ch);
		if (cmd (fbc,ch, last))
			break;
		last = ch;
	}

	endwin ();
}

int main(int argc, char *argv[])
{
	extern int optind;

	struct fbcanvas *fbc;
	char filename[256];

	if (parse_arguments (argc, argv) || (optind != argc - 1))
	{
		fprintf (stderr, "Usage: %s [-c] [-pn] [-sn] [-xn] [-yn] file.pdf\n", argv[0]);
		return 1;
	}

	fbc = fbcanvas_create(argv[optind]);
	if (just_pagecount)
	{
		fprintf(stderr, "%s has %d page%s.\n",
			fbc->filename,
			fbc->pagecount,
			fbc->pagecount > 1 ? "s":"");
		goto out_nostatus;
	}

	if (prefs.page < fbc->pagecount)
		fbc->pagenum = prefs.page;
	fbc->xoffset = prefs.x;
	fbc->yoffset = prefs.y;
	fbc->scale = prefs.scale;

	fbc->update(fbc);

	if (! just_pagecount)
		main_loop (fbc);

	fprintf (stderr, "%s %s -p%d -s%f -x%d -y%d\n", argv[0],
		 argv[optind], fbc->pagenum + 1,
		 fbc->scale, fbc->xoffset, fbc->yoffset);
out_nostatus:
	fbcanvas_destroy(fbc);
	return 0;
}
