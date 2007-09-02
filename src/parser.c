/*
 *  parser.c - argument parser & dispatcher module - implementation 
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

RCSID("@(#) $Header: /Users/cleishma/work/nc6-repo/nc6/src/parser.c,v 1.67.2.1 2007-09-02 13:32:44 chris Exp $");



/* default UDP MTU is 8kb */
static const size_t DEFAULT_UDP_MTU = 8192;
/* default UDP NRU is the maximum allowed MTU of 64k */
static const size_t DEFAULT_UDP_NRU = 65536;
/* default BLUETOOTH MTU is 672b */
static const size_t DEFAULT_BLUETOOTH_MTU = 672;
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
	{"help",                no_argument,        NULL, 'h'},
#define OPT_VERSION		1
	{"version",             no_argument,        NULL,  0 },
#define OPT_LISTEN		2
	{"listen",              no_argument,        NULL, 'l'},
#define OPT_PORT		3
	{"port",                required_argument,  NULL, 'p'},
#define OPT_HOLD_TIMEOUT	4
	{"hold-timeout",        required_argument,  NULL, 'q'},
#define OPT_ADDRESS		5
	{"address",             required_argument,  NULL, 's'},
#define OPT_UDP			6
	{"udp",                 no_argument,        NULL, 'u'},
#define OPT_TIMEOUT		7
	{"timeout",             required_argument,  NULL, 'w'},
#define OPT_IDLE_TIMEOUT	8
	{"idle-timeout",        required_argument,  NULL, 't'},
#define OPT_TRANSFER		9
	{"transfer",            no_argument,        NULL, 'x'},
#define OPT_REV_TRANSFER	10
	{"rev-transfer",        no_argument,        NULL, 'X'},
#define OPT_RECV_ONLY		11
	{"recv-only",           no_argument,        NULL,  0 },
#define OPT_SEND_ONLY		12
	{"send-only",           no_argument,        NULL,  0 },
#define OPT_BUFFER_SIZE		13
	{"buffer-size",         required_argument,  NULL,  0 },
#define OPT_MTU			14
	{"mtu",                 required_argument,  NULL,  0 },
#define OPT_NRU			15
	{"nru",                 required_argument,  NULL,  0 },
#define OPT_HALF_CLOSE		16
	{"half-close",          no_argument,        NULL,  0 },
#define OPT_DISABLE_NAGLE	17
	{"disable-nagle",       no_argument,        NULL,  0 },
#define OPT_NO_REUSEADDR	18
	{"no-reuseaddr",        no_argument,        NULL,  0 },
#define OPT_SNDBUF_SIZE		19
	{"sndbuf-size",         required_argument,  NULL,  0 },
#define OPT_RCVBUF_SIZE		20
	{"rcvbuf-size",         required_argument,  NULL,  0 },
#define OPT_EXEC		21
	{"exec",                required_argument,  NULL, 'e'},
#define OPT_CONTINUOUS		22
	{"continuous",          no_argument,        NULL,  0 },
#define OPT_BLUETOOTH		23
	{"bluetooth",           no_argument,        NULL, 'b'},
#define OPT_SCO			24
	{"sco",			no_argument,        NULL,  0 },
#define OPT_MAX			25
	{0, 0, 0, 0}
};



static int parse_int_pair(const char *str, int *first, int *second);
static void print_usage(FILE *fp);
static void print_version(FILE *fp);



