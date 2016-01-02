/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */
#ifndef __NRF24L01_PROTO_NET_H__
#define __NRF24L01_PROTO_NET_H__

// net layer result codes
#define NRF24_SUCCESS						0
#define NRF24_ERROR							-1
#define NRF24_INVALID_VERSION		-2
#define NRF24_NO_JOIN							-3

// Protocol version
#define NRF24_VERSION_MAJOR		01
#define NRF24_VERSION_MINOR		00

// Network retransmiting parameters
#define NRF24_TIMEOUT	100		//miliseconds
#define NRF24_RETRIES		5

// Network messages
#define NRF24_MSG_INVALID					0x00
#define NRF24_MSG_JOIN_LOCAL			0x01
#define NRF24_MSG_UNJOIN_LOCAL	0x02
#define NRF24_MSG_JOIN_GATEWAY	0x03
#define NRF24_MSG_JOIN_RESULT		0x04
#define NRF24_MSG_APPMSG					0x05

/**
 * struct nrf24_header - net layer message header
 * @net_addr: net address
 * @msg_type: message type
 * @offset: message fragment offset
 *
 * This struct defines the network layer message header
 */
typedef struct __attribute__ ((packed)) {
	uint16_t		net_addr;
	uint8_t		msg_type;
	uint8_t		offset;
} nrf24_header;

// Network message size parameters
#define NRF24_PW_SIZE							32
#define NRF24_MSG_PW_SIZE				(NRF24_PW_SIZE-sizeof(nrf24_header))
#define NRF24_MSG_MAX_OFFSET	(NRF24_MSG_PW_SIZE*255)
#define NRF24_MSG_MAX_SIZE			((NRF24_MSG_PW_SIZE*NRF24_MSG_MAX_OFFSET)+NRF24_MSG_PW_SIZE)

/**
 * struct nrf24_join_local - net layer join local message
 * @net_maj_version: protocol version, major number
 * @net_min_version: protocol version, minor number
 * @result: result for the join process
 *
 * This struct defines the net layer join local message.
 */
typedef struct __attribute__ ((packed)) {
	uint8_t				maj_version;
	uint8_t				min_version;
	uint8_t				pipe;
	uint32_t				hashid;
	int8_t					result;
} nrf24_join_local;

/**
 * union nrf24_payload - defines a network layer payload
 * @hdr: net layer message header
 * @msg: application layer message
 * @join: net layer join local message
 *
 * This union defines the network layer payload.
 */
typedef struct __attribute__ ((packed))  {
	nrf24_header		hdr;
	union {
		nrf24_join_local	join;
		uint8_t					data[NRF24_MSG_PW_SIZE];
	} msg;
} nrf24_payload;

#define NRF24_JOIN_PW_SIZE		(sizeof(nrf24_header)+sizeof(nrf24_join_local))

#endif //	__NRF24L01_PROTO_NET_H__
