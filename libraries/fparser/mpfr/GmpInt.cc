#include "GmpInt.hh"
#include <gmp.h>
#include <deque>
#include <vector>
#include <cstring>
#include <cctype>

//===========================================================================
// Shared data
//===========================================================================
namespace
{
    unsigned long gIntDefaultNumberOfBits = 256;

    std::vector<char>& intString()
    {
        static std::vector<char> str;
        return str;
    }
}

//===========================================================================
// Auxiliary structs
//===========================================================================
struct GmpInt::GmpIntData
{
    unsigned mRefCount;
    GmpIntData* nextFreeNode;
    mpz_t mInteger;

    GmpIntData(): mRefCount(1), nextFreeNode(0) {}
};

class GmpInt::GmpIntDataContainer
{
    std::deque<GmpInt::GmpIntData> mData;
    GmpInt::GmpIntData* mFirstFreeNode;
    GmpInt::GmpIntData* mConst_0;

 public:
    GmpIntDataContainer(): mFirstFreeNode(0), mConst_0(0) {}

    ~GmpIntDataContainer()
    {
        for(size_t i = 0; i < mData.size(); ++i)
            mpz_clear(mData[i].mInteger);
    }

    GmpInt::GmpIntData* allocateGmpIntData(unsigned long numberOfBits,
                                           bool initToZero)
    {
        if(mFirstFreeNode)
        {
            GmpInt::GmpIntData* node = mFirstFreeNode;
            mFirstFreeNode = node->nextFreeNode;
            if(initToZero) mpz_set_si(node->mInteger, 0);
            ++(node->mRefCount);
            return node;
        }

        mData.push_back(GmpInt::GmpIntData());
        if(numberOfBits > 0)
            mpz_init2(mData.back().mInteger, numberOfBits);
        else
            mpz_init(mData.back().mInteger);
        return &mData.back();
    }

    void releaseGmpIntData(GmpIntData* data)
    {
        if(--(data->mRefCount) == 0)
        {
            data->nextFreeNode = mFirstFreeNode;
            mFirstFreeNode = data;
        }
    }

    GmpInt::GmpIntData* const_0()
    {
        if(!mConst_0)
            mConst_0 = allocateGmpIntData(gIntDefaultNumberOfBits, true);
        return mConst_0;
    }
};


GmpInt::GmpIntDataContainer& GmpInt::gmpIntDataContainer()
{
    static GmpIntDataContainer container;
    return container;
}

//===========================================================================
// Auxiliary functions
//===========================================================================
void GmpInt::setDefaultNumberOfBits(unsigned long value)
{
    gIntDefaultNumberOfBits = value;
}

unsigned long GmpInt::getDefaultNumberOfBits()
{
    return gIntDefaultNumberOfBits;
}

inline void GmpInt::copyIfShared()
{
    if(mData->mRefCount > 1)
    {
        --(mData->mRefCount);
        GmpIntData* oldData = mData;
        mData = gmpIntDataContainer().allocateGmpIntData(0, false);
        mpz_set(mData->mInteger, oldData->mInteger);
    }
}


//===========================================================================
// Constructors, destructor, assignment
//===========================================================================
GmpInt::GmpInt(DummyType):
    mData(gmpIntDataContainer().allocateGmpIntData(0, false))
{}

GmpInt::GmpInt()
{
    mData = gmpIntDataContainer().const_0();
    ++(mData->mRefCount);
}

GmpInt::GmpInt(long value)
{
    if(value == 0)
    {
        mData = gmpIntDataContainer().const_0();
        ++(mData->mRefCount);
    }
    else
    {
        mData = gmpIntDataContainer().allocateGmpIntData
            (gIntDefaultNumberOfBits, false);
        mpz_set_si(mData->mInteger, value);
    }
}

GmpInt::GmpInt(unsigned long value)
{
    if(value == 0)
    {
        mData = gmpIntDataContainer().const_0();
        ++(mData->mRefCount);
    }
    else
    {
        mData = gmpIntDataContainer().allocateGmpIntData
            (gIntDefaultNumberOfBits, false);
        mpz_set_ui(mData->mInteger, value);
    }
}

