/*
 *  parser.c - argument parser & dispatcher module - implementation 
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
#include "misc.h"  
#include "network.h"  
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

RCSID("@(#) $Header: /Users/cleishma/work/nc6-repo/nc6/src/parser.c,v 1.13 2002-12-24 20:20:31 mauro Exp $");


static unsigned long flags_mask;
static void set_flag(unsigned long mask);
static void parse_and_set_timeouts(const char *str,
                                   connection_attributes *attrs);
static void print_usage(FILE *fp);



int parse_arguments(int argc, char **argv, connection_attributes *attrs)
{
	int c, verbosity_level = 0;
	bool listen_mode = FALSE;
	bool file_transfer = FALSE;

	/* set socket types to default values */
	attrs->proto = PROTO_UNSPECIFIED;
	attrs->type  = TCP_SOCKET;

	/* initialize to zero for correct use of getopt */
	opterr = 0;

	/* option recognition loop */
	while ((c = getopt(argc, argv, "46hlnp:q:s:uvx")) >= 0) {
 		switch(c) {
		case '4':
			if (attrs->proto != PROTO_UNSPECIFIED) 
				fatal("cannot specify the address family twice");
			attrs->proto = PROTO_IPv4;
			break;
		case '6':	
			if (attrs->proto != PROTO_UNSPECIFIED) 
				fatal("cannot specify the address family twice");
			attrs->proto = PROTO_IPv6;
			set_flag(STRICT_IPV6);
			break;
		case 'd':	
			set_flag(DONT_REUSE_ADDR);
			break;
		case 'h':	
			print_usage(stdout);
			exit(EXIT_SUCCESS);
		case 'l':
			listen_mode = TRUE;
			break;
		case 'n':	
			set_flag(NUMERIC_MODE);
			break;
		case 'p':	
			if (optarg == NULL) {
				warn("you must specify a port with the -p switch");
				print_usage(stderr);
				exit(EXIT_FAILURE);
			}
			attrs->local_address.service = xstrdup(optarg);
			break;	
		case 'q':
			parse_and_set_timeouts(optarg, attrs);
			break;	
		case 's':	
			attrs->local_address.address = xstrdup(optarg);
			break;	
		case 'u':	
			attrs->type = UDP_SOCKET;
			break;
		case 'v':	
			if (++verbosity_level > 1) 
				set_flag(VERY_VERBOSE_MODE); 
			set_flag(VERBOSE_MODE); 
			break;
		case 'x':	
			file_transfer = TRUE;
			break;
		default:	
			print_usage(stderr);
			exit(EXIT_FAILURE);
		}
	}
	
	argv += optind;
	argc -= optind;

	/* set mode flags */
	set_flag((listen_mode)? LISTEN_MODE : CONNECT_MODE);

	/* setup file transfer depending on the mode */
	if (file_transfer == TRUE) {
		set_flag((listen_mode)? RECV_DATA_ONLY : SEND_DATA_ONLY);
	}

	/* additional arguments are the remote address/service */
	switch(argc) {
	case 0:
		attrs->remote_address.address = NULL;
		attrs->remote_address.service = NULL;
		break;
	case 1:
		attrs->remote_address.address = argv[0];
		attrs->remote_address.service = NULL;
		break;
	case 2:
		attrs->remote_address.address = argv[0];
		attrs->remote_address.service = argv[1];
		break;
	default:
		print_usage(stderr);
		exit(EXIT_FAILURE);
	}

	/* sanity check */
	if (attrs->remote_address.address != NULL &&
		strlen(attrs->remote_address.address) == 0) {
		attrs->remote_address.address = NULL;
	}

	if (attrs->remote_address.service != NULL &&
		strlen(attrs->remote_address.service) == 0) {
		attrs->remote_address.service = NULL;
	}

	if (listen_mode) {	
		if (attrs->local_address.service == NULL) {
			warn("in listen mode you must specify a port with the -p switch");
			print_usage(stderr);
			exit(EXIT_FAILURE);
		}

		return LISTEN_MODE;
	} else {
		if (is_flag_set(DONT_REUSE_ADDR)) {
			warn("-d option can be used only in listen mode");
			print_usage(stderr);
			exit(EXIT_FAILURE);
		}
		
		if (attrs->remote_address.address == NULL ||
		    attrs->remote_address.service == NULL) {
			warn("you must specify the address/port couple of the remote endpoint");
			print_usage(stderr);
			exit(EXIT_FAILURE);
		}

		return CONNECT_MODE;
	}
}



static void print_usage(FILE *fp)
{
	const char *program_name = get_program_name();

	fprintf(fp, "\nUsage:\n"
		"\t%s [-46hnux] [-p port] [-s addr] hostname port\n"
		"\t%s -l -p port [-s addr] [-46dhnux] [hostname] [port]\n\n"
		"Recognized options are:\n", program_name, program_name);
	fprintf(fp,	
		"    -4         Use only IPv4\n"
		"    -6         Use only IPv6\n"
		"    -d         Disable SO_REUSEADDR socket option (only in listen mode)\n"
		"    -h         Display help\n"
		"    -l         Listen mode, for inbound connects\n"
		"    -n         Numeric-only IP addresses, no DNS\n" 
		"    -p port    Local source port\n"
		"    -q n1[:n2] Hold timeouts\n"
		"    -s addr    Local source address\n"
		"    -u         Require use of UDP\n"
		"    -v         Increase program verbosity (call twice for max verbosity)\n"
		"    -x         File transfer mode\n\n");
}



static void parse_and_set_timeouts(const char *str,
		connection_attributes *attrs)
{
	char *s;

	assert(str != NULL);

	if ((s = strchr(str, ':')) != NULL) {
		*s++ = '\0';
		ios_set_hold_timeout(&(attrs->remote_stream),
		                     (s[0] == '-')? -1 : safe_atoi(s));
	}

	ios_set_hold_timeout(&(attrs->local_stream),
	                     (str[0] == '-')? -1 : safe_atoi(str));
}



bool is_flag_set(unsigned long mask)
{
	return ((flags_mask & mask) ? TRUE : FALSE);
}



static void set_flag(unsigned long mask)
{
	flags_mask = flags_mask | mask;
}
