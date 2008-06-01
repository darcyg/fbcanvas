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

static void open_pdf(struct fbcanvas *fbc, char *filename)
{
	GError *err = NULL;
	char fullname[256];
	char *canon_name = canonicalize_file_name(filename);

	/* PDF vaatii absoluuttisen "file:///tiedostonimen". */
	sprintf(fullname, "file://%s", canon_name);

	fbc->page = NULL;
	fbc->document = poppler_document_new_from_file(fullname, NULL, &err);
	if (!fbc->document)
	{
		fprintf(stderr, "open_pdf: %s\n", err->message);
		exit(1);
	}

	free(canon_name);
	fbc->pagecount = poppler_document_get_n_pages(fbc->document);
}

static void close_pdf(struct fbcanvas *fbc)
{
	if (fbc->page)
		g_object_unref(fbc->page);
	if (fbc->document)
		g_object_unref(fbc->document);
	if (fbc->gdkpixbuf)
		g_object_unref(fbc->gdkpixbuf);
	fbc->page = NULL;
	fbc->document = NULL;
	fbc->gdkpixbuf = NULL;
}

static int grep_pdf(struct fbcanvas *fbc, char *regexp)
{
	/* TODO: use real regexps. */
	int i, ret = 1, len = strlen (regexp);
	char *str, *beg, *end;

	/* Set up methods & canvas size. */
	fbc->update (fbc);

	if (!fbc->page)
	{
		fprintf (stderr, "%s",
			 "Grepping is only implemented for PDF-files\n");
		return 1;
	}

	for (i = 0; i < fbc->pagecount; i++)
	{
		PopplerRectangle rec = {0, 0, fbc->width, fbc->height};
		fbc->page = poppler_document_get_page (fbc->document, i);
		str = poppler_page_get_text (fbc->page, POPPLER_SELECTION_LINE, &rec);

		while (str)
		{
			beg = strstr (str, regexp);
			end = beg + len;

			if (beg)
			{
				ret = 0; /* found match */

				/* try to find line beginning and end. */
				while (beg < str && beg[-1] != '\n')
					beg--;
				while (*end && *end != '\n')
					end++;

				printf ("%s:%d: %.*s\n", fbc->filename, i + 1, end - beg, beg);
				str = end;
			} else str = NULL;
		}
	}

	return ret;
}

static struct
{
	char *type;
	void (*open)(struct fbcanvas *fbc, char *filename);
	void (*close)(struct fbcanvas *fbc);
	void (*update)(struct fbcanvas *fbc);

	int (*grep)(struct fbcanvas *fbc, char *regexp);
} file_ops[] = {
	{
		.type = "PC bitmap",
		.open = open_image,
		.close = close_image,
		.update = update_image,
	}, {
		.type = "PCX",
		.open = open_image,
		.close = close_image,
		.update = update_image,
	}, {
		.type = "GIF image",
		.open = open_image,
		.close = close_image,
		.update = update_image,
	}, {
		.type = "JPEG",
		.open = open_image,
		.close = close_image,
		.update = update_image,
	}, {
		.type = "PDF",
		.open = open_pdf,
		.close = close_pdf,
		.update = update_pdf,
		.grep = grep_pdf,
	}, {
		.type = "PNG",
		.open = open_image,
		.close = close_image,
		.update = update_image,
	}, {
		.type = "Netpbm PPM",
		.open = open_image,
		.close = close_image,
		.update = update_image,
	}, {
		.type = "TIFF image data",
		.open = open_image,
		.close = close_image,
		.update = update_image,
	}, {
		.type = "X pixmap image text",
		.open = open_image,
		.close = close_image,
		.update = update_image,
	}, {
		NULL
	}
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
			for (i = 0; file_ops[i].type; i++)
			{
				if (!strncmp(type, file_ops[i].type, strlen(file_ops[i].type)))
				{
					magic_close(magic);
					fbc->open = file_ops[i].open;
					fbc->close = file_ops[i].close;
					fbc->update = file_ops[i].update;
					fbc->grep = file_ops[i].grep;
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

