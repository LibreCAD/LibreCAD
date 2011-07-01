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


#include "rs_debug.h"

#include <stdio.h>
#include <stdarg.h>

#include <QDateTime>

RS_Debug* RS_Debug::uniqueInstance = NULL;


/**
 *  Gets the one and only RS_Debug instance
 *  (creates a new one on first call only)
 *
 *  @return Pointer to the single instance of this
 * singleton class
 */
RS_Debug* RS_Debug::instance() {
    if(uniqueInstance==NULL) {
        QDateTime now = QDateTime::currentDateTime();
        QString nowStr;
		nowStr = now.toString("yyyyMMdd_hhmmss");

                //QString fName = QString("%1/debug_%2.log")
		//	.arg(RS_SYSTEM->getHomeDir())
		//	.arg(nowStr);
                QString fName = QString("debug_%1.log")
			.arg(nowStr);

        uniqueInstance = new RS_Debug;
        //uniqueInstance->stream = fopen(fName.latin1(), "wt");
        uniqueInstance->stream = stderr;
    }
    return uniqueInstance;
}


/**
 * Deletes the one and only RS_Debug instance.
 */
void
RS_Debug::deleteInstance() {
    if (uniqueInstance!=NULL) {
        fclose(uniqueInstance->stream);
        delete uniqueInstance;
    }
}


/**
 * Constructor for a point with default coordinates.
 */
RS_Debug::RS_Debug() {
    debugLevel = D_DEBUGGING;
}

/**
 * Sets the debugging level.
 */
void RS_Debug::setLevel(RS_DebugLevel level) {
    debugLevel = level;
    print("RS_DEBUG: Warnings", D_WARNING);
    print("RS_DEBUG: Errors", D_ERROR);
    print("RS_DEBUG: Notice", D_NOTICE);
    print("RS_DEBUG: Informational", D_INFORMATIONAL);
    print("RS_DEBUG: Debugging", D_DEBUGGING);
}


/**
 * Gets the current debugging level.
 */
RS_Debug::RS_DebugLevel RS_Debug::getLevel() {
    return debugLevel;
}


/**
 * Prints the given message to stdout.
 */
void RS_Debug::print(const char* format ...) {
    if(debugLevel==D_DEBUGGING) {
        va_list ap;
        va_start(ap, format);
        vfprintf(stream, format, ap);
        fprintf(stream, "\n");
        va_end(ap);
        fflush(stream);
    }

}

/**
 * Prints the given message to stdout if the current debug level
 * is lower then the given level
 *
 * @param level Debug level.
 */
void RS_Debug::print(RS_DebugLevel level, const char* format ...) {

    if(debugLevel>=level) {
        va_list ap;
        va_start(ap, format);
        vfprintf(stream, format, ap);
        fprintf(stream, "\n");
        va_end(ap);
        fflush(stream);
    }

}


/**
 * Prints a time stamp in the format yyyyMMdd_hhmmss.
 */
void RS_Debug::timestamp() {
    QDateTime now = QDateTime::currentDateTime();
    QString nowStr;

		nowStr = now.toString("yyyyMMdd_hh:mm:ss:zzz ");
    fprintf(stream, "%s", nowStr.toLatin1().data());
    fprintf(stream, "\n");
    fflush(stream);
}


/**
 * Prints the unicode for every character in the given string.
 */
void RS_Debug::printUnicode(const QString& text) {
    for (int i=0; i<(int)text.length(); i++) {
        print("[%X] %c", text.at(i).unicode(), text.at(i).toLatin1());
    }
}


// EOF
