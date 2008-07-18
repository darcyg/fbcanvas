dnl configure.ac - 17.7.2008 - 18.7.2008 Ari & Tero Roponen
AC_INIT([fb],[20080717])
AC_CONFIG_FILES([Makefile])

AC_DEFUN([FB_OPTIONAL_FEATURE],
  [AC_ARG_ENABLE([$1],
    [AS_HELP_STRING([--disable-$1], [disable $1-support])],
    [AC_SUBST([enable_$1]) FB_SUPPORT="$FB_SUPPORT\n\t$1 = $enable_$1"],
    [FB_SUPPORT="$FB_SUPPORT\n\t$1 = yes"])])

FB_SUPPORT="Supported filetypes:"

FB_OPTIONAL_FEATURE([djvu])
FB_OPTIONAL_FEATURE([ps])
FB_OPTIONAL_FEATURE([pdf])
FB_OPTIONAL_FEATURE([text])

AC_ARG_ENABLE([x11],
  [AS_HELP_STRING([--disable-x11], [disable x11-support])],
  [AC_SUBST([enable_x11])])

AC_OUTPUT

echo -e $FB_SUPPORT
