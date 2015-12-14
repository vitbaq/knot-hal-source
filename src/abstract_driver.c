/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include "abstract_driver.h"

#define EMPTY_DRIVER_NAME	"Empty driver"

/*
 * HAL functions
 */
static int empty_socket()
{
	return SOCKET_INVALID;
}

static int empty_close(int socket)
{
	return SOCKET_INVALID;
}

static int empty_connect(int socket, const void *addr, size_t len)
{
	return AD_ERROR;
}

static int empty_accept(int socket)
{
	return AD_ERROR;
}

static int empty_available(int sockfd)
{
	return AD_ERROR;
}

static size_t empty_recv (int sockfd, void *buffer, size_t len)
{
	return 0;
}

static size_t empty_send (int sockfd, const void *buffer, size_t len)
{
	return 0;
}

static int empty_probe()
{
	return AD_ERROR;
}

static void empty_remove()
{
}

/*
 * HAL interface
 */
abstract_driver_t abstract_driver = {
	.socket = empty_socket,
	.close = empty_close,
	.accept = empty_accept,
	.connect = empty_connect,

	.available = empty_available,
	.recv = empty_recv,
	.send = empty_send,

	.name = EMPTY_DRIVER_NAME,
	.probe = empty_probe,
	.remove = empty_remove
};
