## Makefile.am -- modified version from Gary V. Vaughan's example in 
##                his autotools book (http://sources.redhat.com/autotools)

auxdir			= @ac_aux_dir@
AUX_DIST		= $(auxdir)/install-sh \
			  $(auxdir)/missing \
			  $(auxdir)/mkinstalldirs \
			  $(auxdir)/depcomp			  
AUX_DIST_EXTRA		= $(auxdir)/ai_constants.m4 \
		          $(auxdir)/in6_guess_stack.m4 \
			  $(auxdir)/proto_inet6.m4 \
		          $(auxdir)/type_struct_sockaddr_storage.m4 \
		          $(auxdir)/type_struct_sockaddr_in6.m4 \
		          $(auxdir)/type_socklen_t.m4 
EXTRA_DIST		= bootstrap TODO COPYING CREDITS nc6.spec

AUTOMAKE_OPTIONS 	= foreign dist-bzip2
ACLOCAL_AMFLAGS		= -I $(auxdir)
SUBDIRS 		= docs src #config 
MAINTAINERCLEANFILES 	= Makefile.in aclocal.m4 configure \
			  config.status config.log $(AUX_DIST)

dist-hook:
	cd $(distdir)
	for file in $(AUX_DIST) $(AUX_DIST_EXTRA); do \
	  cp $$file $(distdir)/$$file; \
	done
