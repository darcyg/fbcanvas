# Makefile - 20.5.2008 - 12.6.2008 Ari & Tero Roponen

C_FILES:=$(wildcard *.c)
OBJ_FILES:=$(C_FILES:.c=.o)

CFLAGS:=$(shell pkg-config --cflags poppler-glib ddjvuapi) -Os -D_GNU_SOURCE
LIBS:=$(shell pkg-config --libs poppler-glib ddjvuapi) -lncurses -lmagic

oma: $(OBJ_FILES)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

.PHONY: clean
clean:
	rm -f oma *.o
