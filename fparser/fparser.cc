/***************************************************************************\
|* Function Parser for C++ v4.3                                            *|
|*-------------------------------------------------------------------------*|
|* Copyright: Juha Nieminen, Joel Yliluoma                                 *|
|*                                                                         *|
|* This library is distributed under the terms of the                      *|
|* GNU Lesser General Public License version 3.                            *|
|* (See lgpl.txt and gpl.txt for the license text.)                        *|
\***************************************************************************/

#include "fpconfig.hh"
#include "fparser.hh"

#include <set>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cmath>
#include <cassert>
#include <limits>
using namespace std;

#include "fptypes.hh"
using namespace FUNCTIONPARSERTYPES;

#ifdef FP_USE_THREAD_SAFE_EVAL_WITH_ALLOCA
#ifndef FP_USE_THREAD_SAFE_EVAL
#define FP_USE_THREAD_SAFE_EVAL
#endif
#endif

#ifdef __GNUC__
# define likely(x)       __builtin_expect(!!(x), 1)
# define unlikely(x)     __builtin_expect(!!(x), 0)
#else
# define likely(x)   (x)
# define unlikely(x) (x)
#endif


//=========================================================================
// Opcode analysis functions
//=========================================================================
// These functions are used by the Parse() bytecode optimizer (mostly from
// code in fp_opcode_add.inc).

bool FUNCTIONPARSERTYPES::IsLogicalOpcode(unsigned op)
{
    switch(op)
    {
      case cAnd: case cAbsAnd:
      case cOr:  case cAbsOr:
      case cNot: case cAbsNot:
      case cNotNot: case cAbsNotNot:
      case cEqual: case cNEqual:
      case cLess: case cLessOrEq:
      case cGreater: case cGreaterOrEq:
          return true;
      default: break;
    }
    return false;
}

bool FUNCTIONPARSERTYPES::IsComparisonOpcode(unsigned op)
{
    switch(op)
    {
      case cEqual: case cNEqual:
      case cLess: case cLessOrEq:
      case cGreater: case cGreaterOrEq:
          return true;
      default: break;
    }
    return false;
}

unsigned FUNCTIONPARSERTYPES::OppositeComparisonOpcode(unsigned op)
{
    switch(op)
    {
      case cLess: return cGreater;
      case cGreater: return cLess;
      case cLessOrEq: return cGreaterOrEq;
      case cGreaterOrEq: return cLessOrEq;
    }
    return op;
}

bool FUNCTIONPARSERTYPES::IsNeverNegativeValueOpcode(unsigned op)
{
    switch(op)
    {
      case cAnd: case cAbsAnd:
      case cOr:  case cAbsOr:
      case cNot: case cAbsNot:
      case cNotNot: case cAbsNotNot:
      case cEqual: case cNEqual:
      case cLess: case cLessOrEq:
      case cGreater: case cGreaterOrEq:
      case cSqrt: case cRSqrt: case cSqr:
      case cHypot:
      case cAbs:
      case cAcos: case cCosh:
          return true;
      default: break;
    }
    return false;
}

bool FUNCTIONPARSERTYPES::IsAlwaysIntegerOpcode(unsigned op)
{
    switch(op)
    {
      case cAnd: case cAbsAnd:
      case cOr:  case cAbsOr:
      case cNot: case cAbsNot:
      case cNotNot: case cAbsNotNot:
      case cEqual: case cNEqual:
      case cLess: case cLessOrEq:
      case cGreater: case cGreaterOrEq:
      case cInt: case cFloor: case cCeil: case cTrunc:
          return true;
      default: break;
    }
    return false;
}

bool FUNCTIONPARSERTYPES::IsUnaryOpcode(unsigned op)
{
    switch(op)
    {
      case cInv: case cNeg:
      case cNot: case cAbsNot:
      case cNotNot: case cAbsNotNot:
      case cSqr: case cRSqrt:
      case cDeg: case cRad:
          return true;
    }
    return (op < FUNC_AMOUNT && Functions[op].params == 1);
}

bool FUNCTIONPARSERTYPES::IsBinaryOpcode(unsigned op)
{
    switch(op)
    {
      case cAdd: case cSub: case cRSub:
      case cMul: case cDiv: case cRDiv:
      case cMod:
      case cEqual: case cNEqual: case cLess:
      case cLessOrEq: case cGreater: case cGreaterOrEq:
      case cAnd: case cAbsAnd:
      case cOr: case cAbsOr:
          return true;
    }
    return (op < FUNC_AMOUNT && Functions[op].params == 2);
}

bool FUNCTIONPARSERTYPES::HasInvalidRangesOpcode(unsigned op)
{
#ifndef FP_NO_EVALUATION_CHECKS
    // Returns true, if the given opcode has a range of
    // input values that gives an error.
    switch(op)
    {
      case cAcos: // allowed range: |x| <= 1
      case cAsin: // allowed range: |x| <= 1
      case cAcosh: // allowed range: x >= 1
      case cAtanh: // allowed range: |x| < 1
          //case cCot: // note: no range, just separate values
          //case cCsc: // note: no range, just separate values
      case cLog: // allowed range: x > 0
      case cLog2: // allowed range: x > 0
      case cLog10: // allowed range: x > 0
#ifdef FP_SUPPORT_OPTIMIZER
      case cLog2by: // allowed range: x > 0
#endif
          //case cPow: // note: no range, just separate values
          //case cSec: // note: no range, just separate values
      case cSqrt: // allowed range: x >= 0
      case cRSqrt: // allowed range: x > 0
          //case cDiv: // note: no range, just separate values
          //case cRDiv: // note: no range, just separate values
          //case cInv: // note: no range, just separate values
          return true;
    }
#endif
    return false;
}


//=========================================================================
// Mathematical template functions
//=========================================================================
/* fp_pow() is a wrapper for std::pow()
 * that produces an identical value for
 * exp(1) ^ 2.0  (0x4000000000000000)
 * as exp(2.0)   (0x4000000000000000)
 * - std::pow() on x86_64
 * produces 2.0  (0x3FFFFFFFFFFFFFFF) instead!
 * See comments below for other special traits.
 */
namespace
{
    template<typename ValueT>
    inline ValueT fp_pow_with_exp_log(const ValueT& x, const ValueT& y)
    {
        // Exponentiation using exp(log(x)*y).
        // See http://en.wikipedia.org/wiki/Exponentiation#Real_powers
        // Requirements: x > 0.
        return fp_exp(fp_log(x) * y);
    }

    template<typename ValueT>
    inline ValueT fp_powi(ValueT x, unsigned long y)
    {
        // Fast binary exponentiation algorithm
        // See http://en.wikipedia.org/wiki/Exponentiation_by_squaring
        // Requirements: y is non-negative integer.
        ValueT result(1);
        while(y != 0)
        {
            if(y & 1) { result *= x; y -= 1; }
            else      { x *= x;      y /= 2; }
        }
        return result;
    }
}

template<typename ValueT>
ValueT FUNCTIONPARSERTYPES::fp_pow(const ValueT& x, const ValueT& y)
{
    if(x == ValueT(1)) return ValueT(1);
    // y is now zero or positive
    if(isLongInteger(y))
    {
        // Use fast binary exponentiation algorithm
        if(y >= ValueT(0))
            return fp_powi(x,              makeLongInteger(y));
        else
            return ValueT(1) / fp_powi(x, -makeLongInteger(y));
    }
    if(y >= ValueT(0))
    {
        // y is now positive. Calculate using exp(log(x)*y).
        if(x > ValueT(0)) return fp_pow_with_exp_log(x, y);
        if(x == ValueT(0)) return ValueT(0);
        // At this point, y > 0.0 and x is known to be < 0.0,
        // because positive and zero cases are already handled.
        if(!isInteger(y*ValueT(16)))
            return -fp_pow_with_exp_log(-x, y);
        // ^This is not technically correct, but it allows
        // functions such as cbrt(x^5), that is, x^(5/3),
        // to be evaluated when x is negative.
        // It is too complicated (and slow) to test whether y
        // is a formed from a ratio of an integer to an odd integer.
        // (And due to floating point inaccuracy, pointless too.)
        // For example, x^1.30769230769... is
        // actually x^(17/13), i.e. (x^17) ^ (1/13).
        // (-5)^(17/13) gives us now -8.204227562330453.
        // To see whether the result is right, we can test the given
        // root: (-8.204227562330453)^13 gives us the value of (-5)^17,
        // which proves that the expression was correct.
        //
        // The y*16 check prevents e.g. (-4)^(3/2) from being calculated,
        // as it would confuse functioninfo when pow() returns no error
        // but sqrt() does when the formula is converted into sqrt(x)*x.
        //
        // The errors in this approach are:
        //     (-2)^sqrt(2) should produce NaN
        //                  or actually sqrt(2)I + 2^sqrt(2),
        //                  produces -(2^sqrt(2)) instead.
        //                  (Impact: Neglible)
        // Thus, at worst, we're changing a NaN (or complex)
        // result into a negative real number result.
    }
    else
    {
        // y is negative. Utilize the x^y = 1/(x^-y) identity.
        if(x > ValueT(0)) return fp_pow_with_exp_log(ValueT(1) / x, -y);
        if(x < ValueT(0))
        {
            if(!isInteger(y*ValueT(-16)))
                return -fp_pow_with_exp_log(ValueT(-1) / x, -y);
            // ^ See comment above.
        }
        // Remaining case: 0.0 ^ negative number
    }
    // This is reached when:
    //      x=0, and y<0
    //      x<0, and y*16 is either positive or negative integer
    // It is used for producing error values and as a safe fallback.
    return fp_pow_base(x, y);
}


//=========================================================================
// Elementary (atom) parsing functions
//=========================================================================
namespace
{
    /* Reads an UTF8-encoded sequence which forms a valid identifier name from
       the given input string and returns its length. If bit 31 is set, the
       return value also contains the internal function opcode (defined in
       fptypes.hh) that matches the name.
    */
    unsigned readIdentifierForFloatType(const char* input)
    {
        /* Assuming unsigned = 32 bits:
              76543210 76543210 76543210 76543210
           Return value if built-in function:
              1PPPPPPP PPPPPPPP LLLLLLLL LLLLLLLL
                P = function opcode      (15 bits)
                L = function name length (16 bits)
           Return value if not built-in function:
              0LLLLLLL LLLLLLLL LLLLLLLL LLLLLLLL
                L = identifier length (31 bits)
           If unsigned has more than 32 bits, the other
           higher order bits are to be assumed zero.
        */
#include "fp_identifier_parser.inc"
        return 0;
    }

    inline unsigned readIdentifierForIntType(const char* input)
    {
        const unsigned value = readIdentifierForFloatType(input);
        if((value & 0x80000000U) != 0 &&
           !Functions[(value >> 16) & 0x7FFF].okForInt())
            return value & 0xFFFF;
        return value;
    }

    template<typename Value_t>
    inline unsigned readIdentifier(const char* input)
    {
        return IsIntType<Value_t>::result
                ? readIdentifierForIntType(input)
                : readIdentifierForFloatType(input);
    }

    // Returns true if the entire string is a valid identifier
    template<typename Value_t>
    bool containsOnlyValidIdentifierChars(const std::string& name)
    {
        if(name.empty()) return false;
        return readIdentifier<Value_t>(name.c_str()) == (unsigned) name.size();
    }


    // -----------------------------------------------------------------------
    // Wrappers for strto... functions
    // -----------------------------------------------------------------------
    template<typename Value_t>
    inline Value_t fp_parseLiteral(const char* str, char** endptr)
    {
        return strtod(str, endptr);
    }

#ifdef FP_SUPPORT_FLOAT_TYPE
    template<>
    inline float fp_parseLiteral<float>(const char* str, char** endptr)
    {
        return strtof(str, endptr);
    }
#endif

#ifdef FP_SUPPORT_LONG_DOUBLE_TYPE
    template<>
    inline long double fp_parseLiteral<long double>(const char* str,
                                                    char** endptr)
    {
        return strtold(str, endptr);
    }
#endif

#ifdef FP_SUPPORT_LONG_INT_TYPE
    template<>
    inline long fp_parseLiteral<long>(const char* str, char** endptr)
    {
        return strtol(str, endptr, 10);
    }
#endif


    // -----------------------------------------------------------------------
    // Hexadecimal floating point literal parsing
    // -----------------------------------------------------------------------
    inline int testXdigit(unsigned c)
    {
        if((c-'0') < 10u) return c&15; // 0..9
        if(((c|0x20)-'a') < 6u) return 9+(c&15); // A..F or a..f
        return -1; // Not a hex digit
    }

    template<typename elem_t, unsigned n_limbs, unsigned limb_bits>
    inline void addXdigit(elem_t* buffer, unsigned nibble)
    {
        for(unsigned p=0; p<n_limbs; ++p)
        {
            unsigned carry = unsigned( buffer[p] >> (elem_t)(limb_bits-4) );
            buffer[p] = (buffer[p] << 4) | nibble;
            nibble = carry;
        }
    }

