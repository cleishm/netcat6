/*
 *  parser.c - argument parser & dispatcher module - implementation 
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

RCSID("@(#) $Header: /Users/cleishma/work/nc6-repo/nc6/src/parser.c,v 1.49 2003-03-16 11:53:40 chris Exp $");



/* default UDP MTU is 8kb */
static const size_t DEFAULT_UDP_MTU = 8192;
/* default UDP NRU is the maximum allowed MTU of 64k */
static const size_t DEFAULT_UDP_NRU = 65536;
/* default UDP buffer size is 128k */
static const size_t DEFAULT_UDP_BUFFER_SIZE = 131072;
/* default buffer size for file transfers is 64k */
static const size_t DEFAULT_FILE_TRANSFER_BUFFER_SIZE = 65536;

/* these *VERBOSE* constants are defined here because they are not used 
 * in any other module */
static const int VERBOSE_MODE      = 0x01;
static const int VERY_VERBOSE_MODE = 0x02;

/* storage for the global flags */
static int _verbosity_level = 0;

/* long options */
static const struct option long_options[] = {
#define OPT_HELP		0
	{"help",                FALSE, NULL, 'h'},
#define OPT_VERSION		1
	{"version",             FALSE, NULL,  0 },
#define OPT_LISTEN		2
	{"listen",              FALSE, NULL, 'l'},
#define OPT_PORT		3
	{"port",                TRUE,  NULL, 'p'},
#define OPT_HOLD_TIMEOUT	4
	{"hold-timeout",        TRUE,  NULL, 'q'},
#define OPT_ADDRESS		5
	{"address",             TRUE,  NULL, 's'},
#define OPT_UDP			6
	{"udp",                 FALSE, NULL, 'u'},
#define OPT_TIMEOUT		7
	{"timeout",             TRUE,  NULL, 'w'},
#define OPT_IDLE_TIMEOUT	8
	{"idle-timeout",        TRUE,  NULL, 't'},
#define OPT_TRANSFER		9
	{"transfer",            FALSE, NULL, 'x'},
#define OPT_RECV_ONLY		10
	{"recv-only",           FALSE, NULL,  0 },
#define OPT_SEND_ONLY		11
	{"send-only",           FALSE, NULL,  0 },
#define OPT_BUFFER_SIZE		12
	{"buffer-size",         TRUE,  NULL,  0 },
#define OPT_MTU			13
	{"mtu",                 TRUE,  NULL,  0 },
#define OPT_NRU			14
	{"nru",                 TRUE,  NULL,  0 },
#define OPT_HALF_CLOSE		15
	{"half-close",          FALSE, NULL,  0 },
#define OPT_DISABLE_NAGLE	16
	{"disable-nagle",       FALSE, NULL,  0 },
#define OPT_NO_REUSEADDR	17
	{"no-reuseaddr",        FALSE, NULL,  0 },
#define OPT_SNDBUF_SIZE		18
	{"sndbuf-size",         TRUE,  NULL,  0 },
#define OPT_RCVBUF_SIZE		19
	{"rcvbuf-size",         TRUE,  NULL,  0 },
#define OPT_MAX			20
	{0, 0, 0, 0}
};


static int parse_int_pair(const char *str, int *first, int *second);
static void print_usage(FILE *fp);
static void print_version(FILE *fp);


