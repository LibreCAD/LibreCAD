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

#include "rs_lisp.h"
#include <QtGlobal>
#include <QJsonArray>
#include <QDebug>
#include <QByteArray>
#include <QMessageBox>
#include <QApplication>
#include <cassert>

#include <iostream>
#include <sstream>

#ifdef DEVELOPER

/**
 * static instance to class RS_Lisp
 */
RS_Lisp* RS_Lisp::unique = nullptr;

RS_Lisp* RS_Lisp::instance() {
    qInfo() << "[RS_Lisp] RS_Lisp::instance requested";
    if (unique == nullptr) {
        unique = new RS_Lisp();
    }
    return unique;
}

/**
 * class RS_Lisp (interpreter)
 */
RS_Lisp::RS_Lisp()
{
    //qputenv("LISPPATH", QByteArray("."));

    Lisp_Initialize();

    qInfo() << qUtf8Printable(Lisp_GetVersionString());
}

RS_Lisp::~RS_Lisp()
{
    qDebug() << "[RS_Lisp::~RS_Lisp] Lisp finalize...";
}

/**
 * Launches the given script.
 */
int RS_Lisp::runFile(const QString& name)
{
#if 0
    //qDebug() << "[RS_Lisp::runFile]" << name;
    return LispRun_SimpleFile(qUtf8Printable(name));
#endif
    std::ostringstream lispErr;
    // save pointer to old std::cout buffer
    auto cout_buff = std::cout.rdbuf();
    // substitute internal std::cout buffer with
    std::cout.rdbuf(lispErr.rdbuf());

    // now std::cout work with 'lispOut' buffer
    lispErr << std::endl << "Traceback (most recent call last):" << std::endl << " " << QObject::tr("File").toStdString() << ": " << name.toStdString() << std::endl;
    std::string lispValue = Lisp_EvalFile(qUtf8Printable(name));

    std::cout.flush();

    // go back to old buffer
    std::cout.rdbuf(cout_buff);

    // check for errors
    if(Lisp_GetError())
    {
        Lisp_FreeError();

        lispErr << lispValue;

        QMessageBox msgBox;
        msgBox.setWindowTitle(QObject::tr("Lisp Error!"));
        msgBox.setText(QObject::tr("File: %1").arg(name));
        msgBox.setDetailedText(lispErr.str().c_str());
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStyleSheet("QTextEdit{min-height: 240px;} QLabel{min-width: 240px;}");
        msgBox.exec();
        return -1;
    }
    return 0;
}

/**
 * run a simple lisp string
 */
int RS_Lisp::runString(const QString& str)
{
    //qDebug() << "[RS_Lisp::runString]" << str;
    return LispRun_SimpleString(qUtf8Printable(str));
}

/**
 * run a string from lisp command line
 */
std::string RS_Lisp::runCommand(const QString& command)
{
    //qDebug() << "[RS_Lisp::runCommand]" << command;
    std::ostringstream lispOut;
    // save pointer to old std::cout buffer
    auto cout_buff = std::cout.rdbuf();
    // substitute internal std::cout buffer with
    std::cout.rdbuf(lispOut.rdbuf());

    // now std::cout work with 'lispOut' buffer
    std::string lispValue = Lisp_EvalString(command.toStdString());
    Lisp_FreeError();
    std::cout.flush();

    // go back to old buffer
    std::cout.rdbuf(cout_buff);
    // add Lisp exec value to Lisp prompt
    lispOut << lispValue;
    // print 'lispOut' content
    std::cout << lispOut.str() << std::endl;

    return lispOut.str();
}

/**
 * Launches the given script in command line.
 */
std::string RS_Lisp::runFileCmd(const QString& name)
{
    std::ostringstream lispOut;
    // save pointer to old std::cout buffer
    auto cout_buff = std::cout.rdbuf();
    // substitute internal std::cout buffer with
    std::cout.rdbuf(lispOut.rdbuf());

    // now std::cout work with 'lispOut' buffer
    std::string lispValue = Lisp_EvalFile(qUtf8Printable(name));

    std::cout.flush();

    // go back to old buffer
    std::cout.rdbuf(cout_buff);
    // add Lisp exec value to Lisp prompt
    lispOut << lispValue;
    // print 'lispOut' content

    if(Lisp_GetError())
    {
        Lisp_FreeError();

        std::cout << QObject::tr("File").toStdString() << ": " << name.toStdString() << std::endl << lispOut.str() << std::endl;
        return QObject::tr("File").toStdString() + ": " + name.toStdString() + "\n" + lispOut.str();
    }

    std::cout << lispOut.str() << std::endl;

    return lispOut.str();
}

#endif // DEVELOPER
