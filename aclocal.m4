dnl aclocal.m4 generated automatically by aclocal 1.4-p5

dnl Copyright (C) 1994, 1995-8, 1999, 2001 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY, to the extent permitted by law; without
dnl even the implied warranty of MERCHANTABILITY or FITNESS FOR A
dnl PARTICULAR PURPOSE.

# Like AC_CONFIG_HEADER, but automatically create stamp file.

AC_DEFUN([AM_CONFIG_HEADER],
[AC_PREREQ([2.12])
AC_CONFIG_HEADER([$1])
dnl When config.status generates a header, we must update the stamp-h file.
dnl This file resides in the same directory as the config header
dnl that is generated.  We must strip everything past the first ":",
dnl and everything past the last "/".
AC_OUTPUT_COMMANDS(changequote(<<,>>)dnl
ifelse(patsubst(<<$1>>, <<[^ ]>>, <<>>), <<>>,
<<test -z "<<$>>CONFIG_HEADERS" || echo timestamp > patsubst(<<$1>>, <<^\([^:]*/\)?.*>>, <<\1>>)stamp-h<<>>dnl>>,
<<am_indx=1
for am_file in <<$1>>; do
  case " <<$>>CONFIG_HEADERS " in
  *" <<$>>am_file "*<<)>>
    echo timestamp > `echo <<$>>am_file | sed -e 's%:.*%%' -e 's%[^/]*$%%'`stamp-h$am_indx
    ;;
  esac
  am_indx=`expr "<<$>>am_indx" + 1`
done<<>>dnl>>)
changequote([,]))])

# Do all the work for Automake.  This macro actually does too much --
# some checks are only needed if your package does certain things.
# But this isn't really a big deal.

# serial 1

dnl Usage:
dnl AM_INIT_AUTOMAKE(package,version, [no-define])

AC_DEFUN([AM_INIT_AUTOMAKE],
[AC_REQUIRE([AC_PROG_INSTALL])
PACKAGE=[$1]
AC_SUBST(PACKAGE)
VERSION=[$2]
AC_SUBST(VERSION)
dnl test to see if srcdir already configured
if test "`cd $srcdir && pwd`" != "`pwd`" && test -f $srcdir/config.status; then
  AC_MSG_ERROR([source directory already configured; run "make distclean" there first])
fi
ifelse([$3],,
AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE", [Name of package])
AC_DEFINE_UNQUOTED(VERSION, "$VERSION", [Version number of package]))
AC_REQUIRE([AM_SANITY_CHECK])
AC_REQUIRE([AC_ARG_PROGRAM])
dnl FIXME This is truly gross.
missing_dir=`cd $ac_aux_dir && pwd`
AM_MISSING_PROG(ACLOCAL, aclocal, $missing_dir)
AM_MISSING_PROG(AUTOCONF, autoconf, $missing_dir)
AM_MISSING_PROG(AUTOMAKE, automake, $missing_dir)
AM_MISSING_PROG(AUTOHEADER, autoheader, $missing_dir)
AM_MISSING_PROG(MAKEINFO, makeinfo, $missing_dir)
AC_REQUIRE([AC_PROG_MAKE_SET])])

#
# Check to make sure that the build environment is sane.
#

AC_DEFUN([AM_SANITY_CHECK],
[AC_MSG_CHECKING([whether build environment is sane])
# Just in case
sleep 1
echo timestamp > conftestfile
# Do `set' in a subshell so we don't clobber the current shell's
# arguments.  Must try -L first in case configure is actually a
# symlink; some systems play weird games with the mod time of symlinks
# (eg FreeBSD returns the mod time of the symlink's containing
# directory).
if (
   set X `ls -Lt $srcdir/configure conftestfile 2> /dev/null`
   if test "[$]*" = "X"; then
      # -L didn't work.
      set X `ls -t $srcdir/configure conftestfile`
   fi
   if test "[$]*" != "X $srcdir/configure conftestfile" \
      && test "[$]*" != "X conftestfile $srcdir/configure"; then

      # If neither matched, then we have a broken ls.  This can happen
      # if, for instance, CONFIG_SHELL is bash and it inherits a
      # broken ls alias from the environment.  This has actually
      # happened.  Such a system could not be considered "sane".
      AC_MSG_ERROR([ls -t appears to fail.  Make sure there is not a broken
alias in your environment])
   fi

   test "[$]2" = conftestfile
   )
then
   # Ok.
   :
else
   AC_MSG_ERROR([newly created file is older than distributed files!
Check your system clock])
fi
rm -f conftest*
AC_MSG_RESULT(yes)])

