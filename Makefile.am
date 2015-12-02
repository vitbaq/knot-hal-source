AM_MAKEFLAGS = --no-print-directory

include_HEADERS =
modules_sources =
modules_ldadd =
modules_cflags =
INCLUDES =

AM_CFLAGS = $(WARNING_CFLAGS) $(BUILD_CFLAGS) @GLIB_CFLAGS@
AM_LDFLAGS = $(BUILD_LDFLAGS)

libexec_PROGRAMS = src/nrf24l01_driver

include Makefile.modules

src_nrf24l01_driver_SOURCES = src/nrf24l01_driver.c \
			src/nrf24l01/nrf24l01.c \
			src/spi/spi_linux.c	\
			$(modules_sources)
src_nrf24l01_driver_LDADD = @GLIB_LIBS@ $(modules_ldadd)
src_nrf24l01_driver_LDFLAGS = $(AM_LDFLAGS)
src_nrf24l01_driver_CFLAGS = $(AM_CFLAGS) $(modules_cflags)

DISTCLEANFILES =

MAINTAINERCLEANFILES = Makefile.in \
	aclocal.m4 configure config.h.in config.sub config.guess \
	ltmain.sh depcomp compile missing install-sh

clean-local:
	$(RM) -r src/nrf24l01_driver
