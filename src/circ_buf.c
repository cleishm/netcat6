/*
 *  circ_buf.c - circular buffer module - implementation
 *  
 *  nc6 - an advanced netcat clone
 *  Copyright (C) 2001-2003 Mauro Tortonesi <mauro _at_ deepspace6.net>
 *  Copyright (C) 2002-2003 Chris Leishman <chris _at_ leishman.org>
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
#include "config.h"
#include "circ_buf.h"
#include "misc.h"
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>

RCSID("@(#) $Header: /Users/cleishma/work/nc6-repo/nc6/src/circ_buf.c,v 1.18 2003-01-11 14:05:47 simone Exp $");



#ifndef NDEBUG
static void cb_assert(const circ_buf *cb)
{
	if (cb == NULL ||
	    cb->buf == NULL ||
	    cb->ptr == NULL ||
	    cb->buf_size < cb->data_size) 
		fatal(_("internal error with circular buffers: please "
		        "contact the authors of nc6 for bugfixing ;-)"));
}
#else
#define cb_assert(CB)	do {} while(0)
#endif



void cb_init(circ_buf *cb, size_t size)
{
	assert(cb != NULL);
	assert(size > 0);
	
	memset(cb, 0, sizeof(circ_buf));
	
	cb->buf = (uint8_t *)xmalloc(size);
	cb->ptr = cb->buf;
	cb->data_size = 0;
	cb->buf_size  = size;

	cb_assert(cb);
}



void cb_destroy(circ_buf *cb)
{
	cb_assert(cb);

	free(cb->buf);
	cb->buf = NULL;
}



void cb_resize(circ_buf *cb, size_t size)
{
	uint8_t *new_buf;

	cb_assert(cb);
	assert(size > 0);

	/* create a new buffer and copy the existing data into it */
	new_buf = (uint8_t *)xmalloc(size);
	cb_extract(cb, new_buf, size);

	/* replace buffer */
	free(cb->buf);
	cb->buf = new_buf;

	/* adjust pointers and sizes */
	cb->ptr = cb->buf;
	cb->buf_size = size;
	if (cb->data_size > size)
		cb->data_size = size;
}



ssize_t cb_read(circ_buf *cb, int fd, size_t nbytes)
{
	ssize_t rr;
	int count;
	struct iovec iov[2];
	size_t len;

	cb_assert(cb);
	assert(fd >= 0);
	
	/* buffer is full, return an error condition */
	if (cb_is_full(cb)) return -1;

	/* set nbytes appropriately */
	if (nbytes == 0 || (size_t)nbytes > cb_space(cb))
		nbytes = cb_space(cb);
	
	/* prepare for writing to buffer */
	if (cb->ptr == cb->buf) {
		/* space only at end of buf */
		iov->iov_base = cb->ptr + cb->data_size;
		iov->iov_len  = nbytes;
		count = 1;
	} else if (cb->ptr + cb->data_size >= cb->buf + cb->buf_size) {
		/* space only before cb->ptr */
		iov->iov_base = cb->ptr + cb->data_size - cb->buf_size;
		iov->iov_len  = nbytes;
		count = 1;
	} else {
		/* space at end and begining of buf */
		iov[0].iov_base = cb->ptr + cb->data_size;
		len = (cb->buf_size - cb->data_size) - (cb->ptr - cb->buf);

		if (len >= nbytes) {
			/* first space provides enough */
			iov[0].iov_len = nbytes;
			count = 1;
		} else {
			/* need to use both free spaces */
			iov[0].iov_len = len;
			iov[1].iov_base = cb->buf;
			iov[1].iov_len = nbytes - len;
			count = 2;
		}
	}		

	/* do the actual read */
	do { 
		errno = 0;
		rr = readv(fd, iov, count);
	} while (errno == EINTR);

	/* if rr < 0 an error has occured, 
	 * if rr = 0 nothing needs to be changed.
	 * update internal stuff only if rr > 0 */
	if (rr > 0) {
		cb->data_size += rr;
		
		/* sanity check */
		cb_assert(cb);
	}

	return rr;
}



ssize_t cb_recv(circ_buf *cb, int fd, size_t nbytes,
                struct sockaddr *from, size_t *fromlen)
{
	ssize_t rr;
	int count;
	struct iovec iov[2];
	struct msghdr msg;
	size_t len;

	cb_assert(cb);
	assert(fd >= 0);

	/* buffer is full, return an error condition */
	if (cb_is_full(cb)) return -1;

	/* set nbytes appropriately */
	if (nbytes == 0 || nbytes > cb_space(cb))
		nbytes = cb_space(cb);
	
