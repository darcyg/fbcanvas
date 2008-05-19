/*
 * main.c - 17.5.2008 - 17.5.2008 Ari & Tero Roponen
 */

#include <poppler/glib/poppler.h>
#include <curses.h>
#include <math.h>
#include <string.h>
#include "fbcanvas.h"

static void show_pdf(char *filename, double scale, int pagenum)
{
	unsigned char *src, *dst;
	struct fbcanvas *fbc;
	GdkPixbuf *gdkpixbuf;
	GError *err = NULL;
	PopplerDocument *document;
	PopplerPage *page;
	int page_count;
	double width, height;
	int i, j;

	g_type_init();

	document = poppler_document_new_from_file(filename, NULL, &err);
	if (!document)
	{
		/* TODO: käsittele virhe */
	}

	page_count = poppler_document_get_n_pages(document);

	page = poppler_document_get_page(document, pagenum);
	if (!page)
	{
		/* TODO: käsittele virhe */
	}

	poppler_page_get_size(page, &width, &height);
	fprintf(stderr, "Size: %lfx%lf\n", width, height);

	gdkpixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
		TRUE, 8, ceil(width * scale), ceil(height * scale));
	poppler_page_render_to_pixbuf(page, 0, 0,
		ceil(width), ceil(height), scale, 0, gdkpixbuf);

	fbc = fbcanvas_create(gdk_pixbuf_get_width(gdkpixbuf),
		gdk_pixbuf_get_height(gdkpixbuf));

	src = gdk_pixbuf_get_pixels(gdkpixbuf);
	dst = fbc->buffer;

	/* TODO: Tämä toimii vain 16-bittisellä framebufferilla. */
	for (j = 0; j < gdk_pixbuf_get_height(gdkpixbuf); j++)
	{
		for (i = 0; i < gdk_pixbuf_get_width(gdkpixbuf); i++)
		{
			unsigned char red = *src++;
			unsigned char green = *src++;
			unsigned char blue = *src++;
			unsigned char alpha = *src++;

			/* TODO: RGB:n suhteiden pitäisi olla 5/6/5 bittiä. */
			*(dst+0) = (((red * 32 / 256) << 3) & 0b11111000) |
				   ((green * 64 / 256) & 0b00000111);
			*(dst+1) = (((green * 64 / 256) << 3) & 0b11100000) |
				   ((blue * 32 / 256) & 0b00011111);
			dst += 2;
		}
	}

	fbcanvas_draw(fbc);
	fbcanvas_destroy(fbc);
}

int main(int argc, char *argv[])
{
	int pagenum = 0;
	double scale = 1.0;
	char filename[256];
	WINDOW *win;

	sprintf (filename, "file://%s", argv[1]);

	win = initscr();
	refresh();
	noecho();
	cbreak();
	keypad(win, 1); /* Handle KEY_xxx */

	show_pdf(filename, scale, pagenum);

	for (;;)
	{
		int ch = getch();

		switch (ch)
		{
			case KEY_NPAGE:
			{
				pagenum++;
				show_pdf(filename, scale, pagenum);
				break;
			}

			case KEY_PPAGE:
			{
				if (pagenum > 0)
				{
					pagenum--;
					show_pdf(filename, scale, pagenum);
				}
				break;
			}

			case KEY_DOWN:
			{
				/* TODO: yoffset += 50 */
				break;
			}

			case KEY_UP:
			{
				/* TODO: yoffset -= 50 */
				break;
			}

			case KEY_LEFT:
			{
				/* TODO: xoffset -= 50 */
				break;
			}

			case KEY_RIGHT:
			{
				/* TODO: xoffset += 50 */
				break;
			}

			case '+':
			{
				scale += 0.5;
				show_pdf(filename, scale, pagenum);
				break;
			}

			case '-':
			{
				scale -= 0.5;
				show_pdf(filename, scale, pagenum);
				break;
			}

			case KEY_END:
				goto out;
                }
	}
out:

	endwin();
	return 0;
}
