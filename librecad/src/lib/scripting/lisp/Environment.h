#ifndef INCLUDE_ENVIRONMENT_H
#define INCLUDE_ENVIRONMENT_H

#ifdef DEVELOPER

#include "LCL.h"

#include <map>

class lclEnv : public RefCounted {
public:
    lclEnv(lclEnvPtr outer = NULL);
    lclEnv(lclEnvPtr outer,
           const StringVec& bindings,
           lclValueIter argsBegin,
           lclValueIter argsEnd);

    ~lclEnv();

    void setLamdaMode(bool mode) { m_isLamda = mode; }
    bool isLamda() const { return m_isLamda; }

    lclValuePtr get(const String& symbol);
    lclEnvPtr   find(const String& symbol);
    lclValuePtr set(const String& symbol, lclValuePtr value);
    lclEnvPtr   getRoot();

private:
    typedef std::map<String, lclValuePtr> Map;
    Map m_map;
    lclEnvPtr m_outer;
    StringVec m_bindings;
    bool m_isLamda = false;
};

extern lclEnvPtr replEnv;

extern lclEnvPtr shadowEnv;

extern lclEnvPtr dclEnv;

#endif // DEVELOPER

#endif // INCLUDE_ENVIRONMENT_H
