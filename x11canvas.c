#include <linux/input.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

#include <stdio.h>
#include <stdlib.h>

#include "commands.h"

struct x11_data
{
	Display *display;
	int screen;
	Window win;
	GC gc;

	cairo_surface_t *visible_surface;
};

static void x11_convert_surface (cairo_surface_t *surface)
{
	int width = cairo_image_surface_get_width (surface);
	int height = cairo_image_surface_get_height (surface);
	int stride = cairo_image_surface_get_stride (surface);
	char *data = cairo_image_surface_get_data (surface);

	// ARGB -> ABGR?
	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			unsigned int *p = (unsigned int *)(char *)(data + j * stride) + i;
			*p = ((((*p & 0xff) >> 0) << 16)
			      | (*p & 0xff00)
			      | (((*p & 0xff0000) >> 16) << 0)
			      | (*p & 0xff000000));
		}
	}
}

static void x11_draw(struct backend *be, cairo_surface_t *surface)
{
	x11_convert_surface (surface);

	cairo_pattern_t *pat = cairo_pattern_create_for_surface (surface);
	cairo_t *cr = cairo_create (((struct x11_data *)(be->data))->visible_surface);
	cairo_set_source (cr, pat);
	cairo_paint (cr);
	cairo_destroy (cr);
	cairo_pattern_destroy (pat);
}

struct backend x11_backend;

static struct backend *open_x11(char *filename)
{
	struct backend *be = &x11_backend;

	XGCValues values;
	XSizeHints size_hints;
	char *display_name = NULL;

	struct x11_data *data = malloc(sizeof(*data));
	if (!data)
	{
		be = NULL;
		goto out;
	}

	be->data = data;
	
	data->display = XOpenDisplay(display_name);
	if (!data->display)
	{
		free(data);
		be = NULL;
		goto out;

	}

	data->screen = DefaultScreen(data->display);
	be->width = DisplayWidth(data->display, data->screen);
	be->height = DisplayHeight(data->display, data->screen);
//	be->fb->depth = DefaultDepth(data->display, data->screen);
	be->draw = x11_draw;

	data->win = XCreateSimpleWindow(data->display,
		RootWindow(data->display, data->screen),
		0, 0,
		be->width,
		be->height,
		0, /* Border width */
		BlackPixel(data->display, data->screen),
		WhitePixel(data->display, data->screen));

	size_hints.flags = PPosition | PSize | PMinSize;
	size_hints.x = 0;
	size_hints.y = 0;
	size_hints.width = be->width;
	size_hints.height = be->height;
	size_hints.min_width = 350;
	size_hints.min_height = 250;

	XSetStandardProperties(data->display, data->win,
		"fb", NULL, NULL,
		NULL, 0, &size_hints);

	XSelectInput(data->display, data->win,
		ExposureMask
		/* | KeyPressMask */
		/* | ButtonPressMask */
		/* | StructureNotifyMask */
		);

	data->gc = XCreateGC(data->display, data->win, 0, &values);
	XSetForeground(data->display, data->gc, BlackPixel(data->display, data->screen));
	XSetLineAttributes(data->display, data->gc, 1, LineSolid, CapButt, JoinMiter);
	XMapWindow(data->display, data->win);

	data->visible_surface = cairo_xlib_surface_create(data->display,
							  data->win,
							  XDefaultVisual(data->display, 0),
							  be->width,
							  be->height);

	be->surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, be->width, be->height);

out:
	return be;
}

static void close_x11(struct backend *be)
{
	struct x11_data *data = be->data;

	XFreeGC(data->display, data->gc);
	XCloseDisplay(data->display);

	free(data);
}

static Bool predicate(Display *display, XEvent *event, XPointer arg)
{
	return True;
}

static void handle_events(struct document *doc)
{
	XEvent ev;
	struct x11_data *data = doc->backend->data;

	/*
	 * Check if there is a pending event, because
	 * we don't want to block.
	 */
	if (XCheckIfEvent(data->display, &ev, predicate, NULL))
	{
		XEvent report;
		XNextEvent(data->display, &report);
		if (report.type == Expose)
		{
			while (XCheckTypedEvent(data->display, Expose, &report))
				;
			if (doc->flags & DOCUMENT_IDLE)
				doc->draw(doc);
		}
	}
}

struct backend x11_backend =
{
	.open = open_x11,
	.close = close_x11,

	.idle_callback = handle_events,
};


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
