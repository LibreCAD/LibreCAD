#include "rs_pythonlisp.h"
#include "lisp.h"

//#ifdef DEVELOPER

int RS_PythonLisp::RunSimpleString(const char *cmd)
{
    return LispRun_SimpleString(cmd);
}

int RS_PythonLisp::RunSimpleFile(const char *filename)
{
    return LispRun_SimpleFile(filename);
}

const char *RS_PythonLisp::EvalSimpleString(const char *cmd)
{
    static std::string result = Lisp_EvalString(cmd);
    return result.c_str();
}

const char *RS_PythonLisp::EvalSimpleFile(const char *filename)
{
    static std::string result = Lisp_EvalFile(filename);
    return result.c_str();
}

//#endif // DEVELOPER
