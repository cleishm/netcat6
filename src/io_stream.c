/*
 *  io_stream.c - stream i/o wrapper - implementation 
 * 
 *  nc6 - an advanced netcat clone
 *  Copyright (C) 2002-2006 Chris Leishman <chris _at_ leishman.org>
 *  Copyright (C) 2001-2006 Mauro Tortonesi <mauro _at_ deepspace6.net>
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
#include "system.h"
#include "io_stream.h"
#include "misc.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

RCSID("@(#) $Header: /Users/cleishma/work/nc6-repo/nc6/src/io_stream.c,v 1.33 2009-04-18 11:39:35 chris Exp $");



#ifndef NDEBUG
static void ios_assert(const io_stream_t *ios)
{
	if (ios == NULL ||
	    ios->name == NULL ||
	    ios->buf_in == NULL ||
	    ios->buf_out == NULL)
	{
		fatal_internal("I/O stream assertion failed");
	}
}
#else
#define ios_assert(IOS) do {} while(0)
#endif



void ios_init_socket(io_stream_t *ios, const char *name, int fd, int socktype,
		circ_buf_t *inbuf, circ_buf_t *outbuf)
{
	/* check arguments */
	assert(ios != NULL);
	assert(name != NULL);
	assert(fd >= 0);
	assert(inbuf  != NULL);
	assert(outbuf != NULL);

	ios_init(ios, name, fd, fd, socktype, inbuf, outbuf);
}



void ios_init_stdio(io_stream_t *ios, const char *name,
		circ_buf_t *inbuf, circ_buf_t *outbuf)
{
	int fd_in, fd_out;

	/* check arguments */
	assert(ios != NULL);
	assert(name != NULL);
	assert(inbuf  != NULL);
	assert(outbuf != NULL);
	
	if ((fd_in  = dup(STDIN_FILENO)) < 0) 
		fatal("error duplicating stdin file descriptor: %s",
		      strerror(errno));
	
	if ((fd_out = dup(STDOUT_FILENO)) < 0) 
		fatal("error duplicating stdout file descriptor: %s",
		      strerror(errno));

	/* pretend stdio is a stream socket */
	ios_init(ios, name, fd_in, fd_out, SOCK_STREAM, inbuf, outbuf);
}



void ios_init(io_stream_t *ios, const char *name,
              int fd_in, int fd_out, int socktype,
              circ_buf_t *inbuf, circ_buf_t *outbuf)
{
	/* check arguments */
	assert(ios    != NULL);
	assert(name   != NULL);
	assert(fd_in  >= 0);
	assert(fd_out >= 0);
	assert(inbuf  != NULL);
	assert(outbuf != NULL);

	ios->fd_in  = fd_in;
	ios->fd_out = fd_out;
	ios->socktype = socktype;

	ios->flags = IOS_OK;

	ios->buf_in  = inbuf;
	ios->buf_out = outbuf;

	ios->mtu = 0; /* unlimited */
	ios->nru = 1; /* at least 1 byte space before reading */

	ios->half_close_suppress = false;

	ios->idle_timeout = -1;  /* infinite  */
	gettimeofday(&(ios->last_active), NULL);

	ios->hold_time = -1;     /* infinite */
	timerclear(&(ios->read_eof));

	ios->name = xstrdup(name);
	ios->rcvd = 0;
	ios->sent = 0;
}



void io_stream_destroy(io_stream_t *ios)
{
	/* check argument */
	ios_assert(ios);
	
	ios_shutdown(ios, SHUT_RDWR);
	free(ios->name);
}



int ios_schedule_read(io_stream_t *ios)
{
	size_t space;

	/* check argument */
	ios_assert(ios);
	
	space = cb_space(ios->buf_in);
	
	/* if closed, the buffer is full or there isn't enough free space in
	 * the buffer to satisfy the nru, then we can't read */
	if ((ios->fd_in < 0) || space == 0 || space < ios->nru)
		return -1;
	
	/* schedule a read from fdin */
	return ios->fd_in;
}



