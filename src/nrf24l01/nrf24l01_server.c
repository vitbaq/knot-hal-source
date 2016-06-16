/*
* Copyright (c) 2015, CESAR.
* All rights reserved.
*
* This software may be modified and distributed under the terms
* of the BSD license. See the LICENSE file for details.
*
*/
#include <glib.h>

#include "nrf24l01_server.h"

//#define TRACE_ALL
#include "debug.h"

#define NEXT_CH	2

static enum {
	UNKNOWN,
	OPEN,
	CLOSE,
	REQUEST,
	PENDING,
	TIMEOUT,
	REFUSED,
	CHANNEL_BUSY,
	CLOSE_PENDING,
	PRX,
	PTX_FIRE,
	PTX_GAP,
	PTX
} e_state;

typedef struct {
	struct list_head	node;
	int	len,
			offset,
			pipe,
			retry,
			offset_retry;
	uint16_t	net_addr;
	byte_t	msg_type,
				raw[];
} data_t;

typedef struct  {
	int			fdsock,
					pipe;
	uint16_t	net_addr,
					packet_size;
	uint32_t	hashid;
	ulong_t	heartbeat_wait;
	guint		watch_id;
	byte_t		rxmn,
					txmn;
	data_t		*rxmsg;
} client_t;

static volatile int		m_fd = SOCKET_INVALID,
									m_state = UNKNOWN;
static GThread			*m_gthread = NULL;
static GMainLoop	*m_loop = NULL;
static GSList				*m_pclients = NULL;
static client_t			*m_pipes_allocate[NRF24L01_PIPE_ADDR_MAX] = {NULL, NULL, NULL, NULL, NULL};
static data_t				*m_gwreq = NULL;
static version_t		*m_pversion = NULL;
static const void		*m_paddr;
static size_t				m_laddr;

static LIST_HEAD(m_ptxlist);
static G_LOCK_DEFINE(m_ptxlist);

/*
 * Local functions
 */
static inline int set_pipe(int pipe, client_t *pc)
{
	if (pipe > BROADCAST && pipe < (NRF24L01_PIPE_ADDR_MAX+1)) {
		m_pipes_allocate[pipe-1] = pc;
	}
	return 0;
}

static inline client_t *get_pipe(int pipe)
{
	if (pipe > BROADCAST && pipe < (NRF24L01_PIPE_ADDR_MAX+1)) {
		return m_pipes_allocate[pipe-1];
	}
	return NULL;
}

static int get_pipe_free(void)
{
	int 			pipe;
	client_t	**pa = m_pipes_allocate;

	for (pipe=NRF24L01_PIPE_ADDR_MAX; pipe!=0 && *pa != NULL; --pipe, ++pa)
		;

	return (pipe != 0 ? (NRF24L01_PIPE_ADDR_MAX - pipe) + 1 : NRF24L01_NO_PIPE);
}

static gint join_match(gconstpointer pentry, gconstpointer pdata)
{
	return !(((client_t*)pentry)->net_addr == ((payload_t*)pdata)->hdr.net_addr &&
				  ((client_t*)pentry)->hashid == ((payload_t*)pdata)->msg.join.hashid);
}

static inline client_t *get_client(payload_t *ppl)
{
	GSList *pentry = g_slist_find_custom(m_pclients, ppl, join_match);
	return (pentry != NULL ? (client_t*)pentry->data : NULL);
}

static inline void client_disconnect(client_t *pc)
{
	TRACE(" IN\n");
	if (pc != NULL && pc->watch_id != 0) {
		g_source_remove(pc->watch_id);
		TRACE("watch = %d\n", pc->watch_id);
		pc->watch_id = 0;
	}
}

static void client_free(client_t *pc)
{
	data_t *pdata, *pnext;

	TRACE("close(%d) pipe=%d packet=%d\n", pc->fdsock, pc->pipe, pc->packet_size);
	close(pc->fdsock);
	g_free(pc->rxmsg);
	G_LOCK(m_ptxlist);
	list_for_each_entry_safe(pdata, pnext, &m_ptxlist, node) {
		if (pc->pipe == pdata->pipe) {
			list_del_init(&pdata->node);
			g_free(pdata);
		}
	}
	G_UNLOCK(m_ptxlist);
	set_pipe(pc->pipe, NULL);
	g_free(pc);
}

