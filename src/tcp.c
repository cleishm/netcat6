/*
 *  tcp.c - tcp networking module - implementation 
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
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "filter.h"
#include "misc.h"
#include "network.h"
#include "parser.h"
#include "readwrite.h"

#if HAVE___SS_FAMILY
#define ss_family __ss_family
#endif 

typedef void callback(int, int, io_stream*);


static int tcp_connect_to(sa_family_t family, address *remote, address *local);
static int tcp_bind_to(sa_family_t family, address *remote, address *local, 
                       callback *fn, io_stream *prm);



void tcp_connect(sa_family_t family, address *remote_addr, address *local_addr)
{
	int fd;
	io_stream remote, local;

	assert(remote_addr != NULL);
	
	stdio_to_io_stream(&local);

	fd = tcp_connect_to(family, remote_addr, local_addr);
	socket_to_io_stream(fd, &remote);

	readwrite(&remote, &local);
	close(fd);
}



void tcp_listen(sa_family_t family, address *remote_addr, address *local_addr)
{
	int fd;
	io_stream remote, local;

	assert(local_addr != NULL);
	
	stdio_to_io_stream(&local);

	fd = tcp_bind_to(family, remote_addr, local_addr, NULL, NULL);
	socket_to_io_stream(fd, &remote);

	readwrite(&remote, &local);
	close(fd);
}



/* this function opens a socket, connects the socket to the address specified 
 * by "remote" and returns the file descriptor of the socket. */
static int tcp_connect_to(sa_family_t family, address *remote, address *local)
{
	int err, fd, destlen;
	struct addrinfo hints, *res = NULL, *ptr;
	struct sockaddr_storage dest;

	/* make sure that the preconditions on the addresses and on the flags 
	 * are respected */
	assert(remote != NULL);
	assert(remote->address != NULL && strlen(remote->address) > 0);
	assert(remote->port != NULL && strlen(remote->port) > 0 );
	assert(is_flag_set(USE_UDP) == FALSE);
	assert(is_flag_set(REUSE_ADDR) == FALSE);
	
	/* setup hints structure to be passed to getaddrinfo */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = family;
	hints.ai_socktype = SOCK_STREAM;
#ifdef AI_ADDRCONFIG
	hints.ai_flags = AI_ADDRCONFIG;
#endif
	
	if (is_flag_set(NUMERIC_MODE) == TRUE) 
		hints.ai_flags |= AI_NUMERICHOST;
	
	/* get the IP address of the remote end of the connection */
	err = getaddrinfo(remote->address, remote->port, &hints, &res);
	if (err != 0) fatal("getaddrinfo error: %s", gai_strerror(err));

	/* check the results of getaddrinfo */
	assert(res != NULL);
	assert(res->ai_addrlen <= sizeof(dest));

	/* if the connect to the first address returned by getaddrinfo fails,
	 * then we keep trying with the other addresses */
	for (ptr = res; ptr != NULL; ptr = ptr->ai_next) {
		
		/* get the first sockaddr structure returned by getaddrinfo */
		memcpy(&dest, ptr->ai_addr, ptr->ai_addrlen);	
		destlen = ptr->ai_addrlen;

		/* create the socket */
		fd = socket(dest.ss_family, SOCK_STREAM, 0);
		if (fd < 0) fatal("cannot create the socket: %s", strerror(errno));

		/* handle -s option */
		if (local != NULL && (local->address != NULL || local->port != NULL)) {
			int srclen;
			struct sockaddr_storage src;
			struct addrinfo hints2, *res2 = NULL;
		
			/* make sure preconditions on local address are respected */
			assert(local->address == NULL || strlen(local->address) > 0);
			assert(local->port    == NULL || strlen(local->port) > 0);
		
			/* setup hints2 structure to be passed to getaddrinfo */
			memset(&hints2, 0, sizeof(hints2));
			hints2.ai_family   = family;
			hints2.ai_socktype = SOCK_STREAM;
	
			if (is_flag_set(NUMERIC_MODE) == TRUE) 
				hints2.ai_flags |= AI_NUMERICHOST;
		
			/* get the IP address of the local end of the connection */
			err = getaddrinfo(local->address, local->port, &hints2, &res2);
			if(err != 0) fatal("getaddrinfo error: %s", gai_strerror(err));

			/* check the results of getaddrinfo */
			assert(res2 != NULL);
			assert(res2->ai_addrlen <= sizeof(src));

			/* get the fisrt sockaddr structure returned by getaddrinfo */
			memcpy(&src, res2->ai_addr, res2->ai_addrlen);
			srclen = res2->ai_addrlen;
		
			/* cleanup to avoid memory leaks */
			freeaddrinfo(res2);

#ifdef IPV6_V6ONLY
			if (family == AF_INET6) {
				int on = 1;
				/* in case of error, we will go on anyway... */
				err = setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &on,
						 sizeof(on));
				if (err < 0) perror("error with sockopt IPV6_V6ONLY");
			}
#endif 

			/* bind to the local address */
			err = bind(fd, (struct sockaddr *)&src, srclen);
			if (err != 0) fatal("cannot use specified source addr/port: %s", 
		        	            strerror(errno));		
		}
		
		/* perform the connection */
		err = connect(fd, (struct sockaddr *)&dest, destlen);
		if (err == 0) break;
	}
	
	/* cleanup to avoid memory leaks */
	freeaddrinfo(res);
	
	/* if we have reached the end of the loop without having err == 0 then
	 * we have failed to establish the connection */
	if (err < 0) fatal("cannot establish connection: %s", strerror(errno));
	
	return fd;
}



