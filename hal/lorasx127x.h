/*
 * Copyright (c) 2017, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#ifndef __HAL_LORASX127X_H__
#define __HAL_LORASX127X_H__

#ifdef __cplusplus
extern "C" {
#endif

struct lora_mac {	//lora_mac == deveui
	union {
		uint8_t b[8];
		uint64_t uint64;
	} address;
};

#define LORA_PAYLOAD_SIZE		64

//nao necessario;
#define LORA_MGMT			00
#define LORA_DATA			01

/* Used to read/write operations */
struct lora_io_pack {
	uint8_t id;
	uint8_t payload[LORA_PAYLOAD_SIZE];
} __attribute__ ((packed));


#ifdef __cplusplus
}
#endif

#endif /* __HAL_LORASX127X_H__ */
