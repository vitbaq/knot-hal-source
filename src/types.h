/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#ifdef ARDUINO
#include <avr/pgmspace.h>
#else
#define	PROGMEM
#define	memcpy_P	memcpy
#endif

#ifndef	__TYPES_H__
#define	__TYPES_H__

/* Fast types definition for the platforms */
#ifdef ARDUINO
typedef uint_fast8_t		len_t;
typedef uint_fast8_t		param_t;
typedef void						*pparam_t;
typedef uint_fast8_t		result_t;
#define	LEN_T_MAX		(UINT_FAST8_MAX)
#else
typedef size_t					len_t;
typedef unsigned int		param_t;
typedef void						*pparam_t;
typedef unsigned int		result_t;
#define	LEN_T_MAX	((size_t)-1)
#endif

#define _MIN(a,b)						((a)<(b)?(a):(b))
#define _MAX(a,b)						((a)>(b)?(a):(b))
#define _ABS(x)							((x)>0?(x):-(x))
#define _CONSTRAIN(x,l,h)		((x)<(l)?(l):((x)>(h)?(h):(x)))

/* Uncomment the line to message trace print via USART */
#define _TRACE_
#ifdef _TRACE_
#define	TRACE	printf
#else
#define	TRACE
#endif

#endif  // __TYPES_H__
