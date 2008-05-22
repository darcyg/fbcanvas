/*
 * main.c - 17.5.2008 - 20.5.2008 Ari & Tero Roponen
 */

#include <magic.h>
#include <ncurses.h>
#undef scroll
#include <stdlib.h>
#include <string.h>
#include "fbcanvas.h"

static void cleanup(void)
{
	endwin();
}

int main(int argc, char *argv[])
{
	char filename[256];
	struct fbcanvas *fbc;
	WINDOW *win;

	if (argc != 2)
	{
		fprintf (stderr, "Usage: %s file.pdf\n", argv[0]);
		return 1;
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
				fbc->scroll(fbc, 0, fbc->height / 10);
				break;
			}

			case KEY_UP:
			{
				fbc->scroll(fbc, 0, -(fbc->height / 10));
				break;
			}

			case KEY_LEFT:
			{
				fbc->scroll(fbc, -(fbc->width / 10), 0);
				break;
			}

			case KEY_RIGHT:
			{
				fbc->scroll(fbc, fbc->width / 10, 0);
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
