/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
**********************************************************************/

// The draw order of a drawing (SORTENTSTABLE) pairs an entity with a sort
// handle: the entity is drawn where an entity of that handle would be. The
// sort handle "does not have to point to an entity" (ODA specification,
// SORTENTSTABLE), and AutoCAD writes values no object has: to draw a hatch
// under its boundary it gives the hatch a free handle just after the
// boundary's. The DWG export took every sort handle for a reference to an
// object and refused a drawing with such a value, without saying why.

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iterator>
#include <set>
#include <string>

#include <QCoreApplication>
#include <QString>

#include "drw_objects.h"
#include "lc_containertraverser.h"
#include "lc_dwgadvancedmetadata.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_line.h"
#include "rs_settings.h"

namespace {

void ensureSettings() {
    static int argc = 1;
    static char arg0[] = "librecad_tests";
    static char* argv[] = {arg0, nullptr};
    static QCoreApplication* app = QCoreApplication::instance()
                                       ? QCoreApplication::instance()
                                       : new QCoreApplication(argc, argv);
    static bool ready = [] {
        QCoreApplication::setOrganizationName("LibreCAD");
        QCoreApplication::setApplicationName("LibreCAD-tests");
        RS_Settings::init("LibreCAD", "LibreCAD-tests");
        return true;
    }();
    (void)app;
    (void)ready;
}

struct Format {
    RS2::FormatType m_type;
    const char* m_name;
};

const Format kDwgFormats[] = {
    {RS2::FormatDWG, "dwg-2000"},          {RS2::FormatDWG2004, "dwg-2004"},
    {RS2::FormatDWG2007, "dwg-2007"},      {RS2::FormatDWG2010, "dwg-2010"},
    {RS2::FormatDWG2013, "dwg-2013"},      {RS2::FormatDWG2018, "dwg-2018"}};

constexpr std::uint32_t kLower = 0xC61u;
constexpr std::uint32_t kFree = 0xC62u;   // no object has this handle
constexpr std::uint32_t kUpper = 0xC63u;
constexpr std::uint32_t kTable = 0xC64u;
constexpr std::uint32_t kAboveAll = 0xC70u;   // nor this one, above them all

RS_Line* addLine(RS_Graphic& drawing, std::uint32_t handle, double y) {
    auto* line = new RS_Line(
        &drawing, RS_LineData{RS_Vector{0.0, y}, RS_Vector{10.0, y}});
    line->setSourceHandle(handle);
    drawing.addEntity(line);
    return line;
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DWG export keeps a SORTENTSTABLE sort handle that names no object",
          "[dwg][filter][sortentstable]") {
    ensureSettings();
    const Format dwg = GENERATE(from_range(std::begin(kDwgFormats), std::end(kDwgFormats)));
    // A free value between the handles, as AutoCAD writes it, and one above
    // every handle of the drawing, which the export must not hand out.
    const std::uint32_t freeKey = GENERATE(kFree, kAboveAll);
    CAPTURE(dwg.m_name, freeKey);

    // The upper line is drawn first, at the free value; the lower one where
    // the upper one would be.
    RS_Graphic source;
    source.initForNewDocument();
    addLine(source, kLower, 0.0);
    addLine(source, kUpper, 1.0);
    DRW_SortEntsTable table;
    table.handle = kTable;
    table.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
    table.m_blockOwnerHandle = DRW::DwgModelSpaceBlockRecordHandle;
    table.m_entityHandles = {kLower, kUpper};
    table.m_sortHandles = {kUpper, freeKey};
    source.dwgAdvancedMetadata().addSortEntsTable(table);

    const std::string path =
        (std::filesystem::temp_directory_path() /
         ("lc_dwg_export_sortents_key_" + std::string(dwg.m_name) + ".dwg"))
            .string();
    std::filesystem::remove(path);
    {
        RS_FilterDXFRW filter;
        REQUIRE(filter.fileExport(source, QString::fromStdString(path), dwg.m_type));
    }

    RS_Graphic imported;
    {
        RS_FilterDXFRW filter;
        REQUIRE(filter.fileImport(imported, QString::fromStdString(path),
                                  RS2::FormatDWG));
    }
    std::set<std::uint32_t> entityHandles;
    for (RS_Entity* entity :
         lc::LC_ContainerTraverser{imported, RS2::ResolveNone}.entities()) {
        if (entity != nullptr && entity->sourceHandle() != 0)
            entityHandles.insert(entity->sourceHandle());
    }
    CHECK(entityHandles.count(freeKey) == 0u);

    const auto& tables = imported.dwgAdvancedMetadata().sortEntsTables();
    const auto written = std::find_if(
        tables.begin(), tables.end(),
        [](const auto& candidate) { return candidate.handle == kTable; });
    REQUIRE(written != tables.end());
    REQUIRE(written->entityHandles.size() == 2u);
    REQUIRE(written->sortHandles.size() == 2u);
    const std::uint32_t lower = written->entityHandles[0];
    const std::uint32_t upper = written->entityHandles[1];
    CHECK(entityHandles.count(lower) == 1u);
    CHECK(entityHandles.count(upper) == 1u);
    // The same order: the lower line where the upper one would be, the upper
    // one at the value no object has, which is drawn first.
    CHECK(written->sortHandles[0] == upper);
    CHECK(written->sortHandles[1] == freeKey);
    CHECK(written->sortHandles[1] < written->sortHandles[0]);

    std::filesystem::remove(path);
}