static void client_close(gpointer pentry, gpointer user_data)
{
	TRACE(" IN\n");
	client_disconnect((client_t*)pentry);
}

static void server_cleanup(void)
{
	data_t *pdata, *pnext;

	if (m_pclients != NULL) {
		g_slist_foreach(m_pclients, client_close, NULL);
	}
	g_free(m_gwreq);
	m_gwreq = NULL;
	m_pversion = NULL;

	TRACE("ptx_list clear:\n");
	G_LOCK(m_ptxlist);
	list_for_each_entry_safe(pdata, pnext, &m_ptxlist, node) {
		list_del_init(&pdata->node);
		DUMP_DATA(" ", pdata->pipe, pdata->raw, pdata->len);
		g_free(pdata);
	}
	G_UNLOCK(m_ptxlist);
	g_mutex_clear(&G_LOCK_NAME(m_ptxlist));
	INIT_LIST_HEAD(&m_ptxlist);
}

static int get_random_value(int interval, int ntime, int min)
{
	int value;

	value = (9973 * ~ tline_us()) + ((value) % 701);
	srand((unsigned int)value);

	value = (rand() % interval) * ntime;
	if (value < 0) {
		value *= -1;
		value = (value % interval) * ntime;
	}
	if (value < min) {
		value += min;
	}
	return value;
}

static data_t *build_data(int pipe, int net_addr, int msg_type, pdata_t praw, len_t len, data_t *msg)
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

static int send_data(data_t *pdata, client_t *pc)
{
	payload_t payload;
	int len = pdata->len;
	byte_t msg_type;

	if (pdata->msg_type == NRF24_APP && len > NRF24_PW_MSG_SIZE) {
		if (pdata->offset == 0) {
			msg_type = NRF24_APP_FIRST;
			len = NRF24_PW_MSG_SIZE;
		} else {
			len -= pdata->offset;
			if (len > NRF24_PW_MSG_SIZE) {
				msg_type = NRF24_APP_FRAG;
				len = NRF24_PW_MSG_SIZE;
			} else {
				msg_type = NRF24_APP_LAST;
			}
		}
	} else {
		msg_type = pdata->msg_type;
	}
	payload.hdr.msg_xmn = MSGXMN_SET(msg_type, pc != NULL ? pc->txmn : 0);
	payload.hdr.net_addr = pdata->net_addr;
	memcpy(payload.msg.raw, pdata->raw + pdata->offset, len);
	pdata->offset_retry = pdata->offset;
	pdata->offset += len;
	DUMP_DATA("SEND: ", pdata->pipe, &payload, len + sizeof(hdr_t));
	nrf24l01_set_ptx(pdata->pipe);
	nrf24l01_ptx_data(&payload, len + sizeof(hdr_t), pdata->pipe != BROADCAST);
	len = nrf24l01_ptx_wait_datasent();
	nrf24l01_set_prx();
	return len;
}

static inline void put_ptx_queue(data_t *pd)
{
	G_LOCK(m_ptxlist);
	list_move_tail(&pd->node, &m_ptxlist);
	G_UNLOCK(m_ptxlist);
}