void parse_arguments(int argc, char **argv, connection_attributes *attrs)
{
	int c;
	int option_index = 0;

	/* configurable parameters and default values */
	sock_family family = PROTO_UNSPECIFIED;
	sock_protocol protocol = TCP_PROTOCOL;
	address local_address, remote_address;
	bool listen_mode = FALSE;
	bool file_transfer = FALSE;
	bool half_close = FALSE;
	int connect_timeout = -1;
	int idle_timeout = -1;
	bool set_local_hold_timeout = FALSE;
	int local_hold_timeout;
	bool set_remote_hold_timeout = FALSE;
	int remote_hold_timeout;
	size_t remote_mtu = 0;
	size_t remote_nru = 0;
	size_t buffer_size = 0;
	size_t sndbuf_size = 0;
	size_t rcvbuf_size = 0;

	/* check arguments */
	assert(argc > 0);
	assert(argv != NULL);
	assert(*argv != NULL);
	assert(attrs != NULL);
	
	/* initialize the addresses of the connection endpoints */
	address_init(&remote_address);
	address_init(&local_address);

	/* set verbosity back to 0 */
	_verbosity_level = 0;

	/* option recognition loop */
	while ((c = getopt_long(argc, argv, "46hlnp:q:s:uvw:x",
	                        long_options, &option_index)) >= 0)
	{
 		switch (c) {
		case 0:
			switch (option_index) {
			case OPT_VERSION:
				print_version(stdout);
				exit(EXIT_SUCCESS);
			case OPT_RECV_ONLY:
				ca_set_flag(attrs, CA_RECV_DATA_ONLY);
				break;
			case OPT_SEND_ONLY:
				ca_set_flag(attrs, CA_SEND_DATA_ONLY);
				break;
			case OPT_BUFFER_SIZE:
				assert(optarg != NULL);
				buffer_size = safe_atoi(optarg);
				break;
			case OPT_MTU:
				assert(optarg != NULL);
				remote_mtu = safe_atoi(optarg);
				break;
			case OPT_NRU:
				assert(optarg != NULL);
				remote_nru = safe_atoi(optarg);
				break;
			case OPT_HALF_CLOSE:
				half_close = TRUE;
				break;
			case OPT_DISABLE_NAGLE:
				ca_set_flag(attrs, CA_DISABLE_NAGLE);
				break;
			case OPT_NO_REUSEADDR:
				ca_set_flag(attrs, CA_DONT_REUSE_ADDR);
				break;
			case OPT_SNDBUF_SIZE:
				assert(optarg != NULL);
				sndbuf_size = safe_atoi(optarg);
				break;
			case OPT_RCVBUF_SIZE:
				assert(optarg != NULL);
				rcvbuf_size = safe_atoi(optarg);
				break;
			default:
				fatal(_("getopt returned unexpected long "
				      "option offset index %d\n"), option_index);
			}
			break;
		case '4':
			family = PROTO_IPv4;
			break;
		case '6':	
			family = PROTO_IPv6;
			ca_set_flag(attrs, CA_STRICT_IPV6);
			break;
		case 'h':	
			print_usage(stdout);
			exit(EXIT_SUCCESS);
		case 'l':
			listen_mode = TRUE;
			break;
		case 'n':	
			ca_set_flag(attrs, CA_NUMERIC_MODE);
			break;
		case 'p':	
			assert(optarg != NULL);
			local_address.service = xstrdup(optarg);
			break;	
		case 'q':
			assert(optarg != NULL);
			switch (parse_int_pair(optarg, &local_hold_timeout,
			                       &remote_hold_timeout))
			{
			case 2: set_remote_hold_timeout = TRUE;
			case 1: set_local_hold_timeout = TRUE;
			};
			break;	
		case 's':	
			assert(optarg != NULL);
			local_address.address = xstrdup(optarg);
			break;	
		case 't':
			assert(optarg != NULL);
			idle_timeout = safe_atoi(optarg);
			break;
		case 'u':	
			protocol = UDP_PROTOCOL;
			/* set remote buffer sizes and mtu's, iff they haven't
			 * already been set */
			if (remote_mtu == 0)
				remote_mtu = DEFAULT_UDP_MTU;
			if (remote_nru == 0)
				remote_nru = DEFAULT_UDP_NRU;
			if (buffer_size == 0)
				buffer_size = DEFAULT_UDP_BUFFER_SIZE;
			break;
		case 'v':	
			++_verbosity_level;
			break;
		case 'w':
			assert(optarg != NULL);
			connect_timeout = safe_atoi(optarg);
			break;
		case 'x':	
			file_transfer = TRUE;
			break;
		case '?':
			print_usage(stderr);
			exit(EXIT_FAILURE);
		default:	
			fatal(_("getopt returned unexpected character 0%o\n"), c);
		}
	}
	
	argv += optind;
	argc -= optind;

	/* set mode flags */
	if (listen_mode == TRUE) {
		ca_set_flag(attrs, CA_LISTEN_MODE);
		ca_clear_flag(attrs, CA_CONNECT_MODE);
	} else {
		ca_set_flag(attrs, CA_CONNECT_MODE);
		ca_clear_flag(attrs, CA_LISTEN_MODE);
	}

	/* setup file transfer depending on the mode */
	if (file_transfer == TRUE) {
		if (buffer_size == 0)
			buffer_size = DEFAULT_FILE_TRANSFER_BUFFER_SIZE;
		if (listen_mode == TRUE) {
			ca_set_flag(attrs, CA_RECV_DATA_ONLY);
			ca_clear_flag(attrs, CA_SEND_DATA_ONLY);
		} else {
			ca_set_flag(attrs, CA_SEND_DATA_ONLY);
			ca_clear_flag(attrs, CA_RECV_DATA_ONLY);
		}
	}

	/* check nru - if it's too big data will never be received */
	if (remote_nru > buffer_size)
		remote_nru = buffer_size;

	/* check to make sure the user wasn't silly enough to set both
	 * --recv-only and --send-only */
	if (ca_is_flag_set(attrs, CA_RECV_DATA_ONLY) &&
	    ca_is_flag_set(attrs, CA_SEND_DATA_ONLY))
	{
		fatal(_("Cannot set both --recv-only and --send-only"));
	}

	/* additional arguments are the remote address/service */
	switch (argc) {
	case 0:
		remote_address.address = NULL;
		remote_address.service = NULL;
		break;
	case 1:
		remote_address.address = argv[0];
		remote_address.service = NULL;
		break;
	case 2:
		remote_address.address = argv[0];
		remote_address.service = argv[1];
		break;
	default:
		print_usage(stderr);
		exit(EXIT_FAILURE);
	}

	/* sanity checks */
	if (remote_address.address != NULL && 
	    strlen(remote_address.address) == 0)
	{
		remote_address.address = NULL;
	}
	if (remote_address.service != NULL &&
	    strlen(remote_address.service) == 0)
	{
		remote_address.service = NULL;
	}

	if (listen_mode == TRUE) {
		if (local_address.service == NULL) {
			warn(_("in listen mode you must specify a port "
			       "with the -p switch"));
			print_usage(stderr);
			exit(EXIT_FAILURE);
		}
	} else {
		if (ca_is_flag_set(attrs, CA_DONT_REUSE_ADDR)) {
			warn(_("--no-reuseaddr option "
			       "can be used only in listen mode"));
			print_usage(stderr);
			exit(EXIT_FAILURE);
		}
		
		if (remote_address.address == NULL ||
		    remote_address.service == NULL)
		{
			warn(_("you must specify the address/port couple "
			       "of the remote endpoint"));
			print_usage(stderr);
			exit(EXIT_FAILURE);
		}
	}

	/* setup attrs */
	ca_set_family(attrs, family);
	ca_set_protocol(attrs, protocol);
	ca_set_remote_addr(attrs, remote_address);
	ca_set_local_addr(attrs, local_address);

	/* setup connection timeout */
	if (connect_timeout != -1)
		ca_set_connect_timeout(attrs, connect_timeout);

	/* setup idle timeout */
	if (idle_timeout != -1)
		ca_set_idle_timeout(attrs, idle_timeout);
	
	/* setup half close mode */
	if (half_close == TRUE) {
		/* keep remote open after half close */
		ca_set_remote_half_close_suppress(attrs, FALSE);
		ca_set_remote_hold_timeout(attrs, -1);
	}

	/* setup hold timeout */
	if (set_remote_hold_timeout == TRUE)
		ca_set_remote_hold_timeout(attrs, remote_hold_timeout);
	if (set_local_hold_timeout == TRUE)
		ca_set_local_hold_timeout(attrs, local_hold_timeout);
	
	/* setup mtu, nru, and buffer sizes if they were specified */
	if (remote_mtu > 0)
		ca_set_remote_MTU(attrs, remote_mtu);
	if (remote_nru > 0)
		ca_set_remote_NRU(attrs, remote_nru);
	if (buffer_size > 0)
		ca_set_buffer_size(attrs, buffer_size);
	if (sndbuf_size > 0)
		ca_set_sndbuf_size(attrs, sndbuf_size);
	if (rcvbuf_size > 0)
		ca_set_rcvbuf_size(attrs, rcvbuf_size);
}



