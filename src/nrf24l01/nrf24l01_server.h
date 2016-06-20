/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */
#include <sys/socket.h>
#include <inttypes.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/un.h>

#include "nrf24l01.h"
#include "util.h"
#include "list.h"

#include "nrf24l01_proto_net.h"

#ifndef __NRF24L01_SERVER_H__
#define __NRF24L01_SERVER_H__

#ifdef __cplusplus
extern "C"{
#endif

int nrf24l01_server_open(int socket, int channel, version_t *pversion, const void *pstr_addr, size_t lstr_addr);
int nrf24l01_server_close(int socket);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __NRF24L01_SERVER_H__
