/*
 *  connection.c - connection description structures and functions - implementation
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
#include "misc.h"
#include "connection.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <assert.h>
#include <netinet/in.h>

RCSID("@(#) $Header: /Users/cleishma/work/nc6-repo/nc6/src/connection.c,v 1.12 2003-01-01 10:05:32 chris Exp $");

/* default buffer size is 8kb */
static const size_t DEFAULT_BUFFER_SIZE = 8192;


void connection_attributes_init(connection_attributes *attrs)
{
	assert(attrs != NULL);

	attrs->proto = PROTO_UNSPECIFIED;
	attrs->type = TCP_SOCKET;

	memset((void*)&(attrs->remote_address), 0,
	       sizeof(attrs->remote_address));
	memset((void*)&(attrs->local_address), 0,
	       sizeof(attrs->local_address));

	cb_init(&(attrs->remote_buffer), DEFAULT_BUFFER_SIZE);
	cb_init(&(attrs->local_buffer), DEFAULT_BUFFER_SIZE);

	/* setup the remote stream to read into the remote buffer and write
	 * from the local buffer */
	io_stream_init(&(attrs->remote_stream),
		&(attrs->remote_buffer), &(attrs->local_buffer));
	/* setup the local stream to read into the local buffer and write
	 * from the remote buffer */
	io_stream_init(&(attrs->local_stream),
		&(attrs->local_buffer), &(attrs->remote_buffer));

	/* the remote stream has an instant hold timeout by default,
	 * which means that as soon as the remote read stream closes, the
	 * entire connection will be torn down */
	ios_set_hold_timeout(&(attrs->remote_stream), 0);

	/* by default we don't send TCP half closes to the remote system */
	ios_suppress_half_close(&(attrs->remote_stream), TRUE);

	/* no connect timeout */
	attrs->connect_timeout = -1;
}



void connection_attributes_destroy(connection_attributes *attrs)
{
	assert(attrs != NULL);

	io_stream_destroy(&(attrs->remote_stream));
	io_stream_destroy(&(attrs->local_stream));
}



void connection_attributes_to_addrinfo(struct addrinfo *ainfo,
                                       const connection_attributes *attrs)
{
	assert(ainfo != NULL);
	assert(attrs != NULL);

	switch (attrs->proto) {
		case PROTO_IPv6:
#ifdef ENABLE_INET6
			ainfo->ai_family = PF_INET6;
#else
			fatal("internal error: system does not support ipv6");
#endif
			break;
		case PROTO_IPv4:
			ainfo->ai_family = PF_INET;
			break;
		case PROTO_UNSPECIFIED:
			ainfo->ai_family = PF_UNSPEC;
			break;
		default:
			fatal("internal error: unknown socket domain");
	}
	
	switch (attrs->type) {
		case UDP_SOCKET:
			ainfo->ai_protocol = IPPROTO_UDP;
			/* strictly speaking, this should not be required
			 * since UDP implies a DGRAM type socket.  However, on
			 * some systems getaddrinfo fails if we set
			 * IPPROTO_UDP and don't set this */
			ainfo->ai_socktype = SOCK_DGRAM;
			break;
		case TCP_SOCKET:
			ainfo->ai_protocol = IPPROTO_TCP;
			/* strictly speaking, this should not be required
			 * since TCP implies a STREAM type socket.  However,
			 * on some systems getaddrinfo fails if we set
			 * IPPROTO_TCP and don't set this */
			ainfo->ai_socktype = SOCK_STREAM;
			break;
		default:
			fatal("internal error: unknown socket type");
	}
}
