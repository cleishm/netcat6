AC_DEFUN([GETADDRINFO_AI_ADDRCONFIG],[
  AC_CACHE_CHECK([if getaddrinfo supports AI_ADDRCONFIG],
    [nc_cv_gai_ai_addrconfig],[
    AC_TRY_CPP([
#include <netdb.h>

#ifndef AI_ADDRCONFIG
#error Missing AI_ADDRCONFIG
#endif
    ],[
      nc_cv_gai_ai_addrconfig=yes
    ],[
      nc_cv_gai_ai_addrconfig=no
    ])
  ])

  if test "X$nc_cv_gai_ai_addrconfig" = "Xyes"; then :
    $1
  else :
    $2
  fi
])



AC_DEFUN([GETADDRINFO_AI_ALL],[
  AC_CACHE_CHECK([if getaddrinfo supports AI_ALL],[nc_cv_gai_ai_all],[
    AC_TRY_CPP([
#include <netdb.h>

#ifndef AI_ALL
#error Missing AI_ALL
#endif
    ],[
      nc_cv_gai_ai_all=yes
    ],[
      nc_cv_gai_ai_all=no
    ])
  ])

  if test "X$nc_cv_gai_ai_all" = "Xyes"; then :
    $1
  else :
    $2
  fi
])



AC_DEFUN([GETADDRINFO_AI_V4MAPPED],[
  AC_CACHE_CHECK([if getaddrinfo supports AI_V4MAPPED],[nc_cv_gai_ai_v4mapped],[
    AC_TRY_CPP([
#include <netdb.h>

#ifndef AI_V4MAPPED
#error Missing AI_V4MAPPED
#endif
    ],[
      nc_cv_gai_ai_v4mapped=yes
    ],[
      nc_cv_gai_ai_v4mapped=no
    ])
  ])

  if test "X$nc_cv_gai_ai_v4mapped" = "Xyes"; then :
    $1
  else :
    $2
  fi
])

