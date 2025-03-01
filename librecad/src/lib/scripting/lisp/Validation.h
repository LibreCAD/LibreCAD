#ifndef INCLUDE_VALIDATION_H
#define INCLUDE_VALIDATION_H

#ifdef DEVELOPER

#include "lisp.h"
#include "lstring.h"
#include <iostream>

//if (!(condition)) { try { throw STRF(__VA_ARGS__); } catch (const char* e) { std::cerr << e << std::endl; } } else { }
//if (!(condition)) { throw STRF(__VA_ARGS__); } else { }


#define LCL_CHECK(condition, ...)  \
if (!(condition)) { try { lisp_error = 1; throw STRF(__VA_ARGS__); } catch (const char* e) { std::cerr << e << std::endl; } } else { }

#define LCL_FAIL(...) LCL_CHECK(false, __VA_ARGS__)

#define LCLTYPE_ERR_STR(name, prog) \
        const lclValuePtr type = lcl::type(name->type()); \
        const String typeError = "'" + String(prog) + "': type is " + type->print(true);

#define ERROR_STR_TYPE(name, prog) \
    "'" prog "': type is " name

#define LCL_TYPE_FAIL(name, nameType) \
    LCLTYPE_ERR_STR(name, nameType) \
    LCL_FAIL(typeError.c_str());

#define CHECK_IS_NUMBER(name) \
    LCL_CONSTAND_FAIL_CHECK(name, "nil", "number?") \
    LCL_CONSTAND_FAIL_CHECK(name, "false", "number?") \
    LCL_CONSTAND_FAIL_CHECK(name, "true", "number?") \
    if (!(name->type() == LCLTYPE::INT) && !(name->type() == LCLTYPE::REAL)) { \
        LCL_TYPE_FAIL(name, "number?") \
    }

#define LCL_CONSTAND_FAIL_CHECK(name, c, nameType) LCL_CHECK(!(name->print(true).compare(c) == 0), ERROR_STR_TYPE(c, nameType));

extern int checkArgsIs(const char* name, int expected, int got);
extern int checkArgsBetween(const char* name, int min, int max, int got);
extern int checkArgsAtLeast(const char* name, int min, int got);
extern int checkArgsEven(const char* name, int got);

#endif // DEVELOPER

#endif // INCLUDE_VALIDATION_H
