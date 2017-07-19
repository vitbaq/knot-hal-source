/*
 * Copyright (c) 2017, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include "hal/lorasx127x.h"


#define LORA_MTU					64

#define LORA_MGMT_PDU_OP_PRESENCE	0x00
#define LORA_MGMT_PDU_OP_CONNECT_REQ	0x01

struct lora_ll_mgmt_pdu
{
	uint8_t opcode:4;
	uint8_t rfu:4;
	uint8_t payload[0];
} __attribute__ ((packed));

struct lora_ll_mgmt_presence
{
	struct lora_mac mac;
	uint8_t name[0];
} __attribute__ ((packed));

struct lora_ll_mgmt_connect
{
	struct lora_mac mac_src;
	struct lora_mac mac_dst;
	uint8_t devaddr;
} __attribute__ ((packed));


/*
TO.DO
data opcode:	data_end	00
		data_frag	01
		data_crtl	10
*/
#define LORA_PDU_OP_DATA_FRAG		0x00 /* Data: Beginning or fragment */
#define LORA_PDU_OP_DATA_END		0x01 /* Data: End of fragment or complete */
#define LORA_PDU_OP_CONTROL		0x03 /* Control */

struct lora_ll_data_pdu
{
	uint8_t opcode:2;
	uint8_t nseq:4;
	uint8_t rfu;
	uint8_t payload[0];
} __attribute__ ((packed));


#define LORA_CTRL_ID_KEEPALIVE		0x00
#define LORA_CTRL_ID_DISCONNECT		0x01

struct lora_ll_data_ctrl
{
	uint8_t opcode;
	uint8_t payload[0];
} __attribute__ ((packed));
