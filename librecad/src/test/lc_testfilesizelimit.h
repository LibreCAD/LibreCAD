/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 * ********************************************************************************
 */

#ifndef LC_TESTFILESIZELIMIT_H
#define LC_TESTFILESIZELIMIT_H

#include <QtGlobal>

#ifdef Q_OS_UNIX
#include <csignal>
#include <sys/resource.h>

#include <catch2/catch_test_macros.hpp>

namespace lc::test {

/**
 * Lowers the file-size limit of the process to 512 bytes while it lives, so a
 * file write is cut short there, as on a full disk. SIGXFSZ is ignored, so the
 * write fails instead of the process. Restore process-wide state before
 * Catch2 reports any failed assertions: end the scope of the limit first.
 */
class FileSizeLimit {
public:
    FileSizeLimit() {
        REQUIRE(getrlimit(RLIMIT_FSIZE, &m_previous) == 0);
        if (m_previous.rlim_max < 512)
            SKIP("The process file-size limit is already below the test limit");
        m_previousSignal = std::signal(SIGXFSZ, SIG_IGN);
        REQUIRE(m_previousSignal != SIG_ERR);
        rlimit limited = m_previous;
        limited.rlim_cur = 512;
        if (setrlimit(RLIMIT_FSIZE, &limited) != 0) {
            std::signal(SIGXFSZ, m_previousSignal);
            FAIL("Could not set the test file-size limit");
        }
    }
    ~FileSizeLimit() {
        setrlimit(RLIMIT_FSIZE, &m_previous);
        std::signal(SIGXFSZ, m_previousSignal);
    }
    FileSizeLimit(const FileSizeLimit&) = delete;
    FileSizeLimit& operator=(const FileSizeLimit&) = delete;

private:
    rlimit m_previous{};
    void (*m_previousSignal)(int) = SIG_DFL;
};

} // namespace lc::test

#endif // Q_OS_UNIX

#endif
