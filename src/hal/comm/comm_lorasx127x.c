/*
 * Copyright (c) 2017, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#ifdef ARDUINO
#include "hal/avr_errno.h"
#include "hal/avr_unistd.h"
#else
#include <errno.h>
#include <unistd.h>
#endif

#include "hal/comm.h"
#include "hal/time.h"
#include "hal/lorasx127x.h"
#include "sx127x_hal.h"
#include "sx127x.h"


#define DATA_SIZE		64

#define FLAG_TIMEOUT		1000

enum {
	DATA,
	CHECK,
	START
};

/* GW address */
static struct lora_mac mac_local = {.address.uint64 = 0};

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

/* ARRAY SIZE */
#define CONNECTION_COUNTER	((int) (sizeof(peers) \
				 / sizeof(peers[0])))


static void init_peers(void)
{
	int i;

	for (i = 0; i < MAX_PEERS; i++) {
		peers[i].sock = -1;
		peers[i].len_tx = 0;
		peers[i].seqnumber_tx = 0;
	}
}

static int alloc_peer(void)
{
	int i;

	for (i = 0; i < CONNECTION_COUNTER; i++) {
		if (peers[i].sock == -1) {
			memset(&peers[i], 0, sizeof(peers[0]));
			peers[i].sock = i + 1;
			return peers[i].sock;
		}
	}

	return -1;
}

static int write_data_radio(int sockfd)
{
	struct lora_io_pack p;

	p.id = sockfd;

	memcpy(p.payload, peers[sockfd-1].buffer_tx, peers[sockfd-1].len_tx);

	radio_tx((uint8_t *)&p, peers[sockfd-1].len_tx);

	return 0;
}


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

	//Init peers.
	init_peers();

	//Put radio on rx mode.
	radio_rx(RXMODE_SCAN);

	mac_local.address.uint64 = mac->address.uint64;
	return 0;
}

int hal_comm_deinit(void)
{
	init_peers();

	//Unmap all gpio pins
	hal_pins_unmap();

	//Put radio on sleep mode.
	radio_sleep();
	return 0;
}

int hal_comm_socket(int domain, int protocol)
{
	int retval;

	if (domain != HAL_COMM_PF_LORA)
		return -EPERM;			//TODO: change error

	switch (protocol) {

	case HAL_COMM_PROTO_RAW:
		retval = alloc_peer();
		if (retval < 0)
			return -EUSERS;

		return retval;
	default:
		return -EINVAL;
	}
}

int hal_comm_close(int sockfd)
{
	if (sockfd >= 1 && sockfd <= 255 && peers[sockfd-1].sock != -1) {
		/*TODO: Send disconnect packet */
		peers[sockfd-1].sock = -1;
		peers[sockfd-1].len_tx = 0;
	}

	return 0;
}

ssize_t hal_comm_read(int sockfd, void *buffer, size_t count)
{
	struct lora_io_pack p;

	size_t len_buffer = 0;

	if (!radio_irq_flag(IRQ_LORA_RXDONE_MASK))
		return 0;

	radio_irq_handler(0, (uint8_t *)&p, &len_buffer);

	memcpy(buffer, p.payload, len_buffer-1);

	//After read, put radio on rx mode.
	radio_rx(RXMODE_SCAN);
	return len_buffer;
}

ssize_t hal_comm_write(int sockfd, const void *buffer, size_t count)
{
	if (peers[sockfd-1].len_tx != 0)
		return -EBUSY;

	/* Copy data to be write in tx buffer */
	memcpy(peers[sockfd-1].buffer_tx, buffer, count);
	peers[sockfd-1].len_tx = count;

	return count;
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

void hal_comm_process(void)
{
	static int sockIndex = 1;
	static int state = DATA;

	static unsigned long time_start;

	switch (state) {
	case DATA:
		/*
		 * Check if peer has something to send and
		 * the radio don't receive anything
		 */
		if (peers[sockIndex-1].len_tx &&
					!radio_irq_flag(IRQ_LORA_RXDONE_MASK)) {
			time_start = hal_time_ms();
			write_data_radio(sockIndex);
			state = CHECK;
		} else {
			sockIndex++;
			if (sockIndex > CONNECTION_COUNTER)
				sockIndex = 1;
		}
		break;
	case CHECK:
		//Check if TX is finished or timeout occurred
		if (radio_irq_flag(IRQ_LORA_TXDONE_MASK)) {
			state = START;
		} else if (hal_timeout(hal_time_ms(),
						time_start, FLAG_TIMEOUT) > 0) {
			state = DATA;
		}
		break;
	case START:
		//Clear peer tx buffer
		peers[sockIndex-1].len_tx = 0;

		//After a transmission put radio on rx mode
		radio_rx(RXMODE_SCAN);
		state = DATA;

		sockIndex++;
		if (sockIndex > CONNECTION_COUNTER)
			sockIndex = 1;
		break;
	}
}
