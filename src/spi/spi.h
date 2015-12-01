/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */
#include "../types.h"

#ifndef __SPI_H__
#define __SPI_H__

#ifdef __cplusplus
extern "C"{
#endif

/*
 * spi_init - initialize the SPI device.
 */
void spi_init(void);

void spi_deinit(void);

/*
 * spi_transfer - sends and receives data buffer by a txd buffer with txl bytes lenght and rxd buffer with rxl bytes length.
 */
bool spi_transfer(void *ptx, len_t ltx, void *prx, len_t lrx);

#ifdef __cplusplus
} // extern "C"
#endif

#endif		// __SPI_H__
