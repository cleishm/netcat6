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
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include "config.h"
#include "circ_buf.h"
#include "filter.h"
#include "misc.h"
#include "network.h"
#include "parser.h"
#include "readwrite.h"
#include "timeout.h"

/* buffer size is 8kb */
static const size_t BUFFER_SIZE = 8192;
static const size_t TEMP_BUFFER_SIZE = 65536;

/* by default, let's make select time out in 1 second. this is clearly an
 * arbitrary choice, and i don't know if it is a good (or clever) one. */
static const time_t SELECT_TIMEOUT_SECS  = 1;
static const time_t SELECT_TIMEOUT_USECS = 0;

/* bytes received, respectively, from local input and from the net */
static int local_rcvd, net_rcvd; 
/* bytes sent, respectively, to local output and to the net */
static int local_sent, net_sent; 

static void shutdown_io_stream(io_stream *ios, int how);
#if 0
static void nonblock(int fd);
#endif



/* ios1 is the remote stream, ios2 the local one - not that it matters */
void readwrite(io_stream *ios1, io_stream *ios2)
{
	int rr, max_fd;
	bool file_xfer_mode;
	bool listen_mode;
	bool timeout_mode;
	fd_set read_fdset, tmp_rd_fdset, write_fdset, tmp_wr_fdset;
	struct timeval timeout; 
	circ_buf *buf1 = NULL;
	uint8_t *tbuf1 = NULL;
	int tbuf1_bytes = 0, tbuf1_offset = 0;
	circ_buf *buf2 = NULL;
	struct timeval timestamp1, timestamp2;
	bool t1 = FALSE;
	bool t2 = FALSE;
	
	/* check function arguments */
	assert(ios1 != NULL);
	assert(ios2 != NULL);

	/* set up the buffers for each ios */
	/* dgram protocols need to read into a temporary buffer first */
	buf1 = alloc_cb(BUFFER_SIZE);
	if (ios1->socktype == SOCK_DGRAM)
		tbuf1 = (uint8_t *)xmalloc(TEMP_BUFFER_SIZE);

	/* since we know ios2 is local, it will be a stream - hence no need for
	 * the temporary buffer */
	buf2 = alloc_cb(BUFFER_SIZE);
	assert(ios2->socktype != SOCK_DGRAM);

	/* reset status */
	local_rcvd = 0;
	net_rcvd   = 0;		
	local_sent = 0;
	net_sent   = 0;		

	file_xfer_mode = is_flag_set(FILE_TRANSFER_MODE);
	listen_mode    = is_flag_set(LISTEN_MODE);
	timeout_mode   = is_flag_set(TIMEOUT_MODE);

	/* setup all the stuff for the select loop */
	FD_ZERO(&read_fdset);
	FD_ZERO(&write_fdset);

	if (file_xfer_mode == TRUE) {
		/* if we are in file transfer mode, setup unidirectional 
		 * data transfers */
		if (listen_mode == TRUE) {
			/* reading only from the remote stream */ 
			FD_SET(ios1->fd_in, &read_fdset);

			/* close the remote stream for writing */
			shutdown_io_stream(ios1, SHUT_WR);

			/* close the local stream for reading */
			shutdown_io_stream(ios2, SHUT_RD);
			
			max_fd = MAX(ios1->fd_in, ios2->fd_out);
		} else {
			/* reading only from the local stream */ 
			FD_SET(ios2->fd_in, &read_fdset);
		
			/* close the remote stream for reading */
			shutdown_io_stream(ios1, SHUT_RD);
			
			/* close the local stream for writing */
			shutdown_io_stream(ios2, SHUT_WR);
			
			max_fd = MAX(ios1->fd_out, ios2->fd_in);
		}
	} else {
		int temp;
		
		/* if we are not in file transfer mode, setup bidirectional 
		 * data transfers */
		FD_SET(ios1->fd_in, &read_fdset);
		FD_SET(ios2->fd_in, &read_fdset);

		max_fd = MAX(ios1->fd_in, ios1->fd_out);
		temp   = MAX(ios2->fd_in, ios2->fd_out);
		if (temp > max_fd) max_fd = temp;
	}
	
	if (max_fd > FD_SETSIZE) {
		fatal("max_fd > FD_SETSIZE");
	}

	
	/* here's the select loop. 
	 *
	 * if the user has chosen normal mode, the loop keeps going 
	 * until the following condition becomes true:
	 * 
	 *    the remote input stream has been closed
	 * 
	 * if the user has chosen file transfer mode, the loop keeps going 
	 * until the following conditions become both true:
	 * 
	 * 1) the local or the remote input streams has been closed
	 * 2) the timeout set by the user is expired 
	 */
	while(((timeout_mode == FALSE) && (ios1->fd_in >= 0)) ||
	      ((timeout_mode == TRUE) && (((ios1->fd_in >= 0) || 
		 ((t1 = timeout_expired(&timestamp1, TIMEOUT1)) == FALSE)) && 
		((ios2->fd_in >= 0) ||  
		 ((t2 = timeout_expired(&timestamp2, TIMEOUT2)) == FALSE))))) {  

		struct timeval *tvp;
		
		/* make a copy of read_fdset and write_fdset before passing 
		 * them to select */
		memcpy(&tmp_rd_fdset, &read_fdset, sizeof(fd_set));
		memcpy(&tmp_wr_fdset, &write_fdset, sizeof(fd_set));

		/* setup select timeout */
		if (timeout_mode == FALSE || (ios1->fd_in >= 0 && ios2->fd_in >= 0)) {
			/* if we aren't doing timeouts, or both inputs are still active
			 * we can block indefinitely */
			tvp = NULL;
		} else {
			/* setup standard timeout */
			timeout.tv_sec  = SELECT_TIMEOUT_SECS;
			timeout.tv_usec = SELECT_TIMEOUT_USECS;
			tvp = &timeout;
		}

		/* blocking select with timeout */		
		rr = select(max_fd + 1, &tmp_rd_fdset, &tmp_wr_fdset, NULL, tvp);

		/* handle select errors. if errno == EINTR we just retry select */
		if (rr < 0) {
			if (errno == EINTR) 
				continue;
			fatal("select error: %s", strerror(errno));
		}
		
		if (ios1->fd_in >= 0 && FD_ISSET(ios1->fd_in, &tmp_rd_fdset)) {
			assert(is_full(buf1) == FALSE);

			/* something is ready to read on ios1 (remote) */
			if (ios1->socktype == SOCK_DGRAM) {
				assert(tbuf1 != NULL);
				rr = recv(ios1->fd_in, (void *)tbuf1, TEMP_BUFFER_SIZE, 0);
			} else {
				rr = read_to_cb(ios1->fd_in, buf1);
			}
			
			if (rr > 0) {
				net_rcvd += rr;
#ifndef NDEBUG
				if (is_flag_set(VERY_VERBOSE_MODE) == TRUE) {
					fprintf(stderr,
						"%s %d bytes from ios1->fd_in (%d)\n",
						tbuf1? "read":"received", rr, ios1->fd_in);
				}
#endif
				/* forward any data in tbuf1 to the circular buffer */
				if (tbuf1) {
					int copied = copy_to_cb(tbuf1, rr, buf1);
					tbuf1_bytes = rr - copied;
					tbuf1_offset = copied;
					/* some MUST have gone into the circular buffer */
					assert(copied);
				}

				/* if the buffer is full, stop reading */
				if (tbuf1_bytes > 0 || is_full(buf1) == TRUE)
					FD_CLR(ios1->fd_in, &read_fdset);

				/* start writing the buffer out */
				assert(ios2->fd_out >= 0);
				FD_SET(ios2->fd_out, &write_fdset);
			} else if (rr == 0) {
				/* if rr == 0, then we are not receiving
				 * data from ios1->fd_in anymore. 
				 * let's close it. */
#ifndef NDEBUG				
				if (is_flag_set(VERY_VERBOSE_MODE) == TRUE) {
					fprintf(stderr, "closing ios1->fd_in (%d)\n",
						ios1->fd_in);
					printf("setting timestamp1\n");
				}
#endif
				FD_CLR(ios1->fd_in, &read_fdset);
				shutdown_io_stream(ios1, SHUT_RD);
				gettimeofday(&timestamp1, NULL);
			} else if (rr < 0 && errno != EAGAIN) {
				/* error while reading ios1->fd_in: 
				 * print an error message and exit. */
				fatal("error reading from fd %d: %s", 
				      ios1->fd_in, strerror(errno));
			}
		}
		
		
		if (ios2->fd_in >= 0 && FD_ISSET(ios2->fd_in, &tmp_rd_fdset)) {
			assert(is_full(buf2) == FALSE);

			/* something is ready to read on ios2 (local) */
			rr = read_to_cb(ios2->fd_in, buf2);
			
			if (rr > 0) {
				local_rcvd += rr;
#ifndef NDEBUG
				if (is_flag_set(VERY_VERBOSE_MODE) == TRUE) {
					fprintf(stderr,
						"read %d bytes from ios2->fd_in (%d)\n", rr, 
						ios2->fd_in);
				}
#endif
				/* if the buffer is full, stop reading */
				if (is_full(buf2) == TRUE)
					FD_CLR(ios2->fd_in, &read_fdset);

				/* start writing the buffer out */
				assert(ios1->fd_out >= 0);
				FD_SET(ios1->fd_out, &write_fdset);
			} else if (rr == 0) {
				/* if rr == 0, then we are not receiving
				 * data from ios2->fd_in anymore. 
				 * let's close it. */
#ifndef NDEBUG				
				if (is_flag_set(VERY_VERBOSE_MODE) == TRUE) {
					fprintf(stderr, "closing ios2->fd_in (%d)\n",
						ios2->fd_in);
					printf("setting timestamp2\n");
				}
#endif
				FD_CLR(ios2->fd_in, &read_fdset);
				shutdown_io_stream(ios2, SHUT_RD);
				gettimeofday(&timestamp2, NULL);
			} else if (rr < 0 && errno != EAGAIN) {
				/* error while reading ios2->fd_in: 
				 * print an error message and exit. */
				fatal("error reading from fd %d: %s", 
				      ios2->fd_in, strerror(errno));
			}
		}
		

		if (ios1->fd_out >= 0 && FD_ISSET(ios1->fd_out, &tmp_wr_fdset)) {
			assert(is_empty(buf2) == FALSE);

			/* ios1 may be written to (remote) */
			if (tbuf1)
				rr = send_from_cb(ios1->fd_out, buf2, NULL, 0);
			else
				rr = write_from_cb(ios1->fd_out, buf2);

			if (rr > 0) {
				net_sent += rr;
#ifndef NDEBUG				
				if (is_flag_set(VERY_VERBOSE_MODE) == TRUE) {
					fprintf(stderr,
						"%s %d bytes to ios1->fd_out (%d)\n",
						tbuf1? "written":"sent", rr, ios1->fd_out);
				}
#endif
				/* if anything was written, ios2 can read again */
				if (ios2->fd_in >= 0)
					FD_SET(ios2->fd_in, &read_fdset);
				/* if the buffer is emptied, stop trying to write */
				if (is_empty(buf2) == TRUE)
					FD_CLR(ios1->fd_out, &write_fdset);
			} else if (rr < 0 && errno != EAGAIN) {
				/* error while writing to ios1->fd_out:
				 * print an error message and exit. */
				fatal("error writing to fd %d: %s",
					ios1->fd_out, strerror(errno));
			}
		}

		
		if (ios2->fd_out >= 0 && FD_ISSET(ios2->fd_out, &tmp_wr_fdset)) {
			assert(is_empty(buf1) == FALSE);

			/* ios2 may be written to (local) */
			rr = write_from_cb(ios2->fd_out, buf1);

			if (rr > 0) {
				local_sent += rr;
#ifndef NDEBUG				
				if (is_flag_set(VERY_VERBOSE_MODE) == TRUE) {
					fprintf(stderr,
						"written %d bytes to ios2->fd_out (%d)\n",
						rr, ios2->fd_out);
				}
#endif
				/* forward more from tbuf1 to the circular buffer */
				if (tbuf1_bytes > 0) {
					int copied = copy_to_cb(
							tbuf1+tbuf1_offset, tbuf1_bytes, buf1);
					tbuf1_bytes -= copied;
					tbuf1_offset += copied;
				}
				/* if the temp buffer was emptied, ios1 can read again */
				if (ios1->fd_in >= 0 && tbuf1_bytes == 0)
					FD_SET(ios1->fd_in, &read_fdset);
				/* if the buffer is emptied, stop trying to write */
				if (is_empty(buf1) == TRUE)
					FD_CLR(ios2->fd_out, &write_fdset);
			} else if (rr < 0 && errno != EAGAIN) {
				/* error while writing to ios1->fd_out:
				 * print an error message and exit. */
				fatal("error writing to fd %d: %s",
					ios2->fd_out, strerror(errno));
			}
		}
	}
	
	/* flush the buffers as much as possible */
	while (is_empty(buf2) == FALSE) {
		if (tbuf1)
			rr = send_from_cb(ios1->fd_out, buf2, NULL, 0);
		else
			rr = write_from_cb(ios1->fd_out, buf2);
		if (rr > 0) {
			net_sent += rr;
#ifndef NDEBUG				
			if (is_flag_set(VERY_VERBOSE_MODE) == TRUE)
				fprintf(stderr, "written %d bytes to ios1->fd_out (%d)\n",
					rr, ios1->fd_out);
#endif
		}
		if (rr < 0) {
			if (errno != EAGAIN)
				fatal("error writing to fd %d: %s",
					ios1->fd_out, strerror(errno));
			break;
		}
	}
	
	while (is_empty(buf1) == FALSE) {
		rr = write_from_cb(ios2->fd_out, buf1);
		if (rr > 0) {
			net_sent += rr;
#ifndef NDEBUG				
			if (is_flag_set(VERY_VERBOSE_MODE) == TRUE)
				fprintf(stderr, "written %d bytes to ios2->fd_out (%d)\n",
					rr, ios2->fd_out);
#endif
		}
		if (rr < 0) {
			if (errno != EAGAIN)
				fatal("error writing to fd %d: %s",
					ios2->fd_out, strerror(errno));
			break;
		}
	}

	/* perform the final cleanup */
	free_cb(&buf1);
	if (tbuf1) free(tbuf1);
	free_cb(&buf2);
	
	shutdown_io_stream(ios1, SHUT_RDWR);
	shutdown_io_stream(ios2, SHUT_RDWR);

	if (is_flag_set(VERBOSE_MODE) == TRUE) {
		warn("connection closed (sent %d, rcvd %d)",
			net_sent, net_rcvd);
	}
}



