AC_DEFUN([GETADDRINFO_AI_ADDRCONFIG],[
  AC_MSG_CHECKING([if getaddrinfo supports AI_ADDRCONFIG])

  AC_CACHE_VAL([cv_gai_ai_addrconfig],[
    AC_TRY_CPP([
#include <netdb.h>

#ifndef AI_ADDRCONFIG
#error Missing AI_ADDRCONFIG
#endif
    ],[
      cv_gai_ai_addrconfig=yes
    ],[
      cv_gai_ai_addrconfig=no
    ])
  ])

  if test "X$cv_gai_ai_addrconfig" = "Xyes"; then
    AC_MSG_RESULT([yes])
    $1
  else
    AC_MSG_RESULT([no])
    $2
  fi
])



AC_DEFUN([GETADDRINFO_AI_ALL],[
  AC_MSG_CHECKING([if getaddrinfo supports AI_ALL])

  AC_CACHE_VAL([cv_gai_ai_all],[
    AC_TRY_CPP([
#include <netdb.h>

#ifndef AI_ALL
#error Missing AI_ALL
#endif
    ],[
      cv_gai_ai_all=yes
    ],[
      cv_gai_ai_all=no
    ])
  ])

  if test "X$cv_gai_ai_all" = "Xyes"; then
    AC_MSG_RESULT([yes])
    $1
  else
    AC_MSG_RESULT([no])
    $2
  fi
])



AC_DEFUN([GETADDRINFO_AI_V4MAPPED],[
  AC_MSG_CHECKING([if getaddrinfo supports AI_V4MAPPED])

  AC_CACHE_VAL([cv_gai_ai_v4mapped],[
    AC_TRY_CPP([
#include <netdb.h>

#ifndef AI_V4MAPPED
#error Missing AI_V4MAPPED
#endif
    ],[
      cv_gai_ai_v4mapped=yes
    ],[
      cv_gai_ai_v4mapped=no
    ])
  ])

  if test "X$cv_gai_ai_v4mapped" = "Xyes"; then
    AC_MSG_RESULT([yes])
    $1
  else
    AC_MSG_RESULT([no])
    $2
  fi
])

