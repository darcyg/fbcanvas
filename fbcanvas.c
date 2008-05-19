/*
 * fbcanvas.c - 17.5.2008 - 17.5.2008 Ari & Tero Roponen
 */
#include <sys/types.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
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

static void draw_8bpp(struct fbcanvas *fbc);
static void draw_16bpp(struct fbcanvas *fbc);
static void draw_24bpp(struct fbcanvas *fbc);
static void data16_from_pixbuf(struct fbcanvas *fbc, GdkPixbuf *gdkpixbuf);

struct fbcanvas *fbcanvas_create(int width, int height)
{
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

		fbc->width = width;
		fbc->height = height;
		fbc->xoffset = 0;
		fbc->yoffset = 0;

		switch (framebuffer.bpp)
		{
			case 8:
				fbc->draw = draw_8bpp;
				break;
			case 16:
				fbc->data_from_pixbuf = data16_from_pixbuf;
				fbc->draw = draw_16bpp;
				break;
			case 24:
				fbc->draw = draw_24bpp;
			default:
				fprintf(stderr, "Unsupported depth: %d\n", framebuffer.bpp);
				exit(1);
		}

		fbc->bpp = framebuffer.bpp;
		fbc->buffer = malloc(width * height * (fbc->bpp / 8));
		/* TODO: tarkista onnistuminen */
	}

	return fbc;
}

void fbcanvas_free(struct fbcanvas *fbc)
{
	if (fbc->buffer)
		free(fbc->buffer);
	fbc->buffer = NULL;
}

void fbcanvas_destroy(struct fbcanvas *fbc)
{
	if (fbc->buffer)
		free(fbc->buffer);
	free(fbc);

	framebuffer.refcount--;

	if (framebuffer.refcount == 0)
	{
		munmap(framebuffer.mem,
			framebuffer.width * framebuffer.height * (framebuffer.bpp / 8));
	}
}

static void draw_8bpp(struct fbcanvas *fbc)
{
	/* TODO: piirtäminen tehdään täällä. Ota huomioon offsetit ym.
	 * TODO: tarkista ettei mennä framebufferin rajojen yli.
	 */
	unsigned int i, j;
	unsigned char *src, *dst;

	for (j = 0; j < fbc->height - fbc->yoffset; j++)
	{
		for (i = 0; i < fbc->width - fbc->xoffset; i++)
		{
			src = (unsigned char *)fbc->buffer + fbc->width * (fbc->yoffset + j) +
				fbc->xoffset + i;
			dst = (unsigned char *)framebuffer.mem + framebuffer.width * j + i;
			*dst = *src;
		}
	}
}

static void draw_16bpp(struct fbcanvas *fbc)
{
	/* TODO: piirtäminen tehdään täällä. Ota huomioon offsetit ym.
	 * TODO: tarkista ettei mennä framebufferin rajojen yli.
	 */
	static unsigned short empty = 0x0000;
	unsigned int i, j;
	unsigned short *src, *dst;

	for (j = 0;; j++)
	{
		/* Framebufferin reuna tuli vastaan - lopetetaan. */
		if (j >= framebuffer.height)
			break;

		for (i = 0;; i++)
		{
			/* Framebufferin reuna tuli vastaan - lopetetaan. */
			if (i >= framebuffer.width)
				break;

			if ((j < fbc->height - fbc->yoffset) && (i < fbc->width - fbc->xoffset))
			{
				src = (unsigned short *)fbc->buffer +
					fbc->width * (fbc->yoffset + j) +
					fbc->xoffset + i;
			} else {
				src = &empty;
			}

			dst = (unsigned short *)framebuffer.mem + framebuffer.width * j + i;
			*dst = *src;
		}
	}
}

static void draw_24bpp(struct fbcanvas *fbc)
{
	/* TODO: piirtäminen tehdään täällä. Ota huomioon offsetit ym.
	 * TODO: tarkista ettei mennä framebufferin rajojen yli.
	 */
	unsigned int i, j;
	unsigned int *src, *dst;

	for (j = 0; j < fbc->height - fbc->yoffset; j++)
	{
		for (i = 0; i < fbc->width - fbc->xoffset; i++)
		{
			src = (unsigned int *)fbc->buffer + fbc->width * (fbc->yoffset + j) +
				fbc->xoffset + i;
			dst = (unsigned int *)framebuffer.mem + framebuffer.width * j + i;
			*dst = (*src & 0xFFFFFF00);
		}
	}
}


static void data16_from_pixbuf(struct fbcanvas *fbc, GdkPixbuf *gdkpixbuf)
{
	unsigned char *src = gdk_pixbuf_get_pixels(gdkpixbuf);
	unsigned char *dst = fbc->buffer;
	int i, j;

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
			*(dst+1) = (((green * 64 / 256) << 5) & 0b11100000) |
				   ((blue * 32 / 256) & 0b00011111);
			dst += 2;
		}
	}
}
