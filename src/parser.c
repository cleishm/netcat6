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
#include <getopt.h>

RCSID("@(#) $Header: /Users/cleishma/work/nc6-repo/nc6/src/parser.c,v 1.21 2002-12-30 16:11:47 chris Exp $");


/* default UDP MTU is 8kb */
static const size_t DEFAULT_UDP_MTU = 8192;
/* default UDP NRU is the maximum allowed MTU of 64k */
static const size_t DEFAULT_UDP_NRU = 65536;
/* default UDP buffer size is 64k */
static const size_t DEFAULT_UDP_BUFFER_SIZE = 65536;
/* default buffer size for file transfers is 64k */
static const size_t DEFAULT_FILE_TRANSFER_BUFFER_SIZE = 65536;

/* storage for the global flags */
static unsigned long flags_mask;

/* long options */
static const struct option long_options[] = {
#define OPT_HELP		0
	{"help",          FALSE, NULL, 'h'},
#define OPT_LISTEN		1
	{"listen",        FALSE, NULL, 'l'},
#define OPT_PORT		2
	{"port",          TRUE,  NULL, 'p'},
#define OPT_HOLD_TIMEOUTS	3
	{"hold-timeouts", TRUE,  NULL, 'q'},
#define OPT_ADDRESS		4
	{"address",       TRUE,  NULL, 's'},
#define OPT_UDP			5
	{"udp",           FALSE, NULL, 'u'},
#define OPT_TIMEOUT		6
	{"timeout",       TRUE,  NULL, 'w'},
#define OPT_TRANSFER		7
	{"transfer",      FALSE, NULL, 'x'},
#define OPT_BUFFER_SIZE		8
	{"buffer-size",   TRUE,  NULL,  0 },
#define OPT_MTU			9
	{"mtu",           TRUE,  NULL,  0 },
#define OPT_NRU			10
	{"nru",           TRUE,  NULL,  0 },
#define OPT_HALF_CLOSE		11
	{"half-close",    FALSE, NULL,  0 },
#define OPT_DISABLE_NAGLE	12
	{"disable-nagle", FALSE, NULL,  0 },
#define OPT_MAX			13
	{0, 0, 0, 0}
};


static void set_flag(unsigned long mask);
static void parse_and_set_timeouts(const char *str,
                                   connection_attributes *attrs);
static void print_usage(FILE *fp);


