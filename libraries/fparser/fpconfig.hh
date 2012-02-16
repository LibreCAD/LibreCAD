/***************************************************************************\
|* Function Parser for C++ v4.3                                            *|
|*-------------------------------------------------------------------------*|
|* Copyright: Juha Nieminen                                                *|
|*                                                                         *|
|* This library is distributed under the terms of the                      *|
|* GNU Lesser General Public License version 3.                            *|
|* (See lgpl.txt and gpl.txt for the license text.)                        *|
\***************************************************************************/

// Configuration file
// ------------------

/* NOTE:
   This file is for the internal use of the function parser only.
   You don't need to include this file in your source files, just
   include "fparser.hh".
*/


/* Uncomment any of these lines or define them in your compiler settings
   to enable the correspondent version of the parser. (These are disabled
   by default because they rely on C99 functions, and non-standard libraries
   in the case pf MPFR and GMP, and they make compiling needlessly slower
   and the resulting binary needlessly larger if they are not used in the
   program.)
*/
//#define FP_SUPPORT_FLOAT_TYPE
//#define FP_SUPPORT_LONG_DOUBLE_TYPE
//#define FP_SUPPORT_LONG_INT_TYPE
//#define FP_SUPPORT_MPFR_FLOAT_TYPE
//#define FP_SUPPORT_GMP_INT_TYPE

/* Uncomment this line of define it in your compiler settings if you want
   to disable compiling the basic double version of the library, in case
   one of the above types is used but not the double type. (If the double
   type is not used, then disabling it makes compiling faster and the
   resulting binary smaller.)
 */
//#define FP_DISABLE_DOUBLE_TYPE

/*
 Note that these do not change what FunctionParser supports, they only
 change how the function is evaluated, potentially making it faster when
 these functions are involved.
 These will make the source code use asinh(), acosh(), atanh(), exp2()
 and log2().
*/
//#define FP_SUPPORT_TR1_MATH_FUNCS

#ifdef FP_SUPPORT_TR1_MATH_FUNCS
#define FP_SUPPORT_ASINH
#define FP_SUPPORT_EXP2
#define FP_SUPPORT_LOG2
#define FP_SUPPORT_CBRT
#define FP_SUPPORT_HYPOT
#endif

/*
 Comment out the following line to enable the eval() function, which can
 be used in the function string to recursively call the same function.
 Note that enabling this function may be dangerous even if the maximum
 recursion level is limited because it is still possible to write functions
 using it which take enormous  amounts of time to evaluate even though the
 maximum recursion is never reached. This may be undesirable in some
 applications.
 Alternatively you can define the FP_ENABLE_EVAL precompiler constant in
 your compiler settings.
*/
#ifndef FP_ENABLE_EVAL
#define FP_DISABLE_EVAL
#endif


/*
 Maximum recursion level for eval() calls:
*/
#define FP_EVAL_MAX_REC_LEVEL 1000


/*
 Whether to use shortcut evaluation for the & and | operators:
*/
#ifndef FP_DISABLE_SHORTCUT_LOGICAL_EVALUATION
#define FP_ENABLE_SHORTCUT_LOGICAL_EVALUATION
#endif

/*
 Whether to enable optimizations that may ignore side effects
 of if() calls, such as changing if(x,!y,0) into x&!y.
 This is basically the polar opposite of "shortcut logical evaluation".
 Disabled by default, because it makes eval() rather unsafe.
*/
#ifdef FP_ENABLE_IGNORE_IF_SIDEEFFECTS
#endif

/*
 Comment out the following lines out if you are not going to use the
 optimizer and want a slightly smaller library. The Optimize() method
 can still be called, but it will not do anything.
 If you are unsure, just leave it. It won't slow down the other parts of
 the library.
*/
#ifndef FP_NO_SUPPORT_OPTIMIZER
#define FP_SUPPORT_OPTIMIZER
#endif


/*
 Epsilon value used with the comparison operators (must be non-negative):
 (Comment it out if you don't want to use epsilon in comparisons. Might
 lead to marginally faster evaluation of the comparison operators, but
 can introduce inaccuracies in comparisons.)
*/
#define FP_EPSILON 1e-14


/*
 No member function of FunctionParser is thread-safe. Most prominently,
 Eval() is not thread-safe. By uncommenting one of these lines the Eval()
 function can be made thread-safe at the cost of a possible small overhead.
 The second version requires that the compiler supports the alloca() function,
 which is not standard, but is faster.
 */
//#define FP_USE_THREAD_SAFE_EVAL
//#define FP_USE_THREAD_SAFE_EVAL_WITH_ALLOCA

/*
 Uncomment (or define in your compiler options) to disable evaluation checks.
 (Consult the documentation for details.)
 */
//#define FP_NO_EVALUATION_CHECKS
