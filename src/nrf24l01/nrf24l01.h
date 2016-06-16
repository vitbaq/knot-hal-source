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
#define NRF24L01_POWER							PWR_0DBM	// Output power in max power
#define NRF24L01_DATA_RATE					DR_1MBPS		// Data rate in Mbps
#define NRF24L01_CHANNEL_DEFAULT	CH_MIN				// Channel = 2400GHz + CHANNEL_DEF [MHz], max 2.525GHz
#define NRF24L01_ARC_MAX						15						// Auto Retransmit Count maximum = 15 attempt
#define NRF24L01_ARD_MAX						4							// Auto Retransmit Delay maximum = 4 ms
#define NRF24L01_ADDR_WIDTHS				5							// Address width is 5 bytes

#define NRF24L01_PIPE_MIN						0									// pipe min
#define NRF24L01_PIPE_MAX						5									// pipe max
#define NRF24L01_NO_PIPE							RX_FIFO_EMPTY	// invalid pipe

#define NRF24L01_PAYLOAD_SIZE				32

#define NRF24L01_PIPE0_ADDR				0
#define NRF24L01_PIPE1_ADDR				1
#define NRF24L01_PIPE2_ADDR				2
#define NRF24L01_PIPE3_ADDR				3
#define NRF24L01_PIPE4_ADDR				4
#define NRF24L01_PIPE5_ADDR				5
#define NRF24L01_PIPE_ADDR_MAX		NRF24L01_PIPE5_ADDR

#ifdef __cplusplus
extern "C"{
#endif

//>>>>>>>>>>
// TODO: functions to test, we can remove them on development end
result_t nrf24l01_inr(byte_t reg);
void nrf24l01_inr_data(byte_t reg, pdata_t pd, len_t len);
void nrf24l01_outr(byte_t reg, byte_t value);
void nrf24l01_outr_data(byte_t reg, pdata_t pd, len_t len);
result_t nrf24l01_command(byte_t cmd);
void nrf24l01_set_address_pipe(byte_t reg, byte_t pipe);
result_t nrf24l01_ce_on(void);
result_t nrf24l01_ce_off(void);
//<<<<<<<<<<

/*
* nrf24l01_init - initialize the nRF24L01 device
*/
result_t nrf24l01_init(void);
result_t nrf24l01_deinit(void);
result_t nrf24l01_set_channel(byte_t ch);
result_t nrf24l01_get_channel(void);
result_t nrf24l01_open_pipe(byte_t pipe, byte_t pipe_addr);
result_t nrf24l01_close_pipe(byte_t pipe);
result_t nrf24l01_set_standby(void);
result_t nrf24l01_set_prx(void);
result_t nrf24l01_prx_pipe_available(void);
result_t nrf24l01_prx_data(pdata_t pdata, len_t len);
result_t nrf24l01_set_ptx(byte_t pipe_addr);
result_t nrf24l01_ptx_data(pdata_t pdata, len_t len, bool ack);
result_t nrf24l01_ptx_wait_datasent(void);
result_t nrf24l01_ptx_isempty(void);
result_t nrf24l01_ptx_isfull(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __NRF24L01_H__
