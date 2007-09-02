/*
 *  iucv.c - iucv networking functions module - implementation
 * 
 *  Copyright IBM Corp. 2007
 *  Author: Tom Zanussi <zanussi _at_ us.ibm.com>
 *  Portions from afindep.c, bluez.c
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
#include "iucv.h"
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

/* suggested size for argument to getnameinfo_ex */
static const int AI_STR_SIZE = (2 * (NI_MAXHOST + NI_MAXSERV + 2)) + 8;

/* iucv names are space-padded */
static void iucv_pad_string(char *dst, const char *src)
{
	int len;
	
	memset(dst, ' ', IUCV_MAXNAME);

	if (!(src && src[0]))
		return;

	len = strlen(src);
	if (len > IUCV_MAXNAME)
		len = IUCV_MAXNAME;

	memcpy(dst, src, len);
}

static struct sockaddr *iucv_create_sockaddr(const char *userid,
					     const char *name,
					     socklen_t *addrlen)
{
	struct sockaddr_iucv *addr;

	/* af_iucv can fill in userid but always needs name */
	if (!name)
		return NULL;

	addr = malloc(sizeof(struct sockaddr_iucv));

	/* Reset the sockaddr structure */
	memset(addr, 0, sizeof(struct sockaddr_iucv));
	addr->siucv_family = AF_IUCV;
	memset(addr->siucv_nodeid,' ',sizeof(addr->siucv_nodeid));
	iucv_pad_string(addr->siucv_userid, userid);
	iucv_pad_string(addr->siucv_name, name);

	*addrlen = sizeof(struct sockaddr_iucv);

	return (struct sockaddr *)addr;
}

static void iucv_destroy_sockaddr(struct sockaddr *addr)
{
	free((struct sockaddr_iucv *)addr);
}

static void iucv_warn_noconnect(const char *remote_address,
				const char *remote_service)
{
	warning(_("unable to connect "
		  "to address %s, service %s"), 
		remote_address, remote_service);
}

int iucv_connect(const struct addrinfo *hints,
		 const char *remote_address, const char *remote_service,
		 const char *local_address, const char *local_service,
		 set_sockopt_handler_t set_sockopt_handler, void *hdata,
		 time_t timeout, int *rt_socktype)
{
	int err, fd = -1;
	char name_buf[AI_STR_SIZE];
	struct sockaddr *remote_addr = NULL;
	struct sockaddr *local_addr = NULL;
	socklen_t addrlen;

	/* make sure arguments are valid and preconditions are respected */
	assert(hints != NULL);
	if (remote_address == NULL || strlen(remote_address) == 0 ||
	    remote_service == NULL || strlen(remote_service) == 0) {
		warning("remote host and service must both be specfied");
		iucv_warn_noconnect(remote_address, remote_service);
		return -1;
	}
	if ((local_address != NULL && strlen(local_address) != 0) &&
	    (local_service == NULL || strlen(local_service) == 0)) {
		warning("local service must be specfied "
			"with local host");
		iucv_warn_noconnect(remote_address, remote_service);
		return -1;
	}
	
	/* create the socket */
	fd = socket(hints->ai_family, hints->ai_socktype, hints->ai_protocol);
	if (fd < 0) {
		warning("cannot create the socket: %s",
			strerror(errno));
		return -1;
	}
		
	if (set_sockopt_handler != NULL)
		set_sockopt_handler(fd, hdata);

	/* setup name_buf if we're in verbose mode */
	if (verbose_mode())
		sprintf(name_buf, "%s %s", remote_address, remote_service);
	
	/* setup local source address and/or service */
	if (local_address != NULL || local_service != NULL) {
		local_addr = iucv_create_sockaddr(local_address,
						  local_service, &addrlen);

		if (local_addr == NULL) {
			if (verbose_mode()) {
					warning(_("couldn't create local "
						  "sockaddr: host %s, "
						  "service %s"),
						  local_address ?
						  local_address : "",
						  local_service);
			}
			close(fd);
			iucv_warn_noconnect(remote_address, remote_service);
			return -1;
		}

		/* try binding to the addresse */
		err = bind(fd, local_addr, addrlen);
			
		if (err != 0) {
			if (verbose_mode()) {
				warning(_("bind to source addr/port "
					  "failed when connecting to "
					  "%s: %s"), name_buf,
					strerror(errno));
			}
			close(fd);
			iucv_destroy_sockaddr(local_addr);
			iucv_warn_noconnect(remote_address, remote_service);
			return -1;
		}
	}

	remote_addr = iucv_create_sockaddr(remote_address,
					   remote_service, &addrlen);

	if (remote_addr == NULL) {
		if (verbose_mode()) {
			warning(_("couldn't create remote "
				  "sockaddr: host %s, "
				  "service %s"),
				  remote_address ?
				  remote_address : "",
				  remote_service);
		}
		close(fd);
		iucv_destroy_sockaddr(local_addr);
		iucv_warn_noconnect(remote_address, remote_service);
		return -1;
	}

	/* attempt the connection */
	err = connect_with_timeout(fd, remote_addr, addrlen, timeout);

	/* check error code */
	if (err) {
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
		iucv_destroy_sockaddr(local_addr);
		iucv_destroy_sockaddr(remote_addr);
		iucv_warn_noconnect(remote_address, remote_service);
		return -1;
	}

	/* let the user know the connection has been established */
	if (verbose_mode())
		warning(_("%s open"), name_buf);

	/* return the socktype */
	if (rt_socktype != NULL)
		*rt_socktype = hints->ai_socktype;

	/* cleanup addr structs */
	if (local_addr)
		iucv_destroy_sockaddr(local_addr);
	if (remote_addr)
		iucv_destroy_sockaddr(remote_addr);

	return fd;
}