    template<typename Value_t>
    Value_t parseHexLiteral(const char* str, char** endptr)
    {
        const unsigned bits_per_char = 8;

        const int MantissaBits =
            std::numeric_limits<Value_t>::radix == 2
            ? std::numeric_limits<Value_t>::digits
            : (((sizeof(Value_t) * bits_per_char) &~ 3) - 4);

        typedef unsigned long elem_t;
        const int ExtraMantissaBits = 4 + ((MantissaBits+3)&~3); // Store one digit more for correct rounding
        const unsigned limb_bits = sizeof(elem_t) * bits_per_char;
        const unsigned n_limbs   = (ExtraMantissaBits + limb_bits-1) / limb_bits;
        elem_t mantissa_buffer[n_limbs] = { 0 };

        int n_mantissa_bits = 0; // Track the number of bits
        int exponent = 0; // The exponent that will be used to multiply the mantissa
        // Read integer portion
        while(true)
        {
            int xdigit = testXdigit(*str);
            if(xdigit < 0) break;
            addXdigit<elem_t,n_limbs,limb_bits> (mantissa_buffer, xdigit);
            ++str;

            n_mantissa_bits += 4;
            if(n_mantissa_bits >= ExtraMantissaBits)
            {
                // Exhausted the precision. Parse the rest (until exponent)
                // normally but ignore the actual digits.
                for(; testXdigit(*str) >= 0; ++str)
                    exponent += 4;
                // Read but ignore decimals
                if(*str == '.')
                    for(++str; testXdigit(*str) >= 0; ++str)
                        {}
                goto read_exponent;
            }
        }
        // Read decimals
        if(*str == '.')
            for(++str; ; )
            {
                int xdigit = testXdigit(*str);
                if(xdigit < 0) break;
                addXdigit<elem_t,n_limbs,limb_bits> (mantissa_buffer, xdigit);
                ++str;

                exponent -= 4;
                n_mantissa_bits += 4;
                if(n_mantissa_bits >= ExtraMantissaBits)
                {
                    // Exhausted the precision. Skip the rest
                    // of the decimals, until the exponent.
                    while(testXdigit(*str) >= 0)
                        ++str;
                    break;
                }
            }

        // Read exponent
    read_exponent:
        if(*str == 'p' || *str == 'P')
        {
            const char* str2 = str+1;
            long p_exponent = strtol(str2, (char**) &str2, 10);
            if(str2 != str+1 && p_exponent == (long)(int)p_exponent)
            {
                exponent += (int)p_exponent;
                str = str2;
            }
        }

        if(endptr) *endptr = (char*) str;

        Value_t result = ldexp(Value_t(mantissa_buffer[0]), exponent);
        for(unsigned p=1; p<n_limbs; ++p)
        {
            exponent += limb_bits;
            result += ldexp(Value_t(mantissa_buffer[p]), exponent);
        }
        return result;
    }

#ifdef FP_SUPPORT_LONG_INT_TYPE
    template<>
    long parseHexLiteral<long>(const char* str, char** endptr)
    {
        return strtol(str, endptr, 16);
    }
#endif
}

//=========================================================================
// Utility functions
//=========================================================================
namespace
{
    // -----------------------------------------------------------------------
    // Add a new identifier to the specified identifier map
    // -----------------------------------------------------------------------
    // Return value will be false if the name already existed
    template<typename Value_t>
    bool addNewNameData(NamePtrsMap<Value_t>& namePtrs,
                        std::pair<NamePtr, NameData<Value_t> >& newName,
                        bool isVar)
    {
        typename NamePtrsMap<Value_t>::iterator nameIter =
            namePtrs.lower_bound(newName.first);

        if(nameIter != namePtrs.end() && newName.first == nameIter->first)
        {
            // redefining a var is not allowed.
            if(isVar) return false;

            // redefining other tokens is allowed, if the type stays the same.
            if(nameIter->second.type != newName.second.type)
                return false;

            // update the data
            nameIter->second = newName.second;
            return true;
        }

        if(!isVar)
        {
            // Allocate a copy of the name (pointer stored in the map key)
            // However, for VARIABLEs, the pointer points to VariableString,
            // which is managed separately. Thusly, only done when !IsVar.
            char* namebuf = new char[newName.first.nameLength];
            memcpy(namebuf, newName.first.name, newName.first.nameLength);
            newName.first.name = namebuf;
        }

        namePtrs.insert(nameIter, newName);
        return true;
    }
}


//=========================================================================
// Data struct implementation
//=========================================================================
template<typename Value_t>
FunctionParserBase<Value_t>::Data::Data():
    mReferenceCounter(1),
    mVariablesAmount(0),
    mStackSize(0)
{}

template<typename Value_t>
FunctionParserBase<Value_t>::Data::Data(const Data& rhs):
    mReferenceCounter(0),
    mVariablesAmount(rhs.mVariablesAmount),
    mVariablesString(rhs.mVariablesString),
    mNamePtrs(),
    mFuncPtrs(rhs.mFuncPtrs),
    mFuncParsers(rhs.mFuncParsers),
    mByteCode(rhs.mByteCode),
    mImmed(rhs.mImmed),
#ifndef FP_USE_THREAD_SAFE_EVAL
    mStack(rhs.mStackSize),
#endif
    mStackSize(rhs.mStackSize)
{
    for(typename NamePtrsMap<Value_t>::const_iterator i = rhs.mNamePtrs.begin();
        i != rhs.mNamePtrs.end();
        ++i)
    {
        if(i->second.type == NameData<Value_t>::VARIABLE)
        {
            const size_t variableStringOffset =
                i->first.name - rhs.mVariablesString.c_str();
            std::pair<NamePtr, NameData<Value_t> > tmp
                (NamePtr(&mVariablesString[variableStringOffset],
                         i->first.nameLength),
                 i->second);
            mNamePtrs.insert(mNamePtrs.end(), tmp);
        }
        else
        {
            std::pair<NamePtr, NameData<Value_t> > tmp
                (NamePtr(new char[i->first.nameLength], i->first.nameLength),
                 i->second );
            memcpy(const_cast<char*>(tmp.first.name), i->first.name,
                   tmp.first.nameLength);
            mNamePtrs.insert(mNamePtrs.end(), tmp);
        }
    }
}

template<typename Value_t>
FunctionParserBase<Value_t>::Data::~Data()
{
    for(typename NamePtrsMap<Value_t>::iterator i = mNamePtrs.begin();
        i != mNamePtrs.end();
        ++i)
    {
        if(i->second.type != NameData<Value_t>::VARIABLE)
            delete[] i->first.name;
    }
}


//=========================================================================
// FunctionParser constructors, destructor and assignment
//=========================================================================
template<typename Value_t>
FunctionParserBase<Value_t>::FunctionParserBase():
    mDelimiterChar(0),
    mParseErrorType(NO_FUNCTION_PARSED_YET), mEvalErrorType(0),
    mData(new Data),
    mUseDegreeConversion(false),
    mEvalRecursionLevel(0),
    mStackPtr(0), mErrorLocation(0)
{
}

template<typename Value_t>
FunctionParserBase<Value_t>::~FunctionParserBase()
{
    if(--(mData->mReferenceCounter) == 0)
        delete mData;
}

template<typename Value_t>
FunctionParserBase<Value_t>::FunctionParserBase(const FunctionParserBase& cpy):
    mDelimiterChar(cpy.mDelimiterChar),
    mParseErrorType(cpy.mParseErrorType),
    mEvalErrorType(cpy.mEvalErrorType),
    mData(cpy.mData),
    mUseDegreeConversion(cpy.mUseDegreeConversion),
    mEvalRecursionLevel(0),
    mStackPtr(0), mErrorLocation(0)
{
    ++(mData->mReferenceCounter);
}

template<typename Value_t>
FunctionParserBase<Value_t>&
FunctionParserBase<Value_t>::operator=(const FunctionParserBase& cpy)
{
    if(mData != cpy.mData)
    {
        if(--(mData->mReferenceCounter) == 0) delete mData;

        mDelimiterChar = cpy.mDelimiterChar;
        mParseErrorType = cpy.mParseErrorType;
        mEvalErrorType = cpy.mEvalErrorType;
        mData = cpy.mData;
        mUseDegreeConversion = cpy.mUseDegreeConversion;
        mEvalRecursionLevel = cpy.mEvalRecursionLevel;

        ++(mData->mReferenceCounter);
    }

    return *this;
}


template<typename Value_t>
void FunctionParserBase<Value_t>::setDelimiterChar(char c)
{
    mDelimiterChar = c;
}


//---------------------------------------------------------------------------
// Copy-on-write method
//---------------------------------------------------------------------------
template<typename Value_t>
void FunctionParserBase<Value_t>::CopyOnWrite()
{
    if(mData->mReferenceCounter > 1)
    {
        Data* oldData = mData;
        mData = new Data(*oldData);
        --(oldData->mReferenceCounter);
        mData->mReferenceCounter = 1;
    }
}

template<typename Value_t>
void FunctionParserBase<Value_t>::ForceDeepCopy()
{
    CopyOnWrite();
}


//=========================================================================
// User-defined identifier addition functions
//=========================================================================
template<typename Value_t>
bool FunctionParserBase<Value_t>::AddConstant(const std::string& name,
                                              Value_t value)
{
    if(!containsOnlyValidIdentifierChars<Value_t>(name)) return false;

    CopyOnWrite();
    std::pair<NamePtr, NameData<Value_t> > newName
        (NamePtr(name.data(), unsigned(name.size())),
         NameData<Value_t>(NameData<Value_t>::CONSTANT, value));

    return addNewNameData(mData->mNamePtrs, newName, false);
}

template<typename Value_t>
bool FunctionParserBase<Value_t>::AddUnit(const std::string& name,
                                          Value_t value)
{
    if(!containsOnlyValidIdentifierChars<Value_t>(name)) return false;

    CopyOnWrite();
    std::pair<NamePtr, NameData<Value_t> > newName
        (NamePtr(name.data(), unsigned(name.size())),
         NameData<Value_t>(NameData<Value_t>::UNIT, value));
    return addNewNameData(mData->mNamePtrs, newName, false);
}

template<typename Value_t>
bool FunctionParserBase<Value_t>::AddFunction
(const std::string& name, FunctionPtr ptr, unsigned paramsAmount)
{
    if(!containsOnlyValidIdentifierChars<Value_t>(name)) return false;

    CopyOnWrite();
    std::pair<NamePtr, NameData<Value_t> > newName
        (NamePtr(name.data(), unsigned(name.size())),
         NameData<Value_t>(NameData<Value_t>::FUNC_PTR,
                           unsigned(mData->mFuncPtrs.size())));

    const bool success = addNewNameData(mData->mNamePtrs, newName, false);
    if(success)
    {
        mData->mFuncPtrs.push_back(typename Data::FuncPtrData());
        mData->mFuncPtrs.back().mFuncPtr = ptr;
        mData->mFuncPtrs.back().mParams = paramsAmount;
    }
    return success;
}

template<typename Value_t>
bool FunctionParserBase<Value_t>::CheckRecursiveLinking
(const FunctionParserBase* fp) const
{
    if(fp == this) return true;
    for(unsigned i = 0; i < fp->mData->mFuncParsers.size(); ++i)
        if(CheckRecursiveLinking(fp->mData->mFuncParsers[i].mParserPtr))
            return true;
    return false;
}

template<typename Value_t>
bool FunctionParserBase<Value_t>::AddFunction(const std::string& name,
                                              FunctionParserBase& fp)
{
    if(!containsOnlyValidIdentifierChars<Value_t>(name) ||
       CheckRecursiveLinking(&fp))
        return false;

    CopyOnWrite();
    std::pair<NamePtr, NameData<Value_t> > newName
        (NamePtr(name.data(), unsigned(name.size())),
         NameData<Value_t>(NameData<Value_t>::PARSER_PTR,
                           unsigned(mData->mFuncParsers.size())));

    const bool success = addNewNameData(mData->mNamePtrs, newName, false);
    if(success)
    {
        mData->mFuncParsers.push_back(typename Data::FuncPtrData());
        mData->mFuncParsers.back().mParserPtr = &fp;
        mData->mFuncParsers.back().mParams = fp.mData->mVariablesAmount;
    }
    return success;
}

template<typename Value_t>
bool FunctionParserBase<Value_t>::RemoveIdentifier(const std::string& name)
{
    CopyOnWrite();

    NamePtr namePtr(name.data(), unsigned(name.size()));

    typename NamePtrsMap<Value_t>::iterator
        nameIter = mData->mNamePtrs.find(namePtr);

    if(nameIter != mData->mNamePtrs.end())
    {
        if(nameIter->second.type == NameData<Value_t>::VARIABLE)
        {
            // Illegal attempt to delete variables
            return false;
        }
        delete[] nameIter->first.name;
        mData->mNamePtrs.erase(nameIter);
        return true;
    }
    return false;
}


//=========================================================================
// Function parsing
//=========================================================================
namespace
{
    // Error messages returned by ErrorMsg():
    const char* const ParseErrorMessage[]=
    {
        "Syntax error",                             // 0
        "Mismatched parenthesis",                   // 1
        "Missing ')'",                              // 2
        "Empty parentheses",                        // 3
        "Syntax error: Operator expected",          // 4
        "Not enough memory",                        // 5
        "An unexpected error occurred. Please make a full bug report "
        "to the author",                            // 6
        "Syntax error in parameter 'Vars' given to "
        "FunctionParser::Parse()",                  // 7
        "Illegal number of parameters to function", // 8
        "Syntax error: Premature end of string",    // 9
        "Syntax error: Expecting ( after function", // 10
        "Syntax error: Unknown identifier",         // 11
        "(No function has been parsed yet)",
        ""
    };

    template<typename Value_t>
    inline typename FunctionParserBase<Value_t>::ParseErrorType
    noCommaError(char c)
    {
        return c == ')' ?
            FunctionParserBase<Value_t>::ILL_PARAMS_AMOUNT :
            FunctionParserBase<Value_t>::SYNTAX_ERROR;
    }

    template<typename Value_t>
    inline typename FunctionParserBase<Value_t>::ParseErrorType
    noParenthError(char c)
    {
        return c == ',' ?
            FunctionParserBase<Value_t>::ILL_PARAMS_AMOUNT :
            FunctionParserBase<Value_t>::MISSING_PARENTH;
    }

    template<unsigned offset>
    struct IntLiteralMask
    {
        enum { mask =
        //    (    1UL << ('-'-offset)) |
            (0x3FFUL << ('0'-offset)) }; /* 0x3FF = 10 bits worth "1" */
        // Note: If you change fparser to support negative numbers parsing
        //       (as opposed to parsing them as cNeg followed by literal),
        //       enable the '-' line above, and change the offset value
        //       in BeginsLiteral() to '-' instead of '.'.
    };

