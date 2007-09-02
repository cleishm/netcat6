/*
 *  iucv.h - iucv networking functions module - header
 * 
 *  Copyright IBM Corp. 2007
 *  Author: Tom Zanussi <zanussi _at_ us.ibm.com>
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
#ifndef IUCV_H
#define IUCV_H

#include "afindep.h"

#ifndef AF_IUCV
#define PF_IUCV		32
#define AF_IUCV		PF_IUCV
#endif

#define IUCV_MAXNAME	8

struct sockaddr_iucv {
  sa_family_t     siucv_family;
  unsigned short  siucv_port;                        /* Reserved */
  unsigned int    siucv_addr;                        /* Reserved */
  char            siucv_nodeid[8];                   /* Reserved */
  char            siucv_userid[IUCV_MAXNAME];        /* Guest User Id */
  char            siucv_name[IUCV_MAXNAME];          /* Application Name */
};

/* establish a connection and return a new fd and socktype */
int iucv_connect(const struct addrinfo *hints,
		 const char *remote_address, const char *remote_service,
		 const char *local_address, const char *local_service,
		 set_sockopt_handler_t set_sockopt_handler, void *hdata,
		 time_t timeout, int *rt_socktype);

int iucv_listener(const struct addrinfo *hints,
		  const char *local_address, const char *local_service,
		  const char *remote_address, const char *remote_service,
		  set_sockopt_handler_t set_sockopt_handler, void *hdata,
		  listen_callback_t callback, void *cdata,
		  time_t timeout, int max_accept);

#endif/*IUCV_H*/
