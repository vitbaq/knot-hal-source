/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include "util.h"

#ifndef ARDUINO

int tline(ulong_t *ptime, ulong_t time_base /*=1*/)
{
	struct timeval tv;
    ulong_t time;
    /* Obtain the current time, expressed as seconds and microseconds */
    if (gettimeofday(&tv, NULL) != 0)
        return -1;

    /* Computes to microseconds */
    time = tv.tv_sec * 1000000UL;
    time += tv.tv_usec;
    /* Converts microseconds to time base */
    time /= time_base;
    *ptime = time;

    return 0;
}

ulong_t tline_ms(void)
{
	ulong_t time;

    while (tline(&time, 1000UL) == -1)
        usleep(1);

    return time;
}

ulong_t tline_us(void)
{
	ulong_t time;

    while (tline(&time, 1UL) == -1)
        usleep(1);

    return time;
}

ulong_t tline_sec(void)
{
    ulong_t time;

    while (tline(&time, 1000000UL) == -1)
        usleep(1);

    return time;
}

#endif

int tline_out(ulong_t time,  ulong_t last,  ulong_t timeout)
{
    /* Time overflow */
    if (time < last)
        /* Fit time overflow and compute time elapsed */
        time += (ULONG_T_MAX - last);
    else
        /* Compute time elapsed */
        time -= last;

    /* Timeout is flagged */
    return (time >= timeout);
}
