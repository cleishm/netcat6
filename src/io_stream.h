/*
 *  io_stream.h - stream i/o wrapper - header
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
#ifndef IO_STREAM_H
#define IO_STREAM_H

#include "misc.h"
#include "circ_buf.h"
#include <sys/time.h>

typedef struct io_stream_t
{
	int fd_in;       /* for reading */
	int fd_out;      /* for writing */
	int socktype;    /* the type of the socket */
	int hold_time;   /* time to hold the stream open after read closes,
	                    -1 means hold indefinately */
	struct timeval read_closed; /* the time that the read was closed */
} io_stream;

void io_stream_init(io_stream *ios);
void io_stream_destroy(io_stream *ios);

#define ios_readfd(IOS)   ((IOS)->fd_in)
#define ios_writefd(IOS)  ((IOS)->fd_out)

void ios_assign_socket(io_stream *ios, int fd, int socktype);
void ios_assign_stdio(io_stream *ios);
void ios_shutdown(io_stream *ios, int how);

#define is_read_open(IOS)   ((IOS)->fd_in >= 0)
#define is_write_open(IOS)  ((IOS)->fd_out >= 0)

/* sets the time (in sec) after read is shutdown that timeout occurs */
#define ios_set_hold_timeout(IOS, T)  ((IOS)->hold_time = (T))

/* Writes the interval to the next timeout into tv and returns a pointer
 * to tv.  If no timeout is active, NULL is returned and tv is unchanged. */
struct timeval* ios_next_timeout(io_stream *ios, struct timeval *tv);

#endif /* IO_STREAM_H */
