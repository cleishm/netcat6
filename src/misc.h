/*
 *  misc.h - miscellaneous funcions module - header
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
#ifndef MISC_H
#define MISC_H

#include "config.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>

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

int open3(char *cmd, int *in, int *out, int *err);

#ifdef HAVE_STRTOL
int safe_atoi(const char *str);
#else
#define safe_atoi atoi
#endif

/* operations on timevals - copied from BSD sys/time.h */
#ifndef timerclear
#define	timerclear(tvp)		(tvp)->tv_sec = (tvp)->tv_usec = 0
#endif
#ifndef timerisset
#define	timerisset(tvp)		((tvp)->tv_sec || (tvp)->tv_usec)
#endif
#ifndef timeradd
#define timeradd(tvp, uvp, vvp)						\
	do {								\
		(vvp)->tv_sec = (tvp)->tv_sec + (uvp)->tv_sec;		\
		(vvp)->tv_usec = (tvp)->tv_usec + (uvp)->tv_usec;	\
		if ((vvp)->tv_usec >= 1000000) {			\
			(vvp)->tv_sec++;				\
			(vvp)->tv_usec -= 1000000;			\
		}							\
	} while (0)
#endif
#ifndef timersub
#define	timersub(tvp, uvp, vvp)						\
	do {								\
		(vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;		\
		(vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;	\
		if ((vvp)->tv_usec < 0) {				\
			(vvp)->tv_sec--;				\
			(vvp)->tv_usec += 1000000;			\
		}							\
	} while (0)
#endif

#define	istimerexpired(tvp)		\
	(((tvp)->tv_sec || (tvp)->tv_usec) ? FALSE : TRUE)

#ifndef lint
#define RCSID(X) static const char rcsid[] = X
#else
#define RCSID(X)
#endif

#endif /* MISC_H */
