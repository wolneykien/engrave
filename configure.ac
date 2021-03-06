#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([engrave], [0.6.2], [manowar@altlinux.org])
AM_INIT_AUTOMAKE([engrave], [0.6.2])
AC_CONFIG_SRCDIR([share/misc.h])
AC_CONFIG_HEADER([config.h])

AC_CANONICAL_HOST
win=
case "$host_os" in
     mingw*)
        win=yes
        ;;
esac
AM_CONDITIONAL([WINDOWS],[test -n "$win"])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lm':
AC_CHECK_LIB([m], [main],[],[echo "Need libm. Please, install it"; exit 1])
# FIXME: Replace `main' with a function in `-ltiff':
AC_CHECK_LIB([tiff], [main],[],[echo "Need libtiff. Please, install it"; exit 1])

# Checks for header files.
AC_FUNC_ALLOCA
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_MAJOR
AC_CHECK_HEADERS([fcntl.h libintl.h locale.h memory.h stdlib.h string.h strings.h sys/file.h sys/param.h sys/time.h unistd.h utime.h tiff.h tiffio.h])

# Check for C++ PDFWriter
AC_PROG_CXX # TODO: conditional
AC_LANG_PUSH([C++])
AC_CHECK_HEADER([PDFWriter/PDFWriter.h], [
  AC_DEFINE([WITH_PDFWRITER], [1],
			[Define if PDFWriter/PDFWriter.h exists.])
  with_pdfwriter=y
])
AC_LANG_POP([C++])
AM_CONDITIONAL([WITH_PDFWRITER],[test -n "$with_pdfwriter"])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STAT
AC_C_CONST
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_CHECK_MEMBERS([struct stat.st_blksize])

# Checks for library functions.
# AC_FUNC_MALLOC # Don't use with MinGW
AC_FUNC_STAT
AC_FUNC_STRTOD
AC_CHECK_FUNCS([atexit memchr memset rint strtol strtoul])

AC_PROG_RANLIB
AC_CONFIG_FILES([Makefile
                 bin/Makefile
                 filters/Makefile
                 share/Makefile])
AC_OUTPUT
