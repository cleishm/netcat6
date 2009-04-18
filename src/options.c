/*
 *  options.c - argument parser module - implementation 
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
#include "options.h"  
#include "connection.h"  
#include "misc.h"  

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <getopt.h>

RCSID("@(#) $Header: /Users/cleishma/work/nc6-repo/nc6/src/options.c,v 1.1 2009-04-18 11:39:35 chris Exp $");



/* long options */
static const struct option options[] = {
#define OPT_HELP                0
	{"help",                no_argument,        NULL, 'h' },
#define OPT_VERSION             1
	{"version",             no_argument,        NULL, 0 },
#define OPT_VERBOSE             2
	{"verbose",             no_argument,        NULL, 'v' },
#define OPT_LISTEN              3
	{"listen",              no_argument,        NULL, 'l' },
#define OPT_ADDRESS             4
	{"address",             required_argument,  NULL, 's' },
#define OPT_PORT                5
	{"port",                required_argument,  NULL, 'p' },
#define OPT_IPV4                6
	{"ipv4",                no_argument,        NULL, '4' },
#define OPT_IPV6                7
	{"ipv6",                no_argument,        NULL, '6' },
#define OPT_BLUETOOTH           8
	{"bluetooth",           no_argument,        NULL, 'b' },
#define OPT_UDP                 9
	{"udp",                 no_argument,        NULL, 'u' },
#define OPT_SCO                 10
	{"sco",                 no_argument,        NULL, 0 },
#define OPT_SOCKTYPE            11
	{"socktype",            required_argument,  NULL, 0 },
#define OPT_TIMEOUT             12
	{"timeout",             required_argument,  NULL, 'w' },
#define OPT_IDLE_TIMEOUT        13
	{"idle-timeout",        required_argument,  NULL, 't' },
#define OPT_HOLD_TIMEOUT        14
	{"hold-timeout",        required_argument,  NULL, 'q' },
#define OPT_TRANSFER            15
	{"transfer",            no_argument,        NULL, 'x' },
#define OPT_REV_TRANSFER        16
	{"rev-transfer",        no_argument,        NULL, 'X' },
#define OPT_RECV_ONLY           17
	{"recv-only",           no_argument,        NULL, 0 },
#define OPT_SEND_ONLY           18
	{"send-only",           no_argument,        NULL, 0 },
#define OPT_BUFFER_SIZE         19
	{"buffer-size",         required_argument,  NULL, 0 },
#define OPT_MTU                 20
	{"mtu",                 required_argument,  NULL, 0 },
#define OPT_NRU                 21
	{"nru",                 required_argument,  NULL, 0 },
#define OPT_HALF_CLOSE          22
	{"half-close",          no_argument,        NULL, 'S' },
#define OPT_NO_DNS              23
	{"no-dns",              no_argument,        NULL, 'n' },
#define OPT_DISABLE_NAGLE       24
	{"disable-nagle",       no_argument,        NULL, 0 },
#define OPT_NO_REUSEADDR        25
	{"no-reuseaddr",        no_argument,        NULL, 0 },
#define OPT_SNDBUF_SIZE         26
	{"sndbuf-size",         required_argument,  NULL, 0 },
#define OPT_RCVBUF_SIZE         27
	{"rcvbuf-size",         required_argument,  NULL, 0 },
#define OPT_EXEC                28
	{"exec",                required_argument,  NULL, 'e' },
#define OPT_CONTINUOUS          29
	{"continuous",          no_argument,        NULL, 0 },
#define OPT_MAX                 30
	{NULL, 0, NULL, 0}
};

/* buffer size for file transfers is 64k */
static const size_t FILE_TRANSFER_BUFFER_SIZE = 65536;
/* bluetooth MTU is 672b */
static const size_t BLUETOOTH_MTU = 672;


static void build_shortopts(const struct option *options,
		char *buffer, size_t buflen);
static int find_long_option(const struct option *options, char short_opt);
static void invalid_argument(int opt_index);
static int optarg_atoi(int opt_index);
static int parse_int_pair(const char *str, int *first, int *second);
static void print_usage(FILE *fp);
static void print_version(FILE *fp);



