AM_CFLAGS = @AM_CFLAGS@ \
	-I$(srcdir)/../parser \
	-I$(srcdir)/../utility \
	-I$(srcdir)/../BlipKit/src

bin_PROGRAMS = bliplay

bliplay_SOURCES = \
	bliplay.c

bliplay_CFLAGS = $(AM_CFLAGS) -DPROGRAM_NAME=\"bliplay\"
bliplay_LDADD = \
	$(srcdir)/../parser/libbliparser.a \
	$(srcdir)/../utility/libutility.a \
	$(srcdir)/../BlipKit/src/libblipkit.a \
	@SDL_CFLAGS@ -lm
