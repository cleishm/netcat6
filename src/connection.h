/*
 *  connection.h - connection description structures and functions - header
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
#ifndef CONNECTION_H
#define CONNECTION_H

#include "io_stream.h"
#include <netdb.h>
#include <sys/time.h>

typedef enum sock_family_t {
	PROTO_UNSPECIFIED,
	PROTO_IPv6,
	PROTO_IPv4
} sock_family;

typedef enum sock_protocol_t {
	TCP_PROTOCOL,
	UDP_PROTOCOL
} sock_protocol;

typedef struct address_t
{
	char *address;
	char *service;
} address;

#define address_init(AD)	((AD)->address = (AD)->service = NULL)

typedef struct connection_attributes_t
{
	sock_family   family;
	sock_protocol protocol;
	address remote_address;
	address local_address;
	circ_buf remote_buffer;
	circ_buf local_buffer;
	io_stream remote_stream;
	io_stream local_stream;
	int connect_timeout;
} connection_attributes;

#define ca_set_family(CA, FAMILY)	((CA)->family = (FAMILY))
#define ca_set_protocol(CA, PROTO)	((CA)->protocol = (PROTO))
#define ca_set_remote_addr(CA, ADDR)	((CA)->remote_address = (ADDR))
#define ca_set_local_addr(CA, ADDR)	((CA)->local_address  = (ADDR))

#define ca_set_MTU(CA, MTU)	\
	ios_set_mtu(&((CA)->remote_stream),(MTU))
#define ca_set_NRU(CA, NRU)	\
	ios_set_nru(&((CA)->remote_stream),(NRU))
	
#define ca_set_connection_timeout(CA, CT)	\
	((CA)->connect_timeout = (CT))
	
#define ca_suppress_half_close_remote(CA)	\
	ios_suppress_half_close(&((CA)->remote_stream), FALSE)
#define ca_suppress_half_close_local(CA)	\
	ios_suppress_half_close(&((CA)->local_stream), FALSE)
	
#define ca_set_hold_timeout_remote(CA)	\
	ios_set_hold_timeout(&((CA)->remote_stream), -1)
#define ca_set_hold_timeout_local(CA)	\
	ios_set_hold_timeout(&((CA)->local_stream), -1)

#define ca_resize_local_buf(CA, SIZE)	cb_resize(&((CA)->local_buffer),(SIZE))
#define ca_resize_remote_buf(CA, SIZE)	cb_resize(&((CA)->remote_buffer),(SIZE))

void connection_attributes_init(connection_attributes *attrs);
void connection_attributes_destroy(connection_attributes *attrs);

void connection_attributes_to_addrinfo(struct addrinfo *ainfo,
                                       const connection_attributes *attrs);

#endif /* CONNECTION_H */
