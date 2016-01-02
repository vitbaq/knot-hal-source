/*
* Copyright (c) 2015, CESAR.
* All rights reserved.
*
* This software may be modified and distributed under the terms
* of the BSD license. See the LICENSE file for details.
*
*/
#ifndef ARDUINO
#include <stdlib.h>
#include <glib.h>

#include "nrf24l01_server.h"
#include "nrf24l01.h"
#include "nrf24l01_proto_net.h"
#include "util.h"

#define BROADCAST		0

#define JOIN_TIMEOUT	NRF24_TIMEOUT		//ms
#define JOIN_RETRY		NRF24_RETRIES
#define JOIN_DELAY			20									//ms
#define JOIN_INTERVAL	50									//20ms <= interval <= 1s

enum {
	eUNKNOWN,
	eCLOSE,
	eOPEN,

	eJOIN,
	eJOIN_PENDING,
	eJOIN_TIMEOUT,
	eJOIN_INTERMITION,
	eJOIN_NO,
	eJOIN_NO_CHANNELS,

	ePRX,
	ePTX
} ;

typedef struct  __attribute__ ((packed)) {
	int	cli;
	int 	srv;
} fds_t;

typedef struct  {
	fds_t	fd;
	int		state;
} client_t;

static volatile int			m_fd = SOCKET_INVALID,
										m_state = eUNKNOWN,
										m_nref = 0;
static int						m_channel = NRF24L01_CHANNEL_DEFAULT;
static eventfd_t			m_naccept = 0;
static GSList					*m_pclients = NULL;
static nrf24_payload	m_payload;
static	len_t					m_payload_len;

static void client_free(gpointer pentry)
{
	close(((client_t*)pentry)->fd.cli);
	close(((client_t*)pentry)->fd.srv);
	g_free(pentry);
}

void client_close(gpointer pentry, gpointer user_data)
{
	close(((client_t*)pentry)->fd.cli);
	close(((client_t*)pentry)->fd.srv);
	((client_t*)pentry)->fd.cli = SOCKET_INVALID;
	((client_t*)pentry)->fd.srv = SOCKET_INVALID;
}

static inline gint fdcli_match(gconstpointer pentry, gconstpointer pdata)
{
	return ((const client_t*)pentry)->fd.cli - *((const int*)pdata);
}

static inline gint state_match(gconstpointer pentry, gconstpointer pdata)
{
	return ((const client_t*)pentry)->state - *((const int*)pdata);
}

static inline int get_delay_random(void)
{
	srand((unsigned)time(NULL));
	return (((rand() % JOIN_INTERVAL) + 1)  * JOIN_DELAY);
}

static int build_payload(int addr, int msg, int offset, pparam_t pd, len_t len)
{
	m_payload.hdr.net_addr = addr;
	m_payload.hdr.msg_type = msg;
	m_payload.hdr.offset = offset;
	if (len > sizeof(m_payload.msg.data)) {
		len = sizeof(m_payload.msg.data);
	}
	memcpy(m_payload.msg.data, pd, len);
	return (len+sizeof(nrf24_header));
}

static int waiting_tx_immediate(int timeout)
{
//
//	uint32_t start = millis();
//
//	while ( ! (read_register(FIFO_STATUS) & _BV(TX_EMPTY)) ){
//		if ( get_status() & _BV(MAX_RT)){
//			write_register(NRF_STATUS,_BV(MAX_RT) );
//				ce(LOW);										  //Set re-transmit
//				ce(HIGH);
//				if (millis() - start >= timeout){
//					ce(LOW); flush_tx(); return 0;
//				}
//		}
//		if ( millis() - start > (timeout+85)){
//			errNotify();
//			#if defined (FAILURE_HANDLING)
//			return 0;
//			#endif
//		}
//	}
//	ce(LOW);				   //Set STANDBY-I mode
	return 1;
}

static int join_read(ulong_t start)
{
	nrf24_payload	data;
	int	pipe,
			len;

	for (pipe=nrf24l01_prx_pipe_available(); pipe!=NRF24L01_NO_PIPE; pipe=nrf24l01_prx_pipe_available()) {
		len = nrf24l01_prx_data(&data, sizeof(data));
		if (len == NRF24_JOIN_PW_SIZE && pipe == BROADCAST &&
			data.hdr.net_addr == m_payload.hdr.net_addr &&
			data.msg.join.hashid == m_payload.msg.join.hashid &&
			data.msg.join.result == NRF24_NO_JOIN) {
			return eJOIN_NO;
		}
	}

	if (tline_out(tline_ms(), start, JOIN_TIMEOUT)) {
		return eJOIN_TIMEOUT;
	}

	return eJOIN_PENDING;
}

static int prx_service(void)
{
	if (m_pclients != NULL) {
	}
	return ePRX;
}

static int build_join_msg(void)
{
	nrf24_join_local join;

	join.maj_version = NRF24_VERSION_MAJOR;
	join.min_version = NRF24_VERSION_MINOR;
	join.pipe = BROADCAST;

	srand((unsigned int)time(NULL));
	join.hashid = rand() ^ (rand() * 65536);
	return build_payload(rand() ^ rand(),
											NRF24_MSG_JOIN_GATEWAY,
											0,
											&join,
											sizeof(join));
}

