/*
 *  connection.h - connection description structures and functions - header
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
#ifndef CONNECTION_H
#define CONNECTION_H

#include "io_stream.h"
#include <netdb.h>
#include <sys/time.h>

typedef enum sock_type_t {
	TCP_SOCKET,
	UDP_SOCKET
} sock_type;

typedef enum sock_protocol_t {
	PROTO_UNSPECIFIED,
	PROTO_IPv6,
	PROTO_IPv4
} sock_proto;

typedef struct address_t
{
	char *address;
	char *service;
} address;

typedef struct connection_attributes_t
{
	sock_proto proto;
	sock_type  type;
	address remote_address;
	address local_address;
	io_stream remote_stream;
	io_stream local_stream;
	time_t connect_timeout;
} connection_attributes;


void connection_attributes_init(connection_attributes *attrs);
void connection_attributes_destroy(connection_attributes *attrs);

void connection_attributes_to_addrinfo(struct addrinfo *ainfo,
                                       const connection_attributes *attrs);

#endif /* CONNECTION_H */
