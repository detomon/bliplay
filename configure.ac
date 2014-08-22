#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])
AC_INIT([bliplay],[0.4],[simon@monoxid.net],[bliplay],[])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdlib.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_OFF_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memmove memset select])

AC_CONFIG_FILES([
 Makefile
 bliplay/Makefile
 bliparser/Makefile
])
AC_CONFIG_SUBDIRS([BlipKit])

# Flags
BK_STD_CFLAGS="-std=c99 -Wall"

AC_SUBST(BK_STD_CFLAGS)

# Tests
AC_RUN_IFELSE([AC_LANG_SOURCE([[
int main () { return !((-15 >> 2) == -4); }
]])],[],[AC_MSG_ERROR(Host CPU seems not to handle shifting of negative values as expected)
],[])

# Platform specific
AC_CANONICAL_HOST

case $host_os in
  darwin* )
      SDL_CFLAGS="-framework SDL"
      ;;
  *)
      SDL_CFLAGS="`sdl-config --cflags` `sdl-config --libs`"
     ;;
esac

echo "using SDL linking flags: $SDL_CFLAGS"

AC_SUBST(SDL_CFLAGS)

AC_OUTPUT
