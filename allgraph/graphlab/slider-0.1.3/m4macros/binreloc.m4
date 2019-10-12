# Check for binary relocation support.
# Written by Hongli Lai
# http://autopackage.org/
# Modified by Andrew Makousky

# AM_BINRELOC([BINRELOC-ETC], [BINRELOC-LIBEXEC])
#
# Check is the system is configured so that BinReloc can be enabled.
# If BINRELOC-ETC or BINRELOC-LIBEXEC are set to 1 (one), then the
# application will expect to use BinReloc for the respective
# directory.
# --------------------------------------------------------------
AC_DEFUN([AM_BINRELOC],
[
  AC_ARG_ENABLE(binreloc,
    AC_HELP_STRING([--enable-binreloc],
      [compile with binary relocation support.  Possible values:
       auto, linux, freebsd, no.  (default=auto)]),
    enable_binreloc=$enableval, enable_binreloc=auto)

  if test "x$enable_binreloc" = "xauto"; then
    # Checking for Linux procfs...
    AC_CHECK_FILE([/proc/self/exe])
    if test "x$ac_cv_file__proc_self_exe" = "xyes"; then
      enable_binreloc=linux
    fi
    # Checking for FreeBSD procfs...
    AC_CHECK_FILE([/proc/curproc/file])
    if test "x$ac_cv_file__proc_curproc_file" = "xyes"; then
      enable_binreloc=freebsd
    fi
    AC_CACHE_CHECK([[whether sysctl() supports process pathname lookup]],
      [br_cv_sysctl],
      [AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
[[#include <limits.h>
#include <sys/types.h>
#include <sys/sysctl.h>]],
[[	char fullpath[PATH_MAX];
	size_t len = PATH_MAX;
	int name[4];
	name[0] = CTL_KERN;
	name[1] = KERN_PROC;
	name[2] = KERN_PROC_PATHNAME;
	name[3] = -1;

	if (sysctl (name, 4, fullpath, &len, (void *)0, 0) == -1 ||
	    len == 0 || fullpath[0] == '\0') {
	    return 1;
	}]])],
      br_cv_sysctl=yes,
      br_cv_sysctl=no)])
    if test "x$br_cv_sysctl" = "xyes"; then
      enable_binreloc=freebsd
    fi
    AC_CACHE_CHECK([whether everything is installed to the same prefix],
             [br_cv_valid_prefixes], [
        if test "$bindir" = '${exec_prefix}/bin' -a \
          "$sbindir" = '${exec_prefix}/sbin' -a \
          \( \( "$datarootdir" = '${prefix}/share' -a \
                "$datadir" = '${datarootdir}' \) -o \
             "$datadir" = '${prefix}/share' \) -a \
          "$libdir" = '${exec_prefix}/lib' -a \
          \( \( "x$2" = "x1" -a "$libexecdir" = '${exec_prefix}/libexec' \) -o \
	     "x$2" != "x1" \) -a \
	  \( \( "x$1" = "x1" -a "$sysconfdir" = '${prefix}/etc' \) -o \
	     "x$1" != "x1" \)
        then
          br_cv_valid_prefixes=yes
        else
          br_cv_valid_prefixes=no
        fi
        ])
    if test "x$br_cv_valid_prefixes" = "xno"; then
      enable_binreloc=no
    fi
  fi

  AC_CACHE_CHECK([whether binary relocation support should be enabled],
           [br_cv_binreloc],
           [if test "x$enable_binreloc" = "xno"; then
             br_cv_binreloc=no
           else
             br_cv_binreloc=yes
           fi])
  if test "x$br_cv_binreloc" = "xyes"; then
    if test "x$enable_binreloc" = "xlinux"; then
      AC_DEFINE(BR_LINUX_PROCFS, 1,
        [Use Linux procfs for binary relocation.])
    elif test "x$enable_binreloc" = "xfreebsd"; then
      AC_DEFINE(BR_FREEBSD, 1,
        [Use binary relocation code suitable for FreeBSD.])
    else
      AC_MSG_ERROR([unsupported binreloc architecture detected])
    fi
    AC_DEFINE(ENABLE_BINRELOC, 1, [Enable binary relocation support.])
  fi
  AM_CONDITIONAL(USE_BINRELOC, test x$br_cv_binreloc = xyes)
])
