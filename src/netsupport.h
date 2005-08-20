/*
 *  netsupport.h - networking support module - header
 * 
 *  nc6 - an advanced netcat clone
 *  Copyright (C) 2001-2005 Mauro Tortonesi <mauro _at_ deepspace6.net>
 *  Copyright (C) 2002-2005 Chris Leishman <chris _at_ leishman.org>
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
#ifndef NETSUPPORT_H
#define NETSUPPORT_H

#include <sys/socket.h>

/* issue the 'connect' call with a timeout.  Returns 0 on success and -1 on
 * failure (with errno set appropriately) */
int connect_with_timeout(int fd, const struct sockaddr *sa,
		socklen_t salen, int timeout);


/* On some systems, getaddrinfo will return results that can't actually be
 * used - resulting in a failure when trying to create the socket.
 * This function checks for all the different error codes that indicate this
 * situation */
bool unsupported_sock_error(int err);


#ifdef ENABLE_IPV6
/* returns true if a represents an ipv4-mapped address */
bool is_address_ipv4_mapped(const struct sockaddr *a);
#endif


/* compare two sockaddr structs to see if they represent the same address */
bool sockaddr_compare(const struct sockaddr *a, socklen_t a_len,
		const struct sockaddr *b, socklen_t b_len);


typedef struct bound_socket {
	int fd;
	int socktype;
	struct bound_socket *next;
} bound_socket_t;

/* add a new fd/socktype pair to the list */
bound_socket_t *add_bound_socket(bound_socket_t *list, int fd, int socktype);

/* get the socketype for a given fd in a list */
int get_bound_socket_type(const bound_socket_t *list, int fd);

/* free a bound socket list */
void free_bound_sockets(bound_socket_t *list);

/* close all bound sockets in a list and free the list */
void close_and_free_bound_sockets(bound_socket_t *list);


#endif/*NETSUPPORT_H*/
