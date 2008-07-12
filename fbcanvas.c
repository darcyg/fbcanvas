/*
 * fbcanvas.c - 17.5.2008 - 12.7.2008 Ari & Tero Roponen
 */
#include <sys/ioctl.h>
#include <sys/types.h>
#include <errno.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "document.h"
#include "file_info.h"

static void draw_16bpp(struct backend *be, cairo_surface_t *surface);

struct backend fb_backend;

struct framebuffer
{
	unsigned char *mem;	/* mmap()ed framebuffer memory  */
	unsigned int depth;     /* Hardware color depth         */
};

static struct backend *open_fb(char *filename)
{
	struct framebuffer *fb;
	struct backend *be = NULL;
	struct fb_var_screeninfo fbinfo;

	int fd = open("/dev/fb0", O_RDWR);
	if (fd < 0)
	{
		fprintf(stderr, "Could not open /dev/fb0: %s\n", strerror(errno));
		goto out;
	}

	fb = malloc(sizeof(*fb));
	if (!fb)
		goto out;

	be = &fb_backend;
	be->data = fb;

	if (ioctl(fd, FBIOGET_VSCREENINFO, &fbinfo) < 0)
	{
		/* TODO: käsittele virhe */
		perror("ioctl");
	}

	be->width = fbinfo.xres;
	be->height = fbinfo.yres;
	fb->depth = fbinfo.bits_per_pixel;

	switch (fb->depth)
	{
		case 16:
			be->draw = draw_16bpp;
			break;
		default:
			fprintf(stderr, "Unsupported depth: %d\n", fb->depth);
			exit(1);
	}

	fb->mem = mmap(NULL, be->width * be->height * (fb->depth / 8),
		PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (fb->mem == MAP_FAILED)
	{
		/* TODO: käsittele virhe */
		perror("mmap");
	}

	close(fd);

	be->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, be->width, be->height);

out:
	return be;
}

static void close_fb(struct backend *be)
{
	struct framebuffer *fb = be->data;

	cairo_surface_destroy (be->surface);

	/* Unmap framebuffer */
	munmap(fb->mem, be->width * be->height * (fb->depth / 8));
	free(fb);
}

/* rgba 5/11, 6/5, 5/0, 0/0 */
static void draw_16bpp(struct backend *be, cairo_surface_t *surface)
{
	struct framebuffer *fb = be->data;
	unsigned int *data = (unsigned int *)cairo_image_surface_get_data(surface);
	const unsigned int width = be->width;
	const unsigned int height = be->height;
	unsigned short vline[be->width];

#define Red(x)   (((x) >> 0)  & 0xFF)
#define Green(x) (((x) >> 8)  & 0xFF)
#define Blue(x)  (((x) >> 16) & 0xFF)
#define Alpha(x) (((x) >> 24) & 0xFF)
#define Scale(x, from, to) ((1 << (to)) * (x) / (1 << (from)))

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			unsigned int val = *(data + width * y + x);

			vline[x] = ((Scale(Red(val), 8, 5) << 11) |
				    (Scale(Green(val), 8, 6) << 5) |
				    (Scale(Blue(val), 8, 5)));
		}

		memcpy(fb->mem + 2 * width * y, vline, 2 * width);
	}
}

struct backend fb_backend =
{
	.open = open_fb,
	.close = close_fb,
};
