/*
 *  circ_buf.h - circular buffer module - header
 * 
 *  nc6 - an advanced netcat clone
 *  Copyright (C) 2001-2004 Mauro Tortonesi <mauro _at_ deepspace6.net>
 *  Copyright (C) 2002-2004 Chris Leishman <chris _at_ leishman.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */  
#ifndef CIRC_BUF_H
#define CIRC_BUF_H

#include "config.h"
#include "misc.h"
#include <sys/types.h>
#include <sys/socket.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

typedef struct circ_buf_t {
	uint8_t *buf;      /* pointer to the buffer */
	uint8_t *ptr;      /* pointer to the beginning of written data */
	size_t data_size;  /* number of bytes that have been written 
	                    * into the buffer */
	size_t buf_size;   /* size of the buffer */
} circ_buf;


void cb_init(circ_buf *cb, size_t size);
void cb_destroy(circ_buf *cb);

void cb_resize(circ_buf *cb, size_t size);

#define cb_size(CB)	((CB)->buf_size)
#define cb_used(CB)	((CB)->data_size)
#define cb_space(CB)	((CB)->buf_size - (CB)->data_size)

#define cb_is_empty(CB)	(cb_used(CB) == 0)
#define cb_is_full(CB)	(cb_space(CB) == 0)

ssize_t cb_read(circ_buf *cb, int fd, size_t nbytes);
ssize_t cb_recv(circ_buf *cb, int fd, size_t nbytes,
                struct sockaddr *from, size_t *fromlen);

ssize_t cb_write(circ_buf *cb, int fd, size_t nbytes);
ssize_t cb_send(circ_buf *cb, int fd, size_t nbytes,
                struct sockaddr *dest, size_t destlen);

ssize_t cb_append(circ_buf *cb, const uint8_t *buf, size_t len);
ssize_t cb_extract(circ_buf *cb, uint8_t *buf, size_t len);

void cb_clear(circ_buf *cb);

#endif /* CIRC_BUF_H */
