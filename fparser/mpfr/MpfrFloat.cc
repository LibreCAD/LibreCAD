#include "MpfrFloat.hh"
#include <stdio.h>
#include <mpfr.h>
#include <deque>
#include <vector>
#include <cstring>
#include <cassert>

//===========================================================================
// Auxiliary structs
//===========================================================================
struct MpfrFloat::MpfrFloatData
{
    unsigned mRefCount;
    MpfrFloatData* nextFreeNode;
    mpfr_t mFloat;

    MpfrFloatData(): mRefCount(1), nextFreeNode(0) {}
};

class MpfrFloat::MpfrFloatDataContainer
{
    unsigned long mDefaultPrecision;
    std::deque<MpfrFloatData> mData;
    MpfrFloatData* mFirstFreeNode;

    MpfrFloatData
    *mConst_0, *mConst_pi, *mConst_e, *mConst_log2, *mConst_epsilon;

    void recalculateEpsilon()
    {
        mpfr_set_si(mConst_epsilon->mFloat, 1, GMP_RNDN);
        mpfr_div_2ui(mConst_epsilon->mFloat, mConst_epsilon->mFloat,
                     mDefaultPrecision*7/8 - 1, GMP_RNDN);
    }

 public:
    MpfrFloatDataContainer():
        mDefaultPrecision(256), mFirstFreeNode(0),
        mConst_pi(0), mConst_e(0), mConst_log2(0), mConst_epsilon(0)
    {}

    ~MpfrFloatDataContainer()
    {
        for(size_t i = 0; i < mData.size(); ++i)
            mpfr_clear(mData[i].mFloat);
    }

    MpfrFloatData* allocateMpfrFloatData(bool initToZero)
    {
        if(mFirstFreeNode)
        {
            MpfrFloatData* node = mFirstFreeNode;
            mFirstFreeNode = node->nextFreeNode;
            if(initToZero) mpfr_set_si(node->mFloat, 0, GMP_RNDN);
            ++(node->mRefCount);
            return node;
        }

        mData.push_back(MpfrFloatData());
        mpfr_init2(mData.back().mFloat, mDefaultPrecision);
        if(initToZero) mpfr_set_si(mData.back().mFloat, 0, GMP_RNDN);
        return &mData.back();
    }

    void releaseMpfrFloatData(MpfrFloatData* data)
    {
        if(--(data->mRefCount) == 0)
        {
            data->nextFreeNode = mFirstFreeNode;
            mFirstFreeNode = data;
        }
    }

    void setDefaultPrecision(unsigned long bits)
    {
        if(bits != mDefaultPrecision)
        {
            mDefaultPrecision = bits;
            for(size_t i = 0; i < mData.size(); ++i)
                mpfr_prec_round(mData[i].mFloat, bits, GMP_RNDN);

            if(mConst_pi) mpfr_const_pi(mConst_pi->mFloat, GMP_RNDN);
            if(mConst_e)
            {
                mpfr_set_si(mConst_e->mFloat, 1, GMP_RNDN);
                mpfr_exp(mConst_e->mFloat, mConst_e->mFloat, GMP_RNDN);
            }
            if(mConst_log2) mpfr_const_log2(mConst_log2->mFloat, GMP_RNDN);
            if(mConst_epsilon) recalculateEpsilon();
        }
    }

    unsigned long getDefaultPrecision() const
    {
        return mDefaultPrecision;
    }

    MpfrFloatData* const_0()
    {
        if(!mConst_0) mConst_0 = allocateMpfrFloatData(true);
        return mConst_0;
    }

    MpfrFloat const_pi()
    {
        if(!mConst_pi)
        {
            mConst_pi = allocateMpfrFloatData(false);
            mpfr_const_pi(mConst_pi->mFloat, GMP_RNDN);
        }
        return MpfrFloat(mConst_pi);
    }

    MpfrFloat const_e()
    {
        if(!mConst_e)
        {
            mConst_e = allocateMpfrFloatData(false);
            mpfr_set_si(mConst_e->mFloat, 1, GMP_RNDN);
            mpfr_exp(mConst_e->mFloat, mConst_e->mFloat, GMP_RNDN);
        }
        return MpfrFloat(mConst_e);
    }

