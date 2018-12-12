#include "emu_c99.h"

#include <cmath>
#include <cfloat>

#ifdef EMU_C99

#if !defined(_MSC_VER) && !defined(_M_IX86) 
#error Don't know how to implement C99 math functions here ...
#else
#if defined(_WIN64)
#error Can't use inline assembler on Win 64 platform ...
#endif
#endif

/* Floating point classification functions. */

int __isinfd(double x)
{
    int c = _fpclass(x);
    return c == _FPCLASS_NINF || c == _FPCLASS_PINF;
}

int __isnand(double x)
{
    int c = _fpclass(x);
    return c == _FPCLASS_SNAN || c == _FPCLASS_QNAN;
}

int __isnormald(double x)
{
    int c = _fpclass(x);
    return c == _FPCLASS_NN || c == _FPCLASS_PN;
}

/* Quoted from ISO 9899:1999 (E):

7.12.9.5 The lrint and llrint functions

    Synopsis

1   #include <math.h>
    long int lrint(double x);
    long int lrintf(float x);
    long int lrintl(long double x);
    long long int llrint(double x);
    long long int llrintf(float x);
    long long int llrintl(long double x);

    Description

2   The lrint and llrint functions round their argument to the nearest 
    integer value, rounding according to the current rounding direction. 
    If the rounded value is outside the range of the return type, the 
    numeric result is unspecified. A range error may occur if the magni-
    tude of x is too large.

    Returns

3   The lrint and llrint functions return the rounded integer value.

*/

long int lrint(double x)
{
    long int i;

    __asm {
        fld   x
        fistp i
    }
    return i;
}

/* Quoted from ISO 9899:1999 (E):

    7.12.9.6 The round functions

    Synopsis

1   #include <math.h>
    double round(double x);
    float roundf(float x);
    long double roundl(long double x);

    Description

2   The round functions round their argument to the nearest integer value in floating-point
    format, rounding halfway cases away from zero, regardless of the current rounding
    direction.

    Returns

3   The round functions return the rounded integer value.
*/

double round(double x)
{
    if (x >= 0.0)
        return floor(x + 0.5);
    else
        return ceil(x - 0.5);
}

/* Quoted from ISO 9899:1999 (E):

    7.12.10.2 The remainder functions

    Synopsis

1   #include <math.h>
    double remainder(double x, double y);
    float remainderf(float x, float y);
    long double remainderl(long double x, long double y);

    Description

2   The remainder functions compute the remainder x REM y required 
    by IEC 60559.201)

    Returns

3   The remainder functions return x REM y.

Footnote:

201) "When y != 0, the remainder r = x REM y is defined regardless of the 
     rounding mode by the mathematical relation r = x - ny, where n is the 
     integer nearest the exact value of x/y; whenever | n - x/y | = 1/2, 
     then n is even. Thus, the remainder is always exact. If r = 0, its 
     sign shall be that of x." This definition is applicable for all 
     implementations.
*/

double remainder(double x, double y)
{
    double r;

    __asm {
            fld y
            fld x
    again:  fprem1           /* Partial IEEE 754 remainder of x/y. */
            fstsw  ax        /* Move status word into AX. */
            fwait            /* Wait till move completed. */
            sahf             /* Move status word into CPU flags. */
            jpe    again     /* Was partial if if C2=PF=1, repeat. */
            fstp   st(1)     /* Move result to TOS. */
            fstp   r         /* Store result and pop stack. */
    }
    return r;
}


#endif // EMU_C99
