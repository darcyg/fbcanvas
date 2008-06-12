/*
 * main.c - 17.5.2008 - 12.6.2008 Ari & Tero Roponen
 */

#include <argp.h>
#include <fcntl.h>
#include <magic.h>
#include <ncurses.h>
#undef scroll
#include <linux/vt.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fbcanvas.h"

struct prefs
{
	int page;
	signed int x;
	signed int y;
	double scale;

	int just_pagecount;
	char *grep_str;
};

/* Can't be static. */
const char *argp_program_version = "fb version 20080612";

static struct argp_option options[] = {
	{"count", 'c', NULL, 0, "display page count"},
	{"grep", 'g', "TEXT", 0, "search for text"},
	{"page", 'p', "PAGE", 0, "goto given page"},
	{"scale", 's', "SCALE", 0, "set scale factor"},
	{NULL, 'x', "X", 0, "set x-offset"},
	{NULL, 'y', "Y", 0, "set y-offset"},
	{NULL}
};

error_t parse_arguments (int key, char *arg, struct argp_state *state)
{
	struct prefs *prefs = state->input;

	switch (key)
	{
	default:
		return ARGP_ERR_UNKNOWN;
	case 'c':
		prefs->just_pagecount = 1;
		break;
	case 'g':
		prefs->grep_str = strdup (arg);
		break;
	case 'p':
		prefs->page = atoi (arg) - 1;
		break;
	case 's':
		prefs->scale = strtod (arg, NULL);
//		fprintf (stderr, "%f\n", prefs->scale);
		break;
	case 'x':
		prefs->x = atoi (arg);
		break;
	case 'y':
		prefs->y = atoi (arg);
		break;
	case ARGP_KEY_END:
		if (state->arg_num != 1)
			argp_usage (state);
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
		fbc->ops->update(fbc);
	}
	return 0;
}

DEFUN (prev_page)
{
	if (fbc->pagenum > 0)
	{
		fbc->pagenum--;
		fbc->ops->update(fbc);
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
	fbc->ops->update (fbc);
	return 0;
}

DEFUN (zoom_in)
{
	fbc->scale += 0.1;
	fbc->ops->update(fbc);
	return 0;
}

DEFUN (zoom_out)
{
	if (fbc->scale >= 0.2)
	{
		fbc->scale -= 0.1;
		fbc->ops->update(fbc);
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
		fbc->yoffset = fbc->height - fbc->hwheight;
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

static struct fbcanvas *ugly_hack;

static void handle_signal(int s)
{
	int fd = open("/dev/tty", O_RDWR);
	if (fd)
	{
		if (s == SIGUSR1) /* */
		{
			/* Release display */
			ioctl(fd, VT_RELDISP, 1);
		} else if (s == SIGUSR2) {
			/* Acquire display */
			ioctl(fd, VT_RELDISP, VT_ACKACQ);
			// ungetch(12); /* Ctrl-L */
			ugly_hack->draw(ugly_hack);
		}

		close(fd);
	}
}

static void main_loop (struct fbcanvas *fbc)
{
	int ch, last = 0;
	command_t cmd;
	WINDOW *win = initscr();

	signal(SIGUSR1, handle_signal);
	signal(SIGUSR2, handle_signal);

	refresh();
	noecho();
	cbreak();
	keypad(win, 1); /* Handle KEY_xxx */

	ugly_hack = fbc;

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

static int count_pages (char *filename)
{
	struct fbcanvas *fbc = fbcanvas_create (filename);
	fprintf (stderr, "%s has %d page%s.\n",
		 fbc->filename, fbc->pagecount,
		 fbc->pagecount > 1 ? "s":"");
	fbcanvas_destroy (fbc);
	return 0;
}

static int grep_text (char *filename, char *text)
{
	struct fbcanvas *fbc = fbcanvas_create (filename);
	int ret;

	if (fbc->ops->grep)
	{
		ret = fbc->ops->grep (fbc, text);
	} else {
		fprintf (stderr, "%s",
			 "Sorry, grepping is not implemented for this file type.\n");
		ret = 1;
	}
	fbcanvas_destroy (fbc);
	return ret;
}

static int view_file (char *file, struct prefs *prefs)
{
	struct fbcanvas *fbc = fbcanvas_create (file);

	if (prefs->page < fbc->pagecount)
		fbc->pagenum = prefs->page;
	fbc->xoffset = prefs->x;
	fbc->yoffset = prefs->y;
	fbc->scale = prefs->scale;

	fbc->ops->update (fbc);

	main_loop (fbc);

	fprintf (stderr, "%s %s -p%d -s%f -x%d -y%d\n",
		 program_invocation_short_name,
		 file, fbc->pagenum + 1,
		 fbc->scale, fbc->xoffset, fbc->yoffset);
}

int main(int argc, char *argv[])
{
	int ind, fd;
	int ret = 0;
	char filename[256];

	struct prefs prefs = {0, 0, 0, 1.0, };
	struct argp argp = {options, parse_arguments, "FILE", };

	argp_parse (&argp, argc, argv, 0, &ind, &prefs);

	if (prefs.just_pagecount)
		return count_pages (argv[ind]);
	else if (prefs.grep_str)
		return grep_text (argv[ind], prefs.grep_str);

	/* Asetetaan konsolinvaihto lähettämään signaaleja */
	fd = open("/dev/tty", O_RDWR);
	if (fd >= 0)
	{
		struct vt_mode vt_mode;
		ioctl(fd, VT_GETMODE, &vt_mode);
		vt_mode.mode = VT_PROCESS; /* Tämä prosessi hoitaa vaihdot. */
		vt_mode.waitv = 0;
		vt_mode.relsig = SIGUSR1;
		vt_mode.acqsig = SIGUSR2;
		ioctl(fd, VT_SETMODE, &vt_mode);
		close(fd);
	}

	return view_file (argv[ind], &prefs);
}