bool verbose_mode()
{
	return ((_verbosity_level >= VERBOSE_MODE) ? TRUE : FALSE);
}



bool very_verbose_mode()
{
	return ((_verbosity_level >= VERY_VERBOSE_MODE) ? TRUE : FALSE);
}



static void print_usage(FILE *fp)
{
	const char *program_name = get_program_name();

	assert(fp != NULL);
	assert(program_name != NULL);
	
	fprintf(fp, _("Usage:\n"
"\t%s [options...] hostname port\n"
"\t%s -l -p port [-s addr] [options...] [hostname] [port]\n\n"
"Recognized options are:\n"), program_name, program_name);
	
	fprintf(fp, _(
"  -4                Use only IPv4\n"
"  -6                Use only IPv6\n"
"  -h, --help        Display help\n"
"  -l, --listen      Listen mode, for inbound connects\n"
"  -n                Numeric-only IP addresses, no DNS\n" 
"  -p, --port=PORT   Local source port\n"
"  -q, --hold-timeout=SEC1[:SEC2]\n"
"                    Set hold timeout(s) for local [and remote]\n"
"  -s, --address=ADDRESS\n"
"                    Local source address\n"
"  -t, --idle-timeout=SECONDS\n"
"                    Idle connection timeout\n"
"  -u, --udp         Require use of UDP\n"
"  -v                Increase program verbosity (call twice for max verbosity)\n"
"  -w, --timeout=SECONDS\n"
"                    Timeout for connects/accepts\n"
"  -x, --transfer    File transfer mode\n"
"      --recv-only   Only receive data, don't transmit\n"
"      --send-only   Only transmit data, don't receive\n"
"      --buffer-size=BYTES\n"
"                    Set buffer size\n"
"      --mtu=BYTES   Set MTU for network connection transmits\n"
"      --nru=BYTES   Set NRU for network connection receives\n"
"      --half-close  Handle network half-closes correctly\n"
"      --disable-nagle\n"
"                    Disable nagle algorithm for TCP connections\n"
"      --no-reuseaddr\n"
"                    Disable SO_REUSEADDR socket option (only in listen mode)\n"
"      --sndbuf-size Kernel send buffer size for network sockets\n"
"      --rcvbuf-size Kernel receive buffer size for network sockets\n"
"      --version     Display nc6 version information\n"
"\n"));
}



static void print_version(FILE *fp)
{
	assert(fp != NULL);
	
	fprintf(fp, _(
"%s version %s\n"
"Copyright (C) 2001-2003\n"), PACKAGE, VERSION);

	fprintf(fp, 
	"\tMauro Tortonesi\n"
	"\tChris Leishman\n"
	"\tSimone Piunno\n"
"<http://www.deepspace6.net>\n");

#ifdef ENABLE_IPV6
	fprintf(fp,
_("Configured with IPv6 support\n"));
#else
	fprintf(fp,
_("Configured without IPv6 support\n"));
#endif
}



static int parse_int_pair(const char *str, int *first, int *second)
{
	char *s;
	int count = 1;

	assert(str != NULL);

	if ((s = strchr(str, ':')) != NULL) {
		*s++ = '\0';
		if (second != NULL)
			*second = (s[0] == '-')? -1 : safe_atoi(s);
		count = 2;
	}

	if (first != NULL)
		*first = (str[0] == '-')? -1 : safe_atoi(str);
	
	return count;
}

