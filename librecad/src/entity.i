%module librecad;

%rename(__aref__)             *::operator[];
%rename(__lshift__)           *::operator<<;
%rename(__rshift__)           *::operator>>;

%{
#include "rs_vector.h"
#include "rs_pythondcl.h"
#include "rs_pythonlisp.h"
#include "rs_pythongui.h"
%}

%include "lib/engine/rs_vector.h"
%include "lib/scripting/rs_pythondcl.h"
%include "lib/scripting/rs_pythongui.h"
%include "lib/scripting/rs_pythonlisp.h"
