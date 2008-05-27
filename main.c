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

/* current command char and previous command char */
static int command, last;
static struct fbcanvas *fbc;
static int last_y;

static void cmd_unbound (void)
{
	printf ("\a"); /* bell */
	fflush (stdout);
}

static void cmd_quit (void)
{
	command = -1;		/* exit */
}

static void cmd_redraw (void)
{
	/* Nothing to do */
}

static void cmd_next_page (void)
{
	if (fbc->pagenum < fbc->pagecount - 1)
	{
		fbc->pagenum++;
		fbc->update(fbc);
	}
}

static void cmd_prev_page (void)
{
	if (fbc->pagenum > 0)
	{
		fbc->pagenum--;
		fbc->update(fbc);
	}
}

static void cmd_down (void)
{
	fbc->scroll(fbc, 0, fbc->height / 20);
}

static void cmd_up (void)
{
	fbc->scroll(fbc, 0, -(fbc->height / 20));
}

static void cmd_left (void)
{
	fbc->scroll(fbc, -(fbc->width / 20), 0);
}

static void cmd_right (void)
{
	fbc->scroll(fbc, fbc->width / 20, 0);
}

static void cmd_set_zoom (void)
{
	double scale = 1.0 + 0.1 * (command - '0');
	fbc->scale = scale;
	fbc->update (fbc);
}

static void cmd_zoom_in (void)
{
	fbc->scale += 0.1;
	fbc->update(fbc);
}

static void cmd_zoom_out (void)
{
	if (fbc->scale >= 0.2)
	{
		fbc->scale -= 0.1;
		fbc->update(fbc);
	}
}

static void cmd_save (void)
{
	GError *err = NULL;
	char savename[256];

	sprintf(savename, "%s-pg-%d.png", basename(fbc->filename), fbc->pagenum + 1);
	if (!gdk_pixbuf_save(fbc->gdkpixbuf, savename, "png", &err, NULL))
		fprintf (stderr, "%s", err->message);
}

static void cmd_flip_x (void)
{
	GdkPixbuf *tmp = gdk_pixbuf_flip(fbc->gdkpixbuf, TRUE);
	g_object_unref(fbc->gdkpixbuf);
	fbc->gdkpixbuf = tmp;
}

static void cmd_flip_y (void)
{
	GdkPixbuf *tmp = gdk_pixbuf_flip(fbc->gdkpixbuf, FALSE);
	g_object_unref(fbc->gdkpixbuf);
	fbc->gdkpixbuf = tmp;
}

static void cmd_flip_z (void)
{
	int angle = (command == 'z' ? 90 : 270);
	GdkPixbuf *tmp = gdk_pixbuf_rotate_simple(fbc->gdkpixbuf, angle);
	g_object_unref(fbc->gdkpixbuf);
	fbc->gdkpixbuf = tmp;
	fbc->width = gdk_pixbuf_get_width(fbc->gdkpixbuf);
	fbc->height = gdk_pixbuf_get_height(fbc->gdkpixbuf);
	fbc->scroll(fbc, 0, 0); /* Update offsets */
}

static void cmd_goto_top (void)
{
	if (last == command)
	{
		fbc->yoffset = last_y;
		command = 0;
	} else {
		last_y = fbc->yoffset;
		fbc->yoffset = 0;
	}
}

static void cmd_goto_bottom (void)
{
	if (last == command)
	{
		fbc->yoffset = last_y;
		command = 0;
	} else {
		last_y = fbc->yoffset;
		// XXX: 600 = framebuffer.height
		fbc->yoffset = fbc->height - 600;
	}
}

typedef void (*command_t) (void);
command_t keymap[] = { [12] = cmd_redraw, /* CTRL-L */
		       [27] = cmd_quit,	  /* ESC */
		       ['q'] = cmd_quit,
		       ['s'] = cmd_save,
		       ['x'] = cmd_flip_x,
		       ['y'] = cmd_flip_y,
		       ['z'] = cmd_flip_z,
		       ['Z'] = cmd_flip_z,
		       [KEY_HOME] = cmd_goto_top, [KEY_END] = cmd_goto_bottom,
		       [KEY_NPAGE] = cmd_next_page, [KEY_PPAGE] = cmd_prev_page,
		       [KEY_DOWN] = cmd_down, [KEY_UP] = cmd_up,
		       [KEY_LEFT] = cmd_left, [KEY_RIGHT] = cmd_right,
		       ['0'] = cmd_set_zoom, ['1'] = cmd_set_zoom,
		       ['2'] = cmd_set_zoom, ['3'] = cmd_set_zoom,
		       ['4'] = cmd_set_zoom, ['5'] = cmd_set_zoom,
		       ['6'] = cmd_set_zoom, ['7'] = cmd_set_zoom,
		       ['8'] = cmd_set_zoom, ['9'] = cmd_set_zoom,
		       ['+'] = cmd_zoom_in, ['-'] = cmd_zoom_out,
};

static command_t get_command (int ch)
{
	if (ch < sizeof (keymap))
		return keymap[ch] ?: cmd_unbound;
	return cmd_unbound;
}

static void main_loop (void)
{
	command_t cmd;
	WINDOW *win = initscr();

	refresh();
	noecho();
	cbreak();
	keypad(win, 1); /* Handle KEY_xxx */

	/* Main loop */
	for (;last != -1;)
	{
		fbc->draw (fbc);

		command = getch ();
		cmd = get_command (command);
		cmd ();
		last = command;
	}

	endwin ();
}

int main(int argc, char *argv[])
{
	extern int optind;
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
		main_loop ();

	fprintf (stderr, "%s %s -p%d -s%f -x%d -y%d\n", argv[0],
		 argv[optind], fbc->pagenum + 1,
		 fbc->scale, fbc->xoffset, fbc->yoffset);
out_nostatus:
	fbcanvas_destroy(fbc);
	return 0;
}