GmpInt::GmpInt(int value)
{
    if(value == 0)
    {
        mData = gmpIntDataContainer().const_0();
        ++(mData->mRefCount);
    }
    else
    {
        mData = gmpIntDataContainer().allocateGmpIntData
            (gIntDefaultNumberOfBits, false);
        mpz_set_si(mData->mInteger, value);
    }
}

GmpInt::GmpInt(double value)
{
    const double absValue = value >= 0.0 ? value : -value;
    if(absValue < 1.0)
    {
        mData = gmpIntDataContainer().const_0();
        ++(mData->mRefCount);
    }
    else
    {
        mData = gmpIntDataContainer().allocateGmpIntData
            (gIntDefaultNumberOfBits, false);
        mpz_set_d(mData->mInteger, value);
    }
}

GmpInt::GmpInt(long double value)
{
    const long double absValue = value >= 0.0L ? value : -value;
    if(absValue < 1.0L)
    {
        mData = gmpIntDataContainer().const_0();
        ++(mData->mRefCount);
    }
    else
    {
        mData = gmpIntDataContainer().allocateGmpIntData
            (gIntDefaultNumberOfBits, false);
        mpz_set_d(mData->mInteger, double(value));
    }
}

GmpInt::GmpInt(const GmpInt& rhs):
    mData(rhs.mData)
{
    ++(mData->mRefCount);
}

GmpInt& GmpInt::operator=(const GmpInt& rhs)
{
    if(mData != rhs.mData)
    {
        gmpIntDataContainer().releaseGmpIntData(mData);
        mData = rhs.mData;
        ++(mData->mRefCount);
    }
    return *this;
}

GmpInt& GmpInt::operator=(signed long value)
{
    if(value == 0)
    {
        gmpIntDataContainer().releaseGmpIntData(mData);
        mData = gmpIntDataContainer().const_0();
        ++(mData->mRefCount);
    }
    else
    {
        if(mData->mRefCount > 1)
        {
            --(mData->mRefCount);
            mData = gmpIntDataContainer().allocateGmpIntData
                (gIntDefaultNumberOfBits, false);
        }
        mpz_set_si(mData->mInteger, value);
    }
    return *this;
}

GmpInt::~GmpInt()
{
    gmpIntDataContainer().releaseGmpIntData(mData);
}


//===========================================================================
// Data getters
//===========================================================================
template<>
void GmpInt::get_raw_mpfr_data<mpz_t>(mpz_t& dest_mpz_t)
{
    std::memcpy(&dest_mpz_t, mData->mInteger, sizeof(mpz_t));
}

const char* GmpInt::getAsString(int base) const
{
    intString().resize(mpz_sizeinbase(mData->mInteger, base) + 2);
    return mpz_get_str(&intString()[0], base, mData->mInteger);
}

long GmpInt::toInt() const
{
    return mpz_get_si(mData->mInteger);
}


//===========================================================================
// Modifying operators
//===========================================================================
GmpInt& GmpInt::operator+=(const GmpInt& rhs)
{
    copyIfShared();
    mpz_add(mData->mInteger, mData->mInteger, rhs.mData->mInteger);
    return *this;
}

GmpInt& GmpInt::operator+=(long value)
{
    copyIfShared();
    if(value >= 0)
        mpz_add_ui(mData->mInteger, mData->mInteger, value);
    else
        mpz_sub_ui(mData->mInteger, mData->mInteger, -value);
    return *this;
}

GmpInt& GmpInt::operator-=(const GmpInt& rhs)
{
    copyIfShared();
    mpz_sub(mData->mInteger, mData->mInteger, rhs.mData->mInteger);
    return *this;
}

GmpInt& GmpInt::operator-=(long value)
{
    copyIfShared();
    if(value >= 0)
        mpz_sub_ui(mData->mInteger, mData->mInteger, value);
    else
        mpz_add_ui(mData->mInteger, mData->mInteger, -value);
    return *this;
}

