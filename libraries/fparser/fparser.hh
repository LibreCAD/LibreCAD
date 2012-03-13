/***************************************************************************\
|* Function Parser for C++ v4.3                                            *|
|*-------------------------------------------------------------------------*|
|* Copyright: Juha Nieminen, Joel Yliluoma                                 *|
|*                                                                         *|
|* This library is distributed under the terms of the                      *|
|* GNU Lesser General Public License version 3.                            *|
|* (See lgpl.txt and gpl.txt for the license text.)                        *|
\***************************************************************************/

#ifndef ONCE_FPARSER_H_
#define ONCE_FPARSER_H_

#include <string>
#include <vector>

#ifdef FUNCTIONPARSER_SUPPORT_DEBUGGING
#include <iostream>
#endif

#ifdef _MSC_VER
// Visual Studio's warning about missing definitions for the explicit
// FunctionParserBase instantiations is irrelevant here.
#pragma warning(disable : 4661)
#endif

namespace FPoptimizer_CodeTree { template<typename Value_t> class CodeTree; }

template<typename Value_t>
class FunctionParserBase
{
public:
    enum ParseErrorType
    {
        SYNTAX_ERROR=0, MISM_PARENTH, MISSING_PARENTH, EMPTY_PARENTH,
        EXPECT_OPERATOR, OUT_OF_MEMORY, UNEXPECTED_ERROR, INVALID_VARS,
        ILL_PARAMS_AMOUNT, PREMATURE_EOS, EXPECT_PARENTH_FUNC,
        UNKNOWN_IDENTIFIER,
        NO_FUNCTION_PARSED_YET,
        FP_NO_ERROR
    };

    typedef Value_t value_type;


    int Parse(const char* Function, const std::string& Vars,
              bool useDegrees = false);
    int Parse(const std::string& Function, const std::string& Vars,
              bool useDegrees = false);

    void setDelimiterChar(char);

    const char* ErrorMsg() const;
    inline ParseErrorType GetParseErrorType() const { return mParseErrorType; }

    Value_t Eval(const Value_t* Vars);
    inline int EvalError() const { return mEvalErrorType; }

    bool AddConstant(const std::string& name, Value_t value);
    bool AddUnit(const std::string& name, Value_t value);

    typedef Value_t (*FunctionPtr)(const Value_t*);

    bool AddFunction(const std::string& name,
                     FunctionPtr, unsigned paramsAmount);
    bool AddFunction(const std::string& name, FunctionParserBase&);

    bool RemoveIdentifier(const std::string& name);

    void Optimize();


    int ParseAndDeduceVariables(const std::string& function,
                                int* amountOfVariablesFound = 0,
                                bool useDegrees = false);
    int ParseAndDeduceVariables(const std::string& function,
                                std::string& resultVarString,
                                int* amountOfVariablesFound = 0,
                                bool useDegrees = false);
    int ParseAndDeduceVariables(const std::string& function,
                                std::vector<std::string>& resultVars,
                                bool useDegrees = false);


    FunctionParserBase();
    ~FunctionParserBase();

    // Copy constructor and assignment operator (implemented using the
    // copy-on-write technique for efficiency):
    FunctionParserBase(const FunctionParserBase&);
    FunctionParserBase& operator=(const FunctionParserBase&);


    void ForceDeepCopy();



#ifdef FUNCTIONPARSER_SUPPORT_DEBUGGING
    // For debugging purposes only.
    // Performs no sanity checks or anything. If the values are wrong, the
    // library will crash. Do not use unless you know what you are doing.
    void InjectRawByteCode(const unsigned* bytecode, unsigned bytecodeAmount,
                           const Value_t* immed, unsigned immedAmount,
                           unsigned stackSize);

    void PrintByteCode(std::ostream& dest, bool showExpression = true) const;
#endif



//========================================================================
private:
//========================================================================

    friend class FPoptimizer_CodeTree::CodeTree<Value_t>;

// Private data:
// ------------
    char mDelimiterChar;
    ParseErrorType mParseErrorType;
    int mEvalErrorType;

    struct Data;
    Data* mData;

    bool mUseDegreeConversion;
    bool mHasByteCodeFlags;
    unsigned mEvalRecursionLevel;
    unsigned mStackPtr;
    const char* mErrorLocation;


// Private methods:
// ---------------
    void CopyOnWrite();
    bool CheckRecursiveLinking(const FunctionParserBase*) const;
    bool NameExists(const char*, unsigned);
    bool ParseVariables(const std::string&);
    int ParseFunction(const char*, bool);
    const char* SetErrorType(ParseErrorType, const char*);

    void AddFunctionOpcode(unsigned);
    void AddImmedOpcode(Value_t v);
    void incStackPtr();
    void CompilePowi(long);
    bool TryCompilePowi(Value_t);

    const char* CompileIf(const char*);
    const char* CompileFunctionParams(const char*, unsigned);
    const char* CompileElement(const char*);
    const char* CompilePossibleUnit(const char*);
    const char* CompilePow(const char*);
    const char* CompileUnaryMinus(const char*);
    const char* CompileMult(const char*);
    const char* CompileAddition(const char*);
    const char* CompileComparison(const char*);
    const char* CompileAnd(const char*);
    const char* CompileExpression(const char*);
    inline const char* CompileFunction(const char*, unsigned);
    inline const char* CompileParenthesis(const char*);
    inline const char* CompileLiteral(const char*);
    template<bool SetFlag>
    inline void PushOpcodeParam(unsigned);
    template<bool SetFlag>
    inline void PutOpcodeParamAt(unsigned, unsigned offset);
    const char* Compile(const char*);

protected:
    // Parsing utility functions
    static std::pair<const char*, Value_t> ParseLiteral(const char*);
    static unsigned ParseIdentifier(const char*);
};

class FunctionParser: public FunctionParserBase<double> {};
class FunctionParser_f: public FunctionParserBase<float> {};
class FunctionParser_ld: public FunctionParserBase<long double> {};
class FunctionParser_li: public FunctionParserBase<long> {};

#endif
