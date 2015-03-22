AM_CFLAGS = @BK_STD_CFLAGS@ -I$(srcdir)/../BlipKit/src -L$(srcdir)/../BlipKit/src

SUBDIRS = amiga-mod

noinst_LIBRARIES = libbliparser.a

libbliparser_a_SOURCES = \
	BKArray.c \
	BKByteBuffer.c \
	BKCompiler.c \
	BKContextWrapper.c \
	BKInterpreter.c \
	BKString.c \
	BKSTTokenizer.c \
	BKSTParser.c

dist_noinst_HEADERS = \
	BKArray.h \
	BKByteBuffer.h \
	BKCompiler.h \
	BKContextWrapper.h \
	BKInterpreter.h \
	BKString.h \
	BKSTTokenizer.h \
	BKSTParser.h

pkginclude_HEADERS = $(HEADER_LIST)

otherincludedir = $(includedir)/BlipKit

otherinclude_HEADERS = $(HEADER_LIST)
