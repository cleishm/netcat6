AC_DEFUN([PROTO_BLUEZ],[
  AC_CACHE_CHECK([for Bluetooth protocol support], [ds6_cv_proto_bluez],[
    AC_TRY_CPP([
#include <sys/types.h>
#include <sys/socket.h>

#ifndef PF_BLUETOOTH
#error Missing PF_BLUETOOTH
#endif
#ifndef AF_BLUETOOTH
#error Mlssing AF_BLUETOOTH
#endif
    ],[
      ds6_cv_proto_bluez=yes
    ],[
      ds6_cv_proto_bluez=no
    ])
  ])

  if test "X$ds6_cv_proto_bluez" = "Xyes"; then :
    $1
  else :
    $2
  fi
])
