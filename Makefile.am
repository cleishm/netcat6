## Makefile.am -- modified version from Gary V. Vaughan's example in 
##                his autotools book (http://sources.redhat.com/autotools)

auxdir			= @ac_aux_dir@
AUX_DIST		= $(auxdir)/install-sh \
			  $(auxdir)/missing \
			  $(auxdir)/mkinstalldirs \
			  $(auxdir)/depcomp			  
AUX_DIST_EXTRA		= $(auxdir)/in6_00_header.m4 \
		          $(auxdir)/in6_enable_ipv6.m4 \
		          $(auxdir)/in6_func_getaddrinfo.m4 \
		          $(auxdir)/in6_func_salen.m4 \
		          $(auxdir)/in6_guess_stack.m4 \
		          $(auxdir)/in6_struct_sockaddr.m4 \
		          $(auxdir)/in6_struct_sockaddrin6.m4 \
		          $(auxdir)/in6x_00_header.m4 \
		          $(auxdir)/in6x_func_ifaddrs.m4 \
		          $(auxdir)/in6x_struct_icmp6hdr.m4 \
		          $(auxdir)/in6x_struct_ip6hdr.m4 \
		          $(auxdir)/socklen_t.m4 
EXTRA_DIST		= bootstrap TODO COPYING CREDITS nc6.spec

AUTOMAKE_OPTIONS 	= foreign dist-bzip2
SUBDIRS 		= docs src #config 
MAINTAINERCLEANFILES 	= Makefile.in aclocal.m4 configure src/config-h.in \
			  src/stamp-h.in $(AUX_DIST)

ACLOCAL			= aclocal -I $(auxdir)

dist-hook:
	cd $(distdir)
	for file in $(AUX_DIST) $(AUX_DIST_EXTRA); do \
	  cp $$file $(distdir)/$$file; \
	done
