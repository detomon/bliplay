#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])
AC_INIT([bliplay],[3.1.6],[simon@monoxid.net],[bliplay],[https://play.blipkit.audio])

AC_CONFIG_SRCDIR([bliplay/bliplay.c])
AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE([-Wall -Werror foreign])

m4_include([BlipKit/m4/common.m4])

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

AM_CFLAGS="$CFLAGS"

# Define flags.
AM_CFLAGS="$AM_CFLAGS -Wall \
	-Wmissing-prototypes \
	-Wmissing-declarations \
	-Wstrict-prototypes \
	-Wformat-security \
	-Winline"

# Use C11 standard.
CHECK_COMPILE_FLAG([-std=c11], [AM_CFLAGS])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h stddef.h stdint.h stdlib.h string.h unistd.h])

AC_ARG_WITH([sdl],
	AS_HELP_STRING([--without-sdl], [do not link against SDL library]))

# Set default of with_sdl to yes.
AS_IF([test "x$with_sdl" = "x"],
	[with_sdl=yes],
	[])

# Check for option with_sdl.
if test "x$with_sdl" = xyes; then
	AC_DEFINE(BK_USE_SDL, 1, [Define to 1 if configure had option --with-sdl])
	AC_CHECK_HEADERS([termios.h])
else
	AC_DEFINE(BK_USE_SDL, 0, [Define to 0 if configure had option --without-sdl])
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([getcwd memmove memset select])

AC_CONFIG_FILES([
	Makefile
	utility/Makefile
	parser/Makefile
	bliplay/Makefile
	test/Makefile
])

AC_CONFIG_SUBDIRS([BlipKit])

# Run tests.
CHECK_SIGNED_SHIFT()

# Check for SDL.
if test "x$with_sdl" = xyes; then
	CHECK_SDL()
fi

AM_CFLAGS="$AM_CFLAGS $SDL_CFLAGS"

AC_SUBST([AM_CFLAGS])

AC_OUTPUT
