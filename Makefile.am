SUBDIRS = src

bin_PROGRAMS = testcpp testc

testcpp_SOURCES = test.cpp
testcpp_LDADD = src/libnrf24l01_driver.a src/nrf24l01/libnrf24l01.a src/spi/libspi.a
testcpp_CPPFLAGS = -I./src

testc_SOURCES = test.c
testc_LDADD = src/libabstract_driver.a
testc_CPPFLAGS = -I./src

DISTCLEANFILES =

MAINTAINERCLEANFILES = Makefile.in \
	aclocal.m4 configure config.h.in config.sub config.guess \
	ltmain.sh depcomp compile missing install-sh

clean-local:
	$(RM) -r testcpp testc