dnl AM_MISSING_PROG(NAME, PROGRAM, DIRECTORY)
dnl The program must properly implement --version.
AC_DEFUN([AM_MISSING_PROG],
[AC_MSG_CHECKING(for working $2)
# Run test in a subshell; some versions of sh will print an error if
# an executable is not found, even if stderr is redirected.
# Redirect stdin to placate older versions of autoconf.  Sigh.
if ($2 --version) < /dev/null > /dev/null 2>&1; then
   $1=$2
   AC_MSG_RESULT(found)
else
   $1="$3/missing $2"
   AC_MSG_RESULT(missing)
fi
AC_SUBST($1)])

#serial 1
# This test replaces the one in autoconf.
# Currently this macro should have the same name as the autoconf macro
# because gettext's gettext.m4 (distributed in the automake package)
# still uses it.  Otherwise, the use in gettext.m4 makes autoheader
# give these diagnostics:
#   configure.in:556: AC_TRY_COMPILE was called before AC_ISC_POSIX
#   configure.in:556: AC_TRY_RUN was called before AC_ISC_POSIX

undefine([AC_ISC_POSIX])

AC_DEFUN([AC_ISC_POSIX],
  [
    dnl This test replaces the obsolescent AC_ISC_POSIX kludge.
    AC_CHECK_LIB(cposix, strerror, [LIBS="$LIBS -lcposix"])
  ]
)

dnl ==========================================================================
dnl Copyright (C) 2000 USAGI/WIDE Project.
dnl All rights reserved.
dnl 
dnl Redistribution and use in source and binary forms, with or without
dnl modification, are permitted provided that the following conditions
dnl are met:
dnl 1. Redistributions of source code must retain the above copyright
dnl    notice, this list of conditions and the following disclaimer.
dnl 2. Redistributions in binary form must reproduce the above copyright
dnl    notice, this list of conditions and the following disclaimer in the
dnl    documentation and/or other materials provided with the distribution.
dnl 3. Neither the name of the project nor the names of its contributors
dnl    may be used to endorse or promote products derived from this software
dnl    without specific prior written permission.
dnl 
dnl THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
dnl ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
dnl IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
dnl ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
dnl FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
dnl DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
dnl OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
dnl HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
dnl LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
dnl OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
dnl SUCH DAMAGE.
dnl ==========================================================================
dnl // $USAGI: in6_struct_sockaddr.m4,v 1.5 2000/10/30 15:19:46 yoshfuji Exp $
dnl =================================================
dnl IN6_STRUCT_SOCKADDR_SA_LEN(action-ok[,action-ng])
dnl =================================================
AC_DEFUN(IN6_STRUCT_SOCKADDR_SA_LEN,[
AC_MSG_CHECKING([whether struct sockaddr has sa_len])
AC_CACHE_VAL(in6_cv_struct_sockaddr_sa_len, [
AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
],[
struct sockaddr sa;
int i = sa.sa_len;
],[	in6_cv_struct_sockaddr_sa_len=yes
],[	in6_cv_struct_sockaddr_sa_len=no
])])
if test "$in6_cv_struct_sockaddr_sa_len" = "yes"; then
	AC_MSG_RESULT([yes])
	$1
else
	AC_MSG_RESULT([no])
	$2
fi
])

dnl ==================================================
dnl IN6_STRUCT_SOCKADDR_STORAGE(action-ok[,action-ng])
dnl ==================================================
AC_DEFUN(IN6_STRUCT_SOCKADDR_STORAGE,[
AC_MSG_CHECKING([whether sys/socket.h has struct sockaddr_storage])
AC_CACHE_VAL(in6_cv_struct_sockaddr_storage, [
AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
],[
struct sockaddr_storage s;
],[	in6_cv_struct_sockaddr_storage=yes
],[	in6_cv_struct_sockaddr_storage=no
])])
if test "$in6_cv_struct_sockaddr_storage" = "yes"; then
	AC_MSG_RESULT([yes])
	$1
else
	AC_MSG_RESULT([no])
	$2
fi
])

