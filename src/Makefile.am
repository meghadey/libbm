lib_LTLIBRARIES = libbm.la
check_PROGRAMS = test example

EXTRA_DIST = libbm.h perf_event.h libbm_example.c

libbm_la_SOURCES = libbm.c

libbm_la_LDFLAGS = -version-info $(LIBBM_SO_VERSION)

include_HEADERS = libbm.h

pkgconfigdir = $(libdir)/pkgconfig

example_SOURCES = libbm_example.c
example_LDADD = libbm.la

pkgconfig_DATA = libbm.pc
