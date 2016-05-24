/*
* Copyright (c) 2015, CESAR.
* All rights reserved.
*
* This software may be modified and distributed under the terms
* of the BSD license. See the LICENSE file for details.
*
*/
#ifndef ARDUINO

#define _GNU_SOURCE
#include <stdlib.h>
#include <glib.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <inttypes.h>

#include "nrf24l01_server.h"
#include "nrf24l01.h"
#include "nrf24l01_proto_net.h"
#include "util.h"
#include "list.h"

#define TRACE_ALL
#include "debug.h"
#ifdef TRACE_ALL
#define DUMP_DATA		dump_data
#else
#define DUMP_DATA
#endif

// defines constant time values
#define POLLTIME_MS											10
#define SERVICE_THREAD_TIMEOUT_MS	1
#define JOIN_TIMEOUT_MS								NRF24_TIMEOUT_MS
#define SEND_DELAY_MS									SERVICE_THREAD_TIMEOUT_MS
#define SEND_INTERVAL									60	//SEND_DELAY_MS <= delay <= (SEND_INTERVAL * SEND_DELAY_MS)

// defines constant retry values
#define JOIN_RETRY			NRF24_RETRIES
#define JOIN_RETRY_MIN	5
#define SEND_RETRY			3

#define BROADCAST		NRF24L01_PIPE0_ADDR

#define SET_ERROR(e)		(-(e*65536))
#define GET_ERROR(v)		((-v)/65536)

enum {
	eUNKNOWN,
	eCLOSE,
	eOPEN,

	eCLOSE_PENDING,

	eJOIN,
	eJOIN_PENDING,
	eJOIN_TIMEOUT,
	eJOIN_GAP,
	eJOIN_ECONNREFUSED,
	eJOIN_CHANNEL_BUSY,

	eTX_FIRE,
	eTX_GAP,
	eTX,

	ePRX,
	ePTX
} ;

typedef struct  __attribute__ ((packed)) {
	struct list_head	node;
	int	len,
			offset,
			pipe,
			retry,
			offset_retry;
	uint16_t	net_addr;
	uint8_t	msg_type,
					raw[];
} data_t;

typedef struct  __attribute__ ((packed)) {
	int	cli;
	int 	srv;
} fds_t;

typedef struct  {
	fds_t			fds;
	int				state,
						pipe;
	uint16_t		net_addr;
	uint32_t		hashid;
	ulong_t		heartbeat_sec;
	data_t			*rxmsg;
} client_t;

static volatile int	m_fd = SOCKET_INVALID,
								m_state = eUNKNOWN,
								m_nref = 0;
static GThread *m_gthread = NULL;
static GMainLoop *m_loop = NULL;
static GSList			*m_pclients = NULL;
static client_t		*m_client_pipes[NRF24L01_PIPE_ADDR_MAX] =  { NULL, NULL, NULL, NULL, NULL };
static int64_t		m_naccept = 0;
static data_t			*m_join_data = NULL;

static LIST_HEAD(m_ptx_list);
static G_LOCK_DEFINE(m_ptx_list);

static LIST_HEAD(m_prx_list);
static G_LOCK_DEFINE(m_prx_list);

static void clear_lists(void)
{
	data_t *pdata,
				*pnext;

	TRACE("ptx_list clear:\n");
	list_for_each_entry_safe(pdata, pnext, &m_ptx_list, node) {
		list_del_init(&pdata->node);
		DUMP_DATA(" ", pdata->pipe, pdata->raw, pdata->len);
		g_free(pdata);
	}
	TRACE("prx_list clear:\n");
	list_for_each_entry_safe(pdata, pnext, &m_prx_list, node) {
		list_del_init(&pdata->node);
		DUMP_DATA(" ", pdata->pipe, pdata->raw, pdata->len);
		g_free(pdata);
	}
	g_mutex_clear(&G_LOCK_NAME(m_ptx_list));
	g_mutex_clear(&G_LOCK_NAME(m_prx_list));
	INIT_LIST_HEAD(&m_ptx_list);
	INIT_LIST_HEAD(&m_prx_list);
}

static void client_free(gpointer pentry)
{
	close(((client_t*)pentry)->fds.srv);
	g_free(pentry);
}

void client_close(gpointer pentry, gpointer user_data)
{
	close(((client_t*)pentry)->fds.srv);
	((client_t*)pentry)->fds.srv = SOCKET_INVALID;
}

static inline gint state_match(gconstpointer pentry, gconstpointer pdata)
{
	return ((const client_t*)pentry)->state - *((const int*)pdata);
}

