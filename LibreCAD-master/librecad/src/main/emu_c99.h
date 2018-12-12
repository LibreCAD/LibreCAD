/*
 * Workaround: Emulate some C99 <math.h> functions.
 */

#ifdef EMU_C99
#include <cassert>

long int lrint(double x);
double round(double x);
double remainder(double x, double y);

#define isfinite(x) \
    ((sizeof (x) == sizeof (float)) ? _finite((double)(x)) : \
     (sizeof (x) == sizeof (double)) ? _finite(x) : \
      (assert(0),-1))

#define isinf(x) \
    ((sizeof (x) == sizeof (float)) ? __isinfd((double)(x)) : \
     (sizeof (x) == sizeof (double)) ? __isinfd(x) : \
      (assert(0),-1))

#define isnan(x) \
    ((sizeof (x) == sizeof (float)) ? __isnand((double)(x)) : \
     (sizeof (x) == sizeof (double)) ? __isnand(x) : \
      (assert(0),-1))

#define isnormal(x) \
    ((sizeof (x) == sizeof (float)) ? __isnormald((double)(x)) : \
     (sizeof (x) == sizeof (double)) ? __isnormald(x) : \
      (assert(0),-1))

int __isinfd(double);
int __isnand(double);
int __isnormald(double);

#endif
