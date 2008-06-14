/*
 * main.c - 17.5.2008 - 14.6.2008 Ari & Tero Roponen
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
#include "document.h"
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

static struct document *ugly_hack;

static void handle_signal(int s)
{
	struct framebuffer *fb = ugly_hack->fbcanvas->fb;

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
			fb->draw(fb,
				ugly_hack->gdkpixbuf,
				ugly_hack->xoffset, ugly_hack->yoffset);
		}

		close(fd);
	}
}

static void main_loop (struct document *doc)
{
	WINDOW *win = initscr();

	signal(SIGUSR1, handle_signal);
	signal(SIGUSR2, handle_signal);

	refresh();
	noecho();
	cbreak();
	keypad(win, 1); /* Handle KEY_xxx */

	setup_keys ();

	ugly_hack = doc;

	if (setjmp (exit_loop) == 0)
	{
		int ch;
		command_t command;
		for (;;)		/* Main loop */
		{
			doc->fbcanvas->fb->draw(doc->fbcanvas->fb,
				doc->gdkpixbuf, doc->xoffset, doc->yoffset);

			ch = getch ();
			command = lookup_command (ch);
			command (doc);
		}
	}

	endwin ();
}

static int view_file (struct document *doc, struct prefs *prefs)
{
	if (prefs->page < doc->pagecount)
		doc->pagenum = prefs->page;
	doc->xoffset = prefs->x;
	doc->yoffset = prefs->y;
	doc->scale = prefs->scale;

	doc->update(doc);

	main_loop (doc);

	fprintf (stderr, "%s %s -p%d -s%f -x%d -y%d\n",
		 program_invocation_short_name,
		 doc->filename, doc->pagenum + 1,
		 doc->scale, doc->xoffset, doc->yoffset);
}

int main(int argc, char *argv[])
{
	int ind;
	int ret = 1;

	struct prefs prefs = {0, 0, 0, 1.0, };
	struct argp argp = {options, parse_arguments, "FILE", };

	struct document *doc;

	argp_parse (&argp, argc, argv, 0, &ind, &prefs);

	doc = open_document(argv[ind]);
	if (doc)
	{
		if (prefs.just_pagecount)
		{
			fprintf (stderr, "%s has %d page%s.\n",
				 doc->filename, doc->pagecount,
				 doc->pagecount > 1 ? "s":"");
			ret = 0;
		} else if (prefs.grep_str) {
			ret = doc->grep(doc, prefs.grep_str);
		} else {
			/* Asetetaan konsolinvaihto lähettämään signaaleja */
			int fd = open("/dev/tty", O_RDWR);
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

			ret = view_file (doc, &prefs);
		}

		doc->close(doc);
	}

	return ret;
}
