/*
 *  timeout.c - timeout handling module  - implementation 
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
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "misc.h"
#include "timeout.h"

static double timeout1; 
static double timeout2; 
static int timeout_flags;


static double to_double(const struct timeval *t) 
{
	assert(t != NULL);
	
	return (double)(t->tv_sec) * 1000000.0 + (double)(t->tv_usec);	
}



bool timeout_expired(const struct timeval *tstamp, int timeout_num)
{
	struct timeval now;
	double dnow, temp;
	bool ret;

	assert(tstamp != NULL);
	
	if (gettimeofday(&now, NULL) != 0) {
		perror("gettimeofday");
		exit(EXIT_FAILURE);
	}
	
	dnow = to_double(&now);
	temp = to_double(tstamp);
	ret = FALSE;
	
	switch (timeout_num) {
		case TIMEOUT1:
			if ((timeout_flags & SET_TIMEOUT1) && 
			    (dnow - temp > timeout1)) 
				ret = TRUE;
			break;
		case TIMEOUT2:
			if ((timeout_flags & SET_TIMEOUT2) && 
			    (dnow - temp > timeout2)) 
				ret = TRUE;
			break;
		default:
			fatal("internal error with timeout_num: please "
	                      "contact the authors of nc6 for bugfixing ;-)");
	}
	
	return ret;
}



void set_timeouts(int tout1, int tout2, int flags)
{
	assert(tout1 >= 0);
	assert(tout2 >= 0);
	assert(flags);
	assert(!(flags & ~(SET_TIMEOUT1 | SET_TIMEOUT2)));
	
	timeout1 = (double)tout1 * 1000000.0;
	timeout2 = (double)tout2 * 1000000.0;
	timeout_flags = flags;
}