static int ptx_service(void)
{
	if (!list_empty(&m_ptxlist)) {
		static int send_state = PTX_FIRE;
		static ulong_t	start = 0,
									delay = 0;
		static data_t		*pdata = NULL;
		static client_t	*pc = NULL;

		switch (send_state) {
		case  PTX_FIRE:
			start = tline_ms();
			delay = get_random_value(SEND_RANGE_MS, SEND_FACTOR, SEND_RANGE_MS_MIN);
			send_state = PTX_GAP;
			/* No break; fall through intentionally */
		case PTX_GAP:
			if (!tline_out(tline_ms(), start, delay)) {
				break;
			}
			send_state = PTX;
			/* No break; fall through intentionally */
		case PTX:
			G_LOCK(m_ptxlist);
			pdata = list_first_entry(&m_ptxlist, data_t, node);
			list_del_init(&pdata->node);
			G_UNLOCK(m_ptxlist);
			pc = get_pipe(pdata->pipe);
			if (send_data(pdata, pc) != SUCCESS) {
				if (--pdata->retry == 0) {
					TERROR("Failed to send message to the pipe#%d\n", pdata->pipe);
					client_disconnect(pc);
					g_free(pdata);
				} else {
					pdata->offset = pdata->offset_retry;
					put_ptx_queue(pdata);
					TRACE("Resend(%d) message to the pipe#%d\n", pdata->retry, pdata->pipe);
				}
			} else {
				if (pc != NULL) {
					pc->heartbeat_wait = tline_ms();
					++pc->txmn;
				}
				if (pdata->len != pdata->offset) {
					TRACE("Message fragment sent to the pipe#%d len=%d\n", pdata->pipe, pdata->len);
					put_ptx_queue(pdata);
				} else {
					g_free(pdata);
					TRACE("Message sent to the pipe#%d\n", pdata->pipe);
				}
			}
			send_state = PTX_FIRE;
			break;
		}
	}
}

static gboolean client_watch(GIOChannel *io, GIOCondition cond, gpointer user_data)
{
	client_t *pc = (client_t*)user_data;
	int nbytes;
	byte_t *pmsg;

	if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL)) {
		return FALSE;
	}

	pmsg = g_new(byte_t, pc->packet_size);
	if (pmsg == NULL) {
		TERROR("read alloc error: %s\n", strerror(errno));
		return FALSE;
	}

	nbytes = read(pc->fdsock, pmsg, pc->packet_size);
	if (nbytes < 0) {
		TERROR("read() error - %s(%d)\n", strerror(errno), errno);
		g_free(pmsg);
		return FALSE;
	}

	put_ptx_queue(build_data(pc->pipe, pc->net_addr, NRF24_APP, pmsg, nbytes, NULL));
	g_free(pmsg);
	return TRUE;
}

static void client_destroy(gpointer user_data)
{
	client_t *pc = user_data;

	TRACE("pipe#%d disconnected\n", pc->pipe);
	m_pclients = g_slist_remove(m_pclients, pc);
	client_free(pc);
}

static int set_new_client(payload_t *ppl, int len)
{
	int	sock,
			pipe;
	struct sockaddr_un addr;
	GIOChannel *sock_io;

	client_t *pc = get_client(ppl);
	if (pc != NULL) {
		return pc->pipe;
	}

	pipe = get_pipe_free();
	if (pipe == NRF24L01_NO_PIPE) {
		return NRF24L01_NO_PIPE;
	}

	sock = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
	if (sock < 0) {
		TERROR(" >socket() failure: %s\n", strerror(errno));
		return NRF24L01_NO_PIPE;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	/* abstract namespace: first character must be null */
	strncpy(addr.sun_path+1, (const char *__restrict)m_paddr, m_laddr);
	if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		TERROR(" >connect() failure: %s\n", strerror(errno));
		close(sock);
		return NRF24L01_NO_PIPE;
	}

	pc = g_new0(client_t, 1);
	if (pc == NULL) {
		TERROR("client alloc error: %s\n", strerror(errno));
		close(sock);
		return NRF24L01_NO_PIPE;
	}

	pc->fdsock = sock;
	pc->pipe = pipe;
	pc->net_addr = ppl->hdr.net_addr;
	pc->packet_size = ppl->msg.join.version.packet_size;
	pc->hashid = ppl->msg.join.hashid;
	pc->heartbeat_wait = tline_ms();
	set_pipe(pipe, pc);

	sock_io = g_io_channel_unix_new(sock);
	if(sock_io == NULL)
	{
		TERROR("error - node channel creation failure\n");
		client_free(pc);
		return NRF24L01_NO_PIPE;
	}

	g_io_channel_set_close_on_unref(sock_io, FALSE);
	m_pclients = g_slist_append(m_pclients, pc);

	/* Watch for unix socket disconnection */
	pc->watch_id = g_io_add_watch_full(sock_io, G_PRIORITY_DEFAULT,
				G_IO_HUP | G_IO_NVAL | G_IO_ERR | G_IO_IN,
				client_watch, pc, client_destroy);
	/* Keep only one ref: GIOChannel watch */
	g_io_channel_unref(sock_io);

	TRACE("pipe#%d connected\n", pipe);
	return pipe;
}

