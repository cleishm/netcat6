/*
 *  network.h - common networking functions module - header
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
#ifndef NETWORK_H
#define NETWORK_H


#include <netdb.h>

typedef enum sock_type_t {
	UDP_SOCKET,
	TCP_SOCKET
} sock_type;

typedef enum sock_protocol_t {
	PROTO_IPv6,
	PROTO_IPv4,
	PROTO_UNSPECIFIED
} sock_proto;

typedef struct address_t
{
	char *address;
	char *port;
} address;

typedef struct connection_attributes_t
{
	sock_proto proto;
	sock_type  type;	
} connection_attributes;


void do_connect(const address *remote, const address *local,
		const connection_attributes *attrs);
void do_listen(const address *remote, const address *local,
	       const connection_attributes *attrs);


#endif /* NETWORK_H */
