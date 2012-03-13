#ifndef ONCE_FP_MPFR_FLOAT_
#define ONCE_FP_MPFR_FLOAT_

#include <iostream>

class MpfrFloat
{
 public:
    /* A default of 256 bits will be used unless changed with this function.
       Note that all existing and cached GMP objects will be resized to the
       specified precision (which can be a somewhat heavy operation).
    */
    static void setDefaultMantissaBits(unsigned long bits);

    static unsigned long getCurrentDefaultMantissaBits();

    /* The default constructor initializes the object to the value 0.
       It's efficient to instantiate such zero-initialized objects because
       all of them will share the same mpfr data. (Also any object initialized
       with or assigned the explicit value of zero will also share that one
       mpfr data.) Thus multiple zero-initialized MpfrFloat instances won't
       consume significant amounts of memory (until they are modified to
       contain some other value, of course).

       Important caveat:
       ----------------
       Note that initializing an MpfrFloat object with, for example, 0.1 will
       suffer from accuracy problems (at least if the MpfrFloat object has
       more mantissa bits than a double). The C++ double value 0.1 has only
       53 mantissa bits, while the MpfrFloat object usually has more. If the
       MpfrFloat object is initialized with a double, only that many bits of
       accuracy will end up in the value of the MpfrFloat object. This can
       create significant rounding/accuracy problems in some cases.
       If you need to initialize the MpfrObject with some value (which cannot
       be represented accurately by base-2 floating point numbers, eg. 0.1)
       at full mantissa precision, you have to use parseValue("0.1") instead,
       rather than relying on the constructor taking a double type value.
    */
    MpfrFloat();
    MpfrFloat(double value);
    MpfrFloat(long double value);
    MpfrFloat(long value);
    MpfrFloat(int value);
    MpfrFloat(const char* value, char** endptr);

    ~MpfrFloat();

    MpfrFloat(const MpfrFloat&);

    MpfrFloat& operator=(const MpfrFloat&);
    MpfrFloat& operator=(double value);
    MpfrFloat& operator=(long double value);
    MpfrFloat& operator=(long value);
    MpfrFloat& operator=(int value);
    //MpfrFloat& operator=(const char* value);

    void parseValue(const char* value);
    void parseValue(const char* value, char** endptr);


    /* This function can be used to retrieve the raw mpfr_t data structure
       used by this object. (The template trick is used to avoid a dependency
       of this header file with <mpfr.h>.)
       In other words, it can be called like:

         mpfr_t raw_mpfr_data;
         floatValue.get_raw_mpfr_data(raw_mpfr_data);

       Note that the returned mpfr_t should be considered as read-only and
       not be modified from the outside because it may be shared among
       several objects. If the calling code needs to modify the data, it
       should copy it for itself first with the appropriate MPFR library
       functions.
     */
    template<typename Mpfr_t>
    void get_raw_mpfr_data(Mpfr_t& dest_mpfr_t);


    /* Note that the returned char* points to an internal (shared) buffer
       which will be valid until the next time this function is called
       (by any object).
    */
    const char* getAsString(unsigned precision) const;

    bool isInteger() const;
    long toInt() const;
    double toDouble() const;

    MpfrFloat& operator+=(const MpfrFloat&);
    MpfrFloat& operator+=(double);
    MpfrFloat& operator-=(const MpfrFloat&);
    MpfrFloat& operator-=(double);
    MpfrFloat& operator*=(const MpfrFloat&);
    MpfrFloat& operator*=(double);
    MpfrFloat& operator/=(const MpfrFloat&);
    MpfrFloat& operator/=(double);
    MpfrFloat& operator%=(const MpfrFloat&);

    void negate();
    void abs();

    MpfrFloat operator+(const MpfrFloat&) const;
    MpfrFloat operator+(double) const;
    MpfrFloat operator-(const MpfrFloat&) const;
    MpfrFloat operator-(double) const;
    MpfrFloat operator*(const MpfrFloat&) const;
    MpfrFloat operator*(double) const;
    MpfrFloat operator/(const MpfrFloat&) const;
    MpfrFloat operator/(double) const;
    MpfrFloat operator%(const MpfrFloat&) const;

    MpfrFloat operator-() const;