static void check_heartbeat(gpointer pentry, gpointer user_data)
{
	if (tline_out(tline_ms(), ((client_t*)pentry)->heartbeat_wait, NRF24_HEARTBEAT_TIMEOUT_MS)) {
		TRACE(" IN\n");
		client_disconnect((client_t*)pentry);
	}
}

static int prx_service(void)
{
	payload_t	data;
	int	pipe,
			len;
	client_t *pc;
	int msg_type;

	for (pipe=nrf24l01_prx_pipe_available(); pipe!=NRF24L01_NO_PIPE; pipe=nrf24l01_prx_pipe_available()) {
		len = nrf24l01_prx_data(&data, sizeof(data));
		if (len > sizeof(data.hdr)) {
			DUMP_DATA("RECV: ", pipe, &data, len);
			len -= sizeof(data.hdr);
			msg_type = MSG_GET(data.hdr.msg_xmn);
			switch (msg_type) {
			case NRF24_GATEWAY_REQ:
				if (len == sizeof(data.msg.result) && pipe == BROADCAST) {
					data.msg.result = NRF24_ECONNREFUSED;
					put_ptx_queue(build_data(pipe, data.hdr.net_addr, msg_type, data.msg.raw, len, NULL));
				}
				break;

			case NRF24_JOIN_LOCAL:
				if (len != sizeof(join_t) || pipe != BROADCAST ||
					data.msg.join.version.major > m_pversion->major ||
					data.msg.join.version.minor > m_pversion->minor ) {
					break;
				}
				if (data.msg.join.version.packet_size > m_pversion->packet_size) {
					data.msg.join.data = m_pversion->packet_size;
					data.msg.join.result = NRF24_EOVERFLOW;
				} else {
					data.msg.join.data = set_new_client(&data, len);
					data.msg.join.result = (data.msg.join.data != NRF24L01_NO_PIPE) ? NRF24_SUCCESS : NRF24_EUSERS;
				}
				put_ptx_queue(build_data(pipe, data.hdr.net_addr, msg_type, data.msg.raw, len, NULL));
				break;

			case NRF24_UNJOIN_LOCAL:
				if (len != sizeof(join_t) ||
					data.msg.join.data != pipe ||
					data.msg.join.version.major > m_pversion->major ||
					data.msg.join.version.minor > m_pversion->minor ||
					data.msg.join.version.packet_size > m_pversion->packet_size) {
					break;
				}
				pc = get_client(&data);
				if (pc != NULL && (MSGXMN_SET(msg_type, pc->rxmn)  == data.hdr.msg_xmn)) {
					++pc->rxmn;
					TRACE(" IN\n");
					client_disconnect(pc);
				}
				break;

			case NRF24_HEARTBEAT:
				if (len != sizeof(join_t) ||
					data.msg.join.data != pipe ||
					data.msg.join.version.major > m_pversion->major ||
					data.msg.join.version.minor > m_pversion->minor ||
					data.msg.join.version.packet_size > m_pversion->packet_size) {
					break;
				}
				pc = get_client(&data);
				if (pc != NULL) {
					if (MSGXMN_SET(msg_type, pc->rxmn)  == data.hdr.msg_xmn) {
						++pc->rxmn;
						data.msg.join.result = NRF24_SUCCESS;
						put_ptx_queue(build_data(pipe, data.hdr.net_addr, msg_type, data.msg.raw, len, NULL));
					}
					pc->heartbeat_wait = tline_ms();
				}
				break;

			case NRF24_APP_FIRST:
			case NRF24_APP_FRAG:
				if (len != NRF24_PW_MSG_SIZE) {
					break;
				}
				/* No break; fall through intentionally */
			case NRF24_APP_LAST:
			case NRF24_APP:
				pc = get_pipe(pipe);
				if (pc != NULL && pc->pipe == pipe && pc->net_addr == data.hdr.net_addr &&
					 (MSGXMN_SET(msg_type, pc->rxmn)  == data.hdr.msg_xmn)) {
					int size;
					++pc->rxmn;
					pc->heartbeat_wait = tline_ms();
					if (msg_type == NRF24_APP || msg_type == NRF24_APP_FIRST) {
						g_free(pc->rxmsg);
						pc->rxmsg = NULL;
						size = len;
					} else {
						size = pc->rxmsg->len + len;
					}
					if (size <= pc->packet_size && size <= m_pversion->packet_size) {
						pc->rxmsg = build_data(pipe, data.hdr.net_addr, msg_type, data.msg.raw, len, pc->rxmsg);
						if (msg_type == NRF24_APP || msg_type == NRF24_APP_LAST) {
							write(pc->fdsock, pc->rxmsg->raw, pc->rxmsg->len);
							g_free(pc->rxmsg);
							pc->rxmsg = NULL;
						}
					}
				}
				break;
			}
		}
	}

	g_slist_foreach(m_pclients, check_heartbeat, NULL);

	ptx_service();

	return PRX;
}

