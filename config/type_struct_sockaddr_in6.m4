AC_DEFUN(TYPE_STRUCT_SOCKADDR_IN6,[
  AC_CHECK_TYPES([struct sockaddr_in6],[$1],[$2],[
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
  ])
])
