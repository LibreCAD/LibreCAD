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

#ifndef MU_PARSER_BYTECODE_H
#define MU_PARSER_BYTECODE_H

#include <string>
#include <stack>
#include <vector>

#include "muParserDef.h"
#include "muParserError.h"
#include "muParserToken.h"

/** \file
	\brief Definition of the parser bytecode class.
*/


namespace mu
{
	struct SToken
	{
		ECmdCode Cmd;

		union
		{
			struct // SValData
			{
				value_type* ptr;
				value_type  data;
				value_type  data2;
			} Val;

			struct // SFunData
			{
				// Note: the type is erased in generic_callable_type and the signature of the
				//       function to call is tracked elsewhere in regard with the number of
				//       parameters (args) and the general kind of function (Cmd: cmFUNC,
				//       cmFUNC_STR, or cmFUNC_BULK)
				generic_callable_type cb;
				int   argc;
				int   idx;
			} Fun;

			struct // SOprtData
			{
				value_type* ptr;
				int offset;
			} Oprt;
		};
	};


	/** \brief Bytecode implementation of the Math Parser.

		The bytecode contains the formula converted to revers polish notation stored in a continious
		memory area. Associated with this data are operator codes, variable pointers, constant
		values and function pointers. Those are necessary in order to calculate the result.
		All those data items will be casted to the underlying datatype of the bytecode.
	*/
	class API_EXPORT_CXX ParserByteCode final
	{
	private:

		/** \brief Token type for internal use only. */
		typedef ParserToken<value_type, string_type> token_type;

		/** \brief Token vector for storing the RPN. */
		typedef std::vector<SToken> rpn_type;

		/** \brief Type for a vector of strings. */
		typedef std::vector<string_type> stringbuf_type;

		/** \brief Position in the Calculation array. */
		unsigned m_iStackPos;

		/** \brief String variable storage. */
		stringbuf_type m_stringBuffer;

		/** \brief The expression associated with this bytecode. */
		string_type m_expr;

		/** \brief Maximum size needed for the stack. */
		std::size_t m_iMaxStackSize;

		/** \brief The actual rpn storage. */
		rpn_type  m_vRPN;

		bool m_bEnableOptimizer;

		void ConstantFolding(ECmdCode a_Oprt);

	public:

		ParserByteCode();
		ParserByteCode(const ParserByteCode& a_ByteCode);
		ParserByteCode& operator=(const ParserByteCode& a_ByteCode);
		void Assign(const ParserByteCode& a_ByteCode);

		void AddVar(value_type* a_pVar);
		void AddVal(value_type a_fVal);
		void AddOp(ECmdCode a_Oprt);
		void AddIfElse(ECmdCode a_Oprt);
		void AddAssignOp(value_type* a_pVar);
		void AddFun(generic_callable_type a_pFun, int a_iArgc, bool isOptimizable);
		void AddBulkFun(generic_callable_type a_pFun, int a_iArgc);
		void AddStrFun(generic_callable_type a_pFun, int a_iArgc, int a_iIdx);

		void EnableOptimizer(bool bStat);

		void Finalize();
		void clear();
		std::size_t GetMaxStackSize() const;

		std::size_t GetSize() const
		{
			return m_vRPN.size();
		}

		inline const SToken* GetBase() const
		{
			if (m_vRPN.size() == 0)
				throw ParserError(ecINTERNAL_ERROR);
			else
				return &m_vRPN[0];
		}

		void StoreEnvironment(string_type expr, stringbuf_type const& strBuf)
		{
			m_stringBuffer = strBuf;
			m_expr = expr;
		}

		std::tuple<string_type, stringbuf_type> RestoreEnvironment() const
		{
			return std::make_tuple(m_expr, m_stringBuffer);
		}

		void AsciiDump() const;
	};

} // namespace mu

#endif


