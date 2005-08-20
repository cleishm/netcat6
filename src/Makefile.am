bin_PROGRAMS = nc6
noinst_HEADERS = \
  system.h \
  circ_buf.h \
  misc.h \
  network.h \
  afindep.h \
  bluez.h \
  netsupport.h \
  parser.h \
  readwrite.h \
  io_stream.h \
  connection.h

nc6_SOURCES = \
  circ_buf.c \
  main.c \
  misc.c \
  network.c \
  afindep.c \
  netsupport.c \
  parser.c \
  readwrite.c \
  io_stream.c \
  connection.c

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
