/*
* Copyright (c) 2015, CESAR.
* All rights reserved.
*
* This software may be modified and distributed under the terms
* of the BSD license. See the LICENSE file for details.
*
*/
#include <stdlib.h>
//#include <sys/socket.h>
//#include <inttypes.h>
//#include <fcntl.h>

#include "nrf24l01_client.h"
#include "nrf24l01.h"
#include "util.h"
#include "list.h"

//#define TRACE_ALL
#include "debug.h"
#ifdef TRACE_ALL
#define DUMP_DATA	dump_data
#else
#define DUMP_DATA
#endif

#ifndef ARDUINO
#define F(string_literal) ((const char*)(string_literal))
#define randomSeed(value) srand((unsigned int)value);
#define random(value)	rand()
#endif

/* The largest number rand will return.  */
#ifndef RAND_MAX
#define	RAND_MAX	2147483647L
#endif

// defines constant time values
#define SEND_INTERVAL	NRF24_TIMEOUT_MS	//SEND_DELAY_MS <= delay <= (SEND_INTERVAL * SEND_DELAY_MS)
#define SEND_DELAY_MS	1

// defines constant retry values
#define CLIRQ_RETRY			NRF24_RETRIES
#define SEND_RETRY			((NRF24_HEARTBEAT_TIMEOUT_MS - NRF24_TIMEOUT_MS) / NRF24_TIMEOUT_MS)

#define BROADCAST			NRF24L01_PIPE0_ADDR
#define CLIENT_PIPE			NRF24L01_PIPE0_ADDR

#define NEXT_CH	2

enum {
	eUNKNOWN,

	eCLIR,
	eCLIR_PENDING,
	eCLIR_TIMEOUT,
	eCLIR_ECONNREFUSED,

	eTX_FIRE,
	eTX_GAP,
	eTX,

	ePRX,
	ePTX
} ;

typedef struct {
	struct list_head	node;
	int16_t	len,
					offset,
					retry,
					offset_retry;
	uint16_t	net_addr;
	uint8_t	msg_type,
					pipe,
					raw[];
} data_t;

typedef struct  {
	int				pipe;
	uint16_t		net_addr;
	uint32_t		hashid;
	ulong_t		heartbeat;
	data_t			*rxmsg;
} client_t;

static volatile int		m_fd = SOCKET_INVALID;
static version_t		*m_pversion = NULL;
static client_t			m_client;

static LIST_HEAD(m_ptx_list);

/*
 * Local functions
 */

// reports the space between the heap and the stack.
//static uint16_t getFreeSram()
//{
//	extern unsigned int __bss_end;
//	extern unsigned int __heap_start;
//	extern void *__brkval;
//	uint8_t newVariable;
//
//	// heap is empty, use bss as start memory address
//	if ((uint16_t)(intptr_t)__brkval == 0) {
//		return (((uint16_t)(intptr_t)&newVariable) - ((uint16_t)(intptr_t)&__bss_end));
//	} else {
//		// use heap end as the start of the memory address
//		return (((uint16_t)(intptr_t)&newVariable) - ((uint16_t)(intptr_t)__brkval));
//	}
//}

static void server_cleanup(void)
{
	data_t *pdata, *pnext;

	m_pversion = NULL;

	TRACE(F("ptx_list clear:\n"));
	list_for_each_entry_safe(pdata, pnext, &m_ptx_list, node) {
		list_del_init(&pdata->node);
		DUMP_DATA(F(" "), pdata->pipe, pdata->raw, pdata->len);
	}
	INIT_LIST_HEAD(&m_ptx_list);
}

static int get_random_value(int interval, int ntime, int min)
{
	int value;

	value = (9973 * ~ tline_us()) + ((value) % 701);
	randomSeed(value);

	value = (random(RAND_MAX) % interval) * ntime;
	if (value < 0) {
		value *= -1;
		value = (value % interval) * ntime;
	}
	if (value < min) {
		value += min;
	}
	return value;
}

static data_t *build_data(int pipe, int net_addr, int msg_type, data_t *data, len_t len, data_t *msg)
{
	len_t size = 0;

	if (msg == NULL) {
		msg = data;
		msg->len = 0;
	} else {
		size = msg->len;
	}

	INIT_LIST_HEAD(&msg->node);
	msg->len = size + len;
	msg->offset = 0;
	msg->pipe = pipe;
	msg->retry = SEND_RETRY;
	msg->offset_retry = 0;
	msg->net_addr = net_addr;
	msg->msg_type = msg_type;
	return msg;
}

