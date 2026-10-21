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

#ifndef LC_FILTERTESTSUPPORT_H
#define LC_FILTERTESTSUPPORT_H

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <system_error>
#include <vector>

#include <QCoreApplication>
#include <QString>

#include "lc_graphicviewport.h"
#include "rs_debug.h"
#include "rs_document.h"
#include "rs_entity.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_settings.h"

// What the tests of RS_FilterDXFRW share: a quiet log, a directory per test
// case, reading a written DXF back as records, and editing a drawing the way
// the application edits it. Needs Qt and the application library.

namespace lc::test {

/// A QCoreApplication and the settings store of the tests, made once per
/// process; an application another test file created is reused.
inline bool ensureSettings() {
    static int argc = 1;
    static char arg0[] = "librecad_tests";
    static char* argv[] = {arg0, nullptr};
    static QCoreApplication* app = QCoreApplication::instance()
                                       ? QCoreApplication::instance()
                                       : new QCoreApplication(argc, argv);
    static const bool ready = [] {
        QCoreApplication::setOrganizationName("LibreCAD");
        QCoreApplication::setApplicationName("LibreCAD-tests");
        RS_Settings::init("LibreCAD", "LibreCAD-tests");
        return true;
    }();
    (void)app;
    return ready;
}

/**
 * Keeps the log to warnings for the length of a scope. RS_Debug starts at the
 * debugging level, at which every border calculation of every letter is
 * printed and the filter switches the libdxfrw trace on for each file it
 * reads. Warnings stay: they name the reason when an export is refused.
 */
class QuietLog {
public:
    QuietLog() : m_previous(RS_DEBUG->getLevel()) { RS_DEBUG->setLevel(RS_Debug::D_WARNING); }
    ~QuietLog() { RS_DEBUG->setLevel(m_previous); }
    QuietLog(const QuietLog&) = delete;
    QuietLog& operator=(const QuietLog&) = delete;

private:
    RS_Debug::RS_DebugLevel m_previous;
};

/**
 * An empty directory of its own for one test case, inside the temporary
 * directory of the process, removed with its content when the case ends.
 * Give every export its own file name in it; do not remove a file to make
 * room for an export.
 */
class OutputDirectory {
public:
    explicit OutputDirectory(const std::string& name) {
        std::error_code error;
        m_path = std::filesystem::temp_directory_path(error) / name;
        std::filesystem::remove_all(m_path, error);
        std::filesystem::create_directories(m_path, error);
    }
    ~OutputDirectory() {
        std::error_code error;
        std::filesystem::remove_all(m_path, error);
    }
    OutputDirectory(const OutputDirectory&) = delete;
    OutputDirectory& operator=(const OutputDirectory&) = delete;

    std::filesystem::path file(const std::string& name) const { return m_path / name; }

