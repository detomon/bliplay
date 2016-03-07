AM_CFLAGS = @AM_CFLAGS@ -I$(srcdir)/../BlipKit/src

lib_LIBRARIES = libbliparser.a

libbliparser_a_SOURCES = \
	BKTKCompiler.c \
	BKTKContext.c \
	BKTKInterpreter.c \
	BKTKParser.c \
	BKTKTokenizer.c \
	BKTKWriter.c

HEADER_LIST = \
	BKTK.h \
	BKTKBase.h \
	BKTKCompiler.h \
	BKTKContext.h \
	BKTKInterpreter.h \
	BKTKParser.h \
	BKTKTokenizer.h \
	BKTKWriter.h

pkginclude_HEADERS = $(HEADER_LIST)

otherincludedir = $(includedir)/blipkit

otherinclude_HEADERS = $(HEADER_LIST)