static inline gint join_match(gconstpointer pentry, gconstpointer pdata)
{
	return !(((const client_t*)pentry)->net_addr == ((const nrf24_payload*)pdata)->hdr.net_addr &&
				  ((const client_t*)pentry)->hashid == ((const nrf24_payload*)pdata)->msg.join.hashid);
}

static inline int get_random_value(int interval, int ntime)
{
	int delay;
	srand(time(NULL) ^ rand() ^ delay);
	delay = ((rand() % interval) + 1) * ntime;
	if (delay < 0) {
		delay *= -1;
		delay = ((delay % interval) + 1) * ntime;
	}
	TRACE(">>> DELAY %d\n", delay);
	return (delay);
}

static int poll_fd(int fd, long int timeout)
{
	struct pollfd fd_poll;

	fd_poll.fd = fd;
	fd_poll.events = POLLIN | POLLPRI | POLLRDHUP;
	fd_poll.revents = 0;
	return poll(&fd_poll, 1, timeout);
}

static int get_freepipe(void)
{
	int 			pipe;
	client_t	**ppc = m_client_pipes;

	for (pipe=NRF24L01_PIPE_ADDR_MAX; pipe!=0 && *ppc != NULL; --pipe, ++ppc)
		;

	return (pipe != 0 ? (NRF24L01_PIPE_ADDR_MAX - pipe) + 1 : NRF24L01_NO_PIPE);
}

static int set_client(int pipe, client_t *pc)
{
	if(pipe > BROADCAST) {
		--pipe;
		if(pipe < NRF24L01_PIPE_ADDR_MAX && m_client_pipes[pipe] == NULL) {
			m_client_pipes[pipe] = pc;
			return SUCCESS;
		}
	}
	return ERROR;
}

static client_t *get_client(int pipe)
{
	return ((pipe > BROADCAST && pipe < (NRF24L01_PIPE_ADDR_MAX+1)) ? m_client_pipes[pipe-1] : NULL);
}

static data_t *build_datasend(int pipe, int net_addr, int msg_type, pdata_t praw, len_t len, data_t *msg)
{
	len_t size = (msg != NULL) ? msg->len : 0;

	msg = (data_t*)g_realloc(msg, sizeof(data_t) + size + len);
	if (msg == NULL) {
		return NULL;
	}
	INIT_LIST_HEAD(&msg->node);
	msg->len = size + len;
	msg->offset = 0;
	msg->pipe = pipe;
	msg->retry = SEND_RETRY;
	msg->offset_retry = 0;
	msg->net_addr = net_addr;
	msg->msg_type = msg_type;
	memcpy(msg->raw + size, praw, len);
	return msg;
}

static int send_payload(data_t *pdata)
{
	nrf24_payload payload;
	int len = pdata->len;

	if (pdata->msg_type == NRF24_MSG_APP && len > NRF24_MSG_PW_SIZE) {
		if (pdata->offset == 0) {
			payload.hdr.msg_type = NRF24_MSG_APP_FIRST;
			len = NRF24_MSG_PW_SIZE;
		} else {
			len -= pdata->offset;
			if (len > NRF24_MSG_PW_SIZE) {
				payload.hdr.msg_type = NRF24_MSG_APP_FRAG;
				len = NRF24_MSG_PW_SIZE;
			}
		}
	} else {
		payload.hdr.msg_type = pdata->msg_type;
	}
	payload.hdr.net_addr = pdata->net_addr;
	memcpy(payload.msg.raw, pdata->raw + pdata->offset, len);
	pdata->offset_retry = pdata->offset;
	pdata->offset += len;
	DUMP_DATA("SEND: ", pdata->pipe, &payload, len + sizeof(nrf24_header));
	nrf24l01_set_ptx(pdata->pipe);
	nrf24l01_ptx_data(&payload, len + sizeof(nrf24_header), pdata->pipe != BROADCAST);
	return nrf24l01_ptx_wait_datasent();
}

static inline void put_ptx_queue(data_t *pd)
{
	G_LOCK(m_ptx_list);
	list_move_tail(&pd->node, &m_ptx_list);
	G_UNLOCK(m_ptx_list);
}