GmpInt& GmpInt::operator*=(const GmpInt& rhs)
{
    copyIfShared();
    mpz_mul(mData->mInteger, mData->mInteger, rhs.mData->mInteger);
    return *this;
}

GmpInt& GmpInt::operator*=(long value)
{
    copyIfShared();
    mpz_mul_si(mData->mInteger, mData->mInteger, value);
    return *this;
}

GmpInt& GmpInt::operator/=(const GmpInt& rhs)
{
    copyIfShared();
    mpz_tdiv_q(mData->mInteger, mData->mInteger, rhs.mData->mInteger);
    return *this;
}

GmpInt& GmpInt::operator/=(long value)
{
    copyIfShared();
    if(value >= 0)
        mpz_tdiv_q_ui(mData->mInteger, mData->mInteger, value);
    else
    {
        mpz_neg(mData->mInteger, mData->mInteger);
        mpz_tdiv_q_ui(mData->mInteger, mData->mInteger, -value);
    }
    return *this;
}

GmpInt& GmpInt::operator%=(const GmpInt& rhs)
{
    copyIfShared();
    if(operator<(0))
    {
        negate();
        mpz_mod(mData->mInteger, mData->mInteger, rhs.mData->mInteger);
        negate();
    }
    else
    {
        mpz_mod(mData->mInteger, mData->mInteger, rhs.mData->mInteger);
    }
    return *this;
}

GmpInt& GmpInt::operator%=(long value)
{
    copyIfShared();
    if(value < 0) value = -value;
    if(operator<(0))
    {
        negate();
        mpz_mod_ui(mData->mInteger, mData->mInteger, value);
        negate();
    }
    else
    {
        mpz_mod_ui(mData->mInteger, mData->mInteger, value);
    }
    return *this;
}

GmpInt& GmpInt::operator<<=(unsigned long bits)
{
    copyIfShared();
    mpz_mul_2exp(mData->mInteger, mData->mInteger, bits);
    return *this;
}

GmpInt& GmpInt::operator>>=(unsigned long bits)
{
    copyIfShared();
    mpz_tdiv_q_2exp(mData->mInteger, mData->mInteger, bits);
    return *this;
}


//===========================================================================
// Modifying functions
//===========================================================================
void GmpInt::addProduct(const GmpInt& value1, const GmpInt& value2)
{
    copyIfShared();
    mpz_addmul(mData->mInteger, value1.mData->mInteger, value2.mData->mInteger);
}

void GmpInt::addProduct(const GmpInt& value1, unsigned long value2)
{
    copyIfShared();
    mpz_addmul_ui(mData->mInteger, value1.mData->mInteger, value2);
}

void GmpInt::subProduct(const GmpInt& value1, const GmpInt& value2)
{
    copyIfShared();
    mpz_submul(mData->mInteger, value1.mData->mInteger, value2.mData->mInteger);
}

void GmpInt::subProduct(const GmpInt& value1, unsigned long value2)
{
    copyIfShared();
    mpz_submul_ui(mData->mInteger, value1.mData->mInteger, value2);
}

void GmpInt::negate()
{
    copyIfShared();
    mpz_neg(mData->mInteger, mData->mInteger);
}

void GmpInt::abs()
{
    copyIfShared();
    mpz_abs(mData->mInteger, mData->mInteger);
}

GmpInt GmpInt::abs(const GmpInt& value)
{
    GmpInt retval(kNoInitialization);
    mpz_abs(retval.mData->mInteger, value.mData->mInteger);
    return retval;
}


//===========================================================================
// Non-modifying operators
//===========================================================================
GmpInt GmpInt::operator+(const GmpInt& rhs) const
{
    GmpInt retval(kNoInitialization);
    mpz_add(retval.mData->mInteger, mData->mInteger, rhs.mData->mInteger);
    return retval;
}

