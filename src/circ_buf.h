#ifndef CIRC_BUF_H
#define CIRC_BUF_H

#include <stdint.h>
#include "misc.h"

typedef struct circ_buf_t {
	uint8_t *buf;  /* pointer to the buffer */
	uint8_t *ptr;  /* pointer to the beginning of written data */
	int data_size; /* number of bytes that have been written 
			* into the buffer */
	int buf_size;  /* size of the buffer */
} circ_buf;

bool is_empty(const circ_buf *cb);
void check_cb(const circ_buf *cb);
int read_to_cb(int fd, circ_buf *cb);
int write_from_cb(int fd, circ_buf *cb);
circ_buf *alloc_cb(size_t size);

#endif /* CIRC_BUF_H */
