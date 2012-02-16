/***************************************************************************\
|* Function Parser for C++ v4.3                                            *|
|*-------------------------------------------------------------------------*|
|* Copyright: Juha Nieminen, Joel Yliluoma                                 *|
|*                                                                         *|
|* This library is distributed under the terms of the                      *|
|* GNU Lesser General Public License version 3.                            *|
|* (See lgpl.txt and gpl.txt for the license text.)                        *|
\***************************************************************************/

// NOTE:
// This file contains only internal types for the function parser library.
// You don't need to include this file in your code. Include "fparser.hh"
// only.

#ifndef ONCE_FPARSER_TYPES_H_
#define ONCE_FPARSER_TYPES_H_

#include "fpconfig.hh"
#include <cstring>

#ifdef ONCE_FPARSER_H_
#include <map>
#endif

namespace FUNCTIONPARSERTYPES
{
    enum OPCODE
    {
// The order of opcodes in the function list must
// match that which is in the Functions[] array.
        cAbs,
        cAcos, cAcosh,
        cAsin, cAsinh,
        cAtan, cAtan2, cAtanh,
        cCbrt, cCeil,
        cCos, cCosh, cCot, cCsc,
        cEval,
        cExp, cExp2, cFloor, cHypot,
        cIf, cInt, cLog, cLog10, cLog2, cMax, cMin,
        cPow, cSec, cSin, cSinh, cSqrt, cTan, cTanh,
        cTrunc,

// These do not need any ordering:
// Except that if you change the order of {eq,neq,lt,le,gt,ge}, you
// must also change the order in ConstantFolding_ComparisonOperations().
        cImmed, cJump,
        cNeg, cAdd, cSub, cMul, cDiv, cMod,
        cEqual, cNEqual, cLess, cLessOrEq, cGreater, cGreaterOrEq,
        cNot, cAnd, cOr,
        cNotNot, /* Protects the double-not sequence from optimizations */

        cDeg, cRad, /* Multiplication and division by 180 / pi */

        cFCall, cPCall,

#ifdef FP_SUPPORT_OPTIMIZER
        cPopNMov, /* cPopNMov(x,y) moves [y] to [x] and deletes anything
                   * above [x]. Used for disposing of temporaries.
                   */
        cLog2by, /* log2by(x,y) = log2(x) * y */
        cNop,    /* Used by fpoptimizer internally; should not occur in bytecode */
#endif
        cSinCos, /* sin(x) followed by cos(x) (two values are pushed to stack) */
        cAbsAnd,    /* As cAnd,       but assume both operands are absolute values */
        cAbsOr,     /* As cOr,        but assume both operands are absolute values */
        cAbsNot,    /* As cAbsNot,    but assume the operand is an absolute value */
        cAbsNotNot, /* As cAbsNotNot, but assume the operand is an absolute value */
        cAbsIf,     /* As cAbsIf,     but assume the 1st operand is an absolute value */

        cDup,   /* Duplicates the last value in the stack: Push [Stacktop] */
        cFetch, /* Same as Dup, except with absolute index
                 * (next value is index) */
        cInv,   /* Inverts the last value in the stack (x = 1/x) */
        cSqr,   /* squares the last operand in the stack, no push/pop */
        cRDiv,  /* reverse division (not x/y, but y/x) */
        cRSub,  /* reverse subtraction (not x-y, but y-x) */
        cRSqrt, /* inverse square-root (1/sqrt(x)) */

        VarBegin
    };

#ifdef ONCE_FPARSER_H_
    struct FuncDefinition
    {
        enum FunctionFlags
        {
            Enabled  = 0x01,
            AngleIn  = 0x02,
            AngleOut = 0x04,
            OkForInt = 0x08
        };

#ifdef FUNCTIONPARSER_SUPPORT_DEBUGGING
        const char name[8];
#else
        struct name { } name;
#endif
        unsigned params : 8;
        unsigned flags  : 8;

