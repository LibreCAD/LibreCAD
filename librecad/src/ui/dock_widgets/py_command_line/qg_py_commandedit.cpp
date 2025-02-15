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

#include "rs_python.h"
#include "qg_py_commandedit.h"

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
QG_Py_CommandEdit::QG_Py_CommandEdit(QWidget* parent)
    : CommandEdit(parent)
{
    reset();
}

void QG_Py_CommandEdit::reset()
{
    doProcess(true);
    doProcessLc(false);
    setPrompt(">>> ");
    prompt();
}

void QG_Py_CommandEdit::processInput(QString input)
{
    setCurrent();

    if (!m_doProcess && !m_doProcessLc)
    {
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

    if (input == "clear" ||
        input == QObject::tr("clear"))
    {
        emit clearCommandsHistory();
        prompt();
        return;
    }

    if (m_doProcessLc ||
        input.startsWith('_'))
    {
        if (input.startsWith('_'))
        {
            input.remove(0, 1);
        }

        qDebug() << "[QG_Py_CommandEdit::processInput] LibreCAD Command" << input;

        // author: ravas

        // convert 10..0 to @10,0
        static const QRegularExpression regex(R"~(([-\w\.\\]+)\.\.)~");
        input.replace(regex, "@\\1,");

        if (isForeignCommand(input))
        {
            qDebug() << "[QG_Py_CommandEdit::processInput] isForeignCommand";
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
                {
                    qDebug() << "[QG_Py_CommandEdit::processInput] emit command";
                    emit command(input);
                }
            }

            historyList.append(input);
            it = historyList.end();
            prompt();
            return;
        }
    }

    qDebug() << "[QG_Py_CommandEdit::processInput] python:" << input;

    QString buffer_out = "";
    QString buffer_err = "";

    RS_PYTHON->runCommand(input, buffer_out, buffer_err);
    historyList.append(input);

    it = historyList.end();
    emit message(input);
    qInfo() << qUtf8Printable(input);
    if (buffer_out.compare("") != 0) {
        const QString out = buffer_out.remove(buffer_out.size()-1,1);
        emit message(out);
        qInfo() << qUtf8Printable(out);
    }
    if (buffer_err.compare("") != 0) {
        const QString err = buffer_err.remove(buffer_err.size()-1,1);
        emit message(err);
        qInfo() << qUtf8Printable(err);
    }
    prompt();
}

void QG_Py_CommandEdit::setCurrent()
{
    Py_CommandEdit = this;
}

void QG_Py_CommandEdit::runFile(const QString& path)
{
    setCurrent();
    QString buffer_out = "";
    QString buffer_err = "";

    if (!RS_PYTHON->runFileCmd(path, buffer_out, buffer_err))
    {
        const QString out = buffer_out.remove(buffer_out.size()-1,1);
        emit message(out);
        qInfo() << qUtf8Printable(out);
    }
    else
    {
        const QString err = buffer_err.remove(buffer_err.size()-1,1);
        emit message(err);
        qInfo() << qUtf8Printable(err);
    }
}

#endif // DEVELOPER
