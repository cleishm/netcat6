/*
 *  filter.h - incoming traffic validator module - header
 * 
 *  nc6 - an advanced netcat clone
 *  Copyright (C) 2001-2004 Mauro Tortonesi <mauro _at_ deepspace6.net>
 *  Copyright (C) 2002-2004 Chris Leishman <chris _at_ leishman.org>
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
#ifndef FILTER_H
#define FILTER_H

#include "misc.h"
#include "connection.h"
#include <sys/types.h>
#include <sys/socket.h>

#ifdef ENABLE_IPV6
bool is_address_ipv4_mapped(const struct sockaddr *a);
#endif

bool is_allowed(const struct sockaddr *sa, const address *addr,
		const connection_attributes* attrs);

#endif /* FILTER_H */
