dnl Check for paths header
AC_DEFUN([HEADER_PATHS],[
  AC_CHECK_HEADERS(paths.h,[ds6_have_paths_h=yes],[ds6_have_paths_h=no])
  AC_DEFINE(HAVE_PATHS_H, 1, [Define if header file <paths.h> is present])
])

dnl Check for bourne shell location
AC_DEFUN([PATH_BSHELL],[
  AC_REQUIRE([HEADER_PATHS])

  have_path_bshell=
  if test "X$ds6_have_paths_h" = "Xyes"; then
    AC_TRY_CPP([
#include <paths.h>

#ifndef _PATH_BSHELL
#error Missing _PATH_BSHELL
#endif
    ],[
      have_path_bshell=yes
    ],[
      have_path_bshell=no
    ])
  fi

  if test "X$have_path_bshell" != "Xyes"; then
    AC_CACHE_CHECK([for bourne shell], [ds6_cv_prog_bshell],[
      ds6_cv_prog_bshell=unknown
      for shell in /bin/sh /usr/bin/sh /sbin/sh /usr/sbin/sh \
                   /bin/ash /usr/bin/ash \
		   /bin/ksh /usr/bin/ksh \
		   /bin/bash /usr/bin/bash \
		   /bin/zsh /usr/bin/zsh /usr/local/bin/zsh; do
        if test -f $shell; then
          ds6_cv_prog_bshell=$shell
          break
        fi
      done
    ])

    if test "X$ds6_cv_prog_bshell" = "Xunknown"; then
      AC_MSG_ERROR(Can't find the bourne shell)
    else
      AC_DEFINE_UNQUOTED(_PATH_BSHELL, "$ds6_cv_prog_bshell",
        [location of bourne shell])
    fi
  fi
])


dnl Check for null device (/dev/null)
AC_DEFUN([PATH_DEVNULL],[
  AC_REQUIRE([HEADER_PATHS])

  have_path_devnull=
  if test "X$ds6_have_paths_h" = "Xyes"; then
    AC_TRY_CPP([
#include <paths.h>

#ifndef _PATH_DEVNULL
#error Missing _PATH_DEVNULL
#endif
    ],[
      have_path_devnull=yes
    ],[
      have_path_devnull=no
    ])
  fi

  if test "X$have_path_devnull" != "Xyes"; then
    AC_CACHE_CHECK([for null device], [ds6_cv_prog_devnull],[
      ds6_cv_prog_devnull=unknown
      for devnull in /dev/null; do
        if test -c $devnull; then
          ds6_cv_prog_devnull=$devnull
          break
        fi
      done
    ])

    if test "X$ds6_cv_prog_devnull" = "Xunknown"; then
      AC_MSG_ERROR(Can't find null device)
    else
      AC_DEFINE_UNQUOTED(_PATH_DEVNULL, "$ds6_cv_prog_devnull",
        [location of null device])
    fi
  fi
])
