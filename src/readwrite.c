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
#include "config.h"
#include "circ_buf.h"
#include "filter.h"
#include "misc.h"
#include "network.h"
#include "parser.h"
#include "readwrite.h"
#include "timeout.h"
#include "udp.h"

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

static int flush_buffer(circ_buf *cb, int fd);
static int sockaddr_len(struct sockaddr *sa);




/* ios1 is the remote stream, ios2 the local one */
void readwrite(io_stream *ios1, io_stream *ios2)
{
	int rr, max_fd;
	bool file_xfer_mode;
	bool timeout_mode;
	fd_set read_fdset, tmp_rd_fdset, write_fdset, tmp_wr_fdset;
	struct timeval timeout; 
	static circ_buf *buf1 = NULL;
	static circ_buf *buf2 = NULL;
	struct timeval timestamp1, timestamp2;
	bool t1,t2;
	
	/* check function arguments */
	assert(ios1 != NULL);
	assert(ios2 != NULL);

	/* setup circular buffers */
	buf1 = alloc_cb(BUFFER_SIZE);
	buf2 = alloc_cb(BUFFER_SIZE);

	/* check circular buffers validity */
	check_cb(buf1);
	check_cb(buf2);
	
	/* reset status */
	local_rcvd = 0;
	net_rcvd = 0;		
	local_sent = 0;
	net_sent = 0;		

	file_xfer_mode = is_flag_set(FILE_TRANSFER_MODE);
	timeout_mode   = is_flag_set(TIMEOUT_MODE);

	/* setup all the stuff for the select loop */
	FD_ZERO(&read_fdset);
	FD_ZERO(&write_fdset);

	if (file_xfer_mode == TRUE) {
		/* if we are in file transfer mode, setup unidirectional 
		 * data transfers */
		if (is_flag_set(LISTEN_MODE) == TRUE) {
			/* we close the remote stream for writing */
			if (ios1->is_tcp_socket == TRUE) {
				/* we don't check for errors here, 
				 * as we don't mind if shutdown fails */
				shutdown(ios1->fd_out, SHUT_WR);
			}
			
			/* we close the local stream for reading. 
			 * notice that this should not be needed, 
			 * as the local stream is not a TCP socket. */
			if (ios2->is_tcp_socket == TRUE) {
				/* we don't check for errors here, 
				 * as we don't mind if shutdown fails */
				shutdown(ios2->fd_in, SHUT_RD);
			}
			
			/* reading from the remote stream is ok */ 
			FD_SET(ios1->fd_in, &read_fdset);
			
			/* writing to the local stream is ok */
			FD_SET(ios2->fd_out, &write_fdset);
			
			max_fd = MAX(ios1->fd_in, ios2->fd_out);
		} else {
			/* we close the remote stream for reading */
			if (ios1->is_tcp_socket == TRUE) {
				/* we don't check for errors here, 
				 * as we don't mind if shutdown fails */
				shutdown(ios1->fd_in, SHUT_RD);
			}
			
			/* we close the local stream for writing. 
			 * notice that this should not be needed, 
			 * as the local stream is not a TCP socket. */
			if (ios2->is_tcp_socket == TRUE) {
				/* we don't check for errors here, 
				 * as we don't mind if shutdown fails */
				shutdown(ios2->fd_out, SHUT_WR);
			}
			
			/* writing to the remote stream is ok */
			FD_SET(ios1->fd_out, &write_fdset);
			
			/* reading from the local stream is ok */ 
			FD_SET(ios2->fd_in, &read_fdset);
		
			max_fd = MAX(ios1->fd_out, ios2->fd_in);
		}
	} else {
		int temp;
		
		/* if we are not in file transfer mode, setup bidirectional 
		 * data transfers */
		FD_SET(ios1->fd_in, &read_fdset);
		FD_SET(ios2->fd_in, &read_fdset);
		FD_SET(ios1->fd_out, &write_fdset);
		FD_SET(ios2->fd_out, &write_fdset);
		
		max_fd = MAX(ios1->fd_in, ios1->fd_out);
		temp   = MAX(ios2->fd_in, ios2->fd_out);
		if (temp > max_fd) max_fd = temp;
	}
	
	if (max_fd > FD_SETSIZE) {
		fatal("max_fd > FD_SETSIZE");
	}

	fprintf(stderr, "timeout_mode is %s\n", ((timeout_mode == TRUE) ? "TRUE": "FALSE"));
	
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
	while(((timeout_mode == FALSE) && 
		FD_ISSET(ios1->fd_in, &read_fdset)) ||
	      ((timeout_mode == TRUE) && 
	       ((FD_ISSET(ios1->fd_in, &read_fdset) || 
		 ((t1 = timeout_expired(&timestamp1, TIMEOUT1)) == FALSE)) && 
		(FD_ISSET(ios2->fd_in, &read_fdset) ||  
		 ((t2 = timeout_expired(&timestamp2, TIMEOUT2)) == FALSE))))) {  

		struct timeval *tmp;
#ifndef NDEBUG	
		if (0 && is_flag_set(VERY_VERBOSE_MODE) == TRUE) {
			int cond1, cond2;

			cond1 = FD_ISSET(ios1->fd_in, &read_fdset);
			cond2 = FD_ISSET(ios2->fd_in, &read_fdset);
		
			fprintf(stderr,"condition 1 is %s\n", (cond1 ? "TRUE": "FALSE"));	
			fprintf(stderr,"condition 2 is %s\n", (cond2 ? "TRUE": "FALSE"));
		}
		if (0 && is_flag_set(VERY_VERBOSE_MODE) == TRUE) {
			fprintf(stderr,"t1 is %s\n", ((t1 == TRUE) ? "TRUE": "FALSE"));	
			fprintf(stderr,"t2 is %s\n", ((t2 == TRUE) ? "TRUE": "FALSE"));
		}
#endif
		
		/* make a copy of read_fdset and write_fdset before passing 
		 * them to select */
		tmp_rd_fdset = read_fdset;
		tmp_wr_fdset = write_fdset;

		/* setup select timeout */
		if ((is_empty(buf1) == TRUE) && (is_empty(buf2) == TRUE)) { 
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
		    (is_full(buf1) == FALSE)) {
			/* something has been received from ios1. 
			 * let's read it. */
			rr = read_to_cb(ios1->fd_in, buf1);
			
			if (rr > 0) {
				net_rcvd += rr;
#ifndef NDEBUG
				if (is_flag_set(VERY_VERBOSE_MODE) == TRUE) {
					fprintf(stderr,"read %d bytes from "
						"ios1->fd_in (%d)\n", rr, 
						ios1->fd_in);
				}
#endif
			} else if (rr == 0) {
				/* if rr == 0, then we are not receiving
				 * data from ios1->fd_in anymore. 
				 * let's close it. */
				if (ios1->is_tcp_socket == TRUE) {
					/* we don't check for errors here, 
					 * as we don't mind if shutdown fails */
					shutdown(ios1->fd_in, SHUT_RD);
				} else {
					close(ios1->fd_in);
				}
#ifndef NDEBUG				
				if (is_flag_set(VERY_VERBOSE_MODE) == TRUE) {
					printf("setting timestamp1\n");
				}
#endif
				gettimeofday(&timestamp1, NULL);
				FD_CLR(ios1->fd_in, &read_fdset);
#ifndef NDEBUG				
				if (is_flag_set(VERY_VERBOSE_MODE) == TRUE) {
					fprintf(stderr, "closing ios1->fd_in (%d)\n",
						ios1->fd_in);
				}
#endif
			} else if (rr < 0) {
				/* error while reading ios1->fd_in: 
				 * print an error message and exit. */
				fatal("error reading from fd %d: %s", 
				      ios1->fd_in, strerror(errno));
			}
		}
		
		
		if (FD_ISSET(ios2->fd_in, &tmp_rd_fdset) &&	
		    (is_full(buf2) == FALSE)) {
			/* something has been received from ios2. 
			 * let's read it. */
			rr = read_to_cb(ios2->fd_in, buf2);
			
			if (rr > 0) {
				local_rcvd += rr;
#ifndef NDEBUG
				if (is_flag_set(VERY_VERBOSE_MODE) == TRUE) {
					fprintf(stderr,"read %d bytes from "
						"ios2->fd_in (%d)\n", rr, 
						ios2->fd_in);
				}
#endif
			} else if (rr == 0) {
				/* if rr == 0, then we are not receiving
				 * data from ios2->fd_in anymore. 
				 * let's close it. */
				if (ios2->is_tcp_socket == TRUE) {
					/* we don't check for errors here, 
					 * as we don't mind if shutdown fails */
					shutdown(ios2->fd_in, SHUT_RD);
				} else {
					close(ios2->fd_in);
				}
#ifndef NDEBUG				
				if (is_flag_set(VERY_VERBOSE_MODE) == TRUE) {
					printf("setting timestamp2\n");
				}
#endif
				gettimeofday(&timestamp2, NULL);
				FD_CLR(ios2->fd_in, &read_fdset);
#ifndef NDEBUG				
				if (is_flag_set(VERY_VERBOSE_MODE) == TRUE) {
					fprintf(stderr, "closing ios2->fd_in (%d)\n",
						ios2->fd_in);
				}
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
			if (is_flag_set(VERY_VERBOSE_MODE) == TRUE) {
				fprintf(stderr,
					"written %d bytes to ios1->fd_out (%d)\n",
					rr, ios1->fd_out);
			}
#endif
		}

		
		if (FD_ISSET(ios2->fd_out, &tmp_wr_fdset) &&
		    is_empty(buf1) == FALSE) {			
			rr = flush_buffer(buf1, ios2->fd_out);
#ifndef NDEBUG				
			if (is_flag_set(VERY_VERBOSE_MODE) == TRUE) {
				fprintf(stderr,
					"written %d bytes to ios2->fd_out (%d)\n",
					rr, ios2->fd_out);
			}
#endif
		}
	}
	
	if (is_flag_set(VERY_VERBOSE_MODE) == TRUE) {
		int cond1, cond2;

		cond1 = FD_ISSET(ios1->fd_in, &read_fdset);
		cond2 = FD_ISSET(ios2->fd_in, &read_fdset);
		fprintf(stderr,"condition 1 is %s\n", (cond1 ? "TRUE": "FALSE"));	
		fprintf(stderr,"condition 2 is %s\n", (cond2 ? "TRUE": "FALSE"));
		fprintf(stderr,"t1 is %s\n", ((t1 == TRUE) ? "TRUE": "FALSE"));	
		fprintf(stderr,"t2 is %s\n", ((t2 == TRUE) ? "TRUE": "FALSE"));
		fflush(stderr);
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

	/* perform the final cleanup */
	free_cb(&buf1);
	free_cb(&buf2);
	
	close(ios1->fd_in);
	if (ios1->is_tcp_socket == FALSE) {
		/* for TCP sockets fd_in == fd_out and we don't need 
		 * to close the same fd twice */
		close(ios1->fd_out);
	}
	
	close(ios2->fd_in);
	if (ios2->is_tcp_socket == FALSE) {
		/* for TCP sockets fd_in == fd_out and we don't need 
		 * to close the same fd twice */
		close(ios2->fd_out);
	}
}



void socket_to_io_stream(int fd, io_stream *ios)
{
#if 0
	nonblock(fd);
#endif
	
	ios->fd_in  = fd;
	ios->fd_out = fd;
	ios->is_tcp_socket = TRUE;
}



void stdio_to_io_stream(io_stream *ios)
{
	if ((ios->fd_in  = dup(STDIN_FILENO)) < 0) 
		fatal("error in duplicating stdin file descriptor: %s", 
		      strerror(errno));
	if ((ios->fd_out = dup(STDOUT_FILENO)) < 0) 
		fatal("error in duplicating stdout file descriptor: %s", 
		      strerror(errno));
	ios->is_tcp_socket = FALSE; 

#if 0
	nonblock(ios->fd_in);
	nonblock(ios->fd_out);
#endif
}



/* udpc->fd is the remote stream, ios the local one */
void udp_readwrite(struct udp_connection *udpc, io_stream *ios)
{
	int rr, max_fd, copied;
	bool file_xfer_mode;
	bool timeout_mode;
	bool listen_mode;
	fd_set read_fdset, tmp_rd_fdset, write_fdset, tmp_wr_fdset;
        struct timeval timeout;
	static circ_buf *buf1 = NULL;
	static circ_buf *buf2 = NULL;
	static uint8_t *tmpbuf = NULL;
	static int tmpbuf_bytes = 0;
	struct timeval timestamp; 

	/* check function arguments */
	assert(udpc != NULL);
	assert(ios  != NULL);

	/* setup circular and temporary buffers */
	buf1 = alloc_cb(BUFFER_SIZE);
	buf2 = alloc_cb(BUFFER_SIZE);
	tmpbuf = (uint8_t *)xmalloc(TEMP_BUFFER_SIZE);

	/* check circular buffers validity */
	check_cb(buf1);
	check_cb(buf2);

	/* reset status */
	local_rcvd = 0;
	net_rcvd = 0;
		
	file_xfer_mode = is_flag_set(FILE_TRANSFER_MODE);
	timeout_mode   = is_flag_set(TIMEOUT_MODE);
	timeout_mode   = is_flag_set(LISTEN_MODE);
	
	/* setup all the stuff for the select loop */
	FD_ZERO(&read_fdset);
	FD_ZERO(&write_fdset);

	if (file_xfer_mode == TRUE) {
		/* if we are in file transfer mode, setup unidirectional 
		 * data transfers */
		if (listen_mode == TRUE) {
			/* reading from the remote stream is ok */ 
			FD_SET(udpc->fd, &read_fdset);
			
			/* writing to the local stream is ok */
			FD_SET(ios->fd_out, &write_fdset);
			
			max_fd = MAX(udpc->fd, ios->fd_out);
		} else {
			/* writing to the remote stream is ok */
			FD_SET(udpc->fd, &write_fdset);
			
			/* reading from the local stream is ok */ 
			FD_SET(ios->fd_in, &read_fdset);
		
			max_fd = MAX(udpc->fd, ios->fd_in);
		}
	} else {
		int temp;
		
		/* if we are not in file transfer mode, setup bidirectional 
		 * data transfers */
		FD_SET(udpc->fd,    &read_fdset);
		FD_SET(ios->fd_in,  &read_fdset);
		FD_SET(udpc->fd,    &write_fdset);
		FD_SET(ios->fd_out, &write_fdset);
		
		temp   = MAX(ios->fd_out, ios->fd_in);
		max_fd = MAX(udpc->fd, temp);
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
	while(((timeout_mode == FALSE) && 
		FD_ISSET(udpc->fd, &read_fdset)) ||
	      ((timeout_mode == TRUE) && 
	       (FD_ISSET(udpc->fd, &read_fdset) && 
		(FD_ISSET(ios->fd_in, &read_fdset) || 
		 (timeout_expired(&timestamp, TIMEOUT1) == FALSE))))) {  
		
		struct timeval *tmp;
		
		/* make a copy of read_fdset and write_fdset before passing 
		 * them to select */
		tmp_rd_fdset = read_fdset;
		tmp_wr_fdset = write_fdset;

		/* setup select timeout */
		if (is_empty(buf1) == TRUE && is_empty(buf2) == TRUE && tmpbuf_bytes == 0) { 
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
		 * to be written in udpc->fd->fd_out or ios->fd_out. */
		if (rr < 0 && errno != EINTR) 
			fatal("select error: %s", strerror(errno));

		/* handle bytes left in the temporary buffer */
		if (tmpbuf_bytes > 0 && (is_full(buf1) == FALSE)) {
			/* forward data from temporary buffer to circular buffer */
			copied = copy_to_cb(tmpbuf, tmpbuf_bytes, buf1);
			tmpbuf_bytes -= copied;

			if (tmpbuf_bytes != 0) {
				memmove(tmpbuf, tmpbuf + copied, tmpbuf_bytes);
			}
		}
		
		if (FD_ISSET(udpc->fd, &tmp_rd_fdset) &&
		    is_full(buf1) == FALSE) {
			struct sockaddr_storage from;
			socklen_t fromlen = sizeof(from);
			
			/* something has been received from udpc->fd. 
			 * let's read it. */
			rr = recvfrom(udpc->fd, (void *)tmpbuf, BUFFER_SIZE, 0, 
				      (struct sockaddr *)&from, &fromlen);

			if (rr < 0 && errno != EINTR) {
				/* error while reading udpc->fd: 
				 * print an error message and exit. */
				fatal("error reading from fd %d: %s", 
				      udpc->fd, strerror(errno));
			}
			
			if (rr > 0) {
#ifndef NDEBUG
				if ((is_flag_set(VERY_VERBOSE_MODE) == TRUE) && 
			            (tmpbuf_bytes > 0)) {
					fprintf(stderr,"lost %d bytes received from the net!!!\n", 
						tmpbuf_bytes);
				}
#endif
				tmpbuf_bytes = rr;
			}
			
			/* this is for listen mode: if we don't have a specified remote 
			 * endpoint, get the source of the first packet as remote endpoint
			 * and accept packets only from the corresponding host. */
			if (udpc->destlen == FALSE) {
				udpc->destlen = sockaddr_len((struct sockaddr *)&from);
				assert(udpc->destlen <= sizeof(struct sockaddr_storage));
				memcpy(&udpc->dest, &from, udpc->destlen);
			}
			
			/* accept data only from the remote connection endpoint */
			if (((udpc->dest_addr == NULL) && 
			      are_address_equal((struct sockaddr *)&from, 
			                        (struct sockaddr *)&udpc->dest) == TRUE) || 
			    ((udpc->dest_addr != NULL) && 
			      is_allowed((struct sockaddr *)&from, udpc->dest_addr) == TRUE)) {
				/* count the received data as valid */
				net_rcvd += tmpbuf_bytes;
				
				/* forward data from temporary buffer to circular buffer */
				copied = copy_to_cb(tmpbuf, tmpbuf_bytes, buf1);
				tmpbuf_bytes -= copied;

				if (tmpbuf_bytes != 0) {
					memmove(tmpbuf, tmpbuf + copied, tmpbuf_bytes);
				}
#ifndef NDEBUG
				if (is_flag_set(VERY_VERBOSE_MODE) == TRUE) {
					fprintf(stderr,"read %d bytes from "
						"udpc->fd (%d)\n", rr, 
						udpc->fd);
				}
#endif
			} else {
				tmpbuf_bytes = 0;
			}
		}
		
		
		if (FD_ISSET(ios->fd_in, &tmp_rd_fdset) && 
		    (is_full(buf2) == FALSE)) {
			/* something has been received from ios. 
			 * let's read it. */
			rr = read_to_cb(ios->fd_in, buf2);
			
			if (rr > 0) {
				/* count the received data as valid */
				local_rcvd += rr;
#ifndef NDEBUG
				if (is_flag_set(VERY_VERBOSE_MODE) == TRUE) {
					fprintf(stderr,"read %d bytes from "
						"ios->fd_in (%d)\n", rr, 
						ios->fd_in);
				}
#endif
			} else if (rr == 0) {
				/* if rr == 0, then we are not receiving
				 * data from ios->fd_in anymore. 
				 * let's close it. */
				if (ios->is_tcp_socket == TRUE) {
					shutdown(ios->fd_in, SHUT_RD);
				} else {
					close(ios->fd_in);
				}
				gettimeofday(&timestamp, NULL);
				FD_CLR(ios->fd_in, &read_fdset);
#ifndef NDEBUG				
				if (is_flag_set(VERY_VERBOSE_MODE) == TRUE) {
					fprintf(stderr, "closing ios->fd_in (%d)\n",
						ios->fd_in);
				}
#endif
			} else if (rr < 0) {
				/* error while reading ios->fd_in: 
				 * print an error message and exit. */
				fatal("error reading from fd: %d; %s", 
				      ios->fd_in, strerror(errno));
			}
		}

		
		/* if we are in listen mode, we must check that we know 
		 * the address of the remote endpoint before flushing 
		 * this buffer */
		if (!((listen_mode == TRUE) && (udpc->destlen > 0)) &&
		    ((FD_ISSET(udpc->fd, &tmp_wr_fdset)) &&
		     is_empty(buf2) == FALSE)) {			
			rr = send_from_cb(udpc->fd, buf2, 
				(const struct sockaddr *)&udpc->dest, 
				udpc->destlen);
#ifndef NDEBUG				
			if (is_flag_set(VERY_VERBOSE_MODE) == TRUE) {
				fprintf(stderr,
					"written %d bytes to udpc->fd (%d)\n",
					rr, udpc->fd);
			}
#endif
		} 

		
		if ((FD_ISSET(ios->fd_out, &tmp_wr_fdset)) &&
		    is_empty(buf1) == FALSE) {			
			rr = flush_buffer(buf1, ios->fd_out);
#ifndef NDEBUG				
			if (is_flag_set(VERY_VERBOSE_MODE) == TRUE) {
				fprintf(stderr,
					"written %d bytes to ios->fd_out (%d)\n",
					rr, ios->fd_out);
			}
#endif
			/* handle bytes left in the temporary buffer */
			if (tmpbuf_bytes > 0 && (is_full(buf1) == FALSE)) {
				
				/* forward data from temporary buffer to circular buffer */
				copied = copy_to_cb(tmpbuf, tmpbuf_bytes, buf1);
				tmpbuf_bytes -= copied;

				if (tmpbuf_bytes != 0) {
					memmove(tmpbuf, tmpbuf + copied, tmpbuf_bytes);
				}
			}
		}
	}
	
	/* perform the final cleanup */
	free_cb(&buf1);
	free_cb(&buf2);
	free(tmpbuf);

	close(udpc->fd);
	close(ios->fd_in);
	close(ios->fd_out);
}



static int flush_buffer(circ_buf *cb, int fd)
{
	int rr;
	
	rr = write_from_cb(fd, cb);
	if (rr < 0) {
		/* error while writing to fd: 
		 * print an error message and exit. */
		fatal("error writing to fd %d: %s", fd, strerror(errno));
	}

	return rr;
}



#if 0
static void nonblock(int fd) 
{
	long arg;
	
	if ((arg = fcntl(fd, F_GETFL, 0)) < 0)
		fatal("error reading flags of file descriptor %d: %s", 
                      fd, strerror(errno));
	
	arg |= O_NONBLOCK;
	
	if (fcntl(fd, F_SETFL, arg) < 0)
		fatal("error setting flag O_NONBLOCK for file descriptor %d: %s", 
		      fd, strerror(errno));
}
#endif



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
		fatal("address family %d is not supported", sa->sa_family);
	}
	
	return ret;
}



