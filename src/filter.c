#include <assert.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "filter.h"
#include "misc.h"


/* compare two sockaddr structs to see if they represent the same address */
bool are_address_equal(const struct sockaddr *a, 
	               const struct sockaddr *b)
{
    struct sockaddr *aa, *bb;
    
    assert(a != NULL);
    assert(b != NULL);
    assert(a->sa_family == AF_INET || a->sa_family == AF_INET6);
    assert(b->sa_family == AF_INET || b->sa_family == AF_INET6);
    
    aa = (struct sockaddr *)a;
    bb = (struct sockaddr *)b;
    
    /* we have to handle those --damned-- IPV4MAPPED addresses */
    if (aa->sa_family != bb->sa_family) {
    	if (a->sa_family == AF_INET6 && 
            IN6_IS_ADDR_V4MAPPED(&((struct sockaddr_in6 *)a)->sin6_addr)) {
	        aa = (struct sockaddr *)alloca(sizeof(struct sockaddr_in));
	        memset(aa, 0, sizeof(struct sockaddr_in));
		memcpy(&(((struct sockaddr_in *)aa)->sin_addr.s_addr),
	    	       &(((struct sockaddr_in6 *)a)->sin6_addr.s6_addr[12]),
		       sizeof(struct in_addr));
	        ((struct sockaddr_in *)aa)->sin_port = 
		    ((struct sockaddr_in6 *)a)->sin6_port;
        } else if (b->sa_family == AF_INET6 && 
            IN6_IS_ADDR_V4MAPPED(&((struct sockaddr_in6 *)b)->sin6_addr)) {
	        bb = (struct sockaddr *)alloca(sizeof(struct sockaddr_in));
	        memset(bb, 0, sizeof(struct sockaddr_in));
		memcpy(&(((struct sockaddr_in *)bb)->sin_addr.s_addr),
	    	       &(((struct sockaddr_in6 *)b)->sin6_addr.s6_addr[12]),
		       sizeof(struct in_addr));
	        ((struct sockaddr_in *)bb)->sin_port = 
		    ((struct sockaddr_in6 *)b)->sin6_port;
        } else {
            return FALSE;
	}
    }

    /* now we can perform the comparison */
    if (aa->sa_family == AF_INET6) {
    	if (memcmp(&((struct sockaddr_in6 *)aa)->sin6_addr, 
		   &((struct sockaddr_in6 *)bb)->sin6_addr, 
		   sizeof(struct sockaddr_in6)) != 0) return FALSE;
    } else { 
    	if (((struct sockaddr_in *)aa)->sin_addr.s_addr != 
	    ((struct sockaddr_in *)bb)->sin_addr.s_addr) return FALSE;
    }
    
    return TRUE;
}



/* returns port number in network byte horder */
static unsigned short get_port(const struct sockaddr *sa)
{
	unsigned short ret;
	
	assert(sa != NULL);
	assert(sa->sa_family == AF_INET || sa->sa_family == AF_INET6);
	
	switch (sa->sa_family) {
		case AF_INET:
			ret = ((struct sockaddr_in *)sa)->sin_port;
			break;
		case AF_INET6:
			ret = ((struct sockaddr_in6 *)sa)->sin6_port;
			break;
		default:
			fatal("address family not supported", sa->sa_family);
	}
	
	return ret;
}



/* returns TRUE if a represents an ipv4-mapped address */
static bool is_address_ipv4_mapped(const struct sockaddr *a)
{
	bool ret = FALSE;
	
	if (a->sa_family == AF_INET6 && 
	    IN6_IS_ADDR_V4MAPPED(&(((struct sockaddr_in6 *)a)->sin6_addr)))
		ret = TRUE;
			
	return ret;
}



/* returns TRUE if sa corresponds to the address/port couple specified in addr */
bool is_allowed(const struct sockaddr *sa, const address *addr, unsigned int flags)
{
	struct addrinfo hints, *res = NULL, *ptr;
	int err;
	bool ret;

	assert(sa != NULL);
	assert(addr != NULL);	
	assert(addr->address == NULL || strlen(addr->address) > 0);
	assert(addr->port    == NULL || strlen(addr->port) > 0);
		
	if (addr->address == NULL && addr->port == NULL) return TRUE;
		
	/* if the address is unspecified and the port is allowed, 
	 * then return TRUE */
	if ((addr->address == NULL) && 
	    ((safe_atoi(addr->port) == ntohs(get_port(sa))))) 
		return TRUE;
	
	ret = FALSE;
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = (flags & USE_UDP ? SOCK_DGRAM : SOCK_STREAM);
	
	err = getaddrinfo(addr->address, addr->port, &hints, &res);
	if (err != 0) fatal("getaddrinfo error: %s", gai_strerror(err));

	for (ptr = res; ptr != NULL; ptr = ptr->ai_next) {
		if ((flags | STRICT_IPV6) &&
		    is_address_ipv4_mapped(ptr->ai_addr)) {
			/* cannot accept address */
			continue;
		}
		if ((are_address_equal(sa, ptr->ai_addr) == TRUE) &&
		    (addr->port == NULL || 
		     (safe_atoi(addr->port) == ntohs(get_port(sa))))) {
			ret = TRUE;
			break;
		}
	}

	freeaddrinfo(res);

	return ret;
}

