#ifndef NETWORK_H
#define NETWORK_H

#include <sys/socket.h>
#include "misc.h"

#define NUMERIC_MODE	0x0001
#define USE_UDP		0x0002
#define STRICT_IPV6	0x0004
#define REUSE_ADDR      0x0008

typedef struct address_t
{
	char *address;
	char *port;
} address;


void tcp_connect(sa_family_t family, unsigned int flags, 
		 address *remote_addr, address *local_addr);
void tcp_listen(sa_family_t family, unsigned int flags, 
	        address *remote_addr, address *local_addr);

void udp_connect(sa_family_t family, unsigned int flags, 
		 address *remote_addr, address *local_addr);
void udp_listen(sa_family_t family, unsigned int flags, 
		address *local_addr);
	
#endif /* NETWORK_H */
