#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([nsdb], [1.0], [lgg@163.com])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([app/nsget/nsget_main.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_RANLIB

# Checks for libraries.

# Checks for header files.
AC_FUNC_ALLOCA
AC_HEADER_RESOLV
AC_CHECK_HEADERS([arpa/inet.h fcntl.h inttypes.h limits.h malloc.h netdb.h netinet/in.h paths.h stddef.h stdint.h stdlib.h string.h sys/file.h sys/ioctl.h sys/mount.h sys/param.h sys/socket.h sys/statvfs.h sys/time.h syslog.h unistd.h utime.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_STRERROR_R
AC_FUNC_STRNLEN
AC_FUNC_STRTOD
AC_CHECK_FUNCS([alarm bzero clock_gettime dup2 fdatasync ftruncate gethostbyname getpagesize gettimeofday inet_ntoa isascii localtime_r memmove memset mkdir munmap select setenv socket strcasecmp strchr strcspn strdup strerror strncasecmp strpbrk strstr strtol strtoul tzset utime])

AC_CONFIG_FILES([lib/Action/makefile
                 lib/client/makefile
                 lib/common/makefile
                 lib/config/makefile
                 lib/dataop/makefile
                 lib/fsm/makefile
                 lib/log/makefile
                 lib/makefile
                 lib/msg/makefile
                 lib/net/makefile
                 lib/netlink/makefile
                 lib/openssl/makefile
                 lib/process/makefile
                 lib/resource/makefile
                 lib/server/makefile
                 lib/sm/makefile
                 lib/string/makefile
                 lib/thread/makefile
                 lib/trigger/makefile
                 web/makefile
                 web/http/makefile])
AC_OUTPUT
