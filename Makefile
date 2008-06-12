# Makefile - 20.5.2008 - 12.6.2008 Ari & Tero Roponen

CFLAGS:=$(shell pkg-config --cflags poppler-glib ddjvuapi) -Os -D_GNU_SOURCE
LIBS:=$(shell pkg-config --libs poppler-glib ddjvuapi) -lncurses -lmagic

oma: main.o fbcanvas.o djvu.o pdf.o pixbuf.o keymap.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

.PHONY: clean
clean:
	rm -f oma *.o
