/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#ifndef __SLEEP_WDT_H__
#define __SLEEP_WDT_H__

#ifdef ARDUINO

#ifdef __cplusplus
extern "C"{
#endif

void sleep_wdt_initilize(uint8_t interval);
void sleep_wdt_end();
uint8_t sleep_wdt_wakeup(uint8_t seconds);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // ARDUINO

#endif // __SLEEP_WDT_H__

