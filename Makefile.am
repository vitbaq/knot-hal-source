SUBDIRS = src

bin_PROGRAMS = testcpp testc

testcpp_SOURCES = test.cpp
testcpp_LDADD = src/libnrf24l01_driver.a
testcpp_CPPFLAGS =

testc_SOURCES = test.c
testc_LDADD = src/libabstract_driver.a
testc_CPPFLAGS =

DISTCLEANFILES =

MAINTAINERCLEANFILES = Makefile.in \
	aclocal.m4 configure config.h.in config.sub config.guess \
	ltmain.sh depcomp compile missing install-sh

clean-local:
	$(RM) -r testcpp testc
