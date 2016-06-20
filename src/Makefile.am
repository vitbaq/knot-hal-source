SUBDIRS = nrf24l01 spi

lib_LIBRARIES = libnrf24l01_driver.a
lib_ARDUINO = abstract_driver.h \
							  printf_serial.h debug.h \
							  wd/sleep_wdt.h \
							  nrf24l01_driver.c nrf24l01_proto_net.h \
							  util.c util.h atypes.h \
							  wd/sleep_wdt.c

libnrf24l01_driver_a_SOURCES = nrf24l01_driver.c
libnrf24l01_driver_a_CPPFLAGS = -I$(top_srcdir)/src/nrf24l01

all-local:
	$(MKDIR_P) $(top_srcdir)/libs && cp $(lib_LIBRARIES) $(top_srcdir)/libs
	$(MKDIR_P) $(top_srcdir)/include/arduino && cp $(lib_ARDUINO) $(top_srcdir)/include/arduino
 	   
clean-local:
	$(RM) -r libnrf24l01_driver.a
	$(RM) -r $(top_srcdir)/libs
	$(RM) -r $(top_srcdir)/include
															    
