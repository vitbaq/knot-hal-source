/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

#include "sleep_wdt.h"

// INT0 generates an interrupt request
#define LOW_LEVEL     0 //The low level of INT0 generates an interrupt request.
#define CHANGE_LEVEL  1 //Any logical change on INT0 generates an interrupt request.
#define FALLING_EDGE  2 //The falling edge of INT0 generates an interrupt request.
#define RISING_EDGE   3 //The rising edge of INT0 generates an interrupt request.

static volatile uint8_t m_time_cycles = 0;
static volatile uint8_t m_wake = 0;

ISR(WDT_vect){
  --m_time_cycles;
}

static void radioactive()
{
  ++m_wake;
}

void sleep_wdt_initilize(uint8_t interval)
{
  uint8_t prescale = interval & 0b00000111;
  if(interval & 0b00001000)
    prescale |= _BV(WDP3);
    
  cli();            // make sure we don't get interrupted before we sleep
  MCUSR &= ~_BV(WDRF);            // clear the WDRF bit
  WDTCSR |= _BV(WDCE) | _BV(WDE); // clear the WDE bit
  WDTCSR = _BV(WDIE) | prescale;  // set new prescalar (timeout)
  wdt_reset();
  attachInterrupt(INT0, radioactive, FALLING_EDGE);  // wake up on falling level
  sei();            // interrupts allowed now, next instruction WILL be executed
}

void sleep_wdt_end()
{
  cli();            // make sure we don't get interrupted before we sleep
  set_sleep_mode(SLEEP_MODE_IDLE);   
  detachInterrupt(INT0);  // stop FALLING interrupt
  wdt_reset();
  MCUSR &= ~_BV(WDRF);
  WDTCSR |= _BV(WDCE) | _BV(WDE); // clear the WDE bit
  WDTCSR = 0;      // turn off WDT
  sei();           // interrupts allowed now, next instruction WILL be executed

  m_time_cycles = 0;
  m_wake = 0;
}

uint8_t sleep_wdt_wakeup(uint8_t seconds)
{
  uint8_t ret;
  
  m_time_cycles = seconds;
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  wdt_reset();
  while(m_time_cycles != 0 && m_wake == 0)
  {
    sleep_cpu();      // here the device is put to sleep
  }
  cli();            // make sure we don't get interrupted before we sleep
  ret = m_wake;
  m_wake = 0;
  sei();            // interrupts allowed now, next instruction WILL be executed
  sleep_disable();
  return ret;
}

