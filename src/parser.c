#include "config.h"  
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "misc.h"  
#include "network.h"  
#include "parser.h"  


/* maximum length of the string representing an IP address */
#define MAX_IP_ADDRLEN sizeof("0000:0000:0000:0000:0000:0000:255.255.255.255")
#define MAX_PORTLEN    sizeof("65535")



static void do_connect(sa_family_t family, unsigned int flags, 
		       address *remote_addr, address *local_addr)
{
	if (flags & USE_UDP) {
		udp_connect(family, flags, remote_addr, local_addr);
	} else {
		tcp_connect(family, flags, remote_addr, local_addr);
	}
}



static void do_listen(sa_family_t family, unsigned int flags, 
	              address *remote_addr, address *local_addr)
{
	if (flags & USE_UDP) {
		udp_listen(family, flags, local_addr);
	} else {
		tcp_listen(family, flags, remote_addr, local_addr);
	}
}



#ifndef NDEBUG
static void print_argv(int argc, char **argv)
{
	int i;

	for (i = 0; i < argc; ++i)
		printf("arg number %d: %s\n", i, argv[i]);
}
#endif



#if 0
void bouncer_mode_parser(int argc, char **argv)
{
	argv += optind;
	argc -= optind;

	assert(argv[0] != NULL);
	assert(argv[1] != NULL);

	/* argv[0] and argv[1] must be ports */
	if (argc != 2 || 
	    isoption(argv[0]) == TRUE || 
	    isoption(argv[1]) == TRUE) {
		print_usage();
		exit(EXIT_FAILURE);
	}

	printf("call to do_bounce(%s,%s)\n", argv[0], argv[1]);
}


int parse_address(char *str, address *addr)
{
	char *s;

	assert(str  != NULL && strlen(str) > 0);
	assert(addr != NULL);

	s = strchr(str, ':');
	if (s == NULL) return -1;

	*s++ = '\0';
	addr->address = (strlen(str) == 0 ? NULL : str);
	addr->port    = (strlen(s)   == 0 ? NULL : s  );

	return 0;
}
#endif



void parse_arguments(int argc, char **argv)
{
	int c;
	char src_addr[MAX_IP_ADDRLEN + 1];
	char src_port[MAX_PORTLEN + 1];
	sa_family_t family;
	bool listen_mode;
	address local, remote;
	uint32_t flags;

	/* initialize local address, address family and other stuff 
	 * with their default values */
	local.address = NULL;
	local.port    = NULL;
	family        = AF_UNSPEC;
	listen_mode   = FALSE;
	flags         = 0;

	/* initialize to zero for correct use of getopt */
	opterr = 0;

	/* option recognition loop */
	while ((c = getopt(argc, argv, "46lnp:rs:u")) >= 0) {
 		switch(c) {
		case '4':
			if (family != AF_UNSPEC) 
				fatal("cannot specify the "
				      "address family twice");
			family = AF_INET;
			break;
		case '6':	
			if (family != AF_UNSPEC) 
				fatal("cannot specify the "
				      "address family twice");
			family = AF_INET6;
			flags |= STRICT_IPV6;
			break;
		case 'l':
			listen_mode = TRUE;
			break;
		case 'n':	
			flags |= NUMERIC_MODE;
			break;
		case 'p':	
			strncpy(src_port, optarg, sizeof(src_port) - 1);
			src_port[sizeof(src_port) - 1] = '\0';
			local.port = src_port;
			break;	
		case 'r':	
			flags |= REUSE_ADDR;
			break;
		case 's':	
			strncpy(src_addr, optarg, sizeof(src_addr) - 1);
			src_addr[sizeof(src_addr) - 1] = '\0';
			local.address = src_addr;
			break;	
		case 'u':	
			flags |= USE_UDP;
			break;
		default:	
			print_usage(stderr);
			exit(EXIT_FAILURE);
		}
	}

	argv += optind;
	argc -= optind;

	if (listen_mode == TRUE) {
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

		assert(remote.address == NULL || strlen(remote.address) > 0);
		assert(remote.port == NULL || strlen(remote.port) > 0);
		assert(local.port != NULL);

		do_listen(family, flags, &remote, &local);
	} else {
		/* argv[0] and argv[1] must be address and port */
		if (argc != 2) {
			print_usage(stderr);
			exit(EXIT_FAILURE);
		}

		assert(argv[0] != NULL && strlen(argv[0]) > 0);
		assert(argv[1] != NULL && strlen(argv[1]) > 0);

		/* call do_connect */
		remote.address = argv[0];
		remote.port    = argv[1];

		do_connect(family, flags, &remote, &local);
	}
}

