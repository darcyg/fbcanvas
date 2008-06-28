#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>

#include "commands.h"
#include "fbcanvas.h"

static Display *display;
static Window win;
static int screen;
static GC gc;

static struct document *hack;

void x11_main_loop(struct document *doc)
{
	XEvent report;

	hack = doc;

	doc->draw(doc);

	for (;;)
	{

		XNextEvent(display, &report);

		switch (report.type)
		{
			case Expose:
				while (XCheckTypedEvent(display, Expose, &report))
					;
				doc->draw(doc);
				break;
			case ConfigureNotify:
				break;
			case ButtonPress:
				break;

			case KeyPress:
			{
				command_t cmd;
				XKeyEvent xkey = report.xkey;
				int key = 0;

				switch (xkey.keycode)
				{
					case 9:  /* ESC */
					case 24: /* 'q' */
						XFreeGC(display, gc);
						XCloseDisplay(display);
						goto out;

					case 99: key = KEY_PPAGE; break;
					case 105: key = KEY_NPAGE; break;
				}

				if (key)
				{
					cmd = lookup_command(key);
					cmd(hack);
					doc->draw(doc);
					break;
				}

				fprintf(stderr, "Keycode: %d\n", xkey.keycode);
				doc->draw(doc);
				break;
			}

			default:
				break;
		}
	}
out:
	;
}

static void draw_16bpp(struct framebuffer *fb, unsigned char *data)
{
	Visual *visual = XDefaultVisual(display, 0);
#if 0
	visual->red_mask = 0xFF << 0;
	visual->green_mask = 0xFF << 8;
	visual->blue_mask = 0xFF << 16;
#endif

	cairo_pattern_t *img = cairo_pattern_create_for_surface (hack->cairo);
	cairo_surface_t *cs = cairo_xlib_surface_create(display,
		win, visual, fb->width, fb->height);
	cairo_t *cr = cairo_create(cs);

	cairo_set_source (cr, img);
	cairo_paint(cr);

	cairo_pattern_destroy(img);
	cairo_surface_destroy(cs);
	cairo_destroy(cr);
}

struct fbcanvas *x11canvas_create(char *filename)
{
	struct fbcanvas *fbc = malloc(sizeof(*fbc));
	{
		XGCValues values;
		XSizeHints size_hints;
		char *display_name = NULL;
		display = XOpenDisplay(display_name);
		if (!display)
		{
out_free:
			free(fbc);
			fbc = NULL;
			goto out;
		}

		fbc->fb = malloc(sizeof(*fbc->fb));
		if (fbc->fb)
		{
			screen = DefaultScreen(display);

			fbc->fb->width = DisplayWidth(display, screen);
			fbc->fb->height = DisplayHeight(display, screen);
			fbc->fb->depth = DefaultDepth(display, screen);

			switch (fbc->fb->depth)
			{
				case 16:
					fbc->fb->draw = draw_16bpp;
					break;
				default:
					fprintf(stderr, "Unsupported depth: %d\n", fbc->fb->depth);
					exit(1);
			}
		} else goto out_free;

		win = XCreateSimpleWindow(display,
			RootWindow(display, screen),
			0, 0,
			fbc->fb->width,
			fbc->fb->height,
			0, /* Border width */
			BlackPixel(display, screen),
			WhitePixel(display,screen));

		size_hints.flags = PPosition | PSize | PMinSize;
		size_hints.x = 0;
	        size_hints.y = 0;
		size_hints.width = fbc->fb->width;
		size_hints.height = fbc->fb->height;
		size_hints.min_width = 350;
		size_hints.min_height = 250;

		XSetStandardProperties(display, win,
			"fb", NULL, NULL,
			NULL, 0, &size_hints);

		XSelectInput(display, win,
			ExposureMask |
			KeyPressMask |
			ButtonPressMask |
			StructureNotifyMask);

		gc = XCreateGC(display, win, 0, &values);
		XSetForeground(display, gc, BlackPixel(display,screen));
		XSetLineAttributes(display, gc, 1, LineSolid, CapButt, JoinMiter);
		XMapWindow(display, win);
	}
out:
	return fbc;
}

