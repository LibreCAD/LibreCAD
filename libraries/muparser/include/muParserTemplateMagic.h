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

#ifndef MU_PARSER_TEMPLATE_MAGIC_H
#define MU_PARSER_TEMPLATE_MAGIC_H

#include <algorithm>
#include <cmath>
#include "muParserError.h"


namespace mu
{
	//-----------------------------------------------------------------------------------------------
	//
	// Compile time type detection
	//
	//-----------------------------------------------------------------------------------------------

	/** \brief A class singling out integer types at compile time using
			   template meta programming.
	*/
	template<typename T>
	struct TypeInfo
	{
		static bool IsInteger() { return false; }
	};

	template<>
	struct TypeInfo<char>
	{
		static bool IsInteger() { return true; }
	};

	template<>
	struct TypeInfo<short>
	{
		static bool IsInteger() { return true; }
	};

	template<>
	struct TypeInfo<int>
	{
		static bool IsInteger() { return true; }
	};

	template<>
	struct TypeInfo<long>
	{
		static bool IsInteger() { return true; }
	};

	template<>
	struct TypeInfo<unsigned char>
	{
		static bool IsInteger() { return true; }
	};

	template<>
	struct TypeInfo<unsigned short>
	{
		static bool IsInteger() { return true; }
	};

	template<>
	struct TypeInfo<unsigned int>
	{
		static bool IsInteger() { return true; }
	};

	template<>
	struct TypeInfo<unsigned long>
	{
		static bool IsInteger() { return true; }
	};


	//-----------------------------------------------------------------------------------------------
	//
	// Standard math functions with dummy overload for integer types
	//
	//-----------------------------------------------------------------------------------------------

	/** \brief A template class for providing wrappers for essential math functions.

	  This template is spezialized for several types in order to provide a unified interface
	  for parser internal math function calls regardless of the data type.
	*/
	template<typename T>
	struct MathImpl
	{
		static T Sin(T v) { return sin(v); }
		static T Cos(T v) { return cos(v); }
		static T Tan(T v) { return tan(v); }
		static T ASin(T v) { return asin(v); }
		static T ACos(T v) { return acos(v); }
		static T ATan(T v) { return atan(v); }
		static T ATan2(T v1, T v2) { return atan2(v1, v2); }
		static T Sinh(T v) { return sinh(v); }
		static T Cosh(T v) { return cosh(v); }
		static T Tanh(T v) { return tanh(v); }
		static T ASinh(T v) { return log(v + sqrt(v * v + 1)); }
		static T ACosh(T v) { return log(v + sqrt(v * v - 1)); }
		static T ATanh(T v) { return ((T)0.5 * log((1 + v) / (1 - v))); }
		static T Log(T v) { return log(v); }
		static T Log2(T v) { return log(v) / log((T)2); } // Logarithm base 2
		static T Log10(T v) { return log10(v); }         // Logarithm base 10
		static T Exp(T v) { return exp(v); }
		static T Abs(T v) { return (v >= 0) ? v : -v; }
		static T Sqrt(T v) { return sqrt(v); }
		static T Rint(T v) { return floor(v + (T)0.5); }
		static T Sign(T v) { return (T)((v < 0) ? -1 : (v > 0) ? 1 : 0); }
		static T Pow(T v1, T v2) { return std::pow(v1, v2); }

		static T UnaryMinus(T v) { return -v; }
		static T UnaryPlus(T v) { return v; }

		static T Sum(const T *a_afArg, int a_iArgc)
		{
			if (!a_iArgc)
				throw ParserError(_T("too few arguments for function sum."));

			T fRes = 0;
			for (int i = 0; i < a_iArgc; ++i) fRes += a_afArg[i];
			return fRes;
		}

		static T Avg(const T *a_afArg, int a_iArgc)
		{
			if (!a_iArgc)
				throw ParserError(_T("too few arguments for function avg."));

			T fRes = 0;
			for (int i = 0; i < a_iArgc; ++i) fRes += a_afArg[i];
			return fRes / (T)a_iArgc;
		}

		static T Min(const T *a_afArg, int a_iArgc)
		{
			if (!a_iArgc)
				throw ParserError(_T("too few arguments for function min."));

			T fRes = a_afArg[0];
			for (int i = 0; i < a_iArgc; ++i)
				fRes = std::min(fRes, a_afArg[i]);

			return fRes;
		}

		static T Max(const T *a_afArg, int a_iArgc)
		{
			if (!a_iArgc)
				throw ParserError(_T("too few arguments for function max."));

			T fRes = a_afArg[0];
			for (int i = 0; i < a_iArgc; ++i) fRes = std::max(fRes, a_afArg[i]);

			return fRes;
		}


#if defined (__GNUG__)
		// Bei zu genauer definition von pi kann die Berechnung von
		// sin(pi*a) mit a=1 10 x langsamer sein! 
		static constexpr T CONST_PI = (T)3.141592653589;
#else
		static constexpr T CONST_PI = (T)3.141592653589793238462643;
#endif

		static constexpr T CONST_E = (T)2.718281828459045235360287;
	};
}

#endif
