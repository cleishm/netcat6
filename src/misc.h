#ifndef MISC_H
#define MISC_H

#include "config.h"
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include <sys/types.h>
#include <stdio.h>


typedef enum { FALSE = 0, TRUE = 1 } bool;

void fatal(const char *template, ...);
void warn(const char *template, ...);
uint8_t *xmalloc(size_t size);
void print_usage(FILE *fp);

#ifdef HAVE_STRTOL
int safe_atoi(char *str);
#else
#define safe_atoi atoi
#endif

#endif /* MISC_H */
