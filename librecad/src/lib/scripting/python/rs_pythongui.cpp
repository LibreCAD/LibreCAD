/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#include "rs_python.h"
#include "rs_pythongui.h"
#include "rs_scriptingapi.h"

#include "rs_dialogs.h"
#include "rs_py_inputhandle.h"

#include "Types.h"
#include "Environment.h"

#include <regex>

#include "qc_applicationwindow.h"
#include "rs_eventhandler.h"
#include "intern/qc_actiongetpoint.h"
#include "intern/qc_actiongetcorner.h"

#include <QMessageBox>
#include <QFileDialog>

void RS_PythonGui::MessageBox(const char *message)
{
    RS_SCRIPTINGAPI->msgInfo(message);
}

const std::string RS_PythonGui::OpenFileDialog(const char *title, const char *filename, const char *ext)
{
    return QFileDialog::getOpenFileName(nullptr, QObject::tr(title), filename, QObject::tr(ext)).toStdString();
}

int RS_PythonGui::GetIntDialog(const char *prompt)
{
    return RS_SCRIPTINGAPI->getIntDlg(prompt);
}

double RS_PythonGui::GetDoubleDialog(const char *prompt)
{
    return RS_SCRIPTINGAPI->getDoubleDlg(prompt);
}

const std::string RS_PythonGui::GetStringDialog(const char *prompt)
{
    return RS_SCRIPTINGAPI->getStrDlg(prompt);
}

PyObject* RS_PythonGui::getString(const char *prompt) const
{
    QString prom = "Enter a string: ";
    QString result;

    if (std::strcmp(prompt, ""))
    {
        prom = prompt;
    }

    if (Py_CommandEdit != nullptr)
    {
        Py_CommandEdit->setPrompt(QObject::tr(qUtf8Printable(prom)));
        Py_CommandEdit->setFocus();
        Py_CommandEdit->doProcess(false);

        result = RS_Py_InputHandle::readLine(Py_CommandEdit);
        if (result.isEmpty())
        {
            Py_RETURN_NONE;
        }

        Py_CommandEdit->resetPrompt();
    }
    else
    {
        result = RS_SCRIPTINGAPI->getStrDlg(qUtf8Printable(prom)).c_str();
    }

    return Py_BuildValue("s", qUtf8Printable(result));
}

RS_Vector RS_PythonGui::getCorner(const char *prompt, const RS_Vector &basePoint) const
{
    if (Py_CommandEdit != nullptr)
    {
        Py_CommandEdit->setPrompt(QObject::tr(prompt));
        Py_CommandEdit->setFocus();
        Py_CommandEdit->doProcessLc(true);
    }

    RS_Vector result = RS_SCRIPTINGAPI->getCorner(prompt, basePoint);

    if (Py_CommandEdit != nullptr)
    {
        Py_CommandEdit->resetPrompt();
        Py_CommandEdit->doProcessLc(false);
    }

    return result;
}

RS_Vector RS_PythonGui::getPoint(const char *prompt, const RS_Vector basePoint) const
{
    if (Py_CommandEdit != nullptr)
    {
        Py_CommandEdit->setPrompt(QObject::tr(prompt));
        Py_CommandEdit->setFocus();
        Py_CommandEdit->doProcessLc(true);
    }

    RS_Vector result = RS_SCRIPTINGAPI->getPoint(prompt, basePoint);

    if (Py_CommandEdit != nullptr)
    {
        Py_CommandEdit->resetPrompt();
        Py_CommandEdit->doProcessLc(false);
    }

    return result;
}

PyObject* RS_PythonGui::getDist(const char *prompt, const RS_Vector &basePoint) const
{
    QString prom = "Enter second point: ";

    if (std::strcmp(prompt, ""))
    {
        prom = prompt;
    }

    if (Py_CommandEdit != nullptr)
    {
        Py_CommandEdit->setPrompt(QObject::tr(prompt));
        Py_CommandEdit->setFocus();
        Py_CommandEdit->doProcessLc(true);
    }

    double distance;

    if (basePoint.valid)
    {
        if (RS_SCRIPTINGAPI->getDist(qUtf8Printable(prom), basePoint, distance))
        {
            if (Py_CommandEdit != nullptr)
            {
                Py_CommandEdit->resetPrompt();
                Py_CommandEdit->doProcessLc(false);
            }
            return Py_BuildValue("d", distance);
        }
    }
    else
    {
        RS_Vector result = RS_SCRIPTINGAPI->getPoint(qUtf8Printable(QObject::tr("Enter first point: ")), basePoint);

        if (result.valid && RS_SCRIPTINGAPI->getDist(qUtf8Printable(prom), basePoint, distance))
        {
            if (Lisp_CommandEdit != nullptr)
            {
                Lisp_CommandEdit->resetPrompt();
                Py_CommandEdit->doProcessLc(false);
            }
            return Py_BuildValue("d", distance);
        }
    }


    if (Py_CommandEdit != nullptr)
    {
        Py_CommandEdit->resetPrompt();
        Py_CommandEdit->doProcessLc(false);
    }

    Py_RETURN_NONE;
}

PyObject* RS_PythonGui::getInt(const char *prompt) const
{
    QString prom = "Enter an integer: ";

    if (std::strcmp(prompt, ""))
    {
        prom = prompt;
    }

    int x;
    static const std::regex intRegex("[+-]?[0-9]+|[+-]?0[xX][0-9A-Fa-f]");

    if (Py_CommandEdit != nullptr)
    {
        while (1)
        {
            Py_CommandEdit->setPrompt(QObject::tr(qUtf8Printable(prom)));
            Py_CommandEdit->setFocus();
            Py_CommandEdit->doProcess(false);

            QString result = RS_Py_InputHandle::readLine(Py_CommandEdit);
            if (result.isEmpty())
            {
                Py_RETURN_NONE;
            }
            if (std::regex_match(qUtf8Printable(result), intRegex))
            {
                x = result.toInt();
                break;
            }
        }
        Py_CommandEdit->resetPrompt();
    }
    else
    {
        x = RS_SCRIPTINGAPI->getIntDlg(qUtf8Printable(prom));
    }
    return Py_BuildValue("i", x);
}

