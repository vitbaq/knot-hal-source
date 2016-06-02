/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */
#ifndef ARDUINO
#define _GNU_SOURCE         /* For POLLRDHUP */
#include <poll.h>
#include "nrf24l01_server.h"
#endif

#include "nrf24l01_proto_net.h"
//#include "nrf24l01_client.h"
#include "nrf24l01.h"

#include "abstract_driver.h"

// protocol version
#define NRF24_VERSION_MAJOR	1
#define NRF24_VERSION_MINOR	0
// application packet size maximum
#define APP_PACKET_SIZE_MAX		128

#define NRF24L01_DRIVER_NAME		"nRF24L01 driver"

#define POLLTIME_MS			10

enum {
	eINVALID,
	eUNKNOWN,
	eSERVER,
	eCLIENT
} ;

static version_t version =  { NRF24_VERSION_MAJOR,
									   	   	   	   NRF24_VERSION_MINOR,
									   	   	   	   APP_PACKET_SIZE_MAX};

#ifdef ARDUINO
int errno = SUCCESS;
#endif

static int	m_state = eINVALID,
					m_fd = SOCKET_INVALID;

/*
 * HAL functions
 */
static int nrf24_socket(void)
{
	if (m_state == eINVALID || m_fd != SOCKET_INVALID) {
		errno = EACCES;
		return ERROR;
	}

#ifndef ARDUINO
	m_fd = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
	if (m_fd < 0) {
		return ERROR;
	}
#else
	m_fd = 0;
#endif
	return m_fd;
}

static int nrf24_close(int socket)
{
	int	state = m_state;
	if (socket == SOCKET_INVALID) {
		errno = EBADF;
		return ERROR;
	}

#ifndef ARDUINO
	if (state == eSERVER) {
		return nrf24l01_server_close(socket);
	} else if (state == eCLIENT) {
		//return nrf24l01_client_close(socket);
	}
	close(socket);
	if (socket == m_fd) {
		m_fd = SOCKET_INVALID;
		m_state =  eUNKNOWN;
	}
#else
	if (state == eCLIENT) {
		//return nrf24l01_client_close(socket);
	}
	if (socket == m_fd) {
		m_fd = SOCKET_INVALID;
		m_state =  eUNKNOWN;
	}
#endif
	return SUCCESS;
}

static int nrf24_listen(int socket, int channel)
{
#ifndef ARDUINO
	int result;
	struct sockaddr_un addr;

	if (m_fd == SOCKET_INVALID || socket != m_fd) {
		errno = EBADF;
		return ERROR;
	}
	if (m_state != eUNKNOWN) {
		errno = EACCES;
		return ERROR;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	/* Abstract namespace: first character must be null */
	strncpy(addr.sun_path + 1, KNOT_UNIX_SOCKET, KNOT_UNIX_SOCKET_SIZE);
	if (bind(m_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		return ERROR;
	}

	if (listen(m_fd, 5) < 0) {
		return ERROR;
	}

	if(channel < CH_MIN)
		channel = NRF24L01_CHANNEL_DEFAULT;

	result = nrf24l01_server_open(socket, channel, &version);
	if (result == SUCCESS) {
		m_state = eSERVER;
	}
	return  result;
#else
	errno = EPERM;
	return ERROR;
#endif
}

static int nrf24_connect(int socket, const void *addr, size_t len)
{
	int result;

	if (m_fd == SOCKET_INVALID || socket != m_fd) {
		errno = EBADF;
		return ERROR;
	}
	if (m_state != eUNKNOWN) {
		errno = EACCES;
		return ERROR;
	}

	//result = nrf24l01_client_open(socket);
	if (result == SUCCESS) {
		m_state = eCLIENT;
	}
	return  result;
}

static int nrf24_available(int socket)
{
	if (socket == SOCKET_INVALID) {
		errno = EBADF;
		return ERROR;
	}
	if (m_state == eUNKNOWN) {
		errno = EACCES;
		return ERROR;
	}

	if (m_state == eCLIENT) {
		//return nrf24l01_client_available(socket);
	}
#ifndef ARDUINO
	if (m_state == eSERVER) {
		struct pollfd fd_poll;

		fd_poll.fd = socket;
		fd_poll.events = POLLIN | POLLPRI | POLLRDHUP;
		fd_poll.revents = 0;
		//returns: 1 for socket available,
		//				 0 for no available,
		//				 or -1 for errors.
		return poll(&fd_poll, 1, POLLTIME_MS);
	}
#endif
	return 0;
}

static void nrf24_service(void)
{
	if (m_state == eCLIENT) {
		//nrf24l01_client_service();
	}
#ifndef ARDUINO
	if (m_state == eSERVER) {
		//nrf24l01_server_service();
	}
#endif
}

static int nrf24_probe(void)
{
	if(m_state)	{
		return SUCCESS;
	}
	m_state = (nrf24l01_init() == SUCCESS) ? eUNKNOWN : eINVALID;
	return (m_state == eUNKNOWN ? SUCCESS : ERROR);
}

static void nrf24_remove(void)
{
	if (m_state != eINVALID) {
		nrf24_close(m_fd);
		nrf24l01_deinit();
		m_state = eINVALID;
	}
}

/*
 * HAL interface
 */
abstract_driver_t nrf24l01_driver = {
	.name = NRF24L01_DRIVER_NAME,
	.probe = nrf24_probe,
	.remove = nrf24_remove,

	.socket = nrf24_socket,
	.close = nrf24_close,
	.listen = nrf24_listen,
	.connect = nrf24_connect,
	.available = nrf24_available,
	.service = nrf24_service
};
