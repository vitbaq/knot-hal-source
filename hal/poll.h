/*
 * Copyright (c) 2017, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

 #ifndef __HAL_POLL_H__
 #define __HAL_POLL_H__

#define POLLIN		0x0001
#define POLLOUT		0x0004
#define POLLHUP		0x0010

struct hal_pollfd {
	int fd;
	short events;
	short revents;
};

#endif /* __HAL_POLL_H__ */
