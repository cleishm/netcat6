AC_DEFUN([TYPE_STRUCT_SOCKADDR_STORAGE],[
  ds6_have_sockaddr_storage=
  AC_CHECK_TYPES([struct sockaddr_storage],[
    ds6_have_sockaddr_storage=yes
  ],[
    ds6_have_sockaddr_storage=no
  ],[
#include <sys/types.h>
#include <sys/socket.h>
  ])

  if test "X$ds6_have_sockaddr_storage" = "Xyes"; then :
    $1
  else :
    $2
  fi
])
