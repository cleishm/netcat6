/*
 *  afindep.c - address family independant networking functions
 * 
 *  nc6 - an advanced netcat clone
 *  Copyright (C) 2001-2005 Mauro Tortonesi <mauro _at_ deepspace6.net>
 *  Copyright (C) 2002-2005 Chris Leishman <chris _at_ leishman.org>
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
#include "afindep.h"
#include "misc.h"
#include "netsupport.h"
#include "parser.h"

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <limits.h>

RCSID("@(#) $Header: /Users/cleishma/work/nc6-repo/nc6/src/afindep.c,v 1.1 2005-08-18 04:05:58 chris Exp $");


/* suggested size for argument to getnameinfo_ex */
static const int AI_STR_SIZE = (2 * (NI_MAXHOST + NI_MAXSERV + 2)) + 8;


static bool skip_address(const struct addrinfo *ai);
#ifdef ENABLE_IPV6
static struct addrinfo *order_ipv6_first(struct addrinfo *ai);
#endif
static void getnameinfo_ex(const struct sockaddr *sa, socklen_t len, char *str,
		size_t size, bool numeric_mode);
static bool is_allowed(const struct sockaddr *sa, socklen_t salen,
		const struct addrinfo *hints,
		const char *address, const char *service);



int afindep_connect(const struct addrinfo *hints,
		const char *remote_address, const char *remote_service,
		const char *local_address, const char *local_service,
		set_sockopt_handler_t set_sockopt_handler, void *hdata,
		time_t timeout, int *rt_socktype)
{
	int err, fd = -1;
	struct addrinfo *res = NULL, *ptr;
	bool connect_attempted = false;
	char name_buf[AI_STR_SIZE];

	/* make sure arguments are valid and preconditions are respected */
	assert(hints != NULL);
	assert(remote_address != NULL && strlen(remote_address) > 0);
	assert(remote_service != NULL && strlen(remote_service) > 0);
	assert(local_address == NULL || strlen(local_address) > 0);
	assert(local_service == NULL || strlen(local_service) > 0);

	/* get the address of the remote end of the connection */
	err = getaddrinfo(remote_address, remote_service, hints, &res);
	if (err != 0) {
		warning(_("forward host lookup failed "
		        "for remote endpoint %s: %s"),
		        remote_address, gai_strerror(err));
		return -1;
	}

	/* check the results of getaddrinfo */
	assert(res != NULL);

	/* try connecting to any of the addresses returned by getaddrinfo */
	for (ptr = res; ptr != NULL; ptr = ptr->ai_next) {

		/* only accept results we can handle */
		if (skip_address(ptr) == true) continue;

		/* we are going to try to connect to this address */
		connect_attempted = true;

		/* create the socket */
		fd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (fd < 0) {
			/* ignore this address if it is not supported */
			if (!unsupported_sock_error(errno))
				continue;
			warning("cannot create the socket: %s",
				strerror(errno));
			return -1;
		}
		
#if defined(ENABLE_IPV6) && defined(IPV6_V6ONLY)
		if (ptr->ai_family == PF_INET6) {
			int on = 1;
			/* in case of error, we will go on anyway... */
			err = setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY,
			                 &on, sizeof(on));
			if (err < 0) 
				warning("error with sockopt IPV6_V6ONLY");
		}
#endif 

		if (set_sockopt_handler != NULL)
			set_sockopt_handler(fd, hdata);

		/* setup name_buf if we're in verbose mode */
		if (verbose_mode())
			getnameinfo_ex(ptr->ai_addr, ptr->ai_addrlen, 
			               name_buf, sizeof(name_buf),
				       (hints->ai_flags & AI_NUMERICHOST));
					
		/* setup local source address and/or service */
		if (local_address != NULL || local_service != NULL) {
			struct addrinfo src_hints, *src_res = NULL, *src_ptr;
		
			/* setup hints structure to be passed to getaddrinfo */
			memset(&src_hints, 0, sizeof(src_hints));
			src_hints.ai_family   = ptr->ai_family;
			src_hints.ai_flags    = AI_PASSIVE;
			src_hints.ai_socktype = ptr->ai_socktype;
			src_hints.ai_protocol = ptr->ai_protocol;
			src_hints.ai_flags    = hints->ai_flags;

			/* get the local IP address of the connection */
			err = getaddrinfo(local_address, local_service,
			                  &src_hints, &src_res);
			if (err != 0) {
				if (verbose_mode()) {
					warning(_("bind to source addr/port "
					     "failed when connecting to "
					     "%s: %s"), name_buf,
					     gai_strerror(err));
				}
				close(fd);
				fd = -1;
				continue;
			}

			/* check the results of getaddrinfo */
			assert(src_res != NULL);

			/* try binding to any of the addresses */
			for (src_ptr = src_res; src_ptr != NULL;
			     src_ptr = src_ptr->ai_next)
			{
				err = bind(fd, src_ptr->ai_addr,
					   src_ptr->ai_addrlen);
				if (err == 0) break;
			}
			
			if (err != 0) {
				/* make sure we have tried all addresses */
				assert(src_ptr == NULL);
				
				if (verbose_mode()) {
					warning(_("bind to source addr/port "
					     "failed when connecting to "
					     "%s: %s"), name_buf,
					     strerror(errno));
				}
				freeaddrinfo(src_res);
				close(fd);
				fd = -1;
				continue;
			}

			freeaddrinfo(src_res);
		}

		/* attempt the connection */
		err = connect_with_timeout(fd,
				ptr->ai_addr, ptr->ai_addrlen, timeout);

		/* exit from the loop if we have a valid connection */
		if (err == 0)
			break;

		/* check error code */
		if (verbose_mode()) {
			/* use different error message for timeout */
			if (errno == ETIMEDOUT) {
				/* connection timed out */
				warning(_("timeout while connecting to %s"),
				        name_buf);
			}
			else {
				/* connection failed */
				warning(_("cannot connect to %s: %s"),
				        name_buf, strerror(errno));
			}
		}
		close(fd);
		fd = -1;
	}

	/* either all possibilities were exahusted, or a connection was made */
	assert(ptr == NULL || fd >= 0);
	
	/* if the connection failed, output an error message */
	if (ptr == NULL) {
		/* if a connection was attempted, an error has been output */
		if (connect_attempted == false) {
			warning(_("forward lookup returned "
			        "no useful socket types"));
		} else {
			warning(_("unable to connect "
			        "to address %s, service %s"), 
			        remote_address, remote_service);
		}
		return -1;
	}

	/* let the user know the connection has been established */
	if (verbose_mode())
		warning(_("%s open"), name_buf);

	/* return the socktype */
	if (rt_socktype != NULL)
		*rt_socktype = ptr->ai_socktype;

	/* cleanup addrinfo structure */
	freeaddrinfo(res);

	return fd;
}



