AM_CFLAGS = @BK_STD_CFLAGS@ -I$(srcdir)/../../BlipKit/src -I$(srcdir)/.. -L$(srcdir)/..

noinst_LIBRARIES = libamigamod.a

libamigamod_a_SOURCES = \
	BKFileAmigaMod.c

dist_noinst_HEADERS = \
	BKFileAmigaMod.h

EXTRA_PROGRAMS = volume_values test_load

volume_values_SOURCES = \
	volume_values.c

volume_values_CFLAGS = $(AM_CFLAGS)
volume_values_LDADD = -lm $(srcdir)/../../BlipKit/src/libblipkit.a @SDL_CFLAGS@

test_load_SOURCES = \
	test_load.c

test_load_CFLAGS = $(AM_CFLAGS)
test_load_LDADD = -lm $(srcdir)/../../BlipKit/src/libblipkit.a ../libbliparser.a libamigamod.a @SDL_CFLAGS@

pkginclude_HEADERS = $(HEADER_LIST)

otherincludedir = $(includedir)/BlipKit

otherinclude_HEADERS = $(HEADER_LIST)
