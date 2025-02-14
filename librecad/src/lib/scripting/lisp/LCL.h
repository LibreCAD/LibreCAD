#ifndef INCLUDE_LCL_H
#define INCLUDE_LCL_H

#ifdef DEVELOPER

#include "Debug.h"
#include "RefCountedPtr.h"
#include "String.h"
#include "Validation.h"
#include <sstream>

#include <vector>

class lclValue;
typedef RefCountedPtr<lclValue>  lclValuePtr;
typedef std::vector<lclValuePtr> lclValueVec;
typedef lclValueVec::iterator    lclValueIter;

class lclEnv;
typedef RefCountedPtr<lclEnv>    lclEnvPtr;

// lisp.cpp
extern lclValuePtr APPLY(lclValuePtr op,
                         lclValueIter argsBegin, lclValueIter argsEnd);
extern lclValuePtr EVAL(lclValuePtr ast, lclEnvPtr env);

extern String rep(const String& input, lclEnvPtr env);

// Core.cpp
extern void installCore(lclEnvPtr env);
extern String noQuotes(const String& s);

// Reader.cpp

typedef struct LclAlias {
    String alias = "", command = "";
} LclAlias_t;

extern std::vector<LclAlias_t> LclCom;
extern bool isAlias(const String& alias);
extern lclValuePtr readStr(const String& input);
extern lclValuePtr loadDcl(const String& path);

#endif // DEVELOPER

#endif // INCLUDE_LCL_H
