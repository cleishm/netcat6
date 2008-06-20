bin_PROGRAMS = nc6
noinst_HEADERS = \
  system.h \
  parser.h \
  attributes.h \
  connection.h \
  readwrite.h \
  io_stream.h \
  circ_buf.h \
  netsupport.h \
  afindep.h \
  bluez.h \
  misc.h

nc6_SOURCES = \
  main.c \
  parser.c \
  attributes.c \
  connection.c \
  readwrite.c \
  io_stream.c \
  circ_buf.c \
  netsupport.c \
  afindep.c \
  misc.c

EXTRA_nc6_SOURCES = bluez.c

if BLUEZ
nc6_bluez = bluez.$(OBJEXT)
endif

localedir=$(datadir)/locale

nc6_LDADD = $(nc6_bluez) $(top_builddir)/contrib/libnc6contrib.a @LIBINTL@

# note: must use ../intl instead of absolute path
AM_CPPFLAGS = -I$(top_srcdir)/contrib -I../intl -DLOCALEDIR=\"$(localedir)\"
AM_CFLAGS = @CFLAGS@ @NC6_CFLAGS@

MAINTAINERCLEANFILES 	= Makefile.in stamp-h.in