static int join(bool reset)
{
	static int	state = eJOIN,
					 	retry = JOIN_RETRY,
					 	ch = NRF24L01_CHANNEL_DEFAULT;
	static ulong_t	start = 0,
								delay = 0;

	if (reset) {
		m_payload_len = build_join_msg();
		ch = m_channel;
		retry = JOIN_RETRY;
		state = eJOIN;
	}

	switch(state) {
	case eJOIN:
		nrf24l01_set_ptx(BROADCAST);
		nrf24l01_ptx_data(&m_payload, m_payload_len, false);
		nrf24l01_ptx_empty();
		nrf24l01_set_prx();
		start = tline_ms();
		state = eJOIN_PENDING;
		break;
	case eJOIN_PENDING:
		state = join_read(start);
		break;
	case eJOIN_NO:
		nrf24l01_set_standby();
		ch += 2;
		if (ch == m_channel) {
			return eJOIN_NO_CHANNELS;
		}
		if (nrf24l01_set_channel(ch) == ERROR) {
			ch = CH_MIN;
			nrf24l01_set_channel(ch);
		}
		retry = JOIN_RETRY;
		state = eJOIN;
		break;
	case eJOIN_TIMEOUT:
		if (--retry == 0) {
			return ePRX;
		}
		nrf24l01_set_standby();
		state = eJOIN_INTERMITION;
		delay = get_delay_random();
		start = tline_ms();
		break;
	case eJOIN_INTERMITION:
		if (tline_out(tline_ms(), start, delay)) {
			state = eJOIN;
		}
		break;
	}
	return eJOIN;
}

void nrf24l01_server_service(void)
{
	if (++m_nref == 1) {
		if (m_fd == SOCKET_INVALID && m_state != eUNKNOWN) {
			m_state = eCLOSE;
		} else if (m_fd != SOCKET_INVALID && m_state == eUNKNOWN) {
			m_state = eOPEN;
		}
		switch(m_state) {
		case eCLOSE:
			nrf24l01_set_standby();
			nrf24l01_close_pipe(BROADCAST);
			if (m_pclients != NULL) {
				g_slist_foreach(m_pclients, client_close, NULL);
				//g_slist_free_full(m_pclients, client_free);
			}
			m_state = eUNKNOWN;
			break;
		case eOPEN:
			nrf24l01_open_pipe(BROADCAST);
			m_state = join(true);
			break;
		case eJOIN:
			m_state = join(false);
			break;
		case eJOIN_NO_CHANNELS:
			m_state = eCLOSE;
			eventfd_write(m_fd, 1);
			break;
		case ePRX:
			m_state = prx_service();
			break;
		}
	}
	--m_nref;
}

int nrf24l01_server_open(int socket, int channel)
{
	if (socket == SOCKET_INVALID) {
		errno = EBADF;
		return ERROR;
	}

	if (m_fd != SOCKET_INVALID) {
		errno = EMFILE;
		return ERROR;
	}

	if (nrf24l01_set_channel(channel) == ERROR) {
		errno = EINVAL;
		return ERROR;
	}

	m_channel = channel;
	m_naccept = 0;
	m_fd = socket;
	nrf24l01_server_service();
	return SUCCESS;
}

int nrf24l01_server_close(int socket)
{
	if (socket == SOCKET_INVALID) {
		errno = EBADF;
		return ERROR;
	}

	if (socket == m_fd) {
		m_fd = SOCKET_INVALID;
	} else {
		GSList *pentry = g_slist_find_custom(m_pclients, &socket, fdcli_match);
		if (pentry == NULL && ((client_t*)pentry)->fd.cli == SOCKET_INVALID) {
			errno = EBADF;
			return ERROR;
		}
		client_close(pentry, NULL);
	}
	nrf24l01_server_service();
	return SUCCESS;
}

int nrf24l01_server_accept(int socket)
{
	GSList *pentry = NULL;
	int st;

	if (m_fd == SOCKET_INVALID || socket != m_fd) {
		errno = EBADF;
		return ERROR;
	}

	while (pentry == NULL) {
		if (m_naccept == 0) {
		   st = eventfd_read(socket, &m_naccept);
		   if (st < 0) {
				st = ERROR;
				break;
		   }
		   if (m_state == eCLOSE || m_state == eUNKNOWN) {
				errno = EBADF;
				st = ERROR;
				break;
		   }
		}

		--m_naccept;

		// find the client in list which is open state
		st = eOPEN;
		pentry = g_slist_find_custom(m_pclients, &st, state_match);
		if (pentry != NULL) {
			st = ((client_t*)pentry->data)->fd.cli;
			((client_t*)pentry->data)->state = ePRX;
		}
	}
	nrf24l01_server_service();
	return st;
//	client_t *pc;
//	pc = g_new0(client_t, 1);
//	if (pc == NULL) {
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
}

int nrf24l01_server_available(int socket)
{
	nrf24l01_server_service();
	return SUCCESS;
}

#endif		// #ifndef ARDUINO)
