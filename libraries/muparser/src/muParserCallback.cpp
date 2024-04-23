/*

	 _____  __ _____________ _______  ______ ___________
	/     \|  |  \____ \__  \\_  __ \/  ___// __ \_  __ \
   |  Y Y  \  |  /  |_> > __ \|  | \/\___ \\  ___/|  | \/
   |__|_|  /____/|   __(____  /__|  /____  >\___  >__|
		 \/      |__|       \/           \/     \/
   Copyright (C) 2022 Ingo Berg

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

#include "muParserCallback.h"

#if defined(_MSC_VER)
	#pragma warning(push)
	#pragma warning(disable : 26812) 
#endif

/** \file
	\brief Implementation of the parser callback class.
*/


namespace mu
{
	static constexpr int CALLBACK_INTERNAL_VAR_ARGS         = 1 << 14;
	static constexpr int CALLBACK_INTERNAL_FIXED_ARGS_MASK  = 0xf;
	static constexpr int CALLBACK_INTERNAL_WITH_USER_DATA	= 1 << 13;

	struct CbWithUserData
	{
		void*	pFun;
		void* 	pUserData;
	};


	ParserCallback::ParserCallback(fun_type0 a_pFun, bool a_bAllowOpti)
		:m_pFun((void*)a_pFun)
		, m_iArgc(0)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(fun_type1 a_pFun, bool a_bAllowOpti, int a_iPrec, ECmdCode a_iCode)
		:m_pFun((void*)a_pFun)
		, m_iArgc(1)
		, m_iPri(a_iPrec)
		, m_eOprtAsct(oaNONE)
		, m_iCode(a_iCode)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(fun_type1 a_pFun, bool a_bAllowOpti)
		: ParserCallback(a_pFun, a_bAllowOpti, -1, cmFUNC)
	{}

	
	/** \brief Constructor for constructing function callbacks taking two arguments.
		\throw nothrow
	*/
	ParserCallback::ParserCallback(fun_type2 a_pFun, bool a_bAllowOpti)
		:m_pFun((void*)a_pFun)
		, m_iArgc(2)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	/** \brief Constructor for constructing binary operator callbacks.
		\param a_pFun Pointer to a static function taking two arguments
		\param a_bAllowOpti A flag indicating this function can be optimized
		\param a_iPrec The operator precedence
		\param a_eOprtAsct The operators associativity
		\throw nothrow
	*/
	ParserCallback::ParserCallback(fun_type2 a_pFun,
		bool a_bAllowOpti,
		int a_iPrec,
		EOprtAssociativity a_eOprtAsct)
		:m_pFun((void*)a_pFun)
		, m_iArgc(2)
		, m_iPri(a_iPrec)
		, m_eOprtAsct(a_eOprtAsct)
		, m_iCode(cmOPRT_BIN)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(fun_type3 a_pFun, bool a_bAllowOpti)
		:m_pFun((void*)a_pFun)
		, m_iArgc(3)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(fun_type4 a_pFun, bool a_bAllowOpti)
		:m_pFun((void*)a_pFun)
		, m_iArgc(4)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(fun_type5 a_pFun, bool a_bAllowOpti)
		:m_pFun((void*)a_pFun)
		, m_iArgc(5)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(fun_type6 a_pFun, bool a_bAllowOpti)
		:m_pFun((void*)a_pFun)
		, m_iArgc(6)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(fun_type7 a_pFun, bool a_bAllowOpti)
		:m_pFun((void*)a_pFun)
		, m_iArgc(7)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(fun_type8 a_pFun, bool a_bAllowOpti)
		:m_pFun((void*)a_pFun)
		, m_iArgc(8)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(fun_type9 a_pFun, bool a_bAllowOpti)
		:m_pFun((void*)a_pFun)
		, m_iArgc(9)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(fun_type10 a_pFun, bool a_bAllowOpti)
		:m_pFun((void*)a_pFun)
		, m_iArgc(10)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(fun_userdata_type0 a_pFun, void* a_pUserData, bool a_bAllowOpti)
		:m_pFun(new CbWithUserData{reinterpret_cast<void*>(a_pFun), a_pUserData})
		, m_iArgc(0 | CALLBACK_INTERNAL_WITH_USER_DATA)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(fun_userdata_type1 a_pFun, void* a_pUserData, bool a_bAllowOpti)
		:m_pFun(new CbWithUserData{reinterpret_cast<void*>(a_pFun), a_pUserData})
		, m_iArgc(1 | CALLBACK_INTERNAL_WITH_USER_DATA)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(fun_userdata_type2 a_pFun, void* a_pUserData, bool a_bAllowOpti)
		:m_pFun(new CbWithUserData{reinterpret_cast<void*>(a_pFun), a_pUserData})
		, m_iArgc(2 | CALLBACK_INTERNAL_WITH_USER_DATA)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(fun_userdata_type3 a_pFun, void* a_pUserData, bool a_bAllowOpti)
		:m_pFun(new CbWithUserData{reinterpret_cast<void*>(a_pFun), a_pUserData})
		, m_iArgc(3 | CALLBACK_INTERNAL_WITH_USER_DATA)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(fun_userdata_type4 a_pFun, void* a_pUserData, bool a_bAllowOpti)
		:m_pFun(new CbWithUserData{reinterpret_cast<void*>(a_pFun), a_pUserData})
		, m_iArgc(4 | CALLBACK_INTERNAL_WITH_USER_DATA)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(fun_userdata_type5 a_pFun, void* a_pUserData, bool a_bAllowOpti)
		:m_pFun(new CbWithUserData{reinterpret_cast<void*>(a_pFun), a_pUserData})
		, m_iArgc(5 | CALLBACK_INTERNAL_WITH_USER_DATA)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(fun_userdata_type6 a_pFun, void* a_pUserData, bool a_bAllowOpti)
		:m_pFun(new CbWithUserData{reinterpret_cast<void*>(a_pFun), a_pUserData})
		, m_iArgc(6 | CALLBACK_INTERNAL_WITH_USER_DATA)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(fun_userdata_type7 a_pFun, void* a_pUserData, bool a_bAllowOpti)
		:m_pFun(new CbWithUserData{reinterpret_cast<void*>(a_pFun), a_pUserData})
		, m_iArgc(7 | CALLBACK_INTERNAL_WITH_USER_DATA)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(fun_userdata_type8 a_pFun, void* a_pUserData, bool a_bAllowOpti)
		:m_pFun(new CbWithUserData{reinterpret_cast<void*>(a_pFun), a_pUserData})
		, m_iArgc(8 | CALLBACK_INTERNAL_WITH_USER_DATA)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(fun_userdata_type9 a_pFun, void* a_pUserData, bool a_bAllowOpti)
		:m_pFun(new CbWithUserData{reinterpret_cast<void*>(a_pFun), a_pUserData})
		, m_iArgc(9 | CALLBACK_INTERNAL_WITH_USER_DATA)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(fun_userdata_type10 a_pFun, void* a_pUserData, bool a_bAllowOpti)
		:m_pFun(new CbWithUserData{reinterpret_cast<void*>(a_pFun), a_pUserData})
		, m_iArgc(10 | CALLBACK_INTERNAL_WITH_USER_DATA)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(bulkfun_type0 a_pFun, bool a_bAllowOpti)
		:m_pFun((void*)a_pFun)
		, m_iArgc(0)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_BULK)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(bulkfun_type1 a_pFun, bool a_bAllowOpti)
		:m_pFun((void*)a_pFun)
		, m_iArgc(1)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_BULK)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	/** \brief Constructor for constructing function callbacks taking two arguments.
		\throw nothrow
	*/
	ParserCallback::ParserCallback(bulkfun_type2 a_pFun, bool a_bAllowOpti)
		:m_pFun((void*)a_pFun)
		, m_iArgc(2)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_BULK)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(bulkfun_type3 a_pFun, bool a_bAllowOpti)
		:m_pFun((void*)a_pFun)
		, m_iArgc(3)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_BULK)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(bulkfun_type4 a_pFun, bool a_bAllowOpti)
		:m_pFun((void*)a_pFun)
		, m_iArgc(4)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_BULK)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(bulkfun_type5 a_pFun, bool a_bAllowOpti)
		:m_pFun((void*)a_pFun)
		, m_iArgc(5)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_BULK)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(bulkfun_type6 a_pFun, bool a_bAllowOpti)
		:m_pFun((void*)a_pFun)
		, m_iArgc(6)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_BULK)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(bulkfun_type7 a_pFun, bool a_bAllowOpti)
		:m_pFun((void*)a_pFun)
		, m_iArgc(7)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_BULK)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(bulkfun_type8 a_pFun, bool a_bAllowOpti)
		:m_pFun((void*)a_pFun)
		, m_iArgc(8)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_BULK)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(bulkfun_type9 a_pFun, bool a_bAllowOpti)
		:m_pFun((void*)a_pFun)
		, m_iArgc(9)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_BULK)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(bulkfun_type10 a_pFun, bool a_bAllowOpti)
		:m_pFun((void*)a_pFun)
		, m_iArgc(10)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_BULK)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(bulkfun_userdata_type0 a_pFun, void* a_pUserData, bool a_bAllowOpti)
		:m_pFun(new CbWithUserData{reinterpret_cast<void*>(a_pFun), a_pUserData})
		, m_iArgc(0 | CALLBACK_INTERNAL_WITH_USER_DATA)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_BULK)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(bulkfun_userdata_type1 a_pFun, void* a_pUserData, bool a_bAllowOpti)
		:m_pFun(new CbWithUserData{reinterpret_cast<void*>(a_pFun), a_pUserData})
		, m_iArgc(1 | CALLBACK_INTERNAL_WITH_USER_DATA)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_BULK)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(bulkfun_userdata_type2 a_pFun, void* a_pUserData, bool a_bAllowOpti)
		:m_pFun(new CbWithUserData{reinterpret_cast<void*>(a_pFun), a_pUserData})
		, m_iArgc(2 | CALLBACK_INTERNAL_WITH_USER_DATA)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_BULK)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(bulkfun_userdata_type3 a_pFun, void* a_pUserData, bool a_bAllowOpti)
		:m_pFun(new CbWithUserData{reinterpret_cast<void*>(a_pFun), a_pUserData})
		, m_iArgc(3 | CALLBACK_INTERNAL_WITH_USER_DATA)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_BULK)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(bulkfun_userdata_type4 a_pFun, void* a_pUserData, bool a_bAllowOpti)
		:m_pFun(new CbWithUserData{reinterpret_cast<void*>(a_pFun), a_pUserData})
		, m_iArgc(4 | CALLBACK_INTERNAL_WITH_USER_DATA)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_BULK)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(bulkfun_userdata_type5 a_pFun, void* a_pUserData, bool a_bAllowOpti)
		:m_pFun(new CbWithUserData{reinterpret_cast<void*>(a_pFun), a_pUserData})
		, m_iArgc(5 | CALLBACK_INTERNAL_WITH_USER_DATA)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_BULK)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(bulkfun_userdata_type6 a_pFun, void* a_pUserData, bool a_bAllowOpti)
		:m_pFun(new CbWithUserData{reinterpret_cast<void*>(a_pFun), a_pUserData})
		, m_iArgc(6 | CALLBACK_INTERNAL_WITH_USER_DATA)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_BULK)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(bulkfun_userdata_type7 a_pFun, void* a_pUserData, bool a_bAllowOpti)
		:m_pFun(new CbWithUserData{reinterpret_cast<void*>(a_pFun), a_pUserData})
		, m_iArgc(7 | CALLBACK_INTERNAL_WITH_USER_DATA)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_BULK)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(bulkfun_userdata_type8 a_pFun, void* a_pUserData, bool a_bAllowOpti)
		:m_pFun(new CbWithUserData{reinterpret_cast<void*>(a_pFun), a_pUserData})
		, m_iArgc(8 | CALLBACK_INTERNAL_WITH_USER_DATA)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_BULK)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(bulkfun_userdata_type9 a_pFun, void* a_pUserData, bool a_bAllowOpti)
		:m_pFun(new CbWithUserData{reinterpret_cast<void*>(a_pFun), a_pUserData})
		, m_iArgc(9 | CALLBACK_INTERNAL_WITH_USER_DATA)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_BULK)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(bulkfun_userdata_type10 a_pFun, void* a_pUserData, bool a_bAllowOpti)
		:m_pFun(new CbWithUserData{reinterpret_cast<void*>(a_pFun), a_pUserData})
		, m_iArgc(10 | CALLBACK_INTERNAL_WITH_USER_DATA)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_BULK)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(multfun_type a_pFun, bool a_bAllowOpti)
		:m_pFun((void*)a_pFun)
		, m_iArgc(CALLBACK_INTERNAL_VAR_ARGS)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(multfun_userdata_type a_pFun, void* a_pUserData, bool a_bAllowOpti)
		:m_pFun(new CbWithUserData{reinterpret_cast<void*>(a_pFun), a_pUserData})
		, m_iArgc(CALLBACK_INTERNAL_VAR_ARGS | CALLBACK_INTERNAL_WITH_USER_DATA)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC)
		, m_iType(tpDBL)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(strfun_type1 a_pFun, bool a_bAllowOpti)
		:m_pFun((void*)a_pFun)
		, m_iArgc(0)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_STR)
		, m_iType(tpSTR)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(strfun_type2 a_pFun, bool a_bAllowOpti)
		:m_pFun((void*)a_pFun)
		, m_iArgc(1)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_STR)
		, m_iType(tpSTR)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(strfun_type3 a_pFun, bool a_bAllowOpti)
		:m_pFun((void*)a_pFun)
		, m_iArgc(2)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_STR)
		, m_iType(tpSTR)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(strfun_type4 a_pFun, bool a_bAllowOpti)
		:m_pFun((void*)a_pFun)
		, m_iArgc(3)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_STR)
		, m_iType(tpSTR)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(strfun_type5 a_pFun, bool a_bAllowOpti)
		:m_pFun((void*)a_pFun)
		, m_iArgc(4)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_STR)
		, m_iType(tpSTR)
		, m_bAllowOpti(a_bAllowOpti)
	{}

	ParserCallback::ParserCallback(strfun_type6 a_pFun, bool a_bAllowOpti)
		:m_pFun((void*)a_pFun)
		, m_iArgc(5)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_STR)
		, m_iType(tpSTR)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(strfun_userdata_type1 a_pFun, void* a_pUserData, bool a_bAllowOpti)
		:m_pFun(new CbWithUserData{reinterpret_cast<void*>(a_pFun), a_pUserData})
		, m_iArgc(0 | CALLBACK_INTERNAL_WITH_USER_DATA)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_STR)
		, m_iType(tpSTR)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(strfun_userdata_type2 a_pFun, void* a_pUserData, bool a_bAllowOpti)
		:m_pFun(new CbWithUserData{reinterpret_cast<void*>(a_pFun), a_pUserData})
		, m_iArgc(1 | CALLBACK_INTERNAL_WITH_USER_DATA)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_STR)
		, m_iType(tpSTR)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(strfun_userdata_type3 a_pFun, void* a_pUserData, bool a_bAllowOpti)
		:m_pFun(new CbWithUserData{reinterpret_cast<void*>(a_pFun), a_pUserData})
		, m_iArgc(2 | CALLBACK_INTERNAL_WITH_USER_DATA)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_STR)
		, m_iType(tpSTR)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(strfun_userdata_type4 a_pFun, void* a_pUserData, bool a_bAllowOpti)
		:m_pFun(new CbWithUserData{reinterpret_cast<void*>(a_pFun), a_pUserData})
		, m_iArgc(3 | CALLBACK_INTERNAL_WITH_USER_DATA)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_STR)
		, m_iType(tpSTR)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(strfun_userdata_type5 a_pFun, void* a_pUserData, bool a_bAllowOpti)
		:m_pFun(new CbWithUserData{reinterpret_cast<void*>(a_pFun), a_pUserData})
		, m_iArgc(4 | CALLBACK_INTERNAL_WITH_USER_DATA)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_STR)
		, m_iType(tpSTR)
		, m_bAllowOpti(a_bAllowOpti)
	{}


	ParserCallback::ParserCallback(strfun_userdata_type6 a_pFun, void* a_pUserData, bool a_bAllowOpti)
		:m_pFun(new CbWithUserData{ reinterpret_cast<void*>(a_pFun), a_pUserData })
		, m_iArgc(5 | CALLBACK_INTERNAL_WITH_USER_DATA)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmFUNC_STR)
		, m_iType(tpSTR)
		, m_bAllowOpti(a_bAllowOpti)
	{}

	/** \brief Default constructor.
		\throw nothrow
	*/
	ParserCallback::ParserCallback()
		:m_pFun(0)
		, m_iArgc(0)
		, m_iPri(-1)
		, m_eOprtAsct(oaNONE)
		, m_iCode(cmUNKNOWN)
		, m_iType(tpVOID)
		, m_bAllowOpti(0)
	{}


	/** \brief Copy constructor.
		\throw nothrow
	*/
	ParserCallback::ParserCallback(const ParserCallback& ref)
		:ParserCallback()
	{
		Assign(ref);
	}

	ParserCallback & ParserCallback::operator=(const ParserCallback & ref)
	{
		Assign(ref);
		return *this;
	}


	ParserCallback::~ParserCallback()
	{
		if (m_iArgc & CALLBACK_INTERNAL_WITH_USER_DATA)
			delete reinterpret_cast<CbWithUserData*>(m_pFun);
	}


	/** \brief Copy callback from argument.

		\throw nothrow
	*/
	void ParserCallback::Assign(const ParserCallback& ref)
	{
		if (this == &ref)
			return;

		if (m_iArgc & CALLBACK_INTERNAL_WITH_USER_DATA) {
			delete reinterpret_cast<CbWithUserData*>(m_pFun);
			m_pFun = nullptr;
		}

		if (ref.m_iArgc & CALLBACK_INTERNAL_WITH_USER_DATA)
			m_pFun = new CbWithUserData(*reinterpret_cast<CbWithUserData*>(ref.m_pFun));
		else
			m_pFun = ref.m_pFun;
		m_iArgc = ref.m_iArgc;
		m_bAllowOpti = ref.m_bAllowOpti;
		m_iCode = ref.m_iCode;
		m_iType = ref.m_iType;
		m_iPri = ref.m_iPri;
		m_eOprtAsct = ref.m_eOprtAsct;
	}


	/** \brief Clone this instance and return a pointer to the new instance. */
	ParserCallback* ParserCallback::Clone() const
	{
		return new ParserCallback(*this);
	}


	/** \brief Return tru if the function is conservative.

		Conservative functions return always the same result for the same argument.
		\throw nothrow
	*/
	bool ParserCallback::IsOptimizable() const
	{
		return m_bAllowOpti;
	}


	/** \brief Get the callback address for the parser function.

		The type of the address is void. It needs to be recasted according to the
		argument number to the right type.

		\throw nothrow
	*/
	void* ParserCallback::GetAddr() const
	{
		if (m_iArgc & CALLBACK_INTERNAL_WITH_USER_DATA)
			return reinterpret_cast<CbWithUserData*>(m_pFun)->pFun;
		else
			return m_pFun;
	}


	/** \brief Get the user data if present, else nullptr

		\throw nothrow
	*/
	void* ParserCallback::GetUserData() const
	{
		if (m_iArgc & CALLBACK_INTERNAL_WITH_USER_DATA)
			return reinterpret_cast<CbWithUserData*>(m_pFun)->pUserData;
		else
			return nullptr;
	}


	/** \brief Check that the callback looks valid
		\throw nothrow

		Check that the function pointer is not null,
		and if there are user data that they are not null.
	*/
	bool ParserCallback::IsValid() const
	{
		return GetAddr() != nullptr
			&& !((m_iArgc & CALLBACK_INTERNAL_WITH_USER_DATA)
			     && GetUserData() == nullptr);
	}


	/** \brief Return the callback code. */
	ECmdCode  ParserCallback::GetCode() const
	{
		return m_iCode;
	}


	ETypeCode ParserCallback::GetType() const
	{
		return m_iType;
	}


	/** \brief Return the operator precedence.
		\throw nothrown

	   Only valid if the callback token is an operator token (binary or infix).
	*/
	int ParserCallback::GetPri()  const
	{
		return m_iPri;
	}


	/** \brief Return the operators associativity.
		\throw nothrown

	   Only valid if the callback token is a binary operator token.
	*/
	EOprtAssociativity ParserCallback::GetAssociativity() const
	{
		return m_eOprtAsct;
	}


	/** \brief Returns the number of numeric function Arguments.

	   This number is negative for functions with variable number of arguments.
	*/
	int ParserCallback::GetArgc() const
	{
		return (m_iArgc & CALLBACK_INTERNAL_VAR_ARGS) ? -1 : (m_iArgc & CALLBACK_INTERNAL_FIXED_ARGS_MASK);
	}
} // namespace mu

#if defined(_MSC_VER)
	#pragma warning(pop)
#endif
