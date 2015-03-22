AM_CFLAGS = @BK_STD_CFLAGS@ -I$(srcdir)/../../BlipKit/src -I$(srcdir)/.. -L$(srcdir)/..

noinst_LIBRARIES = libamigamod.a

libamigamod_a_SOURCES = \
	BKFileAmigaMod.c

dist_noinst_HEADERS = \
	BKFileAmigaMod.h

libamigamod_a_CFLAGS = $(AM_CFLAGS)
libamigamod_a_LIBADD = ../libbliparser.a

EXTRA_PROGRAMS = volume_values

volume_values_SOURCES = \
	volume_values.c

volume_values_CFLAGS = $(AM_CFLAGS)
volume_values_LDADD = -lm -lblipkit @SDL_CFLAGS@

pkginclude_HEADERS = $(HEADER_LIST)

otherincludedir = $(includedir)/BlipKit

otherinclude_HEADERS = $(HEADER_LIST)