static data_t *gwreq_build(void)
{
	int8_t result = NRF24_SUCCESS;
	return build_data(BROADCAST, 0, NRF24_GATEWAY_REQ, &result, sizeof(result), NULL);
}

static int gwreq_read(ulong_t start, ulong_t timeout)
{
	payload_t	data;
	int	pipe,
			len;

	for (pipe=nrf24l01_prx_pipe_available(); pipe!=NRF24L01_NO_PIPE; pipe=nrf24l01_prx_pipe_available()) {
		len = nrf24l01_prx_data(&data, sizeof(data));
		DUMP_DATA("RECV: ", pipe, &data, len > sizeof(data.hdr) ? len : 0);
		if (len == (sizeof(data.hdr)+sizeof(data.msg.result)) && pipe == BROADCAST &&
			MSG_GET(data.hdr.msg_xmn) == NRF24_GATEWAY_REQ &&
			data.msg.result == NRF24_ECONNREFUSED) {
			return REFUSED;
		}
	}

	return (tline_out(tline_ms(), start, timeout) ? TIMEOUT : PENDING);
}

static int gwreq(bool reset)
{
	static int	state = REQUEST,
					 	ch = CH_MIN,
					 	ch0 = CH_MIN;
	static ulong_t	start = 0,
								delay = 0;
	int ret = REQUEST;

	if (reset) {
		ch = nrf24l01_get_channel();
		ch0 = ch;
		m_gwreq->retry = m_gwreq->net_addr = 1;//TODO get_random_value(JOINREQ_RETRY, 2, JOINREQ_RETRY);
		state = REQUEST;
	}

	switch(state) {
	case REQUEST:
		delay = get_random_value(SEND_RANGE_MS, SEND_FACTOR, SEND_RANGE_MS_MIN);
		TRACE("Server joins ch#%d retry=%d/%d delay=%ld\n", ch, m_gwreq->net_addr, m_gwreq->retry, delay);
		send_data(m_gwreq, NULL);
		m_gwreq->offset = 0;
		start = tline_ms();
		state = PENDING;
		break;

	case PENDING:
		state = gwreq_read(start, delay);
		break;

	case TIMEOUT:
		if (--m_gwreq->net_addr == 0) {
			TRACE("Server join on channel #%d\n", nrf24l01_get_channel());
			ret = PRX;
			break;
		}

		state = REQUEST;
		break;

	case REFUSED:
		nrf24l01_set_standby();
		ch += NEXT_CH;
		if (nrf24l01_set_channel(ch) == ERROR) {
			ch = CH_MIN;
			nrf24l01_set_channel(ch);
		}
		if(ch == ch0) {
			TRACE("Server didn't find channel free\n");
			ret = CHANNEL_BUSY;
			break;
		}

		m_gwreq->retry = m_gwreq->net_addr = get_random_value(JOINREQ_RETRY, 2, JOINREQ_RETRY);
		state = REQUEST;
		break;
	}
	return ret;
}

