dnl Process this file with autoconf to produce a configure script.
AC_INIT(nc6, 0.5-cvs, [ds6-devel@deepspace6.net])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_SRCDIR(src/main.c)
AM_INIT_AUTOMAKE(1.6)        dnl Automake 1.6 or better is required
AM_CONFIG_HEADER(config.h)
AC_PREREQ(2.52)              dnl Autoconf 2.52 or better is required


dnl Languages supported
ALL_LINGUAS="it"


dnl Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_RANLIB
AC_ISC_POSIX
AC_PROG_INSTALL


dnl default CFLAGS to use for nc6 source
NC6_CFLAGS=""


dnl internationalization macros
AM_GNU_GETTEXT


dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(
  [errno.h fcntl.h limits.h netdb.h netinet/in.h signal.h sys/socket.h sys/time.h unistd.h],,
  [AC_MSG_ERROR([Missing headers required to compile nc6])]
)
AC_CHECK_HEADER(stdint.h)


dnl Check paths
PATH_BSHELL
PATH_DEVNULL


dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_CHECK_TYPE(ssize_t, int)
TYPE_SOCKLEN_T
TYPE_STRUCT_SOCKADDR_STORAGE(,
  [AC_MSG_ERROR([Your system does not support 'struct sockaddr_storage', which is needed to compile nc6])])
DEFINED_SOMAXCONN(,AC_DEFINE([SOMAXCONN], 5, [Default backlog parameter for listen(2).]))

dnl Configure Bluez support
bluez=
AC_ARG_ENABLE(bluez,
  AC_HELP_STRING(
    [--enable-bluez],[enable Bluetooth support (only on Linux)]),
  [case "${enable_bluez}" in
  no)
    AC_MSG_NOTICE([Disabling Bluetooth at user request])
    bluez=no
    ;;
  *)
    bluez=yes
    ;;
  esac],
  [bluez=yes]
)

dnl Check for sockaddr_l2
if test "X$bluez" = "Xyes"; then
  TYPE_STRUCT_SOCKADDR_L2(,[
    AC_MSG_NOTICE([Disabling Bluetooth support: 'struct sockaddr_l2cap' support is required])
    bluez=no
  ])
fi

dnl Check for Bluetooth protocol flags (PF_BLUETOOTH, etc)
if test "X$bluez" = "Xyes"; then
  PROTO_BLUEZ([
    AC_CHECK_LIB(bluetooth, baswap, , bluez=no)
  ],[
    AC_MSG_NOTICE([Disabling Bluetooth support: PF_BLUETOOTH protocol family is required])
    bluez=no
  ])
fi

if test "X$bluez" = "Xyes"; then
  AC_DEFINE([ENABLE_BLUEZ], 1, [Define if Bluetooth support is enabled.])
fi


dnl Configure IPv6 support
ipv6=
AC_ARG_ENABLE(ipv6,
  AC_HELP_STRING(
    [--disable-ipv6],[disable IPv6 support]),
  [case "${enable_ipv6}" in
  no)
    AC_MSG_NOTICE([Disabling IPv6 at user request])
    ipv6=no
    ;;
  *)
    ipv6=yes
    ;;
  esac],
  [ipv6=yes]
)

dnl Check for sockaddr_in6
if test "X$ipv6" = "Xyes"; then
  TYPE_STRUCT_SOCKADDR_IN6(,[
    AC_MSG_NOTICE([Disabling IPv6 support: 'struct sockaddr_in6' support is required])
    ipv6=no
  ])

  dnl Check for scope id
  MEMBER_SIN6_SCOPE_ID
fi

dnl Check for in6 protocol flags (AF_INET6, etc)
if test "X$ipv6" = "Xyes"; then
  PROTO_INET6(,[
    AC_MSG_NOTICE([Disabling IPv6 support: INET6 protocol support is required])
    ipv6=no
  ])
fi

if test "X$ipv6" = "Xyes"; then
  AC_DEFINE([ENABLE_IPV6], 1, [Define if IPv6 support is enabled.])
fi


AC_ARG_ENABLE(stack-guess,
  AC_HELP_STRING(
    [--disable-stack-guess],
    [disable guess of IPv6 stack]
  ),
  [case "${enable_stack_guess}" in
  yes)  
    stack_guess=yes
    ;;
  no) 
    stack_guess=no
    ;;
  *)    
    AC_MSG_ERROR(bad value ${enable_stack_guess} for --enable-stack-guess option) 
    ;;
  esac],
  [stack_guess=yes]
)

if test "X$stack_guess" != "Xno"; then
  IN6_GUESS_STACK
  NC6_CFLAGS="${NC6_CFLAGS} ${INET6_CFLAGS}"
  LIBS="${INET6_LIBS} ${LIBS}"
fi

dnl Check for libraries
AC_CHECK_LIB(socket, socket)


dnl Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF

AC_CHECK_FUNCS(
  [gettimeofday memset select socket getsockopt strerror strrchr strtol signal fcntl getaddrinfo freeaddrinfo gai_strerror getnameinfo],,
  [AC_MSG_ERROR([Missing functions required to compile nc6])]
)

dnl actually, the GETADDRINFO_AI_* macros do __NOT__ check if getaddrinfo 
dnl supports the AI_* flag, but only if the flag is defined in <netdb.h>
GETADDRINFO_AI_ADDRCONFIG(
  AC_DEFINE(
    [HAVE_GETADDRINFO_AI_ADDRCONFIG], 1, 
    [Define if the system headers support the AI_ADDRCONFIG flag.]
  )
)

GETADDRINFO_AI_V4MAPPED(
  AC_DEFINE(
    [HAVE_GETADDRINFO_AI_V4MAPPED], 1, 
    [Define if the system headers support the AI_V4MAPPED flag.]
  )
)

GETADDRINFO_AI_ALL(
  AC_DEFINE(
    [HAVE_GETADDRINFO_AI_ALL], 1, 
    [Define if the system headers support the AI_ALL flag.]
  )
)

dnl check if EAI_ADDRFAMILY is defined in <netdb.h> 
GETADDRINFO_EAI_ADDRFAMILY(
  AC_DEFINE(
    [HAVE_GETADDRINFO_EAI_ADDRFAMILY], 1,
    [Define if the system headers support the EAI_ADDRFAMILY error code.]
  )
)

dnl check if EAI_NODATA is defined in <netdb.h>
GETADDRINFO_EAI_NODATA(
  AC_DEFINE(
    [HAVE_GETADDRINFO_EAI_NODATA], 1,
    [Define if the system headers support the EAI_NODATA error code.]
  )
)

dnl The CFLAGS to use with GCC
if test "X$GCC" = "Xyes"; then
  NC6_CFLAGS="${NC6_CFLAGS} -pipe -W -Wall -Wpointer-arith -Wstrict-prototypes -Wcast-qual -Wcast-align -finline-functions"
fi

AC_SUBST(NC6_CFLAGS)
AC_SUBST(ac_aux_dir)
AC_CONFIG_FILES([Makefile docs/Makefile src/Makefile contrib/Makefile  config/Makefile \
	intl/Makefile po/Makefile.in nc6.spec docs/nc6.1])
AC_OUTPUT
