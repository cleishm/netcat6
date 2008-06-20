/*
 *  misc.h - miscellaneous funcions module - header
 * 
 *  nc6 - an advanced netcat clone
 *  Copyright (C) 2001-2006 Mauro Tortonesi <mauro _at_ deepspace6.net>
 *  Copyright (C) 2002-2006 Chris Leishman <chris _at_ leishman.org>
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

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>

#ifdef ENABLE_BLUEZ
#include <bluetooth/bluetooth.h>
#endif

const char *get_program_name(void);

void fatal_internal(const char *template, ...);
void fatal(const char *template, ...);
void warning(const char *template, ...);

void *xmalloc(size_t size);
char *xstrdup(const char *str);

/* version of strlcpy that can handle a non-NULL terminated src  */
void strlcpy_trunc(char *dst, const char *src, size_t size);
/* bounded strlen */
size_t strnlen(const char *str, size_t maxlen);

void nonblock(int fd);

int open3(const char *cmd, int *in, int *out, int *err);

int safe_atoi(const char *str, int *result);

#ifdef ENABLE_BLUEZ
#define BA_MAXHOST 18
int safe_ba2str(const bdaddr_t *ba, char *str, size_t strlen);
#endif

#endif/*MISC_H*/
