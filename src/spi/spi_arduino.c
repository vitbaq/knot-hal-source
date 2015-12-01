/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include "spi.h"

#ifdef	ARDUINO

#include <avr/io.h>
#include <util/delay.h>

#define CSN		2
#define MOSI	3
#define MISO	4
#define SCK		5

#define DELAY_US	5	// CSN delay in microseconds

static bool	m_init = false;

void spi_init(void)
{
	if(!m_init) {
		m_init = true;

		//Put CSN HIGH
		PORTB |= (1 << CSN);

		//CSN as output
		DDRB |= (1 << CSN);

		//Enable SPI and set as master
		SPCR |= (1 << SPE) | (1 << MSTR);

		//MISO as input, MOSI and SCK as output
		DDRB &= ~(1 << MISO);
		DDRB |= (1 << MOSI);
		DDRB |= (1 << SCK);

		//SPI mode 0
		SPCR &= ~(1 << CPOL);
		SPCR &= ~(1 << CPHA);

		//vel fosc/16 = 1MHz
		SPCR &= ~(1 << SPI2X);
		SPCR &= ~(1 << SPR1);
		SPCR |= (1 << SPR0);
	}
}

void spi_deinit(void)
{
	if(m_init) {
		m_init = false;

		PORTB |= (1 << CSN);

		//Disable SPI and reset master
		SPCR &= ~((1 << SPE) | (1 << MSTR));
	}
}

bool spi_transfer(void *ptx, len_t ltx, void *prx, len_t lrx)
{
	uint8_t* pd;

	if(!m_init) {
		return false;
	}

	PORTB &= ~(1 << CSN);
	_delay_us(DELAY_US);

	if(ptx != NULL && ltx != 0) {
		for(pd=(uint8_t*)ptx; ltx != 0; --ltx, ++pd) {
			SPDR = *pd;
			/*
			 * The following NOP introduces a small delay that can prevent the wait
			 * loop form iterating when running at the maximum speed. This gives
			 * about 10% more speed, even if it seems counter-intuitive. At lower
			 * speeds it is unnoticed.
			 */
			asm volatile("nop");
			while(!(SPSR & (1<<SPIF)));
			SPDR;
		}
	}

	if(prx != NULL && lrx != 0) {
		for(pd=(uint8_t*)prx; lrx != 0; --lrx, ++pd) {
			SPDR = *pd;
			/*
			 * The following NOP introduces a small delay that can prevent the wait
			 * loop form iterating when running at the maximum speed. This gives
			 * about 10% more speed, even if it seems counter-intuitive. At lower
			 * speeds it is unnoticed.
			 */
			asm volatile("nop");
			while(!(SPSR & (1<<SPIF)));
			*pd = SPDR;
		}
	}

	PORTB |= (1 << CSN);

	return true;
}

#endif // ARDUINO
