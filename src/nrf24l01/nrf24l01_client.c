/*
* Copyright (c) 2015, CESAR.
* All rights reserved.
*
* This software may be modified and distributed under the terms
* of the BSD license. See the LICENSE file for details.
*
*/
#include "nrf24l01_client.h"
#include "abstract_driver.h"

//#define TRACE_ALL
#include "debug.h"

#ifndef ARDUINO
#define srandom(value) srand((unsigned int)value);
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

typedef struct __attribute__ ((packed)) {
	byte_t	msg_type,
				pipe;
	uint_t	net_addr;
	int_t		offset,
				retry,
				offset_retry;
} data_t;

typedef struct __attribute__ ((packed)) {
	uint_t		net_addr;
	uint32_t	hashid;
	ulong_t	heartbeat_wait;
	byte_t		pipe,
					heartbeat;
} client_t;

typedef struct __attribute__ ((packed)) {
	ulong_t	start,
					delay;
} tline_t;

static int				m_fd = SOCKET_INVALID;
static version_t	*m_pversion = NULL;
static client_t		m_client;

/*
 * Local functions
 */

static int_t get_random_value(int_t interval, int_t ntime, int_t min)
{
	int_t value;

	value = (9973 * ~ tline_us()) + ((value) % 701);
	srandom(value);

	value = (random() % interval) * ntime;
	if (value < 0) {
		value *= -1;
		value = (value % interval) * ntime;
	}
	if (value < min) {
		value += min;
	}
	return value;
}

static void disconnect(void)
{
	fprintf(stdout, "Client leaves this channel #%d\n", nrf24l01_get_channel());
	nrf24l01_set_standby();
	nrf24l01_close_pipe(0);
	memset(&m_client, 0, sizeof(m_client));
	m_client.pipe = BROADCAST;
	m_fd = SOCKET_INVALID;
	m_pversion = NULL;
}

static data_t *build_data(data_t *pd, uint8_t pipe, uint16_t net_addr, uint8_t msg_type)
{
	pd->msg_type = msg_type;
	pd->pipe = pipe;
	pd->net_addr = net_addr;
	pd->offset = 0;
	pd->retry = SEND_RETRY;
	pd->offset_retry = 0;
	return pd;
}

static int_t send_data(data_t *pd, pdata_t praw, len_t len)
{
	payload_t payload;

	if (pd->msg_type == NRF24_APP && len > NRF24_PW_MSG_SIZE) {
		if (pd->offset == 0) {
			payload.hdr.msg_type = NRF24_APP_FIRST;
			len = NRF24_PW_MSG_SIZE;
		} else {
			len -= pd->offset;
			if (len > NRF24_PW_MSG_SIZE) {
				payload.hdr.msg_type = NRF24_APP_FRAG;
				len = NRF24_PW_MSG_SIZE;
			} else {
				payload.hdr.msg_type = NRF24_APP_LAST;
			}
		}
	} else {
		payload.hdr.msg_type = pd->msg_type;
	}
	payload.hdr.net_addr = pd->net_addr;
	memcpy(payload.msg.raw, (((byte_t*)praw) + pd->offset), len);
	pd->offset_retry = pd->offset;
	pd->offset += len;
	DUMP_DATA("SEND: ", pd->pipe, &payload, len + sizeof(hdr_t));
	nrf24l01_set_ptx(pd->pipe);
	nrf24l01_ptx_data(&payload, len + sizeof(hdr_t), pd->pipe != BROADCAST);
	len = nrf24l01_ptx_wait_datasent();
	nrf24l01_set_prx();
	return len;
}

static int_t ptx_service(data_t *pd, pdata_t praw, len_t len)
{
	int	state = ePTX_FIRE,
			result = SUCCESS;
	tline_t	timer;

	while (state != ePTX) {
		switch (state) {
		case  ePTX_FIRE:
			timer.start = tline_ms();
			timer.delay = get_random_value(SEND_INTERVAL, SEND_DELAY_MS, SEND_DELAY_MS);
			state = ePTX_GAP;
			/* No break; fall through intentionally */
		case ePTX_GAP:
			if (!tline_out(tline_ms(), timer.start, timer.delay)) {
				break;
			}
			state = ePTX;
			/* No break; fall through intentionally */
		case ePTX:
			result = send_data(pd, praw, len);
			if (result != SUCCESS) {
				if (--pd->retry == 0) {
					TERROR("Failed to send message to the pipe#%d\n", pd->pipe);
					disconnect();
				} else {
					pd->offset = pd->offset_retry;
					state = ePTX_FIRE;
				}
			} else 	if (len != pd->offset) {
				state = ePTX_FIRE;
			} else if (!m_client.heartbeat) {
				m_client.heartbeat_wait = tline_ms();
			}
			break;
		}
	}
	return result;
}

