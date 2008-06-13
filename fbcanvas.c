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
#include "file_info.h"

static unsigned short empty_background_color = 0x0000;

static void draw_16bpp(struct fbcanvas *fbc);

static struct framebuffer *open_framebuffer(char *fbdev)
{
	struct framebuffer *fb = malloc(sizeof(*fb));
	if (fb)
	{
		struct fb_var_screeninfo fbinfo;
		int fd = open(fbdev, O_RDWR);
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

		fb->width = fbinfo.xres;
		fb->height = fbinfo.yres;
		fb->depth = fbinfo.bits_per_pixel;

		switch (fb->depth)
		{
			case 16:
				fb->draw = draw_16bpp;
				break;
			default:
				fprintf(stderr, "Unsupported depth: %d\n", fb->depth);
				exit(1);
		}

		fb->mem = mmap(NULL, fb->width * fb->height * (fb->depth / 8),
			PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (fb->mem == MAP_FAILED)
		{
			/* TODO: käsittele virhe */
			perror("mmap");
		}

		close(fd);
	}

	return fb;
}

static void draw_16bpp(struct fbcanvas *fbc);

static void fbcanvas_scroll(struct fbcanvas *fbc, int dx, int dy)
{
	struct framebuffer *fb = fbc->fb;

	fbc->xoffset += dx;
	fbc->yoffset += dy;

	if (fbc->xoffset >= 0)
	{
		if (fbc->xoffset >= (int)fbc->width)
			fbc->xoffset = fbc->width - 1;
	} else {
		if (-fbc->xoffset >= fb->width)
			fbc->xoffset = -(fb->width - 1);
	}

	if (fbc->yoffset >= 0)
	{
		if (fbc->yoffset >= (int)fbc->height)
			fbc->yoffset = fbc->height - 1;
	} else {
		if (-fbc->yoffset >= fb->height)
			fbc->yoffset = -(fb->height - 1);
	}
}

struct fbcanvas *fbcanvas_create(char *filename)
{
	struct file_info *fi;
	GError *err = NULL;
	struct fbcanvas *fbc = malloc(sizeof(*fbc));
	if (fbc)
	{
		g_type_init();

		/* TODO: tarkista onnistuminen */
		fbc->fb = open_framebuffer("/dev/fb0");

		/* Alustetaan yleiset kentät. */
		fbc->context = NULL;
		fbc->document = NULL;
		fbc->page = NULL;
		fbc->gdkpixbuf = NULL;
		fbc->xoffset = 0;
		fbc->yoffset = 0;
		fbc->scale = 1.0;
		fbc->pagenum = 0;
		fbc->pagecount = 1;
		fbc->filename = strdup(filename);

		/* Asetetaan yleiset metodit. */
		fbc->scroll = fbcanvas_scroll;

		/*
		 * Tunnistetaan tiedostotyyppi ja asetetaan sille oikeat käsittelymetodit.
		 */
		fi = get_file_info (filename);
		if (! fi)
			exit (1);
		fbc->ops = fi->ops;
		fbc->ops->open(fbc, filename);
	}

	return fbc;
}

void fbcanvas_destroy(struct fbcanvas *fbc)
{
	struct framebuffer *fb = fbc->fb;

	/* Free file-type-specific fields */
	if (fbc->ops->close)
		fbc->ops->close(fbc);

	/* Free common fields */
	if (fbc->filename)
		free(fbc->filename);
	if (fbc->gdkpixbuf)
		g_object_unref(fbc->gdkpixbuf);

	/* Unmap framebuffer */
	munmap(fb->mem, fb->width * fb->height * (fb->depth / 8));

	free(fbc->fb);
	free(fbc);
}

static void draw_16bpp(struct fbcanvas *fbc)
{
	unsigned int x, y;
	unsigned short *src, *dst;
	unsigned short color;
	unsigned char *data = gdk_pixbuf_get_pixels(fbc->gdkpixbuf);
	struct framebuffer *fb = fbc->fb;

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
		if (y >= fb->height)
			break;

		for (x = 0;; x++)
		{
			/* Framebufferin reuna tuli vastaan - lopetetaan. */
			if (x >= fb->width)
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
				src = &empty_background_color;
			}

			dst = (unsigned short *)fb->mem +
				y * fb->width + x;
			*dst = *src;
		}
	}
}