#if 0
int main(int argc, char *argv[])
{
	Window win;
	unsigned int width, height, display_width, display_height;
	int x = 0, y = 0;
	unsigned int border_width = 4;
	char *window_name = "Basic Window Program";

	char *icon_name = "basicwin";
	XSizeHints size_hints;
	XEvent report;
	GC gc;
	XFontStruct *font_info;
	char *display_name = NULL;
	int window_size = 0;

	display = XOpenDisplay(display_name);
	if (!display)
	{
		fprintf(stderr,"basicwin: cannot connect to X server %s\n",
			XDisplayName(display_name));
		exit(-1);
	}

	screen = DefaultScreen(display);

	display_width = DisplayWidth(display,screen);
	display_height = DisplayHeight(display,screen);

	width = display_width / 3;
	height = display_height / 4;

	win = XCreateSimpleWindow(display,
		RootWindow(display, screen),
		x, y, width, height, border_width,
		BlackPixel(display, screen),
		WhitePixel(display,screen));

	size_hints.flags = PPosition | PSize | PMinSize;
	size_hints.x = x;
	size_hints.y = y;
	size_hints.width = width;
	size_hints.height = height;
	size_hints.min_width = 350;
	size_hints.min_height = 250;

	XSetStandardProperties(display,
		win, window_name, icon_name, NULL,
		argv, argc, &size_hints);

	XSelectInput(display, win,
		ExposureMask |
		KeyPressMask |
		ButtonPressMask |
		StructureNotifyMask);

	load_font(&font_info);

	get_GC(win, &gc, font_info);

	XMapWindow(display, win);

	... Loppuosa koodista siirrettiin x11_main_loop-metodiin...
}

void get_GC(Window win, GC *gc, XFontStruct *font_info)
{
	unsigned long valuemask = 0;
	XGCValues values;
	unsigned int line_width = 1;
	int line_style = LineSolid; /*LineOnOffDash;*/
	int cap_style = CapButt;
	int join_style = JoinMiter;
	int dash_offset = 0;
	static char dash_list[] = {20, 40};
	int list_length = sizeof(dash_list);

	*gc = XCreateGC(display, win, valuemask, &values);

	XSetFont(display, *gc, font_info->fid);
	XSetForeground(display, *gc, BlackPixel(display,screen));
	XSetLineAttributes(display, *gc, line_width, line_style, cap_style, join_style);
	XSetDashes(display, *gc, dash_offset, dash_list, list_length);
}

void load_font(XFontStruct **font_info)
{
	char *fontname = "9x15";

	if ((*font_info=XLoadQueryFont(display,fontname)) == NULL)
	{
		printf("stderr, basicwin: cannot open 9x15 font\n");
		exit(-1);
	}
}

void draw_text(Window win, GC gc, XFontStruct *font_info,
	unsigned int win_width, unsigned int win_height)
{
	int y = 20;
	char *string1 = "Hi! I'm a window, who are you?";
	char *string2 = "To terminate program; Press any key";
	char *string3 = "or button while in this window.";
	char *string4 = "Screen Dimensions:";
	int len1, len2, len3, len4;
	int width1, width2, width3;
	char cd_height[50], cd_width[50], cd_depth[50];
	int font_height;
	int y_offset, x_offset;

	len1 = strlen(string1);
	len2 = strlen(string2);
	len3 = strlen(string3);

	width1 = XTextWidth(font_info, string1, len1);
	width2 = XTextWidth(font_info, string2, len2);
	width3 = XTextWidth(font_info, string3, len3);

	XDrawString(display, win, gc, (win_width-width1)/2, y, string1, len1);
	XDrawString(display, win, gc, (win_width-width2)/2, (int)(win_height-35), string2, len2);
	XDrawString(display, win, gc, (win_width-width3)/2, (int)(win_height-15), string3, len3);

	sprintf(cd_height,"Height - %d pixels", DisplayHeight(display,screen));
	sprintf(cd_width,"Width - %d pixels", DisplayWidth(display,screen));
	sprintf(cd_depth,"Depth - %d plane(s)", DefaultDepth(display,screen));

	len4 = strlen(string4);
	len1 = strlen(cd_height);
	len2 = strlen(cd_width);
	len3 = strlen(cd_depth);

	font_height = font_info->max_bounds.ascent + 
	font_info->max_bounds.descent;

	y_offset = win_height/2 - font_height - font_info->max_bounds.descent;
	x_offset = (int) win_width/4;

	XDrawString(display, win, gc, x_offset, y_offset, string4, len4);
	y_offset += font_height;

	XDrawString(display, win, gc, x_offset, y_offset, cd_height, len1);
	y_offset += font_height;

	XDrawString(display, win, gc, x_offset, y_offset, cd_width, len2);
	y_offset += font_height;

	XDrawString(display, win, gc, x_offset, y_offset, cd_depth, len3);
}

void draw_graphics(Window win, GC gc, unsigned int window_width, unsigned int window_height)
{
	int x,y;
	unsigned int width, height;

	height = window_height/2;
	width = 3 * window_width/4;

	x = window_width/2 - width/2;
	y = window_height/2 - height/2;

	XDrawRectangle(display, win, gc, x, y, width, height);
}

void TooSmall(Window win, GC gc, XFontStruct *font_info)
{
	char *string1 = "Too Small";
	int x,y;

	y = font_info->max_bounds.ascent + 2;
	x = 2;

	XDrawString(display, win, gc, x, y, string1, strlen(string1));
}
#endif
