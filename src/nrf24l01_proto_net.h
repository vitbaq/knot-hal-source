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

// network messages
enum {
	NRF24_GATEWAY_REQ,
	NRF24_JOIN_LOCAL,
	NRF24_UNJOIN_LOCAL,
	NRF24_HEARTBEAT,
	NRF24_APP,
	NRF24_APP_FIRST,
	NRF24_APP_FRAG,
	NRF24_APP_LAST
};

// network result codes
enum {
	NRF24_SUCCESS,
	NRF24_ERROR,
	NRF24_INVALID_VERSION,
	NRF24_ECONNREFUSED,
	NRF24_EUSERS,
	NRF24_EOVERFLOW
};

// network retransmiting parameters
#define NRF24_TIMEOUT_MS								(NRF24L01_ARD_MAX * NRF24L01_ARC_MAX)
#define NRF24_HEARTBEAT_SEND_MS			1000
#define NRF24_HEARTBEAT_TIMEOUT_MS		(NRF24_HEARTBEAT_SEND_MS * 2)
#define NRF24_RETRIES											200

// constant time values
#define SEND_FACTOR					1
#define SEND_RANGE_MS_MIN	NRF24L01_ARD_MAX
#define SEND_RANGE_MS				NRF24_TIMEOUT_MS	//SEND_RANGE_MS_MIN <= delay <= (SEND_RANGE_MS * SEND_FACTOR)

// constant retry values
#define JOINREQ_RETRY	NRF24_RETRIES
#define SEND_RETRY			((NRF24_HEARTBEAT_TIMEOUT_MS - (NRF24_HEARTBEAT_SEND_MS + SEND_RANGE_MS)) / SEND_RANGE_MS)

#define BROADCAST			NRF24L01_PIPE0_ADDR

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
