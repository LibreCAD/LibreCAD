/* FriBidi
 * common.h - common include for library sources
 *
 * Author:
 *   Behdad Esfahbod, 2004
 *
 * Copyright (C) 2004 Sharif FarsiWeb, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library, in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA
 *
 * For licensing issues, contact <fribidi.license@gmail.com>.
 */
#ifndef _COMMON_H
#define _COMMON_H

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <fribidi-common.h>

#ifndef false
# define false (0)
# endif	/* !false */
# ifndef true
#  define true (!false)
# endif	/* !true */

#ifndef NULL
#  ifdef __cplusplus
#    define NULL        (0L)
#  else	/* !__cplusplus */
#    define NULL        ((void*) 0)
#  endif /* !__cplusplus */
#endif /* !NULL */

/* fribidi_malloc and fribidi_free should be used instead of malloc and free. 
 * No need to include any headers. */
#ifndef fribidi_malloc
# if HAVE_STDLIB_H
#  ifndef __FRIBIDI_DOC
#   include <stdlib.h>
#  endif /* __FRIBIDI_DOC */
#  define fribidi_malloc malloc
# else /* !HAVE_STDLIB_H */
#  define fribidi_malloc (void *) malloc
# endif	/* !HAVE_STDLIB_H */
# define fribidi_free free
#else /* fribidi_malloc */
# ifndef fribidi_free
#  error "You should define fribidi_free too when you define fribidi_malloc."
# endif	/* !fribidi_free */
#endif /* fribidi_malloc */

#ifdef HAVE_STRING_H
# if !STDC_HEADERS && HAVE_MEMORY_H
#  include <memory.h>
# endif
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

/* FRIBIDI_BEGIN_STMT should be used at the beginning of your macro
 * definitions that are to behave like simple statements.  Use
 * FRIBIDI_END_STMT at the end of the macro after the semicolon or brace. */
#ifndef FRIBIDI_BEGIN_STMT
# define FRIBIDI_BEGIN_STMT do {
# define FRIBIDI_END_STMT } while (0)
#endif /* !FRIBIDI_BEGIN_STMT */

/* LIKEYLY and UNLIKELY are used to give a hint on branch prediction to the
 * compiler. */
#ifndef LIKELY
# if defined(__GNUC__) && (__GNUC__ > 2) && defined(__OPTIMIZE__)
#  define FRIBIDI_BOOLEAN_EXPR(expr)              \
   __extension__ ({                               \
     int fribidi_bool_var;                        \
     if (expr)                                    \
        fribidi_bool_var = 1;                     \
     else                                         \
        fribidi_bool_var = 0;                     \
     fribidi_bool_var;                            \
   })
#  define LIKELY(expr) (__builtin_expect (FRIBIDI_BOOLEAN_EXPR(expr), 1))
#  define UNLIKELY(expr) (__builtin_expect (FRIBIDI_BOOLEAN_EXPR(expr), 0))
# else
#  define LIKELY
#  define UNLIKELY
# endif /* _GNUC_ */
#endif /* !LIKELY */

#ifndef FRIBIDI_EMPTY_STMT
# define FRIBIDI_EMPTY_STMT FRIBIDI_BEGIN_STMT (void) 0; FRIBIDI_END_STMT
#endif /* !FRIBIDI_EMPTY_STMT */

#ifdef HAVE_STRINGIZE
# define STRINGIZE(symbol) #symbol
#else /* !HAVE_STRINGIZE */
#  error "No stringize operator available?"
#endif /* !HAVE_STRINGIZE */

/* As per recommendation of GNU Coding Standards. */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif /* !_GNU_SOURCE */

/* We respect our own rules. */
#ifndef FRIBIDI_NO_DEPRECATED
#  define FRIBIDI_NO_DEPRECATED
#endif /* !FRIBIDI_NO_DEPRECATED */


#include "debug.h"

#endif /* !_COMMON_H */
/* Editor directions:
 * vim:textwidth=78:tabstop=8:shiftwidth=2:autoindent:cindent
 */
