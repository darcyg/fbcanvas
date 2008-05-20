/*
 * main.c - 17.5.2008 - 20.5.2008 Ari & Tero Roponen
 */

#define _GNU_SOURCE
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include "fbcanvas.h"

int main(int argc, char *argv[])
{
	char filename[256];
	struct fbcanvas *fbc;
	WINDOW *win;
	char *canon_name;

	if (argc != 2)
	{
		fprintf (stderr, "Usage: %s file.pdf", argv[0]);
		return 1;
	}

	canon_name = canonicalize_file_name (argv[1]);
	sprintf (filename, "file://%s", canon_name);
	free (canon_name);

	win = initscr();
	refresh();
	noecho();
	cbreak();
	keypad(win, 1); /* Handle KEY_xxx */

	fbc = fbcanvas_create(filename);

	fbc->update(fbc);
	fbc->draw(fbc);

	for (;;)
	{
		int ch = getch();

		switch (ch)
		{
			case KEY_NPAGE:
			{
				fbc->pagenum++;
				fbc->update(fbc);
				fbc->draw(fbc);
				break;
			}

			case KEY_PPAGE:
			{
				if (fbc->pagenum > 0)
				{
					fbc->pagenum--;
					fbc->update(fbc);
					fbc->draw(fbc);
				}
				break;
			}

			case KEY_DOWN:
			{
				fbc->yoffset += 50;
				fbc->draw(fbc);
				break;
			}

			case KEY_UP:
			{
				fbc->yoffset -= 50;
				fbc->draw(fbc);
				break;
			}

			case KEY_LEFT:
			{
				fbc->xoffset -= 50;
				fbc->draw(fbc);
				break;
			}

			case KEY_RIGHT:
			{
				fbc->xoffset += 50;
				fbc->draw(fbc);
				break;
			}

			case '+':
			{
				fbc->scale += 0.5;
				fbc->update(fbc);
				fbc->draw(fbc);
				break;
			}

			case '-':
			{
				if (fbc->scale >= 1.0)
				{
					fbc->scale -= 0.5;
					fbc->update(fbc);
					fbc->draw(fbc);
				}
				break;
			}

			case 's':
			{
				GError *err = NULL;
				char savename[256];

				sprintf(savename, "%s-pg-%d.png",
					basename(fbc->filename), fbc->pagenum + 1);
				if (!gdk_pixbuf_save(fbc->gdkpixbuf, savename, "png", &err))
					fprintf (stderr, "%s", err->message);
				break;
			}

			case KEY_HOME:
			{
				fbc->yoffset = 0;
				fbc->draw(fbc);
				break;
			}

			case 27: /* ESC */
				goto out;
                }
	}
out:

	fbcanvas_destroy(fbc);
	endwin();
	return 0;
}
