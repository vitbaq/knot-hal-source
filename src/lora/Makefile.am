lib_LIBRARIES = libsx127x.a
AM_CFLAGS = $(WARNING_CFLAGS) $(BUILD_CFLAGS)
lib_ARDUINO = sx127x.c sx127x.h

libsx127x_a_SOURCES = sx127x.c
libsx127x_a_CPPFLAGS = -I$(top_srcdir)/src/ -I$(top_srcdir)/src/spi

all-local:
	$(MKDIR_P) $(top_srcdir)/libs && cp $(lib_LIBRARIES) $(top_srcdir)/libs
	$(MKDIR_P) $(top_srcdir)/hal/arduino && cp $(lib_ARDUINO) $(top_srcdir)/hal/arduino

clean-local:
	$(RM) -r libsx127x.a