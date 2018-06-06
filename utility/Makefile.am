AM_CFLAGS = @AM_CFLAGS@ \
	-I$(srcdir)/../BlipKit/src \
	-lm

lib_LIBRARIES = libutility.a

libutility_a_SOURCES = \
	BKArray.c \
	BKBlockPool.c \
	BKByteBuffer.c \
	BKFFT.c \
	BKHashTable.c \
	BKString.c

HEADER_LIST = \
	BKArray.h \
	BKBlockPool.h \
	BKByteBuffer.h \
	BKComplex.h \
	BKFFT.h \
	BKHashTable.h \
	BKString.h

pkginclude_HEADERS = $(HEADER_LIST)

otherincludedir = $(includedir)/blipkit

otherinclude_HEADERS = $(HEADER_LIST)