static int send_payload(data_t *pdata)
{
	payload_t payload;
	int len = pdata->len;

	if (pdata->msg_type == NRF24_APP && len > NRF24_PW_MSG_SIZE) {
		if (pdata->offset == 0) {
			payload.hdr.msg_type = NRF24_APP_FIRST;
			len = NRF24_PW_MSG_SIZE;
		} else {
			len -= pdata->offset;
			if (len > NRF24_PW_MSG_SIZE) {
				payload.hdr.msg_type = NRF24_APP_FRAG;
				len = NRF24_PW_MSG_SIZE;
			}
		}
	} else {
		payload.hdr.msg_type = pdata->msg_type;
	}
	payload.hdr.net_addr = pdata->net_addr;
	memcpy(payload.msg.raw, pdata->raw + pdata->offset, len);
	pdata->offset_retry = pdata->offset;
	pdata->offset += len;
	DUMP_DATA(F("SEND: "), pdata->pipe, &payload, len + sizeof(hdr_t));
	nrf24l01_set_ptx(pdata->pipe);
	nrf24l01_ptx_data(&payload, len + sizeof(hdr_t), pdata->pipe != BROADCAST);
	len = nrf24l01_ptx_wait_datasent();
	nrf24l01_set_prx();
	return len;
}

static inline void put_ptx_queue(data_t *pd)
{
	list_move_tail(&pd->node, &m_ptx_list);
}

static void check_heartbeat(void)
{
	ulong_t	now = tline_ms();

	if (tline_out(now, m_client.heartbeat, NRF24_HEARTBEAT_TIMEOUT_MS)) {

	} else 	if (tline_out(now, m_client.heartbeat, NRF24_HEARTBEAT_SEND_MS)) {
		struct __attribute__ ((packed)) {
			data_t		hdr;
			uint8_t	data[sizeof(join_t)];
		} htb;
		join_t *pj = (join_t*)htb.hdr.raw;

		pj->version.major = m_pversion->major;
		pj->version.minor = m_pversion->minor;
		pj->hashid = m_client.hashid;
		pj->data = m_client.pipe;
		pj->result = NRF24_SUCCESS;
		send_payload(build_data(CLIENT_PIPE, m_client.net_addr, NRF24_HEARTBEAT, &htb.hdr, sizeof(join_t), NULL));
		m_client.heartbeat = tline_ms();
	}
}

