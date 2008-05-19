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
static void data16_from_pixbuf(struct fbcanvas *fbc, GdkPixbuf *gdkpixbuf, int w, int h);

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


static void data16_from_pixbuf(struct fbcanvas *fbc, GdkPixbuf *gdkpixbuf, int w, int h)
{
	unsigned char *src = gdk_pixbuf_get_pixels(gdkpixbuf);
	unsigned char *dst;
	int i, j;

	if (fbc->buffer)
	{
		if ((w != fbc->width) || (h != fbc->height))
		{
			free (fbc->buffer);
			fbc->buffer = malloc(w * h * framebuffer.bpp / 8);
			fbc->width = w;
			fbc->height = h;
		}
	}

	dst = fbc->buffer;

	/* TODO: Tämä toimii vain 16-bittisellä framebufferilla. */
	for (j = 0; j < h; j++)
	{
		for (i = 0; i < w; i++)
		{
			unsigned char red = *src++;
			unsigned char green = *src++;
			unsigned char blue = *src++;
			unsigned char alpha = *src++;

			unsigned short color = 0;

			color |= ((32 * red / 256) & 0b00011111) << 11;
			color |= ((64 * green / 256) & 0b00111111) << 5;
			color |= ((32 * blue / 256) & 0b00011111) << 0;

			*(dst+0) = (color & 0x00FF) >> 0;
			*(dst+1) = (color & 0xFF00) >> 8;
			dst += 2;
		}
	}
}
