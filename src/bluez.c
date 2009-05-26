/*
 *  bluez.c - bluetooth networking functions module - implementation
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
#include "bluez.h"
#include "misc.h"
#include "netsupport.h"

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <limits.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/sco.h>
#include <bluetooth/l2cap.h>


/* suggested size for argument to getnameinfo_ex */
static const int BA_STR_SIZE = BA_MAXHOST + 14;


static int getbluezaddr(const char *address, const char *service,
		const struct addrinfo *hints, struct sockaddr *sa,
		socklen_t *salen);
static void getbluezname(const struct sockaddr *sa, int protocol,
		char *str, size_t size);
static bool is_allowed(const struct sockaddr *sa, socklen_t salen,
		const struct addrinfo *hints,
		const char *address, const char *service);



int bluez_connect(struct addrinfo hints,
		const char *remote_address, const char *remote_service,
		set_sockopt_handler_t set_sockopt_handler, void *hdata,
		time_t timeout, int *rt_socktype)
{
	int err, fd = -1;
	struct sockaddr_storage ss;
	socklen_t salen = 0;
	char name_buf[BA_STR_SIZE];

	/* make sure arguments are valid and preconditions are respected */
	assert(remote_address != NULL && strlen(remote_address) > 0);

	/* this function only supports a specific protocol family */
	assert(hints.ai_family == PF_BLUETOOTH);

	memset(&ss, 0, sizeof(ss));
	
	/* get the sockaddr */
	if (getbluezaddr(remote_address, remote_service, &hints,
	                 (struct sockaddr *)&ss, &salen))
	{
		return -1;
	}

	/* setup name_buf if we're in verbose mode */
	if (verbose_mode())
		getbluezname((struct sockaddr *)&ss, hints.ai_protocol,
		             name_buf, sizeof(name_buf));

	fd = socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);
	if (fd < 0) {
		warning("cannot create the bluez socket: %s", strerror(errno));
		return -1;
	}

	if (set_sockopt_handler != NULL)
		set_sockopt_handler(fd, hdata);

	err = connect_with_timeout(fd, (struct sockaddr *)&ss, salen, timeout);

	if (err != 0) {
		if (verbose_mode()) {
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
		return err;
	}

	*rt_socktype = hints.ai_socktype;
	return fd;
}



int bluez_listener(struct addrinfo hints,
		const char *local_address, const char *local_service,
		const char *remote_address, const char *remote_service,
		set_sockopt_handler_t set_sockopt_handler, void *hdata,
		listen_callback_t callback, void *cdata,
		time_t timeout, int max_accept)
{
	int fd = -1;
	struct sockaddr_storage ss;
	socklen_t salen = 0;
	char name_buf[BA_STR_SIZE];

	/* make sure arguments are valid and preconditions are respected */
	assert(local_address == NULL || strlen(local_address) > 0);
	assert(remote_address == NULL || strlen(remote_address) > 0);
	assert(callback != NULL);

	/* this function only supports a specific protocol family */
	assert(hints.ai_family == PF_BLUETOOTH);
	
	/* if max_accept is 0, just return */
	if (max_accept == 0)
		return 0;
	
	memset(&ss, 0, sizeof(ss));
	
	/* get the sockaddr */
	if (getbluezaddr(local_address, local_service, &hints,
	                 (struct sockaddr *)&ss, &salen))
	{
		return -1;
	}

	/* setup name_buf if we're in verbose mode */
	if (verbose_mode())
		getbluezname((struct sockaddr *)&ss, hints.ai_protocol,
		             name_buf, sizeof(name_buf));

	fd = socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);
	if (fd < 0) {
		warning("cannot create the bluez socket: %s", strerror(errno));
		return -1;
	}

	if (set_sockopt_handler != NULL)
		set_sockopt_handler(fd, hdata);

	if (bind(fd, (struct sockaddr *)&ss, salen) < 0) {
		warning(_("bind to source %s failed: %s"),
		        name_buf, strerror(errno));
		return -1;
	}

	if (listen(fd, SOMAXCONN) != 0) {
		warning(_("cannot listen on %s: %s"),
		        name_buf, strerror(errno));
	}

	if (verbose_mode())
		warning(_("listening on %s ..."), name_buf);
	
	/* enter into the accept loop */
 	for (;;) {
		fd_set accept_fdset;
		struct timeval tv, *tvp = NULL;
		struct sockaddr_storage dest;
		socklen_t destlen;
		int ns, err;
		char c_name_buf[BA_STR_SIZE];

		FD_ZERO(&accept_fdset);
		FD_SET(fd, &accept_fdset);

		/* setup timeout */
		if (timeout > 0) {
			tv.tv_sec = timeout;
			tv.tv_usec = 0;
			tvp = &tv;
		}

		/* wait for an incoming connection */
		err = select(fd + 1, &accept_fdset, NULL, NULL, tvp);

		if (err <= 0) {
			if (err < 0 && errno == EINTR)
				continue;
			if (err == 0)
				warning(_("connection timed out"));
			else
				warning("select error: %s", strerror(errno));
			return -1;
		}

		/* double check that the fd is actually set */
		if (!FD_ISSET(fd, &accept_fdset))
			continue;

		destlen = sizeof(dest);	
		ns = accept(fd, (struct sockaddr *)&dest, &destlen);
		if (ns < 0) {
			warning("accept failed: %s", strerror(errno));
			return -1;
		}

		/* get the name for this client */
		if (verbose_mode())
			getbluezname((struct sockaddr *)&dest,
			             hints.ai_protocol,
				     c_name_buf, sizeof(name_buf));

		/* check if connections from this client are allowed */
		if ((remote_address == NULL && remote_service == NULL) ||
		    (is_allowed((struct sockaddr *)&dest, destlen, &hints,
				remote_address, remote_service) == true))
		{
			if (verbose_mode()) {
				warning(_("connect from %s"), c_name_buf);
			}

			callback(ns, SOCK_SEQPACKET, cdata);

			if (max_accept > 0 && --max_accept == 0)
				break;
		} else {
			close(ns);

			if (verbose_mode()) {
				warning(_("refused connect from %s"),
				        c_name_buf);
			}
		}
	}

	/* close the listening socket */
	close(fd);

	return 0;
}



