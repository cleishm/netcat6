/*
 *  udp.c - udp networking module - implementation 
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
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "config.h"
#include "circ_buf.h"
#include "filter.h"
#include "misc.h"
#include "network.h"
#include "parser.h"
#include "readwrite.h"
#include "timeout.h"
#include "udp.h"

#if HAVE___SS_FAMILY
#define ss_family __ss_family
#endif 

static void udp_connect_to(sa_family_t family, const address *remote, const address *local, 
		           struct udp_connection *udpc);
static void udp_bind_to(sa_family_t family, const address *local, struct udp_connection *udpc);



void udp_connect(sa_family_t family, const address *remote_addr, const address *local_addr)
{
	struct udp_connection udpc;
	io_stream local;

	assert(remote_addr != NULL);
	assert(is_flag_set(USE_UDP) == TRUE);
	
	stdio_to_io_stream(&local);

	udp_connect_to(family, remote_addr, local_addr, &udpc);
	
	udp_readwrite(&udpc, &local);

	close(udpc.fd);
}



void udp_listen(sa_family_t family, const address *local_addr)
{
	struct udp_connection udpc;
	io_stream local;

	assert(local_addr != NULL);
	assert(is_flag_set(USE_UDP) == TRUE);
	
	stdio_to_io_stream(&local);

	udp_bind_to(family, local_addr, &udpc);

	udp_readwrite(&udpc, &local);
	
	close(udpc.fd);
}



/* this function opens a socket, connects the socket to the address specified 
 * in addr and returns the file descriptor of the socket. */
static void udp_connect_to(sa_family_t family, const address *remote, const address *local, 
		           struct udp_connection *udpc)
{
	int err;
	size_t srclen;
	struct sockaddr_storage src;
	struct addrinfo hints, *res1 = NULL, *res2 = NULL, *ptr;

	/* make sure that the preconditions on the addresses and on the flags 
	 * are respected */
	assert(udpc != NULL);
	assert(remote != NULL);
	assert(remote->address != NULL && strlen(remote->address) > 0);
	assert(remote->port != NULL && strlen(remote->port) > 0 );
	assert(is_flag_set(USE_UDP) == TRUE);
	assert(is_flag_set(REUSE_ADDR) == FALSE);

	/* reset src structure size */
	srclen  = 0;

	/* initialize structure udpc */
	udpc->destlen = 0;
	udpc->dest_addr = (address *)remote;
		
	/* handle -s option */
	if (local != NULL && (local->address != NULL || local->port != NULL)) {
		
		/* make sure preconditions on local address are respected */
		assert(local->address == NULL || strlen(local->address) > 0);
		assert(local->port    == NULL || strlen(local->port) > 0);
		
		/* setup hints structure to be passed to getaddrinfo */
		memset(&hints, 0, sizeof(hints));
		hints.ai_family   = family;
		hints.ai_flags    = AI_PASSIVE;
		hints.ai_socktype = SOCK_DGRAM;
	
		if (is_flag_set(NUMERIC_MODE) == TRUE) 
			hints.ai_flags |= AI_NUMERICHOST;
		
		/* get the IP address of the local end of the connection */
		err = getaddrinfo(local->address, local->port, &hints, &res1);
		if(err != 0) fatal("getaddrinfo error: %s", gai_strerror(err));

		/* check the results of getaddrinfo */
		assert(res1 != NULL);

		/* get the fisrt sockaddr structure returned by getaddrinfo. 
		 * this should not be a problem, as getaddrinfo should return 
		 * only one struct. */
		assert(res1->ai_addrlen <= sizeof(src));
		memcpy(&src, res1->ai_addr, res1->ai_addrlen);
		srclen = res1->ai_addrlen;
	}
		
	/* setup hints structure to be passed to getaddrinfo */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = family;
#ifdef HAVE_GETADDRINFO_AI_ADDRCONFIG
	hints.ai_flags    = AI_ADDRCONFIG;
#endif
	hints.ai_socktype = SOCK_DGRAM;
	
	if (is_flag_set(NUMERIC_MODE) == TRUE) 
		hints.ai_flags |= AI_NUMERICHOST;
	
	/* get the IP address of the remote end of the connection */
	err = getaddrinfo(remote->address, remote->port, &hints, &res2);
	if(err != 0) fatal("getaddrinfo error: %s", gai_strerror(err));

	/* check the results of getaddrinfo */
	assert(res2 != NULL);
	
