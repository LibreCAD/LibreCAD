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

#ifndef MUP_DEF_H
#define MUP_DEF_H

#include <iostream>
#include <string>
#include <sstream>
#include <map>

#include "muParserFixes.h"

/** \file
	\brief This file contains standard definitions used by the parser.
*/

/** \brief Define the base datatype for values.

  This datatype must be a built in value type. You can not use custom classes.
  It should be working with all types except "int"!
*/
#define MUP_BASETYPE double

/** \brief Activate this option in order to compile with OpenMP support.

  OpenMP is used only in the bulk mode it may increase the performance a bit.

  !!! DO NOT ACTIVATE THIS MACRO HERE IF YOU USE CMAKE FOR BUILDING !!!

  use the cmake option instead!
*/
//#define MUP_USE_OPENMP

#if defined(_UNICODE)
	/** \brief Definition of the basic parser string type. */
	#define MUP_STRING_TYPE std::wstring

	#if !defined(_T)
		#define _T(x) L##x
	#endif // not defined _T
#else
	#ifndef _T
		#define _T(x) x
	#endif

	/** \brief Definition of the basic parser string type. */
	#define MUP_STRING_TYPE std::string
#endif

/** \brief An assertion that does not kill the program. */
#define MUP_ASSERT(COND)											\
            if (!(COND))											\
            {														\
              stringstream_type ss;									\
              ss << _T("Assertion \"") _T(#COND) _T("\" failed: ")	\
                 << __FILE__ << _T(" line ")						\
                 << __LINE__ << _T(".");							\
              throw ParserError( ecINTERNAL_ERROR, -1, ss.str());   \
            }

#if defined(_MSC_VER)
	#pragma warning(push)
	#pragma warning(disable : 26812) 
#endif


namespace mu
{
#if defined(_UNICODE)

	/** \brief Encapsulate wcout. */
	inline std::wostream& console()
	{
		return std::wcout;
	}

	/** \brief Encapsulate cin. */
	inline std::wistream& console_in()
	{
		return std::wcin;
	}

#else

	/** \brief Encapsulate cout.

	  Used for supporting UNICODE more easily.
	*/
	inline std::ostream& console()
	{
		return std::cout;
	}

	/** \brief Encapsulate cin.

	  Used for supporting UNICODE more easily.
	*/
	inline std::istream& console_in()
	{
		return std::cin;
	}

#endif

	/** \brief Bytecode values.

		\attention The order of the operator entries must match the order in ParserBase::c_DefaultOprt!
	*/
	enum ECmdCode
	{
		// The following are codes for built in binary operators
		// apart from built in operators the user has the opportunity to
		// add user defined operators.
		cmLE = 0,			///< Operator item:  less or equal
		cmGE = 1,			///< Operator item:  greater or equal
		cmNEQ = 2,			///< Operator item:  not equal
		cmEQ = 3,			///< Operator item:  equals
		cmLT = 4,			///< Operator item:  less than
		cmGT = 5,			///< Operator item:  greater than
		cmADD = 6,			///< Operator item:  add
		cmSUB = 7,			///< Operator item:  subtract
		cmMUL = 8,			///< Operator item:  multiply
		cmDIV = 9,			///< Operator item:  division
		cmPOW = 10,			///< Operator item:  y to the power of ...
		cmLAND = 11,
		cmLOR = 12,
		cmASSIGN = 13,		///< Operator item:  Assignment operator
		cmBO = 14,			///< Operator item:  opening bracket
		cmBC = 15,			///< Operator item:  closing bracket
		cmIF = 16,			///< For use in the ternary if-then-else operator
		cmELSE = 17,		///< For use in the ternary if-then-else operator
		cmENDIF = 18,		///< For use in the ternary if-then-else operator
		cmARG_SEP = 19,		///< function argument separator
		cmVAR = 20,			///< variable item
		cmVAL = 21,			///< value item

		// For optimization purposes
		cmVARPOW2 = 22,
		cmVARPOW3 = 23,
		cmVARPOW4 = 24,
		cmVARMUL = 25,

		// operators and functions
		cmFUNC = 26,		///< Code for a generic function item
		cmFUNC_STR,			///< Code for a function with a string parameter
		cmFUNC_BULK,		///< Special callbacks for Bulk mode with an additional parameter for the bulk index 
		cmSTRING,			///< Code for a string token
		cmOPRT_BIN,			///< user defined binary operator
		cmOPRT_POSTFIX,		///< code for postfix operators
		cmOPRT_INFIX,		///< code for infix operators
		cmEND,				///< end of formula
		cmUNKNOWN			///< uninitialized item
	};

	/** \brief Types internally used by the parser.
	*/
	enum ETypeCode
	{
		tpSTR = 0,     ///< String type (Function arguments and constants only, no string variables)
		tpDBL = 1,     ///< Floating point variables
		tpVOID = 2      ///< Undefined type.
	};


	enum EParserVersionInfo
	{
		pviBRIEF,
		pviFULL
	};


	/** \brief Parser operator precedence values. */
	enum EOprtAssociativity
	{
		oaLEFT = 0,
		oaRIGHT = 1,
		oaNONE = 2
	};


	/** \brief Parser operator precedence values. */
	enum EOprtPrecedence
	{
		prLOR = 1,		///< logic or
		prLAND = 2,		///< logic and
		prBOR = 3,      ///< bitwise or
		prBAND = 4,     ///< bitwise and
		prCMP = 5,		///< comparsion operators
		prADD_SUB = 6,	///< addition
		prMUL_DIV = 7,	///< multiplication/division
		prPOW = 8,		///< power operator priority (highest)

		// infix operators
		prINFIX = 7,	///< Signs have a higher priority than ADD_SUB, but lower than power operator
		prPOSTFIX = 7	///< Postfix operator priority (currently unused)
	};


	/** \brief Error codes. */
	enum EErrorCodes
	{
		// Formula syntax errors
		ecUNEXPECTED_OPERATOR = 0,	///< Unexpected binary operator found
		ecUNASSIGNABLE_TOKEN = 1,	///< Token can't be identified.
		ecUNEXPECTED_EOF = 2,		///< Unexpected end of formula. (Example: "2+sin(")
		ecUNEXPECTED_ARG_SEP = 3,	///< An unexpected comma has been found. (Example: "1,23")
		ecUNEXPECTED_ARG = 4,		///< An unexpected argument has been found
		ecUNEXPECTED_VAL = 5,		///< An unexpected value token has been found
		ecUNEXPECTED_VAR = 6,		///< An unexpected variable token has been found
		ecUNEXPECTED_PARENS = 7,	///< Unexpected Parenthesis, opening or closing
		ecUNEXPECTED_STR = 8,		///< A string has been found at an inapropriate position
		ecSTRING_EXPECTED = 9,		///< A string function has been called with a different type of argument
		ecVAL_EXPECTED = 10,		///< A numerical function has been called with a non value type of argument
		ecMISSING_PARENS = 11,		///< Missing parens. (Example: "3*sin(3")
		ecUNEXPECTED_FUN = 12,		///< Unexpected function found. (Example: "sin(8)cos(9)")
		ecUNTERMINATED_STRING = 13,	///< unterminated string constant. (Example: "3*valueof("hello)")
		ecTOO_MANY_PARAMS = 14,		///< Too many function parameters
		ecTOO_FEW_PARAMS = 15,		///< Too few function parameters. (Example: "ite(1<2,2)")
		ecOPRT_TYPE_CONFLICT = 16,	///< binary operators may only be applied to value items of the same type
		ecSTR_RESULT = 17,			///< result is a string

		// Invalid Parser input Parameters
		ecINVALID_NAME = 18,			///< Invalid function, variable or constant name.
		ecINVALID_BINOP_IDENT = 19,		///< Invalid binary operator identifier
		ecINVALID_INFIX_IDENT = 20,		///< Invalid function, variable or constant name.
		ecINVALID_POSTFIX_IDENT = 21,	///< Invalid function, variable or constant name.

		ecBUILTIN_OVERLOAD = 22, ///< Trying to overload builtin operator
		ecINVALID_FUN_PTR = 23, ///< Invalid callback function pointer 
		ecINVALID_VAR_PTR = 24, ///< Invalid variable pointer 
		ecEMPTY_EXPRESSION = 25, ///< The Expression is empty
		ecNAME_CONFLICT = 26, ///< Name conflict
		ecOPT_PRI = 27, ///< Invalid operator priority
		// 
		ecDOMAIN_ERROR = 28, ///< catch division by zero, sqrt(-1), log(0) (currently unused)
		ecDIV_BY_ZERO = 29, ///< Division by zero (currently unused)
		ecGENERIC = 30, ///< Generic error
		ecLOCALE = 31, ///< Conflict with current locale

		ecUNEXPECTED_CONDITIONAL = 32,
		ecMISSING_ELSE_CLAUSE = 33,
		ecMISPLACED_COLON = 34,

		ecUNREASONABLE_NUMBER_OF_COMPUTATIONS = 35,

		ecIDENTIFIER_TOO_LONG = 36, ///< Thrown when an identifier with more then 255 characters is used.

		ecEXPRESSION_TOO_LONG = 37, ///< Throw an exception if the expression has more than 10000 characters. (an arbitrary limit)

		ecINVALID_CHARACTERS_FOUND = 38,///< The expression or identifier contains invalid non printable characters

		// internal errors
		ecINTERNAL_ERROR = 39,			///< Internal error of any kind.

		ecBYTECODE_IMPORT_EXPORT_DISABLED = 40,	///< Bytecode cannot be exported.

		// The last two are special entries 
		ecCOUNT,                      ///< This is no error code, It just stores just the total number of error codes
		ecUNDEFINED = -1  ///< Undefined message, placeholder to detect unassigned error messages
	};

	//------------------------------------------------------------------------------
	// Basic Types
	//------------------------------------------------------------------------------

	/** \brief The numeric datatype used by the parser.

	  Normally this is a floating point type either single or double precision.
	*/
	typedef MUP_BASETYPE value_type;

	/** \brief The stringtype used by the parser.

	  Depends on whether UNICODE is used or not.
	*/
	typedef MUP_STRING_TYPE string_type;

	/** \brief The character type used by the parser.

	  Depends on whether UNICODE is used or not.
	*/
	typedef string_type::value_type char_type;

	/** \brief Typedef for easily using stringstream that respect the parser stringtype. */
	typedef std::basic_stringstream<char_type, std::char_traits<char_type>, std::allocator<char_type> > stringstream_type;

	// Data container types

	/** \brief Type used for storing variables. */
	typedef std::map<string_type, value_type*> varmap_type;

	/** \brief Type used for storing constants. */
	typedef std::map<string_type, value_type> valmap_type;

	/** \brief Type for assigning a string name to an index in the internal string table. */
	typedef std::map<string_type, std::size_t> strmap_type;

	// Parser callbacks

	/** \brief Function type used to erase type.  Voluntarily needs explicit cast with all other *fun_type*. */
	typedef void(*erased_fun_type)();

	/** \brief Callback type used for functions without arguments. */
	typedef value_type(*fun_type0)();

	/** \brief Callback type used for functions with a single arguments. */
	typedef value_type(*fun_type1)(value_type);

	/** \brief Callback type used for functions with two arguments. */
	typedef value_type(*fun_type2)(value_type, value_type);

	/** \brief Callback type used for functions with three arguments. */
	typedef value_type(*fun_type3)(value_type, value_type, value_type);

	/** \brief Callback type used for functions with four arguments. */
	typedef value_type(*fun_type4)(value_type, value_type, value_type, value_type);

	/** \brief Callback type used for functions with five arguments. */
	typedef value_type(*fun_type5)(value_type, value_type, value_type, value_type, value_type);

	/** \brief Callback type used for functions with six arguments. */
	typedef value_type(*fun_type6)(value_type, value_type, value_type, value_type, value_type, value_type);

	/** \brief Callback type used for functions with seven arguments. */
	typedef value_type(*fun_type7)(value_type, value_type, value_type, value_type, value_type, value_type, value_type);

	/** \brief Callback type used for functions with eight arguments. */
	typedef value_type(*fun_type8)(value_type, value_type, value_type, value_type, value_type, value_type, value_type, value_type);

	/** \brief Callback type used for functions with nine arguments. */
	typedef value_type(*fun_type9)(value_type, value_type, value_type, value_type, value_type, value_type, value_type, value_type, value_type);

	/** \brief Callback type used for functions with ten arguments. */
	typedef value_type(*fun_type10)(value_type, value_type, value_type, value_type, value_type, value_type, value_type, value_type, value_type, value_type);

	/** \brief Callback type with user data (not null) used for functions without arguments. */
	typedef value_type(*fun_userdata_type0)(void*);

	/** \brief Callback type with user data (not null) used for functions with a single arguments. */
	typedef value_type(*fun_userdata_type1)(void*, value_type);

	/** \brief Callback type with user data (not null) used for functions with two arguments. */
	typedef value_type(*fun_userdata_type2)(void*, value_type, value_type);

	/** \brief Callback type with user data (not null) used for functions with three arguments. */
	typedef value_type(*fun_userdata_type3)(void*, value_type, value_type, value_type);

	/** \brief Callback type with user data (not null) used for functions with four arguments. */
	typedef value_type(*fun_userdata_type4)(void*, value_type, value_type, value_type, value_type);

	/** \brief Callback type with user data (not null) used for functions with five arguments. */
	typedef value_type(*fun_userdata_type5)(void*, value_type, value_type, value_type, value_type, value_type);

	/** \brief Callback type with user data (not null) used for functions with six arguments. */
	typedef value_type(*fun_userdata_type6)(void*, value_type, value_type, value_type, value_type, value_type, value_type);

	/** \brief Callback type with user data (not null) used for functions with seven arguments. */
	typedef value_type(*fun_userdata_type7)(void*, value_type, value_type, value_type, value_type, value_type, value_type, value_type);

	/** \brief Callback type with user data (not null) used for functions with eight arguments. */
	typedef value_type(*fun_userdata_type8)(void*, value_type, value_type, value_type, value_type, value_type, value_type, value_type, value_type);

	/** \brief Callback type with user data (not null) used for functions with nine arguments. */
	typedef value_type(*fun_userdata_type9)(void*, value_type, value_type, value_type, value_type, value_type, value_type, value_type, value_type, value_type);

	/** \brief Callback type with user data (not null) used for functions with ten arguments. */
	typedef value_type(*fun_userdata_type10)(void*, value_type, value_type, value_type, value_type, value_type, value_type, value_type, value_type, value_type, value_type);

	/** \brief Callback type used for functions without arguments. */
	typedef value_type(*bulkfun_type0)(int, int);

	/** \brief Callback type used for functions with a single arguments. */
	typedef value_type(*bulkfun_type1)(int, int, value_type);

	/** \brief Callback type used for functions with two arguments. */
	typedef value_type(*bulkfun_type2)(int, int, value_type, value_type);

	/** \brief Callback type used for functions with three arguments. */
	typedef value_type(*bulkfun_type3)(int, int, value_type, value_type, value_type);

	/** \brief Callback type used for functions with four arguments. */
	typedef value_type(*bulkfun_type4)(int, int, value_type, value_type, value_type, value_type);

	/** \brief Callback type used for functions with five arguments. */
	typedef value_type(*bulkfun_type5)(int, int, value_type, value_type, value_type, value_type, value_type);

	/** \brief Callback type used for functions with six arguments. */
	typedef value_type(*bulkfun_type6)(int, int, value_type, value_type, value_type, value_type, value_type, value_type);

	/** \brief Callback type used for functions with seven arguments. */
	typedef value_type(*bulkfun_type7)(int, int, value_type, value_type, value_type, value_type, value_type, value_type, value_type);

	/** \brief Callback type used for functions with eight arguments. */
	typedef value_type(*bulkfun_type8)(int, int, value_type, value_type, value_type, value_type, value_type, value_type, value_type, value_type);

	/** \brief Callback type used for functions with nine arguments. */
	typedef value_type(*bulkfun_type9)(int, int, value_type, value_type, value_type, value_type, value_type, value_type, value_type, value_type, value_type);

	/** \brief Callback type used for functions with ten arguments. */
	typedef value_type(*bulkfun_type10)(int, int, value_type, value_type, value_type, value_type, value_type, value_type, value_type, value_type, value_type, value_type);

	/** \brief Callback type with user data (not null) used for functions without arguments. */
	typedef value_type(*bulkfun_userdata_type0)(void*, int, int);

	/** \brief Callback type with user data (not null) used for functions with a single arguments. */
	typedef value_type(*bulkfun_userdata_type1)(void*, int, int, value_type);

	/** \brief Callback type with user data (not null) used for functions with two arguments. */
	typedef value_type(*bulkfun_userdata_type2)(void*, int, int, value_type, value_type);

	/** \brief Callback type with user data (not null) used for functions with three arguments. */
	typedef value_type(*bulkfun_userdata_type3)(void*, int, int, value_type, value_type, value_type);

	/** \brief Callback type with user data (not null) used for functions with four arguments. */
	typedef value_type(*bulkfun_userdata_type4)(void*, int, int, value_type, value_type, value_type, value_type);

	/** \brief Callback type with user data (not null) used for functions with five arguments. */
	typedef value_type(*bulkfun_userdata_type5)(void*, int, int, value_type, value_type, value_type, value_type, value_type);

	/** \brief Callback type with user data (not null) used for functions with six arguments. */
	typedef value_type(*bulkfun_userdata_type6)(void*, int, int, value_type, value_type, value_type, value_type, value_type, value_type);

	/** \brief Callback type with user data (not null) used for functions with seven arguments. */
	typedef value_type(*bulkfun_userdata_type7)(void*, int, int, value_type, value_type, value_type, value_type, value_type, value_type, value_type);

	/** \brief Callback type with user data (not null) used for functions with eight arguments. */
	typedef value_type(*bulkfun_userdata_type8)(void*, int, int, value_type, value_type, value_type, value_type, value_type, value_type, value_type, value_type);

	/** \brief Callback type with user data (not null) used for functions with nine arguments. */
	typedef value_type(*bulkfun_userdata_type9)(void*, int, int, value_type, value_type, value_type, value_type, value_type, value_type, value_type, value_type, value_type);

	/** \brief Callback type with user data (not null) used for functions with ten arguments. */
	typedef value_type(*bulkfun_userdata_type10)(void*, int, int, value_type, value_type, value_type, value_type, value_type, value_type, value_type, value_type, value_type, value_type);

	/** \brief Callback type used for functions with a variable argument list. */
	typedef value_type(*multfun_type)(const value_type*, int);

	/** \brief Callback type with user data (not null) used for functions and a variable argument list. */
	typedef value_type(*multfun_userdata_type)(void*, const value_type*, int);

	/** \brief Callback type used for functions taking a string as an argument. */
	typedef value_type(*strfun_type1)(const char_type*);

	/** \brief Callback type used for functions taking a string and a value as arguments. */
	typedef value_type(*strfun_type2)(const char_type*, value_type);

	/** \brief Callback type used for functions taking a string and two values as arguments. */
	typedef value_type(*strfun_type3)(const char_type*, value_type, value_type);

	/** \brief Callback type used for functions taking a string and three values as arguments. */
	typedef value_type(*strfun_type4)(const char_type*, value_type, value_type, value_type);

	/** \brief Callback type used for functions taking a string and four values as arguments. */
	typedef value_type(*strfun_type5)(const char_type*, value_type, value_type, value_type, value_type);

	/** \brief Callback type used for functions taking a string and five values as arguments. */
	typedef value_type(*strfun_type6)(const char_type*, value_type, value_type, value_type, value_type, value_type);

	/** \brief Callback type with user data (not null) used for functions taking a string as an argument. */
	typedef value_type(*strfun_userdata_type1)(void*, const char_type*);

	/** \brief Callback type with user data (not null) used for functions taking a string and a value as arguments. */
	typedef value_type(*strfun_userdata_type2)(void*, const char_type*, value_type);

	/** \brief Callback type with user data (not null) used for functions taking a string and two values as arguments. */
	typedef value_type(*strfun_userdata_type3)(void*, const char_type*, value_type, value_type);

	/** \brief Callback type with user data (not null) used for functions taking a string and a value as arguments. */
	typedef value_type(*strfun_userdata_type4)(void*, const char_type*, value_type, value_type, value_type);

	/** \brief Callback type with user data (not null) used for functions taking a string and two values as arguments. */
	typedef value_type(*strfun_userdata_type5)(void*, const char_type*, value_type, value_type, value_type, value_type);

	/** \brief Callback type with user data (not null) used for functions taking a string and five values as arguments. */
	typedef value_type(*strfun_userdata_type6)(void*, const char_type*, value_type, value_type, value_type, value_type, value_type);

	/** \brief Callback used for functions that identify values in a string. */
	typedef int (*identfun_type)(const char_type* sExpr, int* nPos, value_type* fVal);

	/** \brief Callback used for variable creation factory functions. */
	typedef value_type* (*facfun_type)(const char_type*, void*);

	static const int MaxLenExpression = 20000;
	static const int MaxLenIdentifier = 100;
	static const string_type ParserVersion = string_type(_T("2.3.4 (Develop)"));
	static const string_type ParserVersionDate = string_type(_T("20231224"));
} // end of namespace

#if defined(_MSC_VER)
	#pragma warning(pop)
#endif

#endif

