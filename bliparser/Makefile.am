AM_CFLAGS = @BK_STD_CFLAGS@ -I$(srcdir)/../BlipKit/src -L$(srcdir)/../BlipKit/src

noinst_LIBRARIES = libbliparser.a

libbliparser_a_SOURCES = \
	BKSDLTrack.c \
	BKBlipReader.c \
	BKBlipWriter.c \
	BKCompiler.c \
	BKInterpreter.c \
	BKWaveFileReader.c \
	BKWaveFileWriter.c \
	BKSTTokenizer.c \
	BKSTParser.c

dist_noinst_HEADERS = \
	BKSDLTrack.h \
	BKBlipReader.h \
	BKBlipWriter.h \
	BKCompiler.h \
	BKInterpreter.h \
	BKWaveFileReader.h \
	BKWaveFileWriter.h \
	BKSTTokenizer.h \
	BKSTParser.h \
	item_list.h

pkginclude_HEADERS = $(HEADER_LIST)

otherincludedir = $(includedir)/BlipKit

otherinclude_HEADERS = $(HEADER_LIST)
