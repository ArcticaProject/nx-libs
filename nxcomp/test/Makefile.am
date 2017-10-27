NULL =

noinst_PROGRAMS = logging_test
EXTRA_DIST = logging_test

AM_CPPFLAGS = -I$(top_srcdir)/src
AM_CXXFLAGS =		\
  @PTHREAD_CFLAGS@	\
  $(NULL)

logging_test_SOURCES = logging_test.cpp
logging_test_LDADD =			\
    $(top_srcdir)/src/.libs/libXcomp.a	\
    @PTHREAD_LIBS@			\
    $(NULL)

logging_test_LDFLAGS =	\
    $(PTHREAD_LDFLAGS)	\
    $(NULL)

check: all
	./logging_test