static int prx_service(void)
{
	struct __attribute__ ((packed)) {
		data_t		hdr;
		payload_t payload;
	} data;
	int	pipe,
			len;
	GSList *pentry;

	pipe = nrf24l01_prx_pipe_available();
	if (pipe!=NRF24L01_NO_PIPE) {
		len = nrf24l01_prx_data(&data.payload, sizeof(data.payload));
		if (len > sizeof(data.hdr)) {
			DUMP_DATA(F("RECV: "), pipe, &data, len);
			len -= sizeof(data.hdr);
			switch (data.hdr.msg_type) {
			case NRF24_GATEWAY_REQ:
				if (len == sizeof(data.msg.result) && pipe == BROADCAST) {
					data.msg.result = NRF24_ECONNREFUSED;
					put_ptx_queue(build_data(pipe, data.hdr.net_addr, data.hdr.msg_type, data.msg.raw, len, NULL));
				}
				break;

			case NRF24_JOIN_LOCAL:
				if (len != sizeof(join_t) || pipe != BROADCAST ||
					data.msg.join.version.major > m_pversion->major ||
					data.msg.join.version.minor > m_pversion->minor) {
					break;
				}
				data.msg.join.data = set_new_client(&data, len);
				data.msg.join.result = (data.msg.join.data != NRF24L01_NO_PIPE) ? NRF24_SUCCESS : NRF24_ECONNREFUSED;
				put_ptx_queue(build_data(pipe, data.hdr.net_addr, data.hdr.msg_type, data.msg.raw, len, NULL));
				break;

			case NRF24_UNJOIN_LOCAL:
				if (len != sizeof(join_t) ||
					data.msg.join.data != pipe ||
					data.msg.join.version.major > m_pversion->major ||
					data.msg.join.version.minor > m_pversion->minor) {
					break;
				}
				pentry = g_slist_find_custom(m_pclients, &data, join_match);
				if (pentry != NULL) {
					client_disconnect((client_t*)pentry);
				}
				break;

			case NRF24_HEARTBEAT:
				if (len != sizeof(join_t) ||
					data.msg.join.data != pipe ||
					data.msg.join.version.major > m_pversion->major ||
					data.msg.join.version.minor > m_pversion->minor) {
					break;
				}
				pentry = g_slist_find_custom(m_pclients, &data, join_match);
				if (pentry != NULL) {
					((client_t*)pentry)->heartbeat = tline_ms();
					data.msg.join.result = NRF24_SUCCESS;
				} else {
					data.msg.join.result = NRF24_ECONNREFUSED;
				}
				put_ptx_queue(build_data(pipe, data.hdr.net_addr, data.hdr.msg_type, data.msg.raw, len, NULL));
				break;

			case NRF24_APP_FIRST:
			case NRF24_APP_FRAG:
				if (len != NRF24_PW_MSG_SIZE) {
					break;
				}
				/* No break; fall through intentionally */
			case NRF24_APP:
				pentry = g_slist_find_custom(m_pclients, &data, join_match);
				if (pentry != NULL && ((client_t*)pentry)->pipe == pipe) {
					client_t *pc = (client_t*)pentry;
					if (data.hdr.msg_type == NRF24_APP_FIRST) {
						g_free(pc->rxmsg);
						pc->rxmsg = NULL;
					}
					pc->rxmsg = build_data(pipe, data.hdr.net_addr, data.hdr.msg_type, data.msg.raw, len, pc->rxmsg);
					if (data.hdr.msg_type == NRF24_APP) {
						write(pc->fdsock, pc->rxmsg->raw, pc->rxmsg->len);
						g_free(pc->rxmsg);
						pc->rxmsg = NULL;
					}
				}
				break;
			}
		}
	}

	g_slist_foreach(m_pclients, check_heartbeat, NULL);

	if (!list_empty(&m_ptx_list)) {
		static int send_state = eTX_FIRE;
		static ulong_t	start = 0,
									delay = 0;
		static data_t *pdata = NULL;

		switch (send_state) {
		case  eTX_FIRE:
			start = tline_ms();
			delay = get_random_value(SEND_INTERVAL, SEND_DELAY_MS, SEND_DELAY_MS);
			send_state = eTX_GAP;
			/* No break; fall through intentionally */
		case eTX_GAP:
			if (!tline_out(tline_ms(), start, delay)) {
				break;
			}
			send_state = eTX;
			/* No break; fall through intentionally */
		case eTX:
			G_LOCK(m_ptx_list);
			pdata = list_first_entry(&m_ptx_list, data_t, node);
			list_del_init(&pdata->node);
			G_UNLOCK(m_ptx_list);
			if (send_payload(pdata) != SUCCESS) {
				if (--pdata->retry == 0) {
					//TODO: next step, error issue to the application if sent NRF24_APP message fail
					TERROR(F("Failed to send message to the pipe#%d\n"), pdata->pipe);
					g_free(pdata);
				} else {
					pdata->offset = pdata->offset_retry;
					G_LOCK(m_ptx_list);
					list_move_tail(&pdata->node, &m_ptx_list);
					G_UNLOCK(m_ptx_list);
				}
			} else 	if (pdata->len == pdata->offset) {
				g_free(pdata);
			} else {
				TRACE(F("Message fragment sent to the pipe#%d len=%d\n"), pdata->pipe, pdata->len);
				G_LOCK(m_ptx_list);
				list_move_tail(&pdata->node, &m_ptx_list);
				G_UNLOCK(m_ptx_list);
			}
			send_state = eTX_FIRE;
			break;
		}
	}

	return ePRX;
}

static int clireq_read(ulong_t start, ulong_t timeout, data_t	*preq)
{
	payload_t	data;
	join_t *pj = (join_t*)preq->raw;
	int	pipe,
			len;

	for (pipe=nrf24l01_prx_pipe_available(); pipe!=NRF24L01_NO_PIPE; pipe=nrf24l01_prx_pipe_available()) {
		len = nrf24l01_prx_data(&data, sizeof(data));
		DUMP_DATA(F("RECV: "), pipe, &data, len > sizeof(data.hdr) ? len : 0);
		if (len == (sizeof(data.hdr)+sizeof(data.msg.join)) && pipe == BROADCAST &&
			data.hdr.net_addr == preq->net_addr && data.msg.join.hashid == pj->hashid &&
			data.hdr.msg_type == NRF24_JOIN_LOCAL) {
			if (data.msg.result == NRF24_SUCCESS) {
				m_client.pipe = data.msg.join.data;
				m_client.net_addr = data.hdr.net_addr;
				m_client.hashid = data.msg.join.hashid;
				m_client.heartbeat = tline_ms();
				m_client.rxmsg = NULL;
				nrf24l01_set_standby();
				nrf24l01_close_pipe(0);
				nrf24l01_open_pipe(0, m_client.pipe);
				nrf24l01_set_prx();
				return ePRX;
			} else {
				return eCLIR_ECONNREFUSED;
			}
		}
	}

	return (tline_out(tline_ms(), start, timeout) ? eCLIR_TIMEOUT : eCLIR_PENDING);
}

