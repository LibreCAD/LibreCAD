%module librecad;

%rename(__aref__)             *::operator[];
%rename(__lshift__)           *::operator<<;
%rename(__rshift__)           *::operator>>;

%{
#include <string>
#include "rs_vector.h"
#include "rs_pythoncore.h"
#include "rs_pythondcl.h"
#include "rs_pythonlisp.h"
#include "rs_pythongui.h"
%}

%include "lib/engine/rs_vector.h"
%include <std_string.i>
%include "lib/scripting/python/rs_pythoncore.h"
%include "lib/scripting/python/rs_pythondcl.h"
%include "lib/scripting/python/rs_pythongui.h"
%include "lib/scripting/python/rs_pythonlisp.h"
