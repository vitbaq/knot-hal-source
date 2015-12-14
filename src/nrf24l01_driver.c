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
#include "nrf24l01_proto_net.h"
#include "nrf24l01.h"

#define NRF24_DRIVER_NAME		"nRF24L01 driver"

static bool m_binit = false;
static int m_sockfd = SOCKET_INVALID;

/*
 * HAL functions
 */
static int nrf24_socket()
{
	if (!m_binit || m_sockfd != SOCKET_INVALID)
		return AD_ERROR;

	m_sockfd = 0;
	return m_sockfd;
}

static int nrf24_close(int socket)
{
	if (socket != m_sockfd)
		return AD_ERROR;

	m_sockfd = SOCKET_INVALID;
	return AD_SUCCESS;
}

static int nrf24_connect(int socket, const void *addr, size_t len)
{
	if (socket == SOCKET_INVALID || socket != m_sockfd)
		return AD_ERROR;

	return AD_ERROR;
}

static int nrf24_accept(int socket)
{
#ifndef ARDUINO
	if (socket == SOCKET_INVALID || socket != m_sockfd)
		return AD_ERROR;

	return AD_ERROR;
#else
	return AD_ERROR;
#endif
}

static int nrf24_available(int socket)
{
	if (socket == SOCKET_INVALID)
		return AD_ERROR;

	return AD_SUCCESS;
}

static size_t nrf24_recv (int socket, void *buffer, size_t len)
{
	if (socket == SOCKET_INVALID)
		return AD_ERROR;

	return 0;
}

static size_t nrf24_send (int socket, const void *buffer, size_t len)
{
	if (socket == SOCKET_INVALID)
		return AD_ERROR;

	return 0;
}

static int nrf24_probe()
{
	nrf24_payload pld;
	printf("sizeof(nrf24_payload)=%d pld.data[%d] size=%d\n", (int)sizeof(pld), (int)(pld.msg.data-(uint8_t*)&pld), (int)NRF24_MSG_PW_SIZE);

	m_binit = (nrf24l01_init() != AD_ERROR);
	return (m_binit ? AD_SUCCESS : AD_ERROR);
}

static void nrf24_remove()
{
	if (m_binit) {
		m_binit = false;
		nrf24_close(m_sockfd);
		nrf24l01_deinit();
	}
}

/*
 * HAL interface
 */
abstract_driver_t nrf24l01_driver = {
	.socket = nrf24_socket,
	.close = nrf24_close,
	.accept = nrf24_accept,
	.connect = nrf24_connect,

	.available = nrf24_available,
	.recv = nrf24_recv,
	.send = nrf24_send,

	.name = NRF24_DRIVER_NAME,
	.probe = nrf24_probe,
	.remove = nrf24_remove
};
