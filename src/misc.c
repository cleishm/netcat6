#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "config.h"
#include "misc.h"
#ifdef HAVE_STRTOL
#include <errno.h>
#include <limits.h>
#endif

extern const char *get_program_name();


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



void print_usage(FILE *fp)
{
	const char *program_name = get_program_name();

	fprintf(fp, "\nUsage:\n"
	       "\t%s [-46nh] [-p port] [-s addr] hostname port\n"
	       "\t%s -l -p port [-s addr] [-46nh] [hostname] [port]\n\n"
	       "Recognized options are:\n"
	       "    -4         Use only IPv4\n"
	       "    -6         Use only IPv6\n"
	       "    -l         Listen mode, for inbound connects\n"
	       "    -s addr    Local source address\n"
	       "    -p port    Local source port\n"
	       "    -n         Numeric-only IP addresses, no DNS\n" 
	       "    -h         Display help\n\n", 
	       program_name, program_name);
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
