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

void fbcanvas_draw(struct fbcanvas *fbc)
{
	if (fbc->bpp == 8)
		draw_8bpp(fbc);
	else if (fbc->bpp == 16)
		draw_16bpp(fbc);
	else if (fbc->bpp == 24)
		draw_24bpp(fbc);
	else
		printf("Unimplemented depth: %d\n", fbc->bpp);
}
