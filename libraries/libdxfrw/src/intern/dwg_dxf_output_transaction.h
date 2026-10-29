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
#include <functional>
#include <ios>
#include <memory>
#include <ostream>
#include <string>

/// Writes through an exclusively created temporary file and publishes after
/// a complete flush. Stage identity checks detect replacement at the checked
/// boundaries, but do not make pathname verification and rename atomic.
class DwgDxfOutputTransaction final {
public:
    DwgDxfOutputTransaction(const std::string& target,
                            std::ios::openmode mode);
    ~DwgDxfOutputTransaction();

    DwgDxfOutputTransaction(const DwgDxfOutputTransaction&) = delete;
    DwgDxfOutputTransaction& operator=(const DwgDxfOutputTransaction&) = delete;

    bool open();
    bool commit();
    /// Inspect the flushed, closed private file before atomic publication.
    /// Verification runs synchronously and must not change the staged file.
    /// Rejection or an exception aborts the transaction, preserving the target.
    /// The verifier is responsible for semantic and reread validation; this
    /// Path replacement between identity checks and publication remains a
    /// race; this class is not a security boundary against directory mutation.
    using StagedFileVerifier = bool (*)(const std::filesystem::path&, void*);
    bool commitVerified(StagedFileVerifier verifier, void* context);
    void abort() noexcept;

    [[nodiscard]] bool isOpen() const noexcept { return m_open; }
    std::ostream& stream() noexcept { return m_stream; }

private:
    friend struct LC_OutputTransactionTestAccess;
    struct Stage;
    bool createExclusiveTemporary();
    bool stageMatchesPath() const noexcept;
    bool publish();
    bool finishWrite();
    bool finishPublish();

    std::filesystem::path m_target;
    std::filesystem::path m_temporary;
    std::ios::openmode m_mode;
    std::unique_ptr<Stage> m_stage;
    std::ostream m_stream {nullptr};
    bool m_open {false};
    bool m_committed {false};
    bool m_finalizing {false};
    std::function<void(const std::filesystem::path&)> m_testAfterCreate;
};

#endif // DWG_DXF_OUTPUT_TRANSACTION_H
