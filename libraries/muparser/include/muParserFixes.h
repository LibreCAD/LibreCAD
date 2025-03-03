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

#ifndef MU_PARSER_FIXES_H
#define MU_PARSER_FIXES_H

/** \file
	\brief This file contains compatibility fixes for some platforms.
*/

//
// Compatibility fixes
//

/* From http://gcc.gnu.org/wiki/Visibility */
/* Generic helper definitions for shared library support */
#if defined _WIN32 || defined __CYGWIN__
	#define MUPARSER_HELPER_DLL_IMPORT __declspec(dllimport)
	#define MUPARSER_HELPER_DLL_EXPORT __declspec(dllexport)
	#define MUPARSER_HELPER_DLL_LOCAL
#else
	#if __GNUC__ >= 4
		#define MUPARSER_HELPER_DLL_IMPORT __attribute__ ((visibility ("default")))
		#define MUPARSER_HELPER_DLL_EXPORT __attribute__ ((visibility ("default")))
		#define MUPARSER_HELPER_DLL_LOCAL  __attribute__ ((visibility ("hidden")))
	#else
		#define MUPARSER_HELPER_DLL_IMPORT
		#define MUPARSER_HELPER_DLL_EXPORT
		#define MUPARSER_HELPER_DLL_LOCAL
	#endif
#endif

/* 
	Now we use the generic helper definitions above to define API_EXPORT_CXX and MUPARSER_LOCAL.
	API_EXPORT_CXX is used for the public API symbols. It either DLL imports or DLL exports (or does nothing for static build)
	MUPARSER_LOCAL is used for non-api symbols.
*/

#ifndef MUPARSER_STATIC /* defined if muParser is compiled as a DLL */

	#ifdef MUPARSERLIB_EXPORTS /* defined if we are building the muParser DLL (instead of using it) */
		#define API_EXPORT_CXX MUPARSER_HELPER_DLL_EXPORT
	#else
		#define API_EXPORT_CXX MUPARSER_HELPER_DLL_IMPORT
	#endif /* MUPARSER_DLL_EXPORTS */
	#define MUPARSER_LOCAL MUPARSER_HELPER_DLL_LOCAL

#else /* MUPARSER_STATIC is defined: this means muParser is a static lib. */

	#define API_EXPORT_CXX
	#define MUPARSER_LOCAL

#endif /* !MUPARSER_STATIC */


#ifdef _WIN32
	#define API_EXPORT(TYPE) API_EXPORT_CXX TYPE __cdecl
#else
	#define API_EXPORT(TYPE) TYPE
#endif


#endif // include guard


