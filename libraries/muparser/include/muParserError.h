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

#ifndef MU_PARSER_ERROR_H
#define MU_PARSER_ERROR_H

#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>
#include <memory>

#include "muParserDef.h"

#if defined(_MSC_VER)
	#pragma warning(push)
	#pragma warning(disable : 4251)  // ...needs to have dll-interface to be used by clients of class ...
#endif


/** \file
	\brief This file defines the error class used by the parser.
*/

namespace mu
{
	/** \brief A class that handles the error messages.	*/
	class ParserErrorMsg final
	{
	public:
		static const ParserErrorMsg& Instance();
		string_type operator[](unsigned a_iIdx) const;

	private:
		ParserErrorMsg& operator=(const ParserErrorMsg&) = delete;
		ParserErrorMsg(const ParserErrorMsg&) = delete;
		ParserErrorMsg();

		~ParserErrorMsg() = default;

		std::vector<string_type>  m_vErrMsg;  ///< A vector with the predefined error messages
	};


	/** \brief Error class of the parser.

	  Part of the math parser package.
	*/
	class API_EXPORT_CXX ParserError
	{
	private:

		/** \brief Replace all ocuurences of a substring with another string. */
		void ReplaceSubString(string_type& strSource, const string_type& strFind, const string_type& strReplaceWith);
		void Reset();

	public:

		ParserError();
		explicit ParserError(EErrorCodes a_iErrc);
		explicit ParserError(const string_type& sMsg);
		ParserError(EErrorCodes a_iErrc, const string_type& sTok, const string_type& sFormula = string_type(), int a_iPos = -1);
		ParserError(EErrorCodes a_iErrc, int a_iPos, const string_type& sTok);
		ParserError(const char_type* a_szMsg, int a_iPos = -1, const string_type& sTok = string_type());
		ParserError(const ParserError& a_Obj);

		ParserError& operator=(const ParserError& a_Obj);
		~ParserError();

		void SetFormula(const string_type& a_strFormula);
		const string_type& GetExpr() const;
		const string_type& GetMsg() const;
		int GetPos() const;
		const string_type& GetToken() const;
		EErrorCodes GetCode() const;

	private:
		string_type m_strMsg;     ///< The message string
		string_type m_strFormula; ///< Formula string
		string_type m_strTok;     ///< Token related with the error
		int m_iPos;               ///< Formula position related to the error 
		EErrorCodes m_iErrc;      ///< Error code
		const ParserErrorMsg& m_ErrMsg;
	};
} // namespace mu

#if defined(_MSC_VER)
	#pragma warning(pop)
#endif

#endif

