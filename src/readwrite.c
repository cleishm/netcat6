/*
 *  readwrite.c - stream i/o reading/writing loop - implementation 
 * 
 *  nc6 - an advanced netcat clone
 *  Copyright (C) 2001-2003 Mauro Tortonesi <mauro _at_ deepspace6.net>
 *  Copyright (C) 2002-2003 Chris Leishman <chris _at_ leishman.org>
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

RCSID("@(#) $Header: /Users/cleishma/work/nc6-repo/nc6/src/readwrite.c,v 1.22 2002-12-30 22:35:47 chris Exp $");


/* ios1 is the remote stream, ios2 the local one */
int readwrite(io_stream *ios1, io_stream *ios2)
{
	int rr, max_fd = -1;
	int ios1_read_fd, ios1_write_fd;
	int ios2_read_fd, ios2_write_fd;
	fd_set read_fdset, write_fdset;
	struct timeval tv1, tv2;
	struct timeval *tvp1, *tvp2, *tvp;
	int retval = 0;
	size_t local_rcvd = 0;
	size_t net_rcvd   = 0;		
	size_t local_sent = 0;
	size_t net_sent   = 0;		
	bool half_close_mode = is_flag_set(HALF_CLOSE_MODE);
#ifndef NDEBUG
	bool very_verbose_mode = is_flag_set(VERY_VERBOSE_MODE);
#endif
	
	/* check function arguments */
	assert(ios1 != NULL);
	assert(ios2 != NULL);

	/* setup all the stuff for the select loop */

	/* here's the select loop. 
	 *
	 * the loop continues until one of the following occurs:
	 *
	 * neither side needs to read or write
	 * OR
	 * either side times out
	 * OR
	 * a write error occurs
	 *
	 * note: by default, when the remote read side (ios1) is closed, it
	 * triggers a hold timeout immediately - which closes the local read
	 * side as well.
	 */
	for (;;) {
		/* setup fdsets */
		FD_ZERO(&read_fdset);
		FD_ZERO(&write_fdset);

		ios1_read_fd  = ios_schedule_read(ios1);
		ios1_write_fd = ios_schedule_write(ios1);
		ios2_read_fd  = ios_schedule_read(ios2);
		ios2_write_fd = ios_schedule_write(ios2);

		max_fd = -1;
		if (ios1_read_fd >= 0) {
			FD_SET(ios1_read_fd, &read_fdset);
			max_fd = ios1_read_fd;
		}
		if (ios1_write_fd >= 0) {
			FD_SET(ios1_write_fd, &write_fdset);
			max_fd = MAX(ios1_write_fd, max_fd);
		}
		if (ios2_read_fd >= 0) {
			FD_SET(ios2_read_fd, &read_fdset);
			max_fd = MAX(ios2_read_fd, max_fd);
		}
		if (ios2_write_fd >= 0) {
			FD_SET(ios2_write_fd, &write_fdset);
			max_fd = MAX(ios2_write_fd, max_fd);
		}

		/* stop loop if nothing is to be read or written */
		if (max_fd == -1)
			break;

		/* check timeouts */
		tvp1 = ios_next_timeout(ios1, &tv1);
		tvp2 = ios_next_timeout(ios2, &tv2);

#ifndef NDEBUG
		if (very_verbose_mode == TRUE) {
			if (tvp1)
				warn("ios1 timer expires in %d.%06d",
				     tv1.tv_sec, tv1.tv_usec);
			if (tvp2)
				warn("ios2 timer expires in %d.%06d",
				     tv2.tv_sec, tv2.tv_usec);
		}
#endif

		/* stop loop if either ios has timed out */
		if ((tvp1 && !timerisset(tvp1))||(tvp2 && !timerisset(tvp2))) {
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

		/* blocking select with timeout */		
		rr = select(max_fd + 1, &read_fdset, &write_fdset, NULL, tvp);

		/* handle select errors.
		 * if errno == EINTR we just retry select */
		if (rr < 0) {
			if (errno == EINTR) 
				continue;
			fatal("select error: %s", strerror(errno));
		}
		
		if (ios1_read_fd >= 0 && FD_ISSET(ios1_read_fd, &read_fdset)) {
			/* ios1 is ready to read */
			rr = ios_read(ios1);

			if (rr > 0) {
				net_rcvd += rr;
#ifndef NDEBUG
				if (very_verbose_mode == TRUE)
					warn("read %d bytes from ios1", rr);
#endif
			} else if (rr == 0) {
#ifndef NDEBUG				
				if (very_verbose_mode == TRUE) {
					warn("closing read of ios1");
					if (half_close_mode == TRUE)
						warn("closing write of ios2");
				}
#endif
				ios_shutdown(ios1, SHUT_RD);
				/* send a half close to ios2 */
				if (half_close_mode == TRUE)
					ios_shutdown(ios2, SHUT_WR);
			} else if (rr < 0 && errno != EAGAIN) {
				/* error while reading ios1:
				 * print an error message and exit. */
				fatal("error reading from ios1: %s", 
				      strerror(errno));
			}
		}

		if (ios2_read_fd >= 0 && FD_ISSET(ios2_read_fd, &read_fdset)) {
			/* ios2 is ready to read */
			rr = ios_read(ios2);

			if (rr > 0) {
				local_rcvd += rr;
#ifndef NDEBUG
				if (very_verbose_mode == TRUE)
					warn("read %d bytes from ios2", rr);
#endif
			} else if (rr == 0) {
#ifndef NDEBUG				
				if (very_verbose_mode == TRUE) {
					warn("closing read of ios2");
					if (half_close_mode == TRUE)
						warn("closing write of ios2");
				}
#endif
				ios_shutdown(ios2, SHUT_RD);
				/* send a half close to ios1 */
				if (half_close_mode == TRUE)
					ios_shutdown(ios1, SHUT_WR);
			} else if (rr < 0 && errno != EAGAIN) {
				/* error while reading ios2:
				 * print an error message and exit. */
				fatal("error reading from ios2: %s", 
				      strerror(errno));
			}
		}

		if (ios1_write_fd >= 0 && FD_ISSET(ios1_write_fd, &write_fdset))
		{
			/* ios1 is ready to write */
			rr = ios_write(ios1);

			if (rr > 0) {
				net_sent += rr;
#ifndef NDEBUG				
				if (very_verbose_mode == TRUE)
					warn("wrote %d bytes to ios1", rr);
#endif
			} else if (rr < 0 && errno != EAGAIN) {
				/* error while writing to ios1:
				 * print an error message and exit. */
				if (errno != EPIPE)
					fatal("error writing to fd %d: %s",
					      ios1_write_fd, strerror(errno));

				/* the pipe is broken,
				 * clear buffer and close read/write */
#ifndef NDEBUG
				if (very_verbose_mode == TRUE)
					warn("received SIGPIPE on ios1",
					     ios1_write_fd);
#endif

				ios_shutdown(ios1, SHUT_RDWR);

				/* exit the main loop */
				retval = -1;
				break;
			}

		}

		if (ios2_write_fd >= 0 && FD_ISSET(ios2_write_fd, &write_fdset))
		{
			/* ios2 is ready to write */
			rr = ios_write(ios2);

			if (rr > 0) {
				local_sent += rr;
#ifndef NDEBUG				
				if (very_verbose_mode == TRUE)
					warn("wrote %d bytes to ios2", rr);
#endif
			} else if (rr < 0 && errno != EAGAIN) {
				/* error while writing to ios2:
				 * print an error message and exit. */
				if (errno != EPIPE)
					fatal("error writing to fd %d: %s",
					      ios2_write_fd, strerror(errno));

				/* the pipe is broken,
				 * clear buffer and close read/write */
#ifndef NDEBUG
				if (very_verbose_mode == TRUE)
					warn("received SIGPIPE on ios2",
					     ios2_write_fd);
#endif

				ios_shutdown(ios2, SHUT_RDWR);

				/* exit the main loop */
				retval = -1;
				break;
			}

		}
	}
	
	if (is_flag_set(VERBOSE_MODE) == TRUE)
		warn("connection closed (sent %d, rcvd %d)",
		     net_sent, net_rcvd);

	return retval;
}