static int prx_service(void)
{
	nrf24_payload	data;
	int	pipe,
			len;
	GSList *pentry;
	client_t *pc;

	for (pipe=nrf24l01_prx_pipe_available(); pipe!=NRF24L01_NO_PIPE; pipe=nrf24l01_prx_pipe_available()) {
		len = nrf24l01_prx_data(&data, sizeof(data));
		if (len >= sizeof(data.hdr)) {
			TRACE("type %d\n", data.hdr.msg_type);
			DUMP_DATA(" ", pipe, &data, len);
			len -= sizeof(data.hdr);
			switch (data.hdr.msg_type) {
			case NRF24_MSG_JOIN_LOCAL:
				if (len != sizeof(nrf24_join_local) || pipe != BROADCAST ||
					data.msg.join.maj_version > NRF24_VERSION_MAJOR ||
					data.msg.join.min_version > NRF24_VERSION_MINOR) {
					break;
				}
				data.hdr.msg_type = NRF24_MSG_JOIN_RESULT;
				pentry = g_slist_find_custom(m_pclients, &data, join_match);
				if (pentry == NULL) {
					int freepipe = get_freepipe();
					if (freepipe == NRF24L01_NO_PIPE) {
						data.msg.join.result = NRF24_ECONNREFUSED;
						put_ptx_queue(build_datasend(pipe, data.hdr.net_addr, data.hdr.msg_type, data.msg.raw, len, NULL));
						break;
					}
					pc = g_new0(client_t, 1);
					if (pc == NULL || socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, (int*)&pc->fds) < 0) {
						TERROR("serial: socketpair(): %s\n", strerror(errno));
						g_free(pc);
						break;
					}
					pc->state = eOPEN;
					pc->pipe = freepipe;
					pc->net_addr = data.hdr.net_addr;
					pc->hashid = data.msg.join.hashid;
					if(set_client(freepipe, pc) == ERROR) {
						client_close(pc, NULL);
						g_free(pc);
						break;
					}
					m_pclients = g_slist_append(m_pclients, pc);
					eventfd_write(m_fd, 1);
				} else {
					pc = (client_t*)pentry->data;
				}
				pc->heartbeat_sec = tline_sec();
				data.msg.join.data = pc->pipe;
				data.msg.join.result = NRF24_SUCCESS;
				put_ptx_queue(build_datasend(pipe, data.hdr.net_addr, data.hdr.msg_type, data.msg.raw, len, NULL));
				break;

			case NRF24_MSG_JOIN_GATEWAY:
				if (len != sizeof(nrf24_join_local)  || pipe != BROADCAST) {
 					break;
				}
				data.hdr.msg_type = NRF24_MSG_JOIN_RESULT;
				data.msg.join.result = NRF24_ECONNREFUSED;
				put_ptx_queue(build_datasend(pipe, data.hdr.net_addr, data.hdr.msg_type, data.msg.raw, len, NULL));
				break;

			case NRF24_MSG_HEARTBEAT:
				if (len != sizeof(nrf24_join_local) || (pc=get_client(pipe)) == NULL ||
					data.msg.join.maj_version > NRF24_VERSION_MAJOR ||
					data.msg.join.min_version > NRF24_VERSION_MINOR) {
					break;
				}
				data.hdr.msg_type = NRF24_MSG_JOIN_RESULT;
				data.msg.join.result = NRF24_ECONNREFUSED;
				if(	pc->net_addr == data.hdr.net_addr && pc->hashid == data.msg.join.hashid) {
					pc->heartbeat_sec = tline_sec();
					data.msg.join.result = NRF24_SUCCESS;
				}
				put_ptx_queue(build_datasend(pipe, data.hdr.net_addr, data.hdr.msg_type, data.msg.raw, len, NULL));
				break;

			case NRF24_MSG_UNJOIN_LOCAL:
			case NRF24_MSG_JOIN_RESULT:
				break;

			case NRF24_MSG_APP:
			case NRF24_MSG_APP_FIRST:
			case NRF24_MSG_APP_FRAG:
				break;
			}
		}
	}

	if (!list_empty(&m_ptx_list)) {
		static int send_state = eTX_FIRE;
		static ulong_t	start = 0,
									delay = 0;
		static data_t *pdata = NULL;

		switch (send_state) {
		case  eTX_FIRE:
			start = tline_ms();
			delay = get_random_value(SEND_INTERVAL, SEND_DELAY_MS);
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
					//TODO: next step, error issue to the application if sent message fail
					TERROR("Failed to send message to the pipe#%d\n", pdata->pipe);
					g_free(pdata);
				} else {
					pdata->offset = pdata->offset_retry;
					G_LOCK(m_ptx_list);
					list_move_tail(&pdata->node, &m_ptx_list);
					G_UNLOCK(m_ptx_list);
				}
			} else 	if (pdata->len == pdata->offset) {
				TRACE("Message sent to the pipe#%d len=%d\n", pdata->pipe, pdata->len);
				g_free(pdata);
			} else {
				TRACE("Message fragment sent to the pipe#%d len=%d\n", pdata->pipe, pdata->len);
				G_LOCK(m_ptx_list);
				list_move_tail(&pdata->node, &m_ptx_list);
				G_UNLOCK(m_ptx_list);
			}
			nrf24l01_set_prx();
			send_state = eTX_FIRE;
			break;
		}
	}

	return ePRX;
}

