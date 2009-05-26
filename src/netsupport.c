/*
 *  netsupport.c - networking support module - implementation
 * 
 *  nc6 - an advanced netcat clone
 *  Copyright (C) 2002-2006 Chris Leishman <chris _at_ leishman.org>
 *  Copyright (C) 2001-2006 Mauro Tortonesi <mauro _at_ deepspace6.net>
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
#include "system.h"
#include "misc.h"
#include "netsupport.h"
#ifdef ENABLE_BLUEZ
#include "bluez.h"
#endif

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#if HAVE_ALLOCA_H
#include <alloca.h>
#else
#ifdef _AIX
#pragma alloca
#else
#ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca();
#endif
#endif
#endif


/* call 'connect' in non-blocking mode and use select to await a timeout */
int connect_with_timeout(int fd, const struct sockaddr *sa,
		socklen_t salen, int timeout)
{
	int err;
	struct timeval tv, *tvp = NULL;
	fd_set connect_fdset;
	socklen_t len;
	int optval;
	
	assert(sa != NULL);
	assert(salen > 0);
	
	/* set connect timeout */
	if (timeout > 0) {
		tv.tv_sec = (time_t)timeout;
		tv.tv_usec = 0;
		tvp = &tv;
	}

	/* set fd to nonblocking */
	nonblock(fd);
	
	/* attempt the connection */
	err = connect(fd, sa, salen);
	
	if (err != 0 && errno == EINPROGRESS) {
		/* connection is proceeding
		 * it is complete (or failed) when select returns */

		/* initialize connect_fdset */
		FD_ZERO(&connect_fdset);
		FD_SET(fd, &connect_fdset);

		/* call select */
		do {
			err = select(fd + 1, NULL, &connect_fdset, 
				     NULL, tvp);
		} while (err < 0 && errno == EINTR);

		/* select error */
		if (err < 0)
			return -1;
	
		/* we have reached a timeout */
		if (err == 0) 
		{
			errno = ETIMEDOUT;
			return -1;
		}
		
		/* select returned successfully, but we must test socket 
		 * error for result */
		len = sizeof(optval);
		err = getsockopt(fd, SOL_SOCKET, SO_ERROR, &optval, &len);
		if (err != 0)
			return err;
		
		/* if getsockopt indicates a failure occured, then store the
		 * error code into errno and return a failure */
		if (optval != 0)
		{
			errno = optval;
			return -1;
		}
	}

	return 0;
}



#ifdef ENABLE_IPV6
/* returns true if a represents an ipv4-mapped address */
bool is_address_ipv4_mapped(const struct sockaddr *a)
{
	bool ret = false;
	const struct sockaddr_in6 *tmp = (const struct sockaddr_in6 *)a;
	
	assert(a != NULL);
	
	if ((a->sa_family == AF_INET6) && 
	    IN6_IS_ADDR_V4MAPPED(&(tmp->sin6_addr)))
	{
		ret = true;
	}
			
	return ret;
}
#endif



