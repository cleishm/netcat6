/*
 *  main.c - main module
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
#include "parser.h"
#include "attributes.h"
#include "connection.h"
#include "readwrite.h"
#include "io_stream.h"
#include "misc.h"

#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
 

RCSID("@(#) $Header: /Users/cleishma/work/nc6-repo/nc6/src/main.c,v 1.44 2008-06-20 14:44:51 chris Exp $");


/* program name */
static char *program_name  = NULL;


/* function prototypes */
static void established_callback(const connection_attributes_t *attrs,
		int fd, int socktype, void *cdata);
static int connection_main(const connection_attributes_t *attrs,
		int fd, int socktype);
static void setup_local_stream(const connection_attributes_t *attrs,
                io_stream_t *local, circ_buf_t *remote_buffer,
                circ_buf_t *local_buffer);
static void setup_remote_stream(const connection_attributes_t *attrs,
                int fd, int socktype, io_stream_t *remote,
                circ_buf_t *remote_buffer, circ_buf_t *local_buffer);
static int run_transfer(const connection_attributes_t *attrs,
                io_stream_t *remote_stream, io_stream_t *local_stream);
static void i18n_init(void);
static void sigchld_handler(int signum);



int main(int argc, char **argv)
{
	connection_attributes_t connection_attrs;
	char *ptr;
	int retval, result;

	i18n_init();

	/* initialise connection attributes */
	ca_init(&connection_attrs);

	/* save the program name in a static variable */
	if ((ptr = strrchr(argv[0], '/')) != NULL) {
		program_name = ++ptr;
	} else {
		program_name = argv[0];
	}
	
	/* SIGPIPE and SIGURG must be ignored */
	signal(SIGURG, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);

	/* catch SIGCHLD and cleanup the child processes */
	signal(SIGCHLD, sigchld_handler);

	/* set flags and fill out the addresses and connection attributes */
	parse_arguments(argc, argv, &connection_attrs);

	/* establish connections and callback when connected */
	retval = establish_connections(&connection_attrs,
	                               established_callback, &result);

	/* if only a single connection was established, result will
	 * contain any error code from that connection handler */
	if (retval == 0)
		retval = result;

	/* cleanup */
	ca_destroy(&connection_attrs);

	return (retval)? EXIT_FAILURE : EXIT_SUCCESS;
}



static void established_callback(const connection_attributes_t *attrs,
		int fd, int socktype, void *cdata)
{
	/* a connection has been established */
	bool was_forked = false;
	int result;

	/* check if multiple connections will be established,
	 * in which case a child should be forked to handle this connection */
	if (ca_is_flag_set(attrs, CA_LISTEN_MODE) &&
	    ca_is_flag_set(attrs, CA_CONTINUOUS_ACCEPT))
	{
		/* fork and let the parent return immediately */
		int pid;
		int size;
		char *new_name;

		pid = fork();
		if (pid < 0) {
			fatal("fork failed: %s", strerror(errno));
		} else if (pid > 0) {
			/* parent */
			return;
		}

		was_forked = true;

		/* setup program_name */
		size = strlen(program_name) + 10;
		new_name = (char *) xmalloc(size * sizeof(char));
		snprintf(new_name, size, "%s[%d]", program_name, (int)getpid());
		program_name = new_name;
	}

	/* invoke main connection handler */
	result = connection_main(attrs, fd, socktype);

	/* if this is a forked child, then exit with an appropriate code */
	if (was_forked)
		exit((result)? EXIT_FAILURE : EXIT_SUCCESS);

	/* otherwise write the result code to the cdata before returning */
	*((int *)cdata) = result;
}



static int connection_main(const connection_attributes_t *attrs,
		int fd, int socktype)
{
	circ_buf_t remote_buffer, local_buffer;
	io_stream_t remote_stream, local_stream;
	int retval;

	assert(attrs != NULL);
	assert(fd >= 0);
	assert(socktype >= 0);

	/* initialise buffers */
	cb_init(&remote_buffer, ca_buffer_size(attrs));
	cb_init(&local_buffer, ca_buffer_size(attrs));

	setup_remote_stream(attrs, fd, socktype, &remote_stream,
	                    &remote_buffer, &local_buffer);

	setup_local_stream(attrs, &local_stream, &remote_buffer, &local_buffer);
	
	/* set remote mtu & nru */
	ios_set_mtu(&remote_stream, ca_remote_MTU(attrs));
	ios_set_nru(&remote_stream, ca_remote_NRU(attrs));

	/* set idle timeouts - only on remote ios */
	ios_set_idle_timeout(&remote_stream, ca_idle_timeout(attrs));

	/* set stream hold timeouts */
	ios_set_hold_timeout(&remote_stream, ca_remote_hold_timeout(attrs));
	ios_set_hold_timeout(&local_stream, ca_local_hold_timeout(attrs));

	/* set stream half close suppression */
	ios_suppress_half_close(&remote_stream,
		ca_remote_half_close_suppress(attrs));
	ios_suppress_half_close(&local_stream,
		ca_local_half_close_suppress(attrs));

	/* give information about the connection in very verbose mode */
	if (very_verbose_mode()) {
		warning(_("using buffer size of %d"), remote_buffer.buf_size);
		if (remote_stream.nru > 0)
			warning(_("using remote receive nru of %d"),
			     remote_stream.nru);
		if (remote_stream.mtu > 0)
			warning(_("using remote send mtu of %d"),
			     remote_stream.mtu);
	}

	/* transfer data between endpoints */
	retval = run_transfer(attrs, &remote_stream, &local_stream);

	/* cleanup */
	io_stream_destroy(&local_stream);
	io_stream_destroy(&remote_stream);
	cb_destroy(&local_buffer);
	cb_destroy(&remote_buffer);

	return retval;
}



