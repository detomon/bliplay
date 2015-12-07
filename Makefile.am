AM_CFLAGS = @BK_STD_CFLAGS@ -I$(srcdir)/BlipKit/src

SUBDIRS = BlipKit bliparser bliplay
DIST_SUBDIRS = bliparser bliplay editor-themes

EXTRA_DIST = \
	README.md \
	LICENSE \
	VERSION \
	autogen.sh \
	editor-themes

EXTRA_INSTALL = \
	editor_themes

editor_themesdir = "$(HOME)/Library/Application Support/Sublime Text/Packages"
editor_themes_DATA = editor-themes/sublime-text
