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

#include <cstdlib>
#include <random>
#include <string>
#include <system_error>

namespace {

class PrivateTempDirectory final {
public:
    PrivateTempDirectory() {
        std::error_code error;
        const std::filesystem::path shared = std::filesystem::temp_directory_path(error);
        std::random_device random;
        bool created = false;
        // create_directory() is false, without an error, for a name that is taken
        while (!created && !error) {
            m_path = shared / ("librecad-tests-" + std::to_string(random()));
            created = std::filesystem::create_directory(m_path, error);
        }
        if (!created) {
            m_path.clear();
            return;
        }
        // the variable the standard library and Qt read the directory from
#ifdef _WIN32
        _wputenv((L"TMP=" + m_path.native()).c_str());
#else
        setenv("TMPDIR", m_path.c_str(), 1);
#endif
    }

    ~PrivateTempDirectory() {
        if (!m_path.empty()) {
            std::error_code error;
            std::filesystem::remove_all(m_path, error);
        }
    }

    /// Empty when the directory could not be created.
    const std::filesystem::path& path() const { return m_path; }

private:
    std::filesystem::path m_path;
};

// Constructed before main(), so before any test case asks for the temporary
// directory, and destroyed after every object a test case created.
const PrivateTempDirectory privateTempDirectory;

} // namespace

namespace lc::test {

std::filesystem::path sharedTempDirectory() {
    const std::filesystem::path& path = privateTempDirectory.path();
    return path.empty() ? std::filesystem::temp_directory_path() : path.parent_path();
}

} // namespace lc::test
