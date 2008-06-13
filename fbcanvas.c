/*
 * fbcanvas.c - 17.5.2008 - 20.5.2008 Ari & Tero Roponen
 */
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

static void draw_16bpp(struct fbcanvas *fbc);

extern struct file_info bmp_info;
extern struct file_info djvu_info;
extern struct file_info gif_info;
extern struct file_info jpg_info;
extern struct file_info pcx_info;
extern struct file_info pdf_info;
extern struct file_info png_info;
extern struct file_info ppm_info;
extern struct file_info tiff_info;
extern struct file_info xpm_info;

static struct file_info *file_info[] =
{
	&bmp_info,
	&djvu_info,
	&gif_info,
	&jpg_info,
	&pcx_info,
	&pdf_info,
	&png_info,
	&ppm_info,
	&tiff_info,
	&xpm_info,
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

		switch (fbc->hwdepth)
		{
			case 16:
				fbc->draw = draw_16bpp;
				break;
			default:
				fprintf(stderr, "Unsupported depth: %d\n", fbc->hwdepth);
				exit(1);
		}

		/*
		 * Tunnistetaan tiedostotyyppi ja asetetaan sille oikeat käsittelymetodit.
		 */
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
			for (i = 0; file_info[i]; i++)
			{
				if (!strncmp(type, file_info[i]->type, strlen(file_info[i]->type)))
				{
					magic_close(magic);
					fbc->ops = file_info[i]->ops;
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
		fbc->ops->open(fbc, filename);
	}

	return fbc;
}

void fbcanvas_destroy(struct fbcanvas *fbc)
{
	/* Free file-type-specific fields */
	if (fbc->ops->close)
		fbc->ops->close(fbc);

	/* Free common fields */
	if (fbc->filename)
		free(fbc->filename);
	if (fbc->gdkpixbuf)
		g_object_unref(fbc->gdkpixbuf);

	/* Unmap framebuffer */
	munmap(fbc->hwmem, fbc->hwwidth * fbc->hwheight * (fbc->hwdepth / 8));

	free(fbc);
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

