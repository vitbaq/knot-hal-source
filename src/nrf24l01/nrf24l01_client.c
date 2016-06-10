/*
* Copyright (c) 2015, CESAR.
* All rights reserved.
*
* This software may be modified and distributed under the terms
* of the BSD license. See the LICENSE file for details.
*
*/
#include "nrf24l01_client.h"

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

enum {
	eUNKNOWN,
	eREQUEST,
	ePENDING,
	eTIMEOUT,
	eECONNREFUSED,
	ePRX,
	ePTX_FIRE,
	ePTX_GAP,
	ePTX
} ;

typedef struct {
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
	uint16_t		net_addr;
	uint32_t		hashid;
	ulong_t		heartbeat;
	uint8_t		pipe;
	data_t			*rxmsg;
} client_t;

static int				m_fd = SOCKET_INVALID;
static version_t	*m_pversion = NULL;
static client_t		m_client;

/*
 * Local functions
 */

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
			} else {
				payload.hdr.msg_type = NRF24_APP_LAST;
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

static int put_ptx_queue(data_t *pdata)
{
	int	state = ePTX_FIRE,
			result = SUCCESS;
	ulong_t	start,
					delay;

	while (state != ePTX) {
		switch (state) {
		case  ePTX_FIRE:
			start = tline_ms();
			delay = get_random_value(SEND_INTERVAL, SEND_DELAY_MS, SEND_DELAY_MS);
			state = ePTX_GAP;
			/* No break; fall through intentionally */
		case ePTX_GAP:
			if (!tline_out(tline_ms(), start, delay)) {
				break;
			}
			state = ePTX;
			/* No break; fall through intentionally */
		case ePTX:
			result = send_payload(pdata);
			if (result != SUCCESS) {
				if (--pdata->retry == 0) {
					//TODO: next step, error issue to the application if sent NRF24_APP message fail
					TERROR(F("Failed to send message to the pipe#%d\n"), pdata->pipe);
				} else {
					pdata->offset = pdata->offset_retry;
					state = ePTX_FIRE;
				}
			} else 	if (pdata->len != pdata->offset) {
				state = ePTX_FIRE;
			}
			break;
		}
	}
	return result;
}

static void check_heartbeat(data_t *pd)
{
	ulong_t	now = tline_ms();

	if (tline_out(now, m_client.heartbeat, NRF24_HEARTBEAT_TIMEOUT_MS)) {
		nrf24l01_set_standby();
		nrf24l01_close_pipe(0);
		m_fd = SOCKET_INVALID;
		m_pversion = NULL;
	} else 	if (tline_out(now, m_client.heartbeat, NRF24_HEARTBEAT_SEND_MS)) {
		join_t *pj = (join_t*)pd->raw;

		pj->version.major = m_pversion->major;
		pj->version.minor = m_pversion->minor;
		pj->hashid = m_client.hashid;
		pj->data = m_client.pipe;
		pj->result = NRF24_SUCCESS;
		put_ptx_queue(build_data(m_client.pipe, m_client.net_addr, NRF24_HEARTBEAT, pd, sizeof(join_t), NULL));
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

	pipe = nrf24l01_prx_pipe_available();
	if (pipe!=NRF24L01_NO_PIPE) {
		len = nrf24l01_prx_data(&data.payload, sizeof(data.payload));
		if (len > sizeof(data.hdr)) {
			DUMP_DATA(F("RECV: "), pipe, &data, len);
			len -= sizeof(data.hdr);
			switch (data.hdr.msg_type) {
			case NRF24_UNJOIN_LOCAL:
				if (len != sizeof(join_t) ||
					data.msg.join.data != pipe ||
					data.msg.join.version.major > m_pversion->major ||
					data.msg.join.version.minor > m_pversion->minor) {
					break;
				}
//				pentry = g_slist_find_custom(m_pclients, &data, join_match);
//				if (pentry != NULL) {
//					client_disconnect((client_t*)pentry);
//				}
				break;

			case NRF24_HEARTBEAT:
				if (len != sizeof(join_t) ||
					data.msg.join.data != pipe ||
					data.msg.join.version.major > m_pversion->major ||
					data.msg.join.version.minor > m_pversion->minor) {
					break;
				}
//				pentry = g_slist_find_custom(m_pclients, &data, join_match);
//				if (pentry != NULL) {
//					((client_t*)pentry)->heartbeat = tline_ms();
//					data.msg.join.result = NRF24_SUCCESS;
//				} else {
//					data.msg.join.result = NRF24_ECONNREFUSED;
//				}
				put_ptx_queue(build_data(pipe, data.hdr.net_addr, data.hdr.msg_type, data.msg.raw, len, NULL));
				break;

			case NRF24_APP_FIRST:
			case NRF24_APP_FRAG:
				if (len != NRF24_PW_MSG_SIZE) {
					break;
				}
				/* No break; fall through intentionally */
			case NRF24_APP_LAST:
			case NRF24_APP:
//				pentry = g_slist_find_custom(m_pclients, &data, join_match);
//				if (pentry != NULL && ((client_t*)pentry)->pipe == pipe) {
//					client_t *pc = (client_t*)pentry;
//					if (data.hdr.msg_type == NRF24_APP_FIRST) {
//						g_free(pc->rxmsg);
//						pc->rxmsg = NULL;
//					}
//					pc->rxmsg = build_data(pipe, data.hdr.net_addr, data.hdr.msg_type, data.msg.raw, len, pc->rxmsg);
//					if (data.hdr.msg_type == NRF24_APP || data.hdr.msg_type == NRF24_APP_LAST) {
//						write(pc->fdsock, pc->rxmsg->raw, pc->rxmsg->len);
//						g_free(pc->rxmsg);
//						pc->rxmsg = NULL;
//					}
//				}
				break;
			}
		}
	}

	check_heartbeat();
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
			if (data.msg.result != NRF24_SUCCESS) {
				return eECONNREFUSED;
			}

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
		}
	}

	return (tline_out(tline_ms(), start, timeout) ? eTIMEOUT : ePENDING);
}

