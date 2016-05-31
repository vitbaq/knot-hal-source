/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */
#ifndef ARDUINO
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#endif

#ifndef __ABSTRACT_DRIVER_H__
#define __ABSTRACT_DRIVER_H__

// invalid socket fd
#define SOCKET_INVALID		-1

// operation status codes
#define SUCCESS		0
#define ERROR			-1

#ifdef __cplusplus
extern "C"{
#endif

#ifdef ARDUINO
#define	EPERM			1		/* Operation not permitted */
#define	EBADF			9		/* Bad file number */
#define	EAGAIN			11	/* Try again */
#define	ENOMEM		12	/* Out of memory */
#define	EACCES		13	/* Permission denied */
#define	EFAULT			14	/* Bad address */
#define	EINVAL			22	/* Invalid argument */
#define	EMFILE			24	/* Too many open files */
extern int errno;
#endif

 /**
 * struct abstract_driver - driver abstraction for the physical layer
 * @name: driver name
 * @probe: function to initialize the driver
 * @remove: function to close the driver and free its resources
 * @socket: function to create a new socket FD
 * @listen: function to enable incoming connections
 * @accept: function that waits for a new connection and returns a 'pollable' FD
 * @connect: function to connect to a server socket
 *
 * This 'driver' intends to be an abstraction for Radio technologies or
 * proxy for other services using TCP or any socket based communication.
 */

typedef struct {
	const char *name;
	int (*probe)(void);
	void (*remove)(void);

	int (*socket)(void);
	int (*close)(int sockfd);
	int (*listen)(int sockfd, int backlog);
	int (*connect)(int cli_sockfd, const void *addr, size_t len);
	int (*available)(int sockfd);
	void (*service)(void);
	ssize_t (*read)(int sockfd, void *buffer, size_t len);
	ssize_t (*write)(int sockfd, const void *buffer, size_t len);
} abstract_driver_t;

extern abstract_driver_t nrf24l01_driver;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __ABSTRACT_DRIVER_H__
