//#pragma once

#ifndef RS_PYTHONLISP_H
#define RS_PYTHONLISP_H

//#ifdef DEVELOPER

class RS_PythonLisp
{
public:
    RS_PythonLisp() {}
    ~RS_PythonLisp() {}

    int RunSimpleString(const char *cmd);
    int RunSimpleFile(const char *filename);

    const char *EvalSimpleString(const char *cmd);
    const char *EvalSimpleFile(const char *filename);
};

//#endif // DEVELOPER

#endif // RS_PYTHONLISP_H
