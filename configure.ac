#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([bliplay],[3.0b2],[simon@monoxid.net],[bliplay],[http://play.blipkit.audio])

AC_CONFIG_SRCDIR([bliplay/bliplay.c])
AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE([-Wall -Werror foreign])

m4_include([BlipKit/common.m4])

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC
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
	-Wmissing-format-attribute \
	-Winline"

CHECK_COMPILE_FLAG([-std=c11], [AM_CFLAGS])

AC_CHECK_HEADERS([fcntl.h limits.h stddef.h stdint.h stdlib.h string.h unistd.h])

AC_ARG_ENABLE([player],
	[  --disable-player        do not link against SDL library; only enable file output],
	[with_player=${enableval}],
	[with_player=yes])

if test "x${with_player}" = xyes; then
	AC_DEFINE(BK_USE_PLAYER, 1, [Define to 1 if configure had option --enable-player])
	AC_CHECK_HEADERS([termios.h])
else
	AC_DEFINE(BK_USE_PLAYER, 0, [Define to 1 if configure had option --enable-player])
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
	parser/Makefile
	bliplay/Makefile
])

AC_CONFIG_SUBDIRS([BlipKit])

# Run tests.
CHECK_SIGNED_SHIFT()

if test "x${with_player}" = xyes; then
	CHECK_SDL()
fi

AC_SUBST([AM_CFLAGS])

AC_OUTPUT
