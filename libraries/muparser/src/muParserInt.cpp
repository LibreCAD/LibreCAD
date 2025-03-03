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

#include "muParserInt.h"

#include <cmath>
#include <algorithm>
#include <numeric>

using namespace std;

/** \file
	\brief Implementation of a parser using integer value.
*/

/** \brief Namespace for mathematical applications. */
namespace mu
{
	value_type ParserInt::Abs(value_type v) { return (value_type)Round(fabs((double)v)); }
	value_type ParserInt::Sign(value_type v) { return (Round(v) < 0) ? -1 : (Round(v) > 0) ? 1 : 0; }
	value_type ParserInt::Ite(value_type v1, value_type v2,	value_type v3) { return (Round(v1) == 1) ? Round(v2) : Round(v3); }
	value_type ParserInt::Add(value_type v1, value_type v2) { return Round(v1) + Round(v2); }
	value_type ParserInt::Sub(value_type v1, value_type v2) { return Round(v1) - Round(v2); }
	value_type ParserInt::Mul(value_type v1, value_type v2) { return Round(v1) * Round(v2); }
	value_type ParserInt::Div(value_type v1, value_type v2) { return Round(v1) / Round(v2); }
	value_type ParserInt::Mod(value_type v1, value_type v2) { return Round(v1) % Round(v2); }
	value_type ParserInt::Shr(value_type v1, value_type v2) { return Round(v1) >> Round(v2); }
	value_type ParserInt::Shl(value_type v1, value_type v2) { return Round(v1) << Round(v2); }
	value_type ParserInt::BitAnd(value_type v1, value_type v2) { return Round(v1) & Round(v2); }
	value_type ParserInt::BitOr(value_type v1, value_type v2) { return Round(v1) | Round(v2); }
	value_type ParserInt::And(value_type v1, value_type v2) { return Round(v1) && Round(v2); }
	value_type ParserInt::Or(value_type v1, value_type v2) { return Round(v1) || Round(v2); }
	value_type ParserInt::Less(value_type v1, value_type v2) { return Round(v1) < Round(v2); }
	value_type ParserInt::Greater(value_type v1, value_type v2) { return Round(v1) > Round(v2); }
	value_type ParserInt::LessEq(value_type v1, value_type v2) { return Round(v1) <= Round(v2); }
	value_type ParserInt::GreaterEq(value_type v1, value_type v2) { return Round(v1) >= Round(v2); }
	value_type ParserInt::Equal(value_type v1, value_type v2) { return Round(v1) == Round(v2); }
	value_type ParserInt::NotEqual(value_type v1, value_type v2) { return Round(v1) != Round(v2); }
	value_type ParserInt::Not(value_type v) { return !Round(v); }

	value_type ParserInt::Pow(value_type v1, value_type v2)
	{
		return std::pow((double)Round(v1), (double)Round(v2));
	}


	value_type ParserInt::UnaryMinus(value_type v)
	{
		return -Round(v);
	}


	value_type ParserInt::Sum(const value_type* a_afArg, int a_iArgc)
	{
		if (!a_iArgc)
			throw ParserError(_T("too few arguments for function sum."));

		value_type fRes = 0;
		for (int i = 0; i < a_iArgc; ++i)
			fRes += a_afArg[i];

		return fRes;
	}


	value_type ParserInt::Min(const value_type* a_afArg, int a_iArgc)
	{
		if (!a_iArgc)
			throw ParserError(_T("too few arguments for function min."));

		value_type fRes = a_afArg[0];
		for (int i = 0; i < a_iArgc; ++i)
			fRes = std::min(fRes, a_afArg[i]);

		return fRes;
	}


	value_type ParserInt::Max(const value_type* a_afArg, int a_iArgc)
	{
		if (!a_iArgc)
			throw ParserError(_T("too few arguments for function min."));

		value_type fRes = a_afArg[0];
		for (int i = 0; i < a_iArgc; ++i)
			fRes = std::max(fRes, a_afArg[i]);

		return fRes;
	}


	int ParserInt::IsVal(const char_type* a_szExpr, int* a_iPos, value_type* a_fVal)
	{
		string_type buf(a_szExpr);
		std::size_t pos = buf.find_first_not_of(_T("0123456789"));

		if (pos == std::string::npos)
			return 0;

		stringstream_type stream(buf.substr(0, pos));
		int iVal(0);

		stream >> iVal;
		if (stream.fail())
			return 0;

		stringstream_type::pos_type iEnd = stream.tellg();   // Position after reading
		if (stream.fail())
			iEnd = stream.str().length();

		if (iEnd == (stringstream_type::pos_type) - 1)
			return 0;

		*a_iPos += (int)iEnd;
		*a_fVal = (value_type)iVal;
		return 1;
	}


