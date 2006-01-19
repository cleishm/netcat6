/*
 *  system.h - common header file for all modules
 * 
 *  nc6 - an advanced netcat clone
 *  Copyright (C) 2002-2006 Chris Leishman <chris _at_ leishman.org>
 *  Copyright (C) 2001-2006 Mauro Tortonesi <mauro _at_ deepspace6.net>
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
#ifndef SYSTEM_H
#define SYSTEM_H

/* include autoconf generated header */
#if HAVE_CONFIG_H
#include "config.h"
#endif

/* setup bool type */
#if HAVE_STDBOOL_H
# include <stdbool.h>
#else
# if ! HAVE__BOOL
#  ifdef __cplusplus
typedef bool _Bool;
#  else
typedef unsigned char _Bool;
#  endif
# endif
# define bool _Bool
# define false 0
# define true 1
# define __bool_true_false_are_defined 1
#endif


/* define basic helper macros */
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif
#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef XOR
#define XOR(a,b) (((a)||(b)) && !((a)&&(b)))
#endif


/* operations on timevals - copied from BSD sys/time.h */
#include <sys/time.h>
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
	(((tvp)->tv_sec < 0) || ((tvp)->tv_sec == 0 && (tvp)->tv_usec <= 0))


/* setup RCSID */
#ifndef lint
#define RCSID(X) static const char rcsid[] = X
#else
#define RCSID(X)
#endif


/* setup NLS */
#if ENABLE_NLS
#include <libintl.h>
#define _(String) gettext(String)
#define N_(String) (String)
#else
#define _(String) (String)
#define N_(String) (String)
#endif

#endif/*SYSTEM_H*/