    MpfrFloat const_log2()
    {
        if(!mConst_log2)
        {
            mConst_log2 = allocateMpfrFloatData(false);
            mpfr_const_log2(mConst_log2->mFloat, GMP_RNDN);
        }
        return MpfrFloat(mConst_log2);
    }

    MpfrFloat const_epsilon()
    {
        if(!mConst_epsilon)
        {
            mConst_epsilon = allocateMpfrFloatData(false);
            recalculateEpsilon();
        }
        return MpfrFloat(mConst_epsilon);
    }
};


//===========================================================================
// Shared data
//===========================================================================
// This should ensure that the container is not accessed by any MpfrFloat
// instance before it has been constructed or after it has been destroyed
// (which might otherwise happen if MpfrFloat is instantiated globally.)
MpfrFloat::MpfrFloatDataContainer& MpfrFloat::mpfrFloatDataContainer()
{
    static MpfrFloat::MpfrFloatDataContainer container;
    return container;
}


//===========================================================================
// Auxiliary functions
//===========================================================================
void MpfrFloat::setDefaultMantissaBits(unsigned long bits)
{
    mpfrFloatDataContainer().setDefaultPrecision(bits);
}

unsigned long MpfrFloat::getCurrentDefaultMantissaBits()
{
    return mpfrFloatDataContainer().getDefaultPrecision();
}

inline void MpfrFloat::copyIfShared()
{
    if(mData->mRefCount > 1)
    {
        --(mData->mRefCount);
        MpfrFloatData* oldData = mData;
        mData = mpfrFloatDataContainer().allocateMpfrFloatData(false);
        mpfr_set(mData->mFloat, oldData->mFloat, GMP_RNDN);
    }
}


//===========================================================================
// Constructors, destructor, assignment
//===========================================================================
MpfrFloat::MpfrFloat(DummyType):
    mData(mpfrFloatDataContainer().allocateMpfrFloatData(false))
{}

MpfrFloat::MpfrFloat(MpfrFloatData* data):
    mData(data)
{
    assert(data != 0);
    ++(mData->mRefCount);
}

MpfrFloat::MpfrFloat():
    mData(mpfrFloatDataContainer().const_0())
{
    ++(mData->mRefCount);
}

MpfrFloat::MpfrFloat(double value)
{
    if(value == 0.0)
    {
        mData = mpfrFloatDataContainer().const_0();
        ++(mData->mRefCount);
    }
    else
    {
        mData = mpfrFloatDataContainer().allocateMpfrFloatData(false);
        mpfr_set_d(mData->mFloat, value, GMP_RNDN);
    }
}

MpfrFloat::MpfrFloat(long double value)
{
    if(value == 0.0L)
    {
        mData = mpfrFloatDataContainer().const_0();
        ++(mData->mRefCount);
    }
    else
    {
        mData = mpfrFloatDataContainer().allocateMpfrFloatData(false);
        mpfr_set_ld(mData->mFloat, value, GMP_RNDN);
    }
}

MpfrFloat::MpfrFloat(long value)
{
    if(value == 0)
    {
        mData = mpfrFloatDataContainer().const_0();
        ++(mData->mRefCount);
    }
    else
    {
        mData = mpfrFloatDataContainer().allocateMpfrFloatData(false);
        mpfr_set_si(mData->mFloat, value, GMP_RNDN);
    }
}

MpfrFloat::MpfrFloat(int value)
{
    if(value == 0)
    {
        mData = mpfrFloatDataContainer().const_0();
        ++(mData->mRefCount);
    }
    else
    {
        mData = mpfrFloatDataContainer().allocateMpfrFloatData(false);
        mpfr_set_si(mData->mFloat, value, GMP_RNDN);
    }
}

MpfrFloat::MpfrFloat(const char* value, char** endptr):
    mData(mpfrFloatDataContainer().allocateMpfrFloatData(false))
{
    mpfr_strtofr(mData->mFloat, value, endptr, 0, GMP_RNDN);
}

MpfrFloat::~MpfrFloat()
{
    mpfrFloatDataContainer().releaseMpfrFloatData(mData);
}

