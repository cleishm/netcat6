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

