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

