/*
 *  main.c - main module
 * 
 *  nc6 - an advanced netcat clone
 *  Copyright (C) 2001-2002 Mauro Tortonesi <mauro _at_ ferrara.linux.it>
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

RCSID("@(#) $Header: /Users/cleishma/work/nc6-repo/nc6/src/main.c,v 1.8 2002-12-24 21:05:37 chris Exp $");

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

	/* establish a connection */
	switch (mode) {
	case LISTEN_MODE:
		do_listen(&connection_attrs);
		break;
	case CONNECT_MODE:
		do_connect(&connection_attrs);
		break;
	default:
		fatal("internal error: unknown connection mode");
		break;
	}

	/* setup unidirectional data transfers (if requested) */
	assert(!(is_flag_set(RECV_DATA_ONLY) && is_flag_set(SEND_DATA_ONLY)));

	if (is_flag_set(RECV_DATA_ONLY) == TRUE) {
		/* reading only from the remote stream */

		/* close the remote stream for writing */
		ios_shutdown(&(connection_attrs.remote_stream), SHUT_WR);

		/* close the local stream for reading */
		ios_shutdown(&(connection_attrs.local_stream), SHUT_RD);

		/* don't stop because the read is closed */
		ios_set_hold_timeout(&(connection_attrs.local_stream), -1);

#ifndef NDEBUG
		if (is_flag_set(VERY_VERBOSE_MODE) == TRUE)
			warn("File xfer mode: reading remote only");
#endif
	}

	if (is_flag_set(SEND_DATA_ONLY) == TRUE) {
		/* reading only from the local stream */

		/* close the remote stream for reading */
		ios_shutdown(&(connection_attrs.remote_stream), SHUT_RD);

		/* don't stop because the read is closed */
		ios_set_hold_timeout(&(connection_attrs.remote_stream), -1);

		/* close the local stream for writing */
		ios_shutdown(&(connection_attrs.local_stream), SHUT_WR);

#ifndef NDEBUG
		if (is_flag_set(VERY_VERBOSE_MODE) == TRUE)
			warn("File xfer mode: reading local only");
#endif
	}

	/* run the main read/write loop */
	retval = readwrite(&(connection_attrs.remote_stream),
	                   &(connection_attrs.local_stream));

	/* cleanup */
	connection_attributes_destroy(&connection_attrs);

	return (retval)? EXIT_FAILURE : EXIT_SUCCESS;
}



const char *get_program_name(void)
{
	return program_name;
}