void parse_arguments(int argc, char **argv, connection_attributes_t *attrs)
{
	int c;
	int option_index = 0;

	/* configurable parameters and default values */
	sock_family_t family = PROTO_UNSPECIFIED;
	sock_protocol_t protocol = PROTO_UNSPECIFIED;
	address_t local_address, remote_address;
	bool listen_mode = false;
	bool file_transfer = false;
	bool rev_file_transfer = false;
	bool half_close = false;
	int connect_timeout = -1;
	int idle_timeout = -1;
	bool set_local_hold_timeout = false;
	int local_hold_timeout = 0;
	bool set_remote_hold_timeout = false;
	int remote_hold_timeout = 0;
	int remote_mtu = 0;
	int remote_nru = 0;
	int buffer_size = 0;
	int sndbuf_size = 0;
	int rcvbuf_size = 0;

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
	while ((c = getopt_long(argc, argv, "46be:hlnp:q:s:uvw:xXy",
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
				if (safe_atoi(optarg, &buffer_size))
					fatal(_("invalid argument to "
					      "--buffer-size"));
				break;
			case OPT_MTU:
				assert(optarg != NULL);
				if (safe_atoi(optarg, &remote_mtu))
					fatal(_("invalid argument to --mtu"));
				break;
			case OPT_NRU:
				assert(optarg != NULL);
				if (safe_atoi(optarg, &remote_nru))
					fatal(_("invalid argument to --nru"));
				break;
			case OPT_HALF_CLOSE:
				half_close = true;
				break;
			case OPT_DISABLE_NAGLE:
				ca_set_flag(attrs, CA_DISABLE_NAGLE);
				break;
			case OPT_NO_REUSEADDR:
				ca_set_flag(attrs, CA_DONT_REUSE_ADDR);
				break;
			case OPT_SNDBUF_SIZE:
				assert(optarg != NULL);
				if (safe_atoi(optarg, &sndbuf_size))
					fatal(_("invalid argument to "
					      "--sndbuf-size"));
				break;
			case OPT_RCVBUF_SIZE:
				assert(optarg != NULL);
				if (safe_atoi(optarg, &rcvbuf_size))
					fatal(_("invalid argument to "
					      "--rcvbuf-size"));
				break;
			case OPT_CONTINUOUS:
				ca_set_flag(attrs, CA_CONTINUOUS_ACCEPT);
				break;
			case OPT_SCO:
				protocol = SCO_PROTOCOL;
				break;
			default:
				fatal_internal(
				      "getopt returned unexpected long "
				      "option offset index %d\n", option_index);
			}
			break;
		case '4':
			family = PROTO_IPv4;
			break;
		case '6':	
			family = PROTO_IPv6;
			ca_set_flag(attrs, CA_STRICT_IPV6);
			break;
		case 'b':
			family = PROTO_BLUEZ;
			break;
		case 'e':
			assert(optarg != NULL);
			ca_set_local_exec(attrs, optarg);
			break;
		case 'h':	
			print_usage(stdout);
			exit(EXIT_SUCCESS);
		case 'l':
			listen_mode = true;
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
			case 2: set_remote_hold_timeout = true; /* continue */
			case 1: set_local_hold_timeout = true; break;
			default:
				fatal(_("invalid argument to -q"));
			};
			break;	
		case 's':	
			assert(optarg != NULL);
			local_address.address = xstrdup(optarg);
			break;	
		case 't':
			assert(optarg != NULL);
			if (safe_atoi(optarg, &idle_timeout))
				fatal(_("invalid argument to -t"));
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
			if (safe_atoi(optarg, &connect_timeout))
				fatal(_("invalid argument to -w"));
			break;
		case 'x':	
			file_transfer = true;
			break;
		case 'X':	
			rev_file_transfer = true;
			break;
		case 'y':
			family = PROTO_IUCV;
			break;
		case '?':
			print_usage(stderr);
			exit(EXIT_FAILURE);
		default:	
			fatal_internal(
			      "getopt returned unexpected character 0%o\n", c);
		}
	}
	
	argv += optind;
	argc -= optind;

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

	/* set default protocols */
	if (protocol == PROTO_UNSPECIFIED) {
		switch (family) {
		case PROTO_BLUEZ:
			protocol = L2CAP_PROTOCOL;
			break;
		default:
			protocol = TCP_PROTOCOL;
			break;
		}
	}

	/* check protocol and family combinations are valid */
	if (protocol == UDP_PROTOCOL && family == PROTO_BLUEZ)
		fatal(_("cannot specify UDP protocol and bluetooth"));
	if (protocol == UDP_PROTOCOL && family == PROTO_IUCV)
		fatal(_("cannot specify UDP protocol and iucv"));
	if (protocol == SCO_PROTOCOL && family != PROTO_BLUEZ)
		fatal(_("--sco requires --bluetooth (-b)"));

	/* check compiled options */
#ifndef ENABLE_BLUEZ
	if (family == PROTO_BLUEZ)
		fatal(_("system does not support bluetooth"));
#endif
#ifndef ENABLE_IUCV
	if (family == PROTO_IUCV)
		fatal(_("system does not support iucv"));
#endif
#ifndef ENABLE_IPV6
	if (family == PROTO_IPv6)
		fatal(_("system does not support IPv6"));
