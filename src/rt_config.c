/*
 *  rt_config.c - runtime configuration module - implementation
 * 
 *  nc6 - an advanced netcat clone
 *  Copyright (C) 2001-2002 Mauro Tortonesi <mauro _at_ deepspace6.net>
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
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include "rt_config.h"
#include "netsupport.h"

#define MAX_IP_ADDR_STR_LEN	sizeof("1234:5678:90AB:CDEF:1234:5678:255.255.255.255")
#define MAX_SERVICE_STR_LEN	80

#if defined(ENABLE_IPV6)
bool is_ipv6_enabled(void)
{
	int fd;
	bool ret = TRUE;
	
	fd = socket(PF_INET6, SOCK_STREAM, 0);
	
	if (fd < 0) {
		ret = FALSE; 
	} else {
		close(fd);
	}

	return ret;
}
#endif



bool getaddinfo_supports_flag(int flag)
{
	int err;
	struct addrinfo hints, *res;
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags    = flag;
	
	err = getaddrinfo(NULL, "21", &hints, &res);
	if (err != 0) 
		return FALSE;
	
	freeaddrinfo(res);
	return TRUE;
}



struct results {
	sa_family_t family;
	const char *address;
	const char *service;
	int number;
};



static bool check_gai_results(const char *address, const char *service,
                              int flags, struct results *exp_res, int num)
{
	int i, err;
	struct addrinfo hints, *res = NULL, *ptr;
	char addr[MAX_IP_ADDR_STR_LEN + 1];
	char serv[MAX_SERVICE_STR_LEN + 1];

	/* verify preconditions */
	assert((address != NULL) || (service != NULL));
	assert(exp_res != NULL);
	assert(num > 0);
	
	for (i = 0; i < num; ++i) {
		exp_res[i].number = 0;
	}
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_UNSPEC;
	hints.ai_flags    = flags;
	hints.ai_socktype = SOCK_STREAM;
	
	if ((err = getaddrinfo(address, service, &hints, &res)) != 0) {
		goto exit;
	}
	
	for (ptr = res; ptr != NULL; ptr = ptr->ai_next) {
		bool found = FALSE;
		
		assert(ptr->ai_addr != NULL);
		assert(ptr->ai_addrlen != 0);
		
		if (getnameinfo(ptr->ai_addr, ptr->ai_addrlen,
		                addr, sizeof(addr), serv, sizeof(serv),
                                NI_NUMERICHOST | NI_NUMERICSERV) != 0) {
			/* this should never happen */
			goto exit;
		}
		
		switch (ptr->ai_family) {
#if defined(ENABLE_IPV6) && defined(AF_INET6)
		case AF_INET6:
#endif  	
		case AF_INET:
			for (i = 0; i < num; ++i) {
				if ((exp_res[i].family == ptr->ai_family) &&
				    (strcmp(exp_res[i].address, addr) == 0) && 
				    (strcmp(exp_res[i].service, serv) == 0)) {
					++exp_res[i].number;
					found = TRUE;
					break;
				}
			}
			if (found == FALSE) goto exit;
			break;
		case AF_UNSPEC:
			goto exit;
		default:
			/* another family support? is this an error? */
			break;
		}
	}

	freeaddrinfo(res);
	
	for (i = 0; i < num; ++i) {
		if (exp_res[i].number != 1) return FALSE;
	}

	return TRUE;

exit:
#if 0
	warn("buggy getaddrinfo/getnameinfo implementation");
#endif
	freeaddrinfo(res);
	return FALSE;
}



