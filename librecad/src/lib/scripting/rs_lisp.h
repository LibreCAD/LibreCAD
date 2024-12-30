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

#ifndef RS_LISP_H
#define RS_LISP_H

#include <QString>
#include <QThread>
#include "lisp.h"

#ifdef DEVELOPER

#define RS_LISP RS_Lisp::instance()

#if 0
typedef struct v3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
} v3_t;
#endif
/*
 * LibreCAD Lisp scripting support.
 *
 * written by
 *
 * @author Emanuel Strobel
 *
 * 2024
 *
 */

class RS_Lisp
{
public:
    static RS_Lisp* instance();
    ~RS_Lisp();

    QString Lisp_GetVersionString() const { return QString(Lisp_GetVersion()) + QString(LISP_COPYRIGHT); }

    std::string runCommand(const QString& command);
    std::string runFileCmd(const QString& name);
    int runFile(const QString& name);
    int runString(const QString& str);
private:
    RS_Lisp();
    static RS_Lisp* unique;
};

class SleeperThread : public QThread
{
public:
    static void msleep(unsigned long msecs)
    {
        QThread::msleep(msecs);
    }
};

#endif // DEVELOPER

#endif // RS_PYTHON_H

