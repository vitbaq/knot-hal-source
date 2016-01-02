/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */
#include "atypes.h"
#include "nrf24l01_io.h"

#ifndef __NRF24L01_H__
#define __NRF24L01_H__

/*
 * DEFAULT values to initialize the nRF24L01
 */
#define NRF24L01_POWER							PWR_0DBM		// Output power in max power
#define NRF24L01_DATA_RATE					DR_1MBPS		// Data rate to 1Mbps
#define NRF24L01_CHANNEL_DEFAULT	32						// Channel = 2400Ghz + CHANNEL_DEF [Mhz]
#define NRF24L01_ARC									5							// Auto retransmit count = 5 attempt
#define NRF24L01_ADDR_WIDTHS				5							// Address width is 5 bytes

#define NRF24L01_PIPE_MIN						0									// pipe min
#define NRF24L01_PIPE_MAX						5									// pipe max
#define NRF24L01_NO_PIPE							RX_FIFO_EMPTY	// invalid pipe

#define NRF24L01_PAYLOAD_SIZE				32

#ifdef __cplusplus
extern "C"{
#endif

//>>>>>>>>>>
// TODO: functions to test, we can remove them on development end
result_t nrf24l01_inr(param_t reg);
void nrf24l01_inr_data(param_t reg, pparam_t pd, len_t len);
void nrf24l01_outr(param_t reg, param_t value);
void nrf24l01_outr_data(param_t reg, pparam_t pd, len_t len);
result_t nrf24l01_command(param_t cmd);
void nrf24l01_set_address_pipe(param_t reg, param_t pipe);
result_t nrf24l01_ce_on(void);
result_t nrf24l01_ce_off(void);
//<<<<<<<<<<

/*
* nrf24l01_init - initialize the nRF24L01 device
*/
result_t	nrf24l01_init(void);
result_t	nrf24l01_deinit(void);
result_t	nrf24l01_set_channel(param_t ch);
result_t	nrf24l01_get_channel(void);
result_t	nrf24l01_open_pipe(param_t pipe);
result_t	nrf24l01_close_pipe(param_t pipe);
result_t	nrf24l01_set_standby(void);
result_t	nrf24l01_set_prx(void);
result_t nrf24l01_prx_pipe_available(void);
result_t	nrf24l01_prx_data(pparam_t pdata, len_t len);
result_t	nrf24l01_set_ptx(param_t pipe);
result_t	nrf24l01_ptx_data(pparam_t pdata, len_t len, bool ack);
result_t nrf24l01_ptx_empty(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __NRF24L01_H__
