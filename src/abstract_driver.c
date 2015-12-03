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

int empty_probe()
{
	return DRV_ERROR;
}

void empty_remove()
{
}

int empty_socket()
{
	return DRV_SOCKET_FD_INVALID;
}

void empty_close(int socket)
{
}

int empty_connect(int socket, const void *addr, size_t len)
{
	return DRV_ERROR;
}

int empty_accept(int socket)
{
	return DRV_ERROR;
}

int empty_available(int sockfd)
{
	return DRV_ERROR;
}

size_t empty_recv (int sockfd, void *buffer, size_t len)
{
	return 0;
}

size_t empty_send (int sockfd, const void *buffer, size_t len)
{
	return 0;
}

abstract_driver_t driver = {
	.name = EMPTY_DRIVER_NAME,
	.valid = 0,

	.probe = empty_probe,
	.remove = empty_remove,

	.socket = empty_socket,
	.close = empty_close,
	.accept = empty_accept,
	.connect = empty_connect,

	.available = empty_available,
	.recv = empty_recv,
	.send = empty_send
};