	/* prepare for writing to buffer */
	if (cb->ptr == cb->buf) {
		/* space only at end of buf */
		iov->iov_base = cb->ptr + cb->data_size;
		iov->iov_len  = nbytes;
		count = 1;
	} else if (cb->ptr + cb->data_size >= cb->buf + cb->buf_size) {
		/* space only before cb->ptr */
		iov->iov_base = cb->ptr + cb->data_size - cb->buf_size;
		iov->iov_len  = nbytes;
		count = 1;
	} else {
		/* space at end and begining of buf */
		iov[0].iov_base = cb->ptr + cb->data_size;
		len = (cb->buf_size - cb->data_size) - (cb->ptr - cb->buf);

		if (len >= nbytes) {
			/* first space provides enough */
			iov[0].iov_len = nbytes;
			count = 1;
		} else {
			/* need to use both free spaces */
			iov[0].iov_len = len;
			iov[1].iov_base = cb->buf;
			iov[1].iov_len = nbytes - len;
			count = 2;
		}
	}		

	/* setup msg structure */
	memset(&msg, 0, sizeof(msg));
	msg.msg_name    = (void *)from;
	msg.msg_namelen = (from != NULL && fromlen != 0)? *fromlen : 0;
	msg.msg_iov     = iov;
	msg.msg_iovlen  = count;

	/* do the actual recv */
	do {
		errno = 0;
		rr = recvmsg(fd, &msg, 0);
		
		/* copy out updated namelen */
		if (from != NULL && fromlen != 0) 
			*fromlen = msg.msg_namelen;
	} while (errno == EINTR);

	/* if rr < 0 an error has occured,
	 * if rr = 0 nothing needs to be changed.
	 * update internal stuff only if rr > 0 */
	if (rr > 0) {
		cb->data_size += rr;
		
		/* sanity check */
		cb_assert(cb);
	}

	return rr;
}



ssize_t cb_append(circ_buf *cb, const uint8_t *buf, size_t len)
{
	ssize_t rr;
	int i, count;
	struct iovec iov[2];
	const uint8_t *tmp;

	cb_assert(cb);
	assert(buf != NULL);
	
	/* buffer is full, return an error condition */
	if (cb_is_full(cb)) return -1;
	
	/* return if len is zero */
	if (len == 0) return 0;
	
	/* setup initial values for tmp and rr */
	tmp = (const uint8_t *)buf;
	rr  = 0;
	
	/* prepare for writing to buffer */
	if (cb->ptr == cb->buf) {
		/* space only at end of buf */
		iov->iov_base = cb->ptr + cb->data_size;
		iov->iov_len  = cb->buf_size - cb->data_size;
		count = 1;
	} else if (cb->ptr + cb->data_size >= cb->buf + cb->buf_size) {
		/* space only before cb->ptr */
		iov->iov_base = cb->ptr + cb->data_size - cb->buf_size;
		iov->iov_len  = cb->buf_size - cb->data_size;
		count = 1;
	} else {
		/* space at end and begining of buf */
		iov[0].iov_base = cb->ptr + cb->data_size;
		iov[0].iov_len  = (cb->buf_size - cb->data_size) 
		                - (cb->ptr - cb->buf);
		iov[1].iov_base = cb->buf;
		iov[1].iov_len  = cb->ptr - cb->buf;
		count = 2;
	}		

	/* do the actual copy */
	for (i = 0; i < count; ++i) {
		size_t chunk_size;
		
		chunk_size = MIN(iov[i].iov_len, len);
		assert(chunk_size > 0);
		
		memcpy((void *)iov[i].iov_base, (const void *)tmp, chunk_size);

		tmp = tmp + chunk_size;
		len -= chunk_size;
		cb->data_size += chunk_size;
		rr += chunk_size;

		/* sanity check */
		cb_assert(cb);
		
		if (len == 0) break;
	}

	return rr;
}



ssize_t cb_write(circ_buf *cb, int fd, size_t nbytes)
{
	ssize_t rr;
	int count;
	struct iovec iov[2];
	size_t len;
	
	cb_assert(cb);
	assert(fd >= 0);
	
	/* buffer is empty, return immediately */
	if (cb_is_empty(cb)) return 0;
	
	/* set nbytes appropriately */
	if (nbytes == 0 || nbytes > cb_used(cb))
		nbytes = cb_used(cb);

	/* prepare for reading from buffer */
	if (cb->ptr + cb->data_size > cb->buf + cb->buf_size) {
		/* data after ptr and at beginning of buffer */
		iov[0].iov_base = cb->ptr;
		len = cb->buf_size - (cb->ptr - cb->buf);

		if (len >= nbytes) {
			/* data after ptr is enough */
			iov[0].iov_len = nbytes;
			count = 1;
		} else {
			iov[0].iov_len  = len;
			iov[1].iov_base = cb->buf;
			iov[1].iov_len  = nbytes - len;
			count = 2;
		}
	} else {
		/* data only after ptr */
		iov[0].iov_base = cb->ptr;
		iov[0].iov_len  = nbytes;
		count = 1;
	}		

	/* do the actual write */
	do { 
		errno = 0;
		rr = writev(fd, iov, count);
	} while (errno == EINTR);

	/* if rr < 0 an error has occured, 
	 * if rr = 0 nothing needs to be changed.
	 * update internal stuff only if rr > 0 */
	if (rr > 0) {
		assert((size_t)rr <= cb->data_size);
		cb->data_size -= rr;
		
		/* update value of cb->ptr */
		cb->ptr += rr;
		if (cb->ptr >= cb->buf + cb->buf_size) 
			cb->ptr -= cb->buf_size;
		
		/* sanity check */
		cb_assert(cb);
	}

	return rr;
}



