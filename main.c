/*
 * main.c - 17.5.2008 - 13.7.2008 Ari & Tero Roponen
 */

#include <linux/input.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <attr/xattr.h>
#include <argp.h>
#include <fcntl.h>
#include <poll.h>
#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "document.h"
#include "commands.h"
#include "extcmd.h"
#include "keymap.h"
#include "terminal.h"

struct prefs
{
	char *page;
	signed int x;
	signed int y;
	double scale;

	int just_pagecount;
	char *grep_str;
	int quiet;

	char *state_file;
};

/* Can't be static. */
const char *argp_program_version = "fb version 20080713";

static struct argp_option options[] = {
	{"count", 'c', NULL, 0, "display page count"},
	{"grep", 'g', "TEXT", 0, "search for text"},
	{"page", 'p', "PAGE", OPTION_ARG_OPTIONAL, "goto given page or tag"},
	{"quiet", 'q', NULL, 0, "Don't display startup message"},
	{"restore", 'r', NULL, 0, "Restore previous state"},
	{"scale", 's', "SCALE", 0, "set scale factor"},
	{NULL, 'x', "X", 0, "set x-offset"},
	{NULL, 'y', "Y", 0, "set y-offset"},
	{NULL}
};

/*
 * returns 0 if successful, errno if fails.
 */
static int get_attribute(const char *filename, const char *attr, char **value)
{
	/* Get attribute len */
	char *buf;
	int ret = getxattr(filename, attr, NULL, 0);
	if (ret < 0)
		return errno;

	buf = malloc (ret);
	ret = getxattr(filename, attr, buf, ret);
	if (ret < 0)
	{
		free (buf);
		return errno;
	}

	*value = buf;
	return 0;
}

static void load_prefs(char *filename, struct prefs *prefs)
{
	char *value;

	prefs->state_file = strdup(filename); //XXX

	if (get_attribute(filename, "user.fbprefs.scale", &value) == 0)
	{
		prefs->scale = strtod (value, NULL);
		free (value);
	}
}

static void save_prefs(struct prefs *prefs)
{
	char buf[16];

	sprintf(buf, "%lf", prefs->scale);
	setxattr(prefs->state_file, "user.fbprefs.scale", buf, strlen(buf)+1, 0);
}

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
	case 'p':		/* page */
		if (arg)
			prefs->page = arg;
		else
			prefs->page = "current_page";
		break;
	case 'q':
		prefs->quiet = 1;
		break;
	case 'r':
		prefs->state_file = (char *)1;
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

static int view_file (struct document *doc, struct prefs *prefs)
{
	doc->pagenum = 0;	/* default */
	doc->xoffset = prefs->x;
	doc->yoffset = prefs->y;
	doc->scale = prefs->scale;

	doc->update(doc);

	if (! prefs->quiet)
		doc->set_message (doc, "%s\n%s", argp_program_version, doc->filename);

	if (prefs->page)
	{
		char buf[20];
		sprintf (buf, "goto %s", prefs->page);
		execute_extended_command (doc, buf);
	}

	doc->main_loop(doc);

	if (prefs->page)
	{
		char cmd[] = "tag current_page";
		execute_extended_command (doc, cmd);
	}

	fprintf (stderr, "%s %s -p%d -s%f -x%d -y%d\n",
		 program_invocation_short_name,
		 doc->filename, doc->pagenum + 1,
		 doc->scale, doc->xoffset, doc->yoffset);
}

static void cleanup(void)
{
	int mode = -1;

	/* Make sure console is usable after exit */
	if (ioctl(0, KDGETMODE, &mode) >= 0)
	{
		if (mode == KD_GRAPHICS)
			ioctl(0, KDSETMODE, KD_TEXT);
	}
}

int main(int argc, char *argv[])
{
	int ind;
	int ret = 1;

	struct prefs prefs = {NULL, 0, 0, 1.0, };
	struct argp argp = {options, parse_arguments, "FILE", };

	struct document *doc;

	atexit(cleanup);

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
			if (prefs.state_file)
				load_prefs(argv[i], &prefs);
			ret = view_file (doc, &prefs);
			if (prefs.state_file)
			{
				char buf[8];
				sprintf (buf, "%d", doc->pagenum + 1);
				prefs.page = buf;
				prefs.scale = doc->scale;
				save_prefs(&prefs);
			}
		}

		doc->close (doc);
	}
out:
	return ret;
}