    template<typename Value_t, unsigned offset>
    struct LiteralMask
    {
        enum { mask =
            (    1UL << ('.'-offset)) |
            IntLiteralMask<offset>::mask };
    };
#ifdef FP_SUPPORT_LONG_INT_TYPE
    template<unsigned offset>
    struct LiteralMask<long, offset>: public IntLiteralMask<offset>
    {
    };
#endif
#ifdef FP_SUPPORT_GMP_INT_TYPE
    template<unsigned offset>
    struct LiteralMask<GmpInt, offset>: public IntLiteralMask<offset>
    {
    };
#endif

    template<unsigned offset>
    struct SimpleSpaceMask
    {
        enum { mask =
            (1UL << ('\r'-offset)) |
            (1UL << ('\n'-offset)) |
            (1UL << ('\v'-offset)) |
            (1UL << ('\t'-offset)) |
            (1UL << (' ' -offset)) };
    };

    template<typename Value_t>
    inline bool BeginsLiteral(unsigned byte)
    {
        const unsigned n = sizeof(unsigned long)>=8 ? 0 : '.';
        byte -= n;
        if(byte > (unsigned char)('9'-n)) return false;
        unsigned long shifted = 1UL << byte;
        const unsigned long mask = LiteralMask<Value_t, n>::mask;
        return (mask & shifted) != 0;
    }

    template<typename CharPtr>
    inline void SkipSpace(CharPtr& function)
    {
/*
        Space characters in unicode:
U+0020  SPACE                      Depends on font, often adjusted (see below)
U+00A0  NO-BREAK SPACE             As a space, but often not adjusted
U+2000  EN QUAD                    1 en (= 1/2 em)
U+2001  EM QUAD                    1 em (nominally, the height of the font)
U+2002  EN SPACE                   1 en (= 1/2 em)
U+2003  EM SPACE                   1 em
U+2004  THREE-PER-EM SPACE         1/3 em
U+2005  FOUR-PER-EM SPACE          1/4 em
U+2006  SIX-PER-EM SPACE           1/6 em
U+2007  FIGURE SPACE               Tabular width, the width of digits
U+2008  PUNCTUATION SPACE          The width of a period .
U+2009  THIN SPACE                 1/5 em (or sometimes 1/6 em)
U+200A  HAIR SPACE                 Narrower than THIN SPACE
U+200B  ZERO WIDTH SPACE           Nominally no width, but may expand
U+202F  NARROW NO-BREAK SPACE      Narrower than NO-BREAK SPACE (or SPACE)
U+205F  MEDIUM MATHEMATICAL SPACE  4/18 em
U+3000  IDEOGRAPHIC SPACE          The width of ideographic (CJK) characters.
        Also:
U+000A  \n
U+000D  \r
U+0009  \t
U+000B  \v
        As UTF-8 sequences:
            09
            0A
            0B
            0D
            20
            C2 A0
            E2 80 80-8B
            E2 80 AF
            E2 81 9F
            E3 80 80
*/
        while(true)
        {
            const unsigned n = sizeof(unsigned long)>=8 ? 0 : '\t';
            typedef signed char schar;
            unsigned byte = (unsigned char)*function;
            byte -= n;
            // ^Note: values smaller than n intentionally become
            //        big values here due to integer wrap. The
            //        comparison below thus excludes them, making
            //        the effective range 0x09..0x20 (32-bit)
            //        or 0x00..0x20 (64-bit) within the if-clause.
            if(byte <= (unsigned char)(' '-n))
            {
                unsigned long shifted = 1UL << byte;
                const unsigned long mask = SimpleSpaceMask<n>::mask;
                if(mask & shifted)
                    { ++function; continue; } // \r, \n, \t, \v and space
                break;
            }
            if(likely(byte < 0xC2-n)) break;

            if(byte == 0xC2-n && function[1] == char(0xA0))
                { function += 2; continue; } // U+00A0
            if(byte == 0xE3-n &&
               function[1] == char(0x80) && function[2] == char(0x80))
                { function += 3; continue; } // U+3000
            if(byte == 0xE2-n)
            {
                if(function[1] == char(0x81))
                {
                    if(function[2] != char(0x9F)) break;
                    function += 3; // U+205F
                    continue;
                }
                if(function[1] == char(0x80))
                if(function[2] == char(0xAF) || // U+202F
                   schar(function[2]) <= schar(0x8B) // U+2000..U+200B
                  )
                {
                    function += 3;
                    continue;
                }
            }
            break;
        } // while(true)
    } // SkipSpace(CharPtr& function)
}

// ---------------------------------------------------------------------------
// Return parse error message
// ---------------------------------------------------------------------------
template<typename Value_t>
const char* FunctionParserBase<Value_t>::ErrorMsg() const
{
    return ParseErrorMessage[mParseErrorType];
}


// ---------------------------------------------------------------------------
// Parse variables
// ---------------------------------------------------------------------------
template<typename Value_t>
bool FunctionParserBase<Value_t>::ParseVariables
(const std::string& inputVarString)
{
    if(mData->mVariablesString == inputVarString) return true;

    /* Delete existing variables from mNamePtrs */
    for(typename NamePtrsMap<Value_t>::iterator i =
            mData->mNamePtrs.begin();
        i != mData->mNamePtrs.end(); )
    {
        if(i->second.type == NameData<Value_t>::VARIABLE)
        {
            typename NamePtrsMap<Value_t>::iterator j (i);
            ++i;
            mData->mNamePtrs.erase(j);
        }
        else ++i;
    }
    mData->mVariablesString = inputVarString;

    const std::string& vars = mData->mVariablesString;
    const unsigned len = unsigned(vars.size());

    unsigned varNumber = VarBegin;

    const char* beginPtr = vars.c_str();
    const char* finalPtr = beginPtr + len;
    while(beginPtr < finalPtr)
    {
        SkipSpace(beginPtr);
        unsigned nameLength = readIdentifier<Value_t>(beginPtr);
        if(nameLength == 0 || (nameLength & 0x80000000U)) return false;
        const char* endPtr = beginPtr + nameLength;
        SkipSpace(endPtr);
        if(endPtr != finalPtr && *endPtr != ',') return false;

        std::pair<NamePtr, NameData<Value_t> > newName
            (NamePtr(beginPtr, nameLength),
             NameData<Value_t>(NameData<Value_t>::VARIABLE, varNumber++));

        if(!addNewNameData(mData->mNamePtrs, newName, true))
        {
            return false;
        }

        beginPtr = endPtr + 1;
    }

    mData->mVariablesAmount = varNumber - VarBegin;
    return true;
}

// ---------------------------------------------------------------------------
// Parse() public interface functions
// ---------------------------------------------------------------------------
template<typename Value_t>
int FunctionParserBase<Value_t>::Parse(const char* Function,
                                       const std::string& Vars,
                                       bool useDegrees)
{
    CopyOnWrite();

    if(!ParseVariables(Vars))
    {
        mParseErrorType = INVALID_VARS;
        return int(strlen(Function));
    }

    return ParseFunction(Function, useDegrees);
}

template<typename Value_t>
int FunctionParserBase<Value_t>::Parse(const std::string& Function,
                                       const std::string& Vars,
                                       bool useDegrees)
{
    CopyOnWrite();

    if(!ParseVariables(Vars))
    {
        mParseErrorType = INVALID_VARS;
        return int(Function.size());
    }

    return ParseFunction(Function.c_str(), useDegrees);
}


// ---------------------------------------------------------------------------
// Main parsing function
// ---------------------------------------------------------------------------
template<typename Value_t>
int FunctionParserBase<Value_t>::ParseFunction(const char* function,
                                               bool useDegrees)
{
    mUseDegreeConversion = useDegrees;
    mParseErrorType = FP_NO_ERROR;

    mData->mInlineVarNames.clear();
    mData->mByteCode.clear(); mData->mByteCode.reserve(128);
    mData->mImmed.clear(); mData->mImmed.reserve(128);
    mData->mStackSize = mStackPtr = 0;

    mHasByteCodeFlags = false;

    const char* ptr = Compile(function);
    mData->mInlineVarNames.clear();

    if(mHasByteCodeFlags)
    {
        for(unsigned i = unsigned(mData->mByteCode.size()); i-- > 0; )
            mData->mByteCode[i] &= ~0x80000000U;
    }

    if(mParseErrorType != FP_NO_ERROR) return int(mErrorLocation - function);

    assert(ptr); // Should never be null at this point. It's a bug otherwise.
    if(*ptr)
    {
        if(mDelimiterChar == 0 || *ptr != mDelimiterChar)
            mParseErrorType = EXPECT_OPERATOR;
        return int(ptr - function);
    }

#ifndef FP_USE_THREAD_SAFE_EVAL
    mData->mStack.resize(mData->mStackSize);
#endif

    return -1;
}


//=========================================================================
// Parsing and bytecode compiling functions
//=========================================================================
template<typename Value_t>
inline const char* FunctionParserBase<Value_t>::SetErrorType(ParseErrorType t,
                                                             const char* pos)
{
    mParseErrorType = t;
    mErrorLocation = pos;
    return 0;
}

template<typename Value_t>
inline void FunctionParserBase<Value_t>::incStackPtr()
{
    if(++mStackPtr > mData->mStackSize) ++(mData->mStackSize);
}

namespace
{
    const unsigned char powi_factor_table[128] =
    {
        0,1,0,0,0,0,0,0, 0, 0,0,0,0,0,0,3,/*   0 -  15 */
        0,0,0,0,0,0,0,0, 0, 5,0,3,0,0,3,0,/*  16 -  31 */
        0,0,0,0,0,0,0,3, 0, 0,0,0,0,5,0,0,/*  32 -  47 */
        0,0,5,3,0,0,3,5, 0, 3,0,0,3,0,0,3,/*  48 -  63 */
        0,0,0,0,0,0,0,0, 0, 0,0,3,0,0,3,0,/*  64 -  79 */
        0,9,0,0,0,5,0,3, 0, 0,5,7,0,0,0,5,/*  80 -  95 */
        0,0,0,3,5,0,3,0, 0, 3,0,0,3,0,5,3,/*  96 - 111 */
        0,0,3,5,0,9,0,7, 3,11,0,3,0,5,3,0,/* 112 - 127 */
    };

    inline int get_powi_factor(long abs_int_exponent)
    {
        if(abs_int_exponent >= int(sizeof(powi_factor_table))) return 0;
        return powi_factor_table[abs_int_exponent];
    }

#if 0
    int EstimatePowiComplexity(int abs_int_exponent)
    {
        int cost = 0;
        while(abs_int_exponent > 1)
        {
            int factor = get_powi_factor(abs_int_exponent);
            if(factor)
            {
                cost += EstimatePowiComplexity(factor);
                abs_int_exponent /= factor;
                continue;
            }
            if(!(abs_int_exponent & 1))
            {
                abs_int_exponent /= 2;
                cost += 3; // sqr
            }
            else
            {
                cost += 4; // dup+mul
                abs_int_exponent -= 1;
            }
        }
        return cost;
    }
#endif

    bool IsEligibleIntPowiExponent(long int_exponent)
    {
        if(int_exponent == 0) return false;
        long abs_int_exponent = int_exponent;
    #if 0
        int cost = 0;

        if(abs_int_exponent < 0)
        {
            cost += 11;
            abs_int_exponent = -abs_int_exponent;
        }

        cost += EstimatePowiComplexity(abs_int_exponent);

        return cost < (10*3 + 4*4);
    #else
        if(abs_int_exponent < 0) abs_int_exponent = -abs_int_exponent;

        return (abs_int_exponent >= 1)
            && (abs_int_exponent <= 46 ||
              (abs_int_exponent <= 1024 &&
              (abs_int_exponent & (abs_int_exponent - 1)) == 0));
    #endif
    }

#ifdef FP_EPSILON
    const double EpsilonOrZero = FP_EPSILON;
#else
    const double EpsilonOrZero = 0.0;
#endif

}

template<typename Value_t>
inline void FunctionParserBase<Value_t>::AddImmedOpcode(Value_t value)
{
    mData->mImmed.push_back(value);
    mData->mByteCode.push_back(cImmed);
}

template<typename Value_t>
inline void FunctionParserBase<Value_t>::CompilePowi(long abs_int_exponent)
{
    int num_muls=0;
    while(abs_int_exponent > 1)
    {
        long factor = get_powi_factor(abs_int_exponent);
        if(factor)
        {
            CompilePowi(factor);
            abs_int_exponent /= factor;
            continue;
        }
        if(!(abs_int_exponent & 1))
        {
            abs_int_exponent /= 2;
            mData->mByteCode.push_back(cSqr);
            // ^ Don't put AddFunctionOpcode here,
            //   it would slow down a great deal.
        }
        else
        {
            mData->mByteCode.push_back(cDup);
            incStackPtr();
            abs_int_exponent -= 1;
            ++num_muls;
        }
    }
    if(num_muls > 0)
    {
        mData->mByteCode.resize(mData->mByteCode.size()+num_muls, cMul);
        mStackPtr -= num_muls;
    }
}

