#include "config.h"
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "circ_buf.h"
#include "misc.h"
#include "readwrite.h"

#undef  MAX
#define MAX(a,b) (((a)>(b))?(a):(b))

/* buffer size is 8kb */
static const size_t BUFFER_SIZE = 8192;

/* by default, let's make select time out in 1 second. this is clearly an
 * arbitrary choice, and i don't know if it is a good (or clever) one. */
static const time_t SELECT_TIMEOUT_SECS  = 1;
static const time_t SELECT_TIMEOUT_USECS = 0;


/* ios1 is the remote stream, ios2 the local one */
void readwrite(io_stream *ios1, io_stream *ios2)
{
	int rr, max_fd_in;
	fd_set read_fdset, tmp_fdset;
	struct timeval timeout; 
	static circ_buf *buf1 = NULL;
	static circ_buf *buf2 = NULL;

	assert(ios1 != NULL);
	assert(ios2 != NULL);

	/* setup circular buffers if not already done */
	if (buf1 == NULL) buf1 = alloc_cb(BUFFER_SIZE);
	if (buf2 == NULL) buf2 = alloc_cb(BUFFER_SIZE);

	/* check circular buffers validity */
	check_cb(buf1);
	check_cb(buf2);

	/* setup all the stuff for the select loop */
	max_fd_in = MAX(ios1->fd_in, ios2->fd_in);
	if (max_fd_in > FD_SETSIZE) {
		fatal("max_fd_in > FD_SETSIZE");
	}

	FD_ZERO(&read_fdset);
	FD_SET(ios1->fd_in, &read_fdset);
	FD_SET(ios2->fd_in, &read_fdset);
	
	/* here's the select loop */
	while(FD_ISSET(ios1->fd_in, &read_fdset) &&
	      FD_ISSET(ios2->fd_in, &read_fdset)) {
		struct timeval *tmp;
		
		/* make a copy of read_fdset param before passing it
		 * to select */
		tmp_fdset = read_fdset;

		/* setup select timeout */
		if (is_empty(buf1) == TRUE && is_empty(buf2) == TRUE) { 
			/* if the buffers are empty we have no data
			 * to write to the io_streams, so we can 
			 * block indefinitely */
			tmp = NULL;
		} else {
			/* setup standard timeout */
			timeout.tv_sec  = SELECT_TIMEOUT_SECS;
			timeout.tv_usec = SELECT_TIMEOUT_USECS;
			tmp = &timeout;
		}
			

		/* blocking select with timeout */
		rr = select(max_fd_in + 1, &tmp_fdset, NULL, NULL, tmp);

		/* handle select errors. if errno == EINTR we will go on 
		 * with the loop, as there might be some output waiting 
		 * to be written in ios1->fd_out or ios2->fd_out. */
		if (rr < 0 && errno != EINTR) 
			fatal("select error: %s", strerror(errno));

		
		if (FD_ISSET(ios1->fd_in, &tmp_fdset)) {
			/* something has been received from ios1. 
			 * let's read it. */
			rr = read_to_cb(ios1->fd_in, buf1);
			
			if (ios1->is_tcp_socket == TRUE && rr == 0) {
				/* if rr == 0, then we are not receiving
				 * data from ios1->fd_in anymore. 
				 * let's close it. */
				close(ios1->fd_in);				
				/* this will put an end to the select loop. */
				FD_CLR(ios1->fd_in, &read_fdset);
			} else if (rr < 0) {
				/* error while reading ios1->fd_in: 
				 * print an error message and exit. */
				fatal("error reading from fd %d: %s", 
				      ios1->fd_in, strerror(errno));
			}
		}
		
		
		if (FD_ISSET(ios2->fd_in, &tmp_fdset)) {			
			/* something has been received from ios2. 
			 * let's read it. */
			rr = read_to_cb(ios2->fd_in, buf2);			

			if (ios2->is_tcp_socket == TRUE && rr == 0) {
				/* if rr == 0, then we are not receiving
				 * data from ios2->fd_in anymore. 
				 * let's close it. */
				close(ios2->fd_in);				
				/* this will put an end to the select loop. */
				FD_CLR(ios2->fd_in, &read_fdset);
			} else 
			if (rr < 0) {
				/* error while reading ios2->fd_in: 
				 * print an error message and exit. */
				fatal("error reading from fd: %d; %s", 
				      ios2->fd_in, strerror(errno));
			}
		}


		rr = write_from_cb(ios1->fd_out, buf2);
		if (rr < 0) {
			/* error while writing to ios1->fd_out: 
			 * print an error message and exit. */
			fatal("error writing to fd: %d; %s", 
			      ios1->fd_out, strerror(errno));
		}

		rr = write_from_cb(ios2->fd_out, buf1);
		if (rr < 0) {
			/* error while writing to ios2->fd_out: 
			 * print an error message and exit. */
			fatal("error writing to fd: %d; %s", 
			      ios2->fd_out, strerror(errno));
		}
	}
}



void stdio_to_io_stream(io_stream *ios)
{
	ios->fd_in  = STDIN_FILENO;
	ios->fd_out = STDOUT_FILENO;
	ios->is_tcp_socket = FALSE;
}



void socket_to_io_stream(int fd, io_stream *ios)
{
	ios->fd_in  = fd;
	ios->fd_out = fd;
	ios->is_tcp_socket = TRUE;
}


