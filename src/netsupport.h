/*
 *  netsupport.h - networking support module - header
 * 
 *  nc6 - an advanced netcat clone
 *  Copyright (C) 2001-2002 Mauro Tortonesi <mauro _at_ deepspace6.net>
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

#include <netdb.h>

typedef struct bound_socket_t {
	int fd;
	int socktype;
	struct bound_socket_t *next;
} bound_socket;

bound_socket *add_bound_socket(bound_socket *list,
                               int fd, int socktype);
int find_bound_socket(const bound_socket *list, int fd);
void destroy_bound_sockets(bound_socket *list);
void close_and_destroy_bound_sockets(bound_socket *list);
#ifdef ENABLE_IPV6
struct addrinfo* order_ipv6_first(struct addrinfo *ai);
#endif
bool unsupported_sock_error(int err);

#endif /* NETSUPPORT_H */
