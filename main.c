/*
 * main.c - 17.5.2008 - 24.5.2008 Ari & Tero Roponen
 */

#include <magic.h>
#include <ncurses.h>
#undef scroll
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fbcanvas.h"

static struct
{
	int page;
	signed int x;
	signed int y;
	double scale;
} prefs = {0, 0, 0, 1.0};

static void cleanup(void)
{
	endwin();
}

static int parse_arguments (int argc, char *argv[])
{
	extern char *optarg;
	char *opts = "p:s:x:y:";
	int i;
	
	while ((i = getopt (argc, argv, opts)) != -1)
	{
		switch (i)
		{
		default:
			fprintf (stderr, "Invalid option: %c", i);
			return 1;
		case 'p':
			prefs.page = atoi (optarg) - 1;
			break;
		case 's':
			prefs.scale = strtod (optarg, NULL);
			fprintf (stderr, "%f\n", prefs.scale);
			break;
		case 'x':
			prefs.x = atoi (optarg);
			break;
		case 'y':
			prefs.y = atoi (optarg);
			break;
		}
	}
	return 0;
}

int main(int argc, char *argv[])
{
	extern int optind;

	char filename[256];
	struct fbcanvas *fbc;
	WINDOW *win;

	if (parse_arguments (argc, argv) || (optind != argc - 1))
	{
		fprintf (stderr, "Usage: %s [-pn] [-sn] [-xn] [-yn] file.pdf\n", argv[0]);
		return 1;
	}

	atexit(cleanup);

	win = initscr();
	refresh();
	noecho();
	cbreak();
	keypad(win, 1); /* Handle KEY_xxx */

	fbc = fbcanvas_create(argv[optind]);
	if (prefs.page < fbc->pagecount)
		fbc->pagenum = prefs.page;
	fbc->xoffset = prefs.x;
	fbc->yoffset = prefs.y;
	fbc->scale = prefs.scale;

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
				fbc->scroll(fbc, 0, fbc->height / 20);
				break;
			}

			case KEY_UP:
			{
				fbc->scroll(fbc, 0, -(fbc->height / 20));
				break;
			}

			case KEY_LEFT:
			{
				fbc->scroll(fbc, -(fbc->width / 20), 0);
				break;
			}

			case KEY_RIGHT:
			{
				fbc->scroll(fbc, fbc->width / 20, 0);
				break;
			}

			case '+':
			{
				fbc->scale += 0.1;
				fbc->update(fbc);
				break;
			}

			case '-':
			{
				if (fbc->scale >= 0.2)
				{
					fbc->scale -= 0.1;
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
				fbc->width = gdk_pixbuf_get_width(fbc->gdkpixbuf);
				fbc->height = gdk_pixbuf_get_height(fbc->gdkpixbuf);
				fbc->scroll(fbc, 0, 0); /* Update offsets */
				break;
			}

			case KEY_HOME:
			{
				fbc->yoffset = 0;
				break;
			}

			case KEY_END:
			{
				// XXX: 600 = framebuffer.height
				fbc->yoffset = fbc->height - 600;
				break;
			}

			case 27: /* ESC */
				goto out;
                }
	}
out:
	fprintf (stderr, "%s %s -p%d -s%f -x%d -y%d\n", argv[0],
		 argv[optind], fbc->pagenum + 1,
		 fbc->scale, fbc->xoffset, fbc->yoffset);

	fbcanvas_destroy(fbc);
	return 0;
}
