AC_DEFUN(TYPE_STRUCT_SOCKADDR_STORAGE,[
  AC_CHECK_TYPES([struct sockaddr_storage],[$1],[$2],[
#include <sys/types.h>
#include <sys/socket.h>
  ])
])