PyObject* RS_PythonGui::getReal(const char *prompt) const
{
    QString prom = "Enter a floating point number: ";

    if (std::strcmp(prompt, ""))
    {
        prom = prompt;
    }

    double x = 0;
    QString result;
    static const std::regex floatRegex("[+-]?[0-9]+(?:\\.[0-9]+)?(?:[eE][+-]?[0-9]+)?");

    if (Py_CommandEdit != nullptr)
    {
        while (1)
        {
            Py_CommandEdit->setPrompt(QObject::tr(qUtf8Printable(prom)));
            Py_CommandEdit->setFocus();
            Py_CommandEdit->doProcess(false);

            result = RS_Py_InputHandle::readLine(Py_CommandEdit);
            if (result.isEmpty())
            {
                Py_RETURN_NONE;
            }
            if (std::regex_match(qUtf8Printable(result), floatRegex))
            {
                x = result.toInt();
                break;
            }
        }
        Py_CommandEdit->resetPrompt();
    }
    else
    {
        x = RS_SCRIPTINGAPI->getDoubleDlg(qUtf8Printable(prom));
    }
    return Py_BuildValue("d", x);
}

PyObject* RS_PythonGui::getOrient(const char *prompt, const RS_Vector &basePoint) const
{
    QString prom = "Enter second point: ";

    if (std::strcmp(prompt, ""))
    {
        prom = prompt;
    }

    if (Py_CommandEdit != nullptr)
    {
        Py_CommandEdit->setPrompt(QObject::tr(qUtf8Printable(prom)));
        Py_CommandEdit->setFocus();
        Py_CommandEdit->doProcessLc(true);
    }

    double radius;

    if (basePoint.valid)
    {
        if(RS_SCRIPTINGAPI->getOrient(qUtf8Printable(prom), basePoint, radius))
        {
            if (Lisp_CommandEdit != nullptr)
            {
                Lisp_CommandEdit->resetPrompt();
                Py_CommandEdit->doProcessLc(false);
            }
            return Py_BuildValue("d", radius);
        }
    }
    else
    {
        RS_Vector result = RS_SCRIPTINGAPI->getPoint(qUtf8Printable(QObject::tr("Enter first point: ")), basePoint);

        if (result.valid && RS_SCRIPTINGAPI->getOrient(qUtf8Printable(prom), result, radius))
        {
            if (Lisp_CommandEdit != nullptr)
            {
                Lisp_CommandEdit->resetPrompt();
                Py_CommandEdit->doProcessLc(false);
            }
            return Py_BuildValue("d", radius);
        }
    }

    if (Py_CommandEdit != nullptr)
    {
        Py_CommandEdit->resetPrompt();
        Py_CommandEdit->doProcessLc(false);
    }

    Py_RETURN_NONE;
}

char RS_PythonGui::ReadCharDialog()
{
    return RS_InputDialog::readChar();
}

const std::string RS_PythonGui::getKword(const char *prompt)
{
    const lclInteger* bit = VALUE_CAST(lclInteger, shadowEnv->get("initget_bit"));
    const lclString* pat = VALUE_CAST(lclString, shadowEnv->get("initget_string"));

    std::vector<String> StringList;
    String del = " ";
    QString result;
    String pattern = pat->value();

    auto pos = pattern.find(del);
    while (pos != String::npos)
    {
        StringList.push_back(pattern.substr(0, pos));
        pattern.erase(0, pos + del.length());
        pos = pattern.find(del);
    }
    StringList.push_back(pattern);

    if (Py_CommandEdit != nullptr)
    {
        while (1) {
            Py_CommandEdit->setPrompt(QObject::tr(qUtf8Printable(prompt)));
            Py_CommandEdit->setFocus();
            Py_CommandEdit->doProcess(false);
            result = RS_Py_InputHandle::readLine(Py_CommandEdit);

            for (auto &it : StringList) {
                if (it == qUtf8Printable(result)) {
                    Py_CommandEdit->resetPrompt();
                    std::string res = qUtf8Printable(result);
                    return res.c_str();
                }
            }
            if ((bit->value() & 1) != 1) {
                Py_CommandEdit->resetPrompt();
                return "";
            }
        }
    }
    else
    {
        while (1) {
            result = RS_SCRIPTINGAPI->getStrDlg(qUtf8Printable(prompt)).c_str();

            for (auto &it : StringList)
            {
                if (it == qUtf8Printable(result))
                {
                    return result.toStdString();
                }
            }
            if ((bit->value() & 1) != 1)
            {
                return "";
            }
        }
    }

    return "";
}

void RS_PythonGui::initGet(const char *str, int bit)
{
    shadowEnv->set("initget_bit", lcl::integer(bit));
    shadowEnv->set("initget_string", lcl::string(str));
}

void RS_PythonGui::prompt(const char *prompt)
{
    if (Py_CommandEdit != nullptr)
    {
        Py_CommandEdit->setPrompt(prompt);
        Py_CommandEdit->setFocus();
        Py_CommandEdit->doProcess(false);

        RS_Py_InputHandle::readLine(Py_CommandEdit);
        Py_CommandEdit->resetPrompt();
    }
    else
    {
        RS_SCRIPTINGAPI->msgInfo(prompt);
    }
}