    /// Names of everything in the directory, dot files included, sorted.
    std::vector<std::string> entries() const {
        std::vector<std::string> names;
        std::error_code error;
        for (std::filesystem::directory_iterator it(m_path, error), end; !error && it != end; it.increment(error)) {
            names.push_back(it->path().filename().string());
        }
        std::sort(names.begin(), names.end());
        return names;
    }

private:
    std::filesystem::path m_path;
};

inline QString toQString(const std::filesystem::path& path) {
    return QString::fromStdWString(path.wstring());
}

inline std::string fileBytes(const std::filesystem::path& file) {
    std::ifstream in(file, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

/// Opens a drawing with a filter of its own, as RS_FileIO does.
inline bool importDrawing(RS_Graphic& graphic, const std::filesystem::path& file, const RS2::FormatType format) {
    RS_FilterDXFRW filter;
    return filter.fileImport(graphic, toQString(file), format);
}

/// Copies a file a test wrote to $LIBRECAD_DWG_FIXTURE_DIR, when that is set,
/// for a look with another program: <case directory>_<file name>, because the
/// cases name their files alike. Never fails the test.
inline void keepForInspection(const std::filesystem::path& file) {
    const char* directory = std::getenv("LIBRECAD_DWG_FIXTURE_DIR");
    if (directory == nullptr || *directory == '\0') {
        return;
    }
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error) {
        return;
    }
    const std::string name = file.parent_path().filename().string() + "_" + file.filename().string();
    std::filesystem::copy_file(file, std::filesystem::path(directory) / name,
                               std::filesystem::copy_options::overwrite_existing, error);
}

/// Saves a drawing with a filter of its own.
inline bool exportDrawing(RS_Graphic& graphic, const std::filesystem::path& file, const RS2::FormatType format) {
    RS_FilterDXFRW filter;
    const bool exported = filter.fileExport(graphic, toQString(file), format);
    if (exported) {
        keepForInspection(file);
    }
    return exported;
}

struct DxfGroup {
    int m_code = 0;
    std::string m_value;
};

/// One record of an ASCII DXF file: its groups from the "0" that starts it.
struct DxfRecord {
    std::vector<DxfGroup> m_groups;

    const std::string& type() const { return m_groups.front().m_value; }
    /// Index of the embedded-object marker (101; 100 in files LibreCAD wrote
    /// before), m_groups.size() without one.
    std::size_t embeddedAt() const {
        for (std::size_t i = 0; i < m_groups.size(); ++i) {
            const DxfGroup& group = m_groups[i];
            if ((group.m_code == 101 || group.m_code == 100) && group.m_value == "Embedded Object") {
                return i;
            }
        }
        return m_groups.size();
    }
    int embeddedMarker() const { return embeddedAt() < m_groups.size() ? m_groups[embeddedAt()].m_code : 0; }
    /// Values of a group among the record's own groups, before the embedded object.
    std::vector<std::string> own(const int code) const { return values(code, 0, embeddedAt()); }
    /// Values of a group inside the embedded object.
    std::vector<std::string> embedded(const int code) const {
        return values(code, embeddedAt() + 1, m_groups.size());
    }
    /// The group codes in file order, for checks of the layout.
    std::vector<int> codes() const {
        std::vector<int> all;
        for (const DxfGroup& group : m_groups) {
            all.push_back(group.m_code);
        }
        return all;
    }

private:
    std::vector<std::string> values(const int code, const std::size_t from, const std::size_t to) const {
        std::vector<std::string> found;
        for (std::size_t i = from; i < to && i < m_groups.size(); ++i) {
            if (m_groups[i].m_code == code) {
                found.push_back(m_groups[i].m_value);
            }
        }
        return found;
    }
};

/// Every record of a type in file order; every record when the type is empty.
/// Text groups (1, 3, 1000) keep their blanks, other values are trimmed:
/// AutoCAD pads numbers.
inline std::vector<DxfRecord> dxfRecords(const std::filesystem::path& file, const std::string& type = {}) {
    std::vector<DxfRecord> records;
    std::ifstream in(file);
    std::string codeLine;
    std::string value;
    bool wanted = false;
    while (std::getline(in, codeLine) && std::getline(in, value)) {
        if (!value.empty() && value.back() == '\r') {
            value.pop_back();
        }
        const int code = static_cast<int>(std::strtol(codeLine.c_str(), nullptr, 10));
        if (code != 1 && code != 3 && code != 1000) {
            const auto first = value.find_first_not_of(" \t");
            const auto last = value.find_last_not_of(" \t");
            value = first == std::string::npos ? std::string() : value.substr(first, last - first + 1);
        }
        if (code == 0) {
            wanted = type.empty() || value == type;
            if (wanted) {
                records.emplace_back();
            }
        }
        if (wanted) {
            records.back().m_groups.push_back({code, value});
        }
    }
    return records;
}

/// Record types of the ENTITIES section, in file order.
inline std::vector<std::string> dxfEntitySequence(const std::filesystem::path& file) {
    std::vector<std::string> types;
    bool inEntities = false;
    for (const DxfRecord& record : dxfRecords(file)) {
        if (record.type() == "SECTION") {
            const auto names = record.own(2);
            inEntities = !names.empty() && names.front() == "ENTITIES";
        }
        else if (record.type() == "ENDSEC") {
            inEntities = false;
        }
        else if (inEntities) {
            types.push_back(record.type());
        }
    }
    return types;
}

/// Handles (group 5) that more than one record of the file carries.
inline std::vector<std::string> duplicateDxfHandles(const std::filesystem::path& file) {
    std::map<std::string, int> uses;
    for (const DxfRecord& record : dxfRecords(file)) {
        if (record.type() == "SECTION") {
            continue;   // the header section holds $HANDSEED as a group 5
        }
        for (const std::string& handle : record.own(5)) {
            ++uses[handle];
        }
    }
    std::vector<std::string> twice;
    for (const auto& [handle, count] : uses) {
        if (count > 1) {
            twice.push_back(handle);
        }
    }
    return twice;
}

/// True when the lists are as long as each other and each pair is the same
/// number (AutoCAD writes 1.0 where LibreCAD writes 1) or, where a value is
/// no number, the same text. QString::toDouble() does not follow the locale.
inline bool sameValues(const std::vector<std::string>& expected, const std::vector<std::string>& actual) {
    if (expected.size() != actual.size()) {
        return false;
    }
    for (std::size_t i = 0; i < expected.size(); ++i) {
        bool expectedIsNumber = false;
        bool actualIsNumber = false;
        const double a = QString::fromStdString(expected[i]).toDouble(&expectedIsNumber);
        const double b = QString::fromStdString(actual[i]).toDouble(&actualIsNumber);
        if (expectedIsNumber && actualIsNumber) {
            if (std::abs(a - b) > 1e-9 * std::max(1.0, std::abs(a))) {
                return false;
            }
        }
        else if (expected[i] != actual[i]) {
            return false;
        }
    }
    return true;
}

/**
 * A drawing with what the editing tools need besides it, a viewport. Every
 * change goes through RS_Document::undoableModify(), as every tool's does,
 * and keeps layer and pen, as the tools do by default.
 */
struct EditableDrawing {
    LC_GraphicViewport m_viewport;   // declared first: outlives the drawing's undo list
    RS_Graphic m_graphic;

    template <typename Operation>
    void modify(Operation&& operation) {
        m_graphic.undoableModify(&m_viewport, [&](LC_DocumentModificationBatch& ctx) {
            operation(ctx);
            ctx.dontSetActiveLayerAndPen();
            return true;
        });
    }

    /// Replaces an entity by a changed clone in one undo step. The original
    /// stays in the container, flagged deleted. Returns the clone.
    template <typename Entity, typename Change>
    const Entity* edit(const Entity* original, Change&& change) {
        auto* clone = static_cast<Entity*>(original->clone());
        change(*clone);
        clone->update();
        clone->calculateBorders();
        modify([&](LC_DocumentModificationBatch& ctx) { ctx.replace(const_cast<Entity*>(original), clone); });
        return clone;
    }

    /// Replaces several entities, each by a changed clone, in one undo step:
    /// a block reference together with its attribute texts. Returns the
    /// clones in the order of the originals.
    template <typename Change>
    std::vector<const RS_Entity*> editAll(const std::vector<const RS_Entity*>& originals, Change&& change) {
        const std::vector<RS_Entity*> clones = changedClones(originals, change);
        modify([&](LC_DocumentModificationBatch& ctx) {
            for (std::size_t i = 0; i < originals.size(); ++i) {
                ctx.replace(const_cast<RS_Entity*>(originals[i]), clones[i]);
            }
        });
        return {clones.begin(), clones.end()};
    }

    /// Adds changed copies in one undo step and leaves the originals.
    template <typename Change>
    std::vector<const RS_Entity*> copyAll(const std::vector<const RS_Entity*>& originals, Change&& change) {
        const std::vector<RS_Entity*> clones = changedClones(originals, change);
        modify([&](LC_DocumentModificationBatch& ctx) {
            for (RS_Entity* clone : clones) {
                ctx.add(clone);
            }
        });
        return {clones.begin(), clones.end()};
    }

    /// Erases an entity in one undo step, as the delete tool does.
    void erase(const RS_Entity* entity) {
        modify([&](LC_DocumentModificationBatch& ctx) { ctx.remove(const_cast<RS_Entity*>(entity)); });
    }

    /// What the drawing shows: entities of a type that are not flagged deleted.
    template <typename Entity>
    std::vector<const Entity*> live(const RS2::EntityType type) {
        std::vector<const Entity*> found;
        for (RS_Entity* e : m_graphic) {
            if (e != nullptr && e->rtti() == type && !e->isDeleted()) {
                found.push_back(static_cast<const Entity*>(e));
            }
        }
        return found;
    }

private:
    template <typename Change>
    static std::vector<RS_Entity*> changedClones(const std::vector<const RS_Entity*>& originals, Change&& change) {
        std::vector<RS_Entity*> clones;
        for (const RS_Entity* original : originals) {
            RS_Entity* clone = original->clone();
            change(*clone);
            clone->update();
            clone->calculateBorders();
            clones.push_back(clone);
        }
        return clones;
    }
};

} // namespace lc::test

#endif
