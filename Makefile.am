AM_CFLAGS = @BK_STD_CFLAGS@ -I$(srcdir)/BlipKit/src

SUBDIRS = BlipKit utility parser bliplay test
DIST_SUBDIRS = utility parser bliplay editor-themes

EXTRA_DIST = \
	README.md \
	LICENSE \
	VERSION \
	autogen.sh \
	editor-themes \
	examples/bone-eater.blip \
	examples/cave-xii.blip \
	examples/dont-eat-flashcards.blip \
	examples/gameboy-start.blip \
	examples/generic-boss-appears.blip \
	examples/ghost-bouncer.blip \
	examples/hyperion-star-racer.blip \
	examples/killer-squid.blip \
	examples/short-fused-bombs.blip \
	examples/template.blip \
	examples/wysiwyg.blip

EXTRA_INSTALL = \
	editor_themes

editor_themesdir = "$(HOME)/Library/Application Support/Sublime Text 2/Packages"
editor_themes_DATA = editor-themes/SublimeText