GmpInt GmpInt::operator+(long value) const
{
    GmpInt retval(kNoInitialization);
    if(value >= 0)
        mpz_add_ui(retval.mData->mInteger, mData->mInteger, value);
    else
        mpz_sub_ui(retval.mData->mInteger, mData->mInteger, -value);
    return retval;
}

GmpInt GmpInt::operator-(const GmpInt& rhs) const
{
    GmpInt retval(kNoInitialization);
    mpz_sub(retval.mData->mInteger, mData->mInteger, rhs.mData->mInteger);
    return retval;
}

GmpInt GmpInt::operator-(long value) const
{
    GmpInt retval(kNoInitialization);
    if(value >= 0)
        mpz_sub_ui(retval.mData->mInteger, mData->mInteger, value);
    else
        mpz_add_ui(retval.mData->mInteger, mData->mInteger, -value);
    return retval;
}

GmpInt GmpInt::operator*(const GmpInt& rhs) const
{
    GmpInt retval(kNoInitialization);
    mpz_mul(retval.mData->mInteger, mData->mInteger, rhs.mData->mInteger);
    return retval;
}

GmpInt GmpInt::operator*(long value) const
{
    GmpInt retval(kNoInitialization);
    mpz_mul_si(retval.mData->mInteger, mData->mInteger, value);
    return retval;
}

GmpInt GmpInt::operator/(const GmpInt& rhs) const
{
    GmpInt retval(kNoInitialization);
    mpz_tdiv_q(retval.mData->mInteger, mData->mInteger, rhs.mData->mInteger);
    return retval;
}

GmpInt GmpInt::operator/(long value) const
{
    GmpInt retval(kNoInitialization);
    if(value >= 0)
        mpz_tdiv_q_ui(retval.mData->mInteger, mData->mInteger, value);
    else
    {
        mpz_neg(retval.mData->mInteger, mData->mInteger);
        mpz_tdiv_q_ui(retval.mData->mInteger, retval.mData->mInteger, -value);
    }
    return retval;
}

GmpInt GmpInt::operator%(const GmpInt& rhs) const
{
    GmpInt retval(kNoInitialization);
    if(operator<(0))
    {
        mpz_neg(retval.mData->mInteger, mData->mInteger);
        mpz_mod(retval.mData->mInteger,
                retval.mData->mInteger, rhs.mData->mInteger);
        retval.negate();
    }
    else
    {
        mpz_mod(retval.mData->mInteger, mData->mInteger, rhs.mData->mInteger);
    }
    return retval;
}

GmpInt GmpInt::operator%(long value) const
{
    GmpInt retval(kNoInitialization);
    if(value < 0) value = -value;
    if(operator<(0))
    {
        mpz_neg(retval.mData->mInteger, mData->mInteger);
        mpz_mod_ui(retval.mData->mInteger, retval.mData->mInteger, value);
        retval.negate();
    }
    else
    {
        mpz_mod_ui(retval.mData->mInteger, mData->mInteger, value);
    }
    return retval;
}

GmpInt GmpInt::operator-() const
{
    GmpInt retval(kNoInitialization);
    mpz_neg(retval.mData->mInteger, mData->mInteger);
    return retval;
}

GmpInt GmpInt::operator<<(unsigned long bits) const
{
    GmpInt retval(kNoInitialization);
    mpz_mul_2exp(retval.mData->mInteger, mData->mInteger, bits);
    return retval;
}

GmpInt GmpInt::operator>>(unsigned long bits) const
{
    GmpInt retval(kNoInitialization);
    mpz_tdiv_q_2exp(retval.mData->mInteger, mData->mInteger, bits);
    return retval;
}


//===========================================================================
// Comparison operators
//===========================================================================
bool GmpInt::operator<(const GmpInt& rhs) const
{
    return mpz_cmp(mData->mInteger, rhs.mData->mInteger) < 0;
}

bool GmpInt::operator<(long value) const
{
    return mpz_cmp_si(mData->mInteger, value) < 0;
}

