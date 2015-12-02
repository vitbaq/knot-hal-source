/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <abstract_driver.h>
#include <nrf24l01.h>

#define NRF24_DRIVER_NAME		"nRF24l01 driver"

int nrf24_probe()
{
	return (nrf24l01_init() == 0 ? DRV_SUCCESS : DRV_DONE);
}

void nrf24_remove()
{
	nrf24l01_deinit();
}

int nrf24_socket()
{
	return DRV_SOCKET_FD_INVALID;
}

void nrf24_close(int socket)
{

}

int nrf24_connect(int socket, uint8_t to_addr)
{
	return DRV_ERROR;
}

int nrf24_accept(int socket)
{
	return DRV_ERROR;
}

int nrf24_available(int sockfd)
{
	return 0;
}

size_t nrf24_recv (int sockfd, void *buffer, size_t len)
{
	return 0;
}

size_t nrf24_send (int sockfd, const void *buffer, size_t len)
{
	return 0;
}

abstract_driver driver_nrf24 = {
	.name = NRF24_DRIVER_NAME,
	.valid = 1,

	.probe = nrf24_probe,
	.remove = nrf24_remove,

	.socket = nrf24_socket,
	.close = nrf24_close,
	.accept = nrf24_accept,
	.connect = nrf24_connect,

	.available = nrf24_available,
	.recv = nrf24_recv,
	.send = nrf24_send
};
