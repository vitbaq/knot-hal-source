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
#include <fcntl.h>
#endif
#include "nrf24l01_client.h"

#include "abstract_driver.h"

#define NRF24L01_DRIVER_NAME		"nRF24L01 driver"
// protocol version
#define NRF24_VERSION_MAJOR	1
#define NRF24_VERSION_MINOR	0

#define POLLTIME_MS			10

enum {
	INVALID,
	UNKNOWN,
	SERVER,
	CLIENT
} e_state;

#ifdef ARDUINO
int errno = SUCCESS;
#endif

static int	m_state = INVALID,
					m_fd = SOCKET_INVALID;

static version_t	m_version =  {	major: NRF24_VERSION_MAJOR,
															minor: NRF24_VERSION_MINOR,
															packet_size: 0
};

/*
 * HAL functions
 */
static int nrf24_socket(void)
{
	if (m_state == INVALID) {
		errno = EACCES;
		return ERROR;
	}

	if (m_fd != SOCKET_INVALID) {
		errno = EMFILE;
		return ERROR;
	}

#ifndef ARDUINO
	m_fd = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
	if (m_fd < 0) {
		m_fd = SOCKET_INVALID;
		return ERROR;
	}
#else
	m_fd = STDERR_FILENO+1;
#endif
	return m_fd;
}

static int nrf24_close(int socket)
{
	if (m_state == INVALID) {
		errno = EACCES;
		return ERROR;
	}

	if (socket == SOCKET_INVALID) {
		errno = EBADF;
		return ERROR;
	}

	if (m_state == CLIENT) {
		nrf24l01_client_close(socket);
	}
#ifndef ARDUINO
	if (m_state == SERVER) {
		nrf24l01_server_close(socket);
	}
	close(socket);
#endif
	if (socket == m_fd) {
		m_fd = SOCKET_INVALID;
		m_state =  UNKNOWN;
	}
	return SUCCESS;
}

static int nrf24_listen(int socket, int channel, const void *paddr, size_t laddr)
{
#ifndef ARDUINO
	int result;
	struct sockaddr_un addr;

	if (m_state != UNKNOWN) {
		errno = EACCES;
		return ERROR;
	}

	if (m_fd == SOCKET_INVALID || socket != m_fd) {
		errno = EBADF;
		return ERROR;
	}

	if (channel < CH_MIN || channel > CH_MAX_1MBPS || paddr == NULL || laddr == 0) {
		errno = EINVAL;
		return ERROR;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	/* Abstract namespace: first character must be null */
	strncpy(addr.sun_path+1, (const char *__restrict)paddr, laddr);
	if (bind(socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		return ERROR;
	}

	if (listen(m_fd, 5) < 0) {
		return ERROR;
	}

	result = nrf24l01_server_open(socket, channel, &m_version, paddr, laddr);
	if (result == SUCCESS) {
		m_state = SERVER;
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

	if (m_state != UNKNOWN) {
		errno = EACCES;
		return ERROR;
	}

	if (m_fd == SOCKET_INVALID || socket != m_fd) {
		errno = EBADF;
		return ERROR;
	}

	if (len != sizeof(int) || *((int*)addr) < CH_MIN || *((int*)addr) > CH_MAX_1MBPS) {
		errno = EINVAL;
		return ERROR;
	}

	result = nrf24l01_client_open(socket, *((int*)addr), &m_version);
	if (result == SUCCESS) {
		m_state = CLIENT;
	}
	return  result;
}

static int nrf24_read(int socket, void *buffer, size_t len)
{
	if (m_state <= UNKNOWN) {
		errno = EACCES;
		return ERROR;
	}

#ifndef ARDUINO
	if (m_state == SERVER) {
		return read(socket, buffer, len);
	}
#endif

	return nrf24l01_client_read(socket, buffer, len);
}

static int nrf24_write(int socket, const void *buffer, size_t len)
{
	if (m_state <= UNKNOWN) {
		errno = EACCES;
		return ERROR;
	}

#ifndef ARDUINO
	if (m_state == SERVER) {
		return write(socket, buffer, len);
	}
#endif

	return nrf24l01_client_write(socket, (const byte_t*)buffer, len);
}

static int nrf24_probe(size_t packet_size)
{
	if(m_state > INVALID) {
		return SUCCESS;
	}

	if (packet_size == 0 || packet_size > UINT16_MAX) {
		errno = EINVAL;
		return ERROR;
	}

	m_version.packet_size = packet_size;
	m_state = (nrf24l01_init() == SUCCESS) ? UNKNOWN : INVALID;
	if(m_state == INVALID) {
		errno = EIO;
	}
	return (m_state == UNKNOWN ? SUCCESS : ERROR);
}

static void nrf24_remove(void)
{
	if (m_state != INVALID) {
		nrf24_close(m_fd);
		nrf24l01_deinit();
		m_state = INVALID;
	}
}

/*
 * HAL interface
 */
abstract_driver_t nrf24l01_driver = {
	name: NRF24L01_DRIVER_NAME,
	probe: nrf24_probe,
	remove: nrf24_remove,
	socket: nrf24_socket,
	close: nrf24_close,
	listen:  nrf24_listen,
	connect: nrf24_connect,
	read: nrf24_read,
	write: nrf24_write
};
