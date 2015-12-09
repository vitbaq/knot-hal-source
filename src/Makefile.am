SUBDIRS = nrf24l01 spi

lib_LIBRARIES = libabstract_driver.a \
								libnrf24l01_driver.a

libabstract_driver_a_SOURCES = abstract_driver.c

libnrf24l01_driver_a_SOURCES = nrf24l01_driver.c
libnrf24l01_driver_a_CPPFLAGS = -I$(top_srcdir)/src/nrf24l01

clean-local:
	$(RM) -r libabstract_driver.a libnrf24l01_driver.a
															    