/*
 *  netsupport.c - networking support module - implementation
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

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "misc.h"
#include "netsupport.h"

RCSID("@(#) $Header: /Users/cleishma/work/nc6-repo/nc6/src/netsupport.c,v 1.9 2003-01-18 20:06:36 chris Exp $");


/* add a new fd/socktype pair to the list */
bound_socket *add_bound_socket(bound_socket *list,
                               int fd, int socktype)
{
	bound_socket *fdnew;
	
	fdnew = (bound_socket *)xmalloc(sizeof(bound_socket));
	
	fdnew->fd = fd;
	fdnew->socktype = socktype;
	
	/* prepend to the start of the list */
	fdnew->next = list;
	
	return fdnew;
}



/* retrieve a socktype for a given fd from the list */
int find_bound_socket(const bound_socket *list, int fd)
{
	assert(list != NULL);
	
	while (list != NULL && list->fd != fd)
		list = list->next;
	
	return ((list != NULL) ? list->socktype : -1);
}



/* destroy a bound_socket list */
void destroy_bound_sockets(bound_socket *list)
{
	bound_socket *tmp;
	
	while (list != NULL) {
		tmp = list;
		list = list->next;
		free(tmp);
	}
}



void close_and_destroy_bound_sockets(bound_socket *list)
{
	bound_socket *tmp;
	
	while (list != NULL) {
		tmp = list;
		list = list->next;
		close(tmp->fd);
		free(tmp);
	}
}



/* 
 * Some systems (notably Linux) will bind to both IPv6 AND IPv4 when
 * listening.  Connections will still be accepted from either IPv6 or
 * IPv4 clients (IPv4 will be mapped into IPv6).  However, this means
 * that we MUST bind the IPv6 address ONLY for these hosts.
 *
 * To handle this, we will ensure that IPv6 sockets are bound first and then
 * attempt to bind the IPv4 ones.  On systems that double bind IPv6/IPv4 the
 * IPv4 bind will simply fail.  This function does the reordering - moving all
 * IPv6 addresses to the start of the getaddrinfo results.
 */
#ifdef ENABLE_IPV6
struct addrinfo* order_ipv6_first(struct addrinfo *ai)
{
	struct addrinfo* ptr;
	struct addrinfo* lastv6 = NULL;
	struct addrinfo* tmp;

	assert(ai != NULL);

	/* Move all IPv6 addresses to the start of the list - keeping
	 * them in the original order. */

	if (ai->ai_family == PF_INET6)
		lastv6 = ai;

	for (ptr = ai; ptr != NULL && ptr->ai_next != NULL; ptr = ptr->ai_next) {
		if (ptr->ai_next->ai_family == PF_INET6) {
			tmp = ptr->ai_next;
			ptr->ai_next = tmp->ai_next;
			if (lastv6) {
				tmp->ai_next = lastv6->ai_next;
				lastv6->ai_next = tmp;
			} else {
				tmp->ai_next = ai;
				ai = tmp;
			}
			lastv6 = tmp;
		}
	}

	return ai;
}
#endif



/* On some systems, getaddrinfo will return results that can't actually be
 * used - resulting in a failure when trying to create the socket.
 * This function checks for all the different error codes that indicate this
 * situation */
bool unsupported_sock_error(int err)
{
	return (err == EPFNOSUPPORT ||
	        err == EAFNOSUPPORT ||
	        err == EPROTONOSUPPORT ||
		err == ESOCKTNOSUPPORT ||
	        err == ENOPROTOOPT)?
		TRUE : FALSE;
}



