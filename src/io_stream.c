/* vi:ts=4 sw=4
 *
 *  io_stream.c - stream i/o wrapper - implementation 
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
#include <fcntl.h>

RCSID("@(#) $Header: /Users/cleishma/work/nc6-repo/nc6/src/io_stream.c,v 1.3 2002-12-24 14:54:00 chris Exp $");


/* static void nonblock(int fd); */


void io_stream_init(io_stream *ios)
{
	assert(ios);

	ios->fd_in = -1;
	ios->fd_out = -1;
	ios->socktype = 0; /* unknown */
	ios->hold_time = 0; /* instant */
	timerclear(&(ios->read_closed));
}



void io_stream_destroy(io_stream *ios)
{
	assert(ios);
	ios_shutdown(ios, SHUT_RDWR);
}



void ios_assign_socket(io_stream *ios, int fd, int socktype)
{
	assert(ios);
	assert(fd >= 0);

	/* nonblock(fd); */
	ios->fd_in  = fd;
	ios->fd_out = fd;
	ios->socktype = socktype;
}



void ios_assign_stdio(io_stream *ios)
{
	assert(ios);

	if ((ios->fd_in  = dup(STDIN_FILENO)) < 0) 
		fatal("error in duplicating stdin file descriptor: %s", 
		      strerror(errno));
	
	if ((ios->fd_out = dup(STDOUT_FILENO)) < 0) 
		fatal("error in duplicating stdout file descriptor: %s", 
		      strerror(errno));

	/* nonblock(ios->fd_in); */
	/* nonblock(ios->fd_out); */

	ios->socktype = SOCK_STREAM;   /* pretend stdio is a stream socket */
}



void ios_shutdown(io_stream* ios, int how)
{
	assert(ios);

	if (how == SHUT_RDWR) {
		/* close both the input and the output */
		if (ios->fd_in != -1)
		{
			close(ios->fd_in);
			/* record the read shutdown time */
			gettimeofday(&(ios->read_closed), NULL);
		}
		/* if the same fd is input and output, don't close twice */
		if (ios->fd_out != -1 && ios->fd_out != ios->fd_in)
			close(ios->fd_out);
		ios->fd_in = ios->fd_out = -1;
	} else if (how == SHUT_RD) {
		/* close the input */
		if (ios->fd_in != -1) {
			/* if the fd is duplex, use shutdown */
			if (ios->fd_in == ios->fd_out)
				shutdown(ios->fd_in, SHUT_RD);
			else
				close(ios->fd_in);
			ios->fd_in = -1;
			/* record the read shutdown time */
			gettimeofday(&(ios->read_closed), NULL);
		}
	} else {
		assert(how == SHUT_WR);		
		/* close the output */
		if (ios->fd_out != -1) {
			/* if the fd is duplex, use shutdown */
			if (ios->fd_in == ios->fd_out)
				shutdown(ios->fd_out, SHUT_WR);
			else
				close(ios->fd_out);
			ios->fd_out = -1;
		}
	}
}



struct timeval* ios_next_timeout(io_stream *ios, struct timeval *tv)
{
	struct timeval now;

	assert(ios);
	assert(tv);

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



/*
static void nonblock(int fd)
{
	int arg;
	if ((arg = fcntl(fd, F_GETFL, 0)) < 0)
		fatal("error reading file descriptor flags: %s", strerror(errno));

	arg |= O_NONBLOCK;

	if (fcntl(fd, F_SETFL, arg) < 0)
		fatal("error setting flag O_NONBLOCK on file descriptor",
			strerror(errno));
}
*/