template<typename Value_t>
inline bool FunctionParserBase<Value_t>::TryCompilePowi(Value_t original_immed)
{
    Value_t changed_immed = original_immed;
    for(int sqrt_count=0; /**/; ++sqrt_count)
    {
        long int_exponent = makeLongInteger(changed_immed);
        if(isLongInteger(changed_immed) &&
           IsEligibleIntPowiExponent(int_exponent))
        {
            long abs_int_exponent = int_exponent;
            if(abs_int_exponent < 0)
                abs_int_exponent = -abs_int_exponent;

            mData->mImmed.pop_back(); mData->mByteCode.pop_back();
            --mStackPtr;
            // ^Though the above is accounted for by the procedure
            // that generates cPow, we need it for correct cFetch
            // indexes in CompilePowi().

            while(sqrt_count > 0)
            {
                int opcode = cSqrt;
                if(sqrt_count == 1 && int_exponent < 0)
                {
                    opcode = cRSqrt;
                    int_exponent = -int_exponent;
                }
                mData->mByteCode.push_back(opcode);
                --sqrt_count;
            }
            if((abs_int_exponent & 1) == 0)
            {
                // This special rule fixes the optimization
                // shortcoming of (-x)^2 with minimal overhead.
                AddFunctionOpcode(cSqr);
                abs_int_exponent >>= 1;
            }
            CompilePowi(abs_int_exponent);
            if(int_exponent < 0) mData->mByteCode.push_back(cInv);
            ++mStackPtr; // Needed because cPow adding will assume this.
            return true;
        }
        if(sqrt_count >= 4) break;
        changed_immed += changed_immed;
    }

    // When we don't know whether x >= 0, we still know that
    // x^y can be safely converted into exp(y * log(x))
    // when y is _not_ integer, because we know that x >= 0.
    // Otherwise either expression will give a NaN.
    if(/*!isInteger(original_immed) ||*/
       IsNeverNegativeValueOpcode(mData->mByteCode[mData->mByteCode.size()-2]))
    {
        mData->mImmed.pop_back();
        mData->mByteCode.pop_back();
        //--mStackPtr; - accounted for by the procedure that generates cPow
        AddFunctionOpcode(cLog);
        AddImmedOpcode(original_immed);
        //incStackPtr(); - this and the next are redundant because...
        AddFunctionOpcode(cMul);
        //--mStackPtr;    - ...because the cImmed was popped earlier.
        AddFunctionOpcode(cExp);
        return true;
    }
    return false;
}

//#include "fpoptimizer/opcodename.hh"
// ^ needed only if FP_TRACE_BYTECODE_OPTIMIZATION() is used

template<typename Value_t>
inline void FunctionParserBase<Value_t>::AddFunctionOpcode(unsigned opcode)
{
#define FP_FLOAT_VERSION 1
#include "fp_opcode_add.inc"
#undef FP_FLOAT_VERSION
}

#ifdef FP_SUPPORT_LONG_INT_TYPE
template<>
inline void FunctionParserBase<long>::AddFunctionOpcode(unsigned opcode)
{
    typedef long Value_t;
#define FP_FLOAT_VERSION 0
#include "fp_opcode_add.inc"
#undef FP_FLOAT_VERSION
}
#endif

#ifdef FP_SUPPORT_GMP_INT_TYPE
template<>
inline void FunctionParserBase<GmpInt>::AddFunctionOpcode(unsigned opcode)
{
    typedef GmpInt Value_t;
#define FP_FLOAT_VERSION 0
#include "fp_opcode_add.inc"
#undef FP_FLOAT_VERSION
}
#endif

template<typename Value_t>
unsigned
FunctionParserBase<Value_t>::ParseIdentifier(const char* function)
{
    return readIdentifier<Value_t>(function);
}

template<typename Value_t>
std::pair<const char*, Value_t>
FunctionParserBase<Value_t>::ParseLiteral(const char* function)
{
    char* endptr;
#if 0 /* Profile the hex literal parser */
    if(function[0]=='0' && function[1]=='x')
    {
        // Parse hexadecimal literal if fp_parseLiteral didn't already
        Value_t val = parseHexLiteral<Value_t>(function+2, &endptr);
        if(endptr == function+2)
            return std::pair<const char*,Value_t> (function, Value_t());
        return std::pair<const char*, Value_t> (endptr, val);
    }
#endif
    Value_t val = fp_parseLiteral<Value_t>(function, &endptr);

    if(endptr == function+1 && function[0] == '0' && function[1] == 'x')
    {
        // Parse hexadecimal literal if fp_parseLiteral didn't already
        val = parseHexLiteral<Value_t>(function+2, &endptr);
        if(endptr == function+2)
            return std::pair<const char*,Value_t> (function, Value_t());
    }
    else if(endptr == function)
        return std::pair<const char*,Value_t> (function, Value_t());

    return std::pair<const char*,Value_t> (endptr, val);
}

#ifdef FP_SUPPORT_MPFR_FLOAT_TYPE
template<>
std::pair<const char*, MpfrFloat>
FunctionParserBase<MpfrFloat>::ParseLiteral(const char* function)
{
    char* endPtr;
    const MpfrFloat val = MpfrFloat::parseString(function, &endPtr);
    if(endPtr == function)
        return std::pair<const char*,MpfrFloat> (function, MpfrFloat());
    return std::pair<const char*,MpfrFloat> (endPtr, val);
}
#endif

#ifdef FP_SUPPORT_GMP_INT_TYPE
template<>
std::pair<const char*, GmpInt>
FunctionParserBase<GmpInt>::ParseLiteral(const char* function)
{
    char* endPtr;
    const GmpInt val = GmpInt::parseString(function, &endPtr);
    if(endPtr == function)
        return std::pair<const char*,GmpInt> (function, GmpInt());
    return std::pair<const char*,GmpInt> (endPtr, val);
}
#endif


template<typename Value_t>
inline const char*
FunctionParserBase<Value_t>::CompileLiteral(const char* function)
{
    std::pair<const char*, Value_t> result = ParseLiteral(function);

    if(result.first == function)
        return SetErrorType(SYNTAX_ERROR, result.first);

    AddImmedOpcode(result.second);
    incStackPtr();
    SkipSpace(result.first);
    return result.first;
}

template<typename Value_t>
const char* FunctionParserBase<Value_t>::CompileIf(const char* function)
{
    if(*function != '(') return SetErrorType(EXPECT_PARENTH_FUNC, function);

    function = CompileExpression(function+1);
    if(!function) return 0;
    if(*function != ',')
        return SetErrorType(noCommaError<Value_t>(*function), function);

    OPCODE opcode = cIf;
    if(mData->mByteCode.back() == cNotNot) mData->mByteCode.pop_back();
    if(IsNeverNegativeValueOpcode(mData->mByteCode.back()))
    {
        // If we know that the condition to be tested is always
        // a positive value (such as when produced by "x<y"),
        // we can use the faster opcode to evaluate it.
        // cIf tests whether fabs(cond) >= 0.5,
        // cAbsIf simply tests whether cond >= 0.5.
        opcode = cAbsIf;
    }

    mData->mByteCode.push_back(opcode);
    const unsigned curByteCodeSize = unsigned(mData->mByteCode.size());
    PushOpcodeParam<false>(0); // Jump index; to be set later
    PushOpcodeParam<true> (0); // Immed jump index; to be set later

    --mStackPtr;

    function = CompileExpression(function + 1);
    if(!function) return 0;
    if(*function != ',')
        return SetErrorType(noCommaError<Value_t>(*function), function);

    mData->mByteCode.push_back(cJump);
    const unsigned curByteCodeSize2 = unsigned(mData->mByteCode.size());
    const unsigned curImmedSize2 = unsigned(mData->mImmed.size());
    PushOpcodeParam<false>(0); // Jump index; to be set later
    PushOpcodeParam<true> (0); // Immed jump index; to be set later

    --mStackPtr;

    function = CompileExpression(function + 1);
    if(!function) return 0;
    if(*function != ')')
        return SetErrorType(noParenthError<Value_t>(*function), function);

    PutOpcodeParamAt<true> ( mData->mByteCode.back(), unsigned(mData->mByteCode.size()-1) );
    // ^Necessary for guarding against if(x,1,2)+1 being changed
    //  into if(x,1,3) by fp_opcode_add.inc

    // Set jump indices
    PutOpcodeParamAt<false>( curByteCodeSize2+1, curByteCodeSize );
    PutOpcodeParamAt<false>( curImmedSize2,      curByteCodeSize+1 );
    PutOpcodeParamAt<false>( unsigned(mData->mByteCode.size())-1, curByteCodeSize2);
    PutOpcodeParamAt<false>( unsigned(mData->mImmed.size()),      curByteCodeSize2+1);

    ++function;
    SkipSpace(function);
    return function;
}

template<typename Value_t>
const char* FunctionParserBase<Value_t>::CompileFunctionParams
(const char* function, unsigned requiredParams)
{
    if(*function != '(') return SetErrorType(EXPECT_PARENTH_FUNC, function);

    if(requiredParams > 0)
    {
        const char* function_end = CompileExpression(function+1);
        if(!function_end)
        {
            // If an error occurred, verify whether it was caused by ()
            ++function;
            SkipSpace(function);
            if(*function == ')')
                return SetErrorType(ILL_PARAMS_AMOUNT, function);
            // Not caused by (), use the error message given by CompileExpression()
            return 0;
        }
        function = function_end;

        for(unsigned i = 1; i < requiredParams; ++i)
        {
            if(*function != ',')
                return SetErrorType(noCommaError<Value_t>(*function), function);

            function = CompileExpression(function+1);
            if(!function) return 0;
        }
        // No need for incStackPtr() because each parse parameter calls it
        mStackPtr -= requiredParams-1;
    }
    else
    {
        incStackPtr(); // return value of function is pushed onto the stack
        ++function;
        SkipSpace(function);
    }

    if(*function != ')')
        return SetErrorType(noParenthError<Value_t>(*function), function);
    ++function;
    SkipSpace(function);
    return function;
}

template<typename Value_t>
const char* FunctionParserBase<Value_t>::CompileElement(const char* function)
{
    if(BeginsLiteral<Value_t>( (unsigned char) *function))
        return CompileLiteral(function);

    unsigned nameLength = readIdentifier<Value_t>(function);
    if(nameLength == 0)
    {
        // No identifier found
        if(*function == '(') return CompileParenthesis(function);
        if(*function == ')') return SetErrorType(MISM_PARENTH, function);
        return SetErrorType(SYNTAX_ERROR, function);
    }

    // Function, variable or constant
    if(nameLength & 0x80000000U) // Function
    {
        OPCODE func_opcode = OPCODE( (nameLength >> 16) & 0x7FFF );
        return CompileFunction(function + (nameLength & 0xFFFF), func_opcode);
    }

    NamePtr name(function, nameLength);
    const char* endPtr = function + nameLength;
    SkipSpace(endPtr);

    typename NamePtrsMap<Value_t>::iterator nameIter =
        mData->mNamePtrs.find(name);
    if(nameIter == mData->mNamePtrs.end())
    {
        // Check if it's an inline variable:
        for(typename Data::InlineVarNamesContainer::reverse_iterator iter =
                mData->mInlineVarNames.rbegin();
            iter != mData->mInlineVarNames.rend();
            ++iter)
        {
            if(name == iter->mName)
            {
                if( iter->mFetchIndex+1 == mStackPtr)
                {
                    mData->mByteCode.push_back(cDup);
                }
                else
                {
                    mData->mByteCode.push_back(cFetch);
                    PushOpcodeParam<true>(iter->mFetchIndex);
                }
                incStackPtr();
                return endPtr;
            }
        }

        return SetErrorType(UNKNOWN_IDENTIFIER, function);
    }

    const NameData<Value_t>* nameData = &nameIter->second;
    switch(nameData->type)
    {
      case NameData<Value_t>::VARIABLE: // is variable
          if(unlikely(!mData->mByteCode.empty() &&
                      mData->mByteCode.back() == nameData->index))
              mData->mByteCode.push_back(cDup);
          else
              mData->mByteCode.push_back(nameData->index);
          incStackPtr();
          return endPtr;

      case NameData<Value_t>::CONSTANT: // is constant
          AddImmedOpcode(nameData->value);
          incStackPtr();
          return endPtr;

      case NameData<Value_t>::UNIT: // is unit (error if appears here)
          break;

      case NameData<Value_t>::FUNC_PTR: // is C++ function
          function = CompileFunctionParams
              (endPtr, mData->mFuncPtrs[nameData->index].mParams);
          //if(!function) return 0;
          mData->mByteCode.push_back(cFCall);
          PushOpcodeParam<true>(nameData->index);
          return function;

      case NameData<Value_t>::PARSER_PTR: // is FunctionParser
          function = CompileFunctionParams
              (endPtr, mData->mFuncParsers[nameData->index].mParams);
          //if(!function) return 0;
          mData->mByteCode.push_back(cPCall);
          PushOpcodeParam<true>(nameData->index);
          return function;
    }

    // When it's an unit (or unrecognized type):
    return SetErrorType(SYNTAX_ERROR, function);
}

template<typename Value_t>
inline const char* FunctionParserBase<Value_t>::CompileFunction
(const char* function, unsigned func_opcode)
{
    SkipSpace(function);
    const FuncDefinition& funcDef = Functions[func_opcode];

    if(func_opcode == cIf) // "if" is a special case
        return CompileIf(function);

    unsigned requiredParams = funcDef.params;
#ifndef FP_DISABLE_EVAL
    if(func_opcode == cEval)
        requiredParams = mData->mVariablesAmount;
#endif

    function = CompileFunctionParams(function, requiredParams);
    if(!function) return 0;

    if(mUseDegreeConversion)
    {
        if(funcDef.flags & FuncDefinition::AngleIn)
            AddFunctionOpcode(cRad);

        AddFunctionOpcode(func_opcode);

        if(funcDef.flags & FuncDefinition::AngleOut)
            AddFunctionOpcode(cDeg);
    }
    else
    {
        AddFunctionOpcode(func_opcode);
    }
    return function;
}

template<typename Value_t>
inline const char*
FunctionParserBase<Value_t>::CompileParenthesis(const char* function)
{
    ++function; // Skip '('

    SkipSpace(function);
    if(*function == ')') return SetErrorType(EMPTY_PARENTH, function);
    function = CompileExpression(function);
    if(!function) return 0;

    if(*function != ')') return SetErrorType(MISSING_PARENTH, function);
    ++function; // Skip ')'

    SkipSpace(function);
    return function;
}

