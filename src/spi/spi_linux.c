/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include "spi.h"

#ifndef ARDUINO

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <stdio.h>
#include <stdlib.h>

#define LOW							0
#define HIGH							1
#define DELAY_US				5	// delay for CSN
#define SPI_SPEED				1000000
#define BITS_PER_WORD	8

static int	m_spi_fd = -1;

void spi_init(void)
{
	unsigned char mode = SPI_MODE_0;

	if(m_spi_fd < 0) {
#ifdef RASPBERRY
		m_spi_fd = open("/dev/spidev0.0", O_RDWR);
#else
		m_spi_fd = open("/dev/spidev5.1", O_RDWR);
#endif
		if(m_spi_fd > 0) {
			ioctl(m_spi_fd, SPI_IOC_WR_MODE, &mode);
			ioctl(m_spi_fd, SPI_IOC_RD_MODE, &mode);
		}
	}
}

void spi_deinit(void)
{
	if(m_spi_fd > 0) {
		close(m_spi_fd);
		m_spi_fd = -1;
	}
}

bool spi_transfer(void *ptx, len_t ltx, void *prx, len_t lrx)
{
	struct spi_ioc_transfer data_ioc[2],
											   *pdata_ioc = data_ioc;
	uint8_t *pdummy = NULL;
	int ntransfer = 0;

	if(m_spi_fd < 0) {
		return 0;
	}

	memset(data_ioc, 0, sizeof(data_ioc));

	if(txd != NULL && ltx != 0) {
		pdummy = (uint8_t*)malloc(ltx);
		if(pdummy == NULL)
			return 0;

		pdata_ioc->tx_buf = (unsigned long)ptx;
		pdata_ioc->rx_buf = (unsigned long)pdummy;
		pdata_ioc->len = ltx;
		pdata_ioc->delay_usecs = (prx != NULL && lrx != 0) ? 0 : DELAY_US;
		pdata_ioc->cs_change = (prx != NULL && lrx != 0) ? LOW : HIGH;
		pdata_ioc->speed_hz = SPI_SPEED;
		pdata_ioc->bits_per_word = BITS_PER_WORD;
		++ntransfer;
		++pdata_ioc;
	}

	if(prx != NULL && lrx != 0) {
		pdata_ioc->tx_buf = (unsigned long)prx;
		pdata_ioc->rx_buf = (unsigned long)prx;
		pdata_ioc->len = lrx;
		pdata_ioc->delay_usecs = DELAY_US;
		pdata_ioc->cs_change = HIGH;
		pdata_ioc->speed_hz = SPI_SPEED;
		pdata_ioc->bits_per_word = BITS_PER_WORD;
		++ntransfer;
	}

	ioctl(m_spi_fd, SPI_IOC_MESSAGE(ntransfer), data_ioc);

	free(pdummy);

	return 1;
}

#endif
