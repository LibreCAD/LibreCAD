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
#include "rs_python.h"
#include "qg_py_commandedit.h"

#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QDir>
#include <QKeyEvent>
#include <QRegularExpression>
#include <QTextStream>
#include <QDebug>

#include "rs_commands.h"
#include "rs_dialogfactory.h"
#include "rs_math.h"
#include "rs_settings.h"

#include <iostream>
#include <fstream>
#include <cstdio>
#include <stdio.h>
#include <unistd.h>

#ifdef DEVELOPER

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
    : QLineEdit(parent)
    , keycode_mode(false)
    , relative_ray("none")
    , calculator_mode(false)
    , m_path(QDir(QDir::homePath() + QDir::separator() + ".py-history-librecad").absolutePath())
{
    setStyleSheet("selection-color: white; selection-background-color: green;");
    setFrame(false);
    setFocusPolicy(Qt::StrongFocus);
    prompt();

    QObject::connect(
        this,
        &QLineEdit::cursorPositionChanged,
        this,
        [this](){
            if (cursorPosition() < promptSize()) {
                setCursorPosition(promptSize());
            }
        });

    readHistoryFile();
}

void QG_Py_CommandEdit::readHistoryFile()
{
    m_histFile.setFileName(m_path);

    if (m_histFile.open(QIODevice::ReadOnly | QIODevice::Text | QIODevice::ReadWrite))
    {
        m_histFileStream.setDevice(&m_histFile);
        while (!m_histFileStream.atEnd())
        {
            historyList.append(m_histFileStream.readLine());
        }
        it = historyList.end();
    }
    m_histFile.close();
}

void QG_Py_CommandEdit::writeHistoryFile()
{
    m_histFile.setFileName(m_path);

    if (m_histFile.open(QIODevice::ReadWrite)) {

        m_histFileStream.setDevice(&m_histFile);

        for (const auto& i : historyList) {
            m_histFileStream << i << "\n";
        }
        m_histFile.close();
    }
}

QString QG_Py_CommandEdit::text() const
{
    QString str = QLineEdit::text();
    return (QLineEdit::text().size() >= promptSize()) ? str.remove(0, promptSize()) : QLineEdit::text();
}

/**
 * Bypass for key press events from keys...
 */
bool QG_Py_CommandEdit::event(QEvent* e) {
    return QLineEdit::event(e);
}

/**
 * History (arrow key up/down) support
 */
void QG_Py_CommandEdit::keyPressEvent(QKeyEvent* e)
{
    switch (e->key())
    {
    case Qt::Key_Up:
        if (!historyList.isEmpty() && it > historyList.begin())
        {
            it--;
            setText(prom + *it);
        }
        break;
    case Qt::Key_Down:
        if (!historyList.isEmpty() && it < historyList.end() )
        {
            it++;
            if (it<historyList.end()) {
                setText(prom + *it);
            }
            else {
                prompt();
            }
        }
        break;
    case Qt::Key_Backspace:
        if (cursorPosition() == promptSize())
        {
            break;
        }
        if (QLineEdit::text().size() > promptSize())
        {
            QLineEdit::keyPressEvent(e);
        }
        break;
    case Qt::Key_Enter:
    case Qt::Key_Return:
        processInput(text());
        emit returnPressed();
        break;
    case Qt::Key_Escape:
        if (text().isEmpty()) {
            emit escape();
        }
        else {
            prompt();
        }
        break;
    default:
        QLineEdit::keyPressEvent(e);
        break;
    }

    if (keycode_mode)
    {
        qDebug() << __func__ << "keycode_mode";
        auto input = text();
        if (input.size() == 2)
        {
            emit keycode(input);
        }
    }
}

void QG_Py_CommandEdit::focusInEvent(QFocusEvent *e) {
    emit focusIn();
    setCurrent();
    QLineEdit::focusInEvent(e);
}

void QG_Py_CommandEdit::focusOutEvent(QFocusEvent *e) {
    emit focusOut();
    QLineEdit::focusOutEvent(e);
}

void QG_Py_CommandEdit::processInput(QString input)
{
    setCurrent();

    if (!m_doProcess)
    {
        qDebug() << "[QG_Py_CommandEdit::processInput] !m_doProcess";
        emit message(prom);
        m_doProcess = true;
        return;
    }

    if (input.size() == 0)
    {
        qDebug() << "[QG_Py_CommandEdit::processInput] input.size() == 0";
        it = historyList.end();
        emit message(prom);
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

        qDebug() << "[QG_Py_CommandEdit::processInput] doProcessLc" << input;

        // author: ravas

        // convert 10..0 to @10,0
        static QRegularExpression regex(R"~(([-\w\.\\]+)\.\.)~");
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
    emit message(prom + input);
    qInfo() << qUtf8Printable(prom + input);
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

bool QG_Py_CommandEdit::isForeignCommand(QString input)
{
    // author: ravas

    bool r_value = true;

    if (input.contains("="))
    {
        auto var_value = input.split("=");
        variables[var_value[0]] = var_value[1];
        r_value = false;
    }
    return r_value;
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

    RS_PYTHON->runFileCmd(path, buffer_out, buffer_err);
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
}

void QG_Py_CommandEdit::processVariable(QString input)
{
    // author: ravas

    if (input.contains(","))
    {
        QString rel = "";

        if (input.contains("@"))
        {
            rel = "@";
            input.remove("@");
        }

        auto x_y = input.split(",");
        if (x_y[0].contains("\\"))
        {
            x_y[0].remove("\\");
            if (variables.contains(x_y[0]))
                x_y[0] = variables[x_y[0]];
        }
        if (x_y[1].contains("\\"))
        {
            x_y[1].remove("\\");
            if (variables.contains(x_y[1]))
                x_y[1] = variables[x_y[1]];
        }
        emit command(rel + x_y[0] + "," + x_y[1]);
        return;
    }

    input.remove("\\");
    if (variables.contains(input))
    {
        input = variables[input];
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
        else emit command(input);
    }
}

void QG_Py_CommandEdit::modifiedPaste()
{
    auto txt = qApp->clipboard()->text();
    txt.replace("\n", ";");
    setText(txt);
}

#endif // DEVELOPER
