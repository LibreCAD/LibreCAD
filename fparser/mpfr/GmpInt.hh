#ifndef ONCE_FP_GMP_INT_HH_
#define ONCE_FP_GMP_INT_HH_

#include <iostream>

class GmpInt
{
 public:
    /* A default of 256 bits will be used for all newly-instantiated GmpInt
       objects. This default can be changed with the function below.
    */
    static void setDefaultNumberOfBits(unsigned long);
    static unsigned long getDefaultNumberOfBits();

    GmpInt();
    GmpInt(long value);
    GmpInt(unsigned long value);
    GmpInt(int value);
    GmpInt(double value);
    GmpInt(long double value);

    GmpInt(const GmpInt&);
    GmpInt& operator=(const GmpInt&);
    GmpInt& operator=(signed long value);

    ~GmpInt();


    /* This function can be used to retrieve the raw mpz_t data structure
       used by this object. (The template trick is used to avoid a dependency
       of this header file with <gmp.h>.)
       In other words, it can be called like:

         mpz_t raw_mpz_data;
         intValue.get_raw_mpz_data(raw_mpz_data);

       Note that the returned mpz_t should be considered as read-only and
       not be modified from the outside because it may be shared among
       several objects. If the calling code needs to modify the data, it
       should copy it for itself first with the appropriate GMP library
       functions.
     */
    template<typename Mpz_t>
    void get_raw_mpfr_data(Mpz_t& dest_mpz_t);


    // Note that the returned char* points to an internal (shared) buffer
    // which will be valid until the next time this function is called
    // (by any object).
    const char* getAsString(int base = 10) const;
    long toInt() const;

    GmpInt& operator+=(const GmpInt&);
    GmpInt& operator+=(long);
    GmpInt& operator-=(const GmpInt&);
    GmpInt& operator-=(long);
    GmpInt& operator*=(const GmpInt&);
    GmpInt& operator*=(long);
    GmpInt& operator/=(const GmpInt&);
    GmpInt& operator/=(long);
    GmpInt& operator%=(const GmpInt&);
    GmpInt& operator%=(long);

    GmpInt& operator<<=(unsigned long);
    GmpInt& operator>>=(unsigned long);

    // Equivalent to "+= value1 * value2;"
    void addProduct(const GmpInt& value1, const GmpInt& value2);
    void addProduct(const GmpInt& value1, unsigned long value2);

    // Equivalent to "-= value1 * value2;"
    void subProduct(const GmpInt& value1, const GmpInt& value2);
    void subProduct(const GmpInt& value1, unsigned long value2);

    void negate();
    void abs();
    static GmpInt abs(const GmpInt&);

    GmpInt operator+(const GmpInt&) const;
    GmpInt operator+(long) const;
    GmpInt operator-(const GmpInt&) const;
    GmpInt operator-(long) const;
    GmpInt operator*(const GmpInt&) const;
    GmpInt operator*(long) const;
    GmpInt operator/(const GmpInt&) const;
    GmpInt operator/(long) const;
    GmpInt operator%(const GmpInt&) const;
    GmpInt operator%(long) const;

    GmpInt operator-() const;

    GmpInt operator<<(unsigned long) const;
    GmpInt operator>>(unsigned long) const;

    bool operator<(const GmpInt&) const;
    bool operator<(long) const;
    bool operator<=(const GmpInt&) const;
    bool operator<=(long) const;
    bool operator>(const GmpInt&) const;
    bool operator>(long) const;
    bool operator>=(const GmpInt&) const;
    bool operator>=(long) const;
    bool operator==(const GmpInt&) const;
    bool operator==(long) const;
    bool operator!=(const GmpInt&) const;
    bool operator!=(long) const;

    void parseValue(const char* value);
    void parseValue(const char* value, char** endptr);
    static GmpInt parseString(const char* str, char** endptr);


 private:
    struct GmpIntData;
    class GmpIntDataContainer;

    GmpIntData* mData;

    enum DummyType { kNoInitialization };
    GmpInt(DummyType);

    void copyIfShared();
    static GmpIntDataContainer& gmpIntDataContainer();

    friend GmpInt operator+(long lhs, const GmpInt& rhs);
    friend GmpInt operator-(long lhs, const GmpInt& rhs);
};

GmpInt operator+(long lhs, const GmpInt& rhs);
GmpInt operator-(long lhs, const GmpInt& rhs);
GmpInt operator*(long lhs, const GmpInt& rhs);
GmpInt operator/(long lhs, const GmpInt& rhs);
GmpInt operator%(long lhs, const GmpInt& rhs);

inline bool operator<(long lhs, const GmpInt& rhs) { return rhs > lhs; }
inline bool operator<=(long lhs, const GmpInt& rhs) { return rhs >= lhs; }
inline bool operator>(long lhs, const GmpInt& rhs) { return rhs < lhs; }
inline bool operator>=(long lhs, const GmpInt& rhs) { return rhs <= lhs; }
inline bool operator==(long lhs, const GmpInt& rhs) { return rhs == lhs; }
inline bool operator!=(long lhs, const GmpInt& rhs) { return rhs != lhs; }

inline std::ostream& operator<<(std::ostream& os, const GmpInt& value)
{
    os << value.getAsString();
    return os;
}

#endif
