bin_PROGRAMS = nc6
noinst_HEADERS = \
  circ_buf.h \
  filter.h \
  misc.h \
  network.h \
  netsupport.h \
  parser.h \
  readwrite.h \
  io_stream.h \
  connection.h

nc6_SOURCES = \
  circ_buf.c \
  filter.c \
  main.c \
  misc.c \
  network.c \
  netsupport.c \
  parser.c \
  readwrite.c \
  io_stream.c \
  connection.c

nc6_LDADD = ../contrib/libnc6contrib.a

CPPFLAGS = -I../contrib
CFLAGS = @CFLAGS@ @NC6_CFLAGS@

MAINTAINERCLEANFILES 	= Makefile.in config.h.in stamp-h.in
