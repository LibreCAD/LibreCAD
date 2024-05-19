/*

	 _____  __ _____________ _______  ______ ___________
	/     \|  |  \____ \__  \\_  __ \/  ___// __ \_  __ \
   |  Y Y  \  |  /  |_> > __ \|  | \/\___ \\  ___/|  | \/
   |__|_|  /____/|   __(____  /__|  /____  >\___  >__|
		 \/      |__|       \/           \/     \/
   Copyright (C) 2004 - 2022 Ingo Berg

	Redistribution and use in source and binary forms, with or without modification, are permitted
	provided that the following conditions are met:

	  * Redistributions of source code must retain the above copyright notice, this list of
		conditions and the following disclaimer.
	  * Redistributions in binary form must reproduce the above copyright notice, this list of
		conditions and the following disclaimer in the documentation and/or other materials provided
		with the distribution.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
	IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
	FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
	CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
	DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
	DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
	IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
	OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "muParser.h"
#include "muParserTemplateMagic.h"

//--- Standard includes ------------------------------------------------------------------------
#include <cmath>
#include <algorithm>
#include <numeric>
#include <cstdlib>


using namespace std;

/** \file
	\brief Implementation of the standard floating point parser.
*/



/** \brief Namespace for mathematical applications. */
namespace mu
{
	//---------------------------------------------------------------------------
	/** \brief Default value recognition callback.
		\param [in] a_szExpr Pointer to the expression
		\param [in, out] a_iPos Pointer to an index storing the current position within the expression
		\param [out] a_fVal Pointer where the value should be stored in case one is found.
		\return 1 if a value was found 0 otherwise.
	*/
	int Parser::IsVal(const char_type* a_szExpr, int* a_iPos, value_type* a_fVal)
	{
#if defined(__APPLE__)
        // 2023-12-23 Issue #136: This code breaks localization!
		//
		// I decided to not give a shit about localization being 
		// broken on Macs's because APPLE does not give a shit about the 
		// stringstream  being broken either. They have the resources to fix their
		// compiler, i do not have the resources to work around their failures.

		// fix for #123; std::Stringstream is broken on Mac; use std::stod instead
		try
		{
			std::size_t charsProcessed;
			value_type fVal = static_cast<value_type>(std::stod(string_type(a_szExpr), &charsProcessed));
			if (charsProcessed == 0)
				return 0;

			*a_iPos += (int)charsProcessed;
			*a_fVal = fVal;
			return 1;
		}
		catch (...)
		{
			return 0;
		}
#else
		value_type fVal(0);

		stringstream_type stream(a_szExpr);
		stream.imbue(Parser::s_locale);
		stream >> fVal;
		stringstream_type::pos_type iEnd = stream.tellg(); // Position after reading

		if (iEnd == (stringstream_type::pos_type) - 1)
			return 0;

		*a_iPos += (int)iEnd;
		*a_fVal = fVal;
		return 1;
#endif		
	}


	//---------------------------------------------------------------------------
	/** \brief Constructor.

	  Call ParserBase class constructor and trigger Function, Operator and Constant initialization.
	*/
	Parser::Parser()
		:ParserBase()
	{
		AddValIdent(IsVal);

		InitCharSets();
		InitFun();
		InitConst();
		InitOprt();
	}

	//---------------------------------------------------------------------------
	/** \brief Define the character sets.
		\sa DefineNameChars, DefineOprtChars, DefineInfixOprtChars

	  This function is used for initializing the default character sets that define
	  the characters to be useable in function and variable names and operators.
	*/
	void Parser::InitCharSets()
	{
		DefineNameChars(_T("0123456789_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"));
		DefineOprtChars(_T("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ+-*^/?<>=#!$%&|~'_{}"));
		DefineInfixOprtChars(_T("/+-*^?<>=#!$%&|~'_"));
	}

