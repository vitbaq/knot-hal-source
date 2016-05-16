/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */
#ifndef ARDUINO
#include <sys/eventfd.h>
#include <errno.h>
#include <unistd.h>

#ifndef __NRF24L01_SERVER_H__
#define __NRF24L01_SERVER_H__

#ifdef __cplusplus
extern "C"{
#endif

int nrf24l01_server_open(int socket, int channel);
int nrf24l01_server_close(int socket);
int nrf24l01_server_accept(int socket);
int nrf24l01_server_available(int socket);
int nrf24l01_server_cancel(int socket);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // #ifndef ARDUINO

#endif // __NRF24L01_SERVER_H__