/* compare two sockaddr structs to see if they represent the same address */
bool sockaddr_compare(const struct sockaddr *a, socklen_t a_len,
		const struct sockaddr *b, socklen_t b_len)
{
	assert(a != NULL);
	assert(b != NULL);
	assert(a_len > 0);
	assert(b_len > 0);
	
#ifdef ENABLE_IPV6
	/* we have to handle IPv6 IPV4MAPPED addresses - convert them to IPv4 */
	if (is_address_ipv4_mapped(a)) {
		const struct sockaddr_in6 *a6;
		struct sockaddr_in *a_in;

		a6 = (const struct sockaddr_in6 *)a;
		a_in = (struct sockaddr_in *)alloca(sizeof(struct sockaddr_in));

		memset(a_in, 0, sizeof(struct sockaddr_in));
		memcpy(&(a_in->sin_addr.s_addr), &(a6->sin6_addr.s6_addr[12]),
		       sizeof(struct in_addr));
		a_in->sin_port = a6->sin6_port;
		a = (const struct sockaddr *)a_in;
	}

	if (is_address_ipv4_mapped(b)) {
		const struct sockaddr_in6 *b6;
		struct sockaddr_in *b_in;

		b6 = (const struct sockaddr_in6 *)b;
		b_in = (struct sockaddr_in *)alloca(sizeof(struct sockaddr_in));

		memset(b_in, 0, sizeof(struct sockaddr_in));
		memcpy(&(b_in->sin_addr.s_addr), &(b6->sin6_addr.s6_addr[12]),
		       sizeof(struct in_addr));
		b_in->sin_port = b6->sin6_port;
		b = (const struct sockaddr *)b_in;
	}
#endif

	/* now we can perform the comparison */

	/* check family is the same */
	if (a->sa_family != b->sa_family)
		return false;

	/* the comparison is unique for each real sockaddr family */

#ifdef ENABLE_IPV6
	if (a->sa_family == AF_INET6) {
		const struct sockaddr_in6 *a6 = (const struct sockaddr_in6 *)a;
		const struct sockaddr_in6 *b6 = (const struct sockaddr_in6 *)b;

#ifdef HAVE_SOCKADDR_IN6_SCOPE_ID
		/* compare scope */
		if (a6->sin6_scope_id && b6->sin6_scope_id &&
		    (a6->sin6_scope_id != b6->sin6_scope_id))
		{
			return false;
		}
#endif
		/* compare address part 
		 * either may be IN6ADDR_ANY, resulting in a good match */
		if ((memcmp(&(a6->sin6_addr), &in6addr_any,
		            sizeof(struct in6_addr)) != 0) &&
		    (memcmp(&(b6->sin6_addr), &in6addr_any,
			    sizeof(struct in6_addr)) != 0) &&
		    (memcmp(&(a6->sin6_addr), &(b6->sin6_addr), 
			    sizeof(struct in6_addr)) != 0))
		{
			return false;
		}

		/* compare port part 
		 * either port may be 0(any), resulting in a good match */
		return ((a6->sin6_port == 0) || (b6->sin6_port == 0) ||
		        (a6->sin6_port == b6->sin6_port))? true : false;
	}
#endif

	if (a->sa_family == AF_INET) { 
		const struct sockaddr_in *a_in = (const struct sockaddr_in *)a;
		const struct sockaddr_in *b_in = (const struct sockaddr_in *)b;

		/* compare address part
		 * either may be INADDR_ANY, resulting in a good match */
		if ((a_in->sin_addr.s_addr != INADDR_ANY) &&
		    (b_in->sin_addr.s_addr != INADDR_ANY) &&
		    (a_in->sin_addr.s_addr != b_in->sin_addr.s_addr))
		{
			return false;
		}

		/* compare port part */
		/* either port may be 0(any), resulting in a good match */
		return ((a_in->sin_port == 0) || (b_in->sin_port == 0) ||
		        (a_in->sin_port == b_in->sin_port))? true : false;
	}

#ifdef ENABLE_BLUEZ
	if (a->sa_family == AF_BLUETOOTH) {
		/* just compare the sockaddr structures directly */
		return ((a_len == b_len) && memcmp(a, b, a_len) == 0)?
			true : false;
	}
#endif
	
	/* for all other socket types, return false */
	return false;
}



/* On some systems, getaddrinfo will return results that can't actually be
 * used - resulting in a failure when trying to create the socket.
 * This function checks for all the different error codes that indicate this
 * situation */
bool unsupported_sock_error(int err)
{
	return (err == EPFNOSUPPORT ||
	        err == EAFNOSUPPORT ||
	        err == EPROTONOSUPPORT ||
		err == ESOCKTNOSUPPORT ||
	        err == ENOPROTOOPT)?
		true : false;
}



