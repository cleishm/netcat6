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
dnl // $USAGI: in6_enable_ipv6.m4,v 1.4 2000/10/27 08:11:23 yoshfuji Exp $
dnl ====================================================================================
dnl IN6_ENABLE_IPV6[action-if-enabled[,action-if-disabled[,action-if-cross-compiling]]])
dnl ====================================================================================
AC_DEFUN(IN6_ENABLE_IPV6,[
AC_MSG_CHECKING([whether to enable ipv6])
AC_ARG_ENABLE(ipv6,
[AC_HELP_STRING([--enable-ipv6],[Enable ipv6 support])
AC_HELP_STRING([--disable-ipv6],[Disable ipv6 support])],
[case "$enableval" in
  no)	AC_MSG_RESULT(no)
        ipv6=no
	$2
	;;
  *)	AC_MSG_RESULT(yes)
        ipv6=${enableval}
	$1
	;;
esac],
AC_TRY_RUN([ /* PF_INET6 avalable check */
#include <sys/types.h>
#include <sys/socket.h>
main(){if (socket(PF_INET6, SOCK_STREAM, 0) < 0) exit(1); else exit(0);}
],[	AC_MSG_RESULT(yes)
        ipv6=yes 
	$1
],[	AC_MSG_RESULT(no)
        ipv6=no
	$2
],[	AC_MSG_RESULT(unknown)
        ipv6=no
	$3
]))])
