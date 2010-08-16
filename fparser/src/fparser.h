/***************************************************************************\
|* Function parser v2.51 by Warp                                           *|
|* -----------------------------                                           *|
|* Parses and evaluates the given function with the given variable values. *|
|*                                                                         *|
\***************************************************************************/

#ifndef ONCE_FPARSER_H_
#define ONCE_FPARSER_H_

#include <string>
#include <map>
#include <vector>
#include <iostream>

class FunctionParser
{
public:
    int Parse(const std::string& Function, const std::string& Vars,
              bool useDegrees = false);
    const char* ErrorMsg(void) const;
    double Eval(const double* Vars);
    inline int EvalError(void) const { return EvalErrorType; }

    bool AddConstant(const std::string& name, double value);

    typedef double (*FunctionPtr)(const double*);

    bool AddFunction(const std::string& name,
                     FunctionPtr, unsigned paramsAmount);
    bool AddFunction(const std::string& name, FunctionParser&);

    void Optimize();


    FunctionParser();
    ~FunctionParser();



    // For debugging purposes only:
    void PrintByteCode(std::ostream& dest) const;

//========================================================================
private:
//========================================================================
    int varAmount, ParseErrorType,EvalErrorType;

    typedef std::map<std::string, unsigned> VarMap_t;
    VarMap_t Variables;

    typedef std::map<std::string, double> ConstMap_t;
    ConstMap_t Constants;

    VarMap_t FuncPtrNames;
    struct FuncPtrData
    {
        FunctionPtr ptr; unsigned params;
        FuncPtrData(FunctionPtr p, unsigned par): ptr(p), params(par) {}
    };
    std::vector<FuncPtrData> FuncPtrs;

    VarMap_t FuncParserNames;
    std::vector<FunctionParser*> FuncParsers;

    struct CompiledCode
    {   CompiledCode();
        ~CompiledCode();

        unsigned* ByteCode;
        unsigned ByteCodeSize;
        double* Immed;
        unsigned ImmedSize;
        double* Stack;
        unsigned StackSize, StackPtr;
    } Comp;

    // Temp vectors for bytecode:
    std::vector<unsigned>* tempByteCode;
    std::vector<double>* tempImmed;

    bool useDegreeConversion;

    bool checkRecursiveLinking(const FunctionParser*);

    bool isValidName(const std::string&);
    VarMap_t::const_iterator FindVariable(const char*, const VarMap_t&);
    ConstMap_t::const_iterator FindConstant(const char*);
    int CheckSyntax(const char*);
    bool Compile(const char*);
    bool IsVariable(int);
    void AddCompiledByte(unsigned);
    void AddImmediate(double);
    void AddFunctionOpcode(unsigned);
    int CompileIf(const char*, int);
    int CompileFunctionParams(const char*, int, unsigned);
    int CompileElement(const char*, int);
    int CompilePow(const char*, int);
    int CompileMult(const char*, int);
    int CompileAddition(const char*, int);
    int CompileComparison(const char*, int);
    int CompileAnd(const char*, int);
    int CompileOr(const char*, int);
    int CompileExpression(const char*, int, bool=false);


    void MakeTree(struct CodeTree *result) const;

    FunctionParser(const FunctionParser&);
    const FunctionParser& operator=(const FunctionParser&);
};

#endif
