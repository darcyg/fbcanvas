/*
 * main.c - 17.5.2008 - 17.5.2008 Ari & Tero Roponen
 */

#include <string.h>
#include "fbcanvas.h"

int main(int argc, char *argv[])
{
	struct fbcanvas *fbc = fbcanvas_create(200,100);
	if (fbc)
	{
		memset(fbc->buffer, 0xFFFF, 200*100*(fbc->bpp/8));
		fbcanvas_draw(fbc);
		fbcanvas_destroy(fbc);
	}

	return 0;
}