static void setup_local_stream(const connection_attributes_t *attrs,
		io_stream_t *stream, circ_buf_t *remote_buffer,
		circ_buf_t *local_buffer)
{
	const char *cmd;
	assert(attrs != NULL);
	assert(stream != NULL);

	cmd = ca_local_exec(attrs);
	if (cmd != NULL) {
		int in, out;
		if (very_verbose_mode())
			warning(_("executing '%s'"), cmd);
		if (open3(cmd, &in, &out, NULL) < 0) {
			fatal(_("failed to exec '%s': %s"),
			      cmd, strerror(errno));
		}
		ios_init(stream, "local", out, in, SOCK_STREAM,
		         local_buffer, remote_buffer);
	}
	else {
		ios_init_stdio(stream, "local", local_buffer, remote_buffer);
	}
}



static void setup_remote_stream(const connection_attributes_t *attrs,
		int fd, int socktype, io_stream_t *stream,
		circ_buf_t *remote_buffer, circ_buf_t *local_buffer)
{
	/* suppress unused attrs warning */
	while (0&&attrs);
	assert(attrs != NULL);
	assert(fd >= 0);
	assert(socktype >= 0);
	assert(stream != NULL);

	ios_init_socket(stream, "remote", fd, socktype,
	                remote_buffer, local_buffer);
}



static int run_transfer(const connection_attributes_t *attrs,
		io_stream_t *remote_stream, io_stream_t *local_stream)
{
	int retval;

	assert(remote_stream != NULL);
	assert(local_stream != NULL);

	/* setup unidirectional data transfers (if requested) */
	assert(!ca_is_flag_set(attrs, CA_RECV_DATA_ONLY) ||
	       !ca_is_flag_set(attrs, CA_SEND_DATA_ONLY));

	if (ca_is_flag_set(attrs, CA_RECV_DATA_ONLY)) {
		/* reading only from the remote stream */

		/* close the remote stream for writing */
		ios_shutdown(remote_stream, SHUT_WR);
		/* close the local stream for reading */
		ios_shutdown(local_stream, SHUT_RD);
		/* disable all hold timeouts */
		ios_set_hold_timeout(remote_stream, -1);
		ios_set_hold_timeout(local_stream, -1);

		if (very_verbose_mode())
			warning(_("receiving from remote only, "
			     "transmit disabled"));
	}

	if (ca_is_flag_set(attrs, CA_SEND_DATA_ONLY)) {
		/* reading only from the local stream */

		/* close the remote stream for reading */
		ios_shutdown(remote_stream, SHUT_RD);
		/* close the local stream for writing */
		ios_shutdown(local_stream, SHUT_WR);
		/* disable all hold timeouts */
		ios_set_hold_timeout(remote_stream, -1);
		ios_set_hold_timeout(local_stream, -1);

		if (very_verbose_mode())
			warning(_("transmitting to remote only, "
			     "receive disabled"));
	}

	/* run the main read/write loop */
	retval = readwrite(remote_stream, local_stream);

	if (very_verbose_mode())
		warning(_("connection closed (sent %d, rcvd %d)"),
		     ios_bytes_sent(remote_stream),
		     ios_bytes_received(remote_stream));
#ifndef NDEBUG
	if (very_verbose_mode())
		warning("readwrite returned %d", retval);
#endif

	return retval;
}



const char *get_program_name(void)
{
	return program_name;
}



static void i18n_init(void)
{
#ifdef ENABLE_NLS
#ifdef HAVE_SETLOCALE
	setlocale(LC_ALL, "");
	setlocale(LC_MESSAGES, "");
#endif /* HAVE_SETLOCALE */
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
#endif /* ENABLE_NLS */
}



/* cleanup any child processes created via --exec */
static void sigchld_handler(int signum)
{
	int pid;

	/* suppress unused attrs warning */
	while (0&&signum);
	assert(signum == SIGCHLD);

	do {
		int status;
		pid = waitpid(WAIT_ANY, &status, WNOHANG);
	} while (pid > 0);
}
