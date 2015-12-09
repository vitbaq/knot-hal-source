/*
* Copyright (c) 2015, CESAR.
* All rights reserved.
*
* This software may be modified and distributed under the terms
* of the BSD license. See the LICENSE file for details.
*
*/
#include "nrf24l01.h"
#include "spi.h"

// Alias for spi_transfer function of the SPI module
#define rf_io		spi_transfer

// Pipes addresses base
#define	PIPE0_ADDR_BASE	0x55aa55aa5aLL
#define	PIPE1_ADDR_BASE	0xaa55aa55a5LL

// data word size
#define DATA_SIZE	sizeof(uint8_t)

// *************heartbeat*************

typedef struct {
	uint8_t	enaa,
					en_rxaddr,
					rx_addr,
					rx_pw;
} pipe_reg_t;
static const pipe_reg_t pipe_reg[] PROGMEM = {
	{ AA_P0, RXADDR_P0, RX_ADDR_P0, RX_PW_P0 },
	{ AA_P1, RXADDR_P1, RX_ADDR_P1, RX_PW_P1 },
	{ AA_P2, RXADDR_P2, RX_ADDR_P2, RX_PW_P2 },
	{ AA_P3, RXADDR_P3, RX_ADDR_P3, RX_PW_P3 },
	{ AA_P4, RXADDR_P4, RX_ADDR_P4, RX_PW_P4 },
	{ AA_P5, RXADDR_P5, RX_ADDR_P5, RX_PW_P5 }
};

typedef enum {
	NONE_MODE,
	RX_MODE,
	TX_MODE,
	STANDBY_II_MODE,
	STANDBY_I_MODE,
	UNKOWN_MODE,
	POWER_DOWN_MODE,
} en_modes_t ;
static en_modes_t m_mode = NONE_MODE;

//uint32_t m_txRxDelay; /**< Var for adjusting delays depending on datarate */

#ifdef ARDUINO
#include <Arduino.h>

#define CE	1

#define DELAY(d)	delay(d)

/* ----------------------------------
 * Local operation functions
 */

static inline void enable(void)
{
	PORTB |= (1 << CE);
}

static inline void disable(void)
{
	PORTB &= ~(1 << CE);
}

static inline void io_setup()
{
	PORTB &= ~(1 << CE);
	DDRB |= (1 << CE);

	disable();
	spi_init();
}

static inline void io_reset()
{
	disable();
	spi_deinit();
}

#else		// ifdef (ARDUINO)

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>

#define CE	25

#define LOW	0
#define HIGH	1

#define DELAY(d)	usleep(d)

#ifdef RASP_MODEL_1

#define BCM2708_PERI_BASE 0x20000000

#else

#define BCM2708_PERI_BASE 0x3F000000

#endif

#define GPIO_BASE (BCM2708_PERI_BASE + 0x200000)
#define PAGE_SIZE 4*1024
#define BLOCK_SIZE 4*1024

