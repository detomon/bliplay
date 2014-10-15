AM_CFLAGS = @BK_STD_CFLAGS@ -I$(srcdir)/../BlipKit/src -L$(srcdir)/../BlipKit/src

noinst_LIBRARIES = libbliparser.a

libbliparser_a_SOURCES = \
	BKCompiler2.c \
	BKInterpreter.c \
	BKSTTokenizer.c \
	BKSTParser.c \
	BKArray.c \
	BKByteBuffer.c

dist_noinst_HEADERS = \
	BKCompiler2.h \
	BKInterpreter.h \
	BKSTTokenizer.h \
	BKSTParser.h \
	BKArray.h \
	BKByteBuffer.h

pkginclude_HEADERS = $(HEADER_LIST)

otherincludedir = $(includedir)/BlipKit

otherinclude_HEADERS = $(HEADER_LIST)
