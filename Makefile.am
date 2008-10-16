# Makefile.am - 20.5.2008 - 19.7.2008 Ari & Tero Roponen

bin_PROGRAMS = oma

oma_SOURCES = commands.c document.c extcmd.c file_info.c \
	keymap.c main.c pixbuf.c readline.c terminal.c util.c \
	text.c x11canvas.c djvu.c ps.c pdf.c

# TODO: file_info.c must be recompiled if configure-options are changed

DEBUG =
AM_CPPFLAGS = -Os -D_GNU_SOURCE -std=gnu99 $(DEBUG)
