/*
 *  readwrite.h - stream i/o reading/writing loop - header
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

#ifndef READWRITE_H
#define READWRITE_H

#include "misc.h"
#include "udp.h"

typedef struct io_stream_t
{
	int fd_in;       /* for reading */
	int fd_out;      /* for writing */
	int socktype;    /* the type of the socket */
} io_stream;

void readwrite(io_stream *ios1, io_stream *ios2);
void stdio_to_io_stream(io_stream *ios);
void socket_to_io_stream(io_stream *ios, int fd, int socktype);

#endif /* READWRITE_H */
