AM_CFLAGS = @BK_STD_CFLAGS@ -I$(srcdir)/../BlipKit/src -I$(srcdir)/../bliparser -L$(srcdir)/../BlipKit/src

#bin_PROGRAMS = bliplay parsertest
bin_PROGRAMS = parsertest

#bliplay_SOURCES = \
#	bliplay.c

parsertest_SOURCES = \
	parsertest.c

# LDFLAGS should be used for linking libraries not LDADD
# But some newer GCC versions need to have the linkings at the end

#bliplay_CFLAGS = $(AM_CFLAGS) -DPROGRAM_NAME=\"bliplay\"
#bliplay_LDADD = ../bliparser/libbliparser.a ../BlipKit/src/libblipkit.a @SDL_CFLAGS@

parsertest_CFLAGS = $(AM_CFLAGS)
parsertest_LDADD = ../bliparser/libbliparser.a ../BlipKit/src/libblipkit.a @SDL_CFLAGS@
