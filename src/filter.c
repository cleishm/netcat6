/* 
 *  filter.c - incoming traffic validator module - implementation
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

#if HAVE_ALLOCA_H
#include <alloca.h>
#else
#ifdef _AIX
#pragma alloca
#else
#ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca();
#endif
#endif
#endif

RCSID("@(#) $Header: /Users/cleishma/work/nc6-repo/nc6/src/filter.c,v 1.27 2003-07-22 18:51:43 mauro Exp $");



#ifdef ENABLE_IPV6
/* returns TRUE if a represents an ipv4-mapped address */
bool is_address_ipv4_mapped(const struct sockaddr *a)
{
	bool ret = FALSE;
	const struct sockaddr_in6 *tmp = (const struct sockaddr_in6 *)a;
	
	assert(a != NULL);
	
	if ((a->sa_family == AF_INET6) && 
	    IN6_IS_ADDR_V4MAPPED(&(tmp->sin6_addr)))
	{
		ret = TRUE;
	}
			
	return ret;
}
#endif



/* compare two sockaddr structs to see if they represent the same address */
static bool sockaddr_compare(const struct sockaddr *a, const struct sockaddr *b)
{
	const struct sockaddr *aa, *bb;
    
	assert(a != NULL);
	assert(b != NULL);
	
	aa = (const struct sockaddr *)a;
	bb = (const struct sockaddr *)b;

#ifdef ENABLE_IPV6
	/* we have to handle IPv6 IPV4MAPPED addresses - convert them to IPv4 */
	if (is_address_ipv4_mapped(a)) {
		struct sockaddr *ap;
		ap = (struct sockaddr *)alloca(sizeof(struct sockaddr_in));
		memset(ap, 0, sizeof(struct sockaddr_in));
		memcpy(&(((struct sockaddr_in *)ap)->sin_addr.s_addr),
		       &(((const struct sockaddr_in6 *)a)->sin6_addr.s6_addr[12]),
		       sizeof(struct in_addr));
		((struct sockaddr_in *)ap)->sin_port =
			((const struct sockaddr_in6 *)a)->sin6_port;
		aa = ap;
	}

	if (is_address_ipv4_mapped(b)) {
		struct sockaddr *bp;
		bp = (struct sockaddr *)alloca(sizeof(struct sockaddr_in));
		memset(bp, 0, sizeof(struct sockaddr_in));
		memcpy(&(((struct sockaddr_in *)bp)->sin_addr.s_addr),
		       &(((const struct sockaddr_in6 *)b)->sin6_addr.s6_addr[12]),
		       sizeof(struct in_addr));
		((struct sockaddr_in *)bp)->sin_port =
			((const struct sockaddr_in6 *)b)->sin6_port;
		bb = bp;
	}
#endif

	/* now we can perform the comparison */

	/* check family is the same */
	if (aa->sa_family != bb->sa_family)
			return FALSE;

	/* the comparison is unique for each real sockaddr family */

#ifdef ENABLE_IPV6
	if (aa->sa_family == AF_INET6) {
#ifdef HAVE_SOCKADDR_IN6_SCOPE_ID
		/* compare scope */
		if (((const struct sockaddr_in6 *)aa)->sin6_scope_id &&
		    ((const struct sockaddr_in6 *)bb)->sin6_scope_id &&
		    (((const struct sockaddr_in6 *)aa)->sin6_scope_id !=
		     ((const struct sockaddr_in6 *)bb)->sin6_scope_id))
		{
			return FALSE;
		}
#endif
		/* compare address part 
		 * either may be IN6ADDR_ANY, resulting in a good match */
		if ((memcmp(&((const struct sockaddr_in6 *)aa)->sin6_addr,
		            &in6addr_any, sizeof(struct in6_addr)) != 0) &&
		    (memcmp(&((const struct sockaddr_in6 *)bb)->sin6_addr,
		            &in6addr_any, sizeof(struct in6_addr)) != 0) &&
		    (memcmp(&((const struct sockaddr_in6 *)aa)->sin6_addr, 
		            &((const struct sockaddr_in6 *)bb)->sin6_addr, 
			    sizeof(struct in6_addr)) != 0))
		{
			return FALSE;
		}

		/* compare port part 
		 * either port may be 0(any), resulting in a good match */
		return ((((const struct sockaddr_in6 *)aa)->sin6_port == 0) ||
		        (((const struct sockaddr_in6 *)bb)->sin6_port == 0) ||
		        (((const struct sockaddr_in6 *)aa)->sin6_port ==
		         ((const struct sockaddr_in6 *)bb)->sin6_port))?
		         TRUE : FALSE;
	}
#endif

	if (aa->sa_family == AF_INET) { 
		/* compare address part
		 * either may be INADDR_ANY, resulting in a good match */
		if ((((const struct sockaddr_in *)aa)->sin_addr.s_addr != INADDR_ANY) &&
		    (((const struct sockaddr_in *)bb)->sin_addr.s_addr != INADDR_ANY) &&
		    (((const struct sockaddr_in *)aa)->sin_addr.s_addr != 
		     ((const struct sockaddr_in *)bb)->sin_addr.s_addr))
		{
			return FALSE;
		}

		/* compare port part */
		/* either port may be 0(any), resulting in a good match */
		return ((((const struct sockaddr_in *)aa)->sin_port == 0) ||
		        (((const struct sockaddr_in *)bb)->sin_port == 0) ||
		        (((const struct sockaddr_in *)aa)->sin_port == 
		         ((const struct sockaddr_in *)bb)->sin_port))?
		         TRUE : FALSE;
	}
	
	/* for all other socket types, return false */
	return FALSE;
}



/* returns TRUE if sa corresponds to the address/port specified in addr */
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
	assert(attrs != NULL);

	/* if no address or port is supplied, match everything */
	if (addr->address == NULL && addr->service == NULL) return TRUE;
		
	memset(&hints, 0, sizeof(hints));
	ca_to_addrinfo(&hints, attrs);

	if (ca_is_flag_set(attrs, CA_NUMERIC_MODE))
		hints.ai_flags |= AI_NUMERICHOST;
	
	hints.ai_flags |= AI_PASSIVE;
	
	err = getaddrinfo(addr->address, addr->service, &hints, &res);
	if (err != 0) {
		/* some errors just indicate that the address wasn't suitable */
		switch (err) {
		case EAI_NODATA:
		case EAI_ADDRFAMILY:
		case EAI_SERVICE:
		case EAI_SOCKTYPE:
			return FALSE;
		default:
			fatal(_("getaddrinfo error: %s"), gai_strerror(err));
		}
	}

	ret = FALSE;
	
	for (ptr = res; ptr != NULL; ptr = ptr->ai_next) {

#ifdef ENABLE_IPV6
		/* skip IPv4 mapped addresses returned from getaddrinfo */
		if (is_address_ipv4_mapped(ptr->ai_addr))
			continue;
#endif

		if (sockaddr_compare(sa, ptr->ai_addr) == TRUE) {
			ret = TRUE;
			break;
		}
	}

	freeaddrinfo(res);

	return ret;
}