bool GmpInt::operator<=(const GmpInt& rhs) const
{
    return mpz_cmp(mData->mInteger, rhs.mData->mInteger) <= 0;
}

bool GmpInt::operator<=(long value) const
{
    return mpz_cmp_si(mData->mInteger, value) <= 0;
}

bool GmpInt::operator>(const GmpInt& rhs) const
{
    return mpz_cmp(mData->mInteger, rhs.mData->mInteger) > 0;
}

bool GmpInt::operator>(long value) const
{
    return mpz_cmp_si(mData->mInteger, value) > 0;
}

bool GmpInt::operator>=(const GmpInt& rhs) const
{
    return mpz_cmp(mData->mInteger, rhs.mData->mInteger) >= 0;
}

bool GmpInt::operator>=(long value) const
{
    return mpz_cmp_si(mData->mInteger, value) >= 0;
}

bool GmpInt::operator==(const GmpInt& rhs) const
{
    return mpz_cmp(mData->mInteger, rhs.mData->mInteger) == 0;
}

bool GmpInt::operator==(long value) const
{
    return mpz_cmp_si(mData->mInteger, value) == 0;
}

bool GmpInt::operator!=(const GmpInt& rhs) const
{
    return mpz_cmp(mData->mInteger, rhs.mData->mInteger) != 0;
}

bool GmpInt::operator!=(long value) const
{
    return mpz_cmp_si(mData->mInteger, value) != 0;
}

void GmpInt::parseValue(const char* value)
{
    mpz_set_str(mData->mInteger, value, 10);
}

void GmpInt::parseValue(const char* value, char** endptr)
{
    static std::vector<char> str;

    unsigned startIndex = 0;
    while(value[startIndex] && std::isspace(value[startIndex])) ++startIndex;
    if(!value[startIndex]) { *endptr = const_cast<char*>(value); return; }

    unsigned endIndex = startIndex;
    if(value[endIndex] == '-') ++endIndex;
    if(!std::isdigit(value[endIndex]))
    { *endptr = const_cast<char*>(value); return; }
    if(value[endIndex] == '0' && value[endIndex+1] == 'x')
    {
        endIndex += 1;
        while(std::isxdigit(value[++endIndex])) {}
    }
    else
    {
        while(std::isdigit(value[++endIndex])) {}
    }

    str.reserve(endIndex - startIndex + 1);
    str.assign(value + startIndex, value + endIndex);
    str.push_back(0);

    mpz_set_str(mData->mInteger, &str[0], 0);
    *endptr = const_cast<char*>(value + endIndex);
}

GmpInt GmpInt::parseString(const char* str, char** endptr)
{
    GmpInt retval(kNoInitialization);
    retval.parseValue(str, endptr);
    return retval;
}

//===========================================================================
// Operator functions
//===========================================================================
GmpInt operator+(long lhs, const GmpInt& rhs)
{
    GmpInt retval(GmpInt::kNoInitialization);
    if(lhs >= 0)
        mpz_add_ui(retval.mData->mInteger, rhs.mData->mInteger, lhs);
    else
        mpz_sub_ui(retval.mData->mInteger, rhs.mData->mInteger, -lhs);
    return retval;
}

GmpInt operator-(long lhs, const GmpInt& rhs)
{
    GmpInt retval(GmpInt::kNoInitialization);
    if(lhs >= 0)
        mpz_ui_sub(retval.mData->mInteger, lhs, rhs.mData->mInteger);
    else
    {
        mpz_add_ui(retval.mData->mInteger, rhs.mData->mInteger, -lhs);
        mpz_neg(retval.mData->mInteger, retval.mData->mInteger);
    }
    return retval;
}

GmpInt operator*(long lhs, const GmpInt& rhs)
{
    return rhs * lhs;
}

GmpInt operator/(long lhs, const GmpInt& rhs)
{
    return GmpInt(lhs) / rhs;
}

GmpInt operator%(long lhs, const GmpInt& rhs)
{
    return GmpInt(lhs) % rhs;
}
