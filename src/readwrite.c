/*
 *  readwrite.c - stream i/o reading/writing loop - implementation 
 * 
 *  nc6 - an advanced netcat clone
 *  Copyright (C) 2001-2005 Mauro Tortonesi <mauro _at_ deepspace6.net>
 *  Copyright (C) 2002-2005 Chris Leishman <chris _at_ leishman.org>
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
#include "system.h"
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

RCSID("@(#) $Header: /Users/cleishma/work/nc6-repo/nc6/src/readwrite.c,v 1.42 2005-08-18 04:13:13 chris Exp $");



/* ios1 is the remote stream, ios2 the local one */
int readwrite(io_stream_t *ios1, io_stream_t *ios2)
{
	int rr, max_fd = -1;
	int ios1_read_fd, ios1_write_fd;
	int ios2_read_fd, ios2_write_fd;
	fd_set read_fdset, write_fdset;
	struct timeval tv1, tv2;
	struct timeval *tvp1, *tvp2, *tvp;
	bool timedout1 = false, timedout2 = false;
	int retval = 0;
	
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
		tvp1 = tvp2 = NULL;
		if (!timedout1) {
			tvp1 = ios_next_timeout(ios1, &tv1);

			/* handle timeouts */
			if (ios_idle_timedout(ios1)) {
				/* stop the readwrite loop */
				retval = -1;
				break;
			}
			if (ios_hold_timedout(ios1)) {
				/* stop reading from the other endpoint */
				ios_shutdown(ios2, SHUT_RD);
				/* stop sending to this endpoint */
				ios_shutdown(ios1, SHUT_WR);
				timedout1 = true;
				continue;
			}
		}
		if (!timedout2) {
			tvp2 = ios_next_timeout(ios2, &tv2);

			/* handle timeouts */
			if (ios_idle_timedout(ios2)) {
				/* stop the readwrite loop */
				retval = -1;
				break;
			}
			if (ios_hold_timedout(ios2)) {
				/* stop reading from the other endpoint */
				ios_shutdown(ios1, SHUT_RD);
				/* stop sending to this endpoint */
				ios_shutdown(ios2, SHUT_WR);
				timedout2 = true;
				continue;
			}
		}

		/* select smallest timeout for select */
		if (tvp1 != NULL) {
			if (tvp2 != NULL)
				tvp = timercmp(tvp1, tvp2, <) ? tvp1 : tvp2;
			else
				tvp = tvp1;
		} else {
			tvp = tvp2;  /* tvp2 may be NULL */
		}

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

			if (rr < 0) {
				if (rr == IOS_EOF) {
					ios_write_eof(ios2);
				} else {
					/* something bad happened -
					 * exit the main loop */
					retval = -1;
					break;
				}
			}
		}

		if (ios2_read_fd >= 0 && FD_ISSET(ios2_read_fd, &read_fdset)) {
			/* ios2 is ready to read */
			rr = ios_read(ios2);

			if (rr < 0) {
				if (rr == IOS_EOF) {
					ios_write_eof(ios1);
				} else {
					/* something bad happened -
					 * exit the main loop */
					retval = -1;
					break;
				}
			}
		}

		if (ios1_write_fd >= 0 && FD_ISSET(ios1_write_fd, &write_fdset))
		{
			/* ios1 is ready to write */
			rr = ios_write(ios1);

			if (rr < 0) {
				/* write failed -
				 * exit the main loop */
				retval = -1;
				break;
			}
		}

		if (ios2_write_fd >= 0 && FD_ISSET(ios2_write_fd, &write_fdset))
		{
			/* ios2 is ready to write */
			rr = ios_write(ios2);

			if (rr < 0) {
				/* write failed -
				 * exit the main loop */
				retval = -1;
				break;
			}
		}
	}
	
	return retval;
}