bool is_getaddinfo_sane(void)
{
	const char *service = "80";
	struct results passive[] = { 
#if defined(ENABLE_IPV6) && defined(AF_INET6)
		{ AF_INET6, "::", service, 0}, 
#endif  	
		{ AF_INET, "0.0.0.0", service, 0} };
	struct results active[] = { 
#if defined(ENABLE_IPV6) && defined(AF_INET6)
		{ AF_INET6, "::1", service, 0}, 
#endif  	
		{ AF_INET, "127.0.0.1", service, 0} };
	int numactive = sizeof(active)/sizeof(active[0]);
	int numpassive = sizeof(passive)/sizeof(passive[0]);
	
	/* check for conflicts */
	if((
#ifdef HAVE_GETADDRINFO_AI_ADDRCONFIG
            AI_ADDRCONFIG & 
#endif
#ifdef HAVE_GETADDRINFO_AI_ALL
	    AI_ALL & 
#endif
#ifdef HAVE_GETADDRINFO_AI_V4MAPPED
	    AI_V4MAPPED & 
#endif
	    AI_PASSIVE & AI_CANONNAME & AI_NUMERICHOST) != 0) return FALSE;

	/* check results */
	if ((check_gai_results(NULL, service, 0, active, numactive) == TRUE) &&
	    (check_gai_results(NULL, service, AI_PASSIVE, passive, numpassive) == TRUE)) 
		return TRUE;

	return FALSE;
}



bool is_double_binding_sane(void)
{
	int err, port;
	struct addrinfo hints, *res = NULL, *ptr;
	
	if (is_getaddinfo_sane() == FALSE) 
		return FALSE;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_UNSPEC;
	hints.ai_flags    = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM;
	
	port = 1025;
	
	for (;;) {
		int i;
		struct bound_socket_t* list = NULL;
		char service[80];

		snprintf(service, sizeof(service), "%d", port);
		service[sizeof(service) - 1] = '\0'; 

		err = getaddrinfo(NULL, service, &hints, &res);
		if (err != 0) {
			fatal("getaddinfo failed: %s", gai_strerror(err));
		}
	
		assert(res != NULL);

		for (i = 1, ptr = res; ptr != NULL; ptr=ptr->ai_next, ++i) {
			int fd;
		
			fd = socket(ptr->ai_family, ptr->ai_socktype, 
				    ptr->ai_protocol);
			if (fd < 0) {
				/* ignore this address if it is not supported */
	                        if (unsupported_sock_error(errno))
	                                continue;
	                        fatal("cannot create the socket: %s", 
				      strerror(errno));
			}
			
#if defined(ENABLE_IPV6) && defined(IPV6_V6ONLY)
			if (ptr->ai_family == PF_INET6) {
				int on = 1;
				/* in case of error, we will go on anyway... */
				err = setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY,
				                 &on, sizeof(on));
				if (err < 0) 
					perror("error with sockopt IPV6_V6ONLY");
			}
#endif 
			
			err = bind(fd, ptr->ai_addr, ptr->ai_addrlen);
			if (err < 0) {
				fprintf(stderr, "bind returned %d, (%s)\n", 
				        err, strerror(errno));
				if (errno == EADDRINUSE) {
					close_and_destroy_bound_sockets(list);
					list = NULL;
					if (i != 1) {
						return FALSE;
					} else {
						/* another program already 
						 * was already bound to the 
						 * port. try another port. */
						++port;
						continue;
					}
				}
	                        fatal("cannot bind to the socket: %s", 
				      strerror(errno));
			}
			list = add_bound_socket(list, fd, ptr->ai_socktype);
		}
		
		close_and_destroy_bound_sockets(list);
		break;	
	}

	return TRUE;
}


#if 0
int main()
{
	printf("getaddrinfo is %s\n", 
	       (is_getaddinfo_sane() == TRUE ? "sane": "__NOT__ sane"));
	printf("getaddrinfo %s AI_ADDRCONFIG flag\n", 
	       (getaddinfo_supports_ai_addrconfig() == TRUE ? "supports": 
		"does __NOT__ support"));
	printf("IPv6 is %s\n", 
	       (is_ipv6_enabled() == TRUE ? "enabled": "__NOT__ enabled"));
	printf("double binding is %s\n", 
	       (is_double_binding_sane() == TRUE ? "sane": "__NOT__ sane"));

	return 0;
}



const char *get_program_name(void)
{
	return "test";
}
#endif
