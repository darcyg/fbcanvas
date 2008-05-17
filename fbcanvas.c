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
	char *mem;
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
			}

			if (ioctl(fd, FBIOGET_VSCREENINFO, &fbinfo) < 0)
			{
				/* TODO: käsittele virhe */
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
			}

			close(fd);

			framebuffer.mem = mem;
			framebuffer.refcount++;
		}

		fbc->width = width;
		fbc->height = height;
		fbc->buffer = malloc(width * height * (framebuffer.bpp / 8));
		/* TODO: tarkista onnistuminen */
	}

	return fbc;
}

void fbcanvas_destroy(struct fbcanvas *fbc)
{
	free(fbc->buffer);
	free(fbc);

	framebuffer.refcount--;

	if (framebuffer.refcount == 0)
	{
		munmap(framebuffer.mem,
			framebuffer.width * framebuffer.height * (framebuffer.bpp / 8));
	}
}

void fbcanvas_draw(struct fbcanvas *fbc)
{
	/* TODO: piirtäminen tehdään täällä. Ota huomioon offsetit ym. */
}