        inline bool enabled() const { return flags != 0; }
        inline bool okForInt() const { return (flags & OkForInt) != 0; }
    };

#ifndef FP_DISABLE_EVAL
# define FP_EVAL_FUNCTION_ENABLED \
    FuncDefinition::Enabled | FuncDefinition::OkForInt
#else
# define FP_EVAL_FUNCTION_ENABLED 0
#endif
#ifdef FUNCTIONPARSER_SUPPORT_DEBUGGING
# define FP_FNAME(n) n
#else
# define FP_FNAME(n) {}
#endif
// This list must be in the same order as that in OPCODE enum,
// because the opcode value is used to index this array, and
// the pointer to array element is used for generating the opcode.
    const FuncDefinition Functions[]=
    {
        /*cAbs  */ { FP_FNAME("abs"),   1,
                     FuncDefinition::Enabled | FuncDefinition::OkForInt },
        /*cAcos */ { FP_FNAME("acos"),  1,
                     FuncDefinition::Enabled | FuncDefinition::AngleOut },
        /*cAcosh*/ { FP_FNAME("acosh"), 1,
                     FuncDefinition::Enabled | FuncDefinition::AngleOut },
        /*cAsin */ { FP_FNAME("asin"),  1,
                     FuncDefinition::Enabled | FuncDefinition::AngleOut },
        /*cAsinh*/ { FP_FNAME("asinh"), 1,
                     FuncDefinition::Enabled | FuncDefinition::AngleOut },
        /*cAtan */ { FP_FNAME("atan"),  1,
                     FuncDefinition::Enabled | FuncDefinition::AngleOut },
        /*cAtan2*/ { FP_FNAME("atan2"), 2,
                     FuncDefinition::Enabled | FuncDefinition::AngleOut },
        /*cAtanh*/ { FP_FNAME("atanh"), 1, FuncDefinition::Enabled },
        /*cCbrt */ { FP_FNAME("cbrt"),  1, FuncDefinition::Enabled },
        /*cCeil */ { FP_FNAME("ceil"),  1, FuncDefinition::Enabled },
        /*cCos  */ { FP_FNAME("cos"),   1,
                     FuncDefinition::Enabled | FuncDefinition::AngleIn },
        /*cCosh */ { FP_FNAME("cosh"),  1,
                     FuncDefinition::Enabled | FuncDefinition::AngleIn },
        /*cCot  */ { FP_FNAME("cot"),   1,
                     FuncDefinition::Enabled | FuncDefinition::AngleIn },
        /*cCsc  */ { FP_FNAME("csc"),   1,
                     FuncDefinition::Enabled | FuncDefinition::AngleIn },
        /*cEval */ { FP_FNAME("eval"),  0, FP_EVAL_FUNCTION_ENABLED },
        /*cExp  */ { FP_FNAME("exp"),   1, FuncDefinition::Enabled },
        /*cExp2 */ { FP_FNAME("exp2"),  1, FuncDefinition::Enabled },
        /*cFloor*/ { FP_FNAME("floor"), 1, FuncDefinition::Enabled },
        /*cHypot*/ { FP_FNAME("hypot"), 2, FuncDefinition::Enabled },
        /*cIf   */ { FP_FNAME("if"),    0,
                     FuncDefinition::Enabled | FuncDefinition::OkForInt },
        /*cInt  */ { FP_FNAME("int"),   1, FuncDefinition::Enabled },
        /*cLog  */ { FP_FNAME("log"),   1, FuncDefinition::Enabled },
        /*cLog10*/ { FP_FNAME("log10"), 1, FuncDefinition::Enabled },
        /*cLog2 */ { FP_FNAME("log2"),  1, FuncDefinition::Enabled },
        /*cMax  */ { FP_FNAME("max"),   2,
                     FuncDefinition::Enabled | FuncDefinition::OkForInt },
        /*cMin  */ { FP_FNAME("min"),   2,
                     FuncDefinition::Enabled | FuncDefinition::OkForInt },
        /*cPow  */ { FP_FNAME("pow"),   2, FuncDefinition::Enabled },
        /*cSec  */ { FP_FNAME("sec"),   1,
                     FuncDefinition::Enabled | FuncDefinition::AngleIn },
        /*cSin  */ { FP_FNAME("sin"),   1,
                     FuncDefinition::Enabled | FuncDefinition::AngleIn },
        /*cSinh */ { FP_FNAME("sinh"),  1,
                     FuncDefinition::Enabled | FuncDefinition::AngleIn },
        /*cSqrt */ { FP_FNAME("sqrt"),  1,
                     FuncDefinition::Enabled },
        /*cTan  */ { FP_FNAME("tan"),   1,
                     FuncDefinition::Enabled | FuncDefinition::AngleIn },
        /*cTanh */ { FP_FNAME("tanh"),  1,
                     FuncDefinition::Enabled | FuncDefinition::AngleIn },
        /*cTrunc*/ { FP_FNAME("trunc"), 1,
                     FuncDefinition::Enabled }
    };
#undef FP_FNAME

