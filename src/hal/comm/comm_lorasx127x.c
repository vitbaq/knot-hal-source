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
#include "lorasx127x_ll.h"
#include "sx127x_hal.h"
#include "sx127x.h"


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

static int write_mgmt_radio(void)
{
	struct lora_io_pack p;

	p.id = 0;

	memcpy(p.payload, mgmt.buffer_tx, mgmt.len_tx);

	radio_tx((uint8_t *)&p, mgmt.len_tx);
	mgmt.len_tx = 0;

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
	struct lora_ll_mgmt_pdu *mgmt_pdu;
	struct lora_ll_data_pdu *data_pdu;
	// struct lora_ll_mgmt_presence *mgmt_pres;
	// struct lora_ll_mgmt_connect *mgmt_conn;
	// struct lora_ll_data_ctrl *data_ctrl;

	size_t len_buffer = 0;

	radio_irq_handler(0, (uint8_t *)&p, &len_buffer);

	if (p.id == LORA_MGMT) {
		mgmt_pdu = (struct lora_ll_mgmt_pdu *)p.payload;
		switch (mgmt_pdu->opcode) {

		case LORA_MGMT_PDU_OP_PRESENCE:
			//mgmt_pres =
			//(struct lora_ll_mgmt_presence *)mgmt_pdu->payload;
			break;
		case LORA_MGMT_PDU_OP_CONNECT_REQ:
			//mgmt_conn =
			//(struct lora_ll_mgmt_connect *)mgmt_pdu->payload;
			break;
		}
	} else {
		data_pdu = (struct lora_ll_data_pdu *)p.payload;
		switch (data_pdu->opcode) {

		case LORA_PDU_OP_CONTROL:
			//data_ctrl =
			//(struct lora_ll_data_ctrl *)data_pdu->payload;
			break;
		case LORA_PDU_OP_DATA_FRAG:
		case LORA_PDU_OP_DATA_END:
			memcpy(buffer, data_pdu->payload, len_buffer-3);
			break;
		}
	}

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
	struct lora_ll_mgmt_pdu *opdu =
		(struct lora_ll_mgmt_pdu *)mgmt.buffer_tx;
	struct lora_ll_mgmt_connect *payload =
				(struct lora_ll_mgmt_connect *)opdu->payload;
	size_t len;

	/* If already has something to write then returns busy */
	if (mgmt.len_tx != 0)
		return -EBUSY;

	opdu->opcode = LORA_MGMT_PDU_OP_CONNECT_REQ;

	payload->mac_src = mac_local;
	payload->mac_dst.address.uint64 = *addr;
	payload->devaddr = sockfd-1;
	/*
	 * Set in payload the addr to be set in client.
	 * sockfd contains the pipe allocated for the client
	 * aa_pipes contains the Access Address for each pipe
	 */

	/* Source address for keepalive message */
	peers[sockfd-1].mac.address.uint64 = *addr;

	len = sizeof(struct lora_ll_mgmt_connect);
	len += sizeof(struct lora_ll_mgmt_pdu);

	mgmt.len_tx = len;

	return 0;
}
