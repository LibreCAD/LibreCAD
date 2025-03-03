/*

	 _____  __ _____________ _______  ______ ___________
	/     \|  |  \____ \__  \\_  __ \/  ___// __ \_  __ \
   |  Y Y  \  |  /  |_> > __ \|  | \/\___ \\  ___/|  | \/
   |__|_|  /____/|   __(____  /__|  /____  >\___  >__|
		 \/      |__|       \/           \/     \/
   Copyright (C) 2004 - 2023 Ingo Berg

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

#ifndef MU_PARSER_DLL_H
#define MU_PARSER_DLL_H

#include "muParserFixes.h"

#ifdef __cplusplus
extern "C"
{
#endif

	/** \file
		\brief This file contains the DLL interface of muparser.
	*/

	// Basic types
	typedef void* muParserHandle_t;    // parser handle

#ifndef _UNICODE
	typedef char   muChar_t;            // character type
#else
	typedef wchar_t   muChar_t;            // character type
#endif

	typedef int    muBool_t;            // boolean type
	typedef int    muInt_t;             // integer type 
	typedef double muFloat_t;           // floating point type

	// function types for calculation
	typedef muFloat_t(*muFun0_t)(void);
	typedef muFloat_t(*muFun1_t)(muFloat_t);
	typedef muFloat_t(*muFun2_t)(muFloat_t, muFloat_t);
	typedef muFloat_t(*muFun3_t)(muFloat_t, muFloat_t, muFloat_t);
	typedef muFloat_t(*muFun4_t)(muFloat_t, muFloat_t, muFloat_t, muFloat_t);
	typedef muFloat_t(*muFun5_t)(muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t);
	typedef muFloat_t(*muFun6_t)(muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t);
	typedef muFloat_t(*muFun7_t)(muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t);
	typedef muFloat_t(*muFun8_t)(muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t);
	typedef muFloat_t(*muFun9_t)(muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t);
	typedef muFloat_t(*muFun10_t)(muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t);
	// with user data (not null)
	typedef muFloat_t(*muFunUserData0_t)(void*);
	typedef muFloat_t(*muFunUserData1_t)(void*, muFloat_t);
	typedef muFloat_t(*muFunUserData2_t)(void*, muFloat_t, muFloat_t);
	typedef muFloat_t(*muFunUserData3_t)(void*, muFloat_t, muFloat_t, muFloat_t);
	typedef muFloat_t(*muFunUserData4_t)(void*, muFloat_t, muFloat_t, muFloat_t, muFloat_t);
	typedef muFloat_t(*muFunUserData5_t)(void*, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t);
	typedef muFloat_t(*muFunUserData6_t)(void*, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t);
	typedef muFloat_t(*muFunUserData7_t)(void*, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t);
	typedef muFloat_t(*muFunUserData8_t)(void*, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t);
	typedef muFloat_t(*muFunUserData9_t)(void*, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t);
	typedef muFloat_t(*muFunUserData10_t)(void*, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t);

	// Function prototypes for bulkmode functions
	typedef muFloat_t(*muBulkFun0_t)(int, int);
	typedef muFloat_t(*muBulkFun1_t)(int, int, muFloat_t);
	typedef muFloat_t(*muBulkFun2_t)(int, int, muFloat_t, muFloat_t);
	typedef muFloat_t(*muBulkFun3_t)(int, int, muFloat_t, muFloat_t, muFloat_t);
	typedef muFloat_t(*muBulkFun4_t)(int, int, muFloat_t, muFloat_t, muFloat_t, muFloat_t);
	typedef muFloat_t(*muBulkFun5_t)(int, int, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t);
	typedef muFloat_t(*muBulkFun6_t)(int, int, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t);
	typedef muFloat_t(*muBulkFun7_t)(int, int, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t);
	typedef muFloat_t(*muBulkFun8_t)(int, int, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t);
	typedef muFloat_t(*muBulkFun9_t)(int, int, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t);
	typedef muFloat_t(*muBulkFun10_t)(int, int, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t);
	// with user data (not null)
	typedef muFloat_t(*muBulkFunUserData0_t)(void*, int, int);
	typedef muFloat_t(*muBulkFunUserData1_t)(void*, int, int, muFloat_t);
	typedef muFloat_t(*muBulkFunUserData2_t)(void*, int, int, muFloat_t, muFloat_t);
	typedef muFloat_t(*muBulkFunUserData3_t)(void*, int, int, muFloat_t, muFloat_t, muFloat_t);
	typedef muFloat_t(*muBulkFunUserData4_t)(void*, int, int, muFloat_t, muFloat_t, muFloat_t, muFloat_t);
	typedef muFloat_t(*muBulkFunUserData5_t)(void*, int, int, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t);
	typedef muFloat_t(*muBulkFunUserData6_t)(void*, int, int, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t);
	typedef muFloat_t(*muBulkFunUserData7_t)(void*, int, int, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t);
	typedef muFloat_t(*muBulkFunUserData8_t)(void*, int, int, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t);
	typedef muFloat_t(*muBulkFunUserData9_t)(void*, int, int, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t);
	typedef muFloat_t(*muBulkFunUserData10_t)(void*, int, int, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t, muFloat_t);

	typedef muFloat_t(*muMultFun_t)(const muFloat_t*, muInt_t);
	typedef muFloat_t(*muMultFunUserData_t)(void*, const muFloat_t*, muInt_t); // with user data (not null)

	typedef muFloat_t(*muStrFun1_t)(const muChar_t*);
	typedef muFloat_t(*muStrFun2_t)(const muChar_t*, muFloat_t);
	typedef muFloat_t(*muStrFun3_t)(const muChar_t*, muFloat_t, muFloat_t);
	typedef muFloat_t(*muStrFun4_t)(const muChar_t*, muFloat_t, muFloat_t, muFloat_t);
	typedef muFloat_t(*muStrFun5_t)(const muChar_t*, muFloat_t, muFloat_t, muFloat_t, muFloat_t);
	// with user data (not null)
	typedef muFloat_t(*muStrFunUserData1_t)(void*, const muChar_t*);
	typedef muFloat_t(*muStrFunUserData2_t)(void*, const muChar_t*, muFloat_t);
	typedef muFloat_t(*muStrFunUserData3_t)(void*, const muChar_t*, muFloat_t, muFloat_t);
	typedef muFloat_t(*muStrFunUserData4_t)(void*, const muChar_t*, muFloat_t, muFloat_t, muFloat_t);
	typedef muFloat_t(*muStrFunUserData5_t)(void*, const muChar_t*, muFloat_t, muFloat_t, muFloat_t, muFloat_t);

	// Functions for parser management
	typedef void (*muErrorHandler_t)(muParserHandle_t a_hParser);           // [optional] callback to an error handler
	typedef muFloat_t* (*muFacFun_t)(const muChar_t*, void*);               // [optional] callback for creating new variables
	typedef muInt_t(*muIdentFun_t)(const muChar_t*, muInt_t*, muFloat_t*); // [optional] value identification callbacks

	//-----------------------------------------------------------------------------------------------------
	// Constants
	static const int muOPRT_ASCT_LEFT = 0;
	static const int muOPRT_ASCT_RIGHT = 1;

	static const int muBASETYPE_FLOAT = 0;
	static const int muBASETYPE_INT = 1;

	//-----------------------------------------------------------------------------------------------------
	//
	//
	// muParser C compatible bindings
	//
	//
	//-----------------------------------------------------------------------------------------------------


	// Basic operations / initialization  
	API_EXPORT(muParserHandle_t) mupCreate(int nBaseType);
	API_EXPORT(void) mupRelease(muParserHandle_t a_hParser);
	API_EXPORT(const muChar_t*) mupGetExpr(muParserHandle_t a_hParser);
	API_EXPORT(void) mupSetExpr(muParserHandle_t a_hParser, const muChar_t* a_szExpr);
	API_EXPORT(void) mupSetVarFactory(muParserHandle_t a_hParser, muFacFun_t a_pFactory, void* pUserData);
	API_EXPORT(const muChar_t*) mupGetVersion(muParserHandle_t a_hParser);
	API_EXPORT(muFloat_t) mupEval(muParserHandle_t a_hParser);
	API_EXPORT(muFloat_t*) mupEvalMulti(muParserHandle_t a_hParser, int* nNum);
	API_EXPORT(void) mupEvalBulk(muParserHandle_t a_hParser, muFloat_t* a_fResult, int nSize);

	// Defining callbacks / variables / constants
	API_EXPORT(void) mupDefineFun0(muParserHandle_t a_hParser, const muChar_t* a_szName, muFun0_t a_pFun, muBool_t a_bOptimize);
	API_EXPORT(void) mupDefineFun1(muParserHandle_t a_hParser, const muChar_t* a_szName, muFun1_t a_pFun, muBool_t a_bOptimize);
	API_EXPORT(void) mupDefineFun2(muParserHandle_t a_hParser, const muChar_t* a_szName, muFun2_t a_pFun, muBool_t a_bOptimize);
	API_EXPORT(void) mupDefineFun3(muParserHandle_t a_hParser, const muChar_t* a_szName, muFun3_t a_pFun, muBool_t a_bOptimize);
	API_EXPORT(void) mupDefineFun4(muParserHandle_t a_hParser, const muChar_t* a_szName, muFun4_t a_pFun, muBool_t a_bOptimize);
	API_EXPORT(void) mupDefineFun5(muParserHandle_t a_hParser, const muChar_t* a_szName, muFun5_t a_pFun, muBool_t a_bOptimize);
	API_EXPORT(void) mupDefineFun6(muParserHandle_t a_hParser, const muChar_t* a_szName, muFun6_t a_pFun, muBool_t a_bOptimize);
	API_EXPORT(void) mupDefineFun7(muParserHandle_t a_hParser, const muChar_t* a_szName, muFun7_t a_pFun, muBool_t a_bOptimize);
	API_EXPORT(void) mupDefineFun8(muParserHandle_t a_hParser, const muChar_t* a_szName, muFun8_t a_pFun, muBool_t a_bOptimize);
	API_EXPORT(void) mupDefineFun9(muParserHandle_t a_hParser, const muChar_t* a_szName, muFun9_t a_pFun, muBool_t a_bOptimize);
	API_EXPORT(void) mupDefineFun10(muParserHandle_t a_hParser, const muChar_t* a_szName, muFun10_t a_pFun, muBool_t a_bOptimize);
	// with user data (not null)
	API_EXPORT(void) mupDefineFunUserData0(muParserHandle_t a_hParser, const muChar_t* a_szName, muFunUserData0_t a_pFun, void* a_pUserData, muBool_t a_bOptimize);
	API_EXPORT(void) mupDefineFunUserData1(muParserHandle_t a_hParser, const muChar_t* a_szName, muFunUserData1_t a_pFun, void* a_pUserData, muBool_t a_bOptimize);
	API_EXPORT(void) mupDefineFunUserData2(muParserHandle_t a_hParser, const muChar_t* a_szName, muFunUserData2_t a_pFun, void* a_pUserData, muBool_t a_bOptimize);
	API_EXPORT(void) mupDefineFunUserData3(muParserHandle_t a_hParser, const muChar_t* a_szName, muFunUserData3_t a_pFun, void* a_pUserData, muBool_t a_bOptimize);
	API_EXPORT(void) mupDefineFunUserData4(muParserHandle_t a_hParser, const muChar_t* a_szName, muFunUserData4_t a_pFun, void* a_pUserData, muBool_t a_bOptimize);
	API_EXPORT(void) mupDefineFunUserData5(muParserHandle_t a_hParser, const muChar_t* a_szName, muFunUserData5_t a_pFun, void* a_pUserData, muBool_t a_bOptimize);
	API_EXPORT(void) mupDefineFunUserData6(muParserHandle_t a_hParser, const muChar_t* a_szName, muFunUserData6_t a_pFun, void* a_pUserData, muBool_t a_bOptimize);
	API_EXPORT(void) mupDefineFunUserData7(muParserHandle_t a_hParser, const muChar_t* a_szName, muFunUserData7_t a_pFun, void* a_pUserData, muBool_t a_bOptimize);
	API_EXPORT(void) mupDefineFunUserData8(muParserHandle_t a_hParser, const muChar_t* a_szName, muFunUserData8_t a_pFun, void* a_pUserData, muBool_t a_bOptimize);
	API_EXPORT(void) mupDefineFunUserData9(muParserHandle_t a_hParser, const muChar_t* a_szName, muFunUserData9_t a_pFun, void* a_pUserData, muBool_t a_bOptimize);
	API_EXPORT(void) mupDefineFunUserData10(muParserHandle_t a_hParser, const muChar_t* a_szName, muFunUserData10_t a_pFun, void* a_pUserData, muBool_t a_bOptimize);

	// Defining bulkmode functions
	API_EXPORT(void) mupDefineBulkFun0(muParserHandle_t a_hParser, const muChar_t* a_szName, muBulkFun0_t a_pFun);
	API_EXPORT(void) mupDefineBulkFun1(muParserHandle_t a_hParser, const muChar_t* a_szName, muBulkFun1_t a_pFun);
	API_EXPORT(void) mupDefineBulkFun2(muParserHandle_t a_hParser, const muChar_t* a_szName, muBulkFun2_t a_pFun);
	API_EXPORT(void) mupDefineBulkFun3(muParserHandle_t a_hParser, const muChar_t* a_szName, muBulkFun3_t a_pFun);
	API_EXPORT(void) mupDefineBulkFun4(muParserHandle_t a_hParser, const muChar_t* a_szName, muBulkFun4_t a_pFun);
	API_EXPORT(void) mupDefineBulkFun5(muParserHandle_t a_hParser, const muChar_t* a_szName, muBulkFun5_t a_pFun);
	API_EXPORT(void) mupDefineBulkFun6(muParserHandle_t a_hParser, const muChar_t* a_szName, muBulkFun6_t a_pFun);
	API_EXPORT(void) mupDefineBulkFun7(muParserHandle_t a_hParser, const muChar_t* a_szName, muBulkFun7_t a_pFun);
	API_EXPORT(void) mupDefineBulkFun8(muParserHandle_t a_hParser, const muChar_t* a_szName, muBulkFun8_t a_pFun);
	API_EXPORT(void) mupDefineBulkFun9(muParserHandle_t a_hParser, const muChar_t* a_szName, muBulkFun9_t a_pFun);
	API_EXPORT(void) mupDefineBulkFun10(muParserHandle_t a_hParser, const muChar_t* a_szName, muBulkFun10_t a_pFun);
	// with user data (not null)
	API_EXPORT(void) mupDefineBulkFunUserData0(muParserHandle_t a_hParser, const muChar_t* a_szName, muBulkFunUserData0_t a_pFun, void* a_pUserData);
	API_EXPORT(void) mupDefineBulkFunUserData1(muParserHandle_t a_hParser, const muChar_t* a_szName, muBulkFunUserData1_t a_pFun, void* a_pUserData);
	API_EXPORT(void) mupDefineBulkFunUserData2(muParserHandle_t a_hParser, const muChar_t* a_szName, muBulkFunUserData2_t a_pFun, void* a_pUserData);
	API_EXPORT(void) mupDefineBulkFunUserData3(muParserHandle_t a_hParser, const muChar_t* a_szName, muBulkFunUserData3_t a_pFun, void* a_pUserData);
	API_EXPORT(void) mupDefineBulkFunUserData4(muParserHandle_t a_hParser, const muChar_t* a_szName, muBulkFunUserData4_t a_pFun, void* a_pUserData);
	API_EXPORT(void) mupDefineBulkFunUserData5(muParserHandle_t a_hParser, const muChar_t* a_szName, muBulkFunUserData5_t a_pFun, void* a_pUserData);
	API_EXPORT(void) mupDefineBulkFunUserData6(muParserHandle_t a_hParser, const muChar_t* a_szName, muBulkFunUserData6_t a_pFun, void* a_pUserData);
	API_EXPORT(void) mupDefineBulkFunUserData7(muParserHandle_t a_hParser, const muChar_t* a_szName, muBulkFunUserData7_t a_pFun, void* a_pUserData);
	API_EXPORT(void) mupDefineBulkFunUserData8(muParserHandle_t a_hParser, const muChar_t* a_szName, muBulkFunUserData8_t a_pFun, void* a_pUserData);
	API_EXPORT(void) mupDefineBulkFunUserData9(muParserHandle_t a_hParser, const muChar_t* a_szName, muBulkFunUserData9_t a_pFun, void* a_pUserData);
	API_EXPORT(void) mupDefineBulkFunUserData10(muParserHandle_t a_hParser, const muChar_t* a_szName, muBulkFunUserData10_t a_pFun, void* a_pUserData);

	// string functions
	API_EXPORT(void) mupDefineStrFun1(muParserHandle_t a_hParser, const muChar_t* a_szName, muStrFun1_t a_pFun);
	API_EXPORT(void) mupDefineStrFun2(muParserHandle_t a_hParser, const muChar_t* a_szName, muStrFun2_t a_pFun);
	API_EXPORT(void) mupDefineStrFun3(muParserHandle_t a_hParser, const muChar_t* a_szName, muStrFun3_t a_pFun);
	API_EXPORT(void) mupDefineStrFun4(muParserHandle_t a_hParser, const muChar_t* a_szName, muStrFun4_t a_pFun);
	API_EXPORT(void) mupDefineStrFun5(muParserHandle_t a_hParser, const muChar_t* a_szName, muStrFun5_t a_pFun);
	// with user data (not null)
	API_EXPORT(void) mupDefineStrFunUserData1(muParserHandle_t a_hParser, const muChar_t* a_szName, muStrFunUserData1_t a_pFun, void* a_pUserData);
	API_EXPORT(void) mupDefineStrFunUserData2(muParserHandle_t a_hParser, const muChar_t* a_szName, muStrFunUserData2_t a_pFun, void* a_pUserData);
	API_EXPORT(void) mupDefineStrFunUserData3(muParserHandle_t a_hParser, const muChar_t* a_szName, muStrFunUserData3_t a_pFun, void* a_pUserData);
	API_EXPORT(void) mupDefineStrFunUserData4(muParserHandle_t a_hParser, const muChar_t* a_szName, muStrFunUserData4_t a_pFun, void* a_pUserData);
	API_EXPORT(void) mupDefineStrFunUserData5(muParserHandle_t a_hParser, const muChar_t* a_szName, muStrFunUserData5_t a_pFun, void* a_pUserData);

	API_EXPORT(void) mupDefineMultFun(muParserHandle_t a_hParser,
		const muChar_t* a_szName,
		muMultFun_t a_pFun,
		muBool_t a_bOptimize);
	// with user data (not null)
	API_EXPORT(void) mupDefineMultFunUserData(muParserHandle_t a_hParser,
		const muChar_t* a_szName,
		muMultFunUserData_t a_pFun,
		void* a_pUserData,
		muBool_t a_bOptimize);

	API_EXPORT(void) mupDefineOprt(muParserHandle_t a_hParser,
		const muChar_t* a_szName,
		muFun2_t a_pFun,
		muInt_t a_nPrec,
		muInt_t a_nOprtAsct,
		muBool_t a_bOptimize);

	API_EXPORT(void) mupDefineConst(muParserHandle_t a_hParser,
		const muChar_t* a_szName,
		muFloat_t a_fVal);

	API_EXPORT(void) mupDefineStrConst(muParserHandle_t a_hParser,
		const muChar_t* a_szName,
		const muChar_t* a_sVal);

	API_EXPORT(void) mupDefineVar(muParserHandle_t a_hParser,
		const muChar_t* a_szName,
		muFloat_t* a_fVar);

	API_EXPORT(void) mupDefineBulkVar(muParserHandle_t a_hParser,
		const muChar_t* a_szName,
		muFloat_t* a_fVar);

	API_EXPORT(void) mupDefinePostfixOprt(muParserHandle_t a_hParser,
		const muChar_t* a_szName,
		muFun1_t a_pOprt,
		muBool_t a_bOptimize);

	// signature changed to fix #125 (https://github.com/beltoforion/muparser/issues/125)
	API_EXPORT(void) mupDefineInfixOprt(
		muParserHandle_t a_hParser, 
		const muChar_t* a_szName, 
		muFun1_t a_pOprt, 
		int a_iPrec, 
		muBool_t a_bAllowOpt);

	// Define character sets for identifiers
	API_EXPORT(void) mupDefineNameChars(muParserHandle_t a_hParser, const muChar_t* a_szCharset);
	API_EXPORT(void) mupDefineOprtChars(muParserHandle_t a_hParser, const muChar_t* a_szCharset);
	API_EXPORT(void) mupDefineInfixOprtChars(muParserHandle_t a_hParser, const muChar_t* a_szCharset);

	// Remove all / single variables
	API_EXPORT(void) mupRemoveVar(muParserHandle_t a_hParser, const muChar_t* a_szName);
	API_EXPORT(void) mupClearVar(muParserHandle_t a_hParser);
	API_EXPORT(void) mupClearConst(muParserHandle_t a_hParser);
	API_EXPORT(void) mupClearOprt(muParserHandle_t a_hParser);
	API_EXPORT(void) mupClearFun(muParserHandle_t a_hParser);

	// Querying variables / expression variables / constants
	API_EXPORT(int) mupGetExprVarNum(muParserHandle_t a_hParser);
	API_EXPORT(int) mupGetVarNum(muParserHandle_t a_hParser);
	API_EXPORT(int) mupGetConstNum(muParserHandle_t a_hParser);
	API_EXPORT(void) mupGetExprVar(muParserHandle_t a_hParser, unsigned a_iVar, const muChar_t** a_pszName, muFloat_t** a_pVar);
	API_EXPORT(void) mupGetVar(muParserHandle_t a_hParser, unsigned a_iVar, const muChar_t** a_pszName, muFloat_t** a_pVar);
	API_EXPORT(void) mupGetConst(muParserHandle_t a_hParser, unsigned a_iVar, const muChar_t** a_pszName, muFloat_t* a_pVar);
	API_EXPORT(void) mupSetArgSep(muParserHandle_t a_hParser, const muChar_t cArgSep);
	API_EXPORT(void) mupSetDecSep(muParserHandle_t a_hParser, const muChar_t cArgSep);
	API_EXPORT(void) mupSetThousandsSep(muParserHandle_t a_hParser, const muChar_t cArgSep);
	API_EXPORT(void) mupResetLocale(muParserHandle_t a_hParser);

	// Add value recognition callbacks
	API_EXPORT(void) mupAddValIdent(muParserHandle_t a_hParser, muIdentFun_t);

	// Error handling
	API_EXPORT(muBool_t) mupError(muParserHandle_t a_hParser);
	API_EXPORT(void) mupErrorReset(muParserHandle_t a_hParser);
	API_EXPORT(void) mupSetErrorHandler(muParserHandle_t a_hParser, muErrorHandler_t a_pErrHandler);
	API_EXPORT(const muChar_t*) mupGetErrorMsg(muParserHandle_t a_hParser);
	API_EXPORT(muInt_t) mupGetErrorCode(muParserHandle_t a_hParser);
	API_EXPORT(muInt_t) mupGetErrorPos(muParserHandle_t a_hParser);
	API_EXPORT(const muChar_t*) mupGetErrorToken(muParserHandle_t a_hParser);
	//API_EXPORT(const muChar_t*) mupGetErrorExpr(muParserHandle_t a_hParser);

	// This is used for .NET only. It creates a new variable allowing the dll to
	// manage the variable rather than the .NET garbage collector.
	API_EXPORT(muFloat_t*) mupCreateVar(void);
	API_EXPORT(void) mupReleaseVar(muFloat_t*);

#ifdef __cplusplus
}
#endif

#endif // include guard
