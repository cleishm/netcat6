/*
 *  io_stream.c - stream i/o wrapper - implementation 
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
#include "io_stream.h"
#include "misc.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

RCSID("@(#) $Header: /Users/cleishma/work/nc6-repo/nc6/src/io_stream.c,v 1.11 2003-01-01 10:05:32 chris Exp $");



void io_stream_init(io_stream *ios, circ_buf *inbuf, circ_buf *outbuf)
{
	assert(ios != NULL);
	assert(inbuf != NULL);
	assert(outbuf != NULL);

	ios->fd_in = -1;
	ios->fd_out = -1;
	ios->socktype = 0;  /* unknown */

	ios->buf_in = inbuf;
	ios->buf_out = outbuf;

	ios->mtu = 0; /* unlimited */
	ios->nru = 0; /* unlimited */

	ios->half_close_suppress = FALSE;

	ios->hold_time = -1; /* infinite */
	timerclear(&(ios->read_closed));

	ios->rcvd = 0;
	ios->sent = 0;
}



void io_stream_destroy(io_stream *ios)
{
	assert(ios != NULL);
	ios_shutdown(ios, SHUT_RDWR);
}



void ios_assign_socket(io_stream *ios, int fd, int socktype)
{
	assert(ios != NULL);
	assert(fd >= 0);

	/* nonblock(fd); */
	ios->fd_in  = fd;
	ios->fd_out = fd;
	ios->socktype = socktype;
}



void ios_assign_stdio(io_stream *ios)
{
	assert(ios != NULL);

	if ((ios->fd_in  = dup(STDIN_FILENO)) < 0) 
		fatal("error in duplicating stdin file descriptor: %s", 
		      strerror(errno));
	
	if ((ios->fd_out = dup(STDOUT_FILENO)) < 0) 
		fatal("error in duplicating stdout file descriptor: %s", 
		      strerror(errno));

	/* nonblock(ios->fd_in); */
	/* nonblock(ios->fd_out); */

	/* pretend stdio is a stream socket */
	ios->socktype = SOCK_STREAM;
}



int ios_schedule_read(io_stream *ios)
{
	size_t space = cb_space(ios->buf_in);

	/* if closed, the buffer is full or there isn't enough free space in
	 * the buffer to satisfy the nru, then we can't read */
	if ((ios->fd_in < 0) || space == 0 || space < ios->nru)
		return -1;
	/* schedule a read from fdin */
	return ios->fd_in;
}



int ios_schedule_write(io_stream *ios)
{
	/* if closed or there is no data in the buffer, then we can't write */
	if ((ios->fd_out < 0) || cb_is_empty(ios->buf_out))
		return -1;
	/* schedule a write to fdout */
	return ios->fd_out;
}



struct timeval* ios_next_timeout(io_stream *ios, struct timeval *tv)
{
	struct timeval now;

	assert(ios != NULL);
	assert(tv != NULL);

	/* no timeout if read is still open or hold time is infinite */
	if (is_read_open(ios) || ios->hold_time < 0)
		return NULL;

	if (ios->hold_time == 0) {
		/* instant timeout */
		timerclear(tv);
		return tv;
	}

	/* calculate the offset from now until the hold_time expiry */
	gettimeofday(&now, NULL);
	now.tv_sec -= ios->hold_time;
	timersub(&(ios->read_closed), &now, tv);

	if (tv->tv_sec < 0) {
		/* timeout has expired */
		timerclear(tv);
	}

	return tv;
}



ssize_t ios_read(io_stream *ios)
{
	ssize_t rr;

	/* should only be called if ios_schedule_read returned a true result */
	assert(ios->fd_in >= 0);
	assert(cb_space(ios->buf_in) >= ios->nru);

	/* read as much as possible */
	if (ios->socktype == SOCK_DGRAM)
		rr = cb_recv(ios->buf_in, ios->fd_in, 0, NULL, 0);
	else
		rr = cb_read(ios->buf_in, ios->fd_in, 0);

	if (rr > 0)
		ios->rcvd += rr;

	return rr;
}



ssize_t ios_write(io_stream *ios)
{
	ssize_t rr;

	/* should only be called if ios_schedule_write returned a true result */
	assert(ios->fd_out >= 0);
	assert(!cb_is_empty(ios->buf_out));

	/* write as much as the mtu allows */
	if (ios->socktype == SOCK_DGRAM)
		rr = cb_send(ios->buf_out, ios->fd_out, ios->mtu, NULL, 0);
	else
		rr = cb_write(ios->buf_out, ios->fd_out, ios->mtu);

	if (rr > 0)
		ios->sent += rr;

	return rr;
}



void ios_shutdown(io_stream* ios, int how)
{
	assert(ios != NULL);

	if (how == SHUT_RDWR) {
		/* close both the input and the output */
		if (ios->fd_in >= 0) {
			close(ios->fd_in);
			/* record the read shutdown time */
			gettimeofday(&(ios->read_closed), NULL);
		}
		/* if the same fd is input and output, don't close twice */
		if (ios->fd_out >= 0 && ios->fd_out != ios->fd_in)
			close(ios->fd_out);
		ios->fd_in = ios->fd_out = -1;
	} else if (how == SHUT_RD) {
		/* close the input */
		if (ios->fd_in >= 0) {
			/* if the fd is duplex, use shutdown */
			if (ios->fd_in == ios->fd_out) {
				if (!ios->half_close_suppress)
					shutdown(ios->fd_in, SHUT_RD);
			} else {
				close(ios->fd_in);
			}
			ios->fd_in = -1;
			/* record the read shutdown time */
			gettimeofday(&(ios->read_closed), NULL);
		}
	} else {
		assert(how == SHUT_WR);		
		/* close the output */
		if (ios->fd_out >= 0) {
			/* if the fd is duplex, use shutdown */
			if (ios->fd_in == ios->fd_out) {
				if (!ios->half_close_suppress)
					shutdown(ios->fd_out, SHUT_WR);
			} else {
				close(ios->fd_out);
			}
			ios->fd_out = -1;
		}
	}
}