/* add a new fd/socktype pair to the list */
bound_socket_t *add_bound_socket(bound_socket_t *list,
                                 int fd, int socktype)
{
	bound_socket_t *fdnew;
	
	fdnew = (bound_socket_t *)xmalloc(sizeof(bound_socket_t));
	
	fdnew->fd = fd;
	fdnew->socktype = socktype;
	
	/* prepend to the start of the list */
	fdnew->next = list;
	
	return fdnew;
}



/* get the socktype for a given fd in a list */
int get_bound_socket_type(const bound_socket_t *list, int fd)
{
	assert(list != NULL);
	
	while (list != NULL && list->fd != fd)
		list = list->next;
	
	return ((list != NULL) ? list->socktype : -1);
}



/* free a bound socket list */
void free_bound_sockets(bound_socket_t *list)
{
	bound_socket_t *tmp;
	
	while (list != NULL) {
		tmp = list;
		list = list->next;
		free(tmp);
	}
}



/* close all bound sockets in a list and free the list */
void close_and_destroy_bound_sockets(bound_socket_t *list)
{
	bound_socket_t *tmp;
	
	while (list != NULL) {
		tmp = list;
		list = list->next;
		close(tmp->fd);
		free(tmp);
	}
}



/* wrapper around getaddrinfo that understands some additional protocols
 * that the system getaddrinfo doesn't. */
int getaddrinfo_ex(const char *nodename, const char *servname,
		const struct addrinfo *hints, struct addrinfo **res)
{
	return getaddrinfo(nodename, servname, hints, res);
}



void freeaddrinfo_ex(struct addrinfo *ai)
{
	freeaddrinfo(ai);
}



/* wrapper arround getnameinfo that understands some additional protocols
 * and formats in a human understandable way. */
int getnameinfo_ex(const struct sockaddr *sa, socklen_t len, char *str,
		size_t size, bool numeric_mode)
{
	int err;
	char hbuf_rev[NI_MAXHOST + 1];
	char hbuf_num[NI_MAXHOST + 1];
	char sbuf_rev[NI_MAXSERV + 1];
	char sbuf_num[NI_MAXSERV + 1];

	/* check arguments */
	assert(sa != NULL);
	assert(len > 0);
	assert(str != NULL);
	assert(size > 0);

	/* get the numeric name for this destination as a string */
	err = getnameinfo(sa, len, hbuf_num, sizeof(hbuf_num),
			  sbuf_num, sizeof(sbuf_num),
			  NI_NUMERICHOST | NI_NUMERICSERV);

	if (err != 0)
		return err;

	if (numeric_mode == false) {
		/* get the real name for this destination as a string */
		err = getnameinfo(sa, len, hbuf_rev, sizeof(hbuf_rev),
				  sbuf_rev, sizeof(sbuf_rev), 0);
		if (err == 0) {
			snprintf(str, size, "%s (%s) %s [%s]", hbuf_rev, 
			         hbuf_num, sbuf_num, sbuf_rev);
		} else {
			warning(_("inverse lookup failed for %s: %s"),
			        hbuf_num, gai_strerror(err));
			
			snprintf(str, size, "%s %s", hbuf_num, sbuf_num);
		}
	} else {
		snprintf(str, size, "%s %s", hbuf_num, sbuf_num);
	}

	return 0;
}



void xgetnameinfo_ex(const struct sockaddr *sa, socklen_t len, char *str,
		size_t size, bool numeric_mode)
{
	int err = getnameinfo_ex(sa, len, str, size, numeric_mode);
	if (err != 0) {
		fatal(_("getnameinfo failed: %s"), gai_strerror(err));
	}
}



/* wrapper around getaddrinfo that understands some additional protocols
 * that the system getsockname doesn't. */
int getsockname_ex(int socket, struct sockaddr *addr, socklen_t *addrlen)
{
	return getsockname(socket, addr, addrlen);
}
