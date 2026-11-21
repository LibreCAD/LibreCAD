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

#include "lc_storepackage.h"

#include <QtGlobal>

#ifdef Q_OS_WIN
#include <vector>
#include <qt_windows.h>
// <appmodel.h> is missing before MinGW-w64 v11; kernel32 exports this.
extern "C" WINBASEAPI LONG WINAPI GetCurrentPackageFullName(UINT32 *packageFullNameLength, PWSTR packageFullName);
#endif

bool LC_StorePackage::runsFromStorePackage() {
#ifdef Q_OS_WIN
    UINT32 length = 0;
    if (GetCurrentPackageFullName(&length, nullptr) != ERROR_INSUFFICIENT_BUFFER) {
        return false; // no package identity
    }
    std::vector<wchar_t> fullName(length);
    if (GetCurrentPackageFullName(&length, fullName.data()) != ERROR_SUCCESS) {
        return false;
    }
    // kernel32.dll does not export GetStagedPackageOrigin, and MinGW-w64
    // before v12 has no import library for it.
    const HMODULE kernelbase = GetModuleHandleW(L"kernelbase.dll");
    using OriginQuery = LONG (WINAPI *)(PCWSTR, int *);
    // void (*)() converts to any function type without -Wcast-function-type
    const auto query = kernelbase == nullptr ? nullptr
        : reinterpret_cast<OriginQuery>(reinterpret_cast<void (*)()>(GetProcAddress(kernelbase, "GetStagedPackageOrigin")));
    // 3 is PackageOrigin_Store: installed from the Microsoft Store.
    int origin = 0;
    return query != nullptr && query(fullName.data(), &origin) == ERROR_SUCCESS && origin == 3;
#else
    return false;
#endif
}
