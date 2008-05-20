/*
 * main.c - 17.5.2008 - 20.5.2008 Ari & Tero Roponen
 */

#include <poppler/glib/poppler.h>
#include <ncurses.h>
#include <math.h>
#include <string.h>
#include "fbcanvas.h"

static void update_pdf(struct fbcanvas *fbc)
{
	static PopplerPage *page = NULL;
	unsigned char *src, *dst;
	GError *err = NULL;
	static double width, height;

	if (page)
		g_object_unref(page);
	page = poppler_document_get_page(fbc->document, fbc->pagenum);
	if (!page)
	{
		/* TODO: käsittele virhe */
	}

	poppler_page_get_size(page, &width, &height);
	//fprintf(stderr, "Size: %lfx%lf\n", width, height);

	fbc->gdkpixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
		TRUE, 8, ceil(width * fbc->scale), ceil(height * fbc->scale));
	poppler_page_render_to_pixbuf(page, 0, 0,
		ceil(width), ceil(height), fbc->scale, 0, fbc->gdkpixbuf);
}

int main(int argc, char *argv[])
{
	char filename[256];
	struct fbcanvas *fbc;
	WINDOW *win;

	sprintf (filename, "file://%s", argv[1]);

	win = initscr();
	refresh();
	noecho();
	cbreak();
	keypad(win, 1); /* Handle KEY_xxx */

	fbc = fbcanvas_create(filename);

	update_pdf(fbc);
	fbc->draw(fbc);

	for (;;)
	{
		int ch = getch();

		switch (ch)
		{
			case KEY_NPAGE:
			{
				fbc->pagenum++;
				update_pdf(fbc);
				fbc->draw(fbc);
				break;
			}

			case KEY_PPAGE:
			{
				if (fbc->pagenum > 0)
				{
					fbc->pagenum--;
					update_pdf(fbc);
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
				if (fbc->yoffset >= 50)
				{
					fbc->yoffset -= 50;
					fbc->draw(fbc);
				}
				break;
			}

			case KEY_LEFT:
			{
				if (fbc->xoffset >= 50)
				{
					fbc->xoffset -= 50;
					fbc->draw(fbc);
				}
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
				update_pdf(fbc);
				fbc->draw(fbc);
				break;
			}

			case '-':
			{
				if (fbc->scale >= 1.0)
				{
					fbc->scale -= 0.5;
					update_pdf(fbc);
					fbc->draw(fbc);
				}
				break;
			}

			case 's':
			{
				GError *err = NULL;
				char savename[256];

				sprintf(savename, "%s-pg-%d.png", fbc->filename, fbc->pagenum + 1);
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
