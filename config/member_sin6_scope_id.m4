AC_DEFUN([MEMBER_SIN6_SCOPE_ID],[
  AC_REQUIRE([TYPE_STRUCT_SOCKADDR_IN6])
  
  ds6_member_sin6_scope_id=
  if test "X$ds6_have_sockaddr_in6" = "Xyes"; then
    AC_CHECK_MEMBER([struct sockaddr_in6.sin6_scope_id],[
      ds6_member_sin6_scope_id=yes
    ],[
      ds6_member_sin6_scope_id=no
    ],[
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
    ])
  fi

  if test "X$ds6_member_sin6_scope_id" = "Xyes"; then
    AC_DEFINE([HAVE_SOCKADDR_IN6_SCOPE_ID], 1,
      [Define if struct sockaddr_in6 has the sin6_scope_id member])
    $1
  else :
    $2
  fi
])
