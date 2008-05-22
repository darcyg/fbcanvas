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

static void update_image(struct fbcanvas *fbc);
static void update_pdf(struct fbcanvas *fbc);
static void draw_16bpp(struct fbcanvas *fbc);

static void open_image(struct fbcanvas *fbc, char *filename)
{
	//fprintf(stderr, "open_image: %s\n", filename);
}

static void open_pdf(struct fbcanvas *fbc, char *filename)
{
	GError *err = NULL;
	char fullname[256];
	char *canon_name = canonicalize_file_name(filename);

	/* PDF vaatii absoluuttisen "file:///tiedostonimen". */
	sprintf(fullname, "file://%s", canon_name);

	fbc->document = poppler_document_new_from_file(fullname, NULL, &err);
	if (!fbc->document)
	{
		fprintf(stderr, "open_pdf: %s\n", err->message);
		exit(1);
	}

	free(canon_name);
	fbc->pagecount = poppler_document_get_n_pages(fbc->document);
}

static struct
{
	char *type;
	void (*open)(struct fbcanvas *fbc, char *filename);
	void (*update)(struct fbcanvas *fbc);
} file_ops[] = {
	{
		.type = "PC bitmap",
		.open = open_image,
		.update = update_image,
	}, {
		.type = "PCX",
		.open = open_image,
		.update = update_image,
	}, {
		.type = "GIF image",
		.open = open_image,
		.update = update_image,
	}, {
		.type = "JPEG",
		.open = open_image,
		.update = update_image,
	}, {
		.type = "PDF",
		.open = open_pdf,
		.update = update_pdf,
	}, {
		.type = "PNG",
		.open = open_image,
		.update = update_image,
	}, {
		.type = "Netpbm PPM",
		.open = open_image,
		.update = update_image,
	}, {
		.type = "TIFF image data",
		.open = open_image,
		.update = update_image,
	}, {
		.type = "X pixmap image text",
		.open = open_image,
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
		if (-fbc->xoffset >= framebuffer.width)
			fbc->xoffset = -(framebuffer.width - 1);
	}

	if (fbc->yoffset >= 0)
	{
		if (fbc->yoffset >= (int)fbc->height)
			fbc->yoffset = fbc->height - 1;
	} else {
		if (-fbc->yoffset >= framebuffer.height)
			fbc->yoffset = -(framebuffer.height - 1);
	}
}

struct fbcanvas *fbcanvas_create(char *filename)
{
	GError *err = NULL;
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

			//printf("%d x %d x %d\n", framebuffer.width,
			//	framebuffer.height, framebuffer.bpp);

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

		g_type_init();

		fbc->scroll = fbcanvas_scroll;
		fbc->page = NULL;
		fbc->filename = strdup(filename);
		fbc->gdkpixbuf = NULL;
		fbc->xoffset = 0;
		fbc->yoffset = 0;
		fbc->scale = 1.0;
		fbc->pagenum = 0;

		switch (framebuffer.bpp)
		{
			case 16:
				fbc->draw = draw_16bpp;
				break;
			default:
				fprintf(stderr, "Unsupported depth: %d\n", framebuffer.bpp);
				exit(1);
		}

		/* Determine file type */
		{
			magic_t magic = magic_open(MAGIC_NONE);
			int i, ret = magic_load(magic, NULL);
			const char *type = magic_file(magic, filename);

			/* Try to identify file by its header */
			for (i = 0; file_ops[i].type; i++)
			{
				if (!strncmp(type, file_ops[i].type, strlen(file_ops[i].type)))
				{
					magic_close(magic);
					fbc->open = file_ops[i].open;
					fbc->update = file_ops[i].update;
					goto type_ok;
				}
			}

			/* TODO: Try to identify file by its extension */

			fprintf(stderr, "Unsupported file type: %s\n", type);
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
	if (fbc->page)
		g_object_unref(fbc->page);
	if (fbc->document)
		g_object_unref(fbc->document);
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
	fbc->width = gdk_pixbuf_get_width(fbc->gdkpixbuf);
	fbc->height = gdk_pixbuf_get_height(fbc->gdkpixbuf);

	if (fbc->scale != 1.0)
	{
		GdkPixbuf *tmp = gdk_pixbuf_scale_simple(fbc->gdkpixbuf,
			ceil(fbc->scale * fbc->width),
			ceil(fbc->scale * fbc->height),
			GDK_INTERP_BILINEAR);
		g_object_unref(fbc->gdkpixbuf);
		fbc->gdkpixbuf = tmp;
	}
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
		if (y >= framebuffer.height)
			break;

		for (x = 0;; x++)
		{
			/* Framebufferin reuna tuli vastaan - lopetetaan. */
			if (x >= framebuffer.width)
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
				src = &empty;
			}

			dst = (unsigned short *)framebuffer.mem +
				y * framebuffer.width + x;
			*dst = *src;
		}
	}
}