	//---------------------------------------------------------------------------
	/** \brief Initialize the default functions. */
	void Parser::InitFun()
	{
		if (mu::TypeInfo<mu::value_type>::IsInteger())
		{
			// When setting MUP_BASETYPE to an integer type
			// Place functions for dealing with integer values here
			// ...
			// ...
			// ...
		}
		else
		{
			// trigonometric functions
			DefineFun(_T("sin"), MathImpl<value_type>::Sin);
			DefineFun(_T("cos"), MathImpl<value_type>::Cos);
			DefineFun(_T("tan"), MathImpl<value_type>::Tan);
			// arcus functions
			DefineFun(_T("asin"), MathImpl<value_type>::ASin);
			DefineFun(_T("acos"), MathImpl<value_type>::ACos);
			DefineFun(_T("atan"), MathImpl<value_type>::ATan);
			DefineFun(_T("atan2"), MathImpl<value_type>::ATan2);
			// hyperbolic functions
			DefineFun(_T("sinh"), MathImpl<value_type>::Sinh);
			DefineFun(_T("cosh"), MathImpl<value_type>::Cosh);
			DefineFun(_T("tanh"), MathImpl<value_type>::Tanh);
			// arcus hyperbolic functions
			DefineFun(_T("asinh"), MathImpl<value_type>::ASinh);
			DefineFun(_T("acosh"), MathImpl<value_type>::ACosh);
			DefineFun(_T("atanh"), MathImpl<value_type>::ATanh);
			// Logarithm functions
			DefineFun(_T("log2"), MathImpl<value_type>::Log2);
			DefineFun(_T("log10"), MathImpl<value_type>::Log10);
			DefineFun(_T("log"), MathImpl<value_type>::Log);
			DefineFun(_T("ln"), MathImpl<value_type>::Log);
			// misc
			DefineFun(_T("exp"), MathImpl<value_type>::Exp);
			DefineFun(_T("sqrt"), MathImpl<value_type>::Sqrt);
			DefineFun(_T("sign"), MathImpl<value_type>::Sign);
			DefineFun(_T("rint"), MathImpl<value_type>::Rint);
			DefineFun(_T("abs"), MathImpl<value_type>::Abs);
			// Functions with variable number of arguments
			DefineFun(_T("sum"), MathImpl<value_type>::Sum);
			DefineFun(_T("avg"), MathImpl<value_type>::Avg);
			DefineFun(_T("min"), MathImpl<value_type>::Min);
			DefineFun(_T("max"), MathImpl<value_type>::Max);
		}
	}

	//---------------------------------------------------------------------------
	/** \brief Initialize constants.

	  By default the parser recognizes two constants. Pi ("pi") and the Eulerian
	  number ("_e").
	*/
	void Parser::InitConst()
	{
		DefineConst(_T("_pi"), MathImpl<value_type>::CONST_PI);
		DefineConst(_T("_e"), MathImpl<value_type>::CONST_E);
	}

	//---------------------------------------------------------------------------
	/** \brief Initialize operators.

	  By default only the unary minus operator is added.
	*/
	void Parser::InitOprt()
	{
		DefineInfixOprt(_T("-"), MathImpl<value_type>::UnaryMinus);
		DefineInfixOprt(_T("+"), MathImpl<value_type>::UnaryPlus);
	}

	//---------------------------------------------------------------------------
	void Parser::OnDetectVar(string_type* /*pExpr*/, int& /*nStart*/, int& /*nEnd*/)
	{
		// this is just sample code to illustrate modifying variable names on the fly.
		// I'm not sure anyone really needs such a feature...
		/*


		string sVar(pExpr->begin()+nStart, pExpr->begin()+nEnd);
		string sRepl = std::string("_") + sVar + "_";

		int nOrigVarEnd = nEnd;
		cout << "variable detected!\n";
		cout << "  Expr: " << *pExpr << "\n";
		cout << "  Start: " << nStart << "\n";
		cout << "  End: " << nEnd << "\n";
		cout << "  Var: \"" << sVar << "\"\n";
		cout << "  Repl: \"" << sRepl << "\"\n";
		nEnd = nStart + sRepl.length();
		cout << "  End: " << nEnd << "\n";
		pExpr->replace(pExpr->begin()+nStart, pExpr->begin()+nOrigVarEnd, sRepl);
		cout << "  New expr: " << *pExpr << "\n";
		*/
	}

	//---------------------------------------------------------------------------
	/** \brief Numerically differentiate with regard to a variable.
		\param [in] a_Var Pointer to the differentiation variable.
		\param [in] a_fPos Position at which the differentiation should take place.
		\param [in] a_fEpsilon Epsilon used for the numerical differentiation.

		Numerical differentiation uses a 5 point operator yielding a 4th order
		formula. The default value for epsilon is 0.00074 which is
		numeric_limits<double>::epsilon() ^ (1/5).
	*/
	value_type Parser::Diff(value_type* a_Var, value_type  a_fPos, value_type  a_fEpsilon) const
	{
		value_type fRes(0);
		value_type fBuf(*a_Var);
		value_type f[4] = { 0,0,0,0 };
		value_type fEpsilon(a_fEpsilon);

		// Backwards compatible calculation of epsilon inc case the user doesn't provide
		// his own epsilon
		if (fEpsilon == 0)
			fEpsilon = (a_fPos == 0) ? (value_type)1e-10 : (value_type)1e-7 * a_fPos;

		*a_Var = a_fPos + 2 * fEpsilon;  f[0] = Eval();
		*a_Var = a_fPos + 1 * fEpsilon;  f[1] = Eval();
		*a_Var = a_fPos - 1 * fEpsilon;  f[2] = Eval();
		*a_Var = a_fPos - 2 * fEpsilon;  f[3] = Eval();
		*a_Var = fBuf; // restore variable

		fRes = (-f[0] + 8 * f[1] - 8 * f[2] + f[3]) / (12 * fEpsilon);
		return fRes;
	}
} // namespace mu
