#ifndef READWRITE_H
#define READWRITE_H

#include "misc.h"

typedef struct io_stream_t
{
	int  fd_in;
	int  fd_out;
	bool is_tcp_socket;
} io_stream;

void readwrite(io_stream *ios1, io_stream *ios2);
void stdio_to_io_stream(io_stream *ios);
void socket_to_io_stream(int fd, io_stream *ios);

#endif /* READWRITE_H */
