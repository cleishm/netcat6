/*
 *  misc.c - miscellaneous funcions module - implementation 
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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "config.h"
#include "misc.h"
#ifdef HAVE_STRTOL
#include <errno.h>
#include <limits.h>
#endif



/* exit the program with an error message */
void fatal(const char *template, ...)
{
	va_list ap;
						      
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
						      
	fprintf(stderr, "%s: ", get_program_name());
							       
	va_start(ap, template);
	vfprintf(stderr, template, ap);
	va_end(ap);

	fprintf(stderr, "\n");
}



/* same as `malloc' but report error if no memory available */
uint8_t *xmalloc(size_t size)
{
	register uint8_t *value = (uint8_t *)malloc(size);
	
	if (value == NULL) fatal("virtual memory exhausted");

	return value;
}



#ifdef HAVE_STRTOL
int safe_atoi(char *str)
{
	long int res;
	char *c;

	errno = 0;
	res = strtol(str, &c, 10);
	if (errno == ERANGE || res > INT_MAX || res < INT_MIN)
		fatal("error parsing integer: out of range");
	
	if (*c != '\0') fatal("error parsing integer from string");

	return ((int)res);
}
#endif
