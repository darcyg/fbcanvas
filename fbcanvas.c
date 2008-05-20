/*
 * fbcanvas.c - 17.5.2008 - 20.5.2008 Ari & Tero Roponen
 */
#include <poppler/glib/poppler.h>
#include <sys/types.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fbcanvas.h"

static struct
{
	unsigned char *mem;
	int width;
	int height;
	int bpp;
	int refcount;
} framebuffer = {
	.mem = MAP_FAILED
};

static void update_pdf(struct fbcanvas *fbc);
static void draw_16bpp(struct fbcanvas *fbc);

struct fbcanvas *fbcanvas_create(char *filename)
{
	GError *err = NULL;
	struct fbcanvas *fbc = malloc(sizeof(*fbc));
	if (fbc)
	{
		/* Yritetään avata framebuffer-laite, ellei se ole jo auki. */
		if (framebuffer.refcount == 0)
		{
			void *mem;
			struct fb_var_screeninfo fbinfo;
			int fd = open("/dev/fb0", O_RDWR);
			if (fd < 0)
			{
				/* TODO: käsittele virhe */
				perror("open");
			}

			if (ioctl(fd, FBIOGET_VSCREENINFO, &fbinfo) < 0)
			{
				/* TODO: käsittele virhe */
				perror("ioctl");
			}

			framebuffer.width = fbinfo.xres;
			framebuffer.height = fbinfo.yres;
			framebuffer.bpp = fbinfo.bits_per_pixel;

			printf("%d x %d x %d\n", framebuffer.width,
				framebuffer.height, framebuffer.bpp);

			mem = mmap (NULL,
				framebuffer.width * framebuffer.height * (framebuffer.bpp / 8),
				PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
			if (mem == MAP_FAILED)
		        {
				/* TODO: käsittele virhe */
				perror("mmap");
			}

			close(fd);

			framebuffer.mem = mem;
			framebuffer.refcount++;
		}

		g_type_init();

		fbc->document = poppler_document_new_from_file(filename, NULL, &err);
		if (!fbc->document)
		{
			/* TODO: käsittele virhe */
		}

		fbc->page = NULL;
		fbc->pagecount = poppler_document_get_n_pages(fbc->document);
		fbc->filename = strdup(basename(filename));
		fbc->gdkpixbuf = NULL;
		fbc->xoffset = 0;
		fbc->yoffset = 0;
		fbc->scale = 1.0;
		fbc->pagenum = 0;

		switch (framebuffer.bpp)
		{
			case 16:
				fbc->draw = draw_16bpp;
				fbc->update = update_pdf;
				break;
			default:
				fprintf(stderr, "Unsupported depth: %d\n", framebuffer.bpp);
				exit(1);
		}
	}

	return fbc;
}

void fbcanvas_destroy(struct fbcanvas *fbc)
{
	if (fbc->page)
		g_object_unref(fbc->page);
	if (fbc->document)
		g_object_unref(fbc->document);
	if (fbc->gdkpixbuf)
		g_object_unref(fbc->gdkpixbuf);
	if (fbc->filename)
		free(fbc->filename);
	free(fbc);

	framebuffer.refcount--;

	if (framebuffer.refcount == 0)
	{
		munmap(framebuffer.mem,
			framebuffer.width * framebuffer.height * (framebuffer.bpp / 8));
	}
}

static void update_pdf(struct fbcanvas *fbc)
{
	GError *err = NULL;
	static double width, height;

	if (fbc->page)
		g_object_unref(fbc->page);
	fbc->page = poppler_document_get_page(fbc->document, fbc->pagenum);
	if (!fbc->page)
	{
		/* TODO: käsittele virhe */
	}

	poppler_page_get_size(fbc->page, &width, &height);
	//fprintf(stderr, "Size: %lfx%lf\n", width, height);

	fbc->gdkpixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
		TRUE, 8, ceil(width * fbc->scale), ceil(height * fbc->scale));
	poppler_page_render_to_pixbuf(fbc->page, 0, 0,
		ceil(width), ceil(height), fbc->scale, 0, fbc->gdkpixbuf);
}

static void draw_16bpp(struct fbcanvas *fbc)
{
	/* TODO: piirtäminen tehdään täällä. Ota huomioon offsetit ym.
	 * TODO: tarkista ettei mennä framebufferin rajojen yli.
	 */
	static unsigned short empty = 0x0000;
	unsigned int x, y;
	unsigned short *src, *dst;
	unsigned short color;
	unsigned char *data = gdk_pixbuf_get_pixels(fbc->gdkpixbuf);

	unsigned int width = gdk_pixbuf_get_width(fbc->gdkpixbuf);
	unsigned int height = gdk_pixbuf_get_height(fbc->gdkpixbuf);
	int pb_xoffset = 0, pb_yoffset = 0;
	int fb_xoffset = 0, fb_yoffset = 0;

	if (fbc->xoffset >= 0)
		pb_xoffset = fbc->xoffset;
	else
		fb_xoffset = -fbc->xoffset;

	if (fbc->yoffset >= 0)
		pb_yoffset = fbc->yoffset;
	else
		fb_yoffset = -fbc->yoffset;

	for (y = 0;; y++)
	{
		/* Framebufferin reuna tuli vastaan - lopetetaan. */
		if (y >= framebuffer.height)
			break;

		for (x = 0;; x++)
		{
			/* Framebufferin reuna tuli vastaan - lopetetaan. */
			if (x >= framebuffer.width)
				break;

			if (x < fb_xoffset || y < fb_yoffset)
				goto empty;

			if ((y - fb_yoffset < height - pb_yoffset) &&
			    (x - fb_xoffset < width - pb_xoffset))
			{
				unsigned char *tmp = data +
					4*width * (pb_yoffset + y - fb_yoffset) +
					4*(pb_xoffset + x - fb_xoffset);

				unsigned char red = *tmp++;
				unsigned char green = *tmp++;
				unsigned char blue = *tmp++;
				unsigned char alpha = *tmp++;
				color = 0;

				color |= ((32 * red / 256) & 0b00011111) << 11;
				color |= ((64 * green / 256) & 0b00111111) << 5;
				color |= ((32 * blue / 256) & 0b00011111) << 0;
				src = &color;
			} else {
empty:
				src = &empty;
			}

			dst = (unsigned short *)framebuffer.mem +
				y * framebuffer.width + x;
			*dst = *src;
		}
	}
}