static int getbluezaddr(const char *address, const char *service,
		const struct addrinfo *hints, struct sockaddr *sa,
		socklen_t *salen)
{
	int psm;

	switch (hints->ai_protocol) {
	case BTPROTO_SCO:
		assert(service == NULL);
		
		*salen = sizeof(struct sockaddr_sco);
		((struct sockaddr_sco *)sa)->sco_family = AF_BLUETOOTH;
		if (address)
			str2ba(address, &((struct sockaddr_sco *)sa)->sco_bdaddr);
		break;
	
	case BTPROTO_L2CAP:
		assert(service != NULL && strlen(service) > 0);
		
		*salen = sizeof(struct sockaddr_l2);
		((struct sockaddr_l2 *)sa)->l2_family = AF_BLUETOOTH;
		if (address)
			str2ba(address, &((struct sockaddr_l2 *)sa)->l2_bdaddr);
		
	        if (safe_atoi(service, &psm) || psm < 0 || psm > USHRT_MAX) {
			warning(_("invalid l2cap psm (service name)"));
			return -1;
		}
		((struct sockaddr_l2 *)sa)->l2_psm = psm;
		break;

	default:
		fatal_internal("invalid bluetooth protocol");
	}

	return 0;
}



static void getbluezname(const struct sockaddr *sa, int protocol,
		char *str, size_t size)
{
	char babuf[BA_MAXHOST];

	/* check arguments */
	assert(sa != NULL);
	assert(str != NULL);
	assert(size > 0);

	switch (protocol) {
	case BTPROTO_SCO:
		safe_ba2str(&(((const struct sockaddr_sco *)sa)->sco_bdaddr),
		            babuf, sizeof(babuf));
		strncpy(str, babuf, size);
		break;

	case BTPROTO_L2CAP:
		safe_ba2str(&(((const struct sockaddr_l2 *)sa)->l2_bdaddr),
		            babuf, sizeof(babuf));
		snprintf(str, size, "%s psm %d", babuf,
		         ((const struct sockaddr_l2 *)sa)->l2_psm);
		break;

	default:
		fatal_internal("invalid bluetooth protocol");
	}
}



static bool is_allowed(const struct sockaddr *sa, socklen_t salen,
		const struct addrinfo *hints,
		const char *address, const char *service)
{
	struct sockaddr_storage ss;
	socklen_t len;

	if (getbluezaddr(address, service, hints, (struct sockaddr *)&ss, &len))
		return false;

	return sockaddr_compare(sa, salen, (const struct sockaddr *)&ss, len);
}
