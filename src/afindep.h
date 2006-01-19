/*
 *  afindep.h - address family independant networking functions
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
#ifndef AFINDEP_H
#define AFINDEP_H

#include <netdb.h>
#include <sys/types.h>

typedef void (*set_sockopt_handler_t)(int sock, void *hdata);
typedef void (*listen_callback_t)(int fd, int socktype, void *cdata);


/* establish a connection and return a new fd and socktype */
int afindep_connect(const struct addrinfo *hints,
		const char *remote_address, const char *remote_service,
		const char *local_address, const char *local_service,
		set_sockopt_handler_t set_sockopt_handler, void *hdata,
		time_t timeout, int *socktype);


/* listen for connects and issue callbacks */
int afindep_listener(const struct addrinfo *hints,
		const char *local_address, const char *local_service,
		const char *remote_address, const char *remote_service,
		set_sockopt_handler_t set_sockopt_handler, void *hdata,
		listen_callback_t callback, void *cdata,
		time_t timeout, int max_accept);

#endif/*AFINDEP_H*/