static int join_local(void)
{
	int state = eCLIR;
	ulong_t	start = 0,
					delay = 0;
	struct __attribute__ ((packed)) {
		data_t		hdr;
		uint8_t	data[sizeof(join_t)];
	} clirq;
	join_t *pj = (join_t*)clirq.hdr.raw;

	pj->version.major = m_pversion->major;
	pj->version.minor = m_pversion->minor;
	pj->hashid = get_random_value(RAND_MAX, 1, 1);
	pj->hashid ^= (get_random_value(RAND_MAX, 1, 1) * 65536);
	pj->data = 0;
	pj->result = NRF24_SUCCESS;
	build_data(BROADCAST, ((pj->hashid / 65536) ^ pj->hashid), NRF24_JOIN_LOCAL, &clirq.hdr, sizeof(join_t), NULL);
	clirq.hdr.retry = get_random_value(CLIRQ_RETRY, 2, CLIRQ_RETRY);
	nrf24l01_open_pipe(0, NRF24L01_PIPE0_ADDR);

	while (state == eCLIR || state == eCLIR_PENDING) {
		switch (state) {
		case eCLIR:
			delay = get_random_value(SEND_INTERVAL, SEND_DELAY_MS, SEND_DELAY_MS);
			TRACE(F("Client joins ch#%d retry=%d delay=%ld\n"), nrf24l01_get_channel(), clirq.hdr.retry, delay);
			send_payload(&clirq);
			clirq.hdr.offset = 0;
			start = tline_ms();
			state = eCLIR_PENDING;
			break;

		case eCLIR_PENDING:
			state = clireq_read(start, delay, &clirq.hdr);
			if (state == eCLIR_TIMEOUT && --clirq.hdr.retry != 0) {
				state = eCLIR;
			}
			break;
		}
	}

	if (state != ePRX) {
		nrf24l01_set_standby();
		nrf24l01_close_pipe(0);
	}
	return state;
}

static int unjoin_local(void)
{
	struct __attribute__ ((packed)) {
		data_t		hdr;
		uint8_t	data[sizeof(join_t)];
	} htb;
	join_t *pj = (join_t*)htb.hdr.raw;

	pj->version.major = m_pversion->major;
	pj->version.minor = m_pversion->minor;
	pj->hashid = m_client.hashid;
	pj->data = m_client.pipe;
	pj->result = NRF24_SUCCESS;
	return send_payload(build_data(CLIENT_PIPE, m_client.net_addr, NRF24_UNJOIN_LOCAL, &htb.hdr, sizeof(join_t), NULL));
}

/*
 * Global functions
 */

int nrf24l01_client_open(int socket, int channel, version_t *pversion)
{
	int ret;

	if (socket == SOCKET_INVALID) {
		errno = EBADF;
		return ERROR;
	}

	if (m_fd != SOCKET_INVALID) {
		errno = EMFILE;
		return ERROR;
	}

	if (pversion == NULL || nrf24l01_set_channel(channel) == ERROR) {
		errno = EINVAL;
		return ERROR;
	}

	fprintf(stdout, "Client try to join on channel #%d\n", nrf24l01_get_channel());
	m_fd = socket;
	m_pversion = pversion;

	ret = join_local();
	if (ret != ePRX) {
		if (ret == eCLIR_TIMEOUT) {
			fprintf(stdout, "Client not joined on channel #%d\n", nrf24l01_get_channel());
			errno = ETIMEDOUT;
		} else {
			fprintf(stdout, "Client is refused the channel #%d\n", nrf24l01_get_channel());
			errno = ECONNREFUSED;
		}
		return ERROR;
	}

	fprintf(stdout, "Client joined on channel #%d\n", nrf24l01_get_channel());
	return SUCCESS;
}

int nrf24l01_client_close(int socket)
{
	if (m_fd == SOCKET_INVALID || m_fd != socket) {
		errno = EBADF;
		return ERROR;
	}

	if (unjoin_local() != SUCCESS) {
		errno = EAGAIN;
		return ERROR;
	}

	m_fd = SOCKET_INVALID;
	fprintf(stdout, "Client leaves this channel #%d\n", nrf24l01_get_channel());
	return SUCCESS;
}
