/*
 *  misc.h - miscellaneous funcions module - header
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
#ifndef MISC_H
#define MISC_H

#include "config.h"
#include <stdio.h>
#include <sys/types.h>

#undef  MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#undef  MIN
#define MIN(a,b) (((a)<(b))?(a):(b))

#undef  XOR
#define XOR(a,b) (((a)||(b)) && !((a)&&(b)))

typedef enum { FALSE = 0, TRUE = 1 } bool;

const char *get_program_name(void);
void fatal(const char *template, ...);
void warn(const char *template, ...);
void *xmalloc(size_t size);
char *xstrdup(const char* str);

void nonblock(int fd);

#ifdef HAVE_STRTOL
int safe_atoi(const char *str);
#else
#define safe_atoi atoi
#endif

#ifndef lint
#define RCSID(X) static const char rcsid[] = X
#else
#define RCSID(X)
#endif

#endif /* MISC_H */