int parse_arguments(int argc, char **argv, connection_attributes *attrs)
{
	int c, verbosity_level = 0;
	bool listen_mode = FALSE;
	bool file_transfer = FALSE;
	size_t remote_mtu = 0;
	size_t remote_nru = 0;
	size_t remote_buffer_size = 0;
	size_t local_buffer_size = 0;
	int option_index = 0;

	/* set socket types to default values */
	attrs->proto = PROTO_UNSPECIFIED;
	attrs->type  = TCP_SOCKET;

	/* option recognition loop */
	while ((c = getopt_long(argc, argv, "46hlnp:q:s:uvw:x",
	                        long_options, &option_index)) >= 0)
	{
 		switch(c) {
		case 0:
			switch(option_index) {
			case OPT_BUFFER_SIZE:
				remote_buffer_size = safe_atoi(optarg);
				break;
			case OPT_MTU:
				remote_mtu = safe_atoi(optarg);
				break;
			case OPT_NRU:
				remote_nru = safe_atoi(optarg);
				break;
			case OPT_HALF_CLOSE:
				set_flag(HALF_CLOSE_MODE);
				/* keep remote open after half close */
				ios_set_hold_timeout(
					&(attrs->remote_stream), -1);
				break;
			case OPT_DISABLE_NAGLE:
				set_flag(DISABLE_NAGLE);
				break;
			default:
				fatal("getopt returned unexpected "
				      "long offset index %d\n", option_index);
			}
			break;
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
			/* set remote buffer sizes and mtu's, iff they haven't
			 * already been set */
			if (remote_mtu == 0)
				remote_mtu = DEFAULT_UDP_MTU;
			if (remote_nru == 0)
				remote_nru = DEFAULT_UDP_NRU;
			if (remote_buffer_size == 0)
				remote_buffer_size = DEFAULT_UDP_BUFFER_SIZE;
			break;
		case 'v':	
			if (++verbosity_level > 1) 
				set_flag(VERY_VERBOSE_MODE); 
			set_flag(VERBOSE_MODE); 
			break;
		case 'w':
			attrs->connect_timeout = safe_atoi(optarg);
			break;
		case 'x':	
			file_transfer = TRUE;
			break;
		case '?':
			print_usage(stderr);
			exit(EXIT_FAILURE);
		default:	
			fatal("getopt returned unexpected character 0%o\n", c);
		}
	}
	
	argv += optind;
	argc -= optind;

	/* set mode flags */
	set_flag((listen_mode)? LISTEN_MODE : CONNECT_MODE);

	/* setup file transfer depending on the mode */
	if (file_transfer == TRUE) {
		if (remote_buffer_size == 0)
			remote_buffer_size = DEFAULT_FILE_TRANSFER_BUFFER_SIZE;
		if (local_buffer_size == 0)
			local_buffer_size = DEFAULT_FILE_TRANSFER_BUFFER_SIZE;
		set_flag((listen_mode)? RECV_DATA_ONLY : SEND_DATA_ONLY);
	}

	/* check nru - if it's too big data will never be received */
	if (remote_nru > remote_buffer_size)
		remote_nru = remote_buffer_size;

	/* setup mtu, nru and buffer size if they were specified */
	if (remote_mtu > 0)
		ios_set_mtu(&(attrs->remote_stream), remote_mtu);
	if (remote_nru > 0)
		ios_set_nru(&(attrs->remote_stream), remote_nru);
	if (remote_buffer_size > 0)
		cb_resize(&(attrs->remote_buffer), remote_buffer_size);
	if (local_buffer_size > 0)
		cb_resize(&(attrs->local_buffer), local_buffer_size);

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
			warn("in listen mode you must specify a port "
			     "with the -p switch");
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
			warn("you must specify the address/port couple "
			     "of the remote endpoint");
			print_usage(stderr);
			exit(EXIT_FAILURE);
		}

		return CONNECT_MODE;
	}
}



static void print_usage(FILE *fp)
{
	const char *program_name = get_program_name();

	fprintf(fp, "Usage:\n"
"\t%s [options...] hostname port\n"
"\t%s -l -p port [-s addr] [options...] [hostname] [port]\n\n"
"Recognized options are:\n", program_name, program_name);
	fprintf(fp,	
"  -4                Use only IPv4\n"
"  -6                Use only IPv6\n"
"  -d                Disable SO_REUSEADDR socket option (only in listen mode)\n"
"  -h, --help        Display help\n"
"  -l, --listen      Listen mode, for inbound connects\n"
"  -n                Numeric-only IP addresses, no DNS\n" 
"  -p, --port=PORT   Local source port\n"
"  -q, --hold-timeouts=SEC1[:SEC2]\n"
"                    Set hold timeouts\n"
"  -s, --address=ADDRESS\n"
"                    Local source address\n"
"  -u, --udp         Require use of UDP\n"
"  -v                Increase program verbosity (call twice for max verbosity)\n"
"  -w, --timeout=SECONDS\n"
"                    Timeout for connects/accepts\n"
"  -x, --transfer    File transfer mode\n"
"      --buffer-size=BYTES\n"
"                    Set buffer size for network receives\n"
"      --mtu=BYTES   Set MTU for network connection transmits\n"
"      --nru=BYTES   Set NRU for network connection receives\n"
"      --half-close  Handle network half-closes correctly\n"
"      --disable-nagle\n"
"                    Disable nagle algorithm for TCP connections\n"
"\n");
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