#endif

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

	switch (family) {
	case PROTO_UNSPECIFIED:
	case PROTO_IPv4:
	case PROTO_IPv6:
	case PROTO_IUCV:
		if (protocol != UDP_PROTOCOL && protocol != TCP_PROTOCOL)
			fatal_internal("unknown/unsupported transport "
			               "protocol %d", protocol);
		break;
	case PROTO_BLUEZ:
		if (protocol != SCO_PROTOCOL && protocol != L2CAP_PROTOCOL)
			fatal_internal("unknown/unsupported bluetooth "
			               "protocol %d", protocol);
		break;
	default:
		fatal_internal("invalid protocol family %d", family);
	}

	/* set mode flags */
	if (listen_mode == true) {
		ca_set_flag(attrs, CA_LISTEN_MODE);
		ca_clear_flag(attrs, CA_CONNECT_MODE);
	} else {
		ca_set_flag(attrs, CA_CONNECT_MODE);
		ca_clear_flag(attrs, CA_LISTEN_MODE);
	}

	/* check to make sure the user didn't set both
	 * --transfer and --rev-transfer */
	if (file_transfer == true && rev_file_transfer == true) {
		fatal(_("cannot set both --transfer (-x) "
		      "and --rev-transfer (-X)"));
	}

	/* setup file transfer depending on the mode */
	if (file_transfer == true || rev_file_transfer == true) {
		if (buffer_size == 0)
			buffer_size = DEFAULT_FILE_TRANSFER_BUFFER_SIZE;
		if (XOR(rev_file_transfer == true, listen_mode == true)) {
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

	/* check to make sure the user didn't set both
	 * --recv-only and --send-only */
	if (ca_is_flag_set(attrs, CA_RECV_DATA_ONLY) &&
	    ca_is_flag_set(attrs, CA_SEND_DATA_ONLY))
	{
		fatal(_("cannot set both --recv-only and --send-only"));
	}

	/* check ports have not been specified with --sco */
	if (protocol == SCO_PROTOCOL) {
		if (remote_address.service != NULL)
			fatal(_("--sco does not support remote port"));
		if (local_address.service != NULL)
			fatal(_("--sco does not support local port (-p)"));
	}

	/* check mode specific option availability and interactions */
	if (listen_mode == true) {
		/* check port has been specified (except with sco) */
		if (local_address.service == NULL && protocol != SCO_PROTOCOL) {
			fatal(_("in listen mode you must specify a port "
			      "with the -p switch"));
		}
		if (ca_is_flag_set(attrs, CA_CONTINUOUS_ACCEPT) &&
		    ca_local_exec(attrs) == NULL)
		{
			fatal(_("--continuous option "
			      "must be used with --exec"));
		}
	} else {
		/* check port has been specified (except with sco) */
		if (remote_address.address == NULL || 
		    (remote_address.service == NULL &&
		    protocol != SCO_PROTOCOL))
		{
			fatal(_("you must specify the address/port couple "
			      "of the remote endpoint"));
		}
		if (ca_is_flag_set(attrs, CA_DONT_REUSE_ADDR)) {
			fatal(_("--no-reuseaddr option "
			      "can be used only in listen mode"));
		}
		if (ca_is_flag_set(attrs, CA_CONTINUOUS_ACCEPT)) {
			fatal(_("--continuous option "
			      "can be used only in listen mode"));
		}
	}

	/* set remote buffer sizes and mtu's,
	 * iff they haven't already been set */
	if (protocol == UDP_PROTOCOL) {
		if (remote_mtu == 0)
			remote_mtu = DEFAULT_UDP_MTU;
		if (remote_nru == 0)
			remote_nru = DEFAULT_UDP_NRU;
		if (buffer_size == 0)
			buffer_size = DEFAULT_UDP_BUFFER_SIZE;
	}
	if (family == PROTO_BLUEZ) {
		/* use standard bluetooth mtu */
		if (remote_mtu == 0)
			remote_mtu = DEFAULT_BLUETOOTH_MTU;
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
	if (half_close == true) {
		/* keep remote open after half close */
		ca_set_remote_half_close_suppress(attrs, false);
		ca_set_remote_hold_timeout(attrs, -1);
	}

	/* setup hold timeout */
	if (set_remote_hold_timeout == true)
		ca_set_remote_hold_timeout(attrs, remote_hold_timeout);
	if (set_local_hold_timeout == true)
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
	return ((_verbosity_level >= VERBOSE_MODE) ? true : false);
}



bool very_verbose_mode()
{
	return ((_verbosity_level >= VERY_VERBOSE_MODE) ? true : false);
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
	
	fprintf(fp, " -4                     %s\n", _("Use only IPv4"));
	fprintf(fp, " -6                     %s\n", _("Use only IPv6"));
	fprintf(fp, " -b, --bluetooth        %s\n",
	              _("Use Bluetooth (defaults to L2CAP protocol)"));
	fprintf(fp, " --buffer-size=BYTES    %s\n", _("Set buffer size"));
	fprintf(fp, " --continuous           %s\n",
	              _("Continuously accept connections\n"
"                        (only in listen mode with --exec)"));
	fprintf(fp, " --disable-nagle        %s\n",
                      _("Disable nagle algorithm for TCP connections"));
	fprintf(fp, " -e, --exec=CMD         %s\n",
	              _("Exec command after connect"));
	fprintf(fp, " --half-close           %s\n",
	              _("Handle network half-closes correctly"));
	fprintf(fp, " -h, --help             %s\n", _("Display help"));
	fprintf(fp, " -l, --listen           %s\n",
	              _("Listen mode, for inbound connects"));
	fprintf(fp, " --mtu=BYTES            %s\n",
	              _("Set MTU for network connection transmits"));
	fprintf(fp, " -n                     %s\n",
	              _("Numeric-only IP addresses, no DNS"));
	fprintf(fp, " --no-reuseaddr         %s\n",
	              _("Disable SO_REUSEADDR socket option\n"
"                        (only in listen mode)\n"));
	fprintf(fp, " --nru=BYTES            %s\n",
	              _("Set NRU for network connection receives"));
	fprintf(fp, " -p, --port=PORT        %s\n", _("Local port"));
	fprintf(fp, " -q, --hold-timeout=SEC1[:SEC2]\n"
"                        %s\n",
                      _("Set hold timeout(s) for local [and remote]"));
	fprintf(fp, " --rcvbuf-size          %s\n",
	              _("Kernel receive buffer size for network sockets"));
	fprintf(fp, " --recv-only            %s\n",
	              _("Only receive data, don't transmit"));
	fprintf(fp, " -s, --address=ADDRESS  %s\n", _("Local source address"));
	fprintf(fp, " --sco                  %s\n",
	              _("Use SCO over Bluetooth"));
	fprintf(fp, " --send-only            %s\n",
	              _("Only transmit data, don't receive"));
	fprintf(fp, " --sndbuf-size          %s\n",
	              _("Kernel send buffer size for network sockets"));
	fprintf(fp, " -t, --idle-timeout=SECONDS\n"
"                        %s\n", _("Idle connection timeout"));
	fprintf(fp, " -u, --udp              %s\n", _("Require use of UDP"));
	fprintf(fp, " -v                     %s\n",
	              _("Increase program verbosity\n"
"                        (call twice for max verbosity)"));
	fprintf(fp, " --version              %s\n",
	              _("Display nc6 version information"));
	fprintf(fp, " -w, --timeout=SECONDS  %s\n",
	              _("Timeout for connects/accepts"));
	fprintf(fp, " -x, --transfer         %s\n", _("File transfer mode"));
	fprintf(fp, " -X, --rev-transfer     %s\n",
	              _("File transfer mode (reverse direction)"));
	fprintf(fp, " -y, --iucv             %s\n",
	              _("Use IUCV (s390 Linux only)"));
	fprintf(fp, "\n");
}



static void print_version(FILE *fp)
{
	assert(fp != NULL);
	
	fprintf(fp,
"%s version %s\n"
"Copyright (C) 2001-2006\n", PACKAGE, VERSION);

	fprintf(fp, 
	"\tMauro Tortonesi\n"
	"\tChris Leishman\n"
	"\tSimone Piunno\n"
	"\tFilippo Natali\n"
"<http://www.deepspace6.net>\n");

#ifdef ENABLE_IPV6
	fprintf(fp,
_("Configured with IPv6 support\n"));
#else
	fprintf(fp,
_("Configured without IPv6 support\n"));
#endif
	
#ifdef ENABLE_BLUEZ
	fprintf(fp,
_("Configured with Bluetooth (bluez) support\n"));
#else
	fprintf(fp,
_("Configured without Bluetooth (bluez) support\n"));
#endif
	
#ifdef ENABLE_IUCV
	fprintf(fp,
_("Configured with IUCV (s390 Inter-User Communications Vehicle) support\n"));
#else
	fprintf(fp,
_("Configured without IUCV (s390 Inter-User Communications Vehicle) support\n"));
#endif
}



static int parse_int_pair(const char *str, int *first, int *second)
{
	char *s;
	int count = 1;

	assert(str != NULL);

	if ((s = strchr(str, ':')) != NULL) {
		*s++ = '\0';
		if (second != NULL) {
			if (s[0] == '-')
				*second = -1;
			else if (safe_atoi(s, second))
				return -1;
		}
		count = 2;
	}

	if (first != NULL) {
		if (str[0] == '-')
			*first = -1;
		else if (safe_atoi(str, first))
			return -1;
	}
	
	return count;
}

