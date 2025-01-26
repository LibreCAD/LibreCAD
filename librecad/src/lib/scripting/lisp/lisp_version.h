#ifndef LISP_VERSION_H
#define LISP_VERSION_H

#define LISP_STR_HELPER(x) #x
#define LISP_STR(x) LISP_STR_HELPER(x)

#ifdef __GNUG__
#define COMPILER "GCC"
#define HOST "linux"
#else
#define COMPILER "MSC"
#define HOST "windows"
#endif

// __LISP__
#ifndef __LISP__
#define LISP_MAJOR_VER  1
#define LISP_MINOR_VER  2
#define LISP_PATCHLEVEL 2
#define LISP_BUILD "devel"

#define LISP_VERSION LISP_MAJOR_VER * 10000 \
    + LISP_MINOR_VER * 100 \
    + LISP_PATCHLEVEL

#define __LISP__ \
    LISP_STR(LISP_MAJOR_VER) \
    "." \
    LISP_STR(LISP_MINOR_VER) \
    "." \
    LISP_STR(LISP_PATCHLEVEL)

#define LISP_VERSION_STR_HELPER(rel, build, date, time) \
    "LibreLisp " rel " (" build ", " date ", " time ") [" COMPILER " " __VERSION__ "] on " HOST

#define LISP_VERSION_STR \
    LISP_VERSION_STR_HELPER(__LISP__,LISP_BUILD,__DATE__,__TIME__)

#define LISP_COPYRIGHT \
    "\nType \"help\", \"copyright\", \"credits\" or \"license\" " \
    "for more information."
#endif

#endif