int afindep_listener(const struct addrinfo *hints,
		const char *local_address, const char *local_service,
		const char *remote_address, const char *remote_service,
		set_sockopt_handler_t set_sockopt_handler, void *hdata,
		listen_callback_t callback, void *cdata,
		time_t timeout, int max_accept)
{
	int nfd, i, fd, err, maxfd = -1;
	struct addrinfo *res = NULL, *ptr;
#ifdef ENABLE_IPV6
	bool set_ipv6_only = false;
	bool bound_ipv6_any = false;
#endif
	bound_socket_t *bound_sockets = NULL;
	fd_set accept_fdset;
	char name_buf[AI_STR_SIZE];

	/* make sure arguments are valid and preconditions are respected */
	assert(hints != NULL);
	assert(remote_address == NULL || strlen(remote_address) > 0);
	assert(remote_service == NULL || strlen(remote_service) > 0);
	assert(local_address == NULL || strlen(local_address) > 0);
	assert(local_service != NULL && strlen(local_service) > 0);

	/* if max_accept is 0, just return */
	if (max_accept == 0)
		return 0;

	/* get the IP address of the local end of the connection */
	err = getaddrinfo(local_address, local_service, hints, &res);
	if (err != 0) {
		warning(_("forward host lookup failed "
		        "for local endpoint %s (%s): %s"),
		        local_address? local_address : _("[unspecified]"),
		        local_service, gai_strerror(err));
		return -1;
	}
		
	/* check the results of getaddrinfo */
	assert(res != NULL);

#ifdef ENABLE_IPV6
	/* 
	 * Some systems (notably Linux) with a shared stack for ipv6 and ipv4, 
	 * have a broken bind(2) implementation. In these systems, binding an 
	 * ipv6 socket to ipv6_addr_any (::) will also bind the socket to
	 * the ipv4 unspecified address (0.0.0.0).
	 * 
	 * So when listening on ipv6_addr_any (::) accept(2) will return also
	 * ipv4 connection attemptes, as ipv4 mapped ipv6 addresses.
	 *
	 * This is a problem, since when called with AF_UNSPEC and AI_PASSIVE, 
	 * getaddrinfo will return results for both ipv6 AND ipv4, but if we 
	 * try to bind on the ipv4 unspecified address (0.0.0.0) AFTER we have 
	 * bound our socket to ipv6_addr_any (::), the syscall bind(2) will 
	 * fail and return -1, setting errno to EADDRINUSE.
	 *
	 * The following algorithm is used to work around this error to some
	 * degree:
	 *
	 * Ensure binds to IPv6 addresses are attempted before IPv4 addresses.
	 * On broken systems where IPV6_V6ONLY is not defined or the setsockopt
	 * fails:
	 * 
	 *   - Keep track of whether an IPv6 socket has been bound to 
	 *     in6_addr_any.
	 *   - If a bind to IPv4 fails with EADDRINUSE and an IPv6 socket
	 *     has been bound, then just ignore the error.
	 */

	/* 
	 * TODO: instead of reordering the results, just loop through the
	 * results twice - once for ipv6 then for the rest.
	 */
	res = order_ipv6_first(res);
#endif

	/* initialize accept_fdset */
	FD_ZERO(&accept_fdset);
	
	/* try binding to all of the addresses returned by getaddrinfo */
	nfd = 0;
	for (ptr = res; ptr != NULL; ptr = ptr->ai_next) {

		/* only accept results we can handle */
		if (skip_address(ptr) == true) continue;
		
		/* create the socket */
		fd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (fd < 0) {
			/* ignore this address if it is not supported */
			if (unsupported_sock_error(errno))
				continue;
			warning("cannot create the socket: %s",
			        strerror(errno));
			return -1;
		}

#if defined(ENABLE_IPV6) && defined(IPV6_V6ONLY)
		if (ptr->ai_family == PF_INET6) {
			int on = 1;
			/* in case of error, we will go on anyway... */
			err = setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY,
			                 &on, sizeof(on));
			if (err < 0)
				warning("error with sockopt IPV6_V6ONLY");
			else
				set_ipv6_only = true;
		}
#endif 

		if (set_sockopt_handler != NULL)
			set_sockopt_handler(fd, hdata);

		/* get the numeric name for this source address */
		getnameinfo_ex(ptr->ai_addr, ptr->ai_addrlen, name_buf, 
		               sizeof(name_buf), true);

		/* bind to the local address */
		err = bind(fd, ptr->ai_addr, ptr->ai_addrlen);
		if (err != 0) {
#ifdef ENABLE_IPV6
			/* suppress ADDRINUSE error for systems that double
			 * bind ipv6 and ipv4 and pretend it succeeded */
			if (errno == EADDRINUSE &&
			    ptr->ai_family == PF_INET &&
			    set_ipv6_only == false &&
			    bound_ipv6_any == true)
			{
				if (verbose_mode())
					warning(_("listening on %s ..."),
						name_buf);
				close(fd);
				continue;
			}
#endif
			warning(_("bind to source %s failed: %s"),
			        name_buf, strerror(errno));
			close(fd);
			continue;
		}

		if (ptr->ai_socktype == SOCK_STREAM) {
			err = listen(fd, SOMAXCONN);
			if (err != 0) {
				warning(_("cannot listen on %s: %s"),
				        name_buf, strerror(errno));
				free_bound_sockets(bound_sockets);
				return -1;
			}
		}

		if (verbose_mode())
			warning(_("listening on %s ..."), name_buf);

#ifdef ENABLE_IPV6
		/* check if this was an IPv6 socket bound to IN6_ADDR_ANY */
		if (ptr->ai_family == PF_INET6 &&
		    memcmp(&((struct sockaddr_in6 *)(ptr->ai_addr))->sin6_addr,
		           &in6addr_any, sizeof(struct in6_addr)) == 0)
		{
			bound_ipv6_any = true;
		}
#endif

		/* add fd to bound_sockets (just add to the head of the list) */
		bound_sockets =	add_bound_socket(bound_sockets, fd, 
				                 ptr->ai_socktype);

		/* add fd to accept_fdset */
		FD_SET(fd, &accept_fdset);
		maxfd = MAX(maxfd, fd);
		nfd++;
	}

	freeaddrinfo(res);
	
	if (nfd == 0) {
		warning(_("failed to bind to any local addr/port"));
		return -1;
	}

	/* enter into the accept loop */
 	for (;;) {
		fd_set tmp_ap_fdset;
		struct timeval tv, *tvp = NULL;
		struct sockaddr_storage dest;
		socklen_t destlen;
		int ns, socktype;
		char c_name_buf[AI_STR_SIZE];

		/* make a copy of accept_fdset before passing to select */
		memcpy(&tmp_ap_fdset, &accept_fdset, sizeof(fd_set));

		/* setup timeout */
		if (timeout > 0) {
			tv.tv_sec = timeout;
			tv.tv_usec = 0;
			tvp = &tv;
		}

		/* wait for an incoming connection */
		err = select(maxfd + 1, &tmp_ap_fdset, NULL, NULL, tvp);

		if (err <= 0) {
			if (err < 0 && errno == EINTR)
				continue;
			if (err == 0)
				warning(_("connection timed out"));
			else
				warning("select error: %s", strerror(errno));
			free_bound_sockets(bound_sockets);
			return -1;
		}
		
		/* find the ready filedescriptor */
		for (fd = 0; fd <= maxfd && !FD_ISSET(fd, &tmp_ap_fdset); ++fd)
			;

		/* if none were ready, loop to select again */
		if (fd > maxfd)
			continue;

		/* find socket type in bound_sockets */
		socktype = get_bound_socket_type(bound_sockets, fd);

		destlen = sizeof(dest);	

		/* for stream sockets we accept a new connection, whereas for
		 * dgram sockets we use MSG_PEEK to determine the sender */
		if (socktype == SOCK_STREAM) {
			ns = accept(fd, (struct sockaddr *)&dest, &destlen);
			if (ns < 0) {
				warning("accept failed: %s", strerror(errno));
				free_bound_sockets(bound_sockets);
				return -1;
			}
		} else {
			/* this is checked when binding listen sockets */
			assert(socktype == SOCK_DGRAM);

			err = recvfrom(fd, NULL, 0, MSG_PEEK,
			               (struct sockaddr *)&dest, &destlen);
			if (err < 0) {
				warning("recvfrom failed: %s", strerror(errno));
				free_bound_sockets(bound_sockets);
				return -1;
			}

			ns = dup(fd);
			if (ns < 0) {
				warning("dup failed: %s", strerror(errno));
				free_bound_sockets(bound_sockets);
				return -1;
			}
		}

		/* get names for each end of the connection */
		if (verbose_mode()) {
			struct sockaddr_storage src;
			socklen_t srclen = sizeof(src);

			/* find out what address the connection was to */
			err = getsockname(ns, (struct sockaddr *)&src, &srclen);
			if (err < 0) {
				warning("getsockname failed: %s",
				        strerror(errno));
				free_bound_sockets(bound_sockets);
				return -1;
			}

			/* get the numeric name for this source */
			getnameinfo_ex((struct sockaddr *)&src, srclen,
			               name_buf, sizeof(name_buf), true);

			/* get the name for this client */
			getnameinfo_ex((struct sockaddr *)&dest, destlen,
			               c_name_buf, sizeof(c_name_buf),
				       (hints->ai_flags & AI_NUMERICHOST));
		}

		/* check if connections from this client are allowed */
		if ((remote_address == NULL && remote_service == NULL) ||
		    (is_allowed((struct sockaddr *)&dest, destlen, hints,
				remote_address, remote_service) == true))
		{

			if (socktype == SOCK_DGRAM) {
				/* connect the socket to ensure we only talk
				 * with this client */
				err = connect(ns, (struct sockaddr *)&dest, 
				              destlen);
				if (err != 0) {
					warning(_("connect failed on "
					        "datagram socket: %s"),
					        strerror(errno));
					free_bound_sockets(bound_sockets);
					return -1;
				}
			}

			if (verbose_mode()) {
				warning(_("connect to %s from %s"),
				        name_buf, c_name_buf);
			}

			callback(ns, socktype, cdata);

			if (max_accept > 0 && --max_accept == 0)
				break;
		} else {
			if (socktype == SOCK_DGRAM) {
				/* the connection wasn't accepted - 
				 * remove the queued packet */
				recvfrom(ns, NULL, 0, 0, NULL, 0);
			}
			close(ns);

			if (verbose_mode()) {
				warning(_("refused connect to %s from %s"),
				        name_buf, c_name_buf);
			}
		}
	}

	/* close the listening sockets */
	for (i = 0; i <= maxfd; ++i) {
		if (FD_ISSET(i, &accept_fdset) != 0) close(i);
	}

	/* free the bound_socket list */
	free_bound_sockets(bound_sockets);
	return 0;
}