	if (srclen != 0) {
		/* if a local address is given, get the first sockaddr 
		 * structure returned by getaddrinfo which is consistent 
		 * with the local address */
		for (ptr = res2; ptr != NULL; ptr = ptr->ai_next) {
			if (ptr->ai_family == src.ss_family) {
				assert(ptr->ai_addrlen <= sizeof(udpc->dest));
				memcpy(&udpc->dest, ptr->ai_addr, ptr->ai_addrlen);
				udpc->destlen = ptr->ai_addrlen;
				break;
			}
		}
		/* check that a consistent result */
		if (udpc->destlen == 0) 
			fatal("cannot connect");
	} else {
		/* if no local address is given, we can simply get the first 
		 * sockaddr structure returned by getaddrinfo. */ 
		assert(res2->ai_addrlen <= sizeof(udpc->dest));
		memcpy(&udpc->dest, res2->ai_addr, res2->ai_addrlen);
		udpc->destlen = res2->ai_addrlen;
	}

	/* create the socket */
	udpc->fd = socket(udpc->dest.ss_family, SOCK_DGRAM, 0);
	if (udpc->fd < 0) fatal("cannot create the socket: %s", strerror(errno));
	
#ifdef IPV6_V6ONLY
	if (udpc->dest.ss_family == AF_INET6) {
		int on = 1;
		
		/* always set IPV6_V6ONLY for AF_INET6 sockets */
		err = setsockopt(udpc->fd, IPPROTO_IPV6, IPV6_V6ONLY, &on,
				 sizeof(on));
		
		/* in case of error, we will go on anyway... */
		if (err < 0) perror("error with sockopt IPV6_V6ONLY");
	}
#endif 

	if (srclen != 0) {
		/* bind to the local address */
		err = bind(udpc->fd, (struct sockaddr *)&src, srclen);
		if (err != 0) 
			fatal("cannot use specified source addr/port: %s", 
		              strerror(errno));
	}

	/* cleanup to avoid memory leaks */
	freeaddrinfo(res1);
	freeaddrinfo(res2);
}



static void udp_bind_to(sa_family_t family, const address *local, 
		        struct udp_connection *udpc)
{
	int err;
	struct addrinfo hints, *res = NULL;
	struct sockaddr_storage src;
	int srclen;

	/* make sure that the preconditions on the addresses and on the flags 
	 * are respected */
	assert(local != NULL);
	assert(local->address != NULL || local->port != NULL);
	assert(local->address == NULL || strlen(local->address) > 0);
	assert(local->port    == NULL || strlen(local->port) > 0);
	assert(is_flag_set(USE_UDP) == TRUE);
	 
	/* initialize structure udpc */
	udpc->destlen = 0;
	udpc->dest_addr = NULL;
	
	/* setup hints structure to be passed to getaddrinfo */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = family;
	hints.ai_flags    = AI_PASSIVE;
	hints.ai_socktype = SOCK_DGRAM;
	
	if (is_flag_set(NUMERIC_MODE) == TRUE) 
		hints.ai_flags |= AI_NUMERICHOST;

	/* get the IP address of the remote end of the connection */
	err = getaddrinfo(local->address, local->port, &hints, &res);
	if (err != 0) fatal("getaddrinfo error: %s", gai_strerror(err));
		
	/* check the results of getaddrinfo */
	assert(res != NULL);

	/* get the first sockaddr structure returned by getaddrinfo */
	assert(res->ai_addrlen <= sizeof(src));
	memcpy(&src, res->ai_addr, res->ai_addrlen);
	srclen = res->ai_addrlen;
	
	/* cleanup to avoid memory leaks */
	freeaddrinfo(res);

	/* create the socket */
	udpc->fd = socket(res->ai_family, SOCK_DGRAM, 0);
	if (udpc->fd < 0) fatal("cannot create the socket: %s", strerror(errno));

#ifdef IPV6_V6ONLY
	if (res->ai_family == AF_INET6) {
		int on = 1;
		
		/* always set IPV6_V6ONLY for AF_INET6 sockets */
		err = setsockopt(udpc->fd, IPPROTO_IPV6, IPV6_V6ONLY, &on,
				 sizeof(on));
		
		/* in case of error, we will go on anyway... */
		if (err < 0) perror("error with sockopt IPV6_V6ONLY");
	}
#endif 
		
	if (is_flag_set(REUSE_ADDR) == TRUE) {
		int on = 1;
		
		err = setsockopt(udpc->fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
		
		/* in case of error, we will go on anyway... */
		if (err < 0) perror("error with sockopt SO_REUSEADDR");
	}

	/* bind to the local address */
	err = bind(udpc->fd, (struct sockaddr *)&src, srclen);
	if (err != 0) fatal("cannot use specified source addr/port: %s", 
		            strerror(errno));
}