dnl =========================================================================
dnl IN6_STRUCT_SOCKADDR_STORAGE_SS_FAMILY(action-ok[,action-old[,action-no]])
dnl =========================================================================
AC_DEFUN(IN6_STRUCT_SOCKADDR_STORAGE_SS_FAMILY,[
AC_MSG_CHECKING([whether struct sockaddr_storage has ss_family])
AC_CACHE_VAL(in6_cv_struct_sockaddr_storage_ss_family, [
AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
],[
struct sockaddr_storage ss;
int i = ss.ss_family;
],[	in6_cv_struct_sockaddr_storage_ss_family=yes
],[	AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
],[
struct sockaddr_storage ss;
int i = ss.__ss_family;
],[	in6_cv_struct_sockaddr_storage_ss_family=old
],[	in6_cv_struct_sockaddr_storage_ss_family=no
])
])])
if test "$in6_cv_struct_sockaddr_storage_ss_family" = "yes"; then
	AC_MSG_RESULT([yes])
	$1
elif test "$in6_cv_struct_sockaddr_storage_ss_family" = "old"; then
	AC_MSG_RESULT([__ss_family])
	$2
else
	AC_MSG_RESULT([no])
	$3
fi
])

dnl ==============================================================
dnl IN6_STRUCT_SOCKADDR_STORAGE___SS_FAMILY(action-ok[,action-ng])
dnl ==============================================================
AC_DEFUN(IN6_STRUCT_SOCKADDR_STORAGE___SS_FAMILY,[
AC_MSG_CHECKING([whether struct sockaddr_storage has __ss_family])
AC_CACHE_VAL(in6_cv_struct_sockaddr_storage___ss_family, [
AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
],[
struct sockaddr_storage ss;
int i = ss.__ss_family;
],[	in6_cv_struct_sockaddr_storage___ss_family=yes
],[	in6_cv_struct_sockaddr_storage___ss_family=no
])])
if test "$in6_cv_struct_sockaddr_storage___ss_family" = "yes"; then
	AC_MSG_RESULT([yes])
	$1
else
	AC_MSG_RESULT([no])
	$2
fi
])


dnl ==========================================================================
dnl Copyright (C) 2000 USAGI/WIDE Project.
dnl All rights reserved.
dnl 
dnl Redistribution and use in source and binary forms, with or without
dnl modification, are permitted provided that the following conditions
dnl are met:
dnl 1. Redistributions of source code must retain the above copyright
dnl    notice, this list of conditions and the following disclaimer.
dnl 2. Redistributions in binary form must reproduce the above copyright
dnl    notice, this list of conditions and the following disclaimer in the
dnl    documentation and/or other materials provided with the distribution.
dnl 3. Neither the name of the project nor the names of its contributors
dnl    may be used to endorse or promote products derived from this software
dnl    without specific prior written permission.
dnl 
dnl THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
dnl ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
dnl IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
dnl ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
dnl FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
dnl DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
dnl OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
dnl HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
dnl LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
dnl OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
dnl SUCH DAMAGE.
dnl ==========================================================================
dnl // $USAGI: in6_struct_sockaddrin6.m4,v 1.6 2000/10/30 15:19:46 yoshfuji Exp $
dnl ==============================================
dnl IN6_STRUCT_SOCKADDR_IN6(action-ok[,action-ng])
dnl ==============================================
AC_DEFUN(IN6_STRUCT_SOCKADDR_IN6,[
AC_MSG_CHECKING([whether netinet/in.h has struct sockaddr_in6])
AC_CACHE_VAL(in6_cv_struct_sockaddr_in6, [
AC_TRY_COMPILE([
#include <sys/types.h>
#include <netinet/in.h>
],[
struct sockaddr_in6 sin6;
],[	in6_cv_struct_sockaddr_in6=yes
],[	in6_cv_struct_sockaddr_in6=no
])])
if test "$in6_cv_struct_sockaddr_in6" = "yes"; then
	AC_MSG_RESULT([yes])
	$1
else
	AC_MSG_RESULT([no])
	$2
fi
])

dnl ============================================================
dnl IN6_STRUCT_SOCKADDR_IN6_SIN6_SCOPE_ID(action-ok[,action-ng])
dnl ============================================================
AC_DEFUN(IN6_STRUCT_SOCKADDR_IN6_SIN6_SCOPE_ID,[
AC_MSG_CHECKING([whether struct sockaddr_in6 has sin6_scope_id])
AC_CACHE_VAL(in6_cv_struct_sockaddr_in6_sin6_scope_id, [
AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
],[	struct sockaddr_in6 sin6;
	int i = sin6.sin6_scope_id;
],[	in6_cv_struct_sockaddr_in6_sin6_scope_id=yes
],[	in6_cv_struct_sockaddr_in6_sin6_scope_id=no
])])
if test "$in6_cv_struct_sockaddr_in6" = "yes"; then
	AC_MSG_RESULT([yes])
	$1