static data_t *build_join_msg(void)
{
	nrf24_join_local join;

	srand(time(NULL) ^ rand() ^ join.hashid);
	join.maj_version = NRF24_VERSION_MAJOR;
	join.min_version = NRF24_VERSION_MINOR;
	join.hashid = (rand() ^ (rand() * 65536)  ^ join.hashid);
	join.data = BROADCAST;
	join.result = NRF24_SUCCESS;
	return build_datasend(BROADCAST,
											  ((join.hashid / 65536) ^ join.hashid),
											  NRF24_MSG_JOIN_GATEWAY,
											  &join,
											  sizeof(join),
											  NULL);
}

static int join_read(ulong_t start)
{
	nrf24_payload	data;
	int	pipe,
			len;

	for (pipe=nrf24l01_prx_pipe_available(); pipe!=NRF24L01_NO_PIPE; pipe=nrf24l01_prx_pipe_available()) {
		len = nrf24l01_prx_data(&data, sizeof(data));
		TRACE("pipe:%d len:%d data.msg.join.hashid:%#x == hashid=%#x\n", pipe, len, data.msg.join.hashid, ((nrf24_join_local*)m_join_data->raw)->hashid);
		DUMP_DATA(" ", pipe, &data, len >= sizeof(data.hdr) ? len : 0);
		if (len == (sizeof(data.hdr)+sizeof(nrf24_join_local)) &&
			pipe == BROADCAST &&
			data.hdr.msg_type == NRF24_MSG_JOIN_RESULT &&
			(data.msg.join.result == NRF24_ECONNREFUSED || data.msg.join.result == NRF24_SUCCESS)) {
			return eJOIN_ECONNREFUSED;
		}
	}

	return (tline_out(tline_ms(), start, JOIN_TIMEOUT_MS) ? eJOIN_TIMEOUT : eJOIN_PENDING);
}

static int join(bool reset)
{
	static int	state = eJOIN,
					 	ch = CH_MIN,
					 	ch0 = CH_MIN;
	static ulong_t	start = 0,
								delay = 0;
	int ret = eJOIN;

	if (reset) {
		ch = nrf24l01_get_channel();
		ch0 = ch;
		m_join_data->retry = get_random_value(JOIN_RETRY, 1);
		if (m_join_data->retry < JOIN_RETRY_MIN) {
			m_join_data->retry += JOIN_RETRY_MIN;
		}
		state = eJOIN;
	}

	switch(state) {
	case eJOIN:
		if (m_join_data == NULL) {
			ret = eJOIN_CHANNEL_BUSY;
		} else {
			send_payload(m_join_data);
			m_join_data->offset = 0;
			m_join_data->offset_retry = 0;
			nrf24l01_set_prx();
			start = tline_ms();
			state = eJOIN_PENDING;
			TRACE("JOIN starting retry #%d\n", m_join_data->retry);
		}
		break;

	case eJOIN_PENDING:
		state = join_read(start);
		break;

	case eJOIN_ECONNREFUSED:
		nrf24l01_set_standby();
		ch += 2;
		if (nrf24l01_set_channel(ch) == ERROR) {
			ch = CH_MIN;
			nrf24l01_set_channel(ch);
		}
		if(ch == ch0) {
			ret = eJOIN_CHANNEL_BUSY;
			break;
		}

		TRACE("JOIN attempt in new channel #%d\n", nrf24l01_get_channel());
		m_join_data->retry = get_random_value(JOIN_RETRY, 1);
		if (m_join_data->retry < JOIN_RETRY_MIN) {
			m_join_data->retry += JOIN_RETRY_MIN;
		}
		state = eJOIN;
		break;

	case eJOIN_TIMEOUT:
		if (--m_join_data->retry == 0) {
			printf("JOIN finished in channel #%d\n", nrf24l01_get_channel());
			ret = ePRX;
			break;
		}

		TRACE("JOIN timout retry %d...\n", m_join_data->retry);
		nrf24l01_set_standby();
		state = eJOIN_GAP;
		delay = get_random_value(SEND_INTERVAL, SEND_DELAY_MS);
		start = tline_ms();
		break;

	case eJOIN_GAP:
		if (tline_out(tline_ms(), start, delay)) {
			state = eJOIN;
		}
		break;
	}
	return ret;
}