template<typename Value_t>
const char*
FunctionParserBase<Value_t>::CompilePossibleUnit(const char* function)
{
    unsigned nameLength = readIdentifier<Value_t>(function);
    if(nameLength & 0x80000000U) return function; // built-in function name
    if(nameLength != 0)
    {
        NamePtr name(function, nameLength);

        typename NamePtrsMap<Value_t>::iterator nameIter =
            mData->mNamePtrs.find(name);
        if(nameIter != mData->mNamePtrs.end())
        {
            const NameData<Value_t>* nameData = &nameIter->second;
            if(nameData->type == NameData<Value_t>::UNIT)
            {
                AddImmedOpcode(nameData->value);
                incStackPtr();
                AddFunctionOpcode(cMul);
                --mStackPtr;

                const char* endPtr = function + nameLength;
                SkipSpace(endPtr);
                return endPtr;
            }
        }
    }

    return function;
}

template<typename Value_t>
inline const char*
FunctionParserBase<Value_t>::CompilePow(const char* function)
{
    function = CompileElement(function);
    if(!function) return 0;
    function = CompilePossibleUnit(function);

    if(*function == '^')
    {
        ++function;
        SkipSpace(function);

        unsigned op = cPow;
        if(mData->mByteCode.back() == cImmed)
        {
            if(mData->mImmed.back() == fp_const_e<Value_t>())
                { op = cExp;  mData->mByteCode.pop_back();
                    mData->mImmed.pop_back(); --mStackPtr; }
            else if(mData->mImmed.back() == Value_t(2))
                { op = cExp2; mData->mByteCode.pop_back();
                    mData->mImmed.pop_back(); --mStackPtr; }
        }

        function = CompileUnaryMinus(function);
        if(!function) return 0;

        // add opcode
        AddFunctionOpcode(op);

        if(op == cPow) --mStackPtr;
    }
    return function;
}

/* Currently the power operator is skipped for integral types because its
   usefulness with them is questionable, and in the case of GmpIng, for safety
   reasons.
   - With long int almost any power, except for very small ones, would
     overflow the result, so the usefulness of this is rather questionable.
   - With GmpInt the power operator could be easily abused to make the program
     run out of memory (think of a function like "10^10^10^10^1000000").
*/
#ifdef FP_SUPPORT_LONG_INT_TYPE
template<>
inline const char*
FunctionParserBase<long>::CompilePow(const char* function)
{
    function = CompileElement(function);
    if(!function) return 0;
    return CompilePossibleUnit(function);
}
#endif

#ifdef FP_SUPPORT_GMP_INT_TYPE
template<>
inline const char*
FunctionParserBase<GmpInt>::CompilePow(const char* function)
{
    function = CompileElement(function);
    if(!function) return 0;
    return CompilePossibleUnit(function);
}
#endif

template<typename Value_t>
inline const char*
FunctionParserBase<Value_t>::CompileUnaryMinus(const char* function)
{
    char op = *function;
    switch(op)
    {
        case '-':
        case '!':
            ++function;
            SkipSpace(function);

            function = CompileUnaryMinus(function);
            if(!function) return 0;

            AddFunctionOpcode(op=='-' ? cNeg : cNot);

            return function;
        default: break;
    }
    return CompilePow(function);
}

template<typename Value_t>
inline const char*
FunctionParserBase<Value_t>::CompileMult(const char* function)
{
    function = CompileUnaryMinus(function);
    if(!function) return 0;

    Value_t pending_immed(1);
    #define FP_FlushImmed(do_reset) \
        if(pending_immed != Value_t(1)) \
        { \
            unsigned op = cMul; \
            if(!IsIntType<Value_t>::result && mData->mByteCode.back() == cInv) \
            { \
                /* (...) cInv 5 cMul -> (...) 5 cRDiv */ \
                /*           ^               ^      | */ \
                mData->mByteCode.pop_back(); \
                op = cRDiv; \
            } \
            AddImmedOpcode(pending_immed); \
            incStackPtr(); \
            AddFunctionOpcode(op); \
            --mStackPtr; \
            if(do_reset) pending_immed = Value_t(1); \
        }
    while(true)
    {
        char c = *function;
        if(c == '%')
        {
            FP_FlushImmed(true);
            ++function;
            SkipSpace(function);
            function = CompileUnaryMinus(function);
            if(!function) return 0;
            AddFunctionOpcode(cMod);
            --mStackPtr;
            continue;
        }
        if(c != '*' && c != '/') break;

        bool safe_cumulation = (c == '*' || !IsIntType<Value_t>::result);
        if(!safe_cumulation)
        {
            FP_FlushImmed(true);
        }

        ++function;
        SkipSpace(function);
        if(mData->mByteCode.back() == cImmed
        && (safe_cumulation
         || mData->mImmed.back() == Value_t(1)))
        {
            // 5 (...) cMul --> (...)      ||| 5 cMul
            // 5 (...) cDiv --> (...) cInv ||| 5 cMul
            //  ^          |              ^
            pending_immed *= mData->mImmed.back();
            mData->mImmed.pop_back();
            mData->mByteCode.pop_back();
            --mStackPtr;
            function = CompileUnaryMinus(function);
            if(!function) return 0;
            if(c == '/')
                AddFunctionOpcode(cInv);
            continue;
        }
        if(safe_cumulation
        && mData->mByteCode.back() == cMul
        && mData->mByteCode[mData->mByteCode.size()-2] == cImmed)
        {
            // (:::) 5 cMul (...) cMul -> (:::) (...) cMul  ||| 5 cMul
            // (:::) 5 cMul (...) cDiv -> (:::) (...) cDiv  ||| 5 cMul
            //             ^                   ^
            pending_immed *= mData->mImmed.back();
            mData->mImmed.pop_back();
            mData->mByteCode.pop_back();
            mData->mByteCode.pop_back();
        }
        // cDiv is not tested here because the bytecode
        // optimizer will convert this kind of cDivs into cMuls.
        bool lhs_inverted = false;
        if(!IsIntType<Value_t>::result && c == '*'
        && mData->mByteCode.back() == cInv)
        {
            // (:::) cInv (...) cMul -> (:::) (...) cRDiv
            // (:::) cInv (...) cDiv -> (:::) (...) cMul cInv
            //           ^                   ^            |
            mData->mByteCode.pop_back();
            lhs_inverted = true;
        }
        function = CompileUnaryMinus(function);
        if(!function) return 0;
        if(safe_cumulation
        && mData->mByteCode.back() == cMul
        && mData->mByteCode[mData->mByteCode.size()-2] == cImmed)
        {
            // (:::) (...) 5 cMul cMul -> (:::) (...) cMul  |||  5 Mul
            // (:::) (...) 5 cMul cDiv -> (:::) (...) cDiv  ||| /5 Mul
            //                   ^                        ^
            if(c == '*')
                pending_immed *= mData->mImmed.back();
            else
                pending_immed /= mData->mImmed.back();
            mData->mImmed.pop_back();
            mData->mByteCode.pop_back();
            mData->mByteCode.pop_back();
        }
        else
        if(safe_cumulation
        && mData->mByteCode.back() == cRDiv
        && mData->mByteCode[mData->mByteCode.size()-2] == cImmed)
        {
            // (:::) (...) 5 cRDiv cMul -> (:::) (...) cDiv  |||  5 cMul
            // (:::) (...) 5 cRDiv cDiv -> (:::) (...) cMul  ||| /5 cMul
            //                    ^                   ^
            if(c == '*')
                { c = '/'; pending_immed *= mData->mImmed.back(); }
            else
                { c = '*'; pending_immed /= mData->mImmed.back(); }
            mData->mImmed.pop_back();
            mData->mByteCode.pop_back();
            mData->mByteCode.pop_back();
        }
        if(!lhs_inverted) // if (/x/y) was changed to /(x*y), add missing cInv
        {
            AddFunctionOpcode(c == '*' ? cMul : cDiv);
            --mStackPtr;
        }
        else if(c == '*') // (/x)*y -> rdiv(x,y)
        {
            AddFunctionOpcode(cRDiv);
            --mStackPtr;
        }
        else // (/x)/y -> /(x*y)
        {
            AddFunctionOpcode(cMul);
            --mStackPtr;
            AddFunctionOpcode(cInv);
        }
    }
    FP_FlushImmed(false);
    #undef FP_FlushImmed
    return function;
}

template<typename Value_t>
inline const char*
FunctionParserBase<Value_t>::CompileAddition(const char* function)
{
    function = CompileMult(function);
    if(!function) return 0;

    Value_t pending_immed(0);
    #define FP_FlushImmed(do_reset) \
        if(pending_immed != Value_t(0)) \
        { \
            unsigned op = cAdd; \
            if(mData->mByteCode.back() == cNeg) \
            { \
                /* (...) cNeg 5 cAdd -> (...) 5 cRSub */ \
                /*           ^               ^      | */ \
                mData->mByteCode.pop_back(); \
                op = cRSub; \
            } \
            AddImmedOpcode(pending_immed); \
            incStackPtr(); \
            AddFunctionOpcode(op); \
            --mStackPtr; \
            if(do_reset) pending_immed = Value_t(0); \
        }
    while(true)
    {
        char c = *function;
        if(c != '+' && c != '-') break;
        ++function;
        SkipSpace(function);
        if(mData->mByteCode.back() == cImmed)
        {
            // 5 (...) cAdd --> (...)      ||| 5 cAdd
            // 5 (...) cSub --> (...) cNeg ||| 5 cAdd
            //  ^          |              ^
            pending_immed += mData->mImmed.back();
            mData->mImmed.pop_back();
            mData->mByteCode.pop_back();
            --mStackPtr;
            function = CompileMult(function);
            if(!function) return 0;
            if(c == '-')
                AddFunctionOpcode(cNeg);
            continue;
        }
        if(mData->mByteCode.back() == cAdd
        && mData->mByteCode[mData->mByteCode.size()-2] == cImmed)
        {
            // (:::) 5 cAdd (...) cAdd -> (:::) (...) cAdd  ||| 5 cAdd
            // (:::) 5 cAdd (...) cSub -> (:::) (...) cSub  ||| 5 cAdd
            //             ^                   ^
            pending_immed += mData->mImmed.back();
            mData->mImmed.pop_back();
            mData->mByteCode.pop_back();
            mData->mByteCode.pop_back();
        }
        // cSub is not tested here because the bytecode
        // optimizer will convert this kind of cSubs into cAdds.
        bool lhs_negated = false;
        if(mData->mByteCode.back() == cNeg)
        {
            // (:::) cNeg (...) cAdd -> (:::) (...) cRSub
            // (:::) cNeg (...) cSub -> (:::) (...) cAdd cNeg
            //           ^                   ^            |
            mData->mByteCode.pop_back();
            lhs_negated = true;
        }
        function = CompileMult(function);
        if(!function) return 0;
        if(mData->mByteCode.back() == cAdd
        && mData->mByteCode[mData->mByteCode.size()-2] == cImmed)
        {
            // (:::) (...) 5 cAdd cAdd -> (:::) (...) cAdd  |||  5 Add
            // (:::) (...) 5 cAdd cSub -> (:::) (...) cSub  ||| -5 Add
            //                   ^                        ^
            if(c == '+')
                pending_immed += mData->mImmed.back();
            else
                pending_immed -= mData->mImmed.back();
            mData->mImmed.pop_back();
            mData->mByteCode.pop_back();
            mData->mByteCode.pop_back();
        }
        else
        if(mData->mByteCode.back() == cRSub
        && mData->mByteCode[mData->mByteCode.size()-2] == cImmed)
        {
            // (:::) (...) 5 cRSub cAdd -> (:::) (...) cSub  |||  5 cAdd
            // (:::) (...) 5 cRSub cSub -> (:::) (...) cAdd  ||| -5 cAdd
            //                    ^                   ^
            if(c == '+')
                { c = '-'; pending_immed += mData->mImmed.back(); }
            else
                { c = '+'; pending_immed -= mData->mImmed.back(); }
            mData->mImmed.pop_back();
            mData->mByteCode.pop_back();
            mData->mByteCode.pop_back();
        }
        if(!lhs_negated) // if (-x-y) was changed to -(x+y), add missing cNeg
        {
            AddFunctionOpcode(c == '+' ? cAdd : cSub);
            --mStackPtr;
        }
        else if(c == '+') // (-x)+y -> rsub(x,y)
        {
            AddFunctionOpcode(cRSub);
            --mStackPtr;
        }
        else // (-x)-y -> -(x+y)
        {
            AddFunctionOpcode(cAdd);
            --mStackPtr;
            AddFunctionOpcode(cNeg);
        }
    }
    FP_FlushImmed(false);
    #undef FP_FlushImmed
    return function;
}

template<typename Value_t>
inline const char*
FunctionParserBase<Value_t>::CompileComparison(const char* function)
{
    unsigned op=0;
    while(true)
    {
        function = CompileAddition(function);
        if(!function) return 0;

        if(op)
        {
            AddFunctionOpcode(op);
            --mStackPtr;
        }
        switch(*function)
        {
          case '=':
              ++function; op = cEqual; break;
          case '!':
              if(function[1] == '=')
              { function += 2; op = cNEqual; break; }
              // If '=' does not follow '!', a syntax error will
              // be generated at the outermost parsing level
              return function;
          case '<':
              if(function[1] == '=')
              { function += 2; op = cLessOrEq; break; }
              ++function; op = cLess; break;
          case '>':
              if(function[1] == '=')
              { function += 2; op = cGreaterOrEq; break; }
              ++function; op = cGreater; break;
          default: return function;
        }
        SkipSpace(function);
    }
    return function;
}

template<typename Value_t>
inline const char* FunctionParserBase<Value_t>::CompileAnd(const char* function)
{
    size_t param0end=0;
    while(true)
    {
        function = CompileComparison(function);
        if(!function) return 0;

        if(param0end)
        {
            if(mData->mByteCode.back() == cNotNot) mData->mByteCode.pop_back();

            AddFunctionOpcode(cAnd);
            --mStackPtr;
        }
        if(*function != '&') break;
        ++function;
        SkipSpace(function);
        param0end = mData->mByteCode.size();
    }
    return function;
}

