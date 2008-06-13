/*
 * main.c - 17.5.2008 - 13.6.2008 Ari & Tero Roponen
 */

#include <argp.h>
#include <fcntl.h>
#include <ncurses.h>
#undef scroll
#include <linux/vt.h>
#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fbcanvas.h"
#include "commands.h"

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
	WINDOW *win = initscr();

	signal(SIGUSR1, handle_signal);
	signal(SIGUSR2, handle_signal);

	refresh();
	noecho();
	cbreak();
	keypad(win, 1); /* Handle KEY_xxx */

	setup_keys ();

	ugly_hack = fbc;

	if (setjmp (exit_loop) == 0)
	{
		command_t command;
		for (;;)		/* Main loop */
		{
			fbc->draw (fbc);

			ch = getch ();
			command = lookup_command (ch);
			command (fbc, ch, last);
			last = ch;
		}
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
