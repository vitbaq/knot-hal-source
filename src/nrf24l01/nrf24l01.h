/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */
#include "types.h"
#include "nrf24l01_io.h"

#ifndef __NRF24L01_H__
#define __NRF24L01_H__

/*
 * DEFAULT values to initialize the nRF24L01
 */
#define RF_POWER					PWR_0DBM		// Output power in max power
#define RF_DATA_RATE			DR_2MBPS		// Data rate to 2Mbps
#define RF_CHANNEL				32						// Channel = 2400Ghz + CHANNEL_DEF [Mhz]
#define RF_ARC							15						// Auto retransmit count = 15 attempt
#define RF_ARD							ARD_1000US	// Auto retransmit delay = 1 ms
#define RF_ADDR_WIDTHS	5							// Address width is 5 bytes

#define PIPE_MIN				0									// pipe min
#define PIPE_MAX				5									// pipe max

#ifdef __cplusplus
extern "C"{
#endif

//>>>>>>>>>>
// TODO: functions to test, we can remove them on development end
result_t nrf24l01_inr(param_t reg);
void nrf24l01_inr_data(param_t reg, pparam_t pd, len_t len);
void nrf24l01_outr(param_t reg, param_t value);
void nrf24l01_outr_data(param_t reg, pparam_t pd, len_t len);
result_t nrf24l01_comand(param_t cmd);
void nrf24l01_set_address_pipe(param_t reg, param_t pipe);
//<<<<<<<<<<

/*
* nrf24l01_init - initialize the nRF24L01 device
*/
result_t 	nrf24l01_init(void);

result_t 	nrf24l01_deinit(void);

void		nrf24l01_set_channel(param_t ch);

result_t	nrf24l01_get_channel(void);

void		nrf24l01_open_pipe(param_t pipe);

void		nrf24l01_close_pipe(param_t pipe);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __NRF24L01_H__