template<typename Value_t>
const char* FunctionParserBase<Value_t>::CompileExpression(const char* function)
{
    size_t param0end=0;
    while(true)
    {
        SkipSpace(function);
        function = CompileAnd(function);
        if(!function) return 0;

        if(param0end)
        {
            if(mData->mByteCode.back() == cNotNot) mData->mByteCode.pop_back();

            AddFunctionOpcode(cOr);
            --mStackPtr;
        }
        if(*function != '|') break;
        ++function;
        param0end = mData->mByteCode.size();
    }
    return function;
}

template<typename Value_t>
const char* FunctionParserBase<Value_t>::Compile(const char* function)
{
    while(true)
    {
        // Check if an identifier appears as first token:
        SkipSpace(function);
        unsigned nameLength = readIdentifier<Value_t>(function);
        if(nameLength > 0 && !(nameLength & 0x80000000U))
        {
            typename Data::InlineVariable inlineVar =
                { NamePtr(function, nameLength), 0 };

            // Check if it's an unknown identifier:
            typename NamePtrsMap<Value_t>::iterator nameIter =
                mData->mNamePtrs.find(inlineVar.mName);
            if(nameIter == mData->mNamePtrs.end())
            {
                const char* function2 = function + nameLength;
                SkipSpace(function2);

                // Check if ":=" follows the unknown identifier:
                if(function2[0] == ':' && function2[1] == '=')
                {
                    // Parse the expression that follows and create the
                    // inline variable:
                    function2 = CompileExpression(function2 + 2);
                    if(!function2) return 0;
                    if(*function2 != ';') return function2;

                    inlineVar.mFetchIndex = mStackPtr - 1;
                    mData->mInlineVarNames.push_back(inlineVar);

                    // Continue with the expression after the ';':
                    function = function2 + 1;
                    continue;
                }
            }
        }
        break;
    }

    return CompileExpression(function);
}

template<typename Value_t> template<bool PutFlag>
inline void FunctionParserBase<Value_t>::PushOpcodeParam
    (unsigned value)
{
    mData->mByteCode.push_back(value | (PutFlag ? 0x80000000U : 0u));
    if(PutFlag) mHasByteCodeFlags = true;
}

template<typename Value_t> template<bool PutFlag>
inline void FunctionParserBase<Value_t>::PutOpcodeParamAt
    (unsigned value, unsigned offset)
{
    mData->mByteCode[offset] = value | (PutFlag ? 0x80000000U : 0u);
    if(PutFlag) mHasByteCodeFlags = true;
}

//===========================================================================
// Function evaluation
//===========================================================================
template<typename Value_t>
Value_t FunctionParserBase<Value_t>::Eval(const Value_t* Vars)
{
    if(mParseErrorType != FP_NO_ERROR) return Value_t(0);

    const unsigned* const byteCode = &(mData->mByteCode[0]);
    const Value_t* const immed = mData->mImmed.empty() ? 0 : &(mData->mImmed[0]);
    const unsigned byteCodeSize = unsigned(mData->mByteCode.size());
    unsigned IP, DP=0;
    int SP=-1;

#ifdef FP_USE_THREAD_SAFE_EVAL
    /* If Eval() may be called by multiple threads simultaneously,
     * then Eval() must allocate its own stack.
     */
#ifdef FP_USE_THREAD_SAFE_EVAL_WITH_ALLOCA
    /* alloca() allocates room from the hardware stack.
     * It is automatically freed when the function returns.
     */
    Value_t* const Stack = (Value_t*)alloca(mData->mStackSize*sizeof(Value_t));
#else
    /* Allocate from the heap. Ensure that it is freed
     * automatically no matter which exit path is taken.
     */
    struct AutoDealloc
    {
        Value_t* ptr;
        ~AutoDealloc() { delete[] ptr; }
    } AutoDeallocStack = { new Value_t[mData->mStackSize] };
    Value_t*& Stack = AutoDeallocStack.ptr;
#endif
#else
    /* No thread safety, so use a global stack. */
    std::vector<Value_t>& Stack = mData->mStack;
#endif

    for(IP=0; IP<byteCodeSize; ++IP)
    {
        switch(byteCode[IP])
        {
// Functions:
          case   cAbs: Stack[SP] = fp_abs(Stack[SP]); break;

          case  cAcos:
#           ifndef FP_NO_EVALUATION_CHECKS
              if(Stack[SP] < Value_t(-1) || Stack[SP] > Value_t(1))
              { mEvalErrorType=4; return Value_t(0); }
#           endif
              Stack[SP] = fp_acos(Stack[SP]); break;

          case cAcosh:
#           ifndef FP_NO_EVALUATION_CHECKS
              if(Stack[SP] < Value_t(1))
              { mEvalErrorType=4; return Value_t(0); }
#           endif
              Stack[SP] = fp_acosh(Stack[SP]); break;

          case  cAsin:
#           ifndef FP_NO_EVALUATION_CHECKS
              if(Stack[SP] < Value_t(-1) || Stack[SP] > Value_t(1))
              { mEvalErrorType=4; return Value_t(0); }
#           endif
              Stack[SP] = fp_asin(Stack[SP]); break;

          case cAsinh: Stack[SP] = fp_asinh(Stack[SP]); break;

          case  cAtan: Stack[SP] = fp_atan(Stack[SP]); break;

          case cAtan2: Stack[SP-1] = fp_atan2(Stack[SP-1], Stack[SP]);
                       --SP; break;

          case cAtanh:
#           ifndef FP_NO_EVALUATION_CHECKS
              if(Stack[SP] <= Value_t(-1) || Stack[SP] >= Value_t(1))
              { mEvalErrorType=4; return Value_t(0); }
#           endif
              Stack[SP] = fp_atanh(Stack[SP]); break;

          case  cCbrt: Stack[SP] = fp_cbrt(Stack[SP]); break;

          case  cCeil: Stack[SP] = fp_ceil(Stack[SP]); break;

          case   cCos: Stack[SP] = fp_cos(Stack[SP]); break;

          case  cCosh: Stack[SP] = fp_cosh(Stack[SP]); break;

          case   cCot:
              {
                  const Value_t t = fp_tan(Stack[SP]);
#               ifndef FP_NO_EVALUATION_CHECKS
                  if(t == Value_t(0)) { mEvalErrorType=1; return Value_t(0); }
#               endif
                  Stack[SP] = Value_t(1)/t; break;
              }

          case   cCsc:
              {
                  const Value_t s = fp_sin(Stack[SP]);
#               ifndef FP_NO_EVALUATION_CHECKS
                  if(s == 0) { mEvalErrorType=1; return Value_t(0); }
#               endif
                  Stack[SP] = Value_t(1)/s; break;
              }


#       ifndef FP_DISABLE_EVAL
          case  cEval:
              {
                  const unsigned varAmount = mData->mVariablesAmount;
                  Value_t retVal = Value_t(0);
                  if(mEvalRecursionLevel == FP_EVAL_MAX_REC_LEVEL)
                  {
                      mEvalErrorType = 5;
                  }
                  else
                  {
                      ++mEvalRecursionLevel;
#                   ifndef FP_USE_THREAD_SAFE_EVAL
                      /* Eval() will use mData->mStack for its storage.
                       * Swap the current stack with an empty one.
                       * This is the not-thread-safe method.
                       */
                      std::vector<Value_t> tmpStack(Stack.size());
                      mData->mStack.swap(tmpStack);
                      retVal = Eval(&tmpStack[SP - varAmount + 1]);
                      mData->mStack.swap(tmpStack);
#                   else
                      /* Thread safety mode. We don't need to
                       * worry about stack reusing here, because
                       * each instance of Eval() will allocate
                       * their own stack.
                       */
                      retVal = Eval(&Stack[SP - varAmount + 1]);
#                   endif
                      --mEvalRecursionLevel;
                  }
                  SP -= varAmount-1;
                  Stack[SP] = retVal;
                  break;
              }
#       endif

          case   cExp: Stack[SP] = fp_exp(Stack[SP]); break;

          case   cExp2: Stack[SP] = fp_exp2(Stack[SP]); break;

          case cFloor: Stack[SP] = fp_floor(Stack[SP]); break;

          case cHypot:
              Stack[SP-1] = fp_hypot(Stack[SP-1], Stack[SP]);
              --SP; break;

          case    cIf:
                  if(fp_truth(Stack[SP--]))
                      IP += 2;
                  else
                  {
                      const unsigned* buf = &byteCode[IP+1];
                      IP = buf[0];
                      DP = buf[1];
                  }
                  break;

          case   cInt: Stack[SP] = fp_int(Stack[SP]); break;

          case   cLog:
#           ifndef FP_NO_EVALUATION_CHECKS
              if(!(Stack[SP] > Value_t(0)))
              { mEvalErrorType=3; return Value_t(0); }
#           endif
              Stack[SP] = fp_log(Stack[SP]); break;

          case cLog10:
#           ifndef FP_NO_EVALUATION_CHECKS
              if(!(Stack[SP] > Value_t(0)))
              { mEvalErrorType=3; return Value_t(0); }
#           endif
              Stack[SP] = fp_log10(Stack[SP]);
              break;

          case  cLog2:
#           ifndef FP_NO_EVALUATION_CHECKS
              if(!(Stack[SP] > Value_t(0)))
              { mEvalErrorType=3; return Value_t(0); }
#           endif
              Stack[SP] = fp_log2(Stack[SP]);
              break;

          case   cMax: Stack[SP-1] = fp_max(Stack[SP-1], Stack[SP]);
                       --SP; break;

          case   cMin: Stack[SP-1] = fp_min(Stack[SP-1], Stack[SP]);
                       --SP; break;

          case   cPow:
#           ifndef FP_NO_EVALUATION_CHECKS
              // x:Negative ^ y:NonInteger is failure,
              // except when the reciprocal of y forms an integer
              /*if(Stack[SP-1] < Value_t(0) &&
                 !isInteger(Stack[SP]) &&
                 !isInteger(1.0 / Stack[SP]))
              { mEvalErrorType=3; return Value_t(0); }*/
              // x:0 ^ y:negative is failure
              if(Stack[SP-1] == Value_t(0) &&
                 Stack[SP] < Value_t(0))
              { mEvalErrorType=3; return Value_t(0); }
#           endif
              Stack[SP-1] = fp_pow(Stack[SP-1], Stack[SP]);
              --SP; break;

          case  cTrunc: Stack[SP] = fp_trunc(Stack[SP]); break;

          case   cSec:
              {
                  const Value_t c = fp_cos(Stack[SP]);
#               ifndef FP_NO_EVALUATION_CHECKS
                  if(c == Value_t(0)) { mEvalErrorType=1; return Value_t(0); }
#               endif
                  Stack[SP] = Value_t(1)/c; break;
              }

          case   cSin: Stack[SP] = fp_sin(Stack[SP]); break;

          case  cSinh: Stack[SP] = fp_sinh(Stack[SP]); break;

          case  cSqrt:
#           ifndef FP_NO_EVALUATION_CHECKS
              if(Stack[SP] < Value_t(0)) { mEvalErrorType=2; return Value_t(0); }
#           endif
              Stack[SP] = fp_sqrt(Stack[SP]); break;

          case   cTan: Stack[SP] = fp_tan(Stack[SP]); break;

          case  cTanh: Stack[SP] = fp_tanh(Stack[SP]); break;


// Misc:
          case cImmed: Stack[++SP] = immed[DP++]; break;

          case  cJump:
              {
                  const unsigned* buf = &byteCode[IP+1];
                  IP = buf[0];
                  DP = buf[1];
                  break;
              }

// Operators:
          case   cNeg: Stack[SP] = -Stack[SP]; break;
          case   cAdd: Stack[SP-1] += Stack[SP]; --SP; break;
          case   cSub: Stack[SP-1] -= Stack[SP]; --SP; break;
          case   cMul: Stack[SP-1] *= Stack[SP]; --SP; break;

          case   cDiv:
#           ifndef FP_NO_EVALUATION_CHECKS
              if(Stack[SP] == Value_t(0))
              { mEvalErrorType=1; return Value_t(0); }
#           else
              if(IsIntType<Value_t>::result && Stack[SP] == Value_t(0))
              { mEvalErrorType=1; return Value_t(0); }
#           endif
              Stack[SP-1] /= Stack[SP]; --SP; break;

          case   cMod:
              if(Stack[SP] == Value_t(0))
              { mEvalErrorType=1; return Value_t(0); }
              Stack[SP-1] = fp_mod(Stack[SP-1], Stack[SP]);
              --SP; break;

          case cEqual:
              Stack[SP-1] = fp_equal(Stack[SP-1], Stack[SP]);
              --SP; break;

          case cNEqual:
              Stack[SP-1] = fp_nequal(Stack[SP-1], Stack[SP]);
              --SP; break;

          case  cLess:
              Stack[SP-1] = fp_less(Stack[SP-1], Stack[SP]);
              --SP; break;

          case  cLessOrEq:
              Stack[SP-1] = fp_lessOrEq(Stack[SP-1], Stack[SP]);
              --SP; break;

          case cGreater:
              Stack[SP-1] = fp_less(Stack[SP], Stack[SP-1]);
              --SP; break;

          case cGreaterOrEq:
              Stack[SP-1] = fp_lessOrEq(Stack[SP], Stack[SP-1]);
              --SP; break;

          case   cNot: Stack[SP] = fp_not(Stack[SP]); break;

          case cNotNot: Stack[SP] = fp_notNot(Stack[SP]); break;

          case   cAnd:
              Stack[SP-1] = fp_and(Stack[SP-1], Stack[SP]);
              --SP; break;

          case    cOr:
              Stack[SP-1] = fp_or(Stack[SP-1], Stack[SP]);
              --SP; break;

// Degrees-radians conversion:
          case   cDeg: Stack[SP] = RadiansToDegrees(Stack[SP]); break;
          case   cRad: Stack[SP] = DegreesToRadians(Stack[SP]); break;

// User-defined function calls:
          case cFCall:
              {
                  unsigned index = byteCode[++IP];
                  unsigned params = mData->mFuncPtrs[index].mParams;
                  Value_t retVal =
                      mData->mFuncPtrs[index].mFuncPtr(&Stack[SP-params+1]);
                  SP -= int(params)-1;
                  Stack[SP] = retVal;
                  break;
              }

          case cPCall:
              {
                  unsigned index = byteCode[++IP];
                  unsigned params = mData->mFuncParsers[index].mParams;
                  Value_t retVal =
                      mData->mFuncParsers[index].mParserPtr->Eval
                      (&Stack[SP-params+1]);
                  SP -= int(params)-1;
                  Stack[SP] = retVal;
                  const int error =
                      mData->mFuncParsers[index].mParserPtr->EvalError();
                  if(error)
                  {
                      mEvalErrorType = error;
                      return 0;
                  }
                  break;
              }


          case   cFetch:
              {
                  unsigned stackOffs = byteCode[++IP];
                  Stack[SP+1] = Stack[stackOffs]; ++SP;
                  break;
              }

#ifdef FP_SUPPORT_OPTIMIZER
          case   cPopNMov:
              {
                  unsigned stackOffs_target = byteCode[++IP];
                  unsigned stackOffs_source = byteCode[++IP];
                  Stack[stackOffs_target] = Stack[stackOffs_source];
                  SP = stackOffs_target;
                  break;
              }

          case  cLog2by:
#           ifndef FP_NO_EVALUATION_CHECKS
              if(Stack[SP-1] <= Value_t(0))
              { mEvalErrorType=3; return Value_t(0); }
#           endif
              Stack[SP-1] = fp_log2(Stack[SP-1]) * Stack[SP];
              --SP;
              break;

          case cNop: break;
#endif // FP_SUPPORT_OPTIMIZER

          case cSinCos:
              fp_sinCos(Stack[SP], Stack[SP+1], Stack[SP]);
              ++SP;
              break;

          case cAbsNot:
              Stack[SP] = fp_absNot(Stack[SP]); break;
          case cAbsNotNot:
              Stack[SP] = fp_absNotNot(Stack[SP]); break;
          case cAbsAnd:
              Stack[SP-1] = fp_absAnd(Stack[SP-1], Stack[SP]);
              --SP; break;
          case cAbsOr:
              Stack[SP-1] = fp_absOr(Stack[SP-1], Stack[SP]);
              --SP; break;
          case cAbsIf:
              if(fp_absTruth(Stack[SP--]))
                  IP += 2;
              else
              {
                  const unsigned* buf = &byteCode[IP+1];
                  IP = buf[0];
                  DP = buf[1];
              }
              break;

          case   cDup: Stack[SP+1] = Stack[SP]; ++SP; break;

          case   cInv:
#           ifndef FP_NO_EVALUATION_CHECKS
              if(Stack[SP] == Value_t(0))
              { mEvalErrorType=1; return Value_t(0); }
#           else
              if(IsIntType<Value_t>::result && Stack[SP] == Value_t(0))
              { mEvalErrorType=1; return Value_t(0); }
#           endif
              Stack[SP] = Value_t(1)/Stack[SP];
              break;

          case   cSqr:
              Stack[SP] = Stack[SP]*Stack[SP];
              break;

          case   cRDiv:
#           ifndef FP_NO_EVALUATION_CHECKS
              if(Stack[SP-1] == Value_t(0))
              { mEvalErrorType=1; return Value_t(0); }
#           else
              if(IsIntType<Value_t>::result && Stack[SP-1] == Value_t(0))
              { mEvalErrorType=1; return Value_t(0); }
#           endif
              Stack[SP-1] = Stack[SP] / Stack[SP-1]; --SP; break;

          case   cRSub: Stack[SP-1] = Stack[SP] - Stack[SP-1]; --SP; break;

          case   cRSqrt:
#           ifndef FP_NO_EVALUATION_CHECKS
              if(Stack[SP] == Value_t(0))
              { mEvalErrorType=1; return Value_t(0); }
#           endif
              Stack[SP] = Value_t(1) / fp_sqrt(Stack[SP]); break;


// Variables:
          default:
              Stack[++SP] = Vars[byteCode[IP]-VarBegin];
        }
    }

    mEvalErrorType=0;
    return Stack[SP];
}


