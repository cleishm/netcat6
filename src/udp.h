/*
 *  udp.h - udp networking module - header 
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
#ifndef UDP_H
#define UDP_H

#include <sys/socket.h>
#include "network.h"

struct udp_connection
{
        int fd;
        size_t destlen;
        struct sockaddr_storage dest;
        address *dest_addr;
};

void udp_connect(const sa_family_t family,
		const address *remote_addr, const address *local_addr);
void udp_listen(const sa_family_t family,
		const address *remote_addr, const address *local_addr);
	
#endif /* UDP_H */
