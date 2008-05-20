/*
 * main.c - 17.5.2008 - 20.5.2008 Ari & Tero Roponen
 */

#include <poppler/glib/poppler.h>
#include <ncurses.h>
#include <math.h>
#include <string.h>
#include "fbcanvas.h"

static void show_pdf(struct fbcanvas *fbc, char *filename, double scale, int pagenum)
{
	static int page_count = -1;
	static int current_pagenum = -1;
	static PopplerDocument *document;
	static PopplerPage *page = NULL;
	unsigned char *src, *dst;
	GError *err = NULL;
	static double width, height;

	g_type_init();

	if (!document)
	{
		document = poppler_document_new_from_file(filename, NULL, &err);
		if (!document)
		{
			/* TODO: käsittele virhe */
		}

		page_count = poppler_document_get_n_pages(document);
	}

	if (pagenum != current_pagenum)
	{
		if (page)
			g_object_unref(page);
		page = poppler_document_get_page(document, pagenum);
		current_pagenum = pagenum;

		poppler_page_get_size(page, &width, &height);
		//fprintf(stderr, "Size: %lfx%lf\n", width, height);
	}

	if (!page)
	{
		/* TODO: käsittele virhe */
	}


	fbc->gdkpixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
		TRUE, 8, ceil(width * scale), ceil(height * scale));
	poppler_page_render_to_pixbuf(page, 0, 0,
		ceil(width), ceil(height), scale, 0, fbc->gdkpixbuf);
}

int main(int argc, char *argv[])
{
	int pagenum = 0;
	double scale = 1.0;
	char filename[256];
	struct fbcanvas *fbc = fbcanvas_create();
	WINDOW *win;

	sprintf (filename, "file://%s", argv[1]);

	win = initscr();
	refresh();
	noecho();
	cbreak();
	keypad(win, 1); /* Handle KEY_xxx */

	show_pdf(fbc, filename, scale, pagenum);
	fbc->draw(fbc);

	for (;;)
	{
		int ch = getch();

		switch (ch)
		{
			case KEY_NPAGE:
			{
				pagenum++;
				show_pdf(fbc, filename, scale, pagenum);
				fbc->draw(fbc);
				break;
			}

			case KEY_PPAGE:
			{
				if (pagenum > 0)
					pagenum--;
				show_pdf(fbc, filename, scale, pagenum);
				fbc->draw(fbc);
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
					fbc->yoffset -= 50;
				fbc->draw(fbc);
				break;
			}

			case KEY_LEFT:
			{
				if (fbc->xoffset >= 50)
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
				scale += 0.5;
				show_pdf(fbc, filename, scale, pagenum);
				fbc->draw(fbc);
				break;
			}

			case '-':
			{
				if (scale >= 1.0)
					scale -= 0.5;
				show_pdf(fbc, filename, scale, pagenum);
				fbc->draw(fbc);
				break;
			}

			case 's':
			{
				GError *err = NULL;
				char savename[256];

				sprintf(savename, "%s-pg-%d.png", basename(filename), pagenum);
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

			case KEY_END:
				goto out;
                }
	}
out:

	fbcanvas_destroy(fbc);
	endwin();
	return 0;
}
