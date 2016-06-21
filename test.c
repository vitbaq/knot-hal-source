/*
 * This file is part of the KNOT Project
 *
 * Copyright (c) 2015, CESAR. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the CESAR nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL CESAR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include <glib.h>

#include "abstract_driver.h"
#include "util.h"

// application packet size maximum
#define PACKET_SIZE_MAX		128

/* Abstract unit socket name space */
#define KNOT_UNIX_SOCKET				"test_knot_nrf24l01"
#define KNOT_UNIX_SOCKET_SIZE		(sizeof(KNOT_UNIX_SOCKET) - 1)

#define MESSAGE				"GOD YZAL EHT REVO SPMUJ XOF NWORB KCIUQ EHT"
#define MESSAGE_SIZE	(sizeof(MESSAGE)-1)

/*
 * Device session storing the connected
 * device context: 'drivers' and file descriptors
 */
typedef struct {
	volatile int		ref;
	int					count,
							msg_count,
							inc,
							size,
							sock;
	char				msg[PACKET_SIZE_MAX];
} session_t;

static GMainLoop *main_loop;
static guint	watch_id;

static gboolean node_io_watch(GIOChannel *io, GIOCondition cond,
			      gpointer user_data)
{
	char buffer[PACKET_SIZE_MAX];
	session_t *ps = user_data;
	ssize_t nbytes;
	int sock;

	if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL)) {
		return FALSE;
	}

	sock = g_io_channel_unix_get_fd(io);

	//fprintf(stdout, "SERVER:\n");
	nbytes = read(sock, buffer, sizeof(buffer));
	if (nbytes < 0) {
		fprintf(stderr, "read() error - %s(%d)\n", strerror(errno), errno);
		return FALSE;
	}

	//fprintf(stdout, "RX(%d):[%03ld]: '%.*s'\n", sock, nbytes, (int)nbytes, buffer);

	if (nbytes != 0) {
		nbytes = sprintf(buffer, "%.*s", ps->count, ps->msg);
		ps->inc = (ps->count == ps->size) ? -1 : (ps->count == 1 ? 1 : ps->inc);
		ps->count += ps->inc;
		if (write(sock, buffer, nbytes) < 0) {
			fprintf(stderr, "write() error - %s(%d)\n", strerror(errno), errno);
			return FALSE;
		}
		//fprintf(stdout, "TX(%d):[%03ld]: %d-'%.*s'\n", sock, nbytes, ++ps->msg_count, (int)nbytes, buffer);
	}

	return TRUE;
}

static void session_release(session_t *ps)
{
	/* Decrements the reference count of session */
	fprintf(stdout, "session(%d) released\n", ps->sock);
	--ps->ref;
	if(ps->ref == 0) {
		g_free(ps);
	}
}


static void node_io_destroy(gpointer user_data)
{
	session_t *ps = user_data;

	/* session reference release */
	session_release(ps);

	fprintf(stdout, "disconnected\n");
}

static gboolean accept_cb(GIOChannel *io, GIOCondition cond,
							gpointer user_data)
{
	GIOChannel *node_io;
	int sockfd, srv_sock;
	session_t *ps;

	if (cond & (G_IO_NVAL | G_IO_HUP | G_IO_ERR)) {
		return FALSE;
	}

	srv_sock = g_io_channel_unix_get_fd(io);
	sockfd = accept(srv_sock, NULL, NULL);
	if (sockfd < 0) {
		fprintf(stderr, "accept(%d): %s\n", -sockfd, strerror(-sockfd));
		return FALSE;
	}

	ps = g_new0(session_t, 1);
	if(ps == NULL)
	{
		close(sockfd);
		fprintf(stderr, "error - session creation failure\n");
		return TRUE;
	}

	node_io = g_io_channel_unix_new(sockfd);
	if(node_io == NULL)
	{
		close(sockfd);
		g_free(ps);
		fprintf(stderr, "error - node channel creation failure\n");
		return TRUE;
	}

	g_io_channel_set_close_on_unref(node_io, TRUE);

	/* Increments the reference count of session */
	++ps->ref;
	for (ps->size=0, ps->inc=MESSAGE_SIZE; ps->size<sizeof(ps->msg);) {
		if ((ps->inc+ps->size) > sizeof(ps->msg)) {
			ps->inc = sizeof(ps->msg) - ps->size;
		}
		ps->size += sprintf(ps->msg+ps->size, "%.*s", ps->inc, MESSAGE);
	}
	ps->count = get_random_value(ps->size, 1, 1);
	ps->inc = 1;
	ps->sock = sockfd;

	/* Watch for unix socket disconnection */
	g_io_add_watch_full(node_io, G_PRIORITY_DEFAULT,
				G_IO_HUP | G_IO_NVAL | G_IO_ERR | G_IO_IN,
				node_io_watch, ps, node_io_destroy);
	/* Keep only one ref: GIOChannel watch */
	g_io_channel_unref(node_io);

	fprintf(stdout, "session(%d) established\n", sockfd);
	return TRUE;
}

static int start_server(void)
{
	GIOCondition cond = G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL;
	GIOChannel *sock_io;
	int sock;

	if (nrf24l01_driver.probe(PACKET_SIZE_MAX) < 0) {
		fprintf(stderr, "probe(%d): %s\n", errno, strerror(errno));
		return 1;
	}

	sock = nrf24l01_driver.socket();
	if (sock < 0) {
		fprintf(stderr, "socket(%d): %s\n", errno, strerror(errno));
		nrf24l01_driver.remove();
		return 2;
	}

	if (nrf24l01_driver.listen(sock, 10, KNOT_UNIX_SOCKET, KNOT_UNIX_SOCKET_SIZE) < 0) {
		fprintf(stderr, "listen(%d): %s\n", errno, strerror(errno));
		nrf24l01_driver.close(sock);
		nrf24l01_driver.remove();
		return 3;
	}

	sock_io = g_io_channel_unix_new(sock);
	if(sock_io == NULL)
	{
		nrf24l01_driver.close(sock);
		nrf24l01_driver.remove();
		fprintf(stderr, "error - server channel creation failure\n");
		return 4;
	}

	g_io_channel_set_close_on_unref(sock_io, TRUE);
	g_io_channel_set_flags(sock_io, G_IO_FLAG_NONBLOCK, NULL);

	/* Use node_ops as parameter to allow multi drivers */
	watch_id = g_io_add_watch(sock_io, cond, accept_cb, NULL);
	/* Keep only one ref: server watch  */
	g_io_channel_unref(sock_io);

	fprintf(stdout, "Server is listening(%d)\n", sock);
	return 0;
}

static void stop_server(void)
{
	if (watch_id != 0) {
		g_source_remove(watch_id);
	}
	nrf24l01_driver.remove();
}

static void sig_term(int sig)
{
	g_main_loop_quit(main_loop);
	putchar('\n');
}

int main(int argc, char *argv[])
{
	int result;

	fprintf(stdout, "Server starting...\n");

	result = start_server();
	if (result == 0) {
		signal(SIGTERM, sig_term);
		signal(SIGINT, sig_term);

		main_loop = g_main_loop_new(NULL, FALSE);
		g_main_loop_run(main_loop);
		g_main_loop_unref(main_loop);

		stop_server();

		fprintf(stdout, "Server finished.\n");
	}

	return result;
}
