dnl configure.ac - 17.7.2008 - 19.7.2008 Ari & Tero Roponen
AC_INIT([fb],[20080717])
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

# extra libraries and headers
LIBS=""
INCS=""

if test "$enable_djvu" = "yes"; then
  PKG_CHECK_MODULES([djvu],[ddjvuapi],
    [AC_DEFINE([ENABLE_DJVU],1,[Enable DJVU-support])
     LIBS="$LIBS $djvu_LIBS"
     INCS="$INCS $djvu_CFLAGS"],
    [enable_djvu="no"])
fi

if test "$enable_ps" = "yes"; then
  AC_CHECK_LIB([gs],[gsapi_init_with_args],
    [AC_DEFINE([ENABLE_PS],1,[Enable postscript-support])
     LIBS="$LIBS -lgs"],
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

AC_SUBST([EXTRA_LIBS],[$LIBS])
AC_SUBST([EXTRA_INCLUDES],[$INCS])
AC_OUTPUT

echo
echo "Support djvu:       $enable_djvu"
echo "Support postscript: $enable_ps"
echo "Support pdf:        $enable_pdf"
echo "Support text:       $enable_text"
echo "Enable X11:         $enable_x11"
