#ifndef FBCANVAS_H
#define FBCANVAS_H

struct fbcanvas
{
	unsigned int width;
	unsigned int height;
	unsigned int xoffset;
	unsigned int yoffset;
	unsigned char bpp;
	unsigned char *buffer;
};

struct fbcanvas *fbcanvas_create(int width, int height);
void fbcanvas_destroy(struct fbcanvas *fbc);

#endif /* FBCANVAS_H */
