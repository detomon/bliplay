AM_CFLAGS = @AM_CFLAGS@ -I$(srcdir)/../BlipKit/src -I$(srcdir)/../bliparser -L$(srcdir)/../BlipKit/src

bin_PROGRAMS = bliplay

bliplay_SOURCES = \
	bliplay.c

bliplay_CFLAGS = $(AM_CFLAGS) -DPROGRAM_NAME=\"bliplay\"
bliplay_LDADD = -lm ../bliparser/libbliparser.a ../BlipKit/src/libblipkit.a @SDL_CFLAGS@

EXTRA_DIST = \
	examples/template.blip \
	examples/gameboy-start.blip \
	examples/generic-boss-appears.blip \
	examples/ghost-bouncer.blip \
	examples/hyperion-star-racer.blip \
	examples/wysiwyg.blip.blip
