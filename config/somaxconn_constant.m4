AC_DEFUN([DEFINED_SOMAXCONN],[
  AC_CACHE_CHECK([if system defines SOMAXCONN],
    [ds6_cv_defined_somaxconn],[
    AC_TRY_CPP([
#include <sys/socket.h>

#ifndef SOMAXCONN
#error Missing SOMAXCONN
#endif
    ],[
      ds6_cv_defined_somaxconn=yes
    ],[
      ds6_cv_defined_somaxconn=no
    ])
  ])

  if test "X$ds6_cv_defined_somaxconn" = "Xyes"; then :
    $1
  else :
    $2
  fi
])
