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
#include "parser.h"
#include "network.h"
#include "readwrite.h"
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

RCSID("@(#) $Header: /Users/cleishma/work/nc6-repo/nc6/src/main.c,v 1.15 2003-01-01 10:05:32 chris Exp $");

/* program name */
static char *program_name  = NULL;


int main(int argc, char **argv)
{
	connection_attributes connection_attrs;
	int mode;
	char *ptr;
	int retval;

	connection_attributes_init(&connection_attrs);

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

	/* setup local stream */
	ios_assign_stdio(&(connection_attrs.local_stream));
	
	/* establish remote connection */
	switch (mode) {
	case LISTEN_MODE:
		do_listen(&connection_attrs);
		break;
	case CONNECT_MODE:
		do_connect(&connection_attrs);
		break;
	default:
		fatal("internal error: unknown connection mode");
	}

	/* give information about the connection in very verbose mode */
	if (is_flag_set(VERY_VERBOSE_MODE) == TRUE) {

		switch (connection_attrs.remote_stream.socktype) {
		case SOCK_STREAM:
			warn("using stream socket");
			break;
		case SOCK_DGRAM:
			warn("using datagram socket");
			break;
		default:
			fatal("internal error: unsupported socktype %d",
			      connection_attrs.remote_stream.socktype);
		}

		warn("using remote receive buffer size of %d",
		     connection_attrs.remote_buffer.buf_size);

		if (connection_attrs.remote_stream.nru)
			warn("using remote receive nru of %d",
			     connection_attrs.remote_stream.nru);

		if (connection_attrs.remote_stream.mtu)
			warn("using remote send mtu of %d",
			     connection_attrs.remote_stream.mtu);
	}

	/* setup unidirectional data transfers (if requested) */
	assert(!(is_flag_set(RECV_DATA_ONLY) && is_flag_set(SEND_DATA_ONLY)));

	if (is_flag_set(RECV_DATA_ONLY) == TRUE) {
		/* reading only from the remote stream */

		/* close the remote stream for writing */
		ios_shutdown(&(connection_attrs.remote_stream), SHUT_WR);
		/* close the local stream for reading */
		ios_shutdown(&(connection_attrs.local_stream), SHUT_RD);
		/* disable all hold timeouts */
		ios_set_hold_timeout(&(connection_attrs.local_stream), -1);
		ios_set_hold_timeout(&(connection_attrs.remote_stream), -1);

#ifndef NDEBUG
		if (is_flag_set(VERY_VERBOSE_MODE) == TRUE)
			warn("receiving from remote only, transmit disabled");
#endif
	}

	if (is_flag_set(SEND_DATA_ONLY) == TRUE) {
		/* reading only from the local stream */

		/* close the remote stream for reading */
		ios_shutdown(&(connection_attrs.remote_stream), SHUT_RD);
		/* close the local stream for writing */
		ios_shutdown(&(connection_attrs.local_stream), SHUT_WR);
		/* disable all hold timeouts */
		ios_set_hold_timeout(&(connection_attrs.local_stream), -1);
		ios_set_hold_timeout(&(connection_attrs.remote_stream), -1);

#ifndef NDEBUG
		if (is_flag_set(VERY_VERBOSE_MODE) == TRUE)
			warn("transmitting to remote only, receive disabled");
#endif
	}

	/* run the main read/write loop */
	retval = readwrite(&(connection_attrs.remote_stream),
	                   &(connection_attrs.local_stream));

	if (is_flag_set(VERBOSE_MODE) == TRUE)
		warn("connection closed (sent %d, rcvd %d)",
			ios_bytes_sent(&(connection_attrs.remote_stream)),
			ios_bytes_received(&(connection_attrs.remote_stream)));

	/* cleanup */
	connection_attributes_destroy(&connection_attrs);

	return (retval)? EXIT_FAILURE : EXIT_SUCCESS;
}



const char *get_program_name(void)
{
	return program_name;
}
