#ifndef UTIL_H
#define UTIL_H
#include <cairo/cairo.h>

int grep_from_str (char *regexp, char *str, char *where, unsigned int page);
void convert_surface_argb_to_abgr (cairo_surface_t *surface);

#endif
