AM_CFLAGS = @BK_STD_CFLAGS@ -I$(srcdir)/../BlipKit/src -I$(srcdir)/../bliparser -L$(srcdir)/../BlipKit/src

bin_PROGRAMS = bliplay

bliplay_SOURCES = \
	bliplay.c

# LDFLAGS should be used for linking libraries not LDADD
# But some newer GCC versions need to have the linkings at the end

bliplay_CFLAGS = $(AM_CFLAGS) -DPROGRAM_NAME=\"bliplay\"
#bliplay_LDFLAGS = -lblipkit @SDL_CFLAGS@
bliplay_LDADD = ../bliparser/libbliparser.a ../BlipKit/src/libblipkit.a @SDL_CFLAGS@
