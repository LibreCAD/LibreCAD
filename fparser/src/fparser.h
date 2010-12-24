/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!  
**
**********************************************************************/

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
