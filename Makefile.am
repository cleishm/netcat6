## Makefile.am -- modified version from Gary V. Vaughan's example in 
##                his autotools book (http://sources.redhat.com/autotools)

auxdir			= @ac_aux_dir@
EXTRA_DIST		= bootstrap BUGS CREDITS \
			  $(auxdir)/ai_constants.m4 \
			  $(auxdir)/in6_guess_stack.m4 \
			  $(auxdir)/proto_inet6.m4 \
			  $(auxdir)/type_struct_sockaddr_storage.m4 \
			  $(auxdir)/type_struct_sockaddr_in6.m4 \
			  $(auxdir)/type_socklen_t.m4 

AUTOMAKE_OPTIONS 	= dist-bzip2
ACLOCAL_AMFLAGS		= -I $(auxdir)
SUBDIRS 		= docs contrib src
MAINTAINERCLEANFILES 	= Makefile.in aclocal.m4 configure \
			  config.h.in config.status config.log \
			  $(auxdir)/depcomp $(auxdir)/install-sh \
			  $(auxdir)/missing $(auxdir)/mkinstalldirs
