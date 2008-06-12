/*
 * fbcanvas.c - 17.5.2008 - 20.5.2008 Ari & Tero Roponen
 */
#define _GNU_SOURCE
#include <poppler/glib/poppler.h>
#include <sys/types.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <magic.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fbcanvas.h"

static unsigned short empty_background_color = 0x0000;

void open_framebuffer(struct fbcanvas *fbc, char *fbdev)
{
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

	fbc->hwwidth = fbinfo.xres;
	fbc->hwheight = fbinfo.yres;
	fbc->hwdepth = fbinfo.bits_per_pixel;

	fbc->hwmem = mmap(NULL, fbc->hwwidth * fbc->hwheight * (fbc->hwdepth / 8),
			  PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (fbc->hwmem == MAP_FAILED)
	{
		/* TODO: käsittele virhe */
		perror("mmap");
	}

	close(fd);
}

static void update_image(struct fbcanvas *fbc);
static void update_pdf(struct fbcanvas *fbc);
static void draw_16bpp(struct fbcanvas *fbc);

static void open_image(struct fbcanvas *fbc, char *filename)
{
	//fprintf(stderr, "open_image: %s\n", filename);
	fbc->page = NULL;
	fbc->pagecount = 1;
}

static void close_image(struct fbcanvas *fbc)
{
	if (fbc->gdkpixbuf)
		g_object_unref(fbc->gdkpixbuf);
	fbc->gdkpixbuf = NULL;
}

extern struct file_ops pdf_ops;

/* TODO: siirrä nämä oikeisiin tiedostoihin */
static struct file_ops bmp_ops =
{
	.type = "PC bitmap",
	.open = open_image,
	.close = close_image,
	.update = update_image,
};

static struct file_ops gif_ops =
{
	.type = "GIF image data",
	.open = open_image,
	.close = close_image,
	.update = update_image,
};

static struct file_ops jpg_ops =
{
	.type = "JPEG",
	.open = open_image,
	.close = close_image,
	.update = update_image,
};

static struct file_ops pcx_ops =
{
	.type = "PCX",
	.open = open_image,
	.close = close_image,
	.update = update_image,
};

static struct file_ops png_ops =
{
	.type = "PNG",
	.open = open_image,
	.close = close_image,
	.update = update_image,
};

static struct file_ops ppm_ops =
{
	.type = "Netpbm PPM",
	.open = open_image,
	.close = close_image,
	.update = update_image,
};

static struct file_ops tiff_ops =
{
	.type = "TIFF image data",
	.open = open_image,
	.close = close_image,
	.update = update_image,
};

static struct file_ops xpm_ops =
{
	.type = "X pixmap image text",
	.open = open_image,
	.close = close_image,
	.update = update_image,
};

static struct file_ops *file_ops[] =
{
	&bmp_ops,
	&gif_ops,
	&jpg_ops,
	&pcx_ops,
	&pdf_ops,
	&png_ops,
	&ppm_ops,
	&tiff_ops,
	&xpm_ops,
	NULL
};

static void fbcanvas_scroll(struct fbcanvas *fbc, int dx, int dy)
{
	/* TODO: tarkista ettei mennä reunusten ohi */
	fbc->xoffset += dx;
	fbc->yoffset += dy;

	if (fbc->xoffset >= 0)
	{
		if (fbc->xoffset >= (int)fbc->width)
			fbc->xoffset = fbc->width - 1;
	} else {
		if (-fbc->xoffset >= fbc->hwwidth)
			fbc->xoffset = -(fbc->hwwidth - 1);
	}

	if (fbc->yoffset >= 0)
	{
		if (fbc->yoffset >= (int)fbc->height)
			fbc->yoffset = fbc->height - 1;
	} else {
		if (-fbc->yoffset >= fbc->hwheight)
			fbc->yoffset = -(fbc->hwheight - 1);
	}
}

struct fbcanvas *fbcanvas_create(char *filename)
{
	GError *err = NULL;
	struct fbcanvas *fbc = malloc(sizeof(*fbc));
	if (fbc)
	{
		g_type_init();

		/* TODO: tarkista onnistuminen */
		open_framebuffer(fbc, "/dev/fb0");

		/* Set common fields */
		fbc->scroll = fbcanvas_scroll;
		fbc->filename = strdup(filename);
		fbc->gdkpixbuf = NULL;
		fbc->xoffset = 0;
		fbc->yoffset = 0;
		fbc->scale = 1.0;
		fbc->pagenum = 0;

		switch (fbc->hwdepth)
		{
			case 16:
				fbc->draw = draw_16bpp;
				break;
			default:
				fprintf(stderr, "Unsupported depth: %d\n", fbc->hwdepth);
				exit(1);
		}

		/* Determine file type */
		{
			magic_t magic = magic_open(MAGIC_NONE);
			int i, ret = magic_load(magic, NULL);
			const char *type = magic_file(magic, filename);
			if (!type)
			{
				fprintf(stderr, "Could not determine file type: %s\n", filename);
				goto unknown;
			}

			/* Try to identify file by its header */
			for (i = 0; file_ops[i]; i++)
			{
				if (!strncmp(type, file_ops[i]->type, strlen(file_ops[i]->type)))
				{
					magic_close(magic);
					/* TODO: fbc->ops = file_ops[i]->ops */
					fbc->open = file_ops[i]->open;
					fbc->close = file_ops[i]->close;
					fbc->update = file_ops[i]->update;
					fbc->grep = file_ops[i]->grep;
					goto type_ok;
				}
			}

			/* TODO: Try to identify file by its extension */
			fprintf(stderr, "Unsupported file type: %s\n", type);
unknown:
			magic_close(magic);
			exit(1);
		}

type_ok:
		fbc->open(fbc, filename);
	}

	return fbc;
}

void fbcanvas_destroy(struct fbcanvas *fbc)
{
	/* Free file-type-specific fields */
	if (fbc->close)
		fbc->close(fbc);

	/* Free common fields */
	if (fbc->filename)
		free(fbc->filename);

	/* Unmap framebuffer */
	munmap(fbc->hwmem, fbc->hwwidth * fbc->hwheight * (fbc->hwdepth / 8));

	free(fbc);
}

static void update_image(struct fbcanvas *fbc)
{
	GError *err = NULL;
	fbc->gdkpixbuf = gdk_pixbuf_new_from_file(fbc->filename, &err);
	if (err)
	{
		fprintf (stderr, "%s", err->message);
		exit(1);
	}

	fbc->gdkpixbuf = gdk_pixbuf_add_alpha(fbc->gdkpixbuf, FALSE, 0, 0, 0);

	if (fbc->scale != 1.0)
	{
		GdkPixbuf *tmp = gdk_pixbuf_scale_simple(fbc->gdkpixbuf,
			ceil(fbc->scale * gdk_pixbuf_get_width(fbc->gdkpixbuf)),
			ceil(fbc->scale * gdk_pixbuf_get_height(fbc->gdkpixbuf)),
			GDK_INTERP_BILINEAR);
		g_object_unref(fbc->gdkpixbuf);
		fbc->gdkpixbuf = tmp;
	}

	fbc->width = gdk_pixbuf_get_width(fbc->gdkpixbuf);
	fbc->height = gdk_pixbuf_get_height(fbc->gdkpixbuf);
}

static void draw_16bpp(struct fbcanvas *fbc)
{
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
		if (y >= fbc->hwheight)
			break;

		for (x = 0;; x++)
		{
			/* Framebufferin reuna tuli vastaan - lopetetaan. */
			if (x >= fbc->hwwidth)
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

			dst = (unsigned short *)fbc->hwmem +
				y * fbc->hwwidth + x;
			*dst = *src;
		}
	}
}

