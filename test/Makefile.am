BLIPLAY = $(top_builddir)/bliplay/bliplay
EXAMPLES = $(top_builddir)/examples

TESTS_ENVIRONMENT = \
	export bliplay=$(BLIPLAY); \
	export examples_dir=$(EXAMPLES); \
	# Enable malloc debugging where available
	export MallocScribble=1; \
	export MallocPreScribble=1; \
	export MallocGuardEdges=1;

TESTS = \
	test-1.sh \
	test-2.sh \
	test-3.sh \
	test-4.sh
