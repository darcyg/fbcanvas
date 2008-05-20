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

static void draw_16bpp(struct fbcanvas *fbc);

struct fbcanvas *fbcanvas_create(char *filename)
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

		fbc->filename = strdup(filename);
		fbc->gdkpixbuf = NULL;
		fbc->xoffset = 0;
		fbc->yoffset = 0;
		fbc->scale = 1.0;
		fbc->pagenum = 0;
		fbc->pagecount = -1;

		switch (framebuffer.bpp)
		{
			case 16:
				fbc->draw = draw_16bpp;
				break;
			default:
				fprintf(stderr, "Unsupported depth: %d\n", framebuffer.bpp);
				exit(1);
		}
	}

	return fbc;
}

void fbcanvas_free(struct fbcanvas *fbc)
{
	g_object_unref(fbc->gdkpixbuf);
	fbc->gdkpixbuf = NULL;
}

void fbcanvas_destroy(struct fbcanvas *fbc)
{
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

			if ((y < height - fbc->yoffset) && (x < width - fbc->xoffset))
			{
				unsigned char *tmp = data +
					4*width * (fbc->yoffset + y) +
					4*(fbc->xoffset + x);

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
				src = &empty;
			}

			dst = (unsigned short *)framebuffer.mem + framebuffer.width * y + x;
			*dst = *src;
		}
	}
}

