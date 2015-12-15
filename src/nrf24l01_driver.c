/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */
#include <stdio.h>

#ifndef ARDUINO
#include <unistd.h>
#include <sys/eventfd.h>
#include <errno.h>
#include <glib.h>
#endif

#include "abstract_driver.h"
#include "nrf24l01_proto_net.h"
#include "nrf24l01.h"

#define NRF24L01_DRIVER_NAME		"nRF24L01 driver"

static bool m_binit = false;
static int m_srvfd = SOCKET_INVALID;

#ifndef ARDUINO
enum state {
	fdUNKNOWN,
	fdCLOSE,
	fdOPEN,
	fdUSE
} ;

typedef struct  {
	int	clifd;
	int	state;
} client_t;

static GSList *m_pclients = NULL;
static eventfd_t m_naccept = 0;

static inline void clients_free(gpointer pentry)
{
	close(((client_t*)pentry)->clifd);
	g_free(pentry);
}

static inline gint clifd_match(gconstpointer pda, gconstpointer pdb)
{
	return ((const client_t*)pda)->clifd - ((const client_t*)pdb)->clifd;
}

static inline gint state_match(gconstpointer pda, gconstpointer pdb)
{
	return ((const client_t*)pda)->state - ((const client_t*)pdb)->state;
}

#endif

/*
 * HAL functions
 */
static int nrf24_socket(void)
{
	if (!m_binit || m_srvfd != SOCKET_INVALID)
		return AD_ERROR;

#ifndef ARDUINO
	m_srvfd = eventfd(0, EFD_CLOEXEC);
	if (m_srvfd < 0) {
		return -errno;
	}
#else
	m_srvfd = 0;
#endif
	return m_srvfd;
}

static int nrf24_close(int socket)
{
	if (socket == SOCKET_INVALID)
		return AD_ERROR;

#ifndef ARDUINO
	if (socket == m_srvfd) {
		g_slist_free_full(m_pclients, clients_free);
		close(m_srvfd);
		m_srvfd = SOCKET_INVALID;
	} else
	{
		GSList *pentry = NULL;
		// find the client socket in list
		pentry = g_slist_find_custom(m_pclients, &socket, clifd_match);
		if (!pentry) {
			return AD_ERROR;
		}
		// set CLOSE state
		((client_t*)pentry->data)->state = fdCLOSE;
	}
#else
	m_srvfd = SOCKET_INVALID;
#endif
	return AD_SUCCESS;
}

static int nrf24_listen(int socket)
{
#ifndef ARDUINO
	if (socket == SOCKET_INVALID || socket != m_srvfd)
		return AD_ERROR;

	return AD_ERROR;
#else
	return AD_ERROR;
#endif
}

static int nrf24_accept(int socket)
{
#ifndef ARDUINO
	int st;
	GSList *pentry = NULL;

	if (socket == SOCKET_INVALID || socket != m_srvfd)
		return AD_ERROR;

	// check for connections pending
	if(m_naccept == 0) {
	   st = eventfd_read(socket, &m_naccept);
	   if (st < 0)
			return -errno;
	}

	--m_naccept;

	// find the client in list which is open state
	st = fdOPEN;
	pentry = g_slist_find_custom(m_pclients, &st, state_match);
	if (!pentry) {
		return AD_ERROR;
	}

	// set USE state
	((client_t*)pentry->data)->state = fdUSE;

//	client_t *pc;
//	pc = g_new0(client_t, 1);
//	if(pc == NULL) {
//		close(clifd);
//		return -errno;
//	}
//
//	clifd = eventfd(0, EFD_CLOEXEC);
//	if (clifd < 0) {
//		return -errno;
//	}
//	pc->clifd = clifd;
//	m_pclients = g_slist_append(m_pclients, pc);

	return ((client_t*)pentry->data)->clifd;
#else
	return AD_ERROR;
#endif
}

static int nrf24_connect(int socket, const void *addr, size_t len)
{
	if (socket == SOCKET_INVALID || socket != m_srvfd)
		return AD_ERROR;

	return AD_ERROR;
}

static int nrf24_available(int socket)
{
	if (socket == SOCKET_INVALID)
		return AD_ERROR;

	return AD_SUCCESS;
}

static size_t nrf24_recv(int socket, void *buffer, size_t len)
{
	if (socket == SOCKET_INVALID)
		return AD_ERROR;

	return 0;
}

static size_t nrf24_send(int socket, const void *buffer, size_t len)
{
	if (socket == SOCKET_INVALID)
		return AD_ERROR;

	return 0;
}

static int nrf24_probe(void)
{
#ifndef ARDUINO
	m_pclients = NULL;
	m_naccept = 0;
#endif
	m_binit = (nrf24l01_init() != AD_ERROR);
	return (m_binit ? AD_SUCCESS : AD_ERROR);
}

static void nrf24_remove(void)
{
	if (m_binit) {
		m_binit = false;
		nrf24_close(m_srvfd);
		nrf24l01_deinit();
	}
}

static void nrf24_service(void)
{
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