//Raspberry pi GPIO Macros
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET *(gpio+7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR *(gpio+10) // clears bits which are 1 ignores bits which are 0

#define GET_GPIO(g) (*(gpio+13)&(1<<g)) // 0 if LOW, (1<<g) if HIGH

#define GPIO_PULL *(gpio+37) // Pull up/pull down
#define GPIO_PULLCLK0 *(gpio+38) // Pull up/pull down clock

static volatile unsigned *gpio;

/* ----------------------------------
 * Local operation functions
 */

static inline void enable(void)
{
	GPIO_SET |= (1<<CE);
}

static inline void disable(void)
{
	GPIO_CLR |= (1<<CE);
}

static void io_setup()
{
	void *gpio_map;

	//open /dev/mem
	int mem_fd = open("/dev/mem", O_RDWR|O_SYNC);
	if (mem_fd < 0) {
		printf("can't open /dev/mem \n");
		exit(-1);
	}

	gpio_map = mmap(NULL,
										BLOCK_SIZE,
										PROT_READ | PROT_WRITE,
										MAP_SHARED,
										mem_fd,
										GPIO_BASE);
   	close(mem_fd);
   	if (gpio_map == MAP_FAILED) {
      		printf("mmap error\n");
      		exit(-1);
   	}

   	gpio = (volatile unsigned *)gpio_map;
	GPIO_CLR |= (1<<CE);
	INP_GPIO(CE);
	OUT_GPIO(CE);

	disable();
	spi_init();
}

static inline void io_reset()
{
	disable();
	munmap((void*)gpio, BLOCK_SIZE);
	spi_deinit();
}

#endif		// ifdef (ARDUINO)

static inline result_t inr(param_t reg)
{
	result_t value = NOP;
	reg = R_REGISTER(reg);
	rf_io(&reg, DATA_SIZE, &value, DATA_SIZE);
	return value;
}

static inline void inr_data(param_t reg, pparam_t pd, len_t len)
{
	memset(pd, NOP, len);
	reg = R_REGISTER(reg);
	rf_io(&reg, DATA_SIZE, pd, len);
}

static inline void outr(param_t reg, param_t value)
{
	reg = W_REGISTER(reg);
	rf_io(&reg, DATA_SIZE, &value, DATA_SIZE);
}

static inline void outr_data(param_t reg, pparam_t pd, len_t len)
{
	reg = W_REGISTER(reg);
	rf_io(&reg, DATA_SIZE, pd, len);
}

static inline result_t comand(param_t cmd)
{
	rf_io(NULL, 0, &cmd, DATA_SIZE);
	// return device status register
	return cmd;
}

static void set_address_pipe(param_t reg, param_t pipe)
{
	uint64_t	pipe_addr = (pipe == 0) ? PIPE0_ADDR_BASE : PIPE1_ADDR_BASE;

	pipe_addr += (pipe << 4) + pipe;
	outr_data(reg, &pipe_addr, (reg == TX_ADDR || pipe < 2) ? AW_RD(inr(SETUP_AW)) : DATA_SIZE);
}

static void standby1(void)
{
   result_t config = inr(CONFIG);

   // check if device in power down mode, and so put device in standby-I mode
   if((config & CFG_PWR_UP) == LOW) {
      outr(CONFIG, config | CFG_PWR_UP);
      m_mode = STANDBY_I_MODE;
	  // delay time to Tpd2stby timing
      DELAY(5);
   }
}

/*	-----------------------------------
 * Public operation functions
 */
//>>>>>>>>>>
// TODO: functions to test, we can remove them on development end
result_t nrf24l01_inr(param_t reg)
{
	return inr(reg);
}

void nrf24l01_inr_data(param_t reg, pparam_t pd, len_t len)
{
	inr_data(reg, pd, len);
}

void nrf24l01_outr(param_t reg, param_t value)
{
	outr(reg, value);
}

void nrf24l01_outr_data(param_t reg, pparam_t pd, len_t len)
{
	outr_data(reg, pd, len);
}

result_t nrf24l01_comand(param_t cmd)
{
	return comand(cmd);
}

void nrf24l01_set_address_pipe(param_t reg, param_t pipe)
{
	set_address_pipe(reg, pipe);
}
//<<<<<<<<<<

result_t nrf24l01_deinit(void)
{
	result_t	value;

	if(m_mode == NONE_MODE || m_mode == POWER_DOWN_MODE)
		return 1;

	m_mode = POWER_DOWN_MODE;

	// Set device in power down mode
	outr(CONFIG, inr(CONFIG) & ~CFG_PWR_UP);

	io_reset();
	return 0;
}

result_t nrf24l01_init(void)
{
	result_t	value;

	if(m_mode != NONE_MODE && m_mode != POWER_DOWN_MODE)
		return 1;

	io_setup();

	// Set device in power down mode
	outr(CONFIG, inr(CONFIG) & ~CFG_PWR_UP);
	// Delay to establish to operational timing of the nRF24L01
	DELAY(5);

	// Enable CRC 16-bit and disable all interrupts
	value = inr(CONFIG) & ~CONFIG_MASK;
	outr(CONFIG, value | CFG_EN_CRC | CFG_CRCO |
						  	  	   	   	   CFG_MASK_RX_DR	| CFG_MASK_TX_DS	| CFG_MASK_MAX_RT);

	// Reset pending status
	value = inr(STATUS) & ~STATUS_MASK;
	outr(STATUS, value | ST_RX_DR | ST_TX_DS | ST_MAX_RT);

	// Reset channel and TX observe registers
	outr(RF_CH, inr(RF_CH) & ~RF_CH_MASK);
	// Set the device channel
	outr(RF_CH, CH(RF_CHANNEL));

	// Set RF speed and output power
	value = inr(RF_SETUP) & ~RF_SETUP_MASK;
	outr(RF_SETUP, value | RF_DR(RF_DATA_RATE) | RF_PWR(RF_POWER));

	// Set address widths
	value = inr(SETUP_AW) & ~SETUP_AW_MASK;
	outr(SETUP_AW, value | AW(RF_ADDR_WIDTHS));

#if (RF_ARC == ARC_DISABLE)
	// Disable Auto Retransmit Count
	outr(SETUP_RETR, RETR_ARC(ARC_DISABLE));
#else
	// Set Auto Retransmision Delay and Auto Retransmit Count
	outr(SETUP_RETR, RETR_ARD(RF_ARD) | RETR_ARC(RF_ARC));
#endif

	// Disable all Auto Acknowledgment of pipes
	outr(EN_AA, inr(EN_AA) & ~EN_AA_MASK);

	// Disable all RX addresses
	outr(EN_RXADDR, inr(EN_RXADDR) & ~EN_RXADDR_MASK);

	// Enable dynamic payload to all pipes
	outr(FEATURE, (inr(FEATURE) & ~FEATURE_MASK) | FT_EN_DPL);
	value = inr(DYNPD) & ~DYNPD_MASK;
	outr(DYNPD, value | (DPL_P5 | DPL_P4 | DPL_P3 | DPL_P2 | DPL_P1 | DPL_P0));

	// Reset all the FIFOs
	comand(FLUSH_TX);
	comand(FLUSH_RX);

	// Set device in standby-I mode
	standby1();

	return 0;
}

void nrf24l01_set_channel(param_t ch)
{
	const param_t max = RF_DR(inr(RF_SETUP)) == DR_2MBPS ? CH_MAX_2MBPS : CH_MAX_1MBPS;

	// Set the device channel
	outr(RF_CH, _CONSTRAIN(ch, CH_MIN, max));
}

result_t nrf24l01_get_channel(void)
{
	return inr(RF_CH);
}

void nrf24l01_open_pipe(param_t pipe)
{
	pipe_reg_t rpipe;

	if(pipe > PIPE_MAX) {
		return;
	}

	memcpy_P(&rpipe, &pipe_reg[pipe], sizeof(pipe_reg_t));

	if(!(inr(EN_RXADDR) & rpipe.en_rxaddr)) {
		set_address_pipe(rpipe.rx_addr, pipe);
		outr(EN_RXADDR, inr(EN_RXADDR) | rpipe.en_rxaddr);
#if (RF_ARC != ARC_DISABLE)
		if(pipe == 0) {
			outr(EN_AA, inr(EN_AA) & ~rpipe.enaa);
		} else {
			outr(EN_AA, inr(EN_AA) | rpipe.enaa);
		}
#endif
	}
}

void nrf24l01_close_pipe(param_t pipe)
{
	pipe_reg_t rpipe;

	if(pipe > PIPE_MAX) {
		return;
	}

	memcpy_P(&rpipe, &pipe_reg[pipe], sizeof(pipe_reg_t));

	if(inr(EN_RXADDR) & rpipe.en_rxaddr) {
		outr(EN_RXADDR, inr(EN_RXADDR) & ~rpipe.en_rxaddr);
#if (RF_ARC != ARC_DISABLE)
		outr(EN_AA, inr(EN_AA) & ~rpipe.enaa);
#endif
	}
}

