SUBDIRS = src

bin_PROGRAMS = testcpp testc

testcpp_SOURCES = test.cpp
testcpp_LDADD = libs/libnrf24l01_driver.a libs/libnrf24l01.a libs/libspi.a  @GLIB_LIBS@
testcpp_CPPFLAGS = -I./include -I./src -std=c++11 @GLIB_CFLAGS@

testc_SOURCES = test.c
testc_LDADD = libs/libnrf24l01_driver.a libs/libnrf24l01.a libs/libspi.a  @GLIB_LIBS@
testc_CPPFLAGS = -I./include -I./src  @GLIB_CFLAGS@

clean-local:
	$(RM) -r testcpp testc
