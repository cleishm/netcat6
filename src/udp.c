#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "circ_buf.h"
#include "misc.h"
#include "network.h"
#include "readwrite.h"

#undef  MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#undef  MIN
#define MIN(a,b) (((a)<(b))?(a):(b))

#define UDP_MSG_SIZE 65536

/* buffer size is 8kb */
static const size_t BUFFER_SIZE = UDP_MSG_SIZE;

/* by default, let's make select time out in 1 second. this is clearly an
 * arbitrary choice, and i don't know if it is a good (or clever) one. */
static const time_t SELECT_TIMEOUT_SECS  = 1;
static const time_t SELECT_TIMEOUT_USECS = 0;

static struct sockaddr_storage dest;
static int destlen;
static address dest_addr;
static bool have_dest      = FALSE;
static bool have_dest_addr = FALSE;



static int sockaddr_len(struct sockaddr *sa)
{
	int ret;
	
	assert(sa != NULL);

	switch (sa->sa_family) {
	case AF_INET:
		ret = sizeof(struct sockaddr_in);
		break;
	case AF_INET6:
		ret = sizeof(struct sockaddr_in6);
		break;
	default:
		fatal("address family not supported");
	}
	
	return ret;
}



/* this function opens a socket, connects the socket to the address specified 
 * in addr and returns the file descriptor of the socket. */
static int udp_connect_to(sa_family_t family, unsigned int flags, 
                          address *remote, address *local)
{
	int err, fd;
	struct addrinfo hints, *res = NULL;

	/* make sure preconditions on remote address are respected */
	assert(remote != NULL);
	assert(remote->address != NULL && strlen(remote->address) > 0);
	assert(remote->port != NULL && strlen(remote->port) > 0 );

	assert(flags & USE_UDP);
	
	/* setup hints structure to be passed to getaddrinfo */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = family;
	hints.ai_socktype = SOCK_DGRAM;
	
	if (flags & NUMERIC_MODE) 
		hints.ai_flags |= AI_NUMERICHOST;
	
	/* get the IP address of the remote end of the connection */
	err = getaddrinfo(remote->address, remote->port, &hints, &res);
	if(err != 0) fatal("getaddrinfo error: %s", gai_strerror(err));

	/* check the results of getaddrinfo */
	assert(res != NULL);
	assert(res->ai_addrlen <= sizeof(dest));

	/* get the fisrt sockaddr structure returned by getaddrinfo */
	memcpy(&dest, res->ai_addr, res->ai_addrlen);
	destlen = res->ai_addrlen;

	/* cleanup to avoid memory leaks */
	freeaddrinfo(res);

	/* create the socket */
	fd = socket(dest.ss_family, SOCK_DGRAM, 0);
	if (fd < 0) fatal("cannot create the socket: %s", strerror(errno));

	/* handle -s option */
	if (local != NULL && (local->address != NULL || local->port != NULL)) {
		struct sockaddr_storage src;
		int srclen;
		
		/* make sure preconditions on local address are respected */
		assert(local->address == NULL || strlen(local->address) > 0);
		assert(local->port    == NULL || strlen(local->port) > 0);
		
		/* setup hints structure to be passed to getaddrinfo */
		memset(&hints, 0, sizeof(hints));
		hints.ai_family   = family;
		hints.ai_socktype = SOCK_DGRAM;
	
		if (flags & NUMERIC_MODE) 
			hints.ai_flags |= AI_NUMERICHOST;
		
		/* get the IP address of the local end of the connection */
		err = getaddrinfo(local->address, local->port, &hints, &res);
		if(err != 0) fatal("getaddrinfo error: %s", gai_strerror(err));

		/* check the results of getaddrinfo */
		assert(res != NULL);
		assert(res->ai_addrlen <= sizeof(src));

		/* get the fisrt sockaddr structure returned by getaddrinfo */
		memcpy(&src, res->ai_addr, res->ai_addrlen);
		srclen = res->ai_addrlen;
		
		/* cleanup to avoid memory leaks */
		freeaddrinfo(res);

#ifdef IPV6_V6ONLY
		if (family == AF_INET6) {
			int on = 1;
			/* in case of error, we will go on anyway... */
			err = setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &on,
					 sizeof(on));
			if (err < 0) perror("error with sockopt IPV6_V6ONLY");
		}
#endif 
		
		/* bind to the local address */
		err = bind(fd, (struct sockaddr *)&src, srclen);
		if (err != 0) 
			fatal("cannot use specified source addr/port: %s", 
		              strerror(errno));
		
	}
	
	return fd;
}



