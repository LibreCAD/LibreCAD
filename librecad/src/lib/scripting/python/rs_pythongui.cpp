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

RS_PythonGui::RS_PythonGui()
{
}

RS_PythonGui::~RS_PythonGui()
{
}

void RS_PythonGui::MessageBox(const char *msg)
{
    RS_SCRIPTINGAPI->msgInfo(msg);
}

const std::string RS_PythonGui::OpenFileDialog(const char *title, const char *fileName, const char *fileExt)
{
    return QFileDialog::getOpenFileName(nullptr, QObject::tr(title), fileName, QObject::tr(fileExt)).toStdString();
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

const std::string RS_PythonGui::getString(const char *msg)
{
    QString result;

    if (Py_CommandEdit != nullptr)
    {
        Py_CommandEdit->setPrompt(QObject::tr(msg));
        Py_CommandEdit->setFocus();
        Py_CommandEdit->doProcess(false);

        result = RS_Py_InputHandle::readLine(Py_CommandEdit);

        Py_CommandEdit->resetPrompt();
    }
    else
    {
        result = RS_SCRIPTINGAPI->getStrDlg(msg).c_str();
    }

    return result.toStdString();
}

RS_Vector RS_PythonGui::getCorner(const char *msg, const RS_Vector &basePoint) const
{
    if (Py_CommandEdit != nullptr)
    {
        Py_CommandEdit->setPrompt(QObject::tr(msg));
        Py_CommandEdit->setFocus();
        Py_CommandEdit->doProcessLc(true);
    }

    RS_Vector result = RS_SCRIPTINGAPI->getCorner(msg, basePoint);

    if (Py_CommandEdit != nullptr)
    {
        Py_CommandEdit->resetPrompt();
        Py_CommandEdit->doProcessLc(false);
    }

    return result;
}

RS_Vector RS_PythonGui::getPoint(const char *msg, const RS_Vector basePoint) const
{
    if (Py_CommandEdit != nullptr)
    {
        Py_CommandEdit->setPrompt(QObject::tr(msg));
        Py_CommandEdit->setFocus();
        Py_CommandEdit->doProcessLc(true);
    }

    RS_Vector result = RS_SCRIPTINGAPI->getPoint(msg, basePoint);

    if (Py_CommandEdit != nullptr)
    {
        Py_CommandEdit->resetPrompt();
        Py_CommandEdit->doProcessLc(false);
    }

    return result;
}

PyObject* RS_PythonGui::getDist(const char *msg, const RS_Vector &basePoint) const
{
    if (Py_CommandEdit != nullptr)
    {
        Py_CommandEdit->setPrompt(QObject::tr(msg));
        Py_CommandEdit->setFocus();
        Py_CommandEdit->doProcessLc(true);
    }

    double distance;

    if (RS_SCRIPTINGAPI->getDist(msg, basePoint, distance))
    {
        if (Py_CommandEdit != nullptr)
        {
            Py_CommandEdit->resetPrompt();
            Py_CommandEdit->doProcessLc(false);
        }
        return Py_BuildValue("d", distance);
    }

    if (Py_CommandEdit != nullptr)
    {
        Py_CommandEdit->resetPrompt();
        Py_CommandEdit->doProcessLc(false);
    }

    Py_RETURN_NONE;
}

int RS_PythonGui::getInt(const char *msg) const
{
    int x = 0;
    QString result;
    static const std::regex intRegex("[+-]?[0-9]+|[+-]?0[xX][0-9A-Fa-f]");

    if (Py_CommandEdit != nullptr)
    {
        while (1)
        {
            Py_CommandEdit->setPrompt(QObject::tr(msg));
            Py_CommandEdit->setFocus();
            Py_CommandEdit->doProcess(false);

            result = RS_Py_InputHandle::readLine(Py_CommandEdit);
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
        x = RS_SCRIPTINGAPI->getIntDlg(msg);
    }
    return x;
}

double RS_PythonGui::getReal(const char *msg) const
{
    double x = 0;
    QString prompt = QObject::tr(msg);
    QString result;
    static const std::regex floatRegex("[+-]?[0-9]+(?:\\.[0-9]+)?(?:[eE][+-]?[0-9]+)?");

    if (Py_CommandEdit != nullptr)
    {
        while (1)
        {
            Py_CommandEdit->setPrompt(QObject::tr(qUtf8Printable(prompt)));
            Py_CommandEdit->setFocus();
            Py_CommandEdit->doProcess(false);

            result = RS_Py_InputHandle::readLine(Py_CommandEdit);
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
        x = RS_SCRIPTINGAPI->getDoubleDlg(qUtf8Printable(prompt));
    }
    return x;
}

double RS_PythonGui::getOrient(const char *msg, const RS_Vector &basePoint) const
{
    double x=0, y=0, z=0;
    QString prompt = QObject::tr(msg);
    Q_UNUSED(z)

    auto& appWin = QC_ApplicationWindow::getAppWindow();
    RS_Document* doc = appWin->getDocument();
    RS_GraphicView* graphicView = appWin->getGraphicView();

    if (graphicView == nullptr || graphicView->getGraphic() == nullptr)
    {
        qDebug() << "graphicView == nullptr";
        return 0.0;
    }

    if (Py_CommandEdit != nullptr)
    {
        Py_CommandEdit->setPrompt(QObject::tr(qUtf8Printable(prompt)));
        Py_CommandEdit->setFocus();
        Py_CommandEdit->doProcessLc(true);
    }

    QC_ActionGetPoint* a = new QC_ActionGetPoint(*doc, *graphicView);
    if (a)
    {
        QPointF *point = new QPointF;
        QPointF *base;
        bool status = false;

        if (!(prompt.isEmpty()))
        {
            a->setMessage(prompt);
        }

        graphicView->killAllActions();
        graphicView->setCurrentAction(a);

        if (basePoint.valid)
        {
            base = new QPointF(basePoint.x, basePoint.y);
            z = basePoint.z;
            a->setBasepoint(base);
        }
        else
        {
            return 0.0;
        }

        QEventLoop ev;
        while (!a->isCompleted())
        {
            ev.processEvents ();
            if (!graphicView->getEventHandler()->hasAction())
                break;
        }
        if (a->isCompleted() && !a->wasCanceled())
        {
            a->getPoint(point);
            status = true;
        }
        //RLZ: delete QC_ActionGetPoint. Investigate how to kill only this action
        graphicView->killAllActions();

        if (Py_CommandEdit != nullptr)
        {
            Py_CommandEdit->resetPrompt();
            Py_CommandEdit->doProcessLc(false);
        }

        if(status)
        {
            x = point->x();
            y = point->y();

            double rad = std::atan2(point->y() - y, point->x() - x);
            delete point;
            if (basePoint.valid)
            {
                delete base;
            }
            return rad;
        }
        else
        {
            delete point;
            if (basePoint.valid)
            {
                delete base;
            }
        }
    }

    return 0.0;
}

char RS_PythonGui::ReadCharDialog()
{
    return RS_InputDialog::readChar();
}

const std::string RS_PythonGui::getKword(const char *msg)
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
            Py_CommandEdit->setPrompt(QObject::tr(qUtf8Printable(msg)));
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
            result = RS_SCRIPTINGAPI->getStrDlg(qUtf8Printable(msg)).c_str();

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
