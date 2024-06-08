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

#include <cstdarg>
#include <cstdio>

#include <memory>
#include <iostream>

#include <QDateTime>
#include <QString>
#include <QTextStream>

#include "rs_debug.h"

// The implementation to delegate methods to QTextStream
struct RS_Debug::LogStream::StreamImpl : public QTextStream {
    StreamImpl(RS_Debug::RS_DebugLevel level) :
        QTextStream{&m_string, QIODeviceBase::WriteOnly}
      , m_debugLevel{level}
    {
    }

    QString m_string;
    RS_Debug::RS_DebugLevel m_debugLevel = RS_Debug::D_INFORMATIONAL;
};

RS_Debug::LogStream::LogStream(RS_Debug::RS_DebugLevel level)
    : m_pStream(std::make_unique<StreamImpl>(level))
{}

RS_Debug::LogStream::~LogStream() {
  if (m_pStream) {
    try {
        if (!m_pStream->m_string.isEmpty())
            RS_Debug::instance()->print(m_pStream->m_debugLevel, "%s",
                                        m_pStream->m_string.toStdString().c_str());
    } catch (...) {
        RS_Debug::instance()->print(RS_Debug::D_CRITICAL,
                                    "RS_Debug::LogStream:: Failed to log");
    }
  }
}

// delegate to QTextStream methods
RS_Debug::LogStream& RS_Debug::LogStream::operator()(RS_Debug::RS_DebugLevel level) {
    m_pStream->m_debugLevel = level;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(QChar ch) {
    *m_pStream << ch;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(char ch) {
    *m_pStream << ch;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(signed short i) {
    *m_pStream << i;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(unsigned short i) {
    *m_pStream << i;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(signed int i) {
    *m_pStream << i;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(unsigned int i) {
    *m_pStream << i;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(signed long i) {
    *m_pStream << i;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(unsigned long i) {
    *m_pStream << i;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(long long i) {
    *m_pStream << i;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(unsigned long long i) {
    *m_pStream << i;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(float f) {
    *m_pStream << f;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(double f) {
    *m_pStream << f;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(const QString &s) {
    *m_pStream << s;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(QStringView s) {
    *m_pStream << s;
    return *this;
}

RS_Debug::LogStream& RS_Debug::LogStream::operator<<(QLatin1String s) {
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
// end of QTextStream delegation

void debugHeader(char const *file, char const *func, int line) {
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
    static std::unique_ptr<RS_Debug> uniqueInstance;
    if (uniqueInstance == nullptr) {
        QDateTime now = QDateTime::currentDateTime();
        QString nowStr = now.toString("yyyyMMdd_hhmmss");

        uniqueInstance.reset(new RS_Debug);
    }
    return uniqueInstance.get();
}

class RS_Debug::impl
{
public:
  impl();
  ~impl();

  impl(const impl&) = delete;
  impl& operator=(const impl&) = delete;

  impl(const impl&&) = delete;
  impl& operator=(const impl&&) = delete;

  static LogStream Log(RS_DebugLevel level = D_DEBUGGING);

  void setLevel(RS_DebugLevel level);
  RS_DebugLevel getLevel();

  void vprint(RS_DebugLevel level, const char* format, va_list &ap);
  void vprint(const char *format, va_list &ap);

  void print(const char *format ...);
  void print(RS_DebugLevel level, const char *format ...);

  void print(const QString& text);
  void printUnicode(const QString& text);
  void timestamp();

private:
  RS_Debug::RS_DebugLevel debugLevel;
  FILE *stream;
};

/**
 * Constructor setting the default debug level.
 */
RS_Debug::impl::impl()
  : debugLevel(D_DEBUGGING)
  , stream(stderr) {
}

RS_Debug::impl::~impl() {
  if (stream != nullptr && stream != stderr && stream != stdout)
    fclose(stream);
}

/**
 * Sets the debugging level.
 */
void RS_Debug::impl::setLevel(RS_DebugLevel level) {
    debugLevel = level;
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
RS_Debug::RS_DebugLevel RS_Debug::impl::getLevel() { return debugLevel; }

/**
 * Prints the given message to stdout.
 */
void RS_Debug::impl::vprint(const char *format, va_list &ap) {
    if (debugLevel == D_DEBUGGING) {
        vfprintf(stream, format, ap);
        fprintf(stream, "\n");
        fflush(stream);
    }
}

/**
 * Prints the given message to stdout if the current debug level
 * is lower then the given level
 *
 * @param level Debug level.
 */
void RS_Debug::impl::vprint(RS_DebugLevel level, const char *format, va_list &ap) {
    if (debugLevel >= level) {
        vfprintf(stream, format, ap);
        fprintf(stream, "\n");
        fflush(stream);
    }
}

void RS_Debug::impl::print(const char *format ...)
{
  va_list ap;
  va_start(ap, format);
  vprint(format, ap);
  va_end(ap);
}

void RS_Debug::impl::print(RS_DebugLevel level, const char *format ...)
{
  va_list ap;
  va_start(ap, format);
  vprint(level, format, ap);
  va_end(ap);
}

/**
 * Prints a time stamp in the format yyyyMMdd_hhmmss.
 */
void RS_Debug::impl::timestamp() {
    QDateTime now = QDateTime::currentDateTime();
    QString nowStr = now.toString("yyyyMMdd_hh:mm:ss:zzz ");
    fprintf(stream, "%s", nowStr.toLatin1().data());
    fprintf(stream, "\n");
    fflush(stream);
}

/**
 * Prints the unicode for every character in the given string.
 */
void RS_Debug::impl::printUnicode(const QString &text) {
    for (char32_t v : text.toUcs4()) {
        print("[0x%X] ", v);
        QString str = QString::fromUcs4(&v, 1);
        print(" %s", str.toStdString().c_str());
    }
}

/**
 * Prints the unicode for every character in the given string.
 */
void RS_Debug::impl::print(const QString &text) {
    std::cerr << text.toStdString() << std::endl;
}

RS_Debug::RS_Debug() : m_pImpl(std::make_unique<impl>()) {}

RS_Debug::~RS_Debug() = default;

void RS_Debug::setLevel(RS_DebugLevel level) {
  m_pImpl->setLevel(level);
}

RS_Debug::RS_DebugLevel RS_Debug::getLevel() { return m_pImpl->getLevel(); }

void RS_Debug::print(const char *format ...) {
  va_list ap;
  va_start(ap, format);
  m_pImpl->vprint(format, ap);
  va_end(ap);
}

void RS_Debug::print(RS_DebugLevel level, const char *format ...) {
  va_list ap;
  va_start(ap, format);
  m_pImpl->vprint(level, format, ap);
  va_end(ap);
}

void RS_Debug::timestamp() {
  m_pImpl->timestamp();
}

void RS_Debug::printUnicode(const QString &text) {
  m_pImpl->printUnicode(text);
}

void RS_Debug::print(const QString &text) {
  m_pImpl->print(text);
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
RS_Debug::LogStream& RS_Debug::LogStream::operator<<(char16_t ch) {
    return *this << QChar(ch);
}