static void iucv_warn_nolisten(const char *local_address,
			       const char *local_service)
{
	warning(_("failed to listen "
		  "on address %s, service %s"), 
		local_address ? local_address : "localhost",
		local_service);
}

static void getiucvaddr(const char *address, const char *service,
			struct sockaddr *sa, socklen_t *salen)
{
	struct sockaddr_iucv *sai;

	sai = (struct sockaddr_iucv *)sa;
	*salen = sizeof(struct sockaddr_iucv);

	sai->siucv_family = AF_IUCV;
	memset(sai->siucv_userid, ' ', IUCV_MAXNAME);
	if (address != NULL)
		memcpy(sai->siucv_userid, address, strlen(address));
	memset(sai->siucv_name, ' ', IUCV_MAXNAME);
	if (service != NULL)
		memcpy(sai->siucv_name, service, strlen(service));
}

static void getiucvname(const struct sockaddr *sa, char *str, size_t size)
{
	const struct sockaddr_iucv *sai;
	char *p = str;
	
	/* check arguments */
	assert(sa != NULL);
	assert(str != NULL);
	assert(size > 2 * IUCV_MAXNAME + 1);

	sai = (const struct sockaddr_iucv *)sa;

	memset(str, 0, size);

	memcpy(p, sai->siucv_userid, IUCV_MAXNAME);
	p += IUCV_MAXNAME;
	*p++ = ' ';
	memcpy(p, sai->siucv_name, IUCV_MAXNAME);
}

static bool is_allowed(const struct sockaddr *sa, socklen_t salen,
		const char *address, const char *service)
{
	struct sockaddr_storage ss;
	socklen_t len;

	getiucvaddr(address, service, (struct sockaddr *)&ss, &len);

	return sockaddr_compare(sa, salen, (const struct sockaddr *)&ss, len);
}

