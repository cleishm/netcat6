/*
 *  network.h - common networking functions module - header
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
#ifndef NETWORK_H
#define NETWORK_H

#include "connection.h"

/* establish a connection and return a new fd and socktype */
int do_connect(const connection_attributes *attrs, int *socktype);
int do_listen(const connection_attributes *attrs, int *socktype);

typedef void (*listen_callback)(int fd, int socktype, void *cdata);

void do_listen_continuous(const connection_attributes *attrs,
                          listen_callback callback, void *cdata,
			  int max_accept);

#endif /* NETWORK_H */
