/***************************************************************************\
|* Function Parser for C++ v4.3                                            *|
|*-------------------------------------------------------------------------*|
|* Copyright: Juha Nieminen                                                *|
\***************************************************************************/

#ifndef ONCE_FPARSER_GMPINT_H_
#define ONCE_FPARSER_GMPINT_H_

#include "fparser.hh"
#include "mpfr/GmpInt.hh"

class FunctionParser_gmpint: public FunctionParserBase<GmpInt> {};

#endif
