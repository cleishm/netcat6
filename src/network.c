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
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include "network.h"
#include "parser.h"
#include "filter.h"
#include "readwrite.h"

/* buffer size for warning messages */
static const size_t WARN_MESSAGE_SIZE = 512;

/* maximum number of fd's to listen for connections on */
static const size_t MAX_LISTEN_FDS = 8;



static bool have_both_ipv4_and_ipv6_addresses(const struct addrinfo *ai)
{
	bool have_ipv4_addresses = FALSE;
	bool have_ipv6_addresses = FALSE;
	const struct addrinfo *ptr;
	
	for (ptr = ai; ptr != NULL; ptr = ptr->ai_next) {
		switch (ptr->ai_family) {
			case AF_INET:
				if (have_ipv6_addresses == TRUE) return TRUE;
				have_ipv4_addresses = TRUE;
				break;
			case AF_INET6:
				if (have_ipv4_addresses == TRUE) return TRUE;
				have_ipv6_addresses = TRUE;
				break;
			default:
				/* should this be a fatal error? i don't think so */
				warn("unknown address family");
		}
	}

	return FALSE;
}



void do_connect(const address *remote, const address *local,
		const connection_attributes *attrs)
{
	io_stream remote_stream, local_stream;
	int err, fd = -1;
	struct addrinfo hints, *res = NULL, *ptr;
	bool have_rev = FALSE;
	bool connect_attempted = FALSE;
	char hbuf_rev[NI_MAXHOST + 1]; /* MAX_IP_ADDRLEN + 1 should be enough */
	char hbuf_num[NI_MAXHOST + 1];
	char sbuf_rev[NI_MAXSERV + 1]; /* MAX_PORTLEN + 1 should be enough */
	char sbuf_num[NI_MAXSERV + 1];

	/* make sure that all the preconditions are respected */
	assert(remote != NULL);
	assert(remote->address != NULL);
	assert(remote->port != NULL);
	assert(local == NULL || ((local->address == NULL || strlen(local->address) > 0) &&
	                         (local->port    == NULL || strlen(local->port)    > 0)));
	assert(attrs != NULL);
	
	/* setup hints structure to be passed to getaddrinfo */
	memset(&hints, 0, sizeof(hints));

	switch (attrs->proto) {
		case PROTO_IPv6:
			hints.ai_family = AF_INET6;
			break;
		case PROTO_IPv4:
			hints.ai_family = AF_INET;
			break;
		case PROTO_UNSPECIFIED:
			hints.ai_family = AF_UNSPEC;
			break;
		default:
			fatal("internal error: unknown socket domain");
	}
	
	switch (attrs->type) {
		case UDP_SOCKET:
			hints.ai_socktype = SOCK_DGRAM;
			break;
		case TCP_SOCKET:
			hints.ai_socktype = SOCK_STREAM;
			break;
		default:
			fatal("internal error: unknown socket type");
	}

#ifdef HAVE_GETADDRINFO_AI_ADDRCONFIG
	hints.ai_flags |= AI_ADDRCONFIG;
#endif
	
	if (is_flag_set(NUMERIC_MODE) == TRUE)
		hints.ai_flags |= AI_NUMERICHOST;

	/* get the address of the remote end of the connection */
	err = getaddrinfo(remote->address, remote->port, &hints, &res);
	if (err != 0)
		fatal("forward host lookup failed for remote enpoint %s: %s",
		      remote->address, gai_strerror(err));

	/* check the results of getaddrinfo */
	assert(res != NULL);

	/* try connecting to any of the addresses returned by getaddrinfo */
	for (ptr = res; ptr != NULL; ptr = ptr->ai_next) {

		/* reset have_rev */
		have_rev = FALSE;
		
		/* only accept socktypes we can handle */
		if (ptr->ai_socktype != SOCK_STREAM && ptr->ai_socktype != SOCK_DGRAM)
			continue;

		/* we are going to try to connect to this address */
		connect_attempted = TRUE;

		/* get the numeric name for this destination as a string */
		err = getnameinfo(ptr->ai_addr, ptr->ai_addrlen,
		                  hbuf_num, sizeof(hbuf_num), sbuf_num, 
				  sizeof(sbuf_num), NI_NUMERICHOST | NI_NUMERICSERV);

		/* this should never happen */
		if(err != 0)
			fatal("getnameinfo failed: %s", gai_strerror(err));

		/* get the real name for this destination as a string */
		if ((is_flag_set(NUMERIC_MODE) == FALSE) && 
		    (is_flag_set(VERY_VERBOSE_MODE) == TRUE)) { /* should this be VERBOSE_MODE? */
			
			/* try reverse dns lookup */
			err = getnameinfo(ptr->ai_addr, ptr->ai_addrlen,
				          hbuf_rev, sizeof(hbuf_rev), sbuf_rev, 
				          sizeof(sbuf_rev), 0);
			if(err != 0) {
				/* this is not a fatal error */
				warn("inverse lookup failed for %s: %s",
				     hbuf_num, gai_strerror(err));
			} else {
				/* we have reverse dns information */
				have_rev = TRUE; 
			}
		}

		/* create the socket */
		fd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (fd < 0) fatal("cannot create the socket: %s", strerror(errno));
		
#ifdef IPV6_V6ONLY
		if (ptr->ai_family == AF_INET6) {
			int on = 1;
			/* in case of error, we will go on anyway... */
			err = setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &on,
					 sizeof(on));
			if (err < 0) warn("error with sockopt IPV6_V6ONLY");
		}
#endif 

		/* setup local source address and/or ports */
		if (local != NULL && (local->address != NULL || local->port != NULL)) {
			struct addrinfo *src_res = NULL, *src_ptr;
		
			/* setup hints structure to be passed to getaddrinfo */
			memset(&hints, 0, sizeof(hints));
			hints.ai_family   = ptr->ai_family;
			hints.ai_flags    = AI_PASSIVE;
			hints.ai_socktype = ptr->ai_socktype;
			hints.ai_protocol = ptr->ai_protocol;
	
			if (is_flag_set(NUMERIC_MODE) == TRUE) 
				hints.ai_flags |= AI_NUMERICHOST;
		
			/* get the IP address of the local end of the connection */
			err = getaddrinfo(local->address, local->port, &hints, &src_res);
			if(err != 0) {
				warn("forward host lookup failed for local endpoint %s: %s",
				     local->address, gai_strerror(err));
				close(fd);
				fd = -1;
				continue;
			}

			/* check the results of getaddrinfo */
			assert(src_res != NULL);

			/* try binding to any of the addresses returned by getaddrinfo */
			for (src_ptr = src_res; src_ptr != NULL; src_ptr = src_ptr->ai_next) {
				err = bind(fd, src_ptr->ai_addr, src_ptr->ai_addrlen);
				if (err == 0)
					break;
			}
			
			if (err != 0) {
				/* make sure we have tried all the addresses returned by 
				 * getaddrinfo */
				assert(src_ptr->ai_next == NULL);
				
				/* print error message */
				if (have_rev == TRUE) {
					warn("bind to source addr/port failed when "
					     "connecting to %s (addr. %s, rev. name %s) "
					     "port %s (number %s, name %s): %s",
					     remote->address, hbuf_num, hbuf_rev, 
					     remote->port, sbuf_num, sbuf_rev, strerror(errno));
				} else {
					warn("bind to source addr/port failed when "
					     "connecting to %s (addr. %s) "
					     "port %s (number %s): %s",
					     remote->address, hbuf_num,  
					     remote->port, sbuf_num, strerror(errno));
				}
				
				freeaddrinfo(src_res);
				close(fd);
				fd = -1;
				continue;
			}

			freeaddrinfo(src_res);
		}
		
		/* perform the connection */
		err = connect(fd, ptr->ai_addr, ptr->ai_addrlen);
		if (err != 0) {
			if (is_flag_set(VERBOSE_MODE) == TRUE) {
				if (have_rev == TRUE) {
					warn("connection to %s (addr. %s, rev. name %s) "
					     "port %s (number %s, name %s) failed: %s",
					     remote->address, hbuf_num, hbuf_rev, 
					     remote->port, sbuf_num, sbuf_rev, strerror(errno));
				} else {
					warn("connection to %s (addr. %s) "
					     "port %s (number %s) failed: %s",
					     remote->address, hbuf_num,  
					     remote->port, sbuf_num, strerror(errno));
				}
			}
			close(fd);
			fd = -1;
			continue;
		}

		if (fd >= 0)
			break;
	}

	assert(ptr == NULL || fd >= 0);
	
	/* if the connection failed, output an error message */
	if (ptr == NULL) {
		if (connect_attempted == FALSE) {
			fatal("forward lookup returned no usable socket types");
		} else {
			fatal("cannot connect to %s port %s", 
			      remote->address, remote->port);
		}
	}

	/* cleanup addrinfo structure */
	freeaddrinfo(res);

	/* let the user know the connection has been established */
	if (is_flag_set(VERBOSE_MODE)) {
		if (have_rev == TRUE) {
			warn("connection to %s (addr. %s, rev. name %s) "
			     "port %s (number %s, name %s) opened",
			     remote->address, hbuf_num, hbuf_rev, 
			     remote->port, sbuf_num, sbuf_rev);
		} else {
			warn("connection to %s (addr. %s) "
			     "port %s (number %s) opened",
			     remote->address, hbuf_num,  
			     remote->port, sbuf_num);
		}
	}

	/* create io_streams for the local and remote streams */
	stdio_to_io_stream(&local_stream);
	socket_to_io_stream(&remote_stream, fd, ptr->ai_socktype);

	/* read and write from the streams until they are closed */
	readwrite(&remote_stream, &local_stream);
}