int ios_schedule_write(io_stream_t *ios)
{
	/* check argument */
	ios_assert(ios);
	
	/* if closed or there is no data in the buffer, then we can't write */
	if ((ios->fd_out < 0) || cb_is_empty(ios->buf_out))
		return -1;
	
	/* schedule a write to fdout */
	return ios->fd_out;
}



struct timeval *ios_next_timeout(io_stream_t *ios, struct timeval *tv)
{
	struct timeval now;
	struct timeval *tvp = NULL;

	/* check arguments */
	ios_assert(ios);
	assert(tv != NULL);

	/* no idle timeout if idle_timeout is infinite */
	if (ios->idle_timeout > 0) {
		/* check if the idle timeout has been triggered */
		gettimeofday(&now, NULL);

		/* calculate the offset from now until the expiry */
		timersub(&(ios->last_active), &now, tv);
		tv->tv_sec += ios->idle_timeout;
		tvp = tv;

		/* check if the timeout has expired */
		if (istimerexpired(tv)) {
			if (very_verbose_mode())
				warning(_("%s idle timed out"), ios->name);
			ios->flags |= IOS_IDLE_TIMEDOUT;
			timerclear(tv);
		}
	}
	
	/* no hold timeout if read hasn't seen EOF or hold time is infinite */
	if ((ios->flags & IOS_INPUT_EOF) && (ios->hold_time >= 0)) {
		struct timeval hold_tv;
		/* check if the hold timeout has been triggered */

		if (ios->hold_time == 0) {
			/* instant timeout */
			/* set flag */
			ios->flags |= IOS_HOLD_TIMEDOUT;
			timerclear(&hold_tv);
		} else {
			/* calculate the offset from now until expiry */
			/* now may already be set from calculating idle above */
			if (tvp == NULL)
				gettimeofday(&now, NULL);
			timersub(&(ios->read_eof), &now, &hold_tv);
			hold_tv.tv_sec += ios->hold_time;
		}

		/* check if the timeout has expired */
		if (istimerexpired(&hold_tv)) {
			if (very_verbose_mode())
				warning(_("%s hold timed out"), ios->name);
			/* set flag */
			ios->flags |= IOS_HOLD_TIMEDOUT;
			timerclear(&hold_tv);
		}

		/* update tv if required */
		if ((tvp == NULL) || (timercmp(&hold_tv, tv, <))) {
			*tv = hold_tv;
			tvp = tv;
		}
	}

#ifndef NDEBUG
	if (tvp && !istimerexpired(tvp) && very_verbose_mode())
		warning("%s timer expires in %d.%06d",
		     ios->name, tvp->tv_sec, tvp->tv_usec);
#endif

	return tvp;
}



ssize_t ios_read(io_stream_t *ios)
{
	ssize_t rr;

	/* check argument */
	ios_assert(ios);
	
	/* should only be called if ios_schedule_read returned a true result */
	assert(ios->fd_in >= 0);
	assert(cb_space(ios->buf_in) >= ios->nru);

	/* read as much as possible */
	if (ios->socktype == SOCK_DGRAM)
		rr = cb_recv(ios->buf_in, ios->fd_in, 0, NULL, 0);
	else
		rr = cb_read(ios->buf_in, ios->fd_in, 0);

	if (rr > 0) {
		ios->rcvd += rr;
#ifndef NDEBUG
		if (very_verbose_mode())
			warning("read %d bytes from %s", rr, ios->name);
#endif
		/* record that the ios was active */
		gettimeofday(&(ios->last_active), NULL);

		return rr;
	} else if (rr == 0) {
		/* read eof - close read stream */
		if (very_verbose_mode())
			warning(_("read eof from %s"), ios->name);

		/* record the time eof was received */
		gettimeofday(&(ios->read_eof), NULL);

		/* set the eof flag */
		ios->flags |= IOS_INPUT_EOF;

		/* shutdown the read endpoint */
		ios_shutdown(ios, SHUT_RD);

		return IOS_EOF;
	} else if (errno == EAGAIN) {
		/* not ready? */
		return 0;
	} else {
		/* weird error */
		if (very_verbose_mode())
			warning(_("error reading from %s: %s"),
			     ios->name, strerror(errno));
		return IOS_FAILED;
	}
}



