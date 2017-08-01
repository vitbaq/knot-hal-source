/*
 * Copyright (c) 2017, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include "include/comm.h"
#include "hal/lorasx127x.h"

#define MGMT_SIZE		60
#define DATA_SIZE		60

/* GW address */
static struct lora_mac mac_local = {.address.uint64 = 0};

struct lora_mgmt {
	uint8_t buffer_tx[MGMT_SIZE];
	size_t len_tx;
};

static struct lora_mgmt mgmt = {.len_tx = 0};

struct lora_data{
	int8_t sock;
	uint8_t buffer_tx[DATA_SIZE];
	size_t len_tx;
	uint8_t seqnumber_tx;
	struct lora_mac mac;
};

#ifndef ARDUINO	/* If master then 5 peers */
#define MAX_PEERS		255
#else	/* If slave then 1 peer */
#define MAX_PEERS		1
#endif

static struct lora_data peers[MAX_PEERS];


int hal_comm_init(const char *pathname, void *params)
{
	return 0;
}

int hal_comm_deinit(void)
{
	return 0;
}

int hal_comm_socket(int domain, int protocol)
{
	return 0;
}

int hal_comm_close(int sockfd)
{
	return 0;
}

ssize_t hal_comm_read(int sockfd, void *buffer, size_t count)
{
	return 0;
}

ssize_t hal_comm_write(int sockfd, const void *buffer, size_t count)
{
	return 0;
}

int hal_comm_listen(int sockfd)
{
	return 0;
}

int hal_comm_accept(int sockfd, void *addr)
{
	return 0;
}

int hal_comm_connect(int sockfd, uint64_t *addr)
{
	return 0;
}