static bool skip_address(const struct addrinfo *ai)
{
	/* check arguments */
	assert(ai != NULL);
	
	/* only use socktypes we can handle */
	if (ai->ai_socktype != SOCK_STREAM &&
	    ai->ai_socktype != SOCK_DGRAM) 
		return true;

#ifdef ENABLE_IPV6
	/* 
	 * skip IPv4 mapped addresses returned from getaddrinfo,
	 * for security reasons. see the documents:
	 *
	 * http://playground.iijlab.net/i-d/
	 *       /draft-itojun-ipv6-transition-abuse-01.txt
	 *       
	 * http://playground.iijlab.net/i-d/
	 *       /draft-cmetz-v6ops-v4mapped-api-harmful-00.txt
	 *       
	 * http://playground.iijlab.net/i-d/
	 *       /draft-itojun-v6ops-v4mapped-harmful-01.txt
	 */
	if (is_address_ipv4_mapped(ai->ai_addr))
		return true;
#else
#ifdef PF_INET6
	/* skip IPv6 if disabled */
	if (ai->ai_family == PF_INET6)
		return true;
#endif
#endif
	
	return false;
}



/* 
 * Some systems (notably Linux) will bind to both IPv6 AND IPv4 when
 * listening.  Connections will still be accepted from either IPv6 or
 * IPv4 clients (IPv4 will be mapped into IPv6).  However, this means
 * that we MUST bind the IPv6 address ONLY for these hosts.
 *
 * To handle this, we will ensure that IPv6 sockets are bound first and then
 * attempt to bind the IPv4 ones.  On systems that double bind IPv6/IPv4 the
 * IPv4 bind will simply fail.  This function does the reordering - moving all
 * IPv6 addresses to the start of the getaddrinfo results.
 */
