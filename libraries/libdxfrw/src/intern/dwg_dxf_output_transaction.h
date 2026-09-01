/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 Dongxu Li (github.com/dxli)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 * ********************************************************************************
 */

#ifndef DWG_DXF_OUTPUT_TRANSACTION_H
#define DWG_DXF_OUTPUT_TRANSACTION_H

#include <filesystem>
#include <fstream>
#include <ios>
#include <string>

/// Owns the temporary output and publishes it only after a complete flush.
/// The destination is never opened with truncation, so a failed write cannot
/// destroy the previous file.
class DwgDxfOutputTransaction final {
public:
    DwgDxfOutputTransaction(const std::string& target,
                            std::ios::openmode mode);
    ~DwgDxfOutputTransaction();

    DwgDxfOutputTransaction(const DwgDxfOutputTransaction&) = delete;
    DwgDxfOutputTransaction& operator=(const DwgDxfOutputTransaction&) = delete;

    bool open();
    bool commit();
    void abort() noexcept;

    [[nodiscard]] bool isOpen() const noexcept { return m_stream.is_open(); }
    std::ofstream& stream() noexcept { return m_stream; }

private:
    bool createExclusiveTemporary();
    bool publish();

    std::filesystem::path m_target;
    std::filesystem::path m_temporary;
    std::ios::openmode m_mode;
    std::ofstream m_stream;
    bool m_committed {false};
};

#endif // DWG_DXF_OUTPUT_TRANSACTION_H
