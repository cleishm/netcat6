/*
 *  connection.c - connection establishment - implementation
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
#include "connection.h"
#include "attributes.h"
#include "afindep.h"
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

RCSID("@(#) $Header: /Users/cleishma/work/nc6-repo/nc6/src/connection.c,v 1.36 2009-04-18 11:39:35 chris Exp $");


/* cdata argument for the established callback */
typedef struct established_cdata {
	const connection_attributes_t *attrs;
	established_callback_t delegate_callback;
	void *callback_cdata;
} established_cdata_t;



static int net_connect(const connection_attributes_t *attrs,
		const struct addrinfo *hints,
		established_cdata_t established_cdata);
static int net_listen(const connection_attributes_t *attrs,
		const struct addrinfo *hints,
		established_cdata_t established_cdata);
static void established_calback(int fd, int socktype, void *cdata);
static void set_sockopt_handler(int sock, void *hdata);
static void warn_socket_details(const connection_attributes_t *attrs,
		int sock, int socktype);



int establish_connections(const connection_attributes_t *attrs,
		established_callback_t callback, void *cdata)
{
	established_cdata_t callback_data;
	struct addrinfo hints;

	assert(attrs != NULL);

	/* store connection attributes and original callback plus cdata */
	callback_data.attrs = attrs;
	callback_data.delegate_callback = callback;
	callback_data.callback_cdata = cdata;

	/* setup getaddrinfo hints */
	memset(&hints, 0, sizeof(hints));
	ca_to_addrinfo(&hints, attrs);

	/* establish connections */
	if (ca_is_flag_set(attrs, CA_PASSIVE)) {
		return net_listen(attrs, &hints, callback_data);
	} else {
		return net_connect(attrs, &hints, callback_data);
	} 

	/* not reached */
	abort();
}



static int net_connect(const connection_attributes_t *attrs,
		const struct addrinfo *hints,
		established_cdata_t established_cdata)
{
	const address_t *remote, *local;
	time_t timeout;
	int fd, socktype;

	/* get addresses */
	remote = ca_remote_address(attrs);
	local = ca_local_address(attrs);

	/* get timeout */
	timeout = ca_connect_timeout(attrs);

	/* invoke the appropriate connector for the protocol family */
	switch (ca_family(attrs)) {
#ifdef ENABLE_BLUEZ
	case PF_BLUETOOTH:
		fd = bluez_connect(*hints,
				remote->nodename, remote->service,
				set_sockopt_handler, &attrs,
				timeout, &socktype);
		break;
#endif/*ENABLE_BLUEZ*/
	default:
		fd = afindep_connect(*hints,
				remote->nodename, remote->service,
				local->nodename, local->service,
				set_sockopt_handler, &attrs,
				timeout, &socktype);
		break;
	}

	/* return errors immediately */
	if (fd < 0)
		return fd;

	/* only support a single connect */
	established_calback(fd, socktype, &established_cdata);
	return 0;
}



static int net_listen(const connection_attributes_t *attrs,
		const struct addrinfo *hints,
		established_cdata_t established_cdata)
{
	const address_t *remote, *local;
	time_t timeout;
	int max_accept;

	/* get addresses */
	remote = ca_remote_address(attrs);
	local = ca_local_address(attrs);

	/* get timeout */
	timeout = ca_connect_timeout(attrs);

	/* get maximum accepted connection (currently either 1 or infinite) */
	max_accept = ca_is_flag_set(attrs, CA_CONTINUOUS_ACCEPT)? -1 : 1;

	/* invoke the appropriate listener for the protocol family */
	switch (ca_family(attrs)) {
#ifdef ENABLE_BLUEZ
	case PF_BLUETOOTH:
		return bluez_listener(*hints,
				local->nodename, local->service,
				remote->nodename, remote->service,
				set_sockopt_handler, &attrs,
				established_calback, &established_cdata,
				timeout, max_accept);
#endif/*ENABLE_BLUEZ*/
	default:
		return afindep_listener(*hints,
				local->nodename, local->service,
				remote->nodename, remote->service,
				set_sockopt_handler, &attrs,
				established_calback, &established_cdata,
				timeout, max_accept);
	}

	/* never reached */
	abort();
}



/* callback when connection is established */
static void established_calback(int fd, int socktype, void *cdata)
{
	established_cdata_t *established_cdata = (established_cdata_t *)cdata;
	const connection_attributes_t *attrs;

	assert(established_cdata != NULL);
	assert(fd >= 0);
	assert(socktype >= 0);

	attrs = established_cdata->attrs;

	if (verbose_mode())
		warn_socket_details(attrs, fd, socktype);

	if (established_cdata->delegate_callback != NULL) {
		established_cdata->delegate_callback(attrs, fd, socktype,
				established_cdata->callback_cdata);
	}
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
