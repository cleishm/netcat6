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
	$1
elif test "$in6_cv_func_getaddrinfo" = "buggy"; then
	AC_MSG_RESULT([buggy])
	$2
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

