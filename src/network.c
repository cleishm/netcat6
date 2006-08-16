/*
 *  network.c - common networking functions module - implementation
 * 
 *  nc6 - an advanced netcat clone
 *  Copyright (C) 2001-2006 Mauro Tortonesi <mauro _at_ deepspace6.net>
 *  Copyright (C) 2002-2006 Chris Leishman <chris _at_ leishman.org>
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
#include "network.h"
#include "connection.h"
#include "afindep.h"
#include "parser.h"
#ifdef ENABLE_BLUEZ
#include "bluez.h"
#endif/*ENABLE_BLUEZ*/

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

RCSID("@(#) $Header: /Users/cleishma/work/nc6-repo/nc6/src/network.c,v 1.60 2006-08-16 09:56:11 mauro Exp $");


/* cddata argument for the listen callback proxy */
typedef struct callback_proxy_data {
	const connection_attributes_t *attrs;
	established_callback_t callback;
	void *cdata;
} callback_proxy_data_t;



static int net_connect(const connection_attributes_t *attrs,
		established_callback_t callback, void *cdata);
static int net_listen(const connection_attributes_t *attrs,
		established_callback_t callback, void *cdata);
static void callback_proxy(int fd, int socktype, void *cdata);
static void set_sockopt_handler(int sock, void *hdata);
static void warn_socket_details(const connection_attributes_t *attrs,
		int sock, int socktype);



int net_establish(const connection_attributes_t *attrs,
		established_callback_t callback, void *cdata)
{
	/* establish remote connection */
	if (ca_is_flag_set(attrs, CA_CONNECT_MODE)) {
		return net_connect(attrs, callback, cdata);
	} else if (ca_is_flag_set(attrs, CA_LISTEN_MODE)) {
		return net_listen(attrs, callback, cdata);
	} else {
		fatal_internal("unknown connection mode");
	}

	/* not reached */
	abort();
}



static int net_connect(const connection_attributes_t *attrs,
		established_callback_t callback, void *cdata)
{
	struct addrinfo hints;
	const address_t *remote, *local;
	time_t timeout;
	int fd, socktype;
	callback_proxy_data_t proxy_data;

	assert(attrs != NULL);

	/* setup getaddrinfo hints */
	memset(&hints, 0, sizeof(hints));
	ca_to_addrinfo(&hints, attrs);
#ifdef HAVE_GETADDRINFO_AI_ADDRCONFIG
	/* make calls to getaddrinfo send AAAA queries only if at least one
	 * IPv6 interface is configured */
	hints.ai_flags |= AI_ADDRCONFIG;
#endif
	if (ca_is_flag_set(attrs, CA_NUMERIC_MODE))
		hints.ai_flags |= AI_NUMERICHOST;

	/* get addresses */
	remote = ca_remote_address(attrs);
	local = ca_local_address(attrs);

	/* get timeout */
	timeout = ca_connect_timeout(attrs);

	/* store requested callback and cdata */
	proxy_data.attrs = attrs;
	proxy_data.callback = callback;
	proxy_data.cdata = cdata;

	/* invoke the appropriate connector for the protocol family */
	switch (ca_family(attrs)) {
#ifdef ENABLE_BLUEZ
	case PROTO_BLUEZ:
		fd = bluez_connect(&hints,
				remote->address, remote->service,
				set_sockopt_handler, &attrs,
				timeout, &socktype);
		break;
#endif/*ENABLE_BLUEZ*/
	default:
		fd = afindep_connect(&hints,
				remote->address, remote->service,
				local->address, local->service,
				set_sockopt_handler, &attrs,
				timeout, &socktype);
		break;
	}

	/* return errors immediately */
	if (fd < 0)
		return fd;

	/* only support a single connect, so issue callback directly */
	callback_proxy(fd, socktype, &proxy_data);
	return 0;
}