MpfrFloat::MpfrFloat(const MpfrFloat& rhs):
    mData(rhs.mData)
{
    ++(mData->mRefCount);
}

MpfrFloat& MpfrFloat::operator=(const MpfrFloat& rhs)
{
    if(mData != rhs.mData)
    {
        mpfrFloatDataContainer().releaseMpfrFloatData(mData);
        mData = rhs.mData;
        ++(mData->mRefCount);
    }
    return *this;
}

MpfrFloat& MpfrFloat::operator=(double value)
{
    if(value == 0.0)
    {
        mpfrFloatDataContainer().releaseMpfrFloatData(mData);
        mData = mpfrFloatDataContainer().const_0();
        ++(mData->mRefCount);
    }
    else
    {
        if(mData->mRefCount > 1)
        {
            --(mData->mRefCount);
            mData = mpfrFloatDataContainer().allocateMpfrFloatData(false);
        }
        mpfr_set_d(mData->mFloat, value, GMP_RNDN);
    }
    return *this;
}

MpfrFloat& MpfrFloat::operator=(long double value)
{
    if(value == 0.0L)
    {
        mpfrFloatDataContainer().releaseMpfrFloatData(mData);
        mData = mpfrFloatDataContainer().const_0();
        ++(mData->mRefCount);
    }
    else
    {
        if(mData->mRefCount > 1)
        {
            --(mData->mRefCount);
            mData = mpfrFloatDataContainer().allocateMpfrFloatData(false);
        }
        mpfr_set_ld(mData->mFloat, value, GMP_RNDN);
    }
    return *this;
}

MpfrFloat& MpfrFloat::operator=(long value)
{
    if(value == 0)
    {
        mpfrFloatDataContainer().releaseMpfrFloatData(mData);
        mData = mpfrFloatDataContainer().const_0();
        ++(mData->mRefCount);
    }
    else
    {
        if(mData->mRefCount > 1)
        {
            --(mData->mRefCount);
            mData = mpfrFloatDataContainer().allocateMpfrFloatData(false);
        }
        mpfr_set_si(mData->mFloat, value, GMP_RNDN);
    }
    return *this;
}

MpfrFloat& MpfrFloat::operator=(int value)
{
    if(value == 0)
    {
        mpfrFloatDataContainer().releaseMpfrFloatData(mData);
        mData = mpfrFloatDataContainer().const_0();
        ++(mData->mRefCount);
    }
    else
    {
        if(mData->mRefCount > 1)
        {
            --(mData->mRefCount);
            mData = mpfrFloatDataContainer().allocateMpfrFloatData(false);
        }
        mpfr_set_si(mData->mFloat, value, GMP_RNDN);
    }
    return *this;
}

/*
MpfrFloat& MpfrFloat::operator=(const char* value)
{
    if(mData->mRefCount > 1)
    {
        --(mData->mRefCount);
        mData = mpfrFloatDataContainer().allocateMpfrFloatData(false);
    }

    mpfr_set_str(mData->mFloat, value, 10, GMP_RNDN);
    return *this;
}
*/

void MpfrFloat::parseValue(const char* value)
{
    copyIfShared();
    mpfr_set_str(mData->mFloat, value, 10, GMP_RNDN);
}

void MpfrFloat::parseValue(const char* value, char** endptr)
{
    copyIfShared();
    mpfr_strtofr(mData->mFloat, value, endptr, 0, GMP_RNDN);
}


//===========================================================================
// Data getters
//===========================================================================
template<>
void MpfrFloat::get_raw_mpfr_data<mpfr_t>(mpfr_t& dest_mpfr_t)
{
    std::memcpy(&dest_mpfr_t, mData->mFloat, sizeof(mpfr_t));
}

const char* MpfrFloat::getAsString(unsigned precision) const
{
#if(MPFR_VERSION_MAJOR < 2 || (MPFR_VERSION_MAJOR == 2 && MPFR_VERSION_MINOR < 4))
    static const char* retval =
        "[mpfr_snprintf() is not supported in mpfr versions prior to 2.4]";
    return retval;
#else
    static std::vector<char> str;
    str.resize(precision+30);
    mpfr_snprintf(&(str[0]), precision+30, "%.*RNg", precision, mData->mFloat);
    return &(str[0]);
#endif
}

