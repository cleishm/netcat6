/*
 *  attributes.h - structs and funcs for connection attributes - header
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
#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

#include "io_stream.h"
#include <netdb.h>
#include <sys/time.h>

typedef struct address {
	const char *nodename;
	const char *service;
} address_t;

#define address_init(AD)	((AD)->nodename = (AD)->service = NULL)


typedef struct connection_attributes
{
	int flags;
	int family;
	int socktype;
	int protocol;
	address_t remote_address;
	address_t local_address;
	size_t buffer_size;
	size_t remote_mtu;
	size_t remote_nru;
	size_t sndbuf_size;
	size_t rcvbuf_size;
	int connect_timeout;
	int idle_timeout;
	int remote_hold_timeout;
	int local_hold_timeout;
	bool remote_half_close_suppress;
	bool local_half_close_suppress;
	char *local_exec;
} connection_attributes_t;

/* CA flags */
#define CA_NUMERICHOST		0x000001
#define CA_DONT_REUSE_ADDR	0x000002
#define CA_PASSIVE		0x000004
#define CA_RECV_DATA_ONLY	0x000008
#define CA_SEND_DATA_ONLY	0x000010
#define CA_DISABLE_NAGLE	0x000020
#define CA_CONTINUOUS_ACCEPT	0x000040

void ca_init(connection_attributes_t *attrs);
void ca_destroy(connection_attributes_t *attrs);

#define ca_is_flag_set(CA, FLG)		((CA)->flags & (FLG))
#define ca_set_flag(CA, FLG)		((CA)->flags |=  (FLG))
#define ca_clear_flag(CA, FLG)		((CA)->flags &= ~(FLG))

#define ca_family(CA)			((CA)->family)
#define ca_set_family(CA, FAMILY)	((CA)->family = (FAMILY))
#define ca_socktype(CA)			((CA)->socktype)
#define ca_set_socktype(CA, SOCKTYPE)	((CA)->socktype = (SOCKTYPE))
#define ca_protocol(CA)			((CA)->protocol)
#define ca_set_protocol(CA, PROTO)	((CA)->protocol = (PROTO))

#define ca_local_address(CA)	((const address_t *)&((CA)->local_address))
#define ca_set_local_addr(CA, ADDR)	((CA)->local_address  = (ADDR))
#define ca_remote_address(CA)	((const address_t *)&((CA)->remote_address))
#define ca_set_remote_addr(CA, ADDR)	((CA)->remote_address = (ADDR))

size_t ca_buffer_size(const connection_attributes_t *attrs, int socktype);
#define ca_set_buffer_size(CA, SZ)	((CA)->buffer_size = (SZ))

size_t ca_remote_MTU(const connection_attributes_t *attrs, int socktype);
#define ca_set_remote_MTU(CA, MTU)	((CA)->remote_mtu = (MTU))

size_t ca_remote_NRU(const connection_attributes_t *attrs, int socktype);
#define ca_set_remote_NRU(CA, NRU)	((CA)->remote_nru = (NRU))

#define ca_sndbuf_size(CA)		((CA)->sndbuf_size)
#define ca_set_sndbuf_size(CA, SZ)	((CA)->sndbuf_size = (SZ))

#define ca_rcvbuf_size(CA)		((CA)->rcvbuf_size)
#define ca_set_rcvbuf_size(CA, SZ)	((CA)->rcvbuf_size = (SZ))

#define ca_connect_timeout(CA)		((CA)->connect_timeout)
#define ca_set_connect_timeout(CA, CT)	((CA)->connect_timeout = (CT))

#define ca_idle_timeout(CA)		((CA)->idle_timeout)
#define ca_set_idle_timeout(CA, CT)	((CA)->idle_timeout = (CT))

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

#define ca_local_exec(CA)		(const char*)(((CA)->local_exec))
void ca_set_local_exec(connection_attributes_t *attrs, const char *exec);

/* fill out an addrinfo structure with parameters from the ca */
void ca_to_addrinfo(struct addrinfo *ainfo,
		const connection_attributes_t *attrs);

#endif/*ATTRIBUTES_H*/
