/*
 *  network.h - common networking functions module - header
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
#ifndef NETWORK_H
#define NETWORK_H

#include <sys/socket.h>

/* maximum length of the string representing an IP address */
#define MAX_IP_ADDRLEN sizeof("0000:0000:0000:0000:0000:0000:255.255.255.255")
#define MAX_PORTLEN    sizeof("65535")


typedef struct address_t
{
	char *address;
	char *port;
} address;


void do_connect(sa_family_t family, address *remote_addr, address *local_addr);
void do_listen(sa_family_t family, address *remote_addr, address *local_addr);
	
#endif /* NETWORK_H */
