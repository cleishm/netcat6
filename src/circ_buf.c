/*
 *  circ_buf.c - circular buffer module - implementation
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
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include "circ_buf.h"
#include "misc.h"


bool is_empty(const circ_buf *cb)
{
	assert(cb != NULL);
	assert(cb->data_size >= 0);
	assert(cb->data_size <= cb->buf_size);

	return (cb->data_size == 0 ? TRUE : FALSE);
}



bool is_full(const circ_buf *cb)
{
	assert(cb != NULL);
	assert(cb->data_size >= 0);
	assert(cb->data_size <= cb->buf_size);

	return (cb->data_size == cb->buf_size ? TRUE : FALSE);
}


#ifndef NDEBUG
void check_cb(const circ_buf *cb)
{
	if (cb == NULL ||
	    cb->buf == NULL ||
	    cb->ptr == NULL ||
	    cb->buf_size  < 0 ||
	    cb->data_size < 0 ||
	    cb->buf_size < cb->data_size) 
		fatal("internal error: contact the author "
		      "of this software for bugfixing ;-)");
}
#endif



int read_to_cb(int fd, circ_buf *cb)
{
	int rr, count;
	struct iovec iov[2];

	check_cb(cb);
	
	/* buffer is full, return an error condition */
	if (cb->data_size == cb->buf_size) return -1;
	
	/* prepare for writing to buffer */
	if (cb->ptr + cb->data_size >= cb->buf + cb->buf_size) {
		iov->iov_base = cb->ptr + (cb->data_size - cb->buf_size);
		iov->iov_len  = cb->buf_size - cb->data_size;
		count = 1;
	} else {
		iov[0].iov_base = cb->ptr + cb->data_size;
		iov[0].iov_len  = cb->buf_size - (cb->ptr - cb->buf) 
			        - cb->data_size;
		iov[1].iov_base = cb->buf;
		iov[1].iov_len  = cb->ptr - cb->buf;
		count = 2;
	}		

	/* do the actual read */
	do { 
		rr = readv(fd, iov, count);
	} while(errno == EINTR);

	/* if rr < 0 an error has occured, 
	 * if rr = 0 nothing needs to be changed.
	 * update internal stuff only if rr > 0 */
	if (rr > 0) {
		cb->data_size += rr;
		check_cb(cb);
	}

	return rr;
}



int udp_read_to_cb(int fd, circ_buf *cb)
{
	int rr, count;
	struct iovec iov[2];

	check_cb(cb);
	
	/* buffer is full, return an error condition */
	if (cb->data_size == cb->buf_size) return -1;
	
	/* prepare for writing to buffer */
	if (cb->ptr + cb->data_size >= cb->buf + cb->buf_size) {
		iov->iov_base = cb->ptr + (cb->data_size - cb->buf_size);
		iov->iov_len  = cb->buf_size - cb->data_size;
		count = 1;
	} else {
		iov[0].iov_base = cb->ptr + cb->data_size;
		iov[0].iov_len  = cb->buf_size - (cb->ptr - cb->buf) 
			        - cb->data_size;
		iov[1].iov_base = cb->buf;
		iov[1].iov_len  = cb->ptr - cb->buf;
		count = 2;
	}		

	/* do the actual read */
	do { 
		rr = readv(fd, iov, count);
	} while(errno == EINTR);

	/* if rr < 0 an error has occured, 
	 * if rr = 0 nothing needs to be changed.
	 * update internal stuff only if rr > 0 */
	if (rr > 0) {
		cb->data_size += rr;
		check_cb(cb);
	}

	return rr;
}



int write_from_cb(int fd, circ_buf *cb)
{
	int rr, count;
	struct iovec iov[2];
	
	check_cb(cb);
	
	/* buffer is empty, return immediately */
	if (cb->data_size == 0) return 0;
	
	/* prepare for reading from buffer */
	if (cb->ptr + cb->data_size >= cb->buf + cb->buf_size) {
		iov[0].iov_base = cb->ptr;
		iov[0].iov_len  = cb->buf_size - (cb->ptr - cb->buf);
		iov[1].iov_base = cb->buf;
		iov[1].iov_len  = cb->data_size - iov[0].iov_len;
		count = 2;
	} else {
		iov[0].iov_base = cb->ptr;
		iov[0].iov_len  = cb->data_size;
		count = 1;
	}		

	/* do the actual write */
	do { 
		rr = writev(fd, iov, count);
	} while(errno == EINTR);

	/* if rr < 0 an error has occured, 
	 * if rr = 0 nothing needs to be changed.
	 * update internal stuff only if rr > 0 */
	if (rr > 0) {
		cb->data_size -= rr;
		
		/* update value of cb->ptr */
		cb->ptr += rr;
		if (cb->ptr >= cb->buf + cb->buf_size) 
			cb->ptr -= cb->buf_size;
		
		check_cb(cb);
	}

	return rr;
}



circ_buf *alloc_cb(size_t size)
{
	circ_buf *cb;

	cb = (circ_buf *)xmalloc(sizeof(circ_buf));
	memset(cb, 0, sizeof(circ_buf));
	
	/* normalization: size must be a multiple of 16 */
	if (size & 0xF) size = (size & ~0xF) + 0x10;
	
	cb->buf = (uint8_t *)xmalloc(size);
	cb->ptr = cb->buf;
	cb->data_size = 0;
	cb->buf_size  = size;

	return cb;
}



void free_cb(circ_buf **cb)
{
	circ_buf *ptr = *cb;
		
	assert(cb != NULL);
	assert(ptr != NULL);
	assert(ptr->data_size >= 0);
	assert(ptr->data_size <= ptr->buf_size);
	assert(ptr->buf != NULL);

	free(ptr->buf);
	free(ptr);
	
	*cb = NULL;
}
