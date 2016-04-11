AM_CFLAGS = @AM_CFLAGS@ -I$(srcdir)/../BlipKit/src -I$(srcdir)/../parser -L$(srcdir)/../BlipKit/src

bin_PROGRAMS = bliplay

bliplay_SOURCES = \
	bliplay.c

bliplay_CFLAGS = $(AM_CFLAGS) -DPROGRAM_NAME=\"bliplay\"
bliplay_LDADD = -lm ../parser/libbliparser.a ../BlipKit/src/libblipkit.a @SDL_CFLAGS@
