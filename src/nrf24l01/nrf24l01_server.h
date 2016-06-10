/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */
#include <stdlib.h>
#include <sys/socket.h>
#include <inttypes.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/un.h>
#include <glib.h>

#include "nrf24l01.h"
#include "util.h"
#include "list.h"

#include "nrf24l01_proto_net.h"

#ifndef __NRF24L01_SERVER_H__
#define __NRF24L01_SERVER_H__

#ifdef __cplusplus
extern "C"{
#endif

/* Abstract unit socket namespace */
#define KNOT_UNIX_SOCKET				"knot_nrf24l01"
#define KNOT_UNIX_SOCKET_SIZE		(sizeof(KNOT_UNIX_SOCKET) - 1)

int nrf24l01_server_open(int socket, int channel, version_t *pversion);
int nrf24l01_server_close(int socket);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __NRF24L01_SERVER_H__
