/*
 *  timeout.h - timeout handling module  - header 
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
#include <sys/time.h>
#include "misc.h"

/* 
 * TIMEOUT1 is for the timeout from the last time we have received data from the user 
 * TIMEOUT2 is for the timeout from the last time we have received data from the net 
 */
#define TIMEOUT1	1
#define TIMEOUT2	2
#define SET_TIMEOUT1	0x0001
#define SET_TIMEOUT2	0x0002

bool timeout_expired(const struct timeval *tstamp, int timeout_num);
void set_timeouts(int tout1, int tout2, int flags);

