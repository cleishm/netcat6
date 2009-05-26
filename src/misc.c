/*
 *  misc.c - miscellaneous funcions module - implementation 
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
#include "system.h"
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
#include <unistd.h>
#ifdef HAVE_PATHS_H
#include <paths.h>
#endif

#ifdef ENABLE_BLUEZ
#ifdef HAVE_STDINT_H
#include <stdint.h> /* needed for uint8_t */
#endif
#include <bluetooth/bluetooth.h>
#endif


/* verbosity global flag */
int _verbosity_level = 0;



/* abort with an internal error message */
void fatal_internal(const char *template, ...)
{
	va_list ap;

	assert(template != NULL);

	fprintf(stderr, "%s: internal error: ", get_program_name());

	va_start(ap, template);
	vfprintf(stderr, template, ap);
	va_end(ap);

	fprintf(stderr, " (please report this error to nc6 authors)\n");
	
	abort();
}



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



void warning(const char *template, ...)
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
	register char *nstr = (char *)xmalloc(strlen(str)+1);
	strcpy(nstr, str);
	return nstr;
}



/* version of strlcpy that can handle a non-NULL terminated src  */
void strlcpy_trunc(char *dst, const char *src, size_t size)
{
	register size_t len = strnlen(src, size-1);
	memcpy(dst, src, len);
	dst[len] = '\0';
}



/* bounded strlen */
size_t strnlen(const char *str, size_t maxlen)
{
	register size_t len;
	/* bounded strlen */
	for (len = 0; len < maxlen && str[len] != '\0'; ++len)
		/* no body */;
	return len;
}



const char *non_empty_string(const char *str)
{
	return (str == NULL || strlen(str) == 0)? NULL : str;
}


void nonblock(int fd)
{
	int arg;
	if ((arg = fcntl(fd, F_GETFL, 0)) < 0)
		fatal("error reading file descriptor flags: %s",
		      strerror(errno));

	arg |= O_NONBLOCK;

	if (fcntl(fd, F_SETFL, arg) < 0)
		fatal("error setting flag O_NONBLOCK on file descriptor",
		      strerror(errno));
}



int open3(const char *cmd, int *in, int *out, int *err)
{
	int inpipe[2];
	int outpipe[2];
	int errpipe[2];
	int pid;

	if (in != NULL) {
		if (pipe(inpipe) < 0)
			return -1;
	} else {
		inpipe[0] = open(_PATH_DEVNULL, O_RDONLY);
		if (inpipe[0] < 0)
			return -1;
	}

	if (out != NULL) {
		if (pipe(outpipe) < 0)
			return -1;
	} else {
		outpipe[1] = open(_PATH_DEVNULL, O_WRONLY);
		if (outpipe[1] < 0)
			return -1;
	}

	if (err != NULL) {
		if (pipe(errpipe) < 0)
			return -1;
	} else {
		errpipe[1] = open(_PATH_DEVNULL, O_WRONLY);
		if (errpipe[1] < 0)
			return -1;
	}

	/* fork the process */
	pid = fork();
	if (pid < 0) {
		return -1;
	} else if (pid == 0) {
		char *argv[4];

		/* child */

		/* close parents descriptors */
		close(inpipe[1]);
		close(outpipe[0]);
		close(errpipe[0]);

		argv[0] = "sh";
		argv[1] = "-c";
		argv[2] = xstrdup(cmd); /* have to strdup to remove const */
		argv[3] = NULL;

		/* replace stdin, stdout and stderr */
		close(STDIN_FILENO);
		if (dup2(inpipe[0], STDIN_FILENO) < 0)
			fatal("dup2 failed: %s", strerror(errno));
		close(inpipe[0]);

		close(STDOUT_FILENO);
		if (dup2(outpipe[1], STDOUT_FILENO) < 0)
			fatal("dup2 failed: %s", strerror(errno));
		close(outpipe[1]);

		close(STDERR_FILENO);
		if (dup2(errpipe[1], STDERR_FILENO) < 0)
			fatal("dup2 failed: %s", strerror(errno));
		close(errpipe[1]);

		/* exec the required command */
		execv(_PATH_BSHELL, argv);
		fatal("execv failed: %s", strerror(errno));
	}

	/* parent */

	/* close childs descriptors */
	close(inpipe[0]);
	close(outpipe[1]);
	close(errpipe[1]);

	if (in != NULL)  *in  = inpipe[1];
	if (out != NULL) *out = outpipe[0];
	if (err != NULL) *err = errpipe[0];

	return pid;
}



int safe_atoi(const char *str, int *result)
{
	long int lresult;
	char *endptr;

	assert(str != NULL);
	assert(result != NULL);

#ifdef HAVE_STRTOL
	errno = 0;
	lresult = strtol(str, &endptr, 10);
	if (*endptr != '\0') {
		errno = EINVAL;
		return -1;
	}
	if (errno == ERANGE || lresult > INT_MAX || lresult < INT_MIN) {
		errno = ERANGE;
		return -1;
	}
#else
	for (endptr = str; *endptr != '\0'; ++endptr) {
		if (!isdigit(*endptr)) {
			errno = EINVAL;
			return -1;
		}
	}
	lresult = atoi(str);
#endif
	
	*result = lresult;
	return 0;
}



#ifdef ENABLE_BLUEZ
int safe_ba2str(const bdaddr_t *ba, char *str, size_t strlen)
{
	uint8_t b[6];

	baswap((bdaddr_t *)b, ba);
	return snprintf(str, strlen, "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",
			b[0], b[1], b[2], b[3], b[4], b[5]);
}
#endif
