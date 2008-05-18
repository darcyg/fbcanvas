/*
 * main.c - 17.5.2008 - 17.5.2008 Ari & Tero Roponen
 */

#include <string.h>
#include "fbcanvas.h"

int main(int argc, char *argv[])
{
	struct fbcanvas *fbc = fbcanvas_create(900,200);
	if (fbc)
	{
		memset(fbc->buffer, 0xFFFF, 900*200*(fbc->bpp/8));
		fbcanvas_draw(fbc);

		fbc->xoffset = 200; /* Aloita piirtäminen kohdasta 200,100 */
		fbc->yoffset = 100;
		memset(fbc->buffer, 0xF0F0, 900*200*(fbc->bpp/8));
		fbcanvas_draw(fbc);

		fbcanvas_destroy(fbc);
	}

	return 0;
}