void do_listen(const address *remote, const address *local,
	       const connection_attributes *attrs)
{
	io_stream remote_stream, local_stream;
	int nfd, i, fd, err, ns = -1, maxfd = -1;
	struct addrinfo hints, *res = NULL, *ptr;
	char hbuf_num[NI_MAXHOST + 1], sbuf_num[NI_MAXSERV + 1];
#if 0 /* REVERSE_DNS */
	cher hbuf_rev[NI_MAXHOST + 1], sbuf_rev[NI_MAXSERV + 1];
#endif
	fd_set accept_fdset;
	char *address;

	/* make sure all the preconditions are respected */
	assert(attrs != NULL);
	assert(local != NULL);
	assert(local->address == NULL || strlen(local->address) > 0);
	assert(local->port != NULL && strlen(local->port) > 0);
	assert(remote == NULL || ((remote->address == NULL || strlen(remote->address) > 0) &&
	                          (remote->port    == NULL || strlen(remote->port)    > 0)));
	
	/* setup this once for all */
	address = (local->address != NULL ? local->address : "UNSPECIFIED ADDRESS");
	
	/* initialize accept_fdset */
	FD_ZERO(&accept_fdset);
	
	/* setup hints structure to be passed to getaddrinfo */
	memset(&hints, 0, sizeof(hints));

	switch (attrs->proto) {
		case PROTO_IPv6:
			hints.ai_family = AF_INET6;
			break;
		case PROTO_IPv4:
			hints.ai_family = AF_INET;
			break;
		case PROTO_UNSPECIFIED:
			hints.ai_family = AF_UNSPEC;
			break;
		default:
			fatal("internal error: unknown socket domain");
	}
	
	switch (attrs->type) {
		case UDP_SOCKET:
			hints.ai_socktype = SOCK_DGRAM;
			break;
		case TCP_SOCKET:
			hints.ai_socktype = SOCK_STREAM;
			break;
		default:
			fatal("internal error: unknown socket type");
	}

	hints.ai_flags = AI_PASSIVE;

	if (is_flag_set(NUMERIC_MODE) == TRUE)
		hints.ai_flags |= AI_NUMERICHOST;

	/* get the IP address of the local end of the connection */
	err = getaddrinfo(local->address, local->port, &hints, &res);
	if (err != 0) 
		fatal("forward host lookup failed for local endpoint %s port %s: %s",
		      address, local->port, gai_strerror(err));
		
	/* check the results of getaddrinfo */
	assert(res != NULL);

#ifndef CAN_DOUBLE_BIND
	/* workaround for those systems that don't allow double binding */
	if (local->address == NULL && have_both_ipv4_and_ipv6_addresses(res) == TRUE)
		fatal("your host does not support indipendent double binding."
		      "Please specify the IP protocol version to use with -4 or -6.");
#endif
	
	nfd = 0;	
	
	/* try binding to all of the addresses returned by getaddrinfo */
	for (ptr = res; ptr != NULL; ptr = ptr->ai_next) {

#if 0 /* REVERSE_DNS */
		have_rev = FALSE;
#endif
		
		/* only accept socktypes we can handle */
		if (ptr->ai_socktype != SOCK_STREAM && ptr->ai_socktype != SOCK_DGRAM)
			continue;

		/* sanity check */
		assert((attrs->type == UDP_SOCKET && ptr->ai_socktype == SOCK_DGRAM) || 
		       (attrs->type == TCP_SOCKET && ptr->ai_socktype == SOCK_STREAM));

		/* get the numeric name for this source as a string */
		err = getnameinfo(ptr->ai_addr, ptr->ai_addrlen,
				  hbuf_num, sizeof(hbuf_num), sbuf_num, 
				  sizeof(sbuf_num), NI_NUMERICHOST | NI_NUMERICSERV);

		/* this should never happen */
		if(err != 0)
			fatal("getnameinfo failed: %s", gai_strerror(err));

#if 0 /* REVERSE_DNS */
		/* get the real name for this destination as a string */
		if ((is_flag_set(NUMERIC_MODE) == FALSE) && 
		    (is_flag_set(VERY_VERBOSE_MODE) == TRUE)) { /* should this be VERBOSE_MODE? */
			
			/* try reverse dns lookup */
			err = getnameinfo(ptr->ai_addr, ptr->ai_addrlen,
				          hbuf_rev, sizeof(hbuf_rev), sbuf_rev, 
				          sizeof(sbuf_rev), 0);
			if(err != 0) {
				/* this is not a fatal error */
				warn("inverse lookup failed for %s: %s",
				     hbuf_num, gai_strerror(err));
			} else {
				/* we have reverse dns information */
				have_rev = TRUE; 
			}
		}
#endif 

		/* create the socket */
		fd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if(fd < 0) fatal("cannot create the socket: %s", strerror(errno));

#ifdef IPV6_V6ONLY
		if (ptr->ai_family == AF_INET6) {
			int on = 1;
			/* in case of error, we will go on anyway... */
			err = setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on));
			if (err < 0) perror("error with sockopt IPV6_V6ONLY");
		}
