/*
 *  network.c - common networking functions module - implementation
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
#include <sys/socket.h>
#include "network.h"
#include "parser.h"
#include "tcp.h"
#include "udp.h"



void do_connect(sa_family_t family, address *remote_addr, address *local_addr)
{
	if (is_flag_set(USE_UDP) == TRUE) {
		udp_connect(family, remote_addr, local_addr);
	} else {
		tcp_connect(family, remote_addr, local_addr);
	}
}



void do_listen(sa_family_t family, address *remote_addr, address *local_addr)
{
	if (is_flag_set(USE_UDP) == TRUE) {
		udp_listen(family, local_addr);
	} else {
		tcp_listen(family, remote_addr, local_addr);
	}
}

