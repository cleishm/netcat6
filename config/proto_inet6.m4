AC_DEFUN([PROTO_INET6],[
  AC_CACHE_CHECK([for INET6 protocol support], [ds6_cv_proto_inet6],[
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
      ds6_cv_proto_inet6=yes
    ],[
      ds6_cv_proto_inet6=no
    ])
  ])

  if test "X$ds6_cv_proto_inet6" = "Xyes"; then :
    $1
  else :
    $2
  fi
])
