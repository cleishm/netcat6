dnl ====================================================
dnl IN6_GUESS_STACK(ipv6,ipv6libdir,ipv6lib,CFLAGS,LIBS)
dnl ====================================================
AC_DEFUN(IN6_GUESS_STACK,[
	inet6_ipv6libdir=
	inet6_ipv6lib="none"
	inet6_ipv6type="unknown"
	AC_MSG_CHECKING([IPv6 stack type])
	for inet6_i in inria kame usagi linux linux_inet6 toshiba v6d zeta solaris7; do
		case ${inet6_i} in
		inria)	
			dnl http://www.kame.net/
			eval inet6_ipv6_${inet6_i}=${inet6_i}
			eval inet6_cflags_${inet6_i}=
			AC_EGREP_CPP(%%%yes%%%, [
#include <netinet/in.h>
#ifdef IPV6_INRIA_VERSION
%%%yes%%%
#endif],
				[if test "$inet6_ipv6type" = "unknown"; then
					inet6_ipv6type=${inet6_i}
				fi])
			;;
		kame)	
			dnl http://www.kame.net/
			eval inet6_ipv6_${inet6_i}=${inet6_i}
			eval inet6_cflags_${inet6_i}=
			AC_EGREP_CPP(%%%yes%%%, [
#include <netinet/in.h>
#ifdef __KAME__
%%%yes%%%
#endif],
			[if test -f /usr/local/v6/lib/libinet6.a; then
				eval inet6_ipv6lib_${inet6_i}=inet6
				eval inet6_ipv6libdir_${inet6_i}=/usr/local/v6/lib
			fi
			if test "$inet6_ipv6type" = "unknown"; then
				inet6_ipv6type=${inet6_i}
			fi])
			;;
		usagi)
			dnl http://www.linux-ipv6.org/
			eval inet6_ipv6_${inet6_i}=${inet6_i}
			eval inet6_cflags_${inet6_i}=
			AC_EGREP_CPP(%%%yes%%%, [
#include <netinet/in.h>
#ifdef __USAGI__
%%%yes%%%
#endif],
			[if test -f /usr/local/v6/lib/libinet6.a; then
				eval inet6_ipv6lib_${inet6_i}=inet6
				eval inet6_ipv6libdir_${inet6_i}=/usr/local/v6/lib
			fi
			if test "$inet6_ipv6type" = "unknown"; then
				inet6_ipv6type=${inet6_i}
			fi])
			;;
		linux)	
			dnl http://www.v6.linux.or.jp/
			eval inet6_ipv6_${inet6_i}=${inet6_i}
			eval inet6_cflags_${inet6_i}=
			AC_EGREP_CPP(%%%yes%%%, [
#include <features.h>
#if defined(__GLIBC__) && ((__GLIBC__ == 2 && __GLIBC_MINOR__ >= 1) || (__GLIBC__ > 2))
%%%yes%%%
#endif],
			[if test "$inet6_ipv6type" = "unknown"; then
				inet6_ipv6type=${inet6_i}
			fi])
			;;
		linux_inet6)
			eval inet6_ipv6_${inet6_i}=${inet6_i}
			eval inet6_ipv6lib_${inet6_i}=inet6
			eval inet6_ipv6libdir_${inet6_i}=/usr/inet6/lib
			eval inet6_cflags_${inet6_i}=-I/usr/inet6/include
			dnl http://www.v6.linux.or.jp/
			if test -d /usr/inet6; then
				if test "$inet6_ipv6type" = "unknown"; then
					inet6_ipv6type=${inet6_i}
				fi
			fi
			;;
		toshiba)
			eval inet6_ipv6_${inet6_i}=${inet6_i}
			eval inet6_ipv6lib_${inet6_i}=inet6
			eval inet6_ipv6libdir_${inet6_i}=/usr/local/v6/lib
			eval inet6_cflags_${inet6_i}=
			AC_EGREP_CPP(%%%yes%%%, [
#include <sys/param.h>
#ifdef _TOSHIBA_INET6
%%%yes%%%
#endif],
				[if test "$inet6_ipv6type" = "unknown"; then
					inet6_ipv6type=${inet6_i}
				fi])
			;;
		v6d)
			eval inet6_ipv6_${inet6_i}=${inet6_i}
			eval inet6_ipv6lib_${inet6_i}=v6
			eval inet6_cflags_${inet6_i}=-I/usr/local/v6/include
			AC_EGREP_CPP(%%%yes%%%, [
#include </usr/local/v6/include/sys/v6config.h>
#ifdef __V6D__
%%%yes%%%
#endif],
				[if test "$inet6_ipv6type" = "unknown"; then
					inet6_ipv6type=${inet6_i}
				fi])
			;;
		zeta)	
			eval inet6_ipv6_${inet6_i}=${inet6_i}
			eval inet6_ipv6lib_${inet6_i}=${inet6_i}
			eval inet6_ipv6libdir_${inet6_i}=/usr/local/v6/lib
			eval inet6_cflags_${inet6_i}=-I/usr/local/v6/include
			AC_EGREP_CPP(%%%yes%%%, [
#include <sys/param.h>
#ifdef _ZETA_MINAMI_INET6
yes
#endif],
				[if test "$inet6_ipv6type" = "unknown"; then
					inet6_ipv6type=${inet6_i}
				fi])
			;;
		solaris7)
			eval inet6_ipv6_${inet6_i}=${inet6_i}
			eval inet6_cflags_${inet6_i}=
			case "$host" in
			*-*-solaris*)
				if test -c /devices/pseudo/ip6@0:ip6; then
					if test "$inet6_ipv6type" = "unknown"; then
						inet6_ipv6type=${inet6_i}
					fi
				fi
			esac
			;;
		*)
			echo "$inet6_ipv6 is unknown"
			;;
		esac
	done
	eval inet6_ipv6=\$$1
	if test "$inet6_ipv6" = "yes"; then
		AC_MSG_RESULT([$inet6_ipv6type (guessed)])
	elif test "$inet6_ipv6" = "$inet6_ipv6type"; then
		AC_MSG_RESULT([$inet6_ipv6])
	else
		AC_MSG_RESULT([$inet6_ipv6, while $inet6_ipv6type is guessed])
		inet6_ipv6type=$inet6_ipv6
	fi
	if test "$inet6_ipv6type" != "unknown"; then
		eval inet6_ipv6lib=\$inet6_ipv6lib_${inet6_ipv6type}
		eval inet6_ipv6libdir=\$inet6_ipv6libdir_${inet6_ipv6type}
		eval inet6_ipv6cflags=\$inet6_cflags_${inet6_ipv6type}
		if test "X$inet6_ipv6cflags" != "X" -a "X$inet6_ipv6cflags" = "Xnone"; then
			INET6_CFLAGS="-DINET6 $inet6_ipv6cflags"
		fi
	fi
	if test "X$inet6_ipv6lib" != "X" -a "X$inet6_ipv6lib" != "Xnone"; then
		if test "X$inet6_ipv6libdir" != "X" && test -d $inet6_ipv6libdir -a -f $inet6_ipv6libdir/lib${inet6_ipv6lib}.a; then
			INET6_LIBS="-L$inet6_ipv6libdir -l$inet6_ipv6lib"
		elif test "X$inet6_ipv6libdir" = "X" -a "X$inet6_ipv6lib" != "X"; then
			INET6_LIBS="-l$inet6_ipv6lib"
		else
			echo $ac_n "Fatal: no $inet6_ipv6lib library found""$ac_c"
			if test "X$inet6_ipv6libdir" != "X"; then
				echo $ac_n " in $inet6_ipv6libdir$ac_c"
			fi
			echo '; cannot continue.'
			echo "You need to fetch lib${inet6_ipv6lib}.a from appropriate"
			echo 'ipv6 kit and compile beforehand.'
			exit 1
		fi
	fi
	$2=$inet6_ipv6libdir
	$3=$inet6_ipv6lib
])
