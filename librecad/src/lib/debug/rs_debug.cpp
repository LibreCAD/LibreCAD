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

#include <cstdarg>
#include <cstdio>
#include <iostream>

#include <QDateTime>
#include <QIODevice>
#include <QString>

#include "rs_debug.h"

namespace {
FILE* stream = nullptr;
}

void debugHeader(char const* file, char const* func, int line)
{
    std::cout<<file<<" : "<<func<<" : line "<<line<<std::endl;
}

/**
 *  Gets the one and only RS_Debug instance
 *  (creates a new one on first call only)
 *
 *  @return Pointer to the single instance of this
 * singleton class
 */
RS_Debug* RS_Debug::instance() {
    static RS_Debug* uniqueInstance = nullptr;
    if(uniqueInstance == nullptr) {
        QDateTime now = QDateTime::currentDateTime();
        QString nowStr;
        nowStr = now.toString("yyyyMMdd_hhmmss");

        uniqueInstance = new RS_Debug;
        //uniqueInstance->stream = fopen(fName.latin1(), "wt");
        stream = stderr;
    }
    return uniqueInstance;
}


/**
 * Constructor setting the default debug level.
 */
RS_Debug::RS_Debug() {
    debugLevel = D_DEBUGGING;
}

RS_Debug::~RS_Debug() {
    if (stream != nullptr and stream != stderr)
        fclose(stream);
}

/**
 * Sets the debugging level.
 */
void RS_Debug::setLevel(RS_DebugLevel level) {
    if(debugLevel==level) return;
    debugLevel = level;
    print( D_NOTHING, "RS_DEBUG::setLevel(%d)", level);
    print( D_CRITICAL, "RS_DEBUG: Critical");
    print( D_ERROR, "RS_DEBUG: Errors");
    print( D_WARNING, "RS_DEBUG: Warnings");
    print( D_NOTICE, "RS_DEBUG: Notice");
    print( D_INFORMATIONAL, "RS_DEBUG: Informational");
    print( D_DEBUGGING, "RS_DEBUG: Debugging");
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
    for(auto const& v: text){
        print("[%X] %c", v.unicode(), v.toLatin1());
    }
}


/**
 * Prints the unicode for every character in the given string.
 */
void RS_Debug::print(const QString& text) {
    std::cerr<<text.toStdString()<<std::endl;
}

/**
 * @brief RS_Debug::Log - returns an instance of stringstream. Anything directed to the stringstream will be redirected
 * to the RS_Debug stream at the end of lifetime of the stringstream
 * @param level - debugging level
 * @return RS_Debug::LogStream - an instance of LogStream
 */
RS_Debug::LogStream RS_Debug::Log(RS_DebugLevel level)
{
    return {level};
}
RS_Debug::LogStream::LogStream(RS_DebugLevel level):
    m_debugLevel{level}
{
    setString(&m_string, QIODevice::WriteOnly);
}

/**
 * output buffered in stringstream
 */
RS_Debug::LogStream::~LogStream()
{
    try {
        if (!m_string.isEmpty())
            instance()->print(m_debugLevel, "%s", m_string.toStdString().c_str());
    } catch (...)
    {
        instance()->print(D_CRITICAL, "RS_Debug::LogStream:: Failed to log");
    }
}
// EOF