static int udp_bind_to(sa_family_t family, unsigned int flags,
                       address *local)
{
	int err, fd, ns;
	struct addrinfo hints, *res = NULL;
	struct sockaddr_storage src;
	int srclen;

	assert(local != NULL);
	assert(local->address != NULL || local->port != NULL);
	assert(local->address == NULL || strlen(local->address) > 0);
	assert(local->port    == NULL || strlen(local->port) > 0);
	
	assert(flags & USE_UDP);
	 
	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = family;
	hints.ai_flags    = AI_PASSIVE;
	hints.ai_socktype = SOCK_DGRAM;
	
	if (flags & NUMERIC_MODE) 
		hints.ai_flags |= AI_NUMERICHOST;

	err = getaddrinfo(local->address, local->port, &hints, &res);
	if (err != 0) fatal("getaddrinfo error: %s", gai_strerror(err));
		
	assert(res != NULL);

	memcpy(&src, res->ai_addr, res->ai_addrlen);
	srclen = res->ai_addrlen;
	
	freeaddrinfo(res);

	fd = socket(src.ss_family, SOCK_DGRAM, 0);
	if (fd < 0) fatal("cannot create the socket: %s", strerror(errno));

#ifdef IPV6_V6ONLY
	if (family == AF_INET6) {
		int on = 1;
		/* in case of error, we will go on anyway... */
		err = setsockopt(fd,IPPROTO_IPV6,IPV6_V6ONLY,&on,sizeof(on));
		if (err < 0) perror("error with sockopt IPV6_V6ONLY");
	}
#endif 
		
	if (flags & REUSE_ADDR) {
		int on = 1;
		/* in case of error, we will go on anyway... */
		err = setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
		if (err < 0) perror("error with sockopt SO_REUSEADDR");
	}

	err = bind(fd, (struct sockaddr *)&src, srclen);
	if (err != 0) fatal("cannot use specified source addr/port: %s", 
		            strerror(errno));

	return fd;
}



