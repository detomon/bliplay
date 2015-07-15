#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([bliplay],[2.4.1],[simon@monoxid.net],[bliplay],[])

AC_CONFIG_SRCDIR([bliplay/bliplay.c])
AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_CPP
AC_PROG_MKDIR_P
AC_PROG_RANLIB
AC_PROG_LN_S
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

# Checks for libraries.
AC_CHECK_LIB([blipkit], [BKContextInit])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h stddef.h stdint.h stdlib.h string.h termios.h unistd.h])

AC_ARG_ENABLE([player],
  [  --disable-player        do not link against SDL library; only enable file output],
  [with_player=${enableval}],
  [with_player=yes])

if test "x${with_player}" = xyes; then
  AC_DEFINE(BK_USE_PLAYER, 1, [Define to 1 if configure had option --enable-player])
else
  AC_DEFINE(BK_USE_PLAYER, 0, [Define to 1 if configure had option --enable-player])
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([getcwd memmove memset select])

AC_CONFIG_FILES([
  Makefile
  bliparser/Makefile
  bliplay/Makefile
])

AC_CONFIG_SUBDIRS([BlipKit])

# Define flags.
BK_STD_CFLAGS="-std=c99 -Wall \
    -Wmissing-prototypes \
    -Wmissing-declarations \
    -Wstrict-prototypes \
    -Wformat-security \
    -Wmissing-format-attribute \
    -Winline"

AC_SUBST(BK_STD_CFLAGS)

# Run tests.
AC_RUN_IFELSE([AC_LANG_SOURCE([[
int main () { return !((-15 >> 2) == -4); }
]])],[],[AC_MSG_ERROR(Host CPU seems not to handle shifting of negative values as expected)
],[])

# Set platform specific values.
AC_CANONICAL_HOST

case $host_os in
  darwin* )
    SDL_CFLAGS="-framework SDL"
    ;;
  *)
    SDL_CFLAGS="`sdl-config --cflags` `sdl-config --libs`"
    ;;
esac

if test "x${with_player}" = xyes; then
  echo "using SDL linking flags: $SDL_CFLAGS"
  AC_SUBST(SDL_CFLAGS)
else
  SDL_CFLAGS=""
  AC_SUBST(SDL_CFLAGS)
fi

AC_OUTPUT
