# Process this file with autoconf to produce a configure script.
AC_INIT(src/main.c)
AC_CONFIG_AUX_DIR(config)
AM_CONFIG_HEADER(src/config.h)
AM_INIT_AUTOMAKE(nc6, 0.2pre4b)
AC_PREREQ(2.52)


# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_ISC_POSIX
AC_PROG_INSTALL


# CFLAGS to use during configure (warnings and such are added later)
CFLAGS=""


# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(
  [errno.h fcntl.h limits.h netdb.h netinet/in.h signal.h sys/socket.h sys/time.h unistd.h],,
  [AC_MSG_ERROR([Missing headers required to compile nc6])]
)
AC_CHECK_HEADER(stdint.h)


# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
TYPE_SOCKLEN_T
TYPE_STRUCT_SOCKADDR_STORAGE(,
  [AC_MSG_ERROR([Your system does not support 'struct sockaddr_storage', which is needed to compile nc6])])


# Configure IPv6 support
ipv6=
AC_ARG_ENABLE(ipv6,
  AC_HELP_STRING(
    [--disable-ipv6],[Disable IPv6 support]),
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

if test "X$ipv6" = "Xyes"; then
  TYPE_STRUCT_SOCKADDR_IN6(,[
    AC_MSG_NOTICE([Disabling IPv6 support: 'struct sockaddr_in6' support is required])
    ipv6=no
  ])
fi

if test "X$ipv6" = "Xyes"; then
  PROTO_INET6(,[
    AC_MSG_NOTICE([Disabling IPv6 support: INET6 protocol support is required])
    ipv6=no
  ])
fi

if test "X$ipv6" = "Xyes"; then
  AC_DEFINE([ENABLE_IPV6], 1, [Define if IPV6 support is enabled.])
fi


AC_ARG_ENABLE(stack-guess,
  AC_HELP_STRING(
    [--enable-stack-guess],
    [Enable guess of IPv6 stack (default)]
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
  IN6_GUESS_STACK(ipv6, ipv6libdir, ipv6lib)
  CFLAGS="$CFLAGS $INET6_CFLAGS"
  LIBS="$INET6_LIBS $LIBS"
fi


# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_MEMCMP
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF

AC_CHECK_FUNCS(
  [gettimeofday memset select socket strerror strrchr strtol signal fcntl getaddrinfo freeaddrinfo gai_strerror getnameinfo],,
  [AC_MSG_ERROR([Missing functions required to compile nc6])]
)

IN6_FUNC_GETADDRINFO([],
  [AC_MSG_ERROR([Your system has a buggy getaddrinfo implementation])]
)

IN6_FUNC_GETADDRINFO_AI_ADDRCONFIG(
  AC_DEFINE(
    [HAVE_GETADDRINFO_AI_ADDRCONFIG], 1, 
    [Define if getaddrinfo supports AI_ADDRCONFIG flag.]
  )
)



# The CFLAGS to use with GCC
if test "X$GCC" = "Xyes"; then
  CFLAGS="${CFLAGS} -pipe -W -Wall -Wpointer-arith -Wstrict-prototypes -Wcast-qual -Wcast-align"
fi

# Add extra flags if in debug mode
AC_ARG_ENABLE(debug,
  AC_HELP_STRING(
    [--enable-debug],
    [Enable debugging - useful only for developers]
  ),
  [case "${enable_debug}" in
  yes)
    AC_DEFINE(DEBUG, 1, [Define if nc6 is to be build with debug information.])
    CFLAGS="${CFLAGS} -g -O0"
    ;;
  no) 
    AC_DEFINE(NDEBUG, 1, [Define if nc6 is to be build without debug information.])
    CFLAGS="${CFLAGS} -O2"
    if test "X$GCC" = "Xyes"; then 
      CFLAGS="${CFLAGS} -finline-functions"
    fi
    ;;
  *)    
    AC_MSG_ERROR(bad value ${enable_debug} for --enable-debug option) 
    ;;
  esac],
  AC_DEFINE(NDEBUG,1)
)
    

AC_SUBST(ac_aux_dir)
AC_OUTPUT([Makefile docs/Makefile src/Makefile nc6.spec],
[test -z "$CONFIG_HEADERS" || echo timestamp > src/stamp-h.in])