void parse_arguments(int argc, char **argv, connection_attributes_t *attrs)
{
	char shortopts[256];
	int c;
	int opt_index = 0;
	char *local_nodename = NULL;
	char *local_service = NULL;
	address_t local_address, remote_address;
	int i1, i2;
	bool file_transfer = false;
	bool rev_file_transfer = false;
	bool buffer_size_set = false;
	bool remote_hold_timeout_set = true;

	/* check arguments */
	assert(argc > 0);
	assert(argv != NULL);
	assert(*argv != NULL);
	assert(attrs != NULL);
	
	/* initialize the addresses of the connection endpoints */
	address_init(&remote_address);
	address_init(&local_address);

	/* set verbosity back to 0 */
	set_verbosity_level(0);

	build_shortopts(options, shortopts, sizeof(shortopts));

	/* option recognition loop */
	while ((c = getopt_long(argc, argv, shortopts,
	        options, &opt_index)) >= 0)
        {
                switch (c) {
                case '?':
                        print_usage(stderr);
                        exit(EXIT_FAILURE);
                case 0:
                        /* was already a long opt */
                        break;
                default:        
                        opt_index = find_long_option(options, c);
                        if (opt_index < 0) {
                                fatal_internal("getopt returned "
                                               "unexpected character 0%o\n", c);
                        }
                }

                switch (opt_index) {
                case OPT_HELP:
                        print_usage(stdout);
                        exit(EXIT_SUCCESS);
                case OPT_VERSION:
                        print_version(stdout);
                        exit(EXIT_SUCCESS);
                case OPT_VERBOSE:
                        set_verbosity_level(verbosity_level() + 1);
                        break;
                case OPT_LISTEN:
                        ca_set_flag(attrs, CA_PASSIVE);
                        break;
                case OPT_ADDRESS:
                        assert(optarg != NULL);
                        if (strlen(optarg) == 0)
                                invalid_argument(opt_index);
                        if (local_nodename != NULL)
                                free(local_nodename);
                        local_nodename = xstrdup(optarg);
                        local_address.nodename = local_nodename;
                        ca_set_local_addr(attrs, local_address);
                        break;  
                case OPT_PORT:
                        assert(optarg != NULL);
                        if (strlen(optarg) == 0)
                                invalid_argument(opt_index);
                        if (local_service != NULL)
                                free(local_service);
                        local_service = xstrdup(optarg);
                        local_address.service = local_service;
                        ca_set_local_addr(attrs, local_address);
                        break;  
                case OPT_IPV4:
                        ca_set_family(attrs, PF_INET);
                        break;
                case OPT_IPV6:
#ifdef ENABLE_IPV6
                        ca_set_family(attrs, PF_INET6);
#else
                        fatal(_("system does not support IPv6"));
#endif
                        break;
                case OPT_BLUETOOTH:
#ifdef ENABLE_BLUEZ
                        ca_set_family(attrs, PF_BLUETOOTH);
                        if (buffer_size_set == false)
                                ca_set_remote_MTU(attrs, BLUETOOTH_MTU);
#else
                        fatal(_("system does not support bluetooth"));
#endif
                        break;
                case OPT_UDP:
                        ca_set_protocol(attrs, IPPROTO_UDP);
                        break;
                case OPT_SCO:
#ifdef ENABLE_BLUEZ
                        ca_set_protocol(attrs, BTPROTO_SCO);
#else
                        fatal(_("system does not support bluetooth"));
#endif
                        break;
		case OPT_SOCKTYPE:
			if (strcmp(optarg, "stream") == 0) {
				ca_set_socktype(attrs, SOCK_STREAM);
			} else if (strcmp(optarg, "dgram") == 0) {
				ca_set_socktype(attrs, SOCK_DGRAM);
			} else if (strcmp(optarg, "seqpacket") == 0) {
				ca_set_socktype(attrs, SOCK_SEQPACKET);
			} else {
				fatal(_("unknown socket type "
				      "specified for --socktype"));
			}
                        break;
                case OPT_TIMEOUT:
                        ca_set_connect_timeout(attrs, optarg_atoi(opt_index));
                        break;
                case OPT_IDLE_TIMEOUT:
                        ca_set_idle_timeout(attrs, optarg_atoi(opt_index));
                        break;
                case OPT_HOLD_TIMEOUT:
                        assert(optarg != NULL);
                        switch (parse_int_pair(optarg, &i1, &i2))
                        {
                        case 2:
                                ca_set_remote_hold_timeout(attrs, i2);
                                remote_hold_timeout_set = true;
                                /* continue */
                        case 1: 
                                ca_set_local_hold_timeout(attrs, i1);
                                break;
                        default:
                                invalid_argument(opt_index);
                        };
                        break;  
                case OPT_TRANSFER:
                        file_transfer = true;
                        break;
                case OPT_REV_TRANSFER:
                        rev_file_transfer = true;
                        break;
                case OPT_RECV_ONLY:
                        ca_set_flag(attrs, CA_RECV_DATA_ONLY);
                        break;
                case OPT_SEND_ONLY:
                        ca_set_flag(attrs, CA_SEND_DATA_ONLY);
                        break;
                case OPT_BUFFER_SIZE:
                        ca_set_buffer_size(attrs, optarg_atoi(opt_index));
                        buffer_size_set = true;
                        break;
                case OPT_MTU:
                        ca_set_remote_MTU(attrs, optarg_atoi(opt_index));
                        break;
                case OPT_NRU:
                        ca_set_remote_NRU(attrs, optarg_atoi(opt_index));
                        break;
                case OPT_HALF_CLOSE:
                        /* keep remote open after half close */
                        ca_set_remote_half_close_suppress(attrs, false);
                        if (remote_hold_timeout_set == false) {
                                ca_set_remote_hold_timeout(attrs, -1);
                        }
                        break;
                case OPT_NO_DNS:
                        ca_set_flag(attrs, CA_NUMERICHOST);
                        break;
                case OPT_DISABLE_NAGLE:
                        ca_set_flag(attrs, CA_DISABLE_NAGLE);
                        break;
                case OPT_NO_REUSEADDR:
                        ca_set_flag(attrs, CA_DONT_REUSE_ADDR);
                        break;
                case OPT_SNDBUF_SIZE:
                        ca_set_sndbuf_size(attrs, optarg_atoi(opt_index));
                        break;
                case OPT_RCVBUF_SIZE:
                        ca_set_rcvbuf_size(attrs, optarg_atoi(opt_index));
                        break;
                case OPT_EXEC:
                        assert(optarg != NULL);
                        ca_set_local_exec(attrs, optarg);
                        break;
                case OPT_CONTINUOUS:
                        ca_set_flag(attrs, CA_CONTINUOUS_ACCEPT);
                        break;
                default:
                        fatal_internal(
                              "getopt returned unexpected long "
                              "option offset index %d\n", opt_index);
                }
        }
        
        argv += optind;
        argc -= optind;

        /* additional arguments are the remote address/service */
        switch (argc) {
        case 0:
                remote_address.nodename = NULL;
                remote_address.service = NULL;
                break;
        case 1:
                remote_address.nodename = non_empty_string(argv[0]);
                remote_address.service = NULL;
                break;
        case 2:
                remote_address.nodename = non_empty_string(argv[0]);
                remote_address.service = non_empty_string(argv[1]);
                break;
        default:
                print_usage(stderr);
                exit(EXIT_FAILURE);
        }

        ca_set_remote_addr(attrs, remote_address);

        /* check to make sure the user didn't set both
         * --transfer and --rev-transfer */
        if (file_transfer == true && rev_file_transfer == true) {
                fatal(_("cannot set both --transfer (-x) "
                      "and --rev-transfer (-X)"));
        }

        /* --transfer and --rev-transfer and shortcuts for
         * --send-only or --recv-only (depending on mode) and --buffer-size
         * (if not already set) */
        if (file_transfer == true || rev_file_transfer == true) {
                if (XOR(rev_file_transfer == true,
                        ca_is_flag_set(attrs, CA_PASSIVE)))
                {
                        ca_set_flag(attrs, CA_RECV_DATA_ONLY);
                        ca_clear_flag(attrs, CA_SEND_DATA_ONLY);
                } else {
                        ca_set_flag(attrs, CA_SEND_DATA_ONLY);
                        ca_clear_flag(attrs, CA_RECV_DATA_ONLY);
                }
                if (buffer_size_set == false)
                        ca_set_buffer_size(attrs, FILE_TRANSFER_BUFFER_SIZE);
        }

        /* check to make sure the user didn't set both
         * --recv-only and --send-only */
        if (ca_is_flag_set(attrs, CA_RECV_DATA_ONLY) &&
            ca_is_flag_set(attrs, CA_SEND_DATA_ONLY))
        {
                fatal(_("cannot set both --recv-only and --send-only"));
        }

        /* check family/protocol specific requirements */
#ifdef ENABLE_BLUEZ
        if (ca_protocol(attrs) == BTPROTO_SCO) {
                /* sco doesn't support services/ports */
                if (remote_address.service != NULL)
                        fatal(_("--sco does not support remote port"));
                if (local_address.service != NULL)
                        fatal(_("--sco does not support --port (-p)"));
                remote_address.service = "";
                local_address.service = "";
        }
#endif

        /* check options that are only valid with --listen */
        if (ca_is_flag_set(attrs, CA_PASSIVE)) {
                if (local_address.service == NULL)
                        fatal(_("in listen mode you must specify a service "
                              "with the -p switch"));
        } else {
                if (remote_address.nodename == NULL)
                        fatal(_("you must specify the address of "
                              "the remote endpoint"));
                if (remote_address.service == NULL)
                        fatal(_("you must specify the service of "
                               "the remote endpoint"));
                if (ca_is_flag_set(attrs, CA_DONT_REUSE_ADDR))
                        fatal(_("--no-reuseaddr option "
                              "can be used only with --listen (-l)"));
                if (ca_is_flag_set(attrs, CA_CONTINUOUS_ACCEPT))
                        fatal(_("--continuous option "
                              "can be used only with --listen (-l)"));
        }

        /* --continuous depends on --exec */
        if (ca_is_flag_set(attrs, CA_CONTINUOUS_ACCEPT) &&
            ca_local_exec(attrs) == NULL)
        {
                fatal(_("--continuous option must be used with --exec"));
        }
}



