/*
 * main.c - 17.5.2008 - 17.5.2008 Ari & Tero Roponen
 */

#include "fbcanvas.h"

int main(int argc, char *argv[])
{
	struct fbcanvas *fbc = fbcanvas_create(100,100);
	if (fbc)
	{
		fbcanvas_destroy(fbc);
	}

	return 0;
}
