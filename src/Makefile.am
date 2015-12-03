lib_LIBRARIES = libabstract_driver.a \
								libnrf24l01_driver.a

libabstract_driver_a_SOURCES = abstract_driver.c
															   abstract_driver.h

libnrf24l01_driver_a_SOURCES = nrf24l01_driver.c \
															    abstract_driver.h \
															    nrf24l01/nrf24l01.c \
															    nrf24l01/nrf24l01.h \
															    nrf24l01/nrf24l01_io.h \
															    spi/spi_linux.c \
															    spi/spi.h

clean-local:
	$(RM) -r src/libabstract_driver.a src/libnrf24l01_driver.a
															    