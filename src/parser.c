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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "config.h"  
#include "misc.h"  
#include "network.h"  
#include "parser.h"  
#include "timeout.h"  


static unsigned long flags_mask;
static void set_flag(unsigned long mask);
static void parse_and_set_timeouts(const char *str);
static void print_usage(FILE *fp);




bool is_flag_set(unsigned long mask)
{
	return ((flags_mask & mask) ? TRUE : FALSE);
}



void parse_arguments(int argc, char **argv)
{
	int c, verbosity_level;
	char src_addr[NI_MAXHOST + 1];
	char src_port[NI_MAXSERV + 1];
	address local, remote;
	connection_attributes attrs;

	/* initialize local address, address family and other stuff 
	 * with their default values */
	local.address   = NULL;
	local.port      = NULL;
	verbosity_level = 0;

	/* initialize hints structure to default values */
	attrs.proto = PROTO_UNSPECIFIED;
	attrs.type  = TCP_SOCKET;
	
	/* initialize to zero for correct use of getopt */
	opterr = 0;

	/* option recognition loop */
	while ((c = getopt(argc, argv, "46hlnp:q:s:uvx")) >= 0) {
 		switch(c) {
		case '4':
			if (attrs.proto != PROTO_UNSPECIFIED) 
				fatal("cannot specify the address family twice");
			attrs.proto = PROTO_IPv4;
			break;
		case '6':	
			if (attrs.proto != PROTO_UNSPECIFIED) 
				fatal("cannot specify the address family twice");
			attrs.proto = PROTO_IPv6;
			set_flag(STRICT_IPV6);
			break;
		case 'd':	
			set_flag(DONT_REUSE_ADDR);
			break;
		case 'h':	
			print_usage(stdout);
			exit(EXIT_SUCCESS);
		case 'l':
			set_flag(LISTEN_MODE);
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
			strncpy(src_port, optarg, sizeof(src_port) - 1);
			src_port[sizeof(src_port) - 1] = '\0';
			local.port = src_port;
			break;	
		case 'q':
			set_flag(TIMEOUT_MODE);
			parse_and_set_timeouts(optarg);
			break;	
		case 's':	
			strncpy(src_addr, optarg, sizeof(src_addr) - 1);
			src_addr[sizeof(src_addr) - 1] = '\0';
			local.address = src_addr;
			break;	
		case 'u':	
			attrs.type = UDP_SOCKET;
			break;
		case 'v':	
			if (++verbosity_level > 1) 
				set_flag(VERY_VERBOSE_MODE); 
			set_flag(VERBOSE_MODE); 
			break;
		case 'x':	
			set_flag(FILE_TRANSFER_MODE);
			set_flag(TIMEOUT_MODE);
			set_timeouts(0, 0, SET_TIMEOUT1 | SET_TIMEOUT2);
			break;
		default:	
			print_usage(stderr);
			exit(EXIT_FAILURE);
		}
	}
	
	argv += optind;
	argc -= optind;

	switch(argc) {
	case 0:
		remote.address = NULL;
		remote.port    = NULL;
		break;
	case 1:
		remote.address = argv[0];
		remote.port    = NULL;
		break;
	case 2:
		remote.address = argv[0];
		remote.port    = argv[1];
		break;
	default:
		print_usage(stderr);
		exit(EXIT_FAILURE);
	}

	if (is_flag_set(LISTEN_MODE) == TRUE) {	
		if (local.port == NULL) {
			warn("in listen mode you must specify a port with the -p switch");
			print_usage(stderr);
			exit(EXIT_FAILURE);
		}

		assert(remote.address == NULL || strlen(remote.address) > 0);
		assert(remote.port == NULL || strlen(remote.port) > 0);

		do_listen(&remote, &local, &attrs);
	} else {
		if (is_flag_set(DONT_REUSE_ADDR)) {
			warn("-d option can be used only in listen mode");
			print_usage(stderr);
			exit(EXIT_FAILURE);
		}
		
		if (remote.address == NULL || remote.port == NULL) {
			warn("you must specify the address/port couple of the remote endpoint");
			print_usage(stderr);
			exit(EXIT_FAILURE);
		}

		/* sanity checks */
		assert(remote.address != NULL && strlen(remote.address) > 0);
		assert(remote.port != NULL && strlen(remote.port) > 0);

		do_connect(&remote, &local, &attrs);
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
		"    -q n1[:n2] Timeout mode\n"
		"    -s addr    Local source address\n"
		"    -u         Require use of UDP\n"
		"    -v         Increase program verbosity (call twice for max verbosity)\n"
		"    -x         File transfer mode\n\n");
}



static void parse_and_set_timeouts(const char *str)
{
	int t1, t2, flags;
	char *s;

	assert(str != NULL);

	flags = SET_TIMEOUT2;
	
	if ((s = strchr(str, ':')) != NULL) {
		*s++ = '\0';
		t1 = safe_atoi(s);
		flags |= SET_TIMEOUT1;
	} else {
		t1 = 0;
	}

	t2 = safe_atoi(str);

#ifndef NDEBUG
	printf("timeouts are: %d,%d\n", t1, t2);
#endif
	
	set_timeouts(t1, t2, flags);
}




static void set_flag(unsigned long mask)
{
	flags_mask = flags_mask | mask;
}