static void check_heartbeat(join_t *pj)
{
	if (tline_out(tline_ms(), m_client.heartbeat_wait, NRF24_HEARTBEAT_TIMEOUT_MS)) {
		disconnect();
	} else if (!m_client.heartbeat && tline_out(tline_ms(), m_client.heartbeat_wait, NRF24_HEARTBEAT_SEND_MS)) {
		data_t data;

		pj->version.major = m_pversion->major;
		pj->version.minor = m_pversion->minor;
		pj->version.packet_size = m_pversion->packet_size;
		pj->hashid = m_client.hashid;
		pj->data = m_client.pipe;
		pj->result = NRF24_SUCCESS;
		if (ptx_service(build_data(&data, m_client.pipe, m_client.net_addr, NRF24_HEARTBEAT), pj, sizeof(join_t)) == SUCCESS) {
			m_client.heartbeat_wait = tline_ms();
			m_client.heartbeat = true;
		}
	}
}

static int_t prx_service(byte_t *buffer, len_t length)
{
	static int16_t	offset = 0;
	payload_t	data;
	int	pipe,
			len;

	for (pipe=nrf24l01_prx_pipe_available(); pipe!=NRF24L01_NO_PIPE; pipe=nrf24l01_prx_pipe_available()) {
		len = nrf24l01_prx_data(&data, sizeof(data));
		if (len > sizeof(data.hdr)) {
			DUMP_DATA("RECV: ", pipe, &data, len);
			len -= sizeof(data.hdr);
			switch (data.hdr.msg_type) {
			case NRF24_HEARTBEAT:
				if (len != sizeof(join_t) ||
					data.msg.join.data != pipe ||
					data.msg.join.version.major != m_pversion->major ||
					data.msg.join.version.minor != m_pversion->minor ||
					data.msg.join.version.packet_size != m_pversion->packet_size) {
					break;
				}
				m_client.heartbeat_wait = tline_ms();
				m_client.heartbeat = false;
				break;

			case NRF24_APP_FIRST:
			case NRF24_APP_FRAG:
				if (len != NRF24_PW_MSG_SIZE) {
					break;
				}
				/* No break; fall through intentionally */
			case NRF24_APP_LAST:
			case NRF24_APP:
				if (m_client.pipe == pipe && m_client.net_addr == data.hdr.net_addr) {
					if (!m_client.heartbeat) {
						m_client.heartbeat_wait = tline_ms();
					}
					if (data.hdr.msg_type == NRF24_APP || data.hdr.msg_type == NRF24_APP_FIRST) {
						offset = 0;
					}
					if ((offset + len) <= m_pversion->packet_size) {
						if ((offset + len) > length) {
							len = length - offset;
						}
						if (len != 0) {
							memcpy(buffer + offset, &data.msg.raw, len);
							offset += len;
						}
						if (data.hdr.msg_type == NRF24_APP || data.hdr.msg_type == NRF24_APP_LAST) {
							return offset;
						}
					}
				}
				break;
			}
		}
	}

	check_heartbeat(&data.msg.join);
	return 0;
}

