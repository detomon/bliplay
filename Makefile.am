AM_CFLAGS = @BK_STD_CFLAGS@ -I$(srcdir)/BlipKit/src

SUBDIRS = BlipKit parser bliplay
DIST_SUBDIRS = parser bliplay editor-themes

EXTRA_DIST = \
	README.md \
	LICENSE \
	VERSION \
	autogen.sh \
	editor-themes

EXTRA_INSTALL = \
	editor_themes

editor_themesdir = "$(HOME)/Library/Application Support/Sublime Text 2/Packages"
editor_themes_DATA = editor-themes/sublime-text
