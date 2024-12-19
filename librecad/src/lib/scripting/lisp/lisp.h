#ifndef LISP_H
#define LISP_H

#include "lisp_version.h"
#include "qg_lsp_commandedit.h"
#include <LCL.h>

#include <stdio.h>
#include <streambuf>
#include <string>
#include <ostream>
#include <iostream>
#include <sstream>

#ifdef DEVELOPER

extern class QG_Lsp_CommandEdit *Lisp_CommandEdit;

const char *Lisp_GetVersion();

int Lisp_Initialize(int argc=0, char* argv[]=NULL);

int LispRun_SimpleFile(const char *filename);

int LispRun_SimpleString(const char *command);

std::string Lisp_EvalString(const String& input);

std::string Lisp_EvalFile(const char *filename);

#endif // DEVELOPER

#endif // LISP_H
