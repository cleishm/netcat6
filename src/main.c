/*
 *  main.c - main module
 * 
 *  nc6 - an advanced netcat clone
 *  Copyright (C) 2001-2003 Mauro Tortonesi <mauro _at_ deepspace6.net>
 *  Copyright (C) 2002-2003 Chris Leishman <chris _at_ leishman.org>
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
#include "config.h"
#include "connection.h"
#include "io_stream.h"
#include "parser.h"
#include "network.h"
#include "readwrite.h"
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

RCSID("@(#) $Header: /Users/cleishma/work/nc6-repo/nc6/src/main.c,v 1.22 2003-01-13 20:30:35 chris Exp $");

/* program name */
static char *program_name  = NULL;


/* callback type for establish_connection */
typedef int (*established_callback)(const connection_attributes *attrs,
                                    int fd, int socktype);

/* function prototypes */
static int establish_connection(int mode, const connection_attributes *attrs,
                                established_callback callback);
static int connection_main(const connection_attributes *attrs,
                           int fd, int socktype);
static void setup_local_stream(const connection_attributes *attrs,
                               io_stream* local);
static void setup_remote_stream(const connection_attributes *attrs,
                                int fd, int socktype, io_stream* remote);
static int run_transfer(io_stream *remote_stream, io_stream *local_stream);



int main(int argc, char **argv)
{
	connection_attributes connection_attrs;
	char *ptr;
	int mode, retval;

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

	/* set flags and fill out the addresses and connection attributes */
	mode = parse_arguments(argc, argv, &connection_attrs);

	/* establish the connection, and call down to connection_main */
	retval = establish_connection(mode, &connection_attrs, connection_main);

	/* cleanup */
	ca_destroy(&connection_attrs);

	return (retval)? EXIT_FAILURE : EXIT_SUCCESS;
}



static int establish_connection(int mode, const connection_attributes *attrs,
                                established_callback callback)
{
	int fd, socktype;

	assert(attrs != NULL);
	assert(callback != NULL);

	/* establish remote connection */
	switch (mode) {
	case LISTEN_MODE:
		fd = do_listen(attrs, &socktype);
		break;
	case CONNECT_MODE:
		fd = do_connect(attrs, &socktype);
		break;
	default:
		fatal("internal error: unknown connection mode");
		/* not reached - but stops warnings about uninitialized fd */
		fd = -1;
		break;
	}

	assert(fd >= 0);
	assert(socktype >= 0);

	/* announce the socket in very verbose mode */
	if (is_flag_set(VERY_VERBOSE_MODE) == TRUE) {
		switch (socktype) {
		case SOCK_STREAM:
			warn("using stream socket");
			break;
		case SOCK_DGRAM:
			warn("using datagram socket");
			break;
		default:
			fatal("internal error: unsupported socktype %d",
			      socktype);
		}
	}

	return callback(attrs, fd, socktype);
}



static int connection_main(const connection_attributes *attrs,
                           int fd, int socktype)
{
	circ_buf remote_buffer, local_buffer;
	io_stream remote_stream, local_stream;
	int retval;

	assert(attrs != NULL);
	assert(fd >= 0);
	assert(socktype >= 0);

	/* initialise buffers */
	cb_init(&remote_buffer, ca_buffer_size(attrs));
	cb_init(&local_buffer, ca_buffer_size(attrs));

	/* initialise io streams */
	io_stream_init(&remote_stream, "remote", &remote_buffer, &local_buffer);
	io_stream_init(&local_stream, "local", &local_buffer, &remote_buffer);

	/* setup remote stream */
	setup_remote_stream(attrs, fd, socktype, &remote_stream);

	/* setup local stream */
	setup_local_stream(attrs, &local_stream);
	
	/* set remote mtu & nru */
	ios_set_mtu(&remote_stream, ca_remote_MTU(attrs));
	ios_set_nru(&remote_stream, ca_remote_NRU(attrs));

	/* set stream hold timeouts */
	ios_set_hold_timeout(&remote_stream, ca_remote_hold_timeout(attrs));
	ios_set_hold_timeout(&local_stream, ca_local_hold_timeout(attrs));

	/* set stream half close suppression */
	ios_suppress_half_close(&remote_stream,
		ca_remote_half_close_suppress(attrs));
	ios_suppress_half_close(&local_stream,
		ca_local_half_close_suppress(attrs));

	/* give information about the connection in very verbose mode */
	if (is_flag_set(VERY_VERBOSE_MODE) == TRUE) {
		warn("using buffer size of %d", remote_buffer.buf_size);
		if (remote_stream.nru > 0)
			warn("using remote receive nru of %d",
			     remote_stream.nru);
		if (remote_stream.mtu > 0)
			warn("using remote send mtu of %d",
			     remote_stream.mtu);
	}

	/* transfer data between endpoints */
	retval = run_transfer(&remote_stream, &local_stream);

	/* cleanup */
	io_stream_destroy(&local_stream);
	io_stream_destroy(&remote_stream);
	cb_destroy(&local_buffer);
	cb_destroy(&remote_buffer);

	return retval;
}



static void setup_local_stream(const connection_attributes *attrs,
                               io_stream* stream)
{
	/* suppress unused attrs warning */
	while (0&&attrs);
	assert(attrs != NULL);
	assert(stream != NULL);

	ios_assign_stdio(stream);
}



static void setup_remote_stream(const connection_attributes *attrs,
                                int fd, int socktype, io_stream* stream)
{
	/* suppress unused attrs warning */
	while (0&&attrs);
	assert(attrs != NULL);
	assert(fd >= 0);
	assert(socktype >= 0);
	assert(stream != NULL);

	ios_assign_socket(stream, fd, socktype);
}



static int run_transfer(io_stream *remote_stream, io_stream *local_stream)
{
	int retval;

	assert(remote_stream != NULL);
	assert(local_stream != NULL);

	/* setup unidirectional data transfers (if requested) */
	assert(!(is_flag_set(RECV_DATA_ONLY) && is_flag_set(SEND_DATA_ONLY)));

	if (is_flag_set(RECV_DATA_ONLY) == TRUE) {
		/* reading only from the remote stream */

		/* close the remote stream for writing */
		ios_shutdown(remote_stream, SHUT_WR);
		/* close the local stream for reading */
		ios_shutdown(local_stream, SHUT_RD);
		/* disable all hold timeouts */
		ios_set_hold_timeout(remote_stream, -1);
		ios_set_hold_timeout(local_stream, -1);

		if (is_flag_set(VERY_VERBOSE_MODE) == TRUE)
			warn("receiving from remote only, transmit disabled");
	}

	if (is_flag_set(SEND_DATA_ONLY) == TRUE) {
		/* reading only from the local stream */

		/* close the remote stream for reading */
		ios_shutdown(remote_stream, SHUT_RD);
		/* close the local stream for writing */
		ios_shutdown(local_stream, SHUT_WR);
		/* disable all hold timeouts */
		ios_set_hold_timeout(remote_stream, -1);
		ios_set_hold_timeout(local_stream, -1);

		if (is_flag_set(VERY_VERBOSE_MODE) == TRUE)
			warn("transmitting to remote only, receive disabled");
	}

	/* run the main read/write loop */
	retval = readwrite(remote_stream, local_stream);

	if (is_flag_set(VERBOSE_MODE) == TRUE)
		warn("connection closed (sent %d, rcvd %d)",
		     ios_bytes_sent(remote_stream),
		     ios_bytes_received(remote_stream));

	return retval;
}



const char *get_program_name(void)
{
	return program_name;
}
