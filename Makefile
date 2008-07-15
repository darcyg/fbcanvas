# Makefile - 20.5.2008 - 20.6.2008 Ari & Tero Roponen

C_FILES:=$(wildcard *.c)
H_FILES:=$(wildcard *.h)
OBJ_FILES:=$(C_FILES:.c=.o)

CFLAGS:=$(shell pkg-config --cflags poppler-glib ddjvuapi) -Os -D_GNU_SOURCE -std=gnu99
LIBS:=$(shell pkg-config --libs poppler-glib ddjvuapi) -lmagic -lgs

oma: $(OBJ_FILES)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# Every h-file has a corresponding c-file. o-file depends on c-file and h-file.
$(foreach header,$(H_FILES),$(eval $(header:.h=.o): $(header:.h=.c) $(header)))

.PHONY: clean
clean:
	rm -f oma *.o