bool MpfrFloat::isInteger() const
{
    return mpfr_integer_p(mData->mFloat) != 0;
}

long MpfrFloat::toInt() const
{
    return mpfr_get_si(mData->mFloat, GMP_RNDN);
}

double MpfrFloat::toDouble() const
{
    return mpfr_get_d(mData->mFloat, GMP_RNDN);
}


//===========================================================================
// Modifying operators
//===========================================================================
MpfrFloat& MpfrFloat::operator+=(const MpfrFloat& rhs)
{
    copyIfShared();
    mpfr_add(mData->mFloat, mData->mFloat, rhs.mData->mFloat, GMP_RNDN);
    return *this;
}

MpfrFloat& MpfrFloat::operator+=(double value)
{
    copyIfShared();
    mpfr_add_d(mData->mFloat, mData->mFloat, value, GMP_RNDN);
    return *this;
}

MpfrFloat& MpfrFloat::operator-=(const MpfrFloat& rhs)
{
    copyIfShared();
    mpfr_sub(mData->mFloat, mData->mFloat, rhs.mData->mFloat, GMP_RNDN);
    return *this;
}

MpfrFloat& MpfrFloat::operator-=(double value)
{
    copyIfShared();
    mpfr_sub_d(mData->mFloat, mData->mFloat, value, GMP_RNDN);
    return *this;
}

MpfrFloat& MpfrFloat::operator*=(const MpfrFloat& rhs)
{
    copyIfShared();
    mpfr_mul(mData->mFloat, mData->mFloat, rhs.mData->mFloat, GMP_RNDN);
    return *this;
}

MpfrFloat& MpfrFloat::operator*=(double value)
{
    copyIfShared();
    mpfr_mul_d(mData->mFloat, mData->mFloat, value, GMP_RNDN);
    return *this;
}

MpfrFloat& MpfrFloat::operator/=(const MpfrFloat& rhs)
{
    copyIfShared();
    mpfr_div(mData->mFloat, mData->mFloat, rhs.mData->mFloat, GMP_RNDN);
    return *this;
}

MpfrFloat& MpfrFloat::operator/=(double value)
{
    copyIfShared();
    mpfr_div_d(mData->mFloat, mData->mFloat, value, GMP_RNDN);
    return *this;
}

MpfrFloat& MpfrFloat::operator%=(const MpfrFloat& rhs)
{
    copyIfShared();
    mpfr_fmod(mData->mFloat, mData->mFloat, rhs.mData->mFloat, GMP_RNDN);
    return *this;
}


//===========================================================================
// Modifying functions
//===========================================================================
void MpfrFloat::negate()
{
    copyIfShared();
    mpfr_neg(mData->mFloat, mData->mFloat, GMP_RNDN);
}

void MpfrFloat::abs()
{
    copyIfShared();
    mpfr_abs(mData->mFloat, mData->mFloat, GMP_RNDN);
}


