AC_DEFUN([TYPE_STRUCT_SOCKADDR_IN6],[
  ds6_have_sockaddr_in6=
  AC_CHECK_TYPES([struct sockaddr_in6],[
    ds6_have_sockaddr_in6=yes
  ],[
    ds6_have_sockaddr_in6=no
  ],[
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
  ])

  if test "X$ds6_have_sockaddr_in6" = "Xyes"; then :
    $1
  else :
    $2
  fi
])
