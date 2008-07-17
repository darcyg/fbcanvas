# Makefile - 20.5.2008 - 17.7.2008 Ari & Tero Roponen

# Comment out if not supported.
support_djvu=1
support_ps=1
support_pdf=1
support_x11=1
support_text=1
# TODO: file_types.c must be recompiled if these are changed

DEBUG=
CFLAGS=$(shell pkg-config --cflags poppler-glib) -Os -D_GNU_SOURCE -std=gnu99 $(DEBUG)
LIBS=$(shell pkg-config --libs poppler-glib) -lmagic

all: oma

.PHONY: clean
clean:
	rm -f oma *.o

# These are compiled always
C_FILES=commands.c document.c extcmd.c fbcanvas.c file_info.c \
	keymap.c main.c pixbuf.c readline.c terminal.c util.c

# Keep these up-to-date
commands.o: commands.c commands.h document.h extcmd.h keymap.h util.h
document.o: document.c document.h file_info.h
extcmd.o: extcmd.c document.h extcmd.h
fbcanvas.o: fbcanvas.c document.h file_info.h
file_info.o: file_info.c file_info.h
keymap.o: keymap.c keymap.h
main.o: main.c document.h commands.h extcmd.h keymap.h terminal.h
pixbuf.o: pixbuf.c document.h file_info.h
readline.o: readline.c document.h keymap.h terminal.h
terminal.o: terminal.c document.h keymap.h
util.o: util.c util.h

ifdef support_text
  C_FILES += text.c
  CFLAGS += -DUSE_TEXT
text.o: text.c document.h extcmd.h file_info.h keymap.h util.h
endif

ifdef support_x11
  C_FILES += x11canvas.c
  CFLAGS += -DUSE_X11
x11canvas.o: x11canvas.c commands.h util.h
endif

ifdef support_djvu
  C_FILES += djvu.c
  CFLAGS += -DUSE_DJVU $(shell pkg-config --cflags ddjvuapi)
  LIBS += $(shell pkg-config --libs ddjvuapi)
djvu.o: djvu.c document.h file_info.h
endif

ifdef support_ps
  C_FILES += ps.c
  CFLAGS += -DUSE_PS
  LIBS += -lgs
ps.o: ps.c document.h file_info.h
endif

ifdef support_pdf
  C_FILES += pdf.c
  CFLAGS += -DUSE_PDF
pdf.o: pdf.c document.h file_info.h util.h
endif

OBJ_FILES:=$(C_FILES:.c=.o)

oma: $(OBJ_FILES)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
