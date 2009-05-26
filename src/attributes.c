/*
 *  attributes.c - structs and funcs for connection attributes - implementation
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
#include "misc.h"
#include "attributes.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <assert.h>
#include <netinet/in.h>


/* default buffer size is 8kb */
static const size_t DEFAULT_BUFFER_SIZE = 8192;
/* default UDP buffer size is 128k */
static const size_t DEFAULT_DGRAM_BUFFER_SIZE = 131072;
/* default MTU is unlimited */
static const size_t DEFAULT_MTU = 0;
/* default datagram MTU is 8kb */
static const size_t DEFAULT_DGRAM_MTU = 8192;
/* default NRU is 1 */
static const size_t DEFAULT_NRU = 1;
/* default datagram NRU is the maximum allowed MTU of 64k */
static const size_t DEFAULT_DGRAM_NRU = 65536;



void ca_init(connection_attributes_t *attrs)
{
	assert(attrs != NULL);

	attrs->flags = 0;
	attrs->family = PF_UNSPEC;
	attrs->protocol = 0;
	attrs->socktype = 0;
	address_init(&(attrs->remote_address));
	address_init(&(attrs->local_address));
	attrs->buffer_size = 0;
	attrs->remote_mtu = 0;
	attrs->remote_nru = 0;
	attrs->sndbuf_size = 0;
	attrs->rcvbuf_size = 0;
	attrs->connect_timeout = -1;
	attrs->idle_timeout = -1;
	attrs->remote_hold_timeout = 0;
	attrs->local_hold_timeout = -1;
	attrs->remote_half_close_suppress = true;
	attrs->local_half_close_suppress = false;
	attrs->local_exec = NULL;
}



void ca_destroy(connection_attributes_t *attrs)
{
	assert(attrs != NULL);
	ca_set_local_exec(attrs, NULL);
}



size_t ca_buffer_size(const connection_attributes_t *attrs, int socktype)
{
	size_t buffer_size = attrs->buffer_size;
	size_t remote_nru = ca_remote_NRU(attrs, socktype);
	if (buffer_size == 0) {
		switch (socktype) {
		case SOCK_DGRAM:
			buffer_size = DEFAULT_DGRAM_BUFFER_SIZE;
			break;
		default:
			buffer_size = DEFAULT_BUFFER_SIZE;
			break;
		}
	}
	/* buffer size can never be smaller than nru or data will never be
	 * received */
	if (buffer_size < remote_nru)
		buffer_size = remote_nru;
	return buffer_size;
}



size_t ca_remote_MTU(const connection_attributes_t *attrs, int socktype)
{
	size_t remote_mtu = attrs->remote_mtu;
	if (remote_mtu == 0) {
		switch (socktype) {
		case SOCK_DGRAM:
			remote_mtu = DEFAULT_DGRAM_MTU;
			break;
		default:
			remote_mtu = DEFAULT_MTU;
			break;
		}
	}
	return remote_mtu;
}



size_t ca_remote_NRU(const connection_attributes_t *attrs, int socktype)
{
	size_t remote_nru = attrs->remote_nru;
	if (remote_nru == 0) {
		switch (socktype) {
		case SOCK_DGRAM:
			remote_nru = DEFAULT_DGRAM_NRU;
			break;
		default:
			remote_nru = DEFAULT_NRU;
			break;
		}
	}
	return remote_nru;
}



void ca_set_local_exec(connection_attributes_t *attrs, const char *exec)
{
	if (attrs->local_exec)
		free(attrs->local_exec);
	attrs->local_exec = exec? xstrdup(exec) : NULL;
}



void ca_to_addrinfo(struct addrinfo *ainfo,
		const connection_attributes_t *attrs)
{
	assert(ainfo != NULL);
	assert(attrs != NULL);

	if (ca_is_flag_set(attrs, CA_NUMERICHOST))
		ainfo->ai_flags |= AI_NUMERICHOST;

	ainfo->ai_family = ca_family(attrs);
	ainfo->ai_socktype = ca_socktype(attrs);
	ainfo->ai_protocol = ca_protocol(attrs);

	switch (ainfo->ai_family) {
#ifdef ENABLE_BLUEZ
	case PF_BLUETOOTH:
		/* default protocol is L2CAP */
		if (ainfo->ai_protocol == 0)
			ainfo->ai_protocol = BTPROTO_L2CAP;
		if (ainfo->ai_socktype == 0) {
			/* set default socktype for protocol */
			switch (ainfo->ai_protocol) {
			case BTPROTO_L2CAP:
			case BTPROTO_SCO:
				ainfo->ai_socktype = SOCK_SEQPACKET;
				break;
			default:
				/* leave undefined */
				break;
			}
		}
		break;
#endif
	case PF_INET:
	case PF_INET6:
	default:
		/* assume unspecified family will be INET or INET6 */
		if (ainfo->ai_socktype == 0) {
			/* set socktype based on protocol, or default */
			switch (ainfo->ai_protocol) {
			case IPPROTO_UDP:
				ainfo->ai_socktype = SOCK_DGRAM;
				break;
			case IPPROTO_TCP:
				ainfo->ai_socktype = SOCK_STREAM;
				break;
			default:
				ainfo->ai_socktype = SOCK_STREAM;
				break;
			}
		}
		break;
	}
}