//===========================================================================
// Non-modifying operators
//===========================================================================
MpfrFloat MpfrFloat::operator+(const MpfrFloat& rhs) const
{
    MpfrFloat retval(kNoInitialization);
    mpfr_add(retval.mData->mFloat, mData->mFloat, rhs.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::operator+(double value) const
{
    MpfrFloat retval(kNoInitialization);
    mpfr_add_d(retval.mData->mFloat, mData->mFloat, value, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::operator-(const MpfrFloat& rhs) const
{
    MpfrFloat retval(kNoInitialization);
    mpfr_sub(retval.mData->mFloat, mData->mFloat, rhs.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::operator-(double value) const
{
    MpfrFloat retval(kNoInitialization);
    mpfr_sub_d(retval.mData->mFloat, mData->mFloat, value, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::operator*(const MpfrFloat& rhs) const
{
    MpfrFloat retval(kNoInitialization);
    mpfr_mul(retval.mData->mFloat, mData->mFloat, rhs.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::operator*(double value) const
{
    MpfrFloat retval(kNoInitialization);
    mpfr_mul_d(retval.mData->mFloat, mData->mFloat, value, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::operator/(const MpfrFloat& rhs) const
{
    MpfrFloat retval(kNoInitialization);
    mpfr_div(retval.mData->mFloat, mData->mFloat, rhs.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::operator/(double value) const
{
    MpfrFloat retval(kNoInitialization);
    mpfr_div_d(retval.mData->mFloat, mData->mFloat, value, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::operator%(const MpfrFloat& rhs) const
{
    MpfrFloat retval(kNoInitialization);
    mpfr_fmod(retval.mData->mFloat, mData->mFloat, rhs.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::operator-() const
{
    MpfrFloat retval(kNoInitialization);
    mpfr_neg(retval.mData->mFloat, mData->mFloat, GMP_RNDN);
    return retval;
}



//===========================================================================
// Comparison operators
//===========================================================================
bool MpfrFloat::operator<(const MpfrFloat& rhs) const
{
    return mpfr_cmp(mData->mFloat, rhs.mData->mFloat) < 0;
}

bool MpfrFloat::operator<(double value) const
{
    return mpfr_cmp_d(mData->mFloat, value) < 0;
}

bool MpfrFloat::operator<=(const MpfrFloat& rhs) const
{
    return mpfr_cmp(mData->mFloat, rhs.mData->mFloat) <= 0;
}

bool MpfrFloat::operator<=(double value) const
{
    return mpfr_cmp_d(mData->mFloat, value) <= 0;
}

bool MpfrFloat::operator>(const MpfrFloat& rhs) const
{
    return mpfr_cmp(mData->mFloat, rhs.mData->mFloat) > 0;
}

bool MpfrFloat::operator>(double value) const
{
    return mpfr_cmp_d(mData->mFloat, value) > 0;
}

bool MpfrFloat::operator>=(const MpfrFloat& rhs) const
{
    return mpfr_cmp(mData->mFloat, rhs.mData->mFloat) >= 0;
}

bool MpfrFloat::operator>=(double value) const
{
    return mpfr_cmp_d(mData->mFloat, value) >= 0;
}

bool MpfrFloat::operator==(const MpfrFloat& rhs) const
{
    return mpfr_cmp(mData->mFloat, rhs.mData->mFloat) == 0;
}

bool MpfrFloat::operator==(double value) const
{
    return mpfr_cmp_d(mData->mFloat, value) == 0;
}

bool MpfrFloat::operator!=(const MpfrFloat& rhs) const
{
    return mpfr_cmp(mData->mFloat, rhs.mData->mFloat) != 0;
}

bool MpfrFloat::operator!=(double value) const
{
    return mpfr_cmp_d(mData->mFloat, value) != 0;
}


//===========================================================================
// Operator functions
//===========================================================================
MpfrFloat operator+(double lhs, const MpfrFloat& rhs)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_add_d(retval.mData->mFloat, rhs.mData->mFloat, lhs, GMP_RNDN);
    return retval;
}

MpfrFloat operator-(double lhs, const MpfrFloat& rhs)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_d_sub(retval.mData->mFloat, lhs, rhs.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat operator*(double lhs, const MpfrFloat& rhs)
{
    return rhs * lhs;
}

MpfrFloat operator/(double lhs, const MpfrFloat& rhs)
{
    return MpfrFloat(lhs) / rhs;
}

MpfrFloat operator%(double lhs, const MpfrFloat& rhs)
{
    return MpfrFloat(lhs) % rhs;
}

std::ostream& operator<<(std::ostream& os, const MpfrFloat& value)
{
    os << value.getAsString(unsigned(os.precision()));
    return os;
}

//===========================================================================
// Static functions
//===========================================================================
MpfrFloat MpfrFloat::log(const MpfrFloat& value)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_log(retval.mData->mFloat, value.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::log2(const MpfrFloat& value)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_log2(retval.mData->mFloat, value.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::log10(const MpfrFloat& value)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_log10(retval.mData->mFloat, value.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::exp(const MpfrFloat& value)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_exp(retval.mData->mFloat, value.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::exp2(const MpfrFloat& value)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_exp2(retval.mData->mFloat, value.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::exp10(const MpfrFloat& value)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_exp10(retval.mData->mFloat, value.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::cos(const MpfrFloat& value)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_cos(retval.mData->mFloat, value.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::sin(const MpfrFloat& value)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_sin(retval.mData->mFloat, value.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::tan(const MpfrFloat& value)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_tan(retval.mData->mFloat, value.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::sec(const MpfrFloat& value)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_sec(retval.mData->mFloat, value.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::csc(const MpfrFloat& value)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_csc(retval.mData->mFloat, value.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::cot(const MpfrFloat& value)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_cot(retval.mData->mFloat, value.mData->mFloat, GMP_RNDN);
    return retval;
}

void MpfrFloat::sincos(const MpfrFloat& value,
                       MpfrFloat& sin,
                       MpfrFloat& cos)
{
    mpfr_sin_cos(
        sin.mData->mFloat, cos.mData->mFloat, value.mData->mFloat, GMP_RNDN);
}

MpfrFloat MpfrFloat::acos(const MpfrFloat& value)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_acos(retval.mData->mFloat, value.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::asin(const MpfrFloat& value)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_asin(retval.mData->mFloat, value.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::atan(const MpfrFloat& value)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_atan(retval.mData->mFloat, value.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::atan2(const MpfrFloat& value1, const MpfrFloat& value2)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_atan2(retval.mData->mFloat,
               value1.mData->mFloat, value2.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::hypot(const MpfrFloat& value1, const MpfrFloat& value2)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_hypot(retval.mData->mFloat,
               value1.mData->mFloat, value2.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::cosh(const MpfrFloat& value)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_cosh(retval.mData->mFloat, value.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::sinh(const MpfrFloat& value)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_sinh(retval.mData->mFloat, value.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::tanh(const MpfrFloat& value)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_tanh(retval.mData->mFloat, value.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::acosh(const MpfrFloat& value)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_acosh(retval.mData->mFloat, value.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::asinh(const MpfrFloat& value)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_asinh(retval.mData->mFloat, value.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::atanh(const MpfrFloat& value)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_atanh(retval.mData->mFloat, value.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::sqrt(const MpfrFloat& value)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_sqrt(retval.mData->mFloat, value.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::cbrt(const MpfrFloat& value)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_cbrt(retval.mData->mFloat, value.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::root(const MpfrFloat& value, unsigned long root)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_root(retval.mData->mFloat, value.mData->mFloat, root, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::pow(const MpfrFloat& value1, const MpfrFloat& value2)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_pow(retval.mData->mFloat,
             value1.mData->mFloat, value2.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::pow(const MpfrFloat& value, long exponent)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_pow_si(retval.mData->mFloat, value.mData->mFloat, exponent, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::abs(const MpfrFloat& value)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_abs(retval.mData->mFloat, value.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::dim(const MpfrFloat& value1, const MpfrFloat& value2)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_dim(retval.mData->mFloat,
             value1.mData->mFloat, value2.mData->mFloat, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::round(const MpfrFloat& value)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_round(retval.mData->mFloat, value.mData->mFloat);
    return retval;
}

MpfrFloat MpfrFloat::ceil(const MpfrFloat& value)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_ceil(retval.mData->mFloat, value.mData->mFloat);
    return retval;
}

MpfrFloat MpfrFloat::floor(const MpfrFloat& value)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_floor(retval.mData->mFloat, value.mData->mFloat);
    return retval;
}

MpfrFloat MpfrFloat::trunc(const MpfrFloat& value)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_trunc(retval.mData->mFloat, value.mData->mFloat);
    return retval;
}

MpfrFloat MpfrFloat::parseString(const char* str, char** endptr)
{
    MpfrFloat retval(MpfrFloat::kNoInitialization);
    mpfr_strtofr(retval.mData->mFloat, str, endptr, 0, GMP_RNDN);
    return retval;
}

MpfrFloat MpfrFloat::const_pi()
{
    return mpfrFloatDataContainer().const_pi();
}

MpfrFloat MpfrFloat::const_e()
{
    return mpfrFloatDataContainer().const_e();
}

MpfrFloat MpfrFloat::const_log2()
{
    return mpfrFloatDataContainer().const_log2();
}

MpfrFloat MpfrFloat::someEpsilon()
{
    return mpfrFloatDataContainer().const_epsilon();
}
