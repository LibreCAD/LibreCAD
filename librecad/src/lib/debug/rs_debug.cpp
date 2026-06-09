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
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/
#include "rs_debug.h"

#include <QDateTime>
#include <QString>
#include <QTextStream>
#include <iostream>

#include "rs_vector.h"

namespace {
FILE *s_logStream = nullptr;
}

// The implementation to delegate methods to QTextStream
struct RS_Debug::LogStream::StreamImpl : QTextStream {
    StreamImpl(const RS_DebugLevel level) :
        QTextStream{&string, WriteOnly}
      , debugLevel{level}{
    }

    QString string;
    RS_DebugLevel debugLevel = D_INFORMATIONAL;
};

RS_Debug::LogStream::LogStream(const RS_DebugLevel level)
    : m_pStream(new StreamImpl{level})
{}

RS_Debug::LogStream::~LogStream() {
    try {
        if (!m_pStream->string.isEmpty()) {
            instance()->print(m_pStream->debugLevel, "%s",
                                        m_pStream->string.toStdString().c_str());
        }
    } catch (...) {
        instance()->print(D_CRITICAL,
                                    "RS_Debug::LogStream:: Failed to log");
    }
    delete m_pStream;
}

// delegate to QTextStream methods
RS_Debug::LogStream& RS_Debug::LogStream::operator()(const RS_DebugLevel level) {
    m_pStream->debugLevel = level;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(const QChar ch) {
    *m_pStream << ch;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(const char ch) {
    *m_pStream << ch;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(const signed short i) {
    *m_pStream << i;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(const unsigned short i) {
    *m_pStream << i;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(const signed int i) {
    *m_pStream << i;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(const unsigned int i) {
    *m_pStream << i;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(const signed long i) {
    *m_pStream << i;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(const unsigned long i) {
    *m_pStream << i;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(const long long i) {
    *m_pStream << i;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(const unsigned long long i) {
    *m_pStream << i;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(const float f) {
    *m_pStream << f;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(const double f) {
    *m_pStream << f;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(const QString &s) {
    *m_pStream << s;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(const QStringView s) {
    *m_pStream << s;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(const QLatin1String s) {
    *m_pStream << s;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(const QByteArray &array) {
    *m_pStream << array;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(const char *c) {
    *m_pStream << c;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(const void *ptr) {
    *m_pStream << ptr;
    return *this;
}

RS_Debug::LogStream &RS_Debug::LogStream::operator<<(const RS_Vector & v) {
    *m_pStream << "(" << v.x << "," << v.y <<  (v.valid ? ")" : "!)");
    return *this;
}
// end of QTextStream delegation

void debugHeader(const char*file, const char*func, const int line) {
    std::cout << file << " : " << func << " : line " << line << std::endl;
}

/**
 *  Gets the one and only RS_Debug instance
 *  (creates a new one on first call only)
 *
 *  @return Pointer to the single instance of this
 * singleton class
 */
RS_Debug *RS_Debug::instance() {
    static RS_Debug *uniqueInstance = nullptr;
    if (uniqueInstance == nullptr) {
        // QDateTime now = QDateTime::currentDateTime();
        // QString nowStr = now.toString("yyyyMMdd_hhmmss");
        uniqueInstance = new RS_Debug;
        s_logStream = stderr;
    }
    return uniqueInstance;
}

/**
 * Constructor setting the default debug level.
 */
RS_Debug::RS_Debug()  :
    m_debugLevel{ D_DEBUGGING}
{}

RS_Debug::~RS_Debug() {
    try {
        if (s_logStream != nullptr && s_logStream != stderr && s_logStream != stdout) {
            fclose(s_logStream);
        }
    }
    catch(...) {
        std::cerr<<"RS_Debug::"<<__func__<<":: Failed to close stream";
    }
}

/**
 * Sets the debugging level.
 */
void RS_Debug::setLevel(const RS_DebugLevel level)
{

    if (m_debugLevel == level) {
        return;
    }
    m_debugLevel = level;
    print(D_NOTHING, "RS_DEBUG::setLevel(%d)", level);
    print(D_CRITICAL, "RS_DEBUG: Critical");
    print(D_ERROR, "RS_DEBUG: Errors");
    print(D_WARNING, "RS_DEBUG: Warnings");
    print(D_NOTICE, "RS_DEBUG: Notice");
    print(D_INFORMATIONAL, "RS_DEBUG: Informational");
    print(D_DEBUGGING, "RS_DEBUG: Debugging");
}

/**
 * Gets the current debugging level.
 */
RS_Debug::RS_DebugLevel RS_Debug::getLevel() const { return m_debugLevel; }

/**
 * Prints the given message to stdout.
 */
void RS_Debug::print(const char *format...)  {
    if (m_debugLevel == D_DEBUGGING) {
        va_list ap;
        va_start(ap, format);
        vfprintf(s_logStream, format, ap);
        fprintf(s_logStream, "\n");
        va_end(ap);
        fflush(s_logStream);
    }
}

/**
 * Prints the given message to stdout if the current debug level
 * is lower than the given level
 *
 * @param level Debug level.
 * @param format
 * @param ...
 */
void RS_Debug::print(const RS_DebugLevel level, const char *format...) const {

    if (m_debugLevel >= level) {
        va_list ap;
        va_start(ap, format);
        vfprintf(s_logStream, format, ap);
        fprintf(s_logStream, "\n");
        va_end(ap);
        fflush(s_logStream);
    }
}

/**
 * Prints a time stamp in the format yyyyMMdd_hhmmss.
 */
void RS_Debug::timestamp() {
    const QDateTime now = QDateTime::currentDateTime();

    const QString nowStr = now.toString("yyyyMMdd_hh:mm:ss:zzz ");
    fprintf(s_logStream, "%s", nowStr.toLatin1().data());
    fprintf(s_logStream, "\n");
    fflush(s_logStream);
}

/**
 * Prints the unicode for every character in the given string.
 */
void RS_Debug::printUnicode(const QString &text) {
    for (char32_t v : text.toUcs4()) {
        print("[0x%X] ", v);
        QString str = QString::fromUcs4(&v, 1);
        print(" %s", str.toStdString().c_str());
    }
}

/**
 * Prints the unicode for every character in the given string.
 */
void RS_Debug::print(const QString &text) {
    std::cerr << text.toStdString() << std::endl;
}

/**
 * @brief RS_Debug::Log - returns an instance of stringstream. Anything directed
 * to the stringstream will be redirected to the RS_Debug stream at the end of
 * lifetime of the stringstream
 * @param level - debugging level
 * @return RS_Debug::LogStream - an instance of LogStream
 */
RS_Debug::LogStream RS_Debug::Log(RS_DebugLevel level) {
    return {level};
}

/**
 * @brief QChar is UTF16 chars
 * @param ch a UTF16 char
 * @return RS_Debug::LogStream& to allow chaining
 */
RS_Debug::LogStream& RS_Debug::LogStream::operator<<(const char16_t ch) {
    return *this << QChar(ch);
}