static int_t clireq_read(uint16_t net_addr, join_t *pj, tline_t *pt)
{
	payload_t	data;
	int	pipe,
			len;

	for (pipe=nrf24l01_prx_pipe_available(); pipe!=NRF24L01_NO_PIPE; pipe=nrf24l01_prx_pipe_available()) {
		len = nrf24l01_prx_data(&data, sizeof(data));
		DUMP_DATA("RECV: ", pipe, &data, len > sizeof(data.hdr) ? len : 0);
		if (len == (sizeof(data.hdr)+sizeof(data.msg.join)) && pipe == BROADCAST &&
			data.hdr.net_addr == net_addr && data.msg.join.hashid == pj->hashid &&
			data.hdr.msg_type == NRF24_JOIN_LOCAL) {
			if (data.msg.result != NRF24_SUCCESS) {
				return eECONNREFUSED;
			}

			m_client.pipe = data.msg.join.data;
			m_client.net_addr = data.hdr.net_addr;
			m_client.hashid = data.msg.join.hashid;
			m_client.heartbeat_wait = tline_ms();
			m_client.heartbeat = false;
			nrf24l01_set_standby();
			nrf24l01_close_pipe(0);
			nrf24l01_open_pipe(0, m_client.pipe);
			nrf24l01_set_prx();
			return ePRX;
		}
	}

	return (tline_out(tline_ms(), pt->start, pt->delay) ? eTIMEOUT : ePENDING);
}

static int_t join_local(void)
{
	int_t state = eREQUEST;
	tline_t	timer;
	join_t	join;
	data_t data;

	nrf24l01_open_pipe(0, BROADCAST);

	join.version.major = m_pversion->major;
	join.version.minor = m_pversion->minor;
	join.version.packet_size = m_pversion->packet_size;
	join.hashid = get_random_value(RAND_MAX, 1, 1);
	join.hashid ^= (get_random_value(RAND_MAX, 1, 1) * 65536);
	join.data = 0;
	join.result = NRF24_SUCCESS;
	build_data(&data, BROADCAST, ((join.hashid / 65536) ^ join.hashid), NRF24_JOIN_LOCAL);
	data.retry = get_random_value(JOINREQ_RETRY, 2, JOINREQ_RETRY);

	while (state == eREQUEST || state == ePENDING) {
		switch (state) {
		case eREQUEST:
			timer.delay = get_random_value(SEND_INTERVAL, SEND_DELAY_MS, SEND_DELAY_MS);
			TRACE("Client joins ch#%d retry=%d delay=%ld\n", nrf24l01_get_channel(), data.retry, delay);
			send_data(&data, &join, sizeof(join_t));
			data.offset = 0;
			timer.start = tline_ms();
			state = ePENDING;
			break;

		case ePENDING:
			state = clireq_read(data.net_addr, &join, &timer);
			if (state == eTIMEOUT && --data.retry != 0) {
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

static int_t unjoin_local(void)
{
	join_t	join;
	data_t data;

	join.version.major = m_pversion->major;
	join.version.minor = m_pversion->minor;
	join.version.packet_size = m_pversion->packet_size;
	join.hashid = m_client.hashid;
	join.data = m_client.pipe;
	join.result = NRF24_SUCCESS;
	return ptx_service(build_data(&data, m_client.pipe, m_client.net_addr, NRF24_UNJOIN_LOCAL), &join, sizeof(join_t));
}

/*
 * Global functions
 */

int_t nrf24l01_client_open(int_t socket, byte_t channel, version_t *pversion)
{
	int_t ret;

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

	memset(&m_client, 0, sizeof(m_client));
	m_client.pipe = BROADCAST;
	m_fd = socket;
	m_pversion = pversion;

	fprintf(stdout, "Client try to join on channel #%d\n", nrf24l01_get_channel());
	ret = join_local();
	if (ret != ePRX) {
		if (ret == eTIMEOUT) {
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

int_t nrf24l01_client_close(int_t socket)
{
	if (m_fd == SOCKET_INVALID || m_fd != socket) {
		errno = EBADF;
		return ERROR;
	}

	if (m_client.net_addr != 0) {
		if (unjoin_local() != SUCCESS) {
			errno = EAGAIN;
			return ERROR;
		}
		disconnect();
	}
	m_fd = SOCKET_INVALID;
	m_pversion = NULL;

	return SUCCESS;
}

int_t nrf24l01_client_read(int_t socket, byte_t *buffer, len_t len)
{
	if (m_fd == SOCKET_INVALID || m_fd != socket) {
		errno = EBADF;
		return ERROR;
	}

	return prx_service(buffer, len);
}

int_t nrf24l01_client_write(int_t socket, const byte_t *buffer, len_t len)
{
	data_t data;

	if (m_fd == SOCKET_INVALID || m_fd != socket) {
		errno = EBADF;
		return ERROR;
	}

	return ptx_service(build_data(&data, m_client.pipe, m_client.net_addr, NRF24_APP), (pdata_t)buffer, len);
}
