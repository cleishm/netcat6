/*
 *  misc.c - miscellaneous funcions module - implementation 
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
#include "misc.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#ifdef HAVE_STRTOL
#include <errno.h>
#include <limits.h>
#endif

RCSID("@(#) $Header: /Users/cleishma/work/nc6-repo/nc6/src/misc.c,v 1.13 2003-01-11 14:05:48 simone Exp $");



/* exit the program with an error message */
void fatal(const char *template, ...)
{
	va_list ap;
	
	assert(template != NULL);
						      
	fprintf(stderr, "%s: ", get_program_name());
							       
	va_start(ap, template);
	vfprintf(stderr, template, ap);
	va_end(ap);

	fprintf(stderr, "\n");
	
	exit(EXIT_FAILURE);
}



void warn(const char *template, ...)
{
	va_list ap;

	assert(template != NULL);
						      
	fprintf(stderr, "%s: ", get_program_name());
							       
	va_start(ap, template);
	vfprintf(stderr, template, ap);
	va_end(ap);

	fprintf(stderr, "\n");
}



/* same as `malloc' but report error if no memory available */
void *xmalloc(size_t size)
{
	register void *value = malloc(size);
	
	if (value == NULL) fatal(_("virtual memory exhausted"));

	return value;
}



char *xstrdup(const char *str)
{
	register char *nstr = (char *)xmalloc(strlen(str));
	/* we should use srtlcpy here instead of strcpy */
	strcpy(nstr, str);
	return nstr;
}



void nonblock(int fd)
{
	int arg;
	if ((arg = fcntl(fd, F_GETFL, 0)) < 0)
		fatal(_("error reading file descriptor flags: %s"),
		      strerror(errno));

	arg |= O_NONBLOCK;

	if (fcntl(fd, F_SETFL, arg) < 0)
		fatal(_("error setting flag O_NONBLOCK on file descriptor"),
		      strerror(errno));
}



#ifdef HAVE_STRTOL
int safe_atoi(const char *str)
{
	long int res;
	char *c;

	assert(str != NULL);

	errno = 0;
	res = strtol(str, &c, 10);
	if (errno == ERANGE || res > INT_MAX || res < INT_MIN)
		fatal(_("error parsing integer: out of range"));
	
	if (*c != '\0') fatal(_("error parsing integer from string"));

	return ((int)res);
}
#endif