static int net_listen(const connection_attributes_t *attrs,
		established_callback_t callback, void *cdata)
{
	struct addrinfo hints;
	const address_t *remote, *local;
	time_t timeout;
	int max_accept;
	callback_proxy_data_t proxy_data;

	/* make sure that attrs is a valid pointer */
	assert(attrs != NULL);

	/* setup getaddrinfo hints */
	memset(&hints, 0, sizeof(hints));
	ca_to_addrinfo(&hints, attrs);
	hints.ai_flags = AI_PASSIVE;
#ifdef HAVE_GETADDRINFO_AI_ADDRCONFIG
	hints.ai_flags |= AI_ADDRCONFIG;
#endif
	if (ca_is_flag_set(attrs, CA_NUMERIC_MODE))
		hints.ai_flags |= AI_NUMERICHOST;

	/* get addresses */
	remote = ca_remote_address(attrs);
	local = ca_local_address(attrs);

	/* get timeout */
	timeout = ca_connect_timeout(attrs);

	/* get maximum accepted connection (currently either 1 or infinite) */
	max_accept = ca_is_flag_set(attrs, CA_CONTINUOUS_ACCEPT)? -1 : 1;

	/* store requested callback and cdata */
	proxy_data.attrs = attrs;
	proxy_data.callback = callback;
	proxy_data.cdata = cdata;

	/* invoke the appropriate listener for the protocol family */
	switch (ca_family(attrs)) {
#ifdef ENABLE_BLUEZ
	case PROTO_BLUEZ:
		return bluez_listener(&hints,
				local->address, local->service,
				remote->address, remote->service,
				set_sockopt_handler, &attrs,
				callback_proxy, &proxy_data,
				timeout, max_accept);
#endif/*ENABLE_BLUEZ*/
	default:
		return afindep_listener(&hints,
				local->address, local->service,
				remote->address, remote->service,
				set_sockopt_handler, &attrs,
				callback_proxy, &proxy_data,
				timeout, max_accept);
	}

	/* never reached */
	abort();
}



/* proxy for the actual callback, to keep track of the connection attributes */
static void callback_proxy(int fd, int socktype, void *cdata)
{
	callback_proxy_data_t *proxy_data = (callback_proxy_data_t *)cdata;
	const connection_attributes_t *attrs;

	assert(proxy_data != NULL);
	assert(fd >= 0);
	assert(socktype >= 0);

	attrs = proxy_data->attrs;

	if (verbose_mode())
		warn_socket_details(attrs, fd, socktype);

	proxy_data->callback(attrs, fd, socktype, proxy_data->cdata);
}



/* handler function to set socket options on newly created sockets */
static void set_sockopt_handler(int sock, void *hdata)
{
	int on, err;
	const connection_attributes_t *attrs =
		*((const connection_attributes_t **)hdata);

	/* check arguments */
	assert(attrs != NULL);
	assert(sock >= 0);
	
	/* set the reuse address socket option */
	if (!ca_is_flag_set(attrs, CA_DONT_REUSE_ADDR)) {
		on = 1;
		/* in case of error, we will go on anyway... */
		err = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
				&on, sizeof(on));
		if (err < 0)
			warning("error with setsockopt SO_REUSEADDR: %s",
			    strerror(errno));
	}

	/* disable the nagle option for TCP sockets */
	if (ca_is_flag_set(attrs, CA_DISABLE_NAGLE)) {
		on = 1;
		/* in case of error, we will go on anyway... */
		err = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
				&on, sizeof(on));
		/* ignore error if this socket does not use TCP */
		if (err < 0 && errno != ENOPROTOOPT) {
			warning("error with setsockopt TCP_NODELAY: %s",
			    strerror(errno));
		}
	}

	/* setup the kernel sndbuf size */
	if ((on = ca_sndbuf_size(attrs)) > 0) {
		/* in case of error, we will go on anyway... */
		err = setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &on, sizeof(on));
		if (err < 0)
			warning("error with setsockopt SO_SNDBUF: %s",
			    strerror(errno));
	}

	/* setup the kernel rcvbuf size */
	if ((on = ca_rcvbuf_size(attrs)) > 0) {
		/* in case of error, we will go on anyway... */
		err = setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &on, sizeof(on));
		if (err < 0)
			warning("error with setsockopt SO_RCVBUF: %s",
			    strerror(errno));
	}
}



static void warn_socket_details(const connection_attributes_t *attrs,
		int sock, int socktype)
{
	int n;
	socklen_t nlen;

	/* check arguments */
	assert(attrs != NULL);
	assert(sock >= 0);
	
	/* announce the socket in very verbose mode */
	switch (socktype) {
	case SOCK_STREAM:
		warning(_("using stream socket"));
		break;
	case SOCK_DGRAM:
		warning(_("using datagram socket"));
		break;
	case SOCK_SEQPACKET:
		warning(_("using seqpacket socket"));
		break;
	default:
		fatal_internal("unsupported socket type %d", socktype);
	}

	/* announce the real sndbuf size */
	if (ca_sndbuf_size(attrs) > 0) {
		nlen = sizeof(n);
		if (getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &n, &nlen) < 0)
			warning("error with getsockopt SO_SNDBUF: %s",
			     strerror(errno));
		else
			warning(_("using socket sndbuf size of %d"), n);
	}

	/* announce the real rcvbuf size */
	if (ca_rcvbuf_size(attrs) > 0) {
		nlen = sizeof(n);
		if (getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &n, &nlen) < 0)
			warning("error with getsockopt SO_RCVBUF: %s",
			    strerror(errno));
		else
			warning(_("using socket rcvbuf size of %d"), n);
	}
}
