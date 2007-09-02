AC_DEFUN([PROTO_IUCV],[
  AC_CACHE_CHECK([for IUCV protocol support], [ds6_cv_proto_iucv],[
    AC_TRY_RUN([
#include <sys/socket.h>
int main ()
{
  /* AF_IUCV is 32 */
  socket(32, SOCK_STREAM, 0) < 0 ? exit(1) : exit(0);
}
    ],[
      ds6_cv_proto_iucv=yes
    ],[
      ds6_cv_proto_iucv=no
    ])
  ])

  if test "X$ds6_cv_proto_iucv" = "Xyes"; then :
    $1
  else :
    $2
  fi
])