else
	AC_MSG_RESULT([no])
	$2
fi
])


dnl ==========================================================================
dnl Copyright (C) 2000 USAGI/WIDE Project.
dnl All rights reserved.
dnl 
dnl Redistribution and use in source and binary forms, with or without
dnl modification, are permitted provided that the following conditions
dnl are met:
dnl 1. Redistributions of source code must retain the above copyright
dnl    notice, this list of conditions and the following disclaimer.
dnl 2. Redistributions in binary form must reproduce the above copyright
dnl    notice, this list of conditions and the following disclaimer in the
dnl    documentation and/or other materials provided with the distribution.
dnl 3. Neither the name of the project nor the names of its contributors
dnl    may be used to endorse or promote products derived from this software
dnl    without specific prior written permission.
dnl 
dnl THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
dnl ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
dnl IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
dnl ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
dnl FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
dnl DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
dnl OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
dnl HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
dnl LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
dnl OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
dnl SUCH DAMAGE.
dnl ==========================================================================
dnl // $USAGI: in6_func_getaddrinfo.m4,v 1.6 2000/10/30 15:19:46 yoshfuji Exp $
dnl ================================================================================
dnl IN6_FUNC_GETADDRINFO(action-if-good[,action-if-bad[,action-if-cross-compiling]])
dnl ================================================================================
AC_DEFUN(IN6_FUNC_GETADDRINFO,[
AC_MSG_CHECKING(whether you have good getaddrinfo)
AC_CACHE_VAL(in6_cv_func_getaddrinfo, [
AC_TRY_RUN([
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>

#define SERVSTR "54321"

#define IPV4_LOOPBACK "127.0.0.1"
#define IPV4_PASSIVE "0.0.0.0"
#define IPV6_LOOPBACK "::1"
#define IPV6_PASSIVE "::"

main(){
  int passive, gaierr, inet4 = 0, inet6 = 0;
  struct addrinfo hints, *ai, *aitop;
  char straddr[INET6_ADDRSTRLEN], strport[16];

  for (passive = 0; passive <= 1; passive++) {
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = passive ? AI_PASSIVE : 0;
    hints.ai_socktype = SOCK_STREAM;
    if ((gaierr = getaddrinfo(NULL, SERVSTR, &hints, &aitop)) != 0) {
      (void)gai_strerror(gaierr);
      goto bad;
    }
    for (ai = aitop; ai; ai = ai->ai_next) {
      if (ai->ai_addr == NULL ||
          ai->ai_addrlen == 0 ||
          getnameinfo(ai->ai_addr, ai->ai_addrlen,
                      straddr, sizeof(straddr), strport, sizeof(strport),
                      NI_NUMERICHOST|NI_NUMERICSERV) != 0) {
        goto bad;
      }
      switch (ai->ai_family) {
      case AF_INET:
        if (strcmp(strport, SERVSTR) != 0) {
	  goto bad;
	}
        if (passive) {
          if (strcmp(straddr, IPV4_PASSIVE) != 0) {
            goto bad;
          }
        } else {
          if (strcmp(straddr, IPV4_LOOPBACK) != 0) {
            goto bad;
          }
        }
        inet4++;
        break;
#if defined(INET6) && defined(AF_INET6)
      case AF_INET6:
        if (strcmp(strport, SERVSTR) != 0) {
	  goto bad;
	}
        if (passive) {
          if (strcmp(straddr, IPV6_PASSIVE) != 0) {
            goto bad;
          }
        } else {
          if (strcmp(straddr, IPV6_LOOPBACK) != 0) {
            goto bad;
          }
        }
        inet6++;
        break;
#endif
      case AF_UNSPEC:
        goto bad;
        break;
      default:
        /* another family support? */
        break;
      }
    }
  }

  if (inet6 != 2 && inet6 != 0)
    goto bad;
  if (inet4 != 2 && inet4 != 0)
    goto bad;

  if (aitop)
    freeaddrinfo(aitop);
  exit(0);

 bad:
  if (aitop)
    freeaddrinfo(aitop);
  exit(1);
}
],[	in6_cv_func_getaddrinfo=yes
],[	in6_cv_func_getaddrinfo=buggy
],[	in6_cv_func_getaddrinfo=no
])])
if test "$in6_cv_func_getaddrinfo" = "yes"; then
	AC_MSG_RESULT([yes])
	$2
elif test "$in6_cv_func_getaddrinfo" = "buggy"; then
	AC_MSG_RESULT([buggy])
	$3
else
	AC_MSG_RESULT([no])
fi
])

