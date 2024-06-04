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
#include <iostream>

#include <QDateTime>
#include <QString>
#include <QTextStream>

#include "rs_debug.h"

namespace {
FILE *stream = nullptr;
}

struct RS_Debug::LogStreamInterface::StreamImpl : public QTextStream {
  StreamImpl(RS_Debug::RS_DebugLevel level) : m_debugLevel{level} {
    setString(&m_string);
  }

  QString m_string;
  RS_Debug::RS_DebugLevel m_debugLevel;
};

RS_Debug::LogStreamInterface::LogStreamInterface(RS_Debug::RS_DebugLevel level)
    : m_pStream(new RS_Debug::LogStreamInterface::StreamImpl{level}) {}

RS_Debug::LogStreamInterface::~LogStreamInterface() {
  try {
    if (!m_pStream->m_string.isEmpty())
      RS_Debug::instance()->print(m_pStream->m_debugLevel, "%s",
                                  m_pStream->m_string.toStdString().c_str());
  } catch (...) {
    RS_Debug::instance()->print(RS_Debug::D_CRITICAL,
                                "RS_Debug::LogStream:: Failed to log");
  }
  delete m_pStream;
}

RS_Debug::LogStreamInterface &
RS_Debug::LogStreamInterface::operator()(RS_Debug::RS_DebugLevel level) {
  m_pStream->m_debugLevel = level;
  return *this;
}

RS_Debug::LogStreamInterface &
RS_Debug::LogStreamInterface::operator<<(QChar ch) {
  (*m_pStream) << ch;
  return *this;
}
RS_Debug::LogStreamInterface &
RS_Debug::LogStreamInterface::operator<<(char ch) {
  (*m_pStream) << ch;
  return *this;
}
RS_Debug::LogStreamInterface &
RS_Debug::LogStreamInterface::operator<<(signed short i) {
  (*m_pStream) << i;
  return *this;
}
RS_Debug::LogStreamInterface &
RS_Debug::LogStreamInterface::operator<<(unsigned short i) {
  (*m_pStream) << i;
  return *this;
}
RS_Debug::LogStreamInterface &
RS_Debug::LogStreamInterface::operator<<(signed int i) {
  (*m_pStream) << i;
  return *this;
}
RS_Debug::LogStreamInterface &
RS_Debug::LogStreamInterface::operator<<(unsigned int i) {
  (*m_pStream) << i;
  return *this;
}
RS_Debug::LogStreamInterface &
RS_Debug::LogStreamInterface::operator<<(signed long i) {
  (*m_pStream) << i;
  return *this;
}
RS_Debug::LogStreamInterface &
RS_Debug::LogStreamInterface::operator<<(unsigned long i) {
  (*m_pStream) << i;
  return *this;
}
RS_Debug::LogStreamInterface &
RS_Debug::LogStreamInterface::operator<<(long long i) {
  (*m_pStream) << i;
  return *this;
}
RS_Debug::LogStreamInterface &
RS_Debug::LogStreamInterface::operator<<(unsigned long long i) {
  (*m_pStream) << i;
  return *this;
}
RS_Debug::LogStreamInterface &
RS_Debug::LogStreamInterface::operator<<(float f) {
  (*m_pStream) << f;
  return *this;
}
RS_Debug::LogStreamInterface &
RS_Debug::LogStreamInterface::operator<<(double f) {
  (*m_pStream) << f;
  return *this;
}
RS_Debug::LogStreamInterface &
RS_Debug::LogStreamInterface::operator<<(const QString &s) {
  (*m_pStream) << s;
  return *this;
}
RS_Debug::LogStreamInterface &
RS_Debug::LogStreamInterface::operator<<(QStringView s) {
  (*m_pStream) << s;
  return *this;
}
RS_Debug::LogStreamInterface &
RS_Debug::LogStreamInterface::operator<<(QLatin1String s) {
  (*m_pStream) << s;
  return *this;
}
RS_Debug::LogStreamInterface &
RS_Debug::LogStreamInterface::operator<<(const QByteArray &array) {
  (*m_pStream) << array;
  return *this;
}
RS_Debug::LogStreamInterface &
RS_Debug::LogStreamInterface::operator<<(const char *c) {
  (*m_pStream) << c;
  return *this;
}
RS_Debug::LogStreamInterface &
RS_Debug::LogStreamInterface::operator<<(const void *ptr) {
  (*m_pStream) << ptr;
  return *this;
}

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
  static RS_Debug *uniqueInstance = nullptr;
  if (uniqueInstance == nullptr) {
    QDateTime now = QDateTime::currentDateTime();
    QString nowStr;
    nowStr = now.toString("yyyyMMdd_hhmmss");

    uniqueInstance = new RS_Debug;
    // uniqueInstance->stream = fopen(fName.latin1(), "wt");
    stream = stderr;
  }
  return uniqueInstance;
}

/**
 * Constructor setting the default debug level.
 */
RS_Debug::RS_Debug() { debugLevel = D_DEBUGGING; }

RS_Debug::~RS_Debug() {
  if (stream != nullptr and stream != stderr)
    fclose(stream);
}

/**
 * Sets the debugging level.
 */
void RS_Debug::setLevel(RS_DebugLevel level) {
  if (debugLevel == level)
    return;
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
RS_Debug::RS_DebugLevel RS_Debug::getLevel() { return debugLevel; }

/**
 * Prints the given message to stdout.
 */
void RS_Debug::print(const char *format...) {
  if (debugLevel == D_DEBUGGING) {
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
void RS_Debug::print(RS_DebugLevel level, const char *format...) {

  if (debugLevel >= level) {
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
void RS_Debug::printUnicode(const QString &text) {
  for (auto const &v : text) {
    print("[%X] %c", v.unicode(), v.toLatin1());
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
RS_Debug::LogStreamInterface RS_Debug::Log(RS_DebugLevel level) {
  return {level};
}

RS_Debug::LogStreamInterface &
RS_Debug::LogStreamInterface::operator<<(char16_t ch) {
  return *this << QChar(ch);
}
// EOF
