/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */
#ifndef ARDUINO
#include <errno.h>
#endif

#include "nrf24l01_proto_net.h"

#ifndef __NRF24L01_CLIENT_H__
#define __NRF24L01_CLIENT_H__

#ifdef __cplusplus
extern "C"{
#endif

int nrf24l01_client_open(int socket, int channel, version_t *pversion);
int nrf24l01_client_close(int socket);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __NRF24L01_CLIENT_H__
