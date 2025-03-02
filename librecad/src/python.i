%module librecad;

%rename(__aref__)             *::operator[];
%rename(__lshift__)           *::operator<<;
%rename(__rshift__)           *::operator>>;

%{

/*
#include <QtCore/qglobal.h>
#include <QtCore/qtmetamacros.h>
#include <QtCore/qnamespace.h>
#include "qpoint.h"
#include "doc_plugin_interface.h"
*/

#include <string>
#include "rs_vector.h"
#include "rs_pythoncore.h"
#include "rs_pythongui.h"
#include "rs_pythondcl.h"
#include "rs_pythonlisp.h"

%}

/*
%include <QtCore/qglobal.h>
%include <QtCore/qtmetamacros.h>
%include <QtCore/qnamespace.h>
%include <QtCore/qpoint.h>
%include "main/doc_plugin_interface.h"
*/

%include <std_string.i>
%include "lib/engine/rs_vector.h"
%include "lib/scripting/python/rs_pythoncore.h"
%include "lib/scripting/python/rs_pythongui.h"
%include "lib/scripting/python/rs_pythondcl.h"
%include "lib/scripting/python/rs_pythonlisp.h"