#ifdef ENABLE_IPV6
static struct addrinfo *order_ipv6_first(struct addrinfo *ai)
{
	struct addrinfo *ptr;
	struct addrinfo *lastv6 = NULL;
	struct addrinfo *tmp;

	assert(ai != NULL);

	/* Move all IPv6 addresses to the start of the list - keeping
	 * them in the original order. */

	if (ai->ai_family == PF_INET6)
		lastv6 = ai;

	for (ptr = ai; ptr != NULL && ptr->ai_next != NULL; ptr = ptr->ai_next){
		if (ptr->ai_next->ai_family == PF_INET6) {
			tmp = ptr->ai_next;
			ptr->ai_next = tmp->ai_next;
			if (lastv6) {
				tmp->ai_next = lastv6->ai_next;
				lastv6->ai_next = tmp;
			} else {
				tmp->ai_next = ai;
				ai = tmp;
			}
			lastv6 = tmp;
		}
	}

	return ai;
}
#endif



static void getnameinfo_ex(const struct sockaddr *sa, socklen_t len, char *str,
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

	/* this should never happen */
	if (err != 0)
		fatal("getnameinfo failed: %s", gai_strerror(err));

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
}



/* returns true if sa corresponds to the address/port specified in addr */
static bool is_allowed(const struct sockaddr *sa, socklen_t salen,
		const struct addrinfo *hints,
		const char *address, const char *service)
{
	struct addrinfo *res = NULL, *ptr;
	int err;
	bool ret;

	assert(sa != NULL);
	assert(hints != NULL);
	assert(address == NULL || strlen(address) > 0);
	assert(service == NULL || strlen(service) > 0);

	/* if no address or service is supplied, match everything */
	if (address == NULL && service == NULL) return true;
		
	err = getaddrinfo(address, service, hints, &res);
	if (err != 0) {
		/* some errors just indicate that the address wasn't suitable */
		switch (err) {
#ifdef HAVE_GETADDRINFO_EAI_NODATA 
  		case EAI_NODATA:
#endif
#ifdef HAVE_GETADDRINFO_EAI_ADDRFAMILY
  		case EAI_ADDRFAMILY:
#endif
  		case EAI_FAMILY:
		case EAI_SERVICE:
		case EAI_SOCKTYPE:
			return false;
		default:
			fatal("getaddrinfo error: %s", gai_strerror(err));
		}
	}

	ret = false;
	
	for (ptr = res; ptr != NULL; ptr = ptr->ai_next) {

#ifdef ENABLE_IPV6
		/* skip IPv4 mapped addresses returned from getaddrinfo */
		if (is_address_ipv4_mapped(ptr->ai_addr))
			continue;
#endif

		if (sockaddr_compare(sa, salen,
				     ptr->ai_addr, ptr->ai_addrlen) == true)
		{
			ret = true;
			break;
		}
	}

	freeaddrinfo(res);

	return ret;
}