//===========================================================================
// Variable deduction
//===========================================================================
namespace
{
    template<typename Value_t>
    int deduceVariables(FunctionParserBase<Value_t>& fParser,
                        const char* funcStr,
                        std::string& destVarString,
                        int* amountOfVariablesFound,
                        std::vector<std::string>* destVarNames,
                        bool useDegrees)
    {
        typedef std::set<std::string> StrSet;
        StrSet varNames;

        int oldIndex = -1;

        while(true)
        {
            destVarString.clear();
            for(StrSet::iterator iter = varNames.begin();
                iter != varNames.end();
                ++iter)
            {
                if(iter != varNames.begin()) destVarString += ",";
                destVarString += *iter;
            }

            const int index =
                fParser.Parse(funcStr, destVarString, useDegrees);
            if(index < 0) break;
            if(index == oldIndex) return index;

            unsigned nameLength = readIdentifier<Value_t>(funcStr + index);
            if(nameLength & 0x80000000U) return index;
            if(nameLength == 0) return index;

            varNames.insert(std::string(funcStr + index, nameLength));
            oldIndex = index;
        }

        if(amountOfVariablesFound)
            *amountOfVariablesFound = int(varNames.size());

        if(destVarNames)
            destVarNames->assign(varNames.begin(), varNames.end());

        return -1;
    }
}

template<typename Value_t>
int FunctionParserBase<Value_t>::ParseAndDeduceVariables
(const std::string& function,
 int* amountOfVariablesFound,
 bool useDegrees)
{
    std::string varString;
    return deduceVariables(*this, function.c_str(), varString,
                           amountOfVariablesFound, 0, useDegrees);
}

template<typename Value_t>
int FunctionParserBase<Value_t>::ParseAndDeduceVariables
(const std::string& function,
 std::string& resultVarString,
 int* amountOfVariablesFound,
 bool useDegrees)
{
    std::string varString;
    const int index =
        deduceVariables(*this, function.c_str(), varString,
                        amountOfVariablesFound, 0, useDegrees);
    if(index < 0) resultVarString = varString;
    return index;
}

template<typename Value_t>
int FunctionParserBase<Value_t>::ParseAndDeduceVariables
(const std::string& function,
 std::vector<std::string>& resultVars,
 bool useDegrees)
{
    std::string varString;
    std::vector<std::string> vars;
    const int index =
        deduceVariables(*this, function.c_str(), varString,
                        0, &vars, useDegrees);
    if(index < 0) resultVars.swap(vars);
    return index;
}


#ifdef FUNCTIONPARSER_SUPPORT_DEBUGGING
//===========================================================================
// Bytecode injection
//===========================================================================
template<typename Value_t>
void FunctionParserBase<Value_t>::InjectRawByteCode
(const unsigned* bytecode, unsigned bytecodeAmount,
 const Value_t* immed, unsigned immedAmount, unsigned stackSize)
{
    CopyOnWrite();

    mData->mByteCode.assign(bytecode, bytecode + bytecodeAmount);
    mData->mImmed.assign(immed, immed + immedAmount);
    mData->mStackSize = stackSize;

#ifndef FP_USE_THREAD_SAFE_EVAL
    mData->mStack.resize(stackSize);
#endif
}

//===========================================================================
// Debug output
//===========================================================================
#include <iomanip>
#include <sstream>
namespace
{
    inline void printHex(std::ostream& dest, unsigned n)
    {
        std::ios::fmtflags flags = dest.flags();
        dest.width(4); dest.fill('0'); std::hex(dest); //uppercase(dest);
        dest << n;
        dest.flags(flags);
    }

    void padLine(std::ostringstream& dest, unsigned destLength)
    {
        for(size_t currentLength = dest.str().length();
            currentLength < destLength;
            ++currentLength)
        {
            dest << ' ';
        }
    }

    template<typename Value_t>
    std::string findName(const NamePtrsMap<Value_t>& nameMap,
                         unsigned index,
                         typename NameData<Value_t>::DataType type)
    {
        for(typename NamePtrsMap<Value_t>::const_iterator
                iter = nameMap.begin();
            iter != nameMap.end();
            ++iter)
        {
            if(iter->second.type == type && iter->second.index == index)
                return std::string(iter->first.name,
                                   iter->first.name + iter->first.nameLength);
        }
        return "?";
    }

    const struct PowiMuliType
    {
        unsigned opcode_square;
        unsigned opcode_cumulate;
        unsigned opcode_invert;
        unsigned opcode_half;
        unsigned opcode_invhalf;
    } iseq_powi = {cSqr,cMul,cInv,cSqrt,cRSqrt},
      iseq_muli = {~unsigned(0), cAdd,cNeg, ~unsigned(0),~unsigned(0) };

    template<typename Value_t>
    Value_t ParsePowiMuli(
        const PowiMuliType& opcodes,
        const std::vector<unsigned>& ByteCode, unsigned& IP,
        unsigned limit,
        size_t factor_stack_base,
        std::vector<Value_t>& stack,
        bool IgnoreExcess)
    {
        Value_t result = Value_t(1);
        while(IP < limit)
        {
            if(ByteCode[IP] == opcodes.opcode_square)
            {
                if(!isInteger(result)) break;
                result *= Value_t(2);
                ++IP;
                continue;
            }
            if(ByteCode[IP] == opcodes.opcode_invert)
            {
                if(result < Value_t(0)) break;
                result = -result;
                ++IP;
                continue;
            }
            if(ByteCode[IP] == opcodes.opcode_half)
            {
                if(result > Value_t(0) && isEvenInteger(result))
                    break;
                if(isInteger(result * Value_t(0.5))) break;
                result *= Value_t(0.5);
                ++IP;
                continue;
            }
            if(ByteCode[IP] == opcodes.opcode_invhalf)
            {
                if(result > Value_t(0) && isEvenInteger(result))
                    break;
                if(isInteger(result * Value_t(-0.5))) break;
                result *= Value_t(-0.5);
                ++IP;
                continue;
            }

            unsigned dup_fetch_pos = IP;
            Value_t lhs = Value_t(1);

            if(ByteCode[IP] == cFetch)
            {
                unsigned index = ByteCode[++IP];
                if(index < factor_stack_base
                || size_t(index-factor_stack_base) >= stack.size())
                {
                    // It wasn't a powi-fetch after all
                    IP = dup_fetch_pos;
                    break;
                }
                lhs = stack[index - factor_stack_base];
                // Note: ^This assumes that cFetch of recentmost
                //        is always converted into cDup.
                goto dup_or_fetch;
            }

            if(ByteCode[IP] == cDup)
            {
                lhs = result;
                goto dup_or_fetch;

            dup_or_fetch:
                stack.push_back(result);
                ++IP;
                Value_t subexponent = ParsePowiMuli
                    (opcodes,
                     ByteCode, IP, limit,
                     factor_stack_base, stack,
                     IgnoreExcess);
                if(IP >= limit && IgnoreExcess)
                    return lhs*subexponent;
                if(IP >= limit || ByteCode[IP] != opcodes.opcode_cumulate)
                {
                    // It wasn't a powi-dup after all
                    IP = dup_fetch_pos;
                    break;
                }
                ++IP; // skip opcode_cumulate
                stack.pop_back();
                result += lhs*subexponent;
                continue;
            }
            break;
        }
        return result;
    }

    template<typename Value_t>
    Value_t ParsePowiSequence(const std::vector<unsigned>& ByteCode,
                              unsigned& IP, unsigned limit,
                              size_t factor_stack_base,
                              bool IgnoreExcess = false)
    {
        std::vector<Value_t> stack;
        stack.push_back(Value_t(1));
        return ParsePowiMuli(iseq_powi, ByteCode, IP, limit,
                             factor_stack_base, stack,
                             IgnoreExcess);
    }

    template<typename Value_t>
    Value_t ParseMuliSequence(const std::vector<unsigned>& ByteCode,
                              unsigned& IP, unsigned limit,
                              size_t factor_stack_base,
                              bool IgnoreExcess = false)
    {
        std::vector<Value_t> stack;
        stack.push_back(Value_t(1));
        return ParsePowiMuli(iseq_muli, ByteCode, IP, limit,
                             factor_stack_base, stack,
                             IgnoreExcess);
    }

    struct IfInfo
    {
        std::pair<int,std::string> condition;
        std::pair<int,std::string> thenbranch;
        unsigned endif_location;

        IfInfo() : condition(), thenbranch(), endif_location() { }
    };
}

