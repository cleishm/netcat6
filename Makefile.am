## Makefile.am -- modified version from Gary V. Vaughan's example in 
##                his autotools book (http://sources.redhat.com/autotools)

# Note:  The m4 files need to be included in EXTRA_DIST with a relative path,
# and a full path must be used in ACLOCAL_AMFLAGS, otherwise distcheck fails.
# In the former the files get copied into the build directory and not cleaned
# out again, in the latter the the m4 files are added as dependencies to
# aclocal.m4, but using relative paths which aren't available from within the
# build directory.  This is probably a bug in my version of automake (1.6.1).

auxdir			= @ac_aux_dir@
EXTRA_DIST		= bootstrap BUGS CREDITS
AUTOMAKE_OPTIONS 	= 1.6 dist-bzip2
ACLOCAL_AMFLAGS		= -I config
SUBDIRS 		= config contrib intl po src docs
MAINTAINERCLEANFILES 	= Makefile.in aclocal.m4 configure \
			  config.h.in config.status config.log 
