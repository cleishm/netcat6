/*
 *  filter.c - incoming traffic validator module - implementation
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
#include "filter.h"
#include "misc.h"
#include "network.h"
#include "parser.h"
#include <assert.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


/* compare two sockaddr structs to see if they represent the same address */
static bool are_address_equal(const struct sockaddr *a, 
			      const struct sockaddr *b)
{
	const struct sockaddr *aa, *bb;
    
	assert(a != NULL);
	assert(b != NULL);
	assert(a->sa_family == AF_INET || a->sa_family == AF_INET6);
	assert(b->sa_family == AF_INET || b->sa_family == AF_INET6);
	
	aa = (const struct sockaddr *)a;
	bb = (const struct sockaddr *)b;
	
	/* we have to handle those --damned-- IPV4MAPPED addresses */
	if (aa->sa_family != bb->sa_family) {
		if (a->sa_family == AF_INET6 && 
		    IN6_IS_ADDR_V4MAPPED(&((const struct sockaddr_in6 *)a)->sin6_addr)) {
			struct sockaddr *ap;
			ap = (struct sockaddr *)alloca(sizeof(struct sockaddr_in));
			memset(ap, 0, sizeof(struct sockaddr_in));
			memcpy(&(((struct sockaddr_in *)ap)->sin_addr.s_addr),
	    	               &(((const struct sockaddr_in6 *)a)->sin6_addr.s6_addr[12]),
		               sizeof(struct in_addr));
			((struct sockaddr_in *)ap)->sin_port = 
				((const struct sockaddr_in6 *)a)->sin6_port;
			aa = ap;
		} else if (b->sa_family == AF_INET6 && 
                           IN6_IS_ADDR_V4MAPPED(&((const struct sockaddr_in6 *)b)->sin6_addr)) {
			struct sockaddr *bp;
			bp = (struct sockaddr *)alloca(sizeof(struct sockaddr_in));
			memset(bp, 0, sizeof(struct sockaddr_in));
			memcpy(&(((struct sockaddr_in *)bp)->sin_addr.s_addr),
	    	               &(((const struct sockaddr_in6 *)b)->sin6_addr.s6_addr[12]),
		               sizeof(struct in_addr));
			((struct sockaddr_in *)bp)->sin_port = 
				((const struct sockaddr_in6 *)b)->sin6_port;
			bb = bp;
		} else {
			return FALSE;
		}
	}

	/* now we can perform the comparison */
	if (aa->sa_family == AF_INET6) {
		if (memcmp(&((const struct sockaddr_in6 *)aa)->sin6_addr, 
		           &((const struct sockaddr_in6 *)bb)->sin6_addr, 
		           sizeof(struct sockaddr_in6)) != 0) return FALSE;
	} else { 
		if (((const struct sockaddr_in *)aa)->sin_addr.s_addr != 
	            ((const struct sockaddr_in *)bb)->sin_addr.s_addr) return FALSE;
	}
	
	return TRUE;
}



/* returns port number in network byte horder */
static unsigned short get_port(const struct sockaddr *sa)
{
	unsigned short ret;
	
	assert(sa != NULL);
	assert(sa->sa_family == AF_INET || sa->sa_family == AF_INET6);
	
	switch (sa->sa_family) {
		case AF_INET:
			ret = ((const struct sockaddr_in *)sa)->sin_port;
			break;
		case AF_INET6:
			ret = ((const struct sockaddr_in6 *)sa)->sin6_port;
			break;
		default:
			fatal("address family not supported", sa->sa_family);
			/* stop a compiler warning about unassigned ret */
			ret = -1;
	}
	
	return ret;
}



/* returns TRUE if a represents an ipv4-mapped address */
static bool is_address_ipv4_mapped(const struct sockaddr *a)
{
	bool ret = FALSE;
	
	assert(a != NULL);
	
	if (a->sa_family == AF_INET6 && 
	    IN6_IS_ADDR_V4MAPPED(&(((const struct sockaddr_in6 *)a)->sin6_addr)))
		ret = TRUE;
			
	return ret;
}



/* returns TRUE if sa corresponds to the address/port couple specified in addr */
bool is_allowed(const struct sockaddr *sa, const address *addr,
		const connection_attributes *attrs)
{
	struct addrinfo hints, *res = NULL, *ptr;
	int err;
	bool ret;

	assert(sa != NULL);
	assert(addr != NULL);	
	assert(addr->address == NULL || strlen(addr->address) > 0);
	assert(addr->service == NULL || strlen(addr->service) > 0);
		
	if (addr->address == NULL && addr->service == NULL) return TRUE;
		
	/* if the address is unspecified and the service is allowed, 
	 * then return TRUE */
	if ((addr->address == NULL) && 
	    ((safe_atoi(addr->service) == ntohs(get_port(sa))))) 
		return TRUE;
	
	ret = FALSE;
	
	memset(&hints, 0, sizeof(hints));
	
	switch (attrs->proto) {
		case PROTO_IPv6:
			hints.ai_family = AF_INET6;
			break;
		case PROTO_IPv4:
			hints.ai_family = AF_INET;
			break;
		case PROTO_UNSPECIFIED:
			hints.ai_family = AF_UNSPEC;
			break;
		default:
			fatal("internal error: unknown socket domain");
	}
	
	switch (attrs->type) {
		case UDP_SOCKET:
			hints.ai_socktype = SOCK_DGRAM;
			break;
		case TCP_SOCKET:
			hints.ai_socktype = SOCK_STREAM;
			break;
		default:
			fatal("internal error: unknown socket type");
	}
	
	err = getaddrinfo(addr->address, addr->service, &hints, &res);
	if (err != 0) fatal("getaddrinfo error: %s", gai_strerror(err));

	for (ptr = res; ptr != NULL; ptr = ptr->ai_next) {
		if ((is_flag_set(STRICT_IPV6) == TRUE) &&
		    is_address_ipv4_mapped(ptr->ai_addr)) {
			/* cannot accept address */
			continue;
		}
		if ((are_address_equal(sa, ptr->ai_addr) == TRUE) &&
		    (addr->service == NULL || 
		     (safe_atoi(addr->service) == ntohs(get_port(sa))))) {
			ret = TRUE;
			break;
		}
	}

	freeaddrinfo(res);

	return ret;
}



