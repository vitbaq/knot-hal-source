SUBDIRS = nrf24l01 spi

AM_CFLAGS = $(WARNING_CFLAGS) $(BUILD_CFLAGS) @GLIB_CFLAGS@

lib_LIBRARIES = libnrf24l01_driver.a
lib_HEADER = abstract_driver.h
lib_ARDUINO = nrf24l01_driver.c nrf24l01_proto_net.h \
							  nrf24l01/nrf24l01.c nrf24l01/nrf24l01.h nrf24l01/nrf24l01_io.h \
							  spi/spi_arduino.c spi/spi.h \
							  types.h printf_serial.h \
							  wd/sleep_wdt.c wd/sleep_wdt.h

libnrf24l01_driver_a_SOURCES = nrf24l01_driver.c
libnrf24l01_driver_a_CPPFLAGS = -I$(top_srcdir)/src/nrf24l01

all-local:
	$(MKDIR_P) $(top_srcdir)/libs
	cp $(lib_LIBRARIES) $(top_srcdir)/libs
	$(MKDIR_P) $(top_srcdir)/include
	cp $(lib_HEADER) $(top_srcdir)/include
	$(MKDIR_P) $(top_srcdir)/include/arduino
	cp $(lib_ARDUINO)  $(top_srcdir)/include/arduino
 	   
clean-local:
	$(RM) -r libnrf24l01_driver.a
	$(RM) -r $(top_srcdir)/libs
	$(RM) -r $(top_srcdir)/include
															    
