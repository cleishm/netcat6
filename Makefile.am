## Makefile.am -- modified version from Gary V. Vaughan's example in 
##                his autotools book (http://sources.redhat.com/autotools)

# Note:  The m4 files need to be included in EXTRA_DIST with a relative path,
# and a full path must be used in ACLOCAL_AMFLAGS, otherwise distcheck fails.
# In the former the files get copied into the build directory and not cleaned
# out again, in the latter the the m4 files are added as dependencies to
# aclocal.m4, but using relative paths which aren't available from within the
# build directory.  This is probably a bug in my version of automake (1.6.1).

auxdir			= @ac_aux_dir@
EXTRA_DIST		= bootstrap BUGS CREDITS \
			  config/ai_constants.m4 \
			  config/in6_guess_stack.m4 \
			  config/member_sin6_scope_id.m4 \
			  config/paths.m4 \
			  config/proto_inet6.m4 \
			  config/type_struct_sockaddr_storage.m4 \
			  config/type_struct_sockaddr_in6.m4 \
			  config/type_socklen_t.m4 \
			  config/somaxconn_constant.m4 \
			  config/proto_bluez.m4 \
			  config/type_struct_sockaddr_l2.m4 \
			  config/gettext.m4

AUTOMAKE_OPTIONS 	= 1.6 dist-bzip2
ACLOCAL_AMFLAGS		= -I $(top_srcdir)/config
SUBDIRS 		= contrib intl po src docs
MAINTAINERCLEANFILES 	= Makefile.in aclocal.m4 configure \
			  config.h.in config.status config.log \
			  $(auxdir)/depcomp $(auxdir)/install-sh \
			  $(auxdir)/missing $(auxdir)/mkinstalldirs
