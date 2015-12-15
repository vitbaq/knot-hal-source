SUBDIRS = nrf24l01 spi

AM_CFLAGS = $(WARNING_CFLAGS) $(BUILD_CFLAGS) @GLIB_CFLAGS@

lib_LIBRARIES = libnrf24l01_driver.a
lib_HEADER = abstract_driver.h

libnrf24l01_driver_a_SOURCES = nrf24l01_driver.c
libnrf24l01_driver_a_CPPFLAGS = -I$(top_srcdir)/src/nrf24l01

all-local:
	$(MKDIR_P) $(top_srcdir)/libs
	cp $(lib_LIBRARIES) $(top_srcdir)/libs
	$(MKDIR_P) $(top_srcdir)/include
	cp $(lib_HEADER) $(top_srcdir)/include
 	   
clean-local:
	$(RM) -r libnrf24l01_driver.a
	$(RM) $(top_srcdir)/libs/*.a
															    
