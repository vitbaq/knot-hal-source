/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */
#ifndef ARDUINO
#include <stdlib.h>
#include <errno.h>
#endif

#include "nrf24l01.h"
#include "util.h"

#include "nrf24l01_proto_net.h"

#ifndef __NRF24L01_CLIENT_H__
#define __NRF24L01_CLIENT_H__

#ifdef __cplusplus
extern "C"{
#endif

int_t nrf24l01_client_open(int_t socket, byte_t channel, version_t *pversion);
int_t nrf24l01_client_close(int_t socket);
int_t nrf24l01_client_read(int_t socket, byte_t *buffer, len_t len);
int_t nrf24l01_client_write(int_t socket, const byte_t *buffer, len_t len);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __NRF24L01_CLIENT_H__
