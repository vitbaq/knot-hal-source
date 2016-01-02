/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef ARDUINO
#include <avr/pgmspace.h>
#else
#define	PROGMEM
#define	memcpy_P	memcpy
#endif

#ifndef	__TYPES_H__
#define	__TYPES_H__

/* Macros */
#define _MIN(a,b)						((a)<(b)?(a):(b))
#define _MAX(a,b)						((a)>(b)?(a):(b))
#define _ABS(x)							((x)>0?(x):-(x))
#define _CONSTRAIN(x,l,h)		((x)<(l)?(l):((x)>(h)?(h):(x)))

// invalid socket fd
#define SOCKET_INVALID		-1

// operation status codes
#define SUCCESS		0
#define ERROR			-1

#ifdef __cplusplus
extern "C"{
#endif

/* Fast types definition for the platforms */
#ifdef ARDUINO
typedef uint_fast8_t		uint_t;
typedef int_fast8_t			int_t;
typedef int_t						param_t;
typedef int_t						result_t;
typedef void						*pparam_t;
typedef uint_fast16_t		len_t;
#define	LEN_T_MAX		 ((uint_fast16_t)-1)
#else
typedef uint_fast16_t		uint_t;
typedef int_fast16_t		int_t;
typedef int_t						param_t;
typedef int_t						result_t;
typedef void						*pparam_t;
typedef uint_t					len_t;
#define	LEN_T_MAX		((uint_fast16_t)-1)
#endif

/* Uncomment the line to message trace print via USART */
#define _TRACE_
#ifdef _TRACE_
#define	TRACE	printf
#else
#define	TRACE
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif  // __TYPES_H__
