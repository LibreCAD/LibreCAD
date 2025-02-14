/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
** Copyright (C) 2016 ravas (github.com/r-a-v-a-s)
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

#ifdef DEVELOPER

#include "rs_lisp.h"
#include "qg_lsp_commandedit.h"

#include <QRegularExpression>
#include <QDebug>

//namespace {
// Limits for command file reading
// limit for the number of lines read together
//constexpr unsigned g_maxLinesToRead = 10240;
// the maximum line length allowed
//constexpr unsigned g_maxLineLength = 4096;
//}

/**
 * Default Constructor. You must call init manually if you choose
 * to use this constructor.
 */
QG_Lsp_CommandEdit::QG_Lsp_CommandEdit(QWidget* parent)
    : CommandEdit(parent)
{
    resetPrompt();
}

void QG_Lsp_CommandEdit::resetPrompt()
{
    if (this->dockName() == "Lisp Ide")
    {
        setPrompt("_$ ");
    }
    else
    {
        setPrompt(QObject::tr("Command: "));
    }
    prompt();
}

void QG_Lsp_CommandEdit::processInput(QString input)
{
    setCurrent();

    if (!m_doProcess)
    {
        emit message("");
        m_doProcess = true;
        return;
    }

    if (input.size() == 0)
    {
        it = historyList.end();
        emit message("");
        prompt();
        return;
    }

    if (input == "(clear)" ||
        input == QObject::tr("clear"))
    {
        emit clearCommandsHistory();
        prompt();
        return;
    }

    if (input == "help" ||
        input == "copyright" ||
        input == "credits" ||
        input == "license")
    {
        input = "(" + input + ")";
    }

    static QRegularExpression lispRegex(QStringLiteral("[ \t]*[\[!(\"'`~:^]|[ \t]*@[a-zA-Z_-]"));
    QRegularExpressionMatch lispCom = lispRegex.match(input);

    if (isAlias(qUtf8Printable(input)) || lispCom.hasMatch())
    {
        QString buffer_out = "";
        //QString buffer_err = "";

        buffer_out += RS_LISP->runCommand(input).c_str();
        historyList.append(input);
        it = historyList.end();
        prompt();

        emit message(input);
        if (buffer_out.compare("") != 0) {
            const QString out = buffer_out;
            emit message(out);
        }
    }
    else
    {
        input.replace("pline", "polyline");
        // author: ravas

        // convert 10..0 to @10,0
        static const QRegularExpression regex(R"~(([-\w\.\\]+)\.\.)~");
        input.replace(regex, "@\\1,");

        if (isForeignCommand(input))
        {
            if (input.contains(";"))
            {
                foreach (auto str, input.split(";"))
                {
                    if (str.contains("\\"))
                        processVariable(str);
                    else
                        emit command(str);
                }
            }
            else
            {
                if (input.contains("\\"))
                    processVariable(input);
                else
                    emit command(input);
            }

            historyList.append(input);
            it = historyList.end();
            prompt();
        }
    }
}

void QG_Lsp_CommandEdit::setCurrent()
{
    Lisp_CommandEdit = this;
}

void QG_Lsp_CommandEdit::runFile(const QString& path)
{
    setCurrent();
    emit message(RS_LISP->runFileCmd(path).c_str());
}

#endif // DEVELOPER
