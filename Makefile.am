## Makefile.am -- modified version from Gary V. Vaughan's example in 
##                his autotools book (http://sources.redhat.com/autotools)

# Note:  The m4 files need to be included in EXTRA_DIST with a relative path,
# and a full path must be used in ACLOCAL_AMFLAGS, otherwise distcheck fails.
# In the former the files get copied into the build directory and not cleaned
# out again, in the latter the the m4 files are added as dependencies to
# aclocal.m4, but using relative paths which aren't available from within the
# build directory.  This is probably a bug in my version of automake (1.6.1).

auxdir			= @ac_aux_dir@
EXTRA_DIST		= ABOUT-NLS bootstrap BUGS CREDITS
AUTOMAKE_OPTIONS 	= 1.6 dist-bzip2
ACLOCAL_AMFLAGS		= -I config
SUBDIRS 		= config intl contrib src docs po

## remove gettext macros here, as aclocal.m4 depends on them
MAINTAINERCLEANFILES 	= ABOUT-NLS Makefile.in aclocal.m4 configure \
			  config.h.in config.status config.log \
                          config/codeset.m4 config/gettext.m4 \
                          config/glibc21.m4 config/iconv.m4 config/intdiv0.m4 \
                          config/intmax.m4 config/inttypes-pri.m4 \
                          config/inttypes.m4 config/inttypes_h.m4 \
                          config/isc-posix.m4 config/lcmessage.m4 \
                          config/lib-ld.m4 config/lib-link.m4 \
                          config/lib-prefix.m4 config/longdouble.m4 \
                          config/longlong.m4 config/nls.m4 config/po.m4 \
                          config/printf-posix.m4 config/progtest.m4 \
                          config/signed.m4 config/size_max.m4 \
                          config/stdint_h.m4 config/uintmax_t.m4 \
                          config/ulonglong.m4 config/wchar_t.m4 \
                          config/wint_t.m4 config/xsize.m4 \
                          po/Makefile.in.in po/Makevars.template \
                          po/Rules-quot po/boldquot.sed po/en@boldquot.header \
                          po/en@quot.header po/insert-header.sin po/quot.sed \
                          po/remove-potcdate.sin po/nc6.pot

maintainer-clean-local:
	rm -rf intl
