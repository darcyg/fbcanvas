/*
 * main.c - 17.5.2008 - 7.7.2008 Ari & Tero Roponen
 */

#include <linux/input.h>
#include <linux/vt.h>
#include <sys/ioctl.h>
#include <argp.h>
#include <fcntl.h>
#include <ncurses.h>
#include <poll.h>
#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "document.h"
#include "commands.h"
#include "keymap.h"
#include "terminal.h"

struct prefs
{
	int page;
	signed int x;
	signed int y;
	double scale;

	int just_pagecount;
	char *grep_str;
	int quiet;
};

/* Can't be static. */
const char *argp_program_version = "fb version 20080706";

static struct argp_option options[] = {
	{"count", 'c', NULL, 0, "display page count"},
	{"grep", 'g', "TEXT", 0, "search for text"},
	{"page", 'p', "PAGE", 0, "goto given page"},
	{"quiet", 'q', NULL, 0, "Don't display startup message"},
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
	case 'q':
		prefs->quiet = 1;
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
		if ((state->argc - state->next != 1)
		    && !prefs->just_pagecount
		    && !prefs->grep_str)
			argp_usage (state);
		break;
	}
	return 0;
}

static void parse_line(char *cmdline)
{
	fprintf(stderr, "Command: '%s'\n", cmdline);
}

void ncurses_main_loop (struct document *doc)
{
	WINDOW *win = initscr();

	refresh();
	noecho();
	cbreak();
	keypad(win, 1); /* Handle KEY_xxx */

	if (setjmp (exit_loop) == 0)
	{
		int ch;
		for (;;)		/* Main loop */
		{
			doc->draw(doc);

			ch = read_key(doc);
			if (doc->flags & COMMAND_MODE)
			{
				char buf[128] = {'>', ' '};
				int ind = 2;

				do
				{
					doc->set_message(doc, buf);
					doc->draw(doc);
					ch = read_key(doc);

					if (ch && (ch != 106)) //106 taitaa olla return...
						buf[ind++] = ch;
				} while (doc->flags & COMMAND_MODE);

				if (buf[2])
					parse_line(buf+2);
			} else {
				command_t command = lookup_command (ch);
				command (doc);
			}
		}
	}

	endwin ();
}

static int view_file (struct document *doc, struct prefs *prefs)
{
	char status[128];

	if (prefs->page < doc->pagecount)
		doc->pagenum = prefs->page;
	doc->xoffset = prefs->x;
	doc->yoffset = prefs->y;
	doc->scale = prefs->scale;

	doc->update(doc);

	if (! prefs->quiet)
	{
		sprintf(status, "%s\n%s", argp_program_version, doc->filename);
		doc->set_message(doc, status);
	}

	doc->main_loop(doc);

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

	setup_keys();

	for (int i = ind; i < argc; i++)
	{
		doc = open_document (argv[i]);
		if (! doc)
			continue;
		if (prefs.just_pagecount)
		{
			fprintf (stderr, "%s has %d page%s.\n",
				 doc->filename, doc->pagecount,
				 doc->pagecount > 1 ? "s":"");
			ret = 0;
		} else if (prefs.grep_str) {
			int r = doc->grep(doc, prefs.grep_str);
			if (!r)
				ret = 0;
		} else {
			ret = init_terminal();
			if (ret < 0)
				goto out;
			ret = view_file (doc, &prefs);
		}
		doc->close (doc);
	}
out:
	return ret;
}