static void build_shortopts(const struct option *options,
		char *buffer, size_t buflen)
{
        const struct option *option;
        unsigned int i;

        for (option = options, i=0;
			i < (buflen-2) && option->name != NULL; ++option) {
                if (option->val != 0) {
                        buffer[i++] = option->val;
                        if (option->has_arg == required_argument)
                                buffer[i++] = ':';
                }
        }
	if (option->name != NULL || i >= buflen) {
		fatal_internal("insufficient options buffer space");
	}
        buffer[i] = '\0';
}



static int find_long_option(const struct option *options, char short_opt)
{
        int i;
        for (i=0; options[i].name != NULL && options[i].val != short_opt; ++i)
                /* no body */;
        return (options[i].name == NULL)? -1 : i;
}



static void invalid_argument(int opt_index)
{
        char shortopt_desc[6];
        if (options[opt_index].val != 0) {
                snprintf(shortopt_desc, sizeof(shortopt_desc),
                         " (-%c)", options[opt_index].val);
        } else {
                shortopt_desc[0] = '\0';
        }
        fatal(_("invalid argument to --%s%s"),
              options[opt_index].name, shortopt_desc);
}


static int optarg_atoi(int opt_index)
{
        int out;
        assert(optarg != NULL);
        if (safe_atoi(optarg, &out))
                invalid_argument(opt_index);
        return out;
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
        
        fprintf(fp, " -4, --ipv4             %s\n", _("Use only IPv4"));
        fprintf(fp, " -6, --ipv6             %s\n", _("Use only IPv6"));
        fprintf(fp, " -a, --any-protocol     %s\n",
                        _("Use any available protocol (default is TCP)"));
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
"                        (only in listen mode)"));
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
                      _("Use SCO protocol over Bluetooth"));
        fprintf(fp, " --send-only            %s\n",
                      _("Only transmit data, don't receive"));
        fprintf(fp, " --sndbuf-size          %s\n",
                      _("Kernel send buffer size for network sockets"));
        fprintf(fp, " --socktype=[stream|dgram|seqpacket]"
"                        %s\n",
                      _("Socket type to use. Default is stream."));
        fprintf(fp, " -t, --idle-timeout=SECONDS\n"
"                        %s\n", _("Idle connection timeout"));
        fprintf(fp, " -u, --udp              %s\n",
                      _("Require use of UDP protocol (implies -d)"));
        fprintf(fp, " -v, --verbose          %s\n",
                      _("Increase program verbosity\n"
"                        (call twice for max verbosity)"));
        fprintf(fp, " --version              %s\n",
                      _("Display nc6 version information"));
        fprintf(fp, " -w, --timeout=SECONDS  %s\n",
                      _("Timeout for connects/accepts"));
        fprintf(fp, " -x, --transfer         %s\n", _("File transfer mode"));
        fprintf(fp, " -X, --rev-transfer     %s\n",
                      _("File transfer mode (reverse direction)"));
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