ssize_t cb_send(circ_buf *cb, int fd, size_t nbytes,
                struct sockaddr *dest, size_t destlen)
{
	ssize_t rr;
	int count;
	struct iovec iov[2];
	struct msghdr msg;
	size_t len;
	
	cb_assert(cb);
	assert(fd >= 0);
	
	/* buffer is empty, return immediately */
	if (cb_is_empty(cb)) return 0;
	
	/* set nbytes appropriately */
	if (nbytes == 0 || nbytes > cb_used(cb))
		nbytes = cb_used(cb);
	
	/* prepare for reading from buffer */
	if (cb->ptr + cb->data_size > cb->buf + cb->buf_size) {
		/* data after ptr and at beginning of buffer */
		iov[0].iov_base = cb->ptr;
		len = cb->buf_size - (cb->ptr - cb->buf);

		if (len >= nbytes) {
			/* data after ptr is enough */
			iov[0].iov_len = nbytes;
			count = 1;
		} else {
			iov[0].iov_len  = len;
			iov[1].iov_base = cb->buf;
			iov[1].iov_len  = nbytes - len;
			count = 2;
		}
	} else {
		/* data only after ptr */
		iov[0].iov_base = cb->ptr;
		iov[0].iov_len  = nbytes;
		count = 1;
	}		
	
	/* setup msg structure */
	memset(&msg, 0, sizeof(msg));
	msg.msg_name    = (void *)dest;
	msg.msg_namelen = destlen;
	msg.msg_iov     = iov;
	msg.msg_iovlen  = count;

	/* do the actual send */
	do { 
		errno = 0;
		rr = sendmsg(fd, &msg, 0);
	} while (errno == EINTR);

	/* if rr < 0 an error has occured, 
	 * if rr = 0 nothing needs to be changed.
	 * update internal stuff only if rr > 0 */
	if (rr > 0) {
		assert((size_t)rr <= cb->data_size);
		cb->data_size -= rr;
		
		/* update value of cb->ptr */
		cb->ptr += rr;
		if (cb->ptr >= cb->buf + cb->buf_size) 
			cb->ptr -= cb->buf_size;
		
		/* sanity check */
		cb_assert(cb);
	}

	return rr;
}



ssize_t cb_extract(circ_buf *cb, uint8_t *buf, size_t len)
{
	ssize_t rr;
	int i, count;
	struct iovec iov[2];

	cb_assert(cb);
	assert(buf != NULL);
	
	/* buffer is empty, return immediately */
	if (cb_is_empty(cb)) return 0;

	/* return if len is zero */
	if (len == 0) return 0;

	/* setup initial value for rr */
	rr = 0;
	
	/* prepare for reading from buffer */
	if (cb->ptr + cb->data_size > cb->buf + cb->buf_size) {
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

	/* do the actual copy */
	for (i = 0; i < count; ++i) {
		size_t chunk_size;

		chunk_size = MIN(iov[i].iov_len, len);
		assert(chunk_size > 0);

		memcpy((void *)buf, (const void *)iov[i].iov_base, chunk_size);

		buf = buf + chunk_size;
		len -= chunk_size;
		rr += chunk_size;

		/* sanity check */
		cb_assert(cb);

		if (len == 0) break;
	}

	/* if rr = 0 nothing needs to be changed,
	 * update internal stuff only if rr > 0 */
	if (rr > 0) {
		assert((size_t)rr <= cb->data_size);
		cb->data_size -= rr;

		/* update value of cb->ptr */
		cb->ptr += rr;
		if (cb->ptr >= cb->buf + cb->buf_size)
			cb->ptr -= cb->buf_size;

		/* sanity check */
		cb_assert(cb);
	}

	return rr;
}



void cb_clear(circ_buf *cb)
{
	cb_assert(cb);
	
	cb->ptr = cb->buf;
	cb->data_size = 0;
}
