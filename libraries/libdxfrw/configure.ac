#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([libdxfrw], [0.6.3], [https://sourceforge.net/projects/libdxfrw/])
AM_INIT_AUTOMAKE([foreign -Wall -Werror dist-bzip2 subdir-objects])
AC_CONFIG_SRCDIR([src/drw_interface.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AM_PROG_AR
LT_INIT
AC_PROG_LIBTOOL
AC_CONFIG_MACRO_DIR([m4])

#
# Libtool library revision control info
# See the libtool documentation under the heading "Libtool's versioning
# system" in order to understand the meaning of these fields
#
# Here are a set of rules to help you update your library version
# information:
#
#  1. Start with version information of `0:0:0' for each libtool library.
#  2. Update the version information only immediately before a public
#     release of your software. More frequent updates are unnecessary, and
#     only guarantee that the current interface number gets larger faster.
#  3. If the library source code has changed at all since the last update,
#     then increment revision (`c:r:a' becomes `c:r+1:a').
#  4. If any interfaces have been added, removed, or changed since the last
#     update, increment current, and set revision to 0.
#  5. If any interfaces have been added since the last public release, then
#     increment age.
#  6. If any interfaces have been removed since the last public release,
#     then set age to 0.
#
#AGE.CURRENT.REVISION
LIBRARY_CURRENT=6
LIBRARY_REVISION=3
LIBRARY_AGE=0

AC_SUBST(LIBRARY_CURRENT)dnl
AC_SUBST(LIBRARY_REVISION)dnl
AC_SUBST(LIBRARY_AGE)dnl

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL

# Checks for library functions.
AC_FUNC_STRTOD
AC_CHECK_FUNCS([sqrt])

AC_ENABLE_SHARED
AC_ENABLE_STATIC

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	dwg2dxf/Makefile
	libdxfrw${LIBRARY_AGE}.pc:libdxfrw.pc.in
	])
AC_OUTPUT
