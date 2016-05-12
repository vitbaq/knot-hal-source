/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#ifndef __UTIL_H__
#define __UTIL_H__

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#endif

typedef unsigned long	ulong_t;
#define ULONG_T_MAX	((ulong_t)-1)

#ifdef __cplusplus
extern "C"{
#endif

#define print_byte_array(s, p, c, len) printDumpData(s, p, c, len)

#ifndef ARDUINO
/**
 * \brief Gets current time in time base.
 * \param   time  Reference variable to the time.
 * \param   time_base  Base time converter value (default in microseconds).
 * \return  0 if success; otherwise -1.
 */
int tline(ulong_t *ptime, ulong_t time_base /*=1*/);
/**
 * \brief Gets current time in MILLISECONDS.
 * \return  timeline value.
 */
ulong_t tline_ms(void);
/**
 * \brief Gets current time in MICROSECONDS.
 * \return  timeline value.
 */
ulong_t tline_us(void);
/**
 * \brief Gets current time in SECONDS.
 * \return  timeline value.
 */
ulong_t tline_sec(void);
#else
/**
 * \brief Gets current time in MILLISECONDS.
 * \return  timeline value.
 */
inline ulong_t tline_ms(void) { return millis(); }

/**
 * \brief Gets current time in MICROSECONDS.
 * \return  timeline value.
 */
inline ulong_t tline_us(void) { return micros(); }
/**
 * \brief Gets current time in SECONDS.
 * \return  timeline value.
 */
inline ulong_t tline_sec(void) { return (millis() / 1000UL); }
#endif

/**
 * \brief Check if timeline has expired.
 * \param   time  Current time reference.
 * \param   last  Last time reference.
 * \param   timeout Elapsed time.
 * \return  true for timeout expired; otherwise, false.
 */
int tline_out(ulong_t time,  ulong_t last,  ulong_t timeout);

/**
 * \brief Print buffer in formated HEX values
 * */
int dump_data(const char *str, int port, void *pdata, int len);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __UTIL_H__ */
