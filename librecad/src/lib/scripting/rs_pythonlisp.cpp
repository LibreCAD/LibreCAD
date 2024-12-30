#include "rs_pythonlisp.h"
#include "lisp.h"

//#ifdef DEVELOPER

RS_PythonLisp::RS_PythonLisp() {}
RS_PythonLisp::~RS_PythonLisp() {}

int RS_PythonLisp::RunSimpleString(const char *cmd)
{
    return LispRun_SimpleString(cmd);
}

int RS_PythonLisp::RunSimpleFile(const char *fileName)
{
    return LispRun_SimpleFile(fileName);
}

const char *RS_PythonLisp::EvalSimpleString(const char *cmd)
{
    static std::string result = Lisp_EvalString(cmd);
    return result.c_str();
}

const char *RS_PythonLisp::EvalSimpleFile(const char *fileName)
{
    static std::string result = Lisp_EvalFile(fileName);
    return result.c_str();
}

//#endif // DEVELOPER
