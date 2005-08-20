AC_DEFUN([DEFINED_WAIT_ANY],[
  AC_CACHE_CHECK([if system defines WAIT_ANY],
    [ds6_cv_defined_wait_any],[
    AC_TRY_CPP([
#include <sys/types.h>
#include <sys/wait.h>

#ifndef WAIT_ANY
#error Missing WAIT_ANY
#endif
    ],[
      ds6_cv_defined_wait_any=yes
    ],[
      ds6_cv_defined_wait_any=no
    ])
  ])

  if test "X$ds6_cv_defined_wait_any" = "Xyes"; then :
    $1
  else :
    $2
  fi
])
