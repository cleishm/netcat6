AC_DEFUN([GETADDRINFO_AI_ADDRCONFIG],[
  AC_CACHE_CHECK([if getaddrinfo supports AI_ADDRCONFIG],
    [ds6_cv_gai_ai_addrconfig],[
    AC_TRY_CPP([
#include <netdb.h>

#ifndef AI_ADDRCONFIG
#error Missing AI_ADDRCONFIG
#endif
    ],[
      ds6_cv_gai_ai_addrconfig=yes
    ],[
      ds6_cv_gai_ai_addrconfig=no
    ])
  ])

  if test "X$ds6_cv_gai_ai_addrconfig" = "Xyes"; then :
    $1
  else :
    $2
  fi
])



AC_DEFUN([GETADDRINFO_AI_ALL],[
  AC_CACHE_CHECK([if getaddrinfo supports AI_ALL],[ds6_cv_gai_ai_all],[
    AC_TRY_CPP([
#include <netdb.h>

#ifndef AI_ALL
#error Missing AI_ALL
#endif
    ],[
      ds6_cv_gai_ai_all=yes
    ],[
      ds6_cv_gai_ai_all=no
    ])
  ])

  if test "X$ds6_cv_gai_ai_all" = "Xyes"; then :
    $1
  else :
    $2
  fi
])



AC_DEFUN([GETADDRINFO_AI_V4MAPPED],[
  AC_CACHE_CHECK([if getaddrinfo supports AI_V4MAPPED],[ds6_cv_gai_ai_v4mapped],[
    AC_TRY_CPP([
#include <netdb.h>

#ifndef AI_V4MAPPED
#error Missing AI_V4MAPPED
#endif
    ],[
      ds6_cv_gai_ai_v4mapped=yes
    ],[
      ds6_cv_gai_ai_v4mapped=no
    ])
  ])

  if test "X$ds6_cv_gai_ai_v4mapped" = "Xyes"; then :
    $1
  else :
    $2
  fi
])



AC_DEFUN([GETADDRINFO_EAI_ADDRFAMILY],[
  AC_CACHE_CHECK([if getaddrinfo returns the EAI_ADDRFAMILY error code],
    [ds6_cv_gai_eai_addrfamily],[
    AC_TRY_CPP([
#include <netdb.h>

#ifndef EAI_ADDRFAMILY 
#error Missing EAI_ADDRFAMILY 
#endif
    ],[
      ds6_cv_gai_eai_addrfamily=yes
    ],[
      ds6_cv_gai_eai_addrfamily=no
    ])
  ])

  if test "X$ds6_cv_gai_eai_addrfamily" = "Xyes"; then :
    $1
  else :
    $2
  fi
])



AC_DEFUN([GETADDRINFO_EAI_NODATA],[
  AC_CACHE_CHECK([if getaddrinfo returns the EAI_NODATA error code],
    [ds6_cv_gai_eai_nodata],[
    AC_TRY_CPP([
#include <netdb.h>

#ifndef EAI_NODATA
#error Missing EAI_NODATA
#endif
    ],[
      ds6_cv_gai_eai_nodata=yes
    ],[
      ds6_cv_gai_eai_nodata=no
    ])
  ])

  if test "X$ds6_cv_gai_eai_nodata" = "Xyes"; then :
    $1
  else :
    $2
  fi
])

