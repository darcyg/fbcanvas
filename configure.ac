dnl configure.ac - 17.7.2008 - 19.7.2008 Ari & Tero Roponen
AC_INIT([fb],[20080719])
AM_INIT_AUTOMAKE
AC_CONFIG_FILES([Makefile])
AC_CONFIG_HEADERS(config.h)

AC_DEFUN([FB_OPTIONAL_FEATURE],
  [AC_ARG_ENABLE([$1],
    [AS_HELP_STRING([--disable-$1], [disable $1-support])],,[enable_$1=[yes]])])

FB_OPTIONAL_FEATURE([djvu])
FB_OPTIONAL_FEATURE([ps])
FB_OPTIONAL_FEATURE([pdf])
FB_OPTIONAL_FEATURE([text])
FB_OPTIONAL_FEATURE([x11])

# poppler-glib is required
PKG_CHECK_MODULES([poppler_glib],[poppler-glib],
  [LIBS="$LIBS $poppler_glib_LIBS"
   CFLAGS="$CFLAGS $poppler_glib_CFLAGS"])

# magic is also required
AC_SEARCH_LIBS([magic_file],[magic])

# so is pango
PKG_CHECK_MODULES([pango],[pangocairo],
  [LIBS="$LIBS $pango_LIBS"
   CFLAGS="$CFLAGS $pango_CFLAGS"])

# and gdk-pixbuf
PKG_CHECK_MODULES([gdk_pixbuf],[gdk-pixbuf-2.0],
  [LIBS="$LIBS $gdk_pixbuf_LIBS"
   CFLAGS="$CFLAGS $gdk_pixbuf_CFLAGS"])

if test "$enable_djvu" = "yes"; then
  PKG_CHECK_MODULES([djvu],[ddjvuapi],
    [AC_DEFINE([ENABLE_DJVU],1,[Enable DJVU-support])
     LIBS="$LIBS $djvu_LIBS"
     INCS="$INCS $djvu_CFLAGS"],
    [enable_djvu="no"])
fi

if test "$enable_ps" = "yes"; then
  AC_SEARCH_LIBS([gsapi_init_with_args],[gs],
    [AC_DEFINE([ENABLE_PS],1,[Enable postscript-support])],
    [enable_ps="no"])
fi

if test "$enable_pdf" = "yes"; then
  AC_DEFINE([ENABLE_PDF],1,[Enable PDF-support])
fi

if test "$enable_text" = "yes"; then
  AC_DEFINE([ENABLE_TEXT],1,[Enable text-support])
fi

if test "$enable_x11" = "yes"; then
  AC_DEFINE([ENABLE_X11],1,[Enable X11-support])
fi

AC_OUTPUT

echo
echo "Support djvu:       $enable_djvu"
echo "Support postscript: $enable_ps"
echo "Support pdf:        $enable_pdf"
echo "Support text:       $enable_text"
echo "Enable X11:         $enable_x11"
