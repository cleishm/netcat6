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
	size_t buffer_size;
	size_t remote_mtu;
	size_t remote_nru;
	int local_hold_timeout;
	int remote_hold_timeout;
	bool remote_half_close_suppress;
	bool local_half_close_suppress;
	int connect_timeout;
} connection_attributes;


void ca_init(connection_attributes *attrs);

#ifndef NDEBUG
void ca_destroy(connection_attributes *attrs);
#else 
#define ca_destroy(_x_)		do {} while(0)
#endif

#define ca_set_family(CA, FAMILY)	((CA)->family = (FAMILY))
#define ca_family(CA)			((CA)->family)
#define ca_set_protocol(CA, PROTO)	((CA)->protocol = (PROTO))
#define ca_protocol(CA)			((CA)->protocol)

#define ca_remote_address(CA)	((const address*)&((CA)->remote_address))
#define ca_local_address(CA)	((const address*)&((CA)->local_address))
#define ca_set_remote_addr(CA, ADDR)	((CA)->remote_address = (ADDR))
#define ca_set_local_addr(CA, ADDR)	((CA)->local_address  = (ADDR))

#define ca_buffer_size(CA)		((CA)->buffer_size)
#define ca_set_buffer_size(CA, SZ)	((CA)->buffer_size = (SZ))

#define ca_remote_MTU(CA)		((CA)->remote_mtu)
#define ca_set_remote_MTU(CA, MTU)	((CA)->remote_mtu = (MTU))

#define ca_remote_NRU(CA)		((CA)->remote_nru)
#define ca_set_remote_NRU(CA, NRU)	((CA)->remote_nru = (NRU))
	
#define ca_connect_timeout(CA)		((CA)->connect_timeout)
#define ca_set_connect_timeout(CA, CT)	((CA)->connect_timeout = (CT))

#define ca_remote_hold_timeout(CA)	((CA)->remote_hold_timeout)
#define ca_set_remote_hold_timeout(CA, T)		\
	((CA)->remote_hold_timeout = (T))
#define ca_local_hold_timeout(CA)	((CA)->local_hold_timeout)
#define ca_set_local_hold_timeout(CA, T)		\
	((CA)->local_hold_timeout = (T))

#define ca_remote_half_close_suppress(CA)		\
	((CA)->remote_half_close_suppress)
#define ca_set_remote_half_close_suppress(CA, B)	\
	((CA)->remote_half_close_suppress = (B))
#define ca_local_half_close_suppress(CA)		\
	((CA)->local_half_close_suppress)
#define ca_set_local_half_close_suppress(CA, B)	\
	((CA)->local_half_close_suppress = (B))
	
/* fill out an addrinfo structure with parameters from the ca */
void ca_to_addrinfo(struct addrinfo *ainfo, const connection_attributes *attrs);

#endif /* CONNECTION_H */
