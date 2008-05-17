#ifndef FBCANVAS_H
#define FBCANVAS_H

struct fbcanvas
{
	int width;
	int height;
	int bpp;
	unsigned char *buffer;
};

struct fbcanvas *fbcanvas_create(int width, int height);
void fbcanvas_destroy(struct fbcanvas *fbc);

#endif /* FBCANVAS_H */
