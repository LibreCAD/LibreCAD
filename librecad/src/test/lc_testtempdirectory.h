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

#ifndef LC_TESTTEMPDIRECTORY_H
#define LC_TESTTEMPDIRECTORY_H

#include <filesystem>

namespace lc::test {

/**
 * A test process has a temporary directory of its own.
 *
 * Tests write fixed file names into the temporary directory, so test processes
 * that run at the same time and share it overwrite each other's files. An
 * executable that lists lc_testtempdirectory.cpp among its sources creates a
 * directory inside the temporary directory before main() runs, makes it what
 * std::filesystem::temp_directory_path() and QDir::tempPath() return, and
 * removes it when the process exits. No test has to ask for it.
 *
 * @return the temporary directory the process was started with, for output
 * that has to outlive the process.
 */
std::filesystem::path sharedTempDirectory();

} // namespace lc::test

#endif
