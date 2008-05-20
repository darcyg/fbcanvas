/*
 * main.c - 17.5.2008 - 20.5.2008 Ari & Tero Roponen
 */

#include <magic.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include "fbcanvas.h"

static char *supported_filetypes[] = {
	"JPEG", "PDF", NULL
};

/* Return NULL or a string describing the unsupported file type.
   Returned string must be freed after use. */
static char *is_supported_filetype (char *filename)
{
	magic_t magic = magic_open (MAGIC_NONE);
	int i, ret = magic_load (magic, NULL);
	const char *type = magic_file (magic, filename);

	for (i = 0; supported_filetypes[i]; i++)
	{
		if (strncmp (type, supported_filetypes[i],
			     strlen (supported_filetypes[i])) == 0)
		{
			magic_close (magic);
			return NULL;
		}
	}

	type = strdup (type);
	magic_close (magic);
	return (char *)type;
}

static void cleanup(void)
{
	endwin();
}

int main(int argc, char *argv[])
{
	char filename[256];
	struct fbcanvas *fbc;
	WINDOW *win;
	char *descr;

	if (argc != 2)
	{
		fprintf (stderr, "Usage: %s file.pdf", argv[0]);
		return 1;
	}

	if (descr = is_supported_filetype(argv[1]))
	{
		fprintf (stderr, "Can't handle type: %s\n", descr);
		free (descr);
		return 2;
	}

	atexit(cleanup);

	win = initscr();
	refresh();
	noecho();
	cbreak();
	keypad(win, 1); /* Handle KEY_xxx */

	fbc = fbcanvas_create(argv[1]);
	fbc->update(fbc);

	/* Main loop */
	for (;;)
	{
		int ch;

		fbc->draw (fbc);

		ch = getch ();
		switch (ch)
		{
			case KEY_NPAGE:
			{
				if (fbc->pagenum < fbc->pagecount - 1)
				{
					fbc->pagenum++;
					fbc->update(fbc);
				}
				break;
			}

			case KEY_PPAGE:
			{
				if (fbc->pagenum > 0)
				{
					fbc->pagenum--;
					fbc->update(fbc);
				}
				break;
			}

			case KEY_DOWN:
			{
				fbc->yoffset += 50;
				break;
			}

			case KEY_UP:
			{
				fbc->yoffset -= 50;
				break;
			}

			case KEY_LEFT:
			{
				fbc->xoffset -= 50;
				break;
			}

			case KEY_RIGHT:
			{
				fbc->xoffset += 50;
				break;
			}

			case '+':
			{
				fbc->scale += 0.5;
				fbc->update(fbc);
				break;
			}

			case '-':
			{
				if (fbc->scale >= 1.0)
				{
					fbc->scale -= 0.5;
					fbc->update(fbc);
				}
				break;
			}

			case 's':
			{
				GError *err = NULL;
				char savename[256];

				sprintf(savename, "%s-pg-%d.png", basename(fbc->filename),
					fbc->pagenum + 1);
				if (!gdk_pixbuf_save(fbc->gdkpixbuf, savename, "png", &err, NULL))
					fprintf (stderr, "%s", err->message);
				break;
			}

			case 'x':
			{
				GdkPixbuf *tmp = gdk_pixbuf_flip(fbc->gdkpixbuf, TRUE);
				g_object_unref(fbc->gdkpixbuf);
				fbc->gdkpixbuf = tmp;
				break;
			}

			case 'y':
			{
				GdkPixbuf *tmp = gdk_pixbuf_flip(fbc->gdkpixbuf, FALSE);
				g_object_unref(fbc->gdkpixbuf);
				fbc->gdkpixbuf = tmp;
				break;
			}

			case 'z':
			case 'Z':
			{
				int angle = (ch == 'z' ? 90 : 270);
				GdkPixbuf *tmp = gdk_pixbuf_rotate_simple(fbc->gdkpixbuf, angle);
				g_object_unref(fbc->gdkpixbuf);
				fbc->gdkpixbuf = tmp;
				break;
			}

			case KEY_HOME:
			{
				fbc->yoffset = 0;
				break;
			}

			case 27: /* ESC */
				goto out;
                }
	}
out:

	fbcanvas_destroy(fbc);
	return 0;
}
