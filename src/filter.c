#include <assert.h>
#include <netdb.h>
#include <sys/socket.h>
#if defined __GLIBC__
#if (__GLIBC__ == 2) && (__GLIBC_MINOR__ == 1)
#include <netinet/in.h>
#endif
#endif
#include "filter.h"
#include "misc.h"


#if 0
/* bool is_filter_empty(const filter *f); */
#define is_filter_empty(_f_)	(*(_f_) == NULL ? TRUE : FALSE)

/* iterator begin(filter *f); */
#define begin(_f_)		(*(_f_))

/* iterator iterator_next(iterator i); */
#define iterator_next(_i_)	((_i_)->next)

/* iterator end(filter *f); */
#define end(_f_)		(NULL)

/* struct filter_t *get_element(iterator i) */
#define get_element(_i_)	((struct filter_t *)(_i_))

void add_to_filter(const address *a, filter *f)
{
	struct filter_t *tmp, *ptr;

	ptr = (struct filter_t *)xmalloc(sizeof(struct filter_t));
	ptr->address = a->address;
	ptr->port    = a->port;
	ptr->next    = *f;
	*f = ptr;
}
#endif


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
	        ((struct sockaddr_in *)aa)->sin_addr.s_addr = 
	    	    ((struct sockaddr_in6 *)a)->sin6_addr.s6_addr32[3];
	        ((struct sockaddr_in *)aa)->sin_port = 
		    ((struct sockaddr_in6 *)a)->sin6_port;
        } else if (b->sa_family == AF_INET6 && 
            IN6_IS_ADDR_V4MAPPED(&((struct sockaddr_in6 *)b)->sin6_addr)) {
	        bb = (struct sockaddr *)alloca(sizeof(struct sockaddr_in));
	        memset(bb, 0, sizeof(struct sockaddr_in));
	        ((struct sockaddr_in *)bb)->sin_addr.s_addr = 
	    	    ((struct sockaddr_in6 *)b)->sin6_addr.s6_addr32[3];
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



static bool is_address_ipv4_mapped(const struct sockaddr *a)
{
	bool ret = FALSE;
	
	if (a->sa_family == AF_INET6 && 
	    IN6_IS_ADDR_V4MAPPED(&(((struct sockaddr_in6 *)a)->sin6_addr)))
		ret = TRUE;
			
	return ret;
}



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
		if (flags | STRICT_IPV6 &&
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



#if 0
bool is_allowed(const struct sockaddr *sa, filter *ft)
{
	bool ret;
	iterator it;
	
	assert(sa != NULL);
	assert(sa->sa_family == AF_INET || sa->sa_family == AF_INET6);
	
	/* if the filter is empty, then the connection is allowed */
	if (ft == NULL || is_filter_empty(ft)) return TRUE; 

	/* else, check every element of the filter list to see if the
	 * connection is allowed */
	ret = FALSE;
	
	for (it = begin(ft); it != end(ft); it = iterator_next(it)) {
		struct addrinfo hints, *res = NULL, *ptr;
		struct filter_t *el;
		int err; 

		el = get_element(it);
		assert(el != NULL);
		assert(el->address != NULL || el->port != NULL);
	
		/* if the address is unspecified and the port is 
		 * allowed, then return TRUE */
		if ((el->address == NULL) && 
		    ((safe_atoi(el->port) == ntohs(get_port(sa))))) 
			return TRUE;
		
		memset(&hints, 0, sizeof(hints));
		hints.ai_family   = AF_UNSPEC;
/*		hints.ai_socktype = SOCK_STREAM; 
 */
			
		err = getaddrinfo(el->address, el->port, &hints, &res);
		if (err != 0) 
			fatal("getaddrinfo error: %s", gai_strerror(err));

		for (ptr = res; ptr != NULL; ptr = ptr->ai_next) { 
			if ((is_address_equal(sa, ptr->ai_addr) == TRUE) &&
			    (el->port == NULL || 
			     (safe_atoi(el->port) == ntohs(get_port(sa))))) {
				ret = TRUE;
				break;
			}
		}

		freeaddrinfo(res);
	}

	return ret;
}
#endif
