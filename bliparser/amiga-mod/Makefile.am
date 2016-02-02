AM_CFLAGS = @AM_CFLAGS@ -I$(srcdir)/../../BlipKit/src -I$(srcdir)/..
BK_LDADD = ../../BlipKit/src/libblipkit.a ../libbliparser.a libamigamod.a @SDL_CFLAGS@

noinst_LIBRARIES = libamigamod.a

libamigamod_a_SOURCES = \
	BKFileAmigaMod.c

dist_noinst_HEADERS = \
	BKFileAmigaMod.h

EXTRA_PROGRAMS = volume_values test_load test_play

volume_values_SOURCES = volume_values.c
volume_values_LDADD = $(BK_LDADD)

test_load_SOURCES = test_load.c
test_load_LDADD = $(BK_LDADD)

test_play_SOURCES = test_play.c
test_play_LDADD = $(BK_LDADD)

pkginclude_HEADERS = $(HEADER_LIST)

otherincludedir = $(includedir)/BlipKit

otherinclude_HEADERS = $(HEADER_LIST)