static gboolean server_service(gpointer dummy)
{
	gboolean result = G_SOURCE_CONTINUE;

	if (m_fd == SOCKET_INVALID || (fcntl(m_fd, F_GETFL) < 0 && errno == EBADF)) {
		m_state = CLOSE;
	} else 	if (m_state == UNKNOWN) {
		m_state = OPEN;
	}
	switch(m_state) {
	case CLOSE:
		nrf24l01_set_standby();
		nrf24l01_close_pipe(5);
		nrf24l01_close_pipe(4);
		nrf24l01_close_pipe(3);
		nrf24l01_close_pipe(2);
		nrf24l01_close_pipe(1);
		nrf24l01_close_pipe(0);
		server_cleanup();
		m_state = UNKNOWN;
		if (m_fd != SOCKET_INVALID) {
			m_fd = SOCKET_INVALID;
			m_gthread == NULL;
		}
		g_main_loop_quit(m_loop);
		result = G_SOURCE_REMOVE;
		break;

	case OPEN:
		m_gwreq = gwreq_build();
		if (m_gwreq == NULL) {
			break;
		}
		g_mutex_init(&G_LOCK_NAME(m_ptxlist));
		nrf24l01_open_pipe(0, NRF24L01_PIPE0_ADDR);
		nrf24l01_open_pipe(1, NRF24L01_PIPE1_ADDR);
		nrf24l01_open_pipe(2, NRF24L01_PIPE2_ADDR);
		nrf24l01_open_pipe(3, NRF24L01_PIPE3_ADDR);
		nrf24l01_open_pipe(4, NRF24L01_PIPE4_ADDR);
		nrf24l01_open_pipe(5, NRF24L01_PIPE5_ADDR);
		m_state = gwreq(true);
		break;

	case REQUEST:
		m_state = gwreq(false);
		break;

	case CHANNEL_BUSY:
		close(m_fd);
		m_state = CLOSE_PENDING;
		break;

	case PRX:
		m_state = prx_service();
		break;
	}
	return result;
}

static gpointer service_thread(gpointer dummy)
{
    GSource *timer;
    GMainContext *context;

    context = g_main_context_new();
    m_loop = g_main_loop_new(context, FALSE);
    g_main_context_unref(context);
    timer = g_timeout_source_new(0);
    g_source_set_callback(timer, server_service, m_loop, NULL);
    g_source_attach(timer, context);
    g_source_unref(timer);
    g_main_loop_run(m_loop);
	g_main_loop_unref(m_loop);
    return NULL;
}

/*
 * Global functions
 */

int nrf24l01_server_open(int socket, int channel, version_t *pversion, const void *paddr, size_t laddr)
{
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

	TRACE("Server try to join on channel #%d\n", nrf24l01_get_channel());
	m_fd = socket;
	m_pversion = pversion;
	m_paddr = paddr;
	m_laddr = laddr;

	m_gthread = g_thread_new("service_thread", service_thread, NULL);
	if (m_gthread == NULL) {
		errno = ENOMEM;
		return ERROR;
	}

	errno = 0;
	return SUCCESS;
}

int nrf24l01_server_close(int socket)
{
	if (m_fd == SOCKET_INVALID || m_fd != socket) {
		errno = EBADF;
		return ERROR;
	}

	if (socket == m_fd) {
		m_fd = SOCKET_INVALID;
		if (m_gthread != NULL) {
			g_thread_join(m_gthread);
			m_gthread == NULL;
		}
	}
	errno = 0;
	return SUCCESS;
}
