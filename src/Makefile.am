ACLOCAL_AMFLAGS=-I m4

lib_LTLIBRARIES = liblustreparallelread.la

liblustreparallelread_la_SOURCES = \
	liblustreparallelread/liblustreparallelread_getstripeinfo.c \
	liblustreparallelread/liblustreparallelread_readfilepart.c
liblustreparallelread_la_LIBADD  = -llustreapi
liblustreparallelread_la_CFLAGS  = -pthread
liblustreparallelread_la_LDFLAGS = -pthread -version-info ${LIBRARY_VERSION}

liblustreparallelread_includedir=$(includedir)/liblustreparallelread
liblustreparallelread_include_HEADERS = liblustreparallelread/liblustreparallelread_getstripeinfo.h \
	liblustreparallelread/liblustreparallelread_readfilepart.h

bin_PROGRAMS = parcat
noinst_PROGRAMS = testrandomread

parcat_SOURCES = programs/parcat.c
parcat_LDADD = liblustreparallelread.la
parcat_CFLAGS = -pthread
parcat_LDFLAGS = -pthread

testrandomread_SOURCES = programs/testrandomread.c
testrandomread_LDADD = liblustreparallelread.la
testrandomread_CFLAGS = -pthread
testrandomread_LDFLAGS = -pthread
