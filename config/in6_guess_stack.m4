dnl ====================================================
dnl IN6_GUESS_STACK(ipv6,ipv6libdir,ipv6lib,CFLAGS,LIBS)
dnl ====================================================
AC_DEFUN([IN6_GUESS_STACK],[
	AC_MSG_CHECKING([IPv6 stack type])
	AC_CACHE_VAL([ds6_cv_ds6_cv_ipv6_stack],[
	ds6_cv_ipv6_stack=unknown
	for inet6_i in inria kame usagi linux linux_inet6 toshiba v6d zeta solaris7; do
		case ${inet6_i} in
		inria)	
			dnl http://www.kame.net/
			AC_EGREP_CPP(%%%yes%%%, [
#include <netinet/in.h>
#ifdef IPV6_INRIA_VERSION
%%%yes%%%
#endif],
			[if test "X$ds6_cv_ipv6_stack" = "Xunknown"; then
				ds6_cv_ipv6_stack=${inet6_i}
			fi])
			;;
		kame)	
			dnl http://www.kame.net/
			AC_EGREP_CPP(%%%yes%%%, [
#include <netinet/in.h>
#ifdef __KAME__
%%%yes%%%
#endif],
			[if test "X$ds6_cv_ipv6_stack" = "Xunknown"; then
				ds6_cv_ipv6_stack=${inet6_i}
			fi])
			;;
		usagi)
			dnl http://www.linux-ipv6.org/
			AC_EGREP_CPP(%%%yes%%%, [
#include <netinet/in.h>
#ifdef __USAGI__
%%%yes%%%
#endif],
			[if test "X$ds6_cv_ipv6_stack" = "Xunknown"; then
				ds6_cv_ipv6_stack=${inet6_i}
			fi])
			;;
		linux)	
			dnl http://www.v6.linux.or.jp/
			AC_EGREP_CPP(%%%yes%%%, [
#include <features.h>
#if defined(__GLIBC__) && ((__GLIBC__ == 2 && __GLIBC_MINOR__ >= 1) || (__GLIBC__ > 2))
%%%yes%%%
#endif],
			[if test "X$ds6_cv_ipv6_stack" = "Xunknown"; then
				ds6_cv_ipv6_stack=${inet6_i}
			fi])
			;;
		linux_inet6)
			dnl http://www.v6.linux.or.jp/
			if test -d /usr/inet6; then
				if test "X$ds6_cv_ipv6_stack" = "Xunknown"; then
					ds6_cv_ipv6_stack=${inet6_i}
				fi
			fi
			;;
		toshiba)
			AC_EGREP_CPP(%%%yes%%%, [
#include <sys/param.h>
#ifdef _TOSHIBA_INET6
%%%yes%%%
#endif],
			[if test "X$ds6_cv_ipv6_stack" = "Xunknown"; then
				ds6_cv_ipv6_stack=${inet6_i}
			fi])
			;;
		v6d)
			AC_EGREP_CPP(%%%yes%%%, [
#include </usr/local/v6/include/sys/v6config.h>
#ifdef __V6D__
%%%yes%%%
#endif],
			[if test "X$ds6_cv_ipv6_stack" = "Xunknown"; then
				ds6_cv_ipv6_stack=${inet6_i}
			fi])
			;;
		zeta)	
			AC_EGREP_CPP(%%%yes%%%, [
#include <sys/param.h>
#ifdef _ZETA_MINAMI_INET6
yes
#endif],
			[if test "X$ds6_cv_ipv6_stack" = "Xunknown"; then
				ds6_cv_ipv6_stack=${inet6_i}
			fi])
			;;
		solaris7)
			case "$host" in
			*-*-solaris*)
				if test -c /devices/pseudo/ip6@0:ip6; then
					if test "X$ds6_cv_ipv6_stack" = "Xunknown"; then
						ds6_cv_ipv6_stack=${inet6_i}
					fi
				fi
			esac
			;;
		*)
			AC_MSG_WARN([Unknown stack type $inet6_i tested])
			;;
		esac
	done
	])

	inet6_ipv6type=$1
	if test "X$inet6_ipv6type" = "X"; then
		inet6_ipv6type=$ds6_cv_ipv6_stack
		AC_MSG_RESULT([$inet6_ipv6type (guessed)])
	elif test "X$ds6_cv_ipv6_stack" = "X$inet6_ipv6type"; then
		AC_MSG_RESULT([$inet6_ipv6type])
	else
		AC_MSG_RESULT([$inet6_ipv6type, while $ds6_cv_ipv6_stack is guessed])
	fi

	inet6_ipv6lib=
	inet6_ipv6libdir=
	inet6_cppflags=

	case "$inet6_ipv6type" in
	unknown) ;;
	inria) ;;
	kame)
		if test -f /usr/local/v6/lib/libinet6.a; then
			inet6_ipv6lib=inet6
			inet6_ipv6libdir=/usr/local/v6/lib
		fi
		;;
	usagi)
		if test -f /usr/local/v6/lib/libinet6.a; then
			inet6_ipv6lib=inet6
			inet6_ipv6libdir=/usr/local/v6/lib
		fi
		;;
	linux) ;;
	linux_inet6)
		inet6_ipv6lib=inet6
		inet6_ipv6libdir=/usr/inet6/lib
		inet6_cppflags=-I/usr/inet6/include
		;;
	toshiba)
		inet6_ipv6lib=inet6
		inet6_ipv6libdir=/usr/local/v6/lib
		;;
	v6d)
		inet6_ipv6lib=v6
		inet6_cppflags=-I/usr/local/v6/include
		;;
	zeta)
		inet6_ipv6lib=zeta
		inet6_ipv6libdir=/usr/local/v6/lib
		inet6_cppflags=-I/usr/local/v6/include
		;;
	solaris7) ;;
	*)
		AC_MSG_WARN([Unknown stack type $inet6_ipv6type])
		;;
	esac

	if test "X$inet6_ipv6type" != "Xunknown"; then
		if test "X$inet6_cppflags" != "X"; then
			INET6_CPPFLAGS="-DINET6 $inet6_cppflags"
		fi
	fi
	if test "X$inet6_ipv6lib" != "X"; then
		INET6_LIBS="-l$inet6_ipv6lib"
		if test "X$inet6_ipv6libdir" != "X"; then
			if test -d $inet6_ipv6libdir -a -f $inet6_ipv6libdir/lib${inet6_ipv6lib}.a; then
				INET6_LIBS="-L$inet6_ipv6libdir $INET6_LIBS"
			else
				AC_MSG_ERROR([
Fatal: no $inet6_ipv6lib library found in $inet6_ipv6libdir
You need to fetch lib${inet6_ipv6lib}.a from the appropriate ipv6 kit
and compile beforehand.])
			fi
		fi
	fi
	if test "X$2" != "X"; then
		$2=$inet6_ipv6libdir
	fi
	if test "X$3" != "X"; then
		$3=$inet6_ipv6lib
	fi
])
