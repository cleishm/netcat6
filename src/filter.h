#ifndef FILTER_H
#define FILTER_H

#include <sys/socket.h>
#include "misc.h"
#include "network.h"

#if 0
typedef struct filter_t {
	struct filter_t *next;
	char *address;
	char *port;
} *filter, *iterator;

#define EMPTY_FILTER NULL;
void add_to_filter(const address *a, filter *f);
#endif

bool is_allowed(const struct sockaddr *sa, const address *addr, unsigned int flags);

#endif /* FILTER_H */