ssize_t ios_write(io_stream_t *ios)
{
	ssize_t rr;

	/* check argument */
	ios_assert(ios);
	
	/* should only be called if ios_schedule_write returned a true result */
	assert(ios->fd_out >= 0);
	assert(!cb_is_empty(ios->buf_out));

	/* write as much as the mtu allows */
	if (ios->socktype == SOCK_DGRAM)
		rr = cb_send(ios->buf_out, ios->fd_out, ios->mtu, NULL, 0);
	else
		rr = cb_write(ios->buf_out, ios->fd_out, ios->mtu);

	if (rr > 0) {
		ios->sent += rr;
#ifndef NDEBUG
		if (very_verbose_mode())
			warning("wrote %d bytes to %s", rr, ios->name);
#endif
		/* record that the ios was active */
		gettimeofday(&(ios->last_active), NULL);

		/* shutdown the write if buf_out is empty and out eof is set */
		if ((ios->flags & IOS_OUTPUT_EOF) && cb_is_empty(ios->buf_out))
			ios_shutdown(ios, SHUT_WR);

		return rr;
	} else if (rr == 0) {
		/* shouldn't happen? */
		return 0;
	} else if (errno == EAGAIN) {
		/* not ready? */
		return 0;
	} else {
		if (very_verbose_mode()) {
			if (errno == EPIPE)
				warning(_("received SIGPIPE on %s"), ios->name);
			else
				warning(_("error writing to %s: %s"),
				     ios->name, strerror(errno));
		}
		return IOS_FAILED;
	}
}



void ios_write_eof(io_stream_t *ios)
{
	/* check argument */
	ios_assert(ios);
	
	ios->flags |= IOS_OUTPUT_EOF;
	/* check if the buffer is already empty */
	if (cb_is_empty(ios->buf_out))
		ios_shutdown(ios, SHUT_WR);
}



void ios_shutdown(io_stream_t *ios, int how)
{
	/* check argument */
	ios_assert(ios);

	if (how == SHUT_RDWR) {
		/* close both the input and the output */
		if (ios->fd_in < 0 && ios->fd_out < 0)
			return;

		if (ios->fd_in >= 0)
			close(ios->fd_in);
		/* if the same fd is input and output, don't close twice */
		if (ios->fd_out >= 0 && ios->fd_out != ios->fd_in)
			close(ios->fd_out);
		if (very_verbose_mode())
			warning(_("closed %s"), ios->name);
		ios->fd_in = ios->fd_out = -1;
	} else if (how == SHUT_RD) {
		/* close the input */
		if (ios->fd_in < 0)
			return;

		/* if the fd is duplex, use shutdown */
		if (ios->fd_in == ios->fd_out) {
			if (!ios->half_close_suppress) {
				shutdown(ios->fd_in, SHUT_RD);
				if (very_verbose_mode())
					warning(_("shutdown %s for read"),
					     ios->name);
			}
		} else {
			close(ios->fd_in);
			if (very_verbose_mode())
				warning(_("closed %s for read"), ios->name);
		}
		ios->fd_in = -1;
	} else {
		assert(how == SHUT_WR);	
		
		/* close the output */
		if (ios->fd_out < 0)
			return;

		/* if the fd is duplex, use shutdown */
		if (ios->fd_in == ios->fd_out) {
			if (!ios->half_close_suppress) {
				shutdown(ios->fd_out, SHUT_WR);
				if (very_verbose_mode())
					warning(_("shutdown %s for write"),
					     ios->name);
			}
		} else {
			close(ios->fd_out);
			if (very_verbose_mode())
				warning(_("closed %s for write"), ios->name);
		}
		ios->fd_out = -1;
	}
}