    struct NamePtr
    {
        const char* name;
        unsigned nameLength;

        NamePtr(const char* n, unsigned l): name(n), nameLength(l) {}

        inline bool operator==(const NamePtr& rhs) const
        {
            return nameLength == rhs.nameLength
                && std::memcmp(name, rhs.name, nameLength) == 0;
        }
        inline bool operator<(const NamePtr& rhs) const
        {
            for(unsigned i = 0; i < nameLength; ++i)
            {
                if(i == rhs.nameLength) return false;
                const char c1 = name[i], c2 = rhs.name[i];
                if(c1 < c2) return true;
                if(c2 < c1) return false;
            }
            return nameLength < rhs.nameLength;
        }
    };

    template<typename Value_t>
    struct NameData
    {
        enum DataType { CONSTANT, UNIT, FUNC_PTR, PARSER_PTR, VARIABLE };
        DataType type;
        unsigned index;
        Value_t value;

        NameData(DataType t, unsigned v) : type(t), index(v), value() { }
        NameData(DataType t, Value_t v) : type(t), index(), value(v) { }
        NameData() { }
    };

    template<typename Value_t>
    class NamePtrsMap: public
    std::map<FUNCTIONPARSERTYPES::NamePtr,
             FUNCTIONPARSERTYPES::NameData<Value_t> >
    {
    };

    const unsigned FUNC_AMOUNT = sizeof(Functions)/sizeof(Functions[0]);
#endif // ONCE_FPARSER_H_
}

#ifdef ONCE_FPARSER_H_
#include <vector>

template<typename Value_t>
struct FunctionParserBase<Value_t>::Data
{
    unsigned mReferenceCounter;

    unsigned mVariablesAmount;
    std::string mVariablesString;
    FUNCTIONPARSERTYPES::NamePtrsMap<Value_t> mNamePtrs;

    struct InlineVariable
    {
        FUNCTIONPARSERTYPES::NamePtr mName;
        unsigned mFetchIndex;
    };

    typedef std::vector<InlineVariable> InlineVarNamesContainer;
    InlineVarNamesContainer mInlineVarNames;

    struct FuncPtrData
    {
        union
        {
            FunctionPtr mFuncPtr;
            FunctionParserBase<Value_t>* mParserPtr;
        };
        unsigned mParams;
    };

    std::vector<FuncPtrData> mFuncPtrs;
    std::vector<FuncPtrData> mFuncParsers;

    std::vector<unsigned> mByteCode;
    std::vector<Value_t> mImmed;
#if !defined(FP_USE_THREAD_SAFE_EVAL) && \
    !defined(FP_USE_THREAD_SAFE_EVAL_WITH_ALLOCA)
    std::vector<Value_t> mStack;
    // Note: When mStack exists,
    //       mStack.size() and mStackSize are mutually redundant.
#endif
    unsigned mStackSize;

    Data();
    Data(const Data&);
    Data& operator=(const Data&); // not implemented on purpose
    ~Data();
};
#endif

#include "fpaux.hh"

#endif
