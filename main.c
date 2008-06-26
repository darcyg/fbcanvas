/*
 * main.c - 17.5.2008 - 25.6.2008 Ari & Tero Roponen
 */

#include <linux/input.h>
#include <linux/vt.h>
#include <sys/ioctl.h>
#include <argp.h>
#include <fcntl.h>
#include <ncurses.h>
#undef scroll
#include <poll.h>
#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "document.h"
#include "fbcanvas.h"
#include "commands.h"
#include "keymap.h"

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
const char *argp_program_version = "fb version 20080624";

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

bool need_repaint = false;

void handle_signal(int s)
{
	int fd = open("/dev/tty", O_RDWR);
	if (fd)
	{
		if (s == SIGUSR1)
		{
			/* Release display */
			ioctl(fd, VT_RELDISP, 1);
		} else if (s == SIGUSR2) {
			/* Acquire display */
			ioctl(fd, VT_RELDISP, VT_ACKACQ);
			need_repaint = true;
		}

		close(fd);
	}
}

static int read_key(void)
{
	static int fd = -1;
	static struct pollfd pfd[2];
	static sigset_t sigs;
	static int modifiers = 0;

	if (fd == -1)
	{
		sigfillset(&sigs);
		sigdelset(&sigs, SIGUSR1);
		sigdelset(&sigs, SIGUSR2);

		fd = open("/dev/input/event2", O_RDONLY);
		if (fd < 0)
		{
			perror("Could not open /dev/input/event2");
			return -1;
		}

		pfd[0].fd = STDIN_FILENO;
		pfd[0].events = POLLIN;
		pfd[1].fd = fd;
		pfd[1].events = POLLIN;
		return 0;
	}

	for (;;)
	{
		int ret = ppoll(pfd, 2, NULL, &sigs);
		if (ret == -1) /* error, most likely EINTR. */
		{
			/* This will be handled later */
		} else if (ret == 0) { /* Timeout */
			/* Nothing to do */
		} else {
			/* evdev input available */
			if (pfd[1].revents)
			{
				struct input_event ev;
				read(fd, &ev, sizeof(ev));

				if (ev.type == EV_KEY)
				{
					unsigned int m = 0;
					if (ev.code == KEY_LEFTSHIFT || ev.code == KEY_RIGHTSHIFT)
						m = SHIFT;
					if (ev.code == KEY_LEFTCTRL || ev.code == KEY_RIGHTCTRL)
						m = CONTROL;
					if (ev.code == KEY_LEFTALT || ev.code == KEY_RIGHTALT)
						m = ALT;

					if (ev.value == 1 || ev.value == 2)
						modifiers |= m;
					else if (ev.value == 0)
						modifiers &= ~m;
				}

				if (need_repaint)
				{
					need_repaint = false;
					return 'l' | CONTROL;
				}
			}

			/* stdin input available */
			if (pfd[0].revents)
			{
				int key = tolower(getch());

				/* CTRL-A ... CTRL-Z */
				if (key >= 1 && key <= 26)
					key += 'a' - 1;

				return modifiers | key;
			}
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
			doc->draw(doc);

			ch = read_key();
			command = lookup_command (ch);
			command (doc);
		}
	}

	endwin ();
}

static int view_file (struct document *doc, struct prefs *prefs)
{
	char status[128];

	/* First call tries to initialize the input devices */
	if (read_key() < 0)
		return 1;

	if (prefs->page < doc->pagecount)
		doc->pagenum = prefs->page;
	doc->xoffset = prefs->x;
	doc->yoffset = prefs->y;
	doc->scale = prefs->scale;

	doc->update(doc);

	if (! prefs->quiet)
	{
		sprintf(status, "%s - %s", argp_program_version, doc->filename);
		doc->set_message(doc, status);
	}

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
			init_terminal();
			ret = view_file (doc, &prefs);
		}
		doc->close (doc);
	}

	return ret;
}