	/** \brief Check a given position in the expression for the presence of
			   a hex value.
		\param a_szExpr Pointer to the expression string
		\param [in/out] a_iPos Pointer to an integer value holding the current parsing
			   position in the expression.
		\param [out] a_fVal Pointer to the position where the detected value shall be stored.

	  Hey values must be prefixed with "0x" in order to be detected properly.
	*/
	int ParserInt::IsHexVal(const char_type* a_szExpr, int* a_iPos, value_type* a_fVal)
	{
		if (a_szExpr[1] == 0 || (a_szExpr[0] != '0' || a_szExpr[1] != 'x'))
			return 0;

		unsigned iVal(0);

		// New code based on streams for UNICODE compliance:
		stringstream_type::pos_type nPos(0);
		stringstream_type ss(a_szExpr + 2);
		ss >> std::hex >> iVal;
		nPos = ss.tellg();

		if (nPos == (stringstream_type::pos_type)0)
			return 1;

		*a_iPos += (int)(2 + nPos);
		*a_fVal = (value_type)iVal;
		return 1;
	}


	int ParserInt::IsBinVal(const char_type* a_szExpr, int* a_iPos, value_type* a_fVal)
	{
		if (a_szExpr[0] != '#')
			return 0;

		unsigned iVal(0),
			iBits(sizeof(iVal) * 8),
			i(0);

		for (i = 0; (a_szExpr[i + 1] == '0' || a_szExpr[i + 1] == '1') && i < iBits; ++i)
			iVal |= (int)(a_szExpr[i + 1] == '1') << ((iBits - 1) - i);

		if (i == 0)
			return 0;

		if (i == iBits)
			throw exception_type(_T("Binary to integer conversion error (overflow)."));

		*a_fVal = (unsigned)(iVal >> (iBits - i));
		*a_iPos += i + 1;

		return 1;
	}


	/** \brief Constructor.

		Call ParserBase class constructor and trigger Function, Operator and Constant initialization.
	*/
	ParserInt::ParserInt()
		:ParserBase()
	{
		AddValIdent(IsVal);    // lowest priority
		AddValIdent(IsBinVal);
		AddValIdent(IsHexVal); // highest priority

		InitCharSets();
		InitFun();
		InitOprt();
	}


	void ParserInt::InitConst()
	{
	}


	void ParserInt::InitCharSets()
	{
		DefineNameChars(_T("0123456789_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"));
		DefineOprtChars(_T("+-*^/?<>=!%&|~'_"));
		DefineInfixOprtChars(_T("/+-*^?<>=!%&|~'_"));
	}


	/** \brief Initialize the default functions. */
	void ParserInt::InitFun()
	{
		DefineFun(_T("sign"), Sign);
		DefineFun(_T("abs"), Abs);
		DefineFun(_T("if"), Ite);
		DefineFun(_T("sum"), Sum);
		DefineFun(_T("min"), Min);
		DefineFun(_T("max"), Max);
	}


	/** \brief Initialize operators. */
	void ParserInt::InitOprt()
	{
		// disable all built in operators, not all of them useful for integer numbers
		// (they don't do rounding of values)
		EnableBuiltInOprt(false);

		// Disable all built in operators, they won't work with integer numbers
		// since they are designed for floating point numbers
		DefineInfixOprt(_T("-"), UnaryMinus);
		DefineInfixOprt(_T("!"), Not);

		DefineOprt(_T("&"), BitAnd, prBAND);
		DefineOprt(_T("|"), BitOr, prBOR);

		DefineOprt(_T("&&"), And, prLAND);
		DefineOprt(_T("||"), Or, prLOR);

		DefineOprt(_T("<"), Less, prCMP);
		DefineOprt(_T(">"), Greater, prCMP);
		DefineOprt(_T("<="), LessEq, prCMP);
		DefineOprt(_T(">="), GreaterEq, prCMP);
		DefineOprt(_T("=="), Equal, prCMP);
		DefineOprt(_T("!="), NotEqual, prCMP);

		DefineOprt(_T("+"), Add, prADD_SUB);
		DefineOprt(_T("-"), Sub, prADD_SUB);

		DefineOprt(_T("*"), Mul, prMUL_DIV);
		DefineOprt(_T("/"), Div, prMUL_DIV);
		DefineOprt(_T("%"), Mod, prMUL_DIV);

		DefineOprt(_T("^"), Pow, prPOW, oaRIGHT);
		DefineOprt(_T(">>"), Shr, prMUL_DIV + 1);
		DefineOprt(_T("<<"), Shl, prMUL_DIV + 1);
	}

} // namespace mu
