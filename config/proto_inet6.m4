AC_DEFUN([PROTO_INET6],[
  AC_MSG_CHECKING([for INET6 protocol support])

  AC_CACHE_VAL([cv_proto_inet6],[
    AC_TRY_CPP([
#include <sys/types.h>
#include <sys/socket.h>

#ifndef PF_INET6
#error Missing PF_INET6
#endif
#ifndef AF_INET6
#error Mlssing AF_INET6
#endif
    ],[
      cv_proto_inet6=yes
    ],[
      cv_proto_inet6=no
    ])
  ])

  if test "X$cv_proto_inet6" = "Xyes"; then
    AC_MSG_RESULT([yes])
    $1
  else
    AC_MSG_RESULT([no])
    $2
  fi
])
