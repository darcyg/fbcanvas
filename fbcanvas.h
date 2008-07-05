#ifndef FBCANVAS_H
#define FBCANVAS_H

struct framebuffer
{
	unsigned char *mem;	/* mmap()ed framebuffer memory	*/
	unsigned int depth;	/* Hardware color depth		*/
};

#endif /* FBCANVAS_H */
