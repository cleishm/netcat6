/*
 *  circ_buf.h - circular buffer module - header
 * 
 *  nc6 - an advanced netcat clone
 *  Copyright (C) 2001-2002 Mauro Tortonesi <mauro _at_ ferrara.linux.it>
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
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include "misc.h"

typedef struct circ_buf_t {
	uint8_t *buf;  /* pointer to the buffer */
	uint8_t *ptr;  /* pointer to the beginning of written data */
	int data_size; /* number of bytes that have been written 
			* into the buffer */
	int buf_size;  /* size of the buffer */
} circ_buf;

bool is_empty(const circ_buf *cb);
bool is_full(const circ_buf *cb);
int read_to_cb(int fd, circ_buf *cb);
int copy_to_cb(const uint8_t *buf, size_t len, circ_buf *cb);
int write_from_cb(int fd, circ_buf *cb);
int send_from_cb(int fd, circ_buf *cb, const struct sockaddr *dest, size_t destlen);
circ_buf *alloc_cb(size_t size);
void free_cb(circ_buf **cb);

#ifdef NDEBUG
#define check_cb(_x_)	do { } while(0)
#else
void check_cb(const circ_buf *cb);
#endif


#endif /* CIRC_BUF_H */