int iucv_listener(const struct addrinfo *hints,
		  const char *local_address, const char *local_service,
		  const char *remote_address, const char *remote_service,
		  set_sockopt_handler_t set_sockopt_handler, void *hdata,
		  listen_callback_t callback, void *cdata,
		  time_t timeout, int max_accept)
{
	int i, fd, err, maxfd = -1;
	bound_socket_t *bound_sockets = NULL;
	fd_set accept_fdset;
	char name_buf[AI_STR_SIZE];
	struct sockaddr *local_addr = NULL;
	socklen_t addrlen;

	/* make sure arguments are valid and preconditions are respected */
	assert(hints != NULL);
	if (local_service == NULL || strlen(local_service) == 0) {
		warning("local service must be specified");
		iucv_warn_nolisten(local_address, local_service);
		return -1;
	}

	/* if max_accept is 0, just return */
	if (max_accept == 0)
		return 0;

	/* initialize accept_fdset */
	FD_ZERO(&accept_fdset);
	
	/* create the socket */
	fd = socket(hints->ai_family, hints->ai_socktype, hints->ai_protocol);
	if (fd < 0) {
		warning("cannot create the socket: %s",
			strerror(errno));
		return -1;
	}

	if (set_sockopt_handler != NULL)
		set_sockopt_handler(fd, hdata);

	/* setup name_buf if we're in verbose mode */
	if (verbose_mode())
		sprintf(name_buf, "%s %s", local_address, local_service);

	local_addr = iucv_create_sockaddr(local_address,
					  local_service, &addrlen);

	if (local_addr == NULL) {
		if (verbose_mode()) {
			warning(_("couldn't create local "
				  "sockaddr: host %s, "
				  "service %s"),
				local_address ?
				local_address : "",
				local_service);
		}
		close(fd);
		iucv_warn_nolisten(local_address, local_service);
		return -1;
	}

	/* bind to the local address */
	err = bind(fd, local_addr, addrlen);
	if (err != 0) {
		warning(_("bind to source %s failed: %s"),
			name_buf, strerror(errno));
		close(fd);
		iucv_destroy_sockaddr(local_addr);
		iucv_warn_nolisten(local_address, local_service);
		return -1;
	}

	if (hints->ai_socktype == SOCK_STREAM) {
		err = listen(fd, SOMAXCONN);
		if (err != 0) {
			warning(_("cannot listen on %s: %s"),
				name_buf, strerror(errno));
			free_bound_sockets(bound_sockets);
			close(fd);
			iucv_destroy_sockaddr(local_addr);
			iucv_warn_nolisten(local_address, local_service);
			return -1;
		}
	}

	if (verbose_mode())
		warning(_("listening on %s ..."), name_buf);

	/* add fd to bound_sockets (just add to the head of the list) */
	bound_sockets =	add_bound_socket(bound_sockets, fd, 
					 hints->ai_socktype);

	/* add fd to accept_fdset */
	FD_SET(fd, &accept_fdset);
	maxfd = MAX(maxfd, fd);
	
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
			iucv_destroy_sockaddr(local_addr);
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

		ns = accept(fd, (struct sockaddr *)&dest, &destlen);
		if (ns < 0) {
			warning("accept failed: %s", strerror(errno));
			free_bound_sockets(bound_sockets);
			iucv_destroy_sockaddr(local_addr);
			return -1;
		}

		/* get names for each end of the connection */
		if (verbose_mode()) {
			/* address the connection was to */
			sprintf(name_buf, "%s %s", local_address ?
				local_address : "localhost", local_service);

			/* get the name for this client */
			getiucvname((struct sockaddr *)&dest, c_name_buf,
				    sizeof(c_name_buf));

		}

		/* check if connections from this client are allowed */
		if ((remote_address == NULL && remote_service == NULL) ||
		    (is_allowed((struct sockaddr *)&dest, destlen,
				remote_address, remote_service) == true))
		{

			if (verbose_mode()) {
				warning(_("connect to %s from %s"),
				        name_buf, c_name_buf);
			}

			callback(ns, socktype, cdata);

			if (max_accept > 0 && --max_accept == 0)
				break;
		} else {
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
	iucv_destroy_sockaddr(local_addr);
	return 0;
}