template<typename Value_t>
void FunctionParserBase<Value_t>::PrintByteCode(std::ostream& dest,
                                                bool showExpression) const
{
    dest << "Size of stack: " << mData->mStackSize << "\n";

    std::ostringstream outputBuffer;
    std::ostream& output = (showExpression ? outputBuffer : dest);

    const std::vector<unsigned>& ByteCode = mData->mByteCode;
    const std::vector<Value_t>& Immed = mData->mImmed;

    std::vector<std::pair<int,std::string> > stack;
    std::vector<IfInfo> if_stack;

    for(unsigned IP = 0, DP = 0; IP <= ByteCode.size(); ++IP)
    {
    after_powi_or_muli:;
        std::string n;
        bool out_params = false;
        unsigned params = 2, produces = 1, opcode = 0;

        if(showExpression && !if_stack.empty() &&
          (   // Normal If termination rule:
              if_stack.back().endif_location == IP
              // This rule matches when cJumps are threaded:
           || (IP < ByteCode.size() && ByteCode[IP] == cJump
               && !if_stack.back().thenbranch.second.empty())
          ))
        {
            printHex(output, IP);
            if(if_stack.back().endif_location == IP)
                output << ": ----- (phi)";
            else
                output << ": ----- (phi+)";

            stack.resize(stack.size()+2);
            std::swap(stack[stack.size()-3], stack[stack.size()-1]);
            std::swap(if_stack.back().condition,  stack[stack.size()-3]);
            std::swap(if_stack.back().thenbranch, stack[stack.size()-2]);
            opcode = cIf;
            params = 3;
            --IP;
            if_stack.pop_back();
        }
        else
        {
            if(IP >= ByteCode.size()) break;
            opcode = ByteCode[IP];

            if(showExpression && (
                opcode == cSqr || opcode == cDup
             || opcode == cInv
             || opcode == cSqrt || opcode == cRSqrt
             || opcode == cFetch
            ))
            {
                unsigned changed_ip = IP;
                Value_t exponent =
                    ParsePowiSequence<Value_t>
                    (ByteCode, changed_ip,
                     if_stack.empty()
                     ? (unsigned)ByteCode.size()
                     : if_stack.back().endif_location,
                     stack.size()-1);
                std::string        operation_prefix;
                std::ostringstream operation_value;
                int prio = 0;
                if(exponent == 1.0)
                {
                    if(opcode != cDup) goto not_powi_or_muli;
                    Value_t factor =
                        ParseMuliSequence<Value_t>
                        (ByteCode, changed_ip,
                         if_stack.empty()
                         ? (unsigned)ByteCode.size()
                         : if_stack.back().endif_location,
                         stack.size()-1);
                    if(factor == Value_t(1) || factor == Value_t(-1))
                        goto not_powi_or_muli;
                    operation_prefix = "*";
                    operation_value << factor;
                    prio = 3;
                }
                else
                {
                    prio = 2;
                    operation_prefix = "^";
                    operation_value << exponent;
                }

                //unsigned explanation_before = changed_ip-2;
                unsigned explanation_before = changed_ip-1;

                const char* explanation_prefix = "_";
                for(const unsigned first_ip = IP; IP < changed_ip; ++IP)
                {
                    printHex(output, IP);
                    output << ": ";

                    const char* sep = "|";
                    if(first_ip+1 == changed_ip)
                    { sep = "="; explanation_prefix = " "; }
                    else if(IP   == first_ip) sep = "\\";
                    else if(IP+1 == changed_ip) sep = "/";
                    else explanation_prefix = "=";

                    switch(ByteCode[IP])
                    {
                        case cInv: output << "inv"; break;
                        case cNeg: output << "neg"; break;
                        case cDup: output << "dup"; break;
                        case cSqr: output << "sqr"; break;
                        case cMul: output << "mul"; break;
                        case cAdd: output << "add"; break;
                        case cCbrt: output << "cbrt"; break;
                        case cSqrt: output << "sqrt"; break;
                        case cRSqrt: output << "rsqrt"; break;
                        case cFetch:
                        {
                            unsigned index = ByteCode[++IP];
                            output << "cFetch(" << index << ")";
                            break;
                        }
                        default: break;
                    }
                    padLine(outputBuffer, 20);
                    output << sep;
                    if(IP >= explanation_before)
                    {
                        explanation_before = (unsigned)ByteCode.size();
                        output << explanation_prefix
                               << '[' << (stack.size()-1) << ']';
                        std::string last = stack.back().second;
                        if(stack.back().first >= prio)
                            last = "(" + last + ")";
                        output << last;
                        output << operation_prefix;
                        output << operation_value.str();
                    }
                    else
                    {
                        unsigned p = first_ip;
                        Value_t exp = operation_prefix=="^" ?
                            ParsePowiSequence<Value_t>
                            (ByteCode, p, IP+1, stack.size()-1, true) :
                            ParseMuliSequence<Value_t>
                            (ByteCode, p, IP+1, stack.size()-1, true);
                        std::string last = stack.back().second;
                        if(stack.back().first >= prio)
                            last = "(" + last + ")";
                        output << " ..." << last;
                        output << operation_prefix;
                        output << exp;
                    }
                    dest << outputBuffer.str() << std::endl;
                    outputBuffer.str("");
                }

                std::string& last = stack.back().second;
                if(stack.back().first >= prio)
                    last = "(" + last + ")";
                last += operation_prefix;
                last += operation_value.str();
                stack.back().first = prio;

                goto after_powi_or_muli;
            }
        not_powi_or_muli:;
            printHex(output, IP);
            output << ": ";

            switch(opcode)
            {
              case cIf:
              {
                  unsigned label = ByteCode[IP+1]+1;
                  output << "jz ";
                  printHex(output, label);
                  params = 1;
                  produces = 0;
                  IP += 2;

                  if_stack.resize(if_stack.size() + 1);
                  std::swap( if_stack.back().condition, stack.back() );
                  if_stack.back().endif_location = (unsigned) ByteCode.size();
                  stack.pop_back();
                  break;
              }
              case cAbsIf:
              {
                  unsigned dp    = ByteCode[IP+2];
                  unsigned label = ByteCode[IP+1]+1;
                  output << "jz_abs " << dp << ",";
                  printHex(output, label);
                  params = 1;
                  produces = 0;
                  IP += 2;

                  if_stack.resize(if_stack.size() + 1);
                  std::swap( if_stack.back().condition, stack.back() );
                  if_stack.back().endif_location = (unsigned) ByteCode.size();
                  stack.pop_back();
                  break;
              }

              case cJump:
              {
                  unsigned dp    = ByteCode[IP+2];
                  unsigned label = ByteCode[IP+1]+1;

                  if(!if_stack.empty() && !stack.empty())
                  {
                      std::swap(if_stack.back().thenbranch, stack.back());
                      if_stack.back().endif_location = label;
                      stack.pop_back();
                  }

                  output << "jump " << dp << ",";
                  printHex(output, label);
                  params = 0;
                  produces = 0;
                  IP += 2;
                  break;
              }
              case cImmed:
              {
                  if(showExpression)
                  {
                      std::ostringstream buf;
                      buf.precision(8);
                      buf << Immed[DP];
                      stack.push_back( std::make_pair(0, buf.str()) );
                  }
                  output.precision(8);
                  output << "push " << Immed[DP];
                  ++DP;
                  produces = 0;
                  break;
              }

              case cFCall:
                  {
                      const unsigned index = ByteCode[++IP];
                      params = mData->mFuncPtrs[index].mParams;
                      static std::string name;
                      name = "f:" + findName(mData->mNamePtrs, index,
                                             NameData<Value_t>::FUNC_PTR);
                      n = name.c_str();
                      out_params = true;
                      break;
                  }

              case cPCall:
                  {
                      const unsigned index = ByteCode[++IP];
                      params = mData->mFuncParsers[index].mParams;
                      static std::string name;
                      name = "p:" + findName(mData->mNamePtrs, index,
                                             NameData<Value_t>::PARSER_PTR);
                      n = name.c_str();
                      out_params = true;
                      break;
                  }

              default:
                  if(OPCODE(opcode) < VarBegin)
                  {
                      switch(opcode)
                      {
                        case cNeg: n = "neg"; params = 1; break;
                        case cAdd: n = "add"; break;
                        case cSub: n = "sub"; break;
                        case cMul: n = "mul"; break;
                        case cDiv: n = "div"; break;
                        case cMod: n = "mod"; break;
                        case cPow: n = "pow"; break;
                        case cEqual: n = "eq"; break;
                        case cNEqual: n = "neq"; break;
                        case cLess: n = "lt"; break;
                        case cLessOrEq: n = "le"; break;
                        case cGreater: n = "gt"; break;
                        case cGreaterOrEq: n = "ge"; break;
                        case cAnd: n = "and"; break;
                        case cOr: n = "or"; break;
                        case cNot: n = "not"; params = 1; break;
                        case cNotNot: n = "notnot"; params = 1; break;
                        case cDeg: n = "deg"; params = 1; break;
                        case cRad: n = "rad"; params = 1; break;

    #ifndef FP_DISABLE_EVAL
                        case cEval: n = "eval"; params = mData->mVariablesAmount;
    #endif

                        case cFetch:
                        {
                            unsigned index = ByteCode[++IP];
                            if(showExpression && index < stack.size())
                                stack.push_back(stack[index]);
                            output << "cFetch(" << index << ")";
                            produces = 0;
                            break;
                        }
    #ifdef FP_SUPPORT_OPTIMIZER
                        case cLog2by: n = "log2by"; params = 2; out_params = 1; break;
                        case cPopNMov:
                        {
                            size_t a = ByteCode[++IP];
                            size_t b = ByteCode[++IP];
                            if(showExpression && b < stack.size())
                            {
                                std::pair<int, std::string> stacktop(0, "?");
                                if(b < stack.size()) stacktop = stack[b];
                                stack.resize(a);
                                stack.push_back(stacktop);
                            }
                            output << "cPopNMov(" << a << ", " << b << ")";
                            produces = 0;
                            break;
                        }
                        case cNop:
                            output << "nop"; params = 0; produces = 0;
                            break;
    #endif
                        case cSinCos:
                        {
                            if(showExpression)
                            {
                                std::pair<int, std::string> sin = stack.back();
                                std::pair<int, std::string> cos(
                                    0, "cos(" + sin.second + ")");
                                sin.first = 0;
                                sin.second = "sin(" + sin.second + ")";
                                stack.back() = sin;
                                stack.push_back(cos);
                            }
                            output << "sincos";
                            produces = 0;
                            break;
                        }
                        case cAbsAnd: n = "abs_and"; break;
                        case cAbsOr:  n = "abs_or"; break;
                        case cAbsNot: n = "abs_not"; params = 1; break;
                        case cAbsNotNot: n = "abs_notnot"; params = 1; break;
                        case cDup:
                        {
                            if(showExpression)
                                stack.push_back(stack.back());
                            output << "dup";
                            produces = 0;
                            break;
                        }
                        case cInv: n = "inv"; params = 1; break;
                        case cSqr: n = "sqr"; params = 1; break;
                        case cRDiv: n = "rdiv"; break;
                        case cRSub: n = "rsub"; break;
                        case cRSqrt: n = "rsqrt"; params = 1; break;

                        default:
                            n = Functions[opcode-cAbs].name;
                            params = Functions[opcode-cAbs].params;
                            out_params = params != 1;
                      }
                  }
                  else
                  {
                      if(showExpression)
                      {
                          stack.push_back(std::make_pair(0,
                              (findName(mData->mNamePtrs, opcode,
                                        NameData<Value_t>::VARIABLE))));
                      }
                      output << "push Var" << opcode-VarBegin;
                      produces = 0;
                  }
            }
        }
        if(produces) output << n;
        if(out_params) output << " (" << params << ")";
        if(showExpression)
        {
            padLine(outputBuffer, 20);

            if(produces > 0)
            {
                std::ostringstream buf;
                const char *paramsep = ",", *suff = "";
                int prio = 0; bool commutative = false;
                switch(opcode)
                {
                  case cIf: buf << "if("; suff = ")";
                      break;
                  case cAbsIf: buf << "if("; suff = ")";
                      break;
                  case cOr:  prio = 6; paramsep = "|"; commutative = true;
                      break;
                  case cAnd: prio = 5; paramsep = "&"; commutative = true;
                      break;
                  case cAdd: prio = 4; paramsep = "+"; commutative = true;
                      break;
                  case cSub: prio = 4; paramsep = "-";
                      break;
                  case cMul: prio = 3; paramsep = "*"; commutative = true;
                      break;
                  case cDiv: prio = 3; paramsep = "/";
                      break;
                  case cPow: prio = 2; paramsep = "^";
                      break;
                  case cAbsOr:  prio = 6; paramsep = "|"; commutative = true;
                      break;
                  case cAbsAnd: prio = 5; paramsep = "&"; commutative = true;
                      break;
                  case cSqr: prio = 2; suff = "^2";
                      break;
                  case cNeg: buf << "(-("; suff = "))";
                      break;
                  case cNot: buf << "(!("; suff = "))";
                      break;
                  default: buf << n << '('; suff = ")";
                }

                const char* sep = "";
                for(unsigned a=0; a<params; ++a)
                {
                    buf << sep;
                    if(stack.size() + a < params)
                        buf << "?";
                    else
                    {
                        const std::pair<int,std::string>& prev =
                            stack[stack.size() - params + a];
                        if(prio > 0 && (prev.first > prio ||
                                        (prev.first==prio && !commutative)))
                            buf << '(' << prev.second << ')';
                        else
                            buf << prev.second;
                    }
                    sep = paramsep;
                }
                if(stack.size() >= params)
                    stack.resize(stack.size() - params);
                else
                    stack.clear();
                buf << suff;
                stack.push_back(std::make_pair(prio, buf.str()));
                //if(n.size() <= 4 && !out_params) padLine(outputBuffer, 20);
            }
            //padLine(outputBuffer, 20);
            output << "= ";
            if(((opcode == cIf || opcode == cAbsIf) && params != 3)
              || opcode == cJump
    #ifdef FP_SUPPORT_OPTIMIZER
              || opcode == cNop
    #endif
                )
                output << "(void)";
            else if(stack.empty())
                output << "[?] ?";
            else
                output << '[' << (stack.size()-1) << ']'
                       << stack.back().second;
        }

        if(showExpression)
        {
            dest << outputBuffer.str() << std::endl;
            outputBuffer.str("");
        }
        else
            output << std::endl;
    }
    dest << std::flush;
}
#endif


#ifndef FP_SUPPORT_OPTIMIZER
template<typename Value_t>
void FunctionParserBase<Value_t>::Optimize()
{
    // Do nothing if no optimizations are supported.
}
#endif

FUNCTIONPARSER_INSTANTIATE_TYPES