/* this function has two working modes. 
 *
 * fd == NULL (single binding mode):
 * 	the function binds to the specified address and listens
 * 	for a single incoming connection. it returns the socket 
 * 	file descriptor of the first connection received.
 *
 * fd != NULL (continous binding mode):
 * 	the function binds to the specified address and enters
 * 	an infinite loop that listens for all incoming connections. 
 * 	at each incoming connection the function fn is called, with
 * 	the socket file descriptor and the prm parameter as arguments.
 */
static int tcp_bind_to(sa_family_t family, address *remote, address *local, 
                       callback *fn, io_stream *prm)
{
	int err, fd, ns;
	struct addrinfo hints, *res = NULL;
	struct sockaddr_storage src;

	/* make sure that the preconditions on the addresses and on the flags 
	 * are respected */
	assert(remote != NULL);
	assert(local != NULL);
	assert(local->address != NULL || local->port != NULL);
	assert(local->address == NULL || strlen(local->address) > 0);
	assert(local->port    == NULL || strlen(local->port) > 0);
	assert(is_flag_set(USE_UDP) == FALSE);
	 
	/* setup hints structure to be passed to getaddrinfo */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = family;
	hints.ai_flags    = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM;
	
	if (is_flag_set(NUMERIC_MODE) == TRUE) 
		hints.ai_flags |= AI_NUMERICHOST;

	/* get the IP address of the local end of the connection */
	err = getaddrinfo(local->address, local->port, &hints, &res);
	if (err != 0) fatal("getaddrinfo error: %s", gai_strerror(err));
		
	/* check the results of getaddrinfo */
	assert(res != NULL);
	assert(res->ai_addrlen <= sizeof(src));

	/* get the fisrt sockaddr structure returned by getaddrinfo */
	memcpy(&src, res->ai_addr, res->ai_addrlen);	
	
	/* cleanup to avoid memory leaks */
	freeaddrinfo(res);

	/* create the socket */
	fd = socket(src.ss_family, SOCK_STREAM, 0);
	if (fd < 0) fatal("cannot create the socket: %s", strerror(errno));

#ifdef IPV6_V6ONLY
	if (family == AF_INET6) {
		int on = 1;
		/* in case of error, we will go on anyway... */
		err = setsockopt(fd,IPPROTO_IPV6,IPV6_V6ONLY,&on,sizeof(on));
		if (err < 0) perror("error with sockopt IPV6_V6ONLY");
	}
#endif 
		
	if (is_flag_set(REUSE_ADDR) == TRUE) {
		int on = 1;
		/* in case of error, we will go on anyway... */
		err = setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
		if (err < 0) perror("error with sockopt SO_REUSEADDR");
	}

	/* bind to the local address */
	err = bind(fd, (struct sockaddr *)&src, sizeof(src));
	if (err != 0) fatal("cannot use specified source addr/port: %s", 
		            strerror(errno));

	/* the socket needs to listen for incoming connections. the backlog 
	 * parameter is set to 5 for backward compatibility (it seems that at
	 * least some BSD-derived system limit the backlog parmeter to this 
	 * value). */
	err = listen(fd, 5);
	if (err != 0) fatal("cannot listen on specified addr/port: %s", 
		            strerror(errno));

	/* enter in the accept loop */
 	for (;;) {
		size_t destlen;
		struct sockaddr_storage dest;
	
		destlen = sizeof(dest);	
		
		ns = accept(fd, (struct sockaddr *)&dest, &destlen);
		if (ns < 0) fatal("cannot accept connection: %s", 
		                  strerror(errno));

		if ((remote->address == NULL && remote->port == NULL) || 
		    is_allowed((struct sockaddr*)&dest, remote) == TRUE) {
			/* break the loop after first accept if 
			 * we're not running in continuos mode */
			if (fn == NULL) break;
			/* else, if we're running in continuos mode 
			 * let's send the results to the callback 
			 * function */
			else fn(ns, fd, prm);
		} else close(ns);
	} 
	
	close(fd);	
	return ns;
}