    bool operator<(const MpfrFloat&) const;
    bool operator<(double) const;
    bool operator<=(const MpfrFloat&) const;
    bool operator<=(double) const;
    bool operator>(const MpfrFloat&) const;
    bool operator>(double) const;
    bool operator>=(const MpfrFloat&) const;
    bool operator>=(double) const;
    bool operator==(const MpfrFloat&) const;
    bool operator==(double) const;
    bool operator!=(const MpfrFloat&) const;
    bool operator!=(double) const;

    static MpfrFloat log(const MpfrFloat&);
    static MpfrFloat log2(const MpfrFloat&);
    static MpfrFloat log10(const MpfrFloat&);
    static MpfrFloat exp(const MpfrFloat&);
    static MpfrFloat exp2(const MpfrFloat&);
    static MpfrFloat exp10(const MpfrFloat&);
    static MpfrFloat cos(const MpfrFloat&);
    static MpfrFloat sin(const MpfrFloat&);
    static MpfrFloat tan(const MpfrFloat&);
    static MpfrFloat sec(const MpfrFloat&);
    static MpfrFloat csc(const MpfrFloat&);
    static MpfrFloat cot(const MpfrFloat&);
    static void sincos(const MpfrFloat&, MpfrFloat& sin, MpfrFloat& cos);
    static MpfrFloat acos(const MpfrFloat&);
    static MpfrFloat asin(const MpfrFloat&);
    static MpfrFloat atan(const MpfrFloat&);
    static MpfrFloat atan2(const MpfrFloat&, const MpfrFloat&);
    static MpfrFloat hypot(const MpfrFloat&, const MpfrFloat&);
    static MpfrFloat cosh(const MpfrFloat&);
    static MpfrFloat sinh(const MpfrFloat&);
    static MpfrFloat tanh(const MpfrFloat&);
    static MpfrFloat acosh(const MpfrFloat&);
    static MpfrFloat asinh(const MpfrFloat&);
    static MpfrFloat atanh(const MpfrFloat&);
    static MpfrFloat sqrt(const MpfrFloat&);
    static MpfrFloat cbrt(const MpfrFloat&);
    static MpfrFloat root(const MpfrFloat&, unsigned long root);
    static MpfrFloat pow(const MpfrFloat&, const MpfrFloat&);
    static MpfrFloat pow(const MpfrFloat&, long exponent);
    static MpfrFloat abs(const MpfrFloat&);
    static MpfrFloat dim(const MpfrFloat&, const MpfrFloat&);
    static MpfrFloat round(const MpfrFloat&);
    static MpfrFloat ceil(const MpfrFloat&);
    static MpfrFloat floor(const MpfrFloat&);
    static MpfrFloat trunc(const MpfrFloat&);

    static MpfrFloat parseString(const char* str, char** endptr);

    // These values are cached (and recalculated every time the mantissa bits
    // change), so it's efficient to call these repeatedly:
    static MpfrFloat const_pi();
    static MpfrFloat const_e();
    static MpfrFloat const_log2();
    static MpfrFloat someEpsilon();


 private:
    struct MpfrFloatData;
    class MpfrFloatDataContainer;

    MpfrFloatData* mData;

    enum DummyType { kNoInitialization };
    MpfrFloat(DummyType);
    MpfrFloat(MpfrFloatData*);

    void copyIfShared();
    static MpfrFloatDataContainer& mpfrFloatDataContainer();

    friend MpfrFloat operator+(double lhs, const MpfrFloat& rhs);
    friend MpfrFloat operator-(double lhs, const MpfrFloat& rhs);
};

MpfrFloat operator+(double lhs, const MpfrFloat& rhs);
MpfrFloat operator-(double lhs, const MpfrFloat& rhs);
MpfrFloat operator*(double lhs, const MpfrFloat& rhs);
MpfrFloat operator/(double lhs, const MpfrFloat& rhs);
MpfrFloat operator%(double lhs, const MpfrFloat& rhs);

inline bool operator<(double lhs, const MpfrFloat& rhs) { return rhs > lhs; }
inline bool operator<=(double lhs, const MpfrFloat& rhs) { return rhs >= lhs; }
inline bool operator>(double lhs, const MpfrFloat& rhs) { return rhs < lhs; }
inline bool operator>=(double lhs, const MpfrFloat& rhs) { return rhs <= lhs; }
inline bool operator==(double lhs, const MpfrFloat& rhs) { return rhs == lhs; }
inline bool operator!=(double lhs, const MpfrFloat& rhs) { return rhs != lhs; }

// This function takes into account the value of os.precision()
std::ostream& operator<<(std::ostream& os, const MpfrFloat& value);

#endif