void socket_to_io_stream(io_stream *ios, int fd, int socktype)
{
	/* nonblock(fd); */
	ios->fd_in  = fd;
	ios->fd_out = fd;
	ios->socktype = socktype;
}



void stdio_to_io_stream(io_stream *ios)
{
	if ((ios->fd_in  = dup(STDIN_FILENO)) < 0) 
		fatal("error in duplicating stdin file descriptor: %s", 
		      strerror(errno));
	
	if ((ios->fd_out = dup(STDOUT_FILENO)) < 0) 
		fatal("error in duplicating stdout file descriptor: %s", 
		      strerror(errno));

	/* nonblock(ios->fd_in); */
	/* nonblock(ios->fd_out); */

	ios->socktype = SOCK_STREAM;   /* pretend stdio is a stream socket */
}



static void shutdown_io_stream(io_stream* ios, int how)
{
	if (how == SHUT_RDWR) {
		/* close both the input and the output */
		if (ios->fd_in != -1)
			close(ios->fd_in);
		/* if the same fd is input and output, don't close twice */
		if (ios->fd_out != -1 && ios->fd_out != ios->fd_in)
			close(ios->fd_out);
		ios->fd_in = ios->fd_out = -1;
	} else if (how == SHUT_RD) {
		/* close the input */
		if (ios->fd_in != -1) {
			/* if the fd is duplex, use shutdown */
			if (ios->fd_in == ios->fd_out)
				shutdown(ios->fd_in, SHUT_RD);
			else
				close(ios->fd_in);
			ios->fd_in = -1;
		}
	} else {
		assert(how == SHUT_WR);		
		/* close the output */
		if (ios->fd_out != -1) {
			/* if the fd is duplex, use shutdown */
			if (ios->fd_in == ios->fd_out)
				shutdown(ios->fd_out, SHUT_WR);
			else
				close(ios->fd_out);
			ios->fd_out = -1;
		}
	}
}



#if 0
static void nonblock(int fd)
{
	int arg;
	if ((arg = fcntl(fd, F_GETFL, 0)) < 0)
		fatal("error reading file descriptor flags: %s", strerror(errno));

	arg |= O_NONBLOCK;

	if (fcntl(fd, F_SETFL, arg) < 0)
		fatal("error setting flag O_NONBLOCK on file descriptor",
			strerror(errno));
}
#endif
