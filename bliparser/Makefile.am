AM_CFLAGS = @AM_CFLAGS@ -I$(srcdir)/../BlipKit/src

lib_LIBRARIES = libbliparser.a

libbliparser_a_SOURCES = \
	BKArray.c \
	BKByteBuffer.c \
	BKByteBufferSource.c \
	BKCompiler.c \
	BKContextWrapper.c \
	BKInterpreter.c \
	BKString.c \
	BKSTTokenizer.c \
	BKSTParser.c

HEADER_LIST = \
	BlipParser.h \
	BKArray.h \
	BKByteBuffer.h \
	BKByteBufferSource.h \
	BKCompiler.h \
	BKContextWrapper.h \
	BKInterpreter.h \
	BKString.h \
	BKSTTokenizer.h \
	BKSTParser.h

pkginclude_HEADERS = $(HEADER_LIST)

otherincludedir = $(includedir)/blipkit

otherinclude_HEADERS = $(HEADER_LIST)
