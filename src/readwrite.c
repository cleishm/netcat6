/*
 *  readwrite.c - stream i/o reading/writing loop - implementation 
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
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include "config.h"
#include "circ_buf.h"
#include "misc.h"
#include "parser.h"
#include "readwrite.h"

#undef  MAX
#define MAX(a,b) (((a)>(b))?(a):(b))

/* buffer size is 8kb */
static const size_t BUFFER_SIZE = 8192;

/* by default, let's make select time out in 1 second. this is clearly an
 * arbitrary choice, and i don't know if it is a good (or clever) one. */
static const time_t SELECT_TIMEOUT_SECS  = 1;
static const time_t SELECT_TIMEOUT_USECS = 0;

static time_t timestamp1; 
static time_t timestamp2; 

static int is_timer_expired(time_t t1, time_t t2)
{
	(void)t1;
	(void)t2;

	return 1;
}

static int flush_buffer(circ_buf *cb, int fd)
{
	int rr;
	
	rr = write_from_cb(fd, cb);
	if (rr < 0) {
		/* error while writing to fd: 
		 * print an error message and exit. */
		fatal("error writing to fd: %d; %s", fd, strerror(errno));
	}

	return rr;
}

/* ios1 is the remote stream, ios2 the local one */
void readwrite(io_stream *ios1, io_stream *ios2)
{
	int rr, max_fd, tmp;
	bool file_transfer_mode;
	fd_set read_fdset, tmp_rd_fdset, write_fdset, tmp_wr_fdset;
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
	max_fd = MAX(ios1->fd_in, ios1->fd_out);
	tmp    = MAX(ios2->fd_in, ios2->fd_out);
	if (tmp > max_fd) max_fd = tmp;

	if (max_fd > FD_SETSIZE) {
		fatal("max_fd > FD_SETSIZE");
	}

	FD_ZERO(&read_fdset);
	FD_SET(ios1->fd_in, &read_fdset);
	FD_SET(ios2->fd_in, &read_fdset);
	
	FD_ZERO(&write_fdset);
	FD_SET(ios1->fd_out, &write_fdset);
	FD_SET(ios2->fd_out, &write_fdset);
	
	file_transfer_mode = is_flag_set(FILE_TRANSFER_MODE);
	
	/* here's the select loop. 
	 *
	 * if the user has chosen normal mode, the loop keeps going 
	 * until one of the following condition becomes true:
	 * 
	 * 1) the remote input streams has been closed
	 * 
	 * if the user has chosen file transfer mode, the loop keeps going 
	 * until one of these condition becomes true:
	 * 
	 * 1) the local or the remote input streams has been closed
	 * 2) the timer set by the user is expired 
	 */
	while(((file_transfer_mode == FALSE) && 
	       FD_ISSET(ios1->fd_in, &read_fdset)) ||
	      ((file_transfer_mode == TRUE) && 
	       ((FD_ISSET(ios1->fd_in, &read_fdset) && 
		 FD_ISSET(ios2->fd_in, &read_fdset)) ||
		!is_timer_expired(timestamp1, timestamp2)))) {  

		struct timeval *tmp;
#ifndef NDEBUG	
		int cond1, cond2;

		cond1 = FD_ISSET(ios1->fd_in, &read_fdset);
		cond2 = FD_ISSET(ios2->fd_in, &read_fdset);
		
		fprintf(stderr,"condition 1 is %s\n", (cond1 ? "TRUE": "FALSE"));	
		fprintf(stderr,"condition 2 is %s\n", (cond2 ? "TRUE": "FALSE"));
#endif
		
		
		/* make a copy of read_fdset and write_fdset before passing 
		 * them to select */
		tmp_rd_fdset = read_fdset;
		tmp_wr_fdset = write_fdset;

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
		rr = select(max_fd + 1, &tmp_rd_fdset, &tmp_wr_fdset, 
			    NULL, tmp);

		/* handle select errors. if errno == EINTR we will go on 
		 * with the loop, as there might be some output waiting 
		 * to be written in ios1->fd_out or ios2->fd_out. */
		if (rr < 0 && errno != EINTR) 
			fatal("select error: %s", strerror(errno));

		
		if (FD_ISSET(ios1->fd_in, &tmp_rd_fdset) &&
		    is_full(buf1) == FALSE) {
			/* something has been received from ios1. 
			 * let's read it. */
			rr = read_to_cb(ios1->fd_in, buf1);
			
#ifndef NDEBUG				
			fprintf(stderr,"read %d bytes from ios1->fd_in (%d)\n",
				rr, ios1->fd_in);				
#endif
			
			if (rr == 0) {
				/* if rr == 0, then we are not receiving
				 * data from ios1->fd_in anymore. 
				 * let's close it. */
				if (ios1->is_tcp_socket == TRUE) {
					shutdown(ios1->fd_in, SHUT_RD);
				} else {
					close(ios1->fd_in);
				}
				time(&timestamp1);
				FD_CLR(ios1->fd_in, &read_fdset);

#if 0
				if (ios2->is_tcp_socket == TRUE &&
				    is_empty(buf1) == TRUE)
					shutdown(ios2->fd_out, SHUT_WR);
				FD_CLR(ios2->fd_out, &write_fdset);
#endif
				
#ifndef NDEBUG				
				fprintf(stderr, "closing ios1->fd_in (%d)\n",
					ios1->fd_in);				
#endif
				
			} else if (rr < 0) {
				/* error while reading ios1->fd_in: 
				 * print an error message and exit. */
				fatal("error reading from fd %d: %s", 
				      ios1->fd_in, strerror(errno));
			}
		}
		
		
		if (FD_ISSET(ios2->fd_in, &tmp_rd_fdset) &&	
		    is_full(buf2) == FALSE) {
			/* something has been received from ios2. 
			 * let's read it. */
			rr = read_to_cb(ios2->fd_in, buf2);

#ifndef NDEBUG				
			fprintf(stderr,"read %d bytes from ios2->fd_in (%d)\n",
				rr, ios2->fd_in);				
#endif

			if (rr == 0) {
				/* if rr == 0, then we are not receiving
				 * data from ios2->fd_in anymore. 
				 * let's close it. */
				if (ios2->is_tcp_socket == TRUE) {
					shutdown(ios2->fd_in, SHUT_RD);
				} else {
					close(ios2->fd_in);
				}
				time(&timestamp2);
				FD_CLR(ios2->fd_in, &read_fdset);

#if 0
				if (ios1->is_tcp_socket == TRUE &&
				    is_empty(buf2) == TRUE)
					shutdown(ios1->fd_out, SHUT_WR);
				FD_CLR(ios1->fd_out, &write_fdset);
#endif
				
#ifndef NDEBUG				
				fprintf(stderr, "closing ios2->fd_in (%d)\n",
					ios2->fd_in);				
#endif
				
			} else if (rr < 0) {
				/* error while reading ios2->fd_in: 
				 * print an error message and exit. */
				fatal("error reading from fd: %d; %s", 
				      ios2->fd_in, strerror(errno));
			}
		}
		

		if (FD_ISSET(ios1->fd_out, &tmp_wr_fdset) &&
		    is_empty(buf2) == FALSE) {
			rr = flush_buffer(buf2, ios1->fd_out);
#ifndef NDEBUG				
			fprintf(stderr,
				"written %d bytes to ios1->fd_out (%d)\n",
				rr, ios1->fd_out);	
#endif
		}

		
		if ((FD_ISSET(ios2->fd_out, &tmp_wr_fdset)) &&
		    is_empty(buf1) == FALSE) {			
			rr = flush_buffer(buf1, ios2->fd_out);
#ifndef NDEBUG				
			fprintf(stderr,
				"written %d bytes to ios2->fd_out (%d)\n",
				rr, ios2->fd_out);
#endif
		}
	}
	
	/* flush the buffers */
	if (is_empty(buf2) == FALSE) {
		rr = flush_buffer(buf2, ios1->fd_out);
#ifndef NDEBUG				
		fprintf(stderr, "written %d bytes to ios1->fd_out (%d)\n",
			rr, ios1->fd_out);
#endif
	}
	
	if (is_empty(buf1) == FALSE) {
		rr = flush_buffer(buf1, ios2->fd_out);
#ifndef NDEBUG				
		fprintf(stderr, "written %d bytes to ios2->fd_out (%d)\n",
			rr, ios2->fd_out);
#endif
	}
}



static void nonblock(int fd) 
{
	long arg;
	
	arg = fcntl(fd, F_GETFL, 0);
	arg |= O_NONBLOCK;
	fcntl(fd, F_SETFL, arg);
}



void stdio_to_io_stream(io_stream *ios)
{
	nonblock(STDIN_FILENO);
	nonblock(STDOUT_FILENO);
	
	ios->fd_in  = STDIN_FILENO;
	ios->fd_out = STDOUT_FILENO;
	ios->is_tcp_socket = FALSE; 
}



void socket_to_io_stream(int fd, io_stream *ios)
{
	nonblock(fd);
	
	ios->fd_in  = fd;
	ios->fd_out = fd;
	ios->is_tcp_socket = TRUE;
}



