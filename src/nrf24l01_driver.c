/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */
#include <stdio.h>

#include "abstract_driver.h"
#include "nrf24l01.h"
//#include "nrf24l01_client.h"

#ifndef ARDUINO
#include "nrf24l01_server.h"
#else
int errno = SUCCESS;
#endif

#define NRF24L01_DRIVER_NAME		"nRF24L01 driver"

#define NRF24L01_CHANNEL				NRF24L01_CHANNEL_DEFAULT

enum {
	eINVALID,
	eUNKNOWN,
	eSERVER,
	eCLIENT
} ;

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
	m_fd = eventfd(0, EFD_CLOEXEC);
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
	if (socket == m_fd) {
		close(m_fd);
		m_fd = SOCKET_INVALID;
		m_state =  eUNKNOWN;
	}
	if (state == eCLIENT) {
		//return nrf24l01_client_close(socket);
	}
	if (state == eSERVER) {
		return nrf24l01_server_close(socket);
	}
#else
	if (socket == m_fd) {
		m_fd = SOCKET_INVALID;
		m_state =  eUNKNOWN;
	}
	if (state == eCLIENT) {
		//return nrf24l01_client_close(socket);
	}
#endif
	return SUCCESS;
}

static int nrf24_listen(int socket)
{
#ifndef ARDUINO
	int result;

	if (m_fd == SOCKET_INVALID || socket != m_fd) {
		errno = EBADF;
		return ERROR;
	}
	if (m_state != eUNKNOWN) {
		errno = EACCES;
		return ERROR;
	}

	result = nrf24l01_server_open(socket, NRF24L01_CHANNEL);
	if (result == SUCCESS) {
		m_state = eSERVER;
	}
	return  result;
#else
	errno = EPERM;
	return ERROR;
#endif
}

static int nrf24_accept(int socket)
{
#ifndef ARDUINO
	if (m_fd == SOCKET_INVALID || socket != m_fd) {
		errno = EBADF;
		return ERROR;
	}
	if (m_state != eSERVER) {
		errno = EACCES;
		return ERROR;
	}

	return nrf24l01_server_accept(socket);
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
		return nrf24l01_server_available(socket);
	}
	errno = EBADF;
	return ERROR;
#else
	return SUCCESS;
#endif
}

static size_t nrf24_recv(int socket, void *buffer, size_t len)
{
	if (socket == SOCKET_INVALID) {
		errno = EBADF;
		return ERROR;
	}
	if (socket == m_fd && m_state != eCLIENT) {
		errno = EACCES;
		return ERROR;
	}

	return 0;
}

static size_t nrf24_send(int socket, const void *buffer, size_t len)
{
	if (socket == SOCKET_INVALID) {
		errno = EBADF;
		return ERROR;
	}
	if (socket == m_fd && m_state != eCLIENT) {
		errno = EACCES;
		return ERROR;
	}

	return 0;
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

static void nrf24_service(void)
{
	if (m_state == eCLIENT) {
		//nrf24l01_client_service();
	}
#ifndef ARDUINO
	if (m_state == eSERVER) {
		nrf24l01_server_service();
	}
#endif
}

/*
 * HAL interface
 */
abstract_driver_t nrf24l01_driver = {
	.socket = nrf24_socket,
	.close = nrf24_close,
	.listen = nrf24_listen,
	.accept = nrf24_accept,
	.connect = nrf24_connect,

	.available = nrf24_available,
	.recv = nrf24_recv,
	.send = nrf24_send,

	.name = NRF24L01_DRIVER_NAME,
	.probe = nrf24_probe,
	.remove = nrf24_remove,
	.service = nrf24_service
};
