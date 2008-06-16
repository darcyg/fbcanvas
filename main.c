/*
 * main.c - 17.5.2008 - 16.6.2008 Ari & Tero Roponen
 */

#include <linux/vt.h>
#include <argp.h>
#include <fcntl.h>
#include <ncurses.h>
#undef scroll
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
	case ARGP_KEY_SUCCESS:
		if (state->argc - state->next != 1)
			argp_usage (state);
		break;
	}
	return 0;
}

int read_key(int s)
{
	static int repaint;

	if (s == 0)
	{
		fd_set rfds;
		sigset_t sigs;

		FD_ZERO(&rfds);
		FD_SET(0, &rfds);

		sigfillset(&sigs);
		sigdelset(&sigs, SIGUSR1);
		sigdelset(&sigs, SIGUSR2);

		for (;;)
		{
			int ret = pselect(1, &rfds, NULL, NULL, NULL, &sigs);
			if (ret == -1)
			{
				if (repaint)
				{
					repaint = 0;
					return 12; /* CTRL-L */
				}
			} else if (ret) {
				return getch();
			}
		}
	} else {
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
				repaint = 1;
			}

			close(fd);
		}
	}
}

static void main_loop (struct document *doc)
{
	WINDOW *win = initscr();

	refresh();
	noecho();
	cbreak();
	keypad(win, 1); /* Handle KEY_xxx */

	setup_keys ();

	if (setjmp (exit_loop) == 0)
	{
		int ch;
		command_t command;
		for (;;)		/* Main loop */
		{
			doc->fbcanvas->fb->draw(doc->fbcanvas->fb,
				doc->gdkpixbuf, doc->xoffset, doc->yoffset);

			ch = read_key(0);
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

	extern void register_plugins (void);
	register_plugins ();

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
			ret = view_file (doc, &prefs);
		}

		doc->close(doc);
	}

	return ret;
}
