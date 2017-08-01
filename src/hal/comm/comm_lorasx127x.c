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


int hal_comm_init(const char *pathname, const void *params)
{
	const struct lora_mac *mac = (const struct lora_mac *)params;

	/* Radio default config:
	 *	freq: 902.3    | tx pow: 27 | spreadfactor: 9
	 *	bandwidth: 125 | cr: 0 | ih: 0 | noCRC: 1
	 */
	radio_set_config(902300000, 27, SF9, BW125, 0, 0, 1);

	//Init gpio, spi and time.
	hal_init();

	//Init radio registers
	radio_init();

	//Put radio on rx mode.
	radio_rx(RXMODE_SCAN);

	mac_local.address.uint64 = mac->address.uint64;
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