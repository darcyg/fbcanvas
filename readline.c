/* readline.c - 8.7.2008 - 8.7.2008 Ari & Tero Roponen */
#include <linux/input.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "document.h"
#include "keymap.h"		/* SHIFT */
#include "terminal.h"

static char fb_key_to_char (int key)
{
	int ind = key & ~SHIFT;
	bool shift = key & SHIFT;
	char ch;

	switch (ind)
	{
	default:
		ch = 0;
		break;
	case KEY_Q ... KEY_P:
		ch = "qQwWeErRtTyYuUiIoOpP"[2*(ind - KEY_Q) + shift];
		break;
	case KEY_A ... KEY_L:
		ch = "aAsSdDfFgGhHjJkKlL"[2*(ind - KEY_A) + shift];
		break;
	case KEY_Z ... KEY_DOT:
		ch = "zZxXcCvVbBnNmM,;.:"[2*(ind - KEY_Z) + shift];
		break;
	case KEY_1 ... KEY_0:
		ch = "1!2\"3#4$5%6&7/8(9)0="[2*(ind - KEY_1) + shift];
		break;
	case KEY_SPACE:
		ch = ' ';
		break;
	}

	return ch;
}

static char linebuf[128];	/* XXX */
static int linepos;

char *fb_read_line (struct document *doc, char *prompt)
{
	int key;
	char ch;
	struct event *ev;

	linebuf[linepos = 0] = '\0';

	while (true)
	{
		doc->set_message (doc, "%s%s_", prompt, linebuf);
		doc->draw (doc);
		ev = get_event (doc);
		key = ev->keycode;
		ch = fb_key_to_char (key);

		if (ch)		/* insertable char */
		{
			linebuf[linepos++] = ch;
			linebuf[linepos] = '\0';
			continue;
		}

		switch (key)
		{
		default:
			printf ("\a"); /* bell */
			fflush (stdout);
			break;
		case KEY_L | CONTROL: /* get_event returns this. */
			break;
		case KEY_ESC:
			return NULL;
		case KEY_ENTER:
			goto finish;
		case KEY_BACKSPACE:
			if (linepos > 0)
				linebuf[--linepos] = '\0';
			break;
		}
	}
finish:
	linebuf[linepos] = '\0';
	return strdup (linebuf);
}
