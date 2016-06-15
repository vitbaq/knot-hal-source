/*
 * This file is part of the KNOT Project
 *
 * Copyright (c) 2015, CESAR. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the CESAR nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL CESAR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdio.h>

#ifndef	_KNOT_HAL_DEBUG_H
#define	_KNOT_HAL_DEBUG_H

#if defined (TRACE_ALL) || defined (TRACE_ONLY)
#define TRACE(...)	do {	\
	fprintf(stdout,					\
        "%s: ",  						\
        __FUNCTION__ 		\
        );									\
	fprintf(stdout,  __VA_ARGS__);		\
} while(0)
#else
#define TRACE(...) (void)0
#endif

#if defined (TRACE_ALL) || defined (TRACE_ERROR)
#define TERROR(...)	do {	\
	fprintf(stderr,					\
        "%s::%s(%d): ",  		\
        __FILE__,					\
        __FUNCTION__, 	\
        __LINE__					\
        );									\
	fprintf(stderr, __VA_ARGS__);		\
} while(0)
#else
#define TERROR(...) (void)0
#endif

#ifdef TRACE_ALL
/**
 * \brief Print buffer in formated HEX values
 * */
static inline void dump_data(const char *str, int port, void *pdata, int len)
{
    unsigned char *pd = pdata;
    int i, off, col, n;
    char buff[96];

    for (off = n = 0; off < len; off += 16) {
    	if (port != -1) {
    		n = sprintf(buff, "%s[%d][%04d]", str, port, off);
    	} else {
    		n = sprintf(buff, "%s[%04d]", str, off);
    	}
        for (i = off, col = 16; col != 0 && i < len; --col, ++i)
            n += sprintf(buff + n, " %02lX", (ulong_t)pd[i]);
        // if (col != 0 && off != 0)
        for (; col != 0; --col)
            n += sprintf(buff + n, "   ");
        n += sprintf(buff + n, " - ");
        for (i = off, col = 16; col != 0 && i < len; --col, ++i)
            n += sprintf(buff + n, "%c", pd[i] >= ' ' && pd[i] < 0x7f ? pd[i] : '.');
        printf("%s\r\n", buff);
    }
}
#define DUMP_DATA	dump_data
#else
#define DUMP_DATA
#endif

#endif	 /* _KNOT_HAL_DEBUG_H */
