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

#include <glib.h>

#include "abstract_driver.h"

/*
 * Device session storing the connected
 * device context: 'drivers' and file descriptors
 */
typedef struct {
	volatile int				ref;
} session_t;

static GMainLoop *main_loop;

static gboolean node_io_watch(GIOChannel *io, GIOCondition cond,
			      gpointer user_data)
{
	char msg[32];
	session_t *ps = user_data;
	ssize_t nbytes;
	int sock;

	if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL)) {
		return FALSE;
	}

	sock = g_io_channel_unix_get_fd(io);

	nbytes = nrf24l01_driver.recv(sock, msg, sizeof(msg));
	if (nbytes < 0) {
		fprintf(stderr, "recv() error - %s(%d)\n", strerror(errno), errno);
		return FALSE;
	}

	fprintf(stdout, "MSG(%ld)>> '%s'\n", nbytes, msg);
	return TRUE;
}

static void session_release(session_t *ps)
{
	/* Decrements the reference count of session */
	--ps->ref;
	if(ps->ref == 0) {
		fprintf(stdout, "session released\n");
	}
}


static void node_io_destroy(gpointer user_data)
{
	session_t *ps = user_data;

	fprintf(stdout, "disconnected\n");

	/* session reference release */
	session_release(ps);
}

static gboolean accept_cb(GIOChannel *io, GIOCondition cond,
							gpointer user_data)
{
	GIOChannel *node_io;
	int sockfd, srv_sock;
	session_t *ps;

	fprintf(stderr, "accept(): %d\n", cond);
	if (cond & (G_IO_NVAL | G_IO_HUP | G_IO_ERR)) {
		return FALSE;
	}

	srv_sock = g_io_channel_unix_get_fd(io);
	sockfd = nrf24l01_driver.accept(srv_sock);
	if (sockfd < 0) {
		fprintf(stderr, "accept(%d): %s\n", -sockfd, strerror(-sockfd));
		return FALSE;
	}

	fprintf(stdout, "accepted socfd=%d\n", sockfd);
	ps = g_new0(session_t, 1);
	if(ps == NULL)
	{
		nrf24l01_driver.close(sockfd);
		fprintf(stderr, "error - session creation failure\n");
		return TRUE;
	}

	node_io = g_io_channel_unix_new(sockfd);
	if(node_io == NULL)
	{
		nrf24l01_driver.close(sockfd);
		g_free(ps);
		fprintf(stderr, "error - node channel creation failure\n");
		return TRUE;
	}

	/* Increments the reference count of session */
	++ps->ref;

	g_io_channel_set_close_on_unref(node_io, TRUE);

	/* Watch for unix socket disconnection */
	g_io_add_watch_full(node_io, G_PRIORITY_DEFAULT,
				G_IO_HUP | G_IO_NVAL | G_IO_ERR | G_IO_IN,
				node_io_watch, ps, node_io_destroy);
	/* Keep only one ref: GIOChannel watch */
	g_io_channel_unref(node_io);

	fprintf(stdout, "connected\n");

	return TRUE;
}

static int start_server(void)
{
	GIOCondition cond = G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL;
	GIOChannel *sock_io;
	int sock;

	nrf24l01_driver.probe();

	sock = nrf24l01_driver.socket();
	if (sock == ERROR) {
		fprintf(stderr, "socket(%d): %s\n", errno, strerror(errno));
		nrf24l01_driver.remove();
		return 1;
	}

	if (nrf24l01_driver.listen(sock, 10) == ERROR) {
		fprintf(stderr, "listen(%d): %s\n", errno, strerror(errno));
		nrf24l01_driver.close(sock);
		nrf24l01_driver.remove();
		return 2;
	}

	sock_io = g_io_channel_unix_new(sock);
	if(sock_io == NULL)
	{
		nrf24l01_driver.close(sock);
		nrf24l01_driver.remove();
		fprintf(stderr, "error - server channel creation failure\n");
		return 3;
	}

	g_io_channel_set_close_on_unref(sock_io, TRUE);
	g_io_channel_set_flags(sock_io, G_IO_FLAG_NONBLOCK, NULL);

	/* Use node_ops as parameter to allow multi drivers */
	g_io_add_watch(sock_io, cond, accept_cb, NULL);
	/* Keep only one ref: server watch  */
	g_io_channel_unref(sock_io);

	return 0;
}

static void stop_server(void)
{
	nrf24l01_driver.remove();
}
static void sig_term(int sig)
{
	g_main_loop_quit(main_loop);
}

int main(int argc, char *argv[])
{
	GOptionContext *context;
	GError *gerr = NULL;
	int err;

	fprintf(stdout, "NRF24 Server Test\n");

	err = start_server();
	if (err != 0) {
		return err;
	}

	signal(SIGTERM, sig_term);
	signal(SIGINT, sig_term);

	fprintf(stdout, "started OK\n");

	main_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(main_loop);
	g_main_loop_unref(main_loop);

	stop_server();

	fprintf(stdout, "finishing OK\n");

	return 0;
}
