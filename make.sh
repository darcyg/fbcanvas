#!/bin/bash
gcc $(pkg-config --cflags poppler-glib --libs poppler-glib) *.c -o oma # -lncurses
