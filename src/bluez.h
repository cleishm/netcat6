/*
 *  bluez.h - bluetooth networking functions module - header
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
#ifndef BLUEZ_H
#define BLUEZ_H

#include "afindep.h"

/* establish a connection and return a new fd and socktype */
int bluez_connect(const struct addrinfo *hints,
		const char *remote_address, const char *remote_service,
		set_sockopt_handler_t set_sockopt_handler, void *hdata,
		time_t timeout, int *socktype);

/* listen for connects and issue callbacks */
int bluez_listener(const struct addrinfo *hints,
		const char *local_address, const char *local_service,
		const char *remote_address, const char *remote_service,
		set_sockopt_handler_t set_sockopt_handler, void *hdata,
		listen_callback_t callback, void *cdata,
		time_t timeout, int max_accept);

#endif/*BLUEZ_H*/
