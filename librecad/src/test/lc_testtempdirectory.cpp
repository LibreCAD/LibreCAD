/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 Dongxu Li (github.com/dxli)
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

#include "lc_testtempdirectory.h"

#include <chrono>
#include <cstdlib>
#include <string>
#include <system_error>

namespace {

// Sets the variable the standard library and Qt read the directory from.
void setTempDirectory(const std::filesystem::path& directory) {
#ifdef _WIN32
    _wputenv((L"TMP=" + directory.native()).c_str());
#else
    setenv("TMPDIR", directory.c_str(), 1);
#endif
}

class PrivateTempDirectory final {
public:
    // Only calls that report a failure through an error code: an exception
    // before main() would end the process.
    PrivateTempDirectory() {
        std::error_code error;
        m_shared = std::filesystem::temp_directory_path(error);
        auto id = std::chrono::system_clock::now().time_since_epoch().count();
        bool created = false;
        // create_directory() is false, without an error, for a name that is taken
        while (!created && !error) {
            m_path = m_shared / ("librecad-tests-" + std::to_string(id++));
            created = std::filesystem::create_directory(m_path, error);
        }
        if (created) {
            setTempDirectory(m_path);
        } else {
            m_path.clear();
        }
    }

    ~PrivateTempDirectory() {
        if (m_path.empty()) {
            return;
        }
        // first, so that whatever still runs is not sent to a directory that is gone
        setTempDirectory(m_shared);
        std::error_code error;
        std::filesystem::remove_all(m_path, error);
    }

    /// The directory the process was started with, which it keeps when it could not make its own.
    std::filesystem::path shared() const {
        return m_path.empty() ? std::filesystem::temp_directory_path() : m_shared;
    }

private:
    std::filesystem::path m_shared;
    std::filesystem::path m_path;
};

// Constructed before main(), so before any test case asks for the temporary
// directory, and destroyed after every object a test case created.
const PrivateTempDirectory privateTempDirectory;

} // namespace

namespace lc::test {

std::filesystem::path sharedTempDirectory() {
    return privateTempDirectory.shared();
}

} // namespace lc::test
