/*
 * Copyright (c) 2017, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include "include/comm.h"

int hal_comm_init(const char *pathname, void *params)
{
	return 0;
}

int hal_comm_deinit(void)
{
	return 0;
}

int hal_comm_socket(int domain, int protocol)
{
	return 0;
}

int hal_comm_close(int sockfd)
{
	return 0;
}

ssize_t hal_comm_read(int sockfd, void *buffer, size_t count)
{
	return 0;
}

ssize_t hal_comm_write(int sockfd, const void *buffer, size_t count)
{
	return 0;
}

int hal_comm_listen(int sockfd)
{
	return 0;
}

int hal_comm_accept(int sockfd, void *addr)
{
	return 0;
}

int hal_comm_connect(int sockfd, uint64_t *addr)
{
	return 0;
}
