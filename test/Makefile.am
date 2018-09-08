BLIPLAY = $(top_builddir)/bliplay/bliplay
EXAMPLES = $(top_builddir)/examples

AM_CFLAGS = @AM_CFLAGS@ \
	-I$(srcdir)/../utility \
	-I$(srcdir)/../BlipKit/src

BK_LDADD = \
	$(srcdir)/../utility/libutility.a \
	$(srcdir)/../BlipKit/src/libblipkit.a \
	-lm

check_PROGRAMS = \
	string \
	fft

string_SOURCES = string.c
string_LDADD = $(BK_LDADD)

fft_SOURCES = fft.c
fft_LDADD = $(BK_LDADD)

# Enable malloc debugging where available
TESTS_ENVIRONMENT = \
	export bliplay=$(BLIPLAY); \
	export examples_dir=$(EXAMPLES); \
	top_builddir=$(top_builddir); \
	export MallocScribble=1; \
	export MallocPreScribble=1; \
	export MallocGuardEdges=1;

TESTS = \
	string \
	fft \
	test-1.sh \
	test-2.sh \
	test-3.sh \
	test-4.sh