static int join_local(void)
{
	int state = eREQUEST;
	ulong_t	start = 0,
					delay = 0;
	struct __attribute__ ((packed)) {
		data_t		hdr;
		uint8_t	data[sizeof(join_t)];
	} clireq;
	join_t *pj = (join_t*)clireq.hdr.raw;

	nrf24l01_open_pipe(0, BROADCAST);

	pj->version.major = m_pversion->major;
	pj->version.minor = m_pversion->minor;
	pj->hashid = get_random_value(RAND_MAX, 1, 1);
	pj->hashid ^= (get_random_value(RAND_MAX, 1, 1) * 65536);
	pj->data = 0;
	pj->result = NRF24_SUCCESS;
	build_data(BROADCAST, ((pj->hashid / 65536) ^ pj->hashid), NRF24_JOIN_LOCAL, &clireq.hdr, sizeof(join_t), NULL);
	clireq.hdr.retry = get_random_value(JOINREQ_RETRY, 2, JOINREQ_RETRY);

	while (state == eREQUEST || state == ePENDING) {
		switch (state) {
		case eREQUEST:
			delay = get_random_value(SEND_INTERVAL, SEND_DELAY_MS, SEND_DELAY_MS);
			TRACE(F("Client joins ch#%d retry=%d delay=%ld\n"), nrf24l01_get_channel(), clireq.hdr.retry, delay);
			send_payload(&clireq);
			clireq.hdr.offset = 0;
			start = tline_ms();
			state = ePENDING;
			break;

		case ePENDING:
			state = clireq_read(start, delay, &clireq.hdr);
			if (state == eTIMEOUT && --clireq.hdr.retry != 0) {
				state = eREQUEST;
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
	} clireq;
	join_t *pj = (join_t*)clireq.hdr.raw;

	pj->version.major = m_pversion->major;
	pj->version.minor = m_pversion->minor;
	pj->hashid = m_client.hashid;
	pj->data = m_client.pipe;
	pj->result = NRF24_SUCCESS;
	return put_ptx_queue(build_data(m_client.pipe, m_client.net_addr, NRF24_UNJOIN_LOCAL, &clireq.hdr, sizeof(join_t), NULL));
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

	m_fd = socket;
	m_pversion = pversion;

	fprintf(stdout, F("Client try to join on channel #%d\n"), nrf24l01_get_channel());
	ret = join_local();
	if (ret != ePRX) {
		if (ret == eTIMEOUT) {
			fprintf(stdout, F("Client not joined on channel #%d\n"), nrf24l01_get_channel());
			errno = ETIMEDOUT;
		} else {
			fprintf(stdout, F("Client is refused the channel #%d\n"), nrf24l01_get_channel());
			errno = ECONNREFUSED;
		}
		return ERROR;
	}

	fprintf(stdout, F("Client joined on channel #%d\n"), nrf24l01_get_channel());
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

	nrf24l01_set_standby();
	nrf24l01_close_pipe(0);
	m_fd = SOCKET_INVALID;
	m_pversion = NULL;

	fprintf(stdout, F("Client leaves this channel #%d\n"), nrf24l01_get_channel());
	return SUCCESS;
}
