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
#include "config.h"
#include "readwrite.h"
#include "misc.h"
#include "circ_buf.h"
#include "parser.h"
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

/* buffer size is 8kb */
static const size_t BUFFER_SIZE = 8192;
static const size_t TEMP_BUFFER_SIZE = 65536;

/* bytes received, respectively, from local input and from the net */
static int local_rcvd, net_rcvd; 
/* bytes sent, respectively, to local output and to the net */
static int local_sent, net_sent; 



/* ios1 is the remote stream, ios2 the local one */
int readwrite(io_stream *ios1, io_stream *ios2)
{
	int rr, max_fd;
	bool file_xfer_mode;
	bool listen_mode;
	fd_set read_fdset, tmp_rd_fdset, write_fdset, tmp_wr_fdset;
	circ_buf *buf1 = NULL;
	uint8_t *tbuf1 = NULL;
	int tbuf1_bytes = 0, tbuf1_offset = 0;
	circ_buf *buf2 = NULL;
	struct timeval tv1, tv2;
	struct timeval *tvp1, *tvp2, *tvp;
	int retval = 0;
	
	/* check function arguments */
	assert(ios1 != NULL);
	assert(ios2 != NULL);

	/* set up the buffers for each ios */
	/* dgram protocols need to read all of each message at once, so we use
	 * a temporary buffer to read into, then move the data gradually into
	 * the circular buffer.  Further reads are suspended until all data
	 * is moved */
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

	/* setup all the stuff for the select loop */
	FD_ZERO(&read_fdset);
	FD_ZERO(&write_fdset);

	if (file_xfer_mode == TRUE) {
		/* if we are in file transfer mode, setup unidirectional 
		 * data transfers */
		if (listen_mode == TRUE) {
			/* reading only from the remote stream */ 
			FD_SET(ios_readfd(ios1), &read_fdset);

			/* close the remote stream for writing */
			ios_shutdown(ios1, SHUT_WR);

			/* close the local stream for reading */
			ios_shutdown(ios2, SHUT_RD);

			/* don't stop because the read is closed */
			ios_set_hold_timeout(ios2, -1);
			
			max_fd = MAX(ios_readfd(ios1), ios_writefd(ios2));

#ifndef NDEBUG
			if (is_flag_set(VERY_VERBOSE_MODE) == TRUE)
				warn("File xfter mode: reading ios1 (remote) only");
#endif
		} else {
			/* reading only from the local stream */ 
			FD_SET(ios_readfd(ios2), &read_fdset);
		
			/* close the remote stream for reading */
			ios_shutdown(ios1, SHUT_RD);

			/* don't stop because the read is closed */
			ios_set_hold_timeout(ios1, -1);
			
			/* close the local stream for writing */
			ios_shutdown(ios2, SHUT_WR);
			
			max_fd = MAX(ios_writefd(ios1), ios_readfd(ios2));

#ifndef NDEBUG
			if (is_flag_set(VERY_VERBOSE_MODE) == TRUE)
				warn("File xfter mode: reading ios2 (local) only");
#endif
		}
	} else {
		int temp;
		
		/* if we are not in file transfer mode, setup bidirectional 
		 * data transfers */
		FD_SET(ios_readfd(ios1), &read_fdset);
		FD_SET(ios_readfd(ios2), &read_fdset);

		max_fd = MAX(ios_readfd(ios1), ios_writefd(ios1));
		temp   = MAX(ios_readfd(ios2), ios_writefd(ios2));
		if (temp > max_fd) max_fd = temp;
	}
	
	if (max_fd > FD_SETSIZE) {
		/* with 4 fd's this shouldn't happen */
		fatal("max_fd > FD_SETSIZE");
	}

	
	/* here's the select loop. 
	 *
	 * the loop continues until one of the following occurs:
	 *
	 * neither side can be read from AND all write buffers have been written
	 * OR
	 * either side times out
	 * OR
	 * a write error occurs
	 *
	 * note: by default, when the remote read side (ios1) is closed, it
	 * triggers a hold timeout immediately - which closes the local read side
	 * as well.
	 */
	while (is_read_open(ios1) || is_read_open(ios2) ||
	       is_empty(buf1) == FALSE || is_empty(buf2) == FALSE)
	{

		/* sanity checks */
		/* writefd should be set iff the buffer contains data */
		assert(XOR(is_empty(buf1) == TRUE,
		    is_write_open(ios2) && FD_ISSET(ios_writefd(ios2), &write_fdset)));
		assert(XOR((is_empty(buf2) == TRUE),
		    is_write_open(ios1) && FD_ISSET(ios_writefd(ios1), &write_fdset)));
		/* readfd should be set (or closed) iff the buffer is not full
		 * or tbuf1 is in use and it still contains data */
		assert(XOR(
		    (tbuf1 && tbuf1_bytes > 0) || (!tbuf1 && is_full(buf1) == TRUE),
		    !is_read_open(ios1) || FD_ISSET(ios_readfd(ios1), &read_fdset)));
		assert(XOR(is_full(buf2) == TRUE,
		    !is_read_open(ios2) || FD_ISSET(ios_readfd(ios2), &read_fdset)));
		/* if tbuf1 is not being used, tbuf1_bytes must be 0 */
		assert(tbuf1 || tbuf1_bytes == 0);

		/* check timeouts */
		tvp1 = ios_next_timeout(ios1, &tv1);
		tvp2 = ios_next_timeout(ios2, &tv2);

#ifndef NDEBUG
		if (is_flag_set(VERY_VERBOSE_MODE) == TRUE) {
			if (tvp1)
				warn("ios1 timer expires in %d.%06d", tv1.tv_sec, tv1.tv_usec);
			if (tvp2)
				warn("ios2 timer expires in %d.%06d", tv2.tv_sec, tv2.tv_usec);
		}
#endif

		/* stop if either ios has timed out */
		if ((tvp1 && !timerisset(tvp1)) || (tvp2 && !timerisset(tvp2))) {
			retval = -1;
			break;
		}

		/* select smallest timeout for select */
		if (tvp1 && tvp2)
			tvp = timercmp(tvp1, tvp2, <)? tvp1 : tvp2;
		else if (tvp1)
			tvp = tvp1;
		else
			tvp = tvp2;  /* tvp2 may be NULL */

		/* make a copy of read_fdset and write_fdset before passing 
		 * them to select */
		memcpy(&tmp_rd_fdset, &read_fdset, sizeof(fd_set));
		memcpy(&tmp_wr_fdset, &write_fdset, sizeof(fd_set));

		/* blocking select with timeout */		
		rr = select(max_fd + 1, &tmp_rd_fdset, &tmp_wr_fdset, NULL, tvp);

		/* handle select errors. if errno == EINTR we just retry select */
		if (rr < 0) {
			if (errno == EINTR) 
				continue;
			fatal("select error: %s", strerror(errno));
		}
		
		if (is_read_open(ios1) && FD_ISSET(ios_readfd(ios1), &tmp_rd_fdset)) {
			assert((tbuf1 && tbuf1_bytes == 0) ||
			       (!tbuf1 && is_full(buf1) == FALSE));

			/* something is ready to read on ios1 (remote) */
			if (ios1->socktype == SOCK_DGRAM) {
				assert(tbuf1 != NULL);
				rr = recv(ios_readfd(ios1), (void *)tbuf1, TEMP_BUFFER_SIZE, 0);
			} else {
				rr = read_to_cb(ios_readfd(ios1), buf1);
			}
			
			if (rr > 0) {
				net_rcvd += rr;
#ifndef NDEBUG
				if (is_flag_set(VERY_VERBOSE_MODE) == TRUE)
					warn("%s %d bytes from ios1 (%d)",
						tbuf1? "read":"received", rr, ios_readfd(ios1));
#endif
				/* forward any data in tbuf1 to the circular buffer */
				if (tbuf1) {
					int copied = copy_to_cb(tbuf1, rr, buf1);
					tbuf1_bytes = rr - copied;
					tbuf1_offset = copied;
					/* some MUST have gone into the circular buffer */
					assert(copied);
				}

				/* if reading into tbuf1 and it still contains data
				 * OR if reading straight to buf1 and it's full,
				 * then suspend further reading */
				if ((tbuf1 && tbuf1_bytes > 0) ||
				    (!tbuf1 && is_full(buf1) == TRUE))
				{
					FD_CLR(ios_readfd(ios1), &read_fdset);
				}
				    
				/* start writing the buffer out */
				assert(is_write_open(ios2));
				FD_SET(ios_writefd(ios2), &write_fdset);
			} else if (rr == 0) {
				/* if rr == 0, then we are not receiving
				 * data from ios1 anymore. 
				 * let's close it. */
#ifndef NDEBUG				
				if (is_flag_set(VERY_VERBOSE_MODE) == TRUE)
					warn("closing read of ios1 (%d)", ios_readfd(ios1));
#endif
				FD_CLR(ios_readfd(ios1), &read_fdset);
				ios_shutdown(ios1, SHUT_RD);
			} else if (rr < 0 && errno != EAGAIN) {
				/* error while reading ios1: 
				 * print an error message and exit. */
				fatal("error reading from fd %d: %s", 
				      ios_readfd(ios1), strerror(errno));
			}
		}
		
		
		if (is_read_open(ios2) && FD_ISSET(ios_readfd(ios2), &tmp_rd_fdset)) {
			assert(is_full(buf2) == FALSE);

			/* something is ready to read on ios2 (local) */
			rr = read_to_cb(ios_readfd(ios2), buf2);
			
			if (rr > 0) {
				local_rcvd += rr;
#ifndef NDEBUG
				if (is_flag_set(VERY_VERBOSE_MODE) == TRUE)
					warn("read %d bytes from ios2 (%d)", rr, ios_readfd(ios2));
#endif
				/* if the buffer is full, suspend further reading */
				if (is_full(buf2) == TRUE)
					FD_CLR(ios_readfd(ios2), &read_fdset);

				/* start writing the buffer out */
				assert(is_write_open(ios1));
				FD_SET(ios_writefd(ios1), &write_fdset);
			} else if (rr == 0) {
				/* if rr == 0, then we are not receiving
				 * data from ios2 anymore. 
				 * let's close it. */
#ifndef NDEBUG				
				if (is_flag_set(VERY_VERBOSE_MODE) == TRUE)
					warn("closing read of ios2 (%d)", ios_readfd(ios2));
#endif
				FD_CLR(ios_readfd(ios2), &read_fdset);
				ios_shutdown(ios2, SHUT_RD);
			} else if (rr < 0 && errno != EAGAIN) {
				/* error while reading ios2: 
				 * print an error message and exit. */
				fatal("error reading from fd %d: %s", 
				      ios_readfd(ios2), strerror(errno));
			}
		}
		

		if (is_write_open(ios1) && FD_ISSET(ios_writefd(ios1), &tmp_wr_fdset)) {
			assert(is_empty(buf2) == FALSE);

			/* ios1 may be written to (remote) */
			if (tbuf1)
				rr = send_from_cb(ios_writefd(ios1), buf2, NULL, 0);
			else
				rr = write_from_cb(ios_writefd(ios1), buf2);

			if (rr > 0) {
				net_sent += rr;
#ifndef NDEBUG				
				if (is_flag_set(VERY_VERBOSE_MODE) == TRUE)
					warn("%s %d bytes to ios1 (%d)",
						tbuf1? "wrote":"sent", rr, ios_writefd(ios1));
#endif
				/* if anything was written, ios2 can resume reading again */
				if (is_read_open(ios2))
					FD_SET(ios_readfd(ios2), &read_fdset);
				/* if the buffer is emptied, stop trying to write */
				if (is_empty(buf2) == TRUE)
					FD_CLR(ios_writefd(ios1), &write_fdset);
			} else if (rr < 0 && errno != EAGAIN) {
				/* error while writing to ios1:
				 * print an error message and exit. */
				if (errno != EPIPE)
					fatal("error writing to fd %d: %s",
						ios_writefd(ios1), strerror(errno));

				/* the pipe is broken, clear buffer and close read/write */
#ifndef NDEBUG
				if (is_flag_set(VERY_VERBOSE_MODE) == TRUE)
					warn("received SIGPIPE on ios1 (%d)", ios_writefd(ios1));
#endif

				clear_cb(buf2);
				ios_shutdown(ios1, SHUT_RDWR);

				/* exit the main loop */
				retval = -1;
				break;
			}
		}

		
		if (is_write_open(ios2) && FD_ISSET(ios_writefd(ios2), &tmp_wr_fdset)) {
			assert(is_empty(buf1) == FALSE);

			/* ios2 may be written to (local) */
			rr = write_from_cb(ios_writefd(ios2), buf1);

			if (rr > 0) {
				local_sent += rr;
#ifndef NDEBUG				
				if (is_flag_set(VERY_VERBOSE_MODE) == TRUE)
					warn("wrote %d bytes to ios2 (%d)", rr, ios_writefd(ios2));
#endif
				/* forward more from tbuf1 to the circular buffer */
				if (tbuf1_bytes > 0) {
					int copied = copy_to_cb(
							tbuf1+tbuf1_offset, tbuf1_bytes, buf1);
					tbuf1_bytes -= copied;
					tbuf1_offset += copied;
				}
				/* if the tbuf1 was emptied, ios1 can resume reading again
				 * if tbuf1 is not in use, tbuf1_bytes will always be 0 */
				if (is_read_open(ios1) && tbuf1_bytes == 0)
					FD_SET(ios_readfd(ios1), &read_fdset);
				/* if the buffer is emptied, stop trying to write */
				if (is_empty(buf1) == TRUE)
					FD_CLR(ios_writefd(ios2), &write_fdset);
			} else if (rr < 0 && errno != EAGAIN) {
				/* error while writing to ios1:
				 * print an error message and exit. */
				if (errno != EPIPE)
					fatal("error writing to fd %d: %s",
						ios_writefd(ios2), strerror(errno));

				/* the pipe is broken, clear buffer and close read/write */
#ifndef NDEBUG
				if (is_flag_set(VERY_VERBOSE_MODE) == TRUE)
					warn("received SIGPIPE on ios2 (%d)", ios_writefd(ios2));
#endif

				clear_cb(buf1);
				ios_shutdown(ios2, SHUT_RDWR);

				/* exit the main loop */
				retval = -1;
				break;
			}
		}
	}
	
	/* perform the final cleanup */
	free_cb(&buf1);
	if (tbuf1) free(tbuf1);
	free_cb(&buf2);
	
	if (is_flag_set(VERBOSE_MODE) == TRUE)
		warn("connection closed (sent %d, rcvd %d)",
			net_sent, net_rcvd);

	return retval;
}
