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

RCSID("@(#) $Header: /Users/cleishma/work/nc6-repo/nc6/src/main.c,v 1.18 2003-01-11 19:46:38 chris Exp $");

/* program name */
static char *program_name  = NULL;


static void establish_connection(int mode, connection_attributes *attrs);
static int do_transfer(connection_attributes *attrs);


int main(int argc, char **argv)
{
	connection_attributes connection_attrs;
	int mode;
	char *ptr;
	int retval;

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

	/* setup local stream */
	ios_assign_stdio(ca_local_stream(&connection_attrs));
	
	/* establish remote connection */
	establish_connection(mode, &connection_attrs);

	/* give information about the connection in very verbose mode */
	if (is_flag_set(VERY_VERBOSE_MODE) == TRUE)
		ca_warn_details(&connection_attrs);

	retval = do_transfer(&connection_attrs);

	/* cleanup */
	ca_destroy(&connection_attrs);

	return (retval)? EXIT_FAILURE : EXIT_SUCCESS;
}


static void establish_connection(int mode, connection_attributes *attrs)
{
	/* establish remote connection */
	switch (mode) {
	case LISTEN_MODE:
		do_listen(attrs);
		break;
	case CONNECT_MODE:
		do_connect(attrs);
		break;
	default:
		fatal("internal error: unknown connection mode");
	}
}


static int do_transfer(connection_attributes *attrs)
{
	io_stream *remote_stream = ca_remote_stream(attrs);
	io_stream *local_stream = ca_local_stream(attrs);
	int retval;

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

#ifndef NDEBUG
		if (is_flag_set(VERY_VERBOSE_MODE) == TRUE)
			warn("receiving from remote only, transmit disabled");
#endif
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

#ifndef NDEBUG
		if (is_flag_set(VERY_VERBOSE_MODE) == TRUE)
			warn("transmitting to remote only, receive disabled");
#endif
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