#endif 
	
		/* set the reuse address socket option */
		if (is_flag_set(DONT_REUSE_ADDR) == FALSE) {
			int on = 1;
			/* in case of error, we will go on anyway... */
			err = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
			if (err < 0) perror("error with sockopt SO_REUSEADDR");
		}

		/* bind to the local address */
		err = bind(fd, ptr->ai_addr, ptr->ai_addrlen);
		if (err != 0) {
			/* print error message */
#if 0 /* REVERSE_DNS */
			if (have_rev == TRUE) {
				warn("bind to source addr/port failed when "
				     "connecting to %s (addr. %s, rev. name %s) "
				     "port %s (number %s, name %s): %s",
				     remote->address, hbuf_num, hbuf_rev, 
				     remote->port, sbuf_num, sbuf_rev, strerror(errno));
			} else {
				warn("bind to source addr/port failed when "
				     "connecting to %s (addr. %s) "
				     "port %s (number %s): %s",
				     remote->address, hbuf_num,  
				     remote->port, sbuf_num, strerror(errno));
			}
#else
			warn("bind to source address %s (%s) port %s (%s) failed: %s",
			     address, hbuf_num, remote->port, sbuf_num, strerror(errno));
#endif
			close(fd);
			continue;
		}

		/* for stream based sockets, the socket needs to listen for incoming
		 * connections. the backlog parameter is set to 5 for backward
		 * compatibility (it seems that at least some BSD-derived system limit
		 * the backlog parameter to this value). */
		if (ptr->ai_socktype == SOCK_STREAM) {
			err = listen(fd, 5);
			if (err != 0)
				fatal("cannot listen on source address %s (%s) port %s (%s): %s",
				      address, hbuf_num, remote->port, sbuf_num, strerror(errno));
		}

		if (is_flag_set(VERBOSE_MODE) == TRUE)
			fatal("listening on source address %s (%s) port %s (%s)",
			      address, hbuf_num, remote->port, sbuf_num, strerror(errno));

		/* add fd to accept_fdset */
		FD_SET(fd, &accept_fdset);
		maxfd = MAX(maxfd, fd);
		nfd++;
	}

	freeaddrinfo(res);
	
	if (nfd == 0)
		fatal("failed to bind to any local addr/port");

	/* enter into the accept loop */
 	for (;;) {
		fd_set tmp_ap_fdset;
		struct sockaddr_storage dest;
		socklen_t destlen;

		/* make a copy of accept_fdset before passing to select */
		memcpy(&tmp_ap_fdset, &accept_fdset, sizeof(fd_set));

		/* wait for an incoming connection */
		err = select(maxfd + 1, &tmp_ap_fdset, NULL, NULL, NULL);
		
		if (err < 0) {
			if (errno == EINTR) continue;
			fatal("select error: %s", strerror(errno));
		}

		/* find the ready filedescriptor */
		for (i = 0; i < maxfd && !FD_ISSET(i, &tmp_ap_fdset); ++i)
			;

		/* if none were ready, loop to select again */
		if (i == maxfd)
			continue;

		destlen = sizeof(dest);	

		/* for stream sockets we can simply accept a new connection, while for 
		 * dgram sockets we have to use MSG_PEEK to determine the sender */
		if (attrs->type == TCP_SOCKET) {
			ns = accept(i, (struct sockaddr *)&dest, &destlen);
			if (ns < 0)
				fatal("cannot accept connection: %s", strerror(errno));
		} else {
			err = recvfrom(i, NULL, 0, MSG_PEEK, (struct sockaddr*)&dest, &destlen);
			if (err < 0)
				fatal("cannot recv from socket: %s", strerror(errno));

			ns = dup(i);
			if (ns < 0)
				fatal("cannot duplicate file descriptor %d: %s", i, strerror(errno));
		}

#if 0 /* REVERSE_DNS */
		/* get names for each end of the connection */
		if (is_flag_set(VERBOSE_MODE) == TRUE) {
			char c_hbuf[NI_MAXHOST + 1], c_hbuf_n[NI_MAXHOST + 1];
			char c_sbuf_n[NI_MAXSERV + 1];
			struct sockaddr_storage src;
			socklen_t srclen = sizeof(src);

			/* find out what address the connection was to */
			err = getsockname(ns, (struct sockaddr *)&src, &srclen);
			if (err < 0)
				fatal("getsockname failed: %s", strerror(errno));

			/* get the numeric name for this source as a string */
			err = getnameinfo((struct sockaddr *)&src, srclen,
				          hbuf_num, sizeof(hbuf_num), NULL, 0,
				          NI_NUMERICHOST | NI_NUMERICSERV);

			/* this should never happen */
			if(err != 0)
				fatal("getnameinfo failed: %s", gai_strerror(err));

			/* get the numeric name for this client as a string */
			err = getnameinfo((struct sockaddr *)&dest, destlen,
				          c_hbuf_n, sizeof(c_hbuf_n), c_sbuf_n, 
					  sizeof(c_sbuf_n), NI_NUMERICHOST | NI_NUMERICSERV);
			if(err != 0)
				fatal("getnameinfo failed: %s", gai_strerror(err));

			/* get the real name for this client as a string */
			if (is_flag_set(NUMERIC_MODE) == FALSE) {
				err = getnameinfo((struct sockaddr *)&dest, destlen,
					c_hbuf, sizeof(c_hbuf), NULL, 0, 0);
				if(err != 0)
					fatal("inverse lookup failed for %s: %s",
					c_hbuf_n, gai_strerror(err));
			} else {
				strcpy(c_hbuf, c_hbuf_n);
			}
		}
#endif

		/* check if connections from this client are allowed */
		if ((remote == NULL) ||
		    (remote->address == NULL && remote->port == NULL) ||
		    (is_allowed((struct sockaddr*)&dest, remote, attrs) == TRUE)) {

			if (ptr->ai_socktype == SOCK_DGRAM) {
				/* connect the socket to ensure we only talk with this client */
				err = connect(ns, (struct sockaddr*)&dest, destlen);
				if (err != 0)
					fatal("cannot connect datagram socket: %s", strerror(errno));
			}

#if 0 /* REVERSE_DNS */
			if (is_flag_set(VERBOSE_MODE) == TRUE) {
				warn("connect to [%s] from %s [%s] %s",
				hbuf_n, c_hbuf, c_hbuf_n, c_sbuf_n);
			}
#endif

			break;
		} else {
			if (ptr->ai_socktype == SOCK_DGRAM) {
				/* the connection wasn't accepted - remove the queued packet */
				recvfrom(ns, NULL, 0, 0, NULL, 0);
			}
			close(ns);
			ns = -1;

#if 0 /* REVERSE_DNS */
			if (is_flag_set(VERBOSE_MODE) == TRUE) {
				warn("refused connect to [%s] from %s [%s] %s",
					hbuf, c_hbuf, c_hbuf_n, c_sbuf_n);
			}
#endif
		}
	}

	/* close the listening sockets */
	for (i = 0; i <= maxfd; ++i) {
		if (FD_ISSET(i, &accept_fdset) != 0) close(i);
	}

	/* create io_streams for the local and remote streams */
	stdio_to_io_stream(&local_stream);
	socket_to_io_stream(&remote_stream, ns, ptr->ai_socktype);

	/* read and write from the streams until they are closed */
	readwrite(&remote_stream, &local_stream);
}

