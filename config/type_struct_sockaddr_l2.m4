AC_DEFUN([TYPE_STRUCT_SOCKADDR_L2],[
  ds6_have_sockaddr_l2=
  AC_CHECK_TYPES([struct sockaddr_l2],[
    ds6_have_sockaddr_l2=yes
  ],[
    ds6_have_sockaddr_l2=no
  ],[
#include <sys/types.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
  ])

  if test "X$ds6_have_sockaddr_l2" = "Xyes"; then :
    $1
  else :
    $2
  fi
])
