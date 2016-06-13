/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */
#include <stdint.h>

#ifndef __NRF24L01_PROTO_NET_H__
#define __NRF24L01_PROTO_NET_H__

// net layer result codes
#define NRF24_SUCCESS						0
#define NRF24_ERROR							-1
#define NRF24_INVALID_VERSION		-2
#define NRF24_ECONNREFUSED		-3

// Network retransmiting parameters
#define NRF24_TIMEOUT_MS								(3 * NRF24L01_ARC)		// 3ms is the ARD maximum of the last pipe, range 1ms <= ARD <= 3ms
#define NRF24_HEARTBEAT_SEND_MS			1000
#define NRF24_HEARTBEAT_TIMEOUT_MS		(NRF24_HEARTBEAT_SEND_MS * 2)
#define NRF24_RETRIES											200

// defines constant time values
#define SEND_DELAY_MS	1
#define SEND_INTERVAL	NRF24_TIMEOUT_MS	//SEND_DELAY_MS <= delay <= (SEND_INTERVAL * SEND_DELAY_MS)

// defines constant retry values
#define JOINREQ_RETRY	NRF24_RETRIES
#define SEND_RETRY			((NRF24_HEARTBEAT_TIMEOUT_MS - (NRF24_HEARTBEAT_SEND_MS + SEND_INTERVAL)) / SEND_INTERVAL)

#define BROADCAST			NRF24L01_PIPE0_ADDR

// Network messages
#define NRF24_GATEWAY_REQ	0x00
#define NRF24_JOIN_LOCAL			0x01
#define NRF24_UNJOIN_LOCAL	0x02
#define NRF24_HEARTBEAT			0x03
#define NRF24_APP							0x04
#define NRF24_APP_FIRST			0x05
#define NRF24_APP_FRAG				0x06
#define NRF24_APP_LAST				0x07

/**
 * struct hdr_t - net layer message header
 * @net_addr: net address
 * @msg_type: message type
 * @offset: message fragment offset
 *
 * This struct defines the network layer message header
 */
typedef struct __attribute__ ((packed)) {
	uint16_t		net_addr;
	uint8_t		msg_type;
} hdr_t;

// Network message size parameters
#define NRF24_PW_SIZE							32
#define NRF24_PW_MSG_SIZE				(NRF24_PW_SIZE - sizeof(hdr_t))

/**
 * struct version_t - network layer version
 * @major: protocol version, major number
 * @minor: protocol version, minor number
 * @packet_size: application packet size maximum
 *
 * This struct defines the network layer version.
 */
typedef struct __attribute__ ((packed)) {
	uint8_t				major;
	uint8_t				minor;
	uint16_t				packet_size;
} version_t;

/**
 * struct join_t - network layer join message
 * @result: join process result
 * @version: network layer version
 * @hashid: id for network
 * @data: join data
 *
 * This struct defines the network layer join message.
 */
typedef struct __attribute__ ((packed)) {
	int8_t					result;
	version_t			version;
	uint32_t				hashid;
	uint32_t				data;
} join_t;

/**
 * union payload_t - defines a network layer payload
 * @hdr: net layer message header
 * @result: process result
 * @join: net layer join local message
 * @raw: raw data of network layer
 *
 * This union defines the network layer payload.
 */
typedef struct __attribute__ ((packed))  {
	hdr_t			hdr;
	union {
		int8_t		result;
		join_t		join;
		uint8_t	raw[NRF24_PW_MSG_SIZE];
	} msg;
} payload_t;

#endif //	__NRF24L01_PROTO_NET_H__
