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
dnl // $USAGI: in6x_struct_icmp6hdr.m4,v 1.4 2000/10/30 15:19:46 yoshfuji Exp $
dnl ==================================================
dnl IN6X_STRUCT_ICMP6_HDR(action-if-ok[,action-if-ng])
dnl ==================================================
AC_DEFUN(IN6X_STRUCT_ICMP6_HDR,[
AC_MSG_CHECKING([whether netinet/icmp6.h has struct icmp6_hdr])
AC_CACHE_VAL(in6x_cv_struct_icmp6_hdr, [
AC_TRY_COMPILE([
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/icmp6.h>
],[
int i = sizeof(struct icmp6_hdr);
],[	in6x_cv_struct_icmp6_hdr=yes
],[	in6x_cv_struct_icmp6_hdr=no
])])
if test "$in6x_cv_struct_icmp6_hdr" = "yes"; then
	AC_MSG_RESULT([yes])
	$1
else
	AC_MSG_RESULT([no])
	$2
fi
])

dnl =======================================================
dnl IN6X_STRUCT_ICMP6_ROUTER_RENUM(action-if-ok[,action-if-ng])
dnl =======================================================
AC_DEFUN(IN6X_STRUCT_ICMP6_ROUTER_RENUM,[
AC_MSG_CHECKING([whether netinet/icmp6.h has struct icmp6_router_renum])
AC_CACHE_VAL(in6x_cv_struct_icmp6_router_renum, [
AC_TRY_COMPILE([
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/icmp6.h>
],[
int i = sizeof(struct icmp6_router_renum);
],[	in6x_cv_struct_icmp6_router_renum=yes
],[	in6x_cv_struct_icmp6_router_renum=no
])])
if test "$in6x_cv_struct_icmp6_router_renum" = "yes"; then
	AC_MSG_RESULT([yes])
	$1
else
	AC_MSG_RESULT([no])
	$2
fi
])

dnl =======================================================
dnl IN6X_STRUCT_ICMP6_NODEINFO(action-if-ok[,action-if-ng])
dnl =======================================================
AC_DEFUN(IN6X_STRUCT_ICMP6_NODEINFO,[
AC_MSG_CHECKING([whether netinet/icmp6.h has struct icmp6_nodeinfo])
AC_CACHE_VAL(in6x_cv_struct_icmp6_nodeinfo, [
AC_TRY_COMPILE([
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/icmp6.h>
],[
int i = sizeof(struct icmp6_nodeinfo);
],[	in6x_cv_struct_icmp6_nodeinfo=yes
],[	in6x_cv_struct_icmp6_nodeinfo=no
])])
if test "$in6x_cv_struct_icmp6_nodeinfo" = "yes"; then
	AC_MSG_RESULT([yes])
	$1
else
	AC_MSG_RESULT([no])
	$2
fi
])