static gboolean server_service(gpointer dummy)
{
	gboolean result = G_SOURCE_CONTINUE;

	if (++m_nref == 1) {
		if (m_fd == SOCKET_INVALID && m_state != eUNKNOWN) {
			m_state = eCLOSE;
		} else if (m_fd != SOCKET_INVALID && m_state == eUNKNOWN) {
			m_state = eOPEN;
		}
		switch(m_state) {
		case eCLOSE:
			nrf24l01_set_standby();
			nrf24l01_close_pipe(5);
			nrf24l01_close_pipe(4);
			nrf24l01_close_pipe(3);
			nrf24l01_close_pipe(2);
			nrf24l01_close_pipe(1);
			nrf24l01_close_pipe(0);
			if (m_pclients != NULL) {
				g_slist_foreach(m_pclients, client_close, NULL);
				//g_slist_free_full(m_pclients, client_free);
			}
			memset(m_client_pipes, 0, sizeof(m_client_pipes));
			m_state = eUNKNOWN;
			g_main_loop_quit (m_loop);
			result = G_SOURCE_REMOVE;
			break;

		case eOPEN:
			nrf24l01_open_pipe(0, NRF24L01_PIPE0_ADDR);
			nrf24l01_open_pipe(1, NRF24L01_PIPE1_ADDR);
			nrf24l01_open_pipe(2, NRF24L01_PIPE2_ADDR);
			nrf24l01_open_pipe(3, NRF24L01_PIPE3_ADDR);
			nrf24l01_open_pipe(4, NRF24L01_PIPE4_ADDR);
			nrf24l01_open_pipe(5, NRF24L01_PIPE5_ADDR);
			m_state = join(true);
			break;

		case eJOIN:
			m_state = join(false);
			break;

		case eJOIN_CHANNEL_BUSY:
			eventfd_write(m_fd, SET_ERROR(EUSERS));
			m_state = eCLOSE_PENDING;
			break;

		case ePRX:
			m_state = prx_service();
			break;
		}
	}
	--m_nref;
	return result;
}

static gpointer service_thread(gpointer dummy)
{
    GSource *timer;
    GMainContext *context;

    context = g_main_context_new();
    m_loop = g_main_loop_new(context, FALSE);
    g_main_context_unref(context);
    timer = g_timeout_source_new(SERVICE_THREAD_TIMEOUT_MS);
    g_source_set_callback(timer, server_service, m_loop, NULL);
    g_source_attach(timer, context);
    g_source_unref(timer);
    g_main_loop_run(m_loop);
    return NULL;
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

	m_join_data = build_join_msg();
	if (m_join_data == NULL) {
		errno = ENOMEM;
		return ERROR;
	}
	DUMP_DATA("Join Data:", -1, m_join_data, m_join_data->len + sizeof(data_t));

	srand(time(NULL));
	g_mutex_init(&G_LOCK_NAME(m_prx_list));
	g_mutex_init(&G_LOCK_NAME(m_ptx_list));
	m_naccept = 0;
	m_fd = socket;

	m_gthread = g_thread_new("service_thread", service_thread, NULL);
	if (m_gthread == NULL) {
		g_free(m_join_data);
		m_join_data = NULL;
		errno = ENOMEM;
		return ERROR;
	}

	errno = SUCCESS;
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
		eventfd_write(socket, SET_ERROR(EBADF));
		if (m_gthread != NULL) {
			g_thread_join(m_gthread);
			g_main_loop_unref (m_loop);
			m_gthread == NULL;
		}
		g_free(m_join_data);
		m_join_data = NULL;
		clear_lists();
	}
	errno = SUCCESS;
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
		}
		if (m_naccept < 0) {
			errno = GET_ERROR(m_naccept);
			st = ERROR;
			break;
		}

		--m_naccept;

		// find the client in list which is in open state
		st = eOPEN;
		pentry = g_slist_find_custom(m_pclients, &st, state_match);
		if (pentry != NULL) {
			st = ((client_t*)pentry->data)->fds.cli;
			((client_t*)pentry->data)->state = ePRX;
		}
	}
	return st;
}

int nrf24l01_server_cancel(int socket)
{
	if (m_fd == SOCKET_INVALID || socket != m_fd) {
		errno = EBADF;
		return ERROR;
	}

	eventfd_write(socket, SET_ERROR(ECANCELED));
	return SUCCESS;
}

int nrf24l01_server_available(int socket)
{
	if (socket == SOCKET_INVALID) {
		errno = EBADF;
		return ERROR;
	}

	//returns: 1 for socket available,
	//				 0 for no available,
	//				 or -1 for errors.
	return poll_fd(socket, POLLTIME_MS);
}

#endif		// ARDUINO