dnl =============================================================================
dnl IN6_FUNC_GETADDRINFO_SOCK_RAW(action-ok[,action-ng[,action-cross-compiling]])
dnl =============================================================================
AC_DEFUN(IN6_FUNC_GETADDRINFO_SOCK_RAW,[
AC_MSG_CHECKING([whether you can use getaddrinfo() for raw sockets])
AC_CACHE_VAL(in6_cv_func_getaddrinfo_sock_raw, [
AC_TRY_RUN([
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
int main(){
	struct addrinfo hints, *res0;
	int ret = 0;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_INET6;
	hints.ai_socktype = SOCK_RAW;
	hints.ai_protocol = IPPROTO_ICMPV6;
	if (getaddrinfo("::1", NULL, &hints, &res0)){
		ret = 1;
	}
	else{
		if (res0->ai_family != PF_INET6 ||
		    res0->ai_socktype != SOCK_RAW ||
		    res0->ai_protocol != IPPROTO_ICMPV6)
			ret = 1;
		freeaddrinfo(res0);
	}
	return ret;
}
],[	in6_cv_func_getaddrinfo_sock_raw=yes
],[	in6_cv_func_getaddrinfo_sock_raw=no
],[	in6_cv_func_getaddrinfo_sock_raw=unknown
])])
if test "$in6_cv_func_getaddrinfo_sock_raw" = "yes"; then
	AC_MSG_RESULT([yes])
	$2
elif test "$in6_cv_func_getaddrinfo_sock_raw" = "no"; then
	AC_MSG_RESULT([no])
	$3
else
	AC_MSG_RESULT([unknown])
	$5
fi
])

dnl ==================================================================================
dnl IN6_FUNC_GETADDRINFO_AI_ADDRCONFIG(action-ok[,action-ng[,action-cross-compiling]])
dnl IN6_FUNC_GETADDRINFO_AI_ALL(action-ok[,action-ng[,action-cross-compiling]])
dnl ==================================================================================
AC_DEFUN(_IN6_FUNC_GETADDRINFO_AI_FLAGS_NO_CONFLICTS,[
AC_TRY_RUN([
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
int main(){	return ((AI_ADDRCONFIG|AI_ALL|AI_V4MAPPED) & (AI_PASSIVE|AI_CANONNAME|AI_NUMERICHOST));	}
],[$1],[$2],[$3])])

AC_DEFUN(__IN6_FUNC_GETADDRINFO_AI_FLAGS,[
AC_TRY_RUN([
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
int main(){
	struct addrinfo hints, *res0;
	int ret;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = $1;
	ret = getaddrinfo(NULL, "12345", &hints, &res0);
	if (ret)
		ret = 1;
	else
		freeaddrinfo(res0);
	return ret;
}
],[$2],[$3],[$4])])

AC_DEFUN(_IN6_FUNC_GETADDRINFO_AI_FLAGS,[
AC_MSG_CHECKING([whether you can use $2 for getaddrinfo])
AC_CACHE_VAL($1, [
_IN6_FUNC_GETADDRINFO_AI_FLAGS_NO_CONFLICTS(
  [	__IN6_FUNC_GETADDRINFO_AI_FLAGS($2,
	  [	$1=yes
	],[	$1=no
	],[	$1=unknown
	])
],[	$1=no
],[	$1=unknown
])])
if test "$$1" = "yes"; then
	AC_MSG_RESULT([yes])
	$3
elif test "$$1" = "no"; then
	AC_MSG_RESULT([no])
	$4
else
	AC_MSG_RESULT([unknown])
	$5
fi
])
AC_DEFUN(IN6_FUNC_GETADDRINFO_AI_ADDRCONFIG,[
_IN6_FUNC_GETADDRINFO_AI_FLAGS(
	in6_cv_func_getaddrinfo_ai_addrconfig,
	AI_ADDRCONFIG,
	[$1],[$2],[$3])])
AC_DEFUN(IN6_FUNC_GETADDRINFO_AI_ALL,[
_IN6_FUNC_GETADDRINFO_AI_FLAGS(
	in6_cv_func_getaddrinfo_ai_all,
	AI_V4MAPPED|AI_ALL,
	[$1],[$2],[$3])])


