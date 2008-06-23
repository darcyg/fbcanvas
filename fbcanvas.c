/*
 * fbcanvas.c - 17.5.2008 - 19.6.2008 Ari & Tero Roponen
 */
#include <sys/types.h>
#include <errno.h>
#include <linux/fb.h>
#include <linux/vt.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fbcanvas.h"
#include "file_info.h"

static void draw_16bpp(struct framebuffer *fb, unsigned char *data);

static struct framebuffer *open_framebuffer(char *fbdev)
{
	struct framebuffer *fb = malloc(sizeof(*fb));
	if (fb)
	{
		struct fb_var_screeninfo fbinfo;
		int fd = open(fbdev, O_RDWR);
		if (fd < 0)
		{
			/* TODO: käsittele virhe */
			fprintf(stderr, "Could not open %s: %s\n",
				fbdev, strerror(errno));
			free(fb);
			fb = NULL;
			goto out;
		}

		if (ioctl(fd, FBIOGET_VSCREENINFO, &fbinfo) < 0)
		{
			/* TODO: käsittele virhe */
			perror("ioctl");
		}

		fb->width = fbinfo.xres;
		fb->height = fbinfo.yres;
		fb->depth = fbinfo.bits_per_pixel;

		switch (fb->depth)
		{
			case 16:
				fb->draw = draw_16bpp;
				break;
			default:
				fprintf(stderr, "Unsupported depth: %d\n", fb->depth);
				exit(1);
		}

		fb->mem = mmap(NULL, fb->width * fb->height * (fb->depth / 8),
			PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (fb->mem == MAP_FAILED)
		{
			/* TODO: käsittele virhe */
			perror("mmap");
		}

		close(fd);

		/* Asetetaan konsolinvaihto lähettämään signaaleja */
		fd = open("/dev/tty", O_RDWR);
		if (fd >= 0)
		{
			struct vt_mode vt_mode;
			ioctl(fd, VT_GETMODE, &vt_mode);

			vt_mode.mode = VT_PROCESS; /* Tämä prosessi hoitaa vaihdot. */
			vt_mode.waitv = 0;
			vt_mode.relsig = SIGUSR1;
			vt_mode.acqsig = SIGUSR2;
			ioctl(fd, VT_SETMODE, &vt_mode);
			close(fd);

			extern void handle_signal(int s);
			signal(SIGUSR1, handle_signal);
			signal(SIGUSR2, handle_signal);
		}
	}
out:
	return fb;
}

static void fbcanvas_scroll(struct document *doc, int dx, int dy)
{
	struct framebuffer *fb = doc->fbcanvas->fb;

	doc->xoffset += dx;
	doc->yoffset += dy;

	if (doc->xoffset >= 0)
	{
		if (doc->xoffset >= (int)doc->width)
			doc->xoffset = doc->width - 1;
	} else {
		if (-doc->xoffset >= fb->width)
			doc->xoffset = -(fb->width - 1);
	}

	if (doc->yoffset >= 0)
	{
		if (doc->yoffset >= (int)doc->height)
			doc->yoffset = doc->height - 1;
	} else {
		if (-doc->yoffset >= fb->height)
			doc->yoffset = -(fb->height - 1);
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
		fbc->fb = open_framebuffer("/dev/fb0");
		if (!fbc->fb)
		{
			free(fbc);
			fbc = NULL;
			goto out;
		}

		/* Asetetaan yleiset metodit. */
		fbc->scroll = fbcanvas_scroll;
	}

out:
	return fbc;
}

void fbcanvas_destroy(struct fbcanvas *fbc)
{
	struct framebuffer *fb = fbc->fb;

	/* Unmap framebuffer */
	munmap(fb->mem, fb->width * fb->height * (fb->depth / 8));

	free(fbc->fb);
	free(fbc);
}

/* rgba 5/11, 6/5, 5/0, 0/0 */
static void draw_16bpp(struct framebuffer *fb, unsigned char *data)
{
	for (int y = 0; y < fb->height; y++)
	{
		for (int x = 0; x < fb->width; x++)
		{
			unsigned char *tmp = data + 4 * (fb->width * y + x);

			unsigned short color =  (((1<<5) * tmp[0] / 256) & ((1<<5)-1)) << 11 |
						(((1<<6) * tmp[1] / 256) & ((1<<6)-1)) << 5 |
						(((1<<5) * tmp[2] / 256) & ((1<<5)-1)) << 0;

			*((unsigned short *)fb->mem + y * fb->width + x) = color;
		}
	}
}