/* udp_sock is the remote stream, ios the local one */
static void udp_readwrite(int udp_sock, io_stream *ios, unsigned int flags)
{
	int rr, max_fd_in;
	fd_set read_fdset, tmp_fdset;
	struct timeval timeout; 
	static uint8_t *buf1 = NULL;
	static uint8_t *buf2 = NULL;

	assert(ios != NULL);

	/* setup circular buffers if not already done */
	if (buf1 == NULL) buf1 = (uint8_t *)xmalloc(BUFFER_SIZE);
	if (buf2 == NULL) buf2 = (uint8_t *)xmalloc(BUFFER_SIZE);

	/* setup all the stuff for the select loop */
	max_fd_in = MAX(udp_sock, ios->fd_in);
	if (max_fd_in > FD_SETSIZE) {
		fatal("max_fd_in > FD_SETSIZE");
	}

	FD_ZERO(&read_fdset);
	FD_SET(udp_sock, &read_fdset);
	FD_SET(ios->fd_in, &read_fdset);
	
	/* here's the select loop */
	while(FD_ISSET(udp_sock, &read_fdset) &&
	      FD_ISSET(ios->fd_in, &read_fdset)) {
		int io_rcvd, net_rcvd; /* bytes received, respectively, 
					  from io and from the net */
		
		/* reset status */
		io_rcvd = 0;
		net_rcvd = 0;
		
		/* make a copy of read_fdset param before passing it
		 * to select */
		tmp_fdset = read_fdset;
		
		/* the buffers are assumed to be empty, so we have no data
		 * to write to the io_streams and we can block indefinitely */
		rr = select(max_fd_in + 1, &tmp_fdset, NULL, NULL, NULL);

		/* handle select errors. if errno == EINTR we will go on 
		 * with the loop, as there might be some output waiting 
		 * to be written in udp_sock->fd_out or ios->fd_out. */
		if (rr < 0 && errno != EINTR) 
			fatal("select error: %s", strerror(errno));

		
		if (FD_ISSET(udp_sock, &tmp_fdset)) {
			struct sockaddr_storage from;
			int fromlen = sizeof(from);
			
			/* something has been received from udp_sock. 
			 * let's read it. */
			rr = recvfrom(udp_sock, (void *)buf1, BUFFER_SIZE, 0, 
				      (struct sockaddr *)&from, &fromlen);
			
			if (have_dest == FALSE) {
				destlen = sockaddr_len((struct sockaddr *)&from);
				assert(destlen <= sizeof(struct sockaddr_storage));
				memcpy(&dest, &from, destlen);
				have_dest == TRUE;
			}
			
			if ((have_dest_addr == FALSE && 
			     are_address_equal(&from, &dest) == TRUE) || 
			    (have_dest_addr == TRUE && 
			     is_allowed(&from, &dest_addr, flags) == TRUE)) {
#if 0
				if (rr == 0) {
					/* if rr == 0, then we are not 
					 * receiving data from udp_sock 
					 * anymore. let's close it. */
					close(udp_sock);				
					/* this will put an end to the 
					 * select loop. */
					FD_CLR(udp_sock, &read_fdset);
				} else 
#endif
				if (rr < 0) {
					/* error while reading udp_sock: 
					 * print an error message and exit. */
					fatal("error reading from fd %d: %s", 
					      udp_sock, strerror(errno));
				}
			}
			
			net_rcvd = rr;
		}
		
		
		if (FD_ISSET(ios->fd_in, &tmp_fdset)) {
			/* something has been received from ios. 
			 * let's read it. */
			rr = read(ios->fd_in, buf2, BUFFER_SIZE);

			if (rr < 0) {
				/* error while reading ios->fd_in: 
				 * print an error message and exit. */
				fatal("error reading from fd: %d; %s", 
				      ios->fd_in, strerror(errno));
			}
			
			io_rcvd = rr;
		}


		if (io_rcvd > 0)
			rr = sendto(udp_sock, buf2, io_rcvd, 0, 
				(struct sockaddr *)&dest, destlen);

		if (rr < 0) {
			/* error while writing to udp_sock: 
			 * print an error message and exit. */
			fatal("error writing to fd: %d; %s", 
			      udp_sock, strerror(errno));
		} else if (rr < io_rcvd) {
			/* if we can't write all data to the net, we simply
			 * issue a warning and proceed. after all, udp is an
			 * unreliable protocol, so why bother if we lose 
			 * some data? */
			warn("%d bytes lost while writing to the net.");
		} 

		
		if (net_rcvd > 0)
			rr = write(ios->fd_out, buf1, net_rcvd);
		
		if (rr < 0) {
			/* error while writing to ios->fd_out: 
			 * print an error message and exit. */
			fatal("error writing to fd: %d; %s", 
			      ios->fd_out, strerror(errno));
		} else if (rr < net_rcvd) {
			/* if we can't write all data to stdio, we simply
			 * issue a warning and proceed. after all, udp is an
			 * unreliable protocol, so why bother if we lose 
			 * some data? */
			warn("%d bytes lost while writing to stdio.");
		}
	}
}



void udp_connect(sa_family_t family, unsigned int flags, 
		 address *remote_addr, address *local_addr)
{
	int fd;
	io_stream remote, local;

	assert(remote_addr != NULL);
	assert(flags & USE_UDP);
	
	dest_addr = *remote_addr;
	have_dest_addr = TRUE;
	stdio_to_io_stream(&local);

	fd = udp_connect_to(family, flags, remote_addr, local_addr);
	have_dest = TRUE;

	udp_readwrite(fd, &local, flags);
}



void udp_listen(sa_family_t family, unsigned int flags, address *local_addr)
{
	int fd;
	io_stream remote, local;

	assert(local_addr != NULL);
	assert(flags & USE_UDP);
	
	have_dest_addr = FALSE;
	have_dest      = FALSE;

	stdio_to_io_stream(&local);

	fd = udp_bind_to(family, flags, local_addr);

	udp_readwrite(fd, &local, flags);
}
