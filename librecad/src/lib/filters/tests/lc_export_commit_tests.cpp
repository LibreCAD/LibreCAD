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

// An export that fails publishes nothing: no file appears, and a file that is
// already at the destination keeps every byte. Both codecs write to a
// temporary file and move it onto the destination at the end. The write
// callbacks of the filter return nothing, though, and the DXF codec used not
// to hear of a failure that the filter had found by itself: it moved the
// incomplete file onto the user's drawing, and fileExport() returned false
// afterwards.

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "drw_base.h"
#include "drw_objects.h"
#include "lc_dwgadvancedmetadata.h"
#include "lc_filtertestsupport.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_line.h"
#include "rs_point.h"

namespace {

struct Target {
    RS2::FormatType m_format;
    const char* m_name;
};

void fill(RS_Graphic& graphic, const int lines) {
    graphic.initForNewDocument();
    for (int i = 0; i < lines; ++i) {
        graphic.addEntity(new RS_Line(&graphic, RS_Vector(0.0, i), RS_Vector(10.0, i)));
    }
}

int lineCount(const RS_Graphic& graphic) {
    int lines = 0;
    for (const RS_Entity* entity : graphic) {
        lines += entity != nullptr && entity->rtti() == RS2::EntityLine ? 1 : 0;
    }
    return lines;
}

// Each of the next three puts into a drawing something that one write callback
// of the filter refuses to save. The filter only notes the refusal: the
// callbacks return nothing, and the codec is handed no record it would reject.

// Refused in writeBlockRecords(), before anything is streamed: two raw records
// that claim one handle.
void addDuplicateRawHandles(RS_Graphic& graphic) {
    const auto raw = [](const char* handle) {
        DRW_RawDxfObject object;
        object.name = "XRECORD";
        object.handle = 0xAAu;
        object.m_version = DRW::AC1021;
        object.groups.emplace_back(5, std::string{handle});
        object.groups.emplace_back(100, std::string{"AcDbXrecord"});
        object.groups.emplace_back(280, static_cast<std::int32_t>(1));
        return object;
    };
    graphic.dwgAdvancedMetadata().addRawDxfObject(raw("AA"));
    graphic.dwgAdvancedMetadata().addRawDxfObject(raw("aa"));
}

// Refused in writeAppId(), among the tables: data on the APPID record that the
// codec writes by itself.
void addDataOnTheAcadAppId(RS_Graphic& graphic) {
    DRW_AppId acad;
    acad.name = "ACAD";
    acad.xDictHandle = 0x99u;
    graphic.dwgAdvancedMetadata().addAppIdTableEntry(acad);
}

// Refused in writeEntities(): a point whose application data names a typed
// conversion and cannot be read back as one.
void addUnreadableConversion(RS_Graphic& graphic) {
    auto* point = new RS_Point(&graphic, RS_Vector(1.0, 2.0));
    point->setDrwExtData({std::make_shared<DRW_Variant>(1001, std::string("LibreCAD_POINT_EXTRUSION")),
                          std::make_shared<DRW_Variant>(1010, DRW_Coord(1.0, 2.0, 3.0)),
                          std::make_shared<DRW_Variant>(1011, DRW_Coord(0.0, 0.0, 1.0)),
                          std::make_shared<DRW_Variant>(1040, 0.0)});
    graphic.addEntity(point);
}

// Compared here, so that a failure does not print two drawings.
bool sameBytes(const std::filesystem::path& file, const std::string& expected) {
    return lc::test::fileBytes(file) == expected;
}

// Content that no export wrote, put at a destination with plain file output.
// Returns the bytes as they are on disk: a text-mode write differs on Windows.
std::string putExistingContent(const std::filesystem::path& file) {
    {
        std::ofstream out(file);
        out << "0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n";
    }
    return lc::test::fileBytes(file);
}

} // namespace

TEST_CASE("a DXF export that fails in a filter callback publishes nothing",
          "[dxf][filter][write-errors][transaction][export-commit]") {
    lc::test::ensureSettings();
    const lc::test::QuietLog quiet;

    struct Refusal {
        const char* m_callback;
        std::function<void(RS_Graphic&)> m_arrange;
    };
    const std::vector<Refusal> refusals{{"block records", addDuplicateRawHandles},
                                        {"tables", addDataOnTheAcadAppId},
                                        {"entities", addUnreadableConversion}};
    const std::vector<std::string> onlyTheDrawing{"drawing.dxf"};

    for (const Refusal& refusal : refusals) {
        for (const bool existing : {false, true}) {
            CAPTURE(refusal.m_callback, existing);
            const lc::test::OutputDirectory out("failed_dxf_export");
            const auto file = out.file("drawing.dxf");
            const std::string before = existing ? putExistingContent(file) : std::string();
            REQUIRE(before.empty() != existing);

            RS_Graphic graphic;
            graphic.initForNewDocument();
            refusal.m_arrange(graphic);
            RS_FilterDXFRW filter;
            CHECK_FALSE(filter.fileExport(graphic, lc::test::toQString(file), RS2::FormatDXFRW));
            if (existing) {
                CHECK(sameBytes(file, before));
                CHECK(out.entries() == onlyTheDrawing);
            }
            else {
                CHECK(out.entries().empty());
            }
        }
    }

    // The control: a drawing with nothing to refuse replaces what is there.
    const lc::test::OutputDirectory out("failed_dxf_export");
    const auto file = out.file("drawing.dxf");
    const std::string before = putExistingContent(file);
    RS_Graphic graphic;
    graphic.initForNewDocument();
    RS_FilterDXFRW filter;
    CHECK(filter.fileExport(graphic, lc::test::toQString(file), RS2::FormatDXFRW));
    CHECK_FALSE(sameBytes(file, before));
    CHECK(out.entries() == onlyTheDrawing);
}

TEST_CASE("a DXF export the filter refuses leaves the existing file as it was", "[dxf][filter][export-commit]") {
    lc::test::ensureSettings();
    const lc::test::QuietLog quiet;
    for (const Target& target : {Target{RS2::FormatDXFRW2018, "dxf2018"}, Target{RS2::FormatDXFRW, "dxf2007"}}) {
        CAPTURE(target.m_name);
        const lc::test::OutputDirectory out(std::string("refused_onto_") + target.m_name);
        const auto file = out.file("drawing.dxf");

        RS_Graphic saved;
        fill(saved, 1);
        REQUIRE(lc::test::exportDrawing(saved, file, target.m_format));
        const std::string before = lc::test::fileBytes(file);
        REQUIRE(!before.empty());

        // The next save is refused. It meets the file of the first one.
        RS_Graphic refused;
        fill(refused, 2);
        addUnreadableConversion(refused);
        CHECK_FALSE(lc::test::exportDrawing(refused, file, target.m_format));
        CHECK(sameBytes(file, before));
        CHECK(out.entries() == std::vector<std::string>{"drawing.dxf"});

        RS_Graphic reopened;
        REQUIRE(lc::test::importDrawing(reopened, file, target.m_format));
        CHECK(lineCount(reopened) == 1);
    }
}

TEST_CASE("a refused DXF export creates no file", "[dxf][filter][export-commit]") {
    lc::test::ensureSettings();
    const lc::test::QuietLog quiet;
    for (const Target& target : {Target{RS2::FormatDXFRW2018, "dxf2018"}, Target{RS2::FormatDXFRW, "dxf2007"}}) {
        CAPTURE(target.m_name);
        const lc::test::OutputDirectory out(std::string("refused_new_") + target.m_name);

        RS_Graphic refused;
        fill(refused, 2);
        addUnreadableConversion(refused);
        CHECK_FALSE(lc::test::exportDrawing(refused, out.file("drawing.dxf"), target.m_format));
        CHECK(out.entries().empty());

        // The control: without the point the same drawing is saved.
        RS_Graphic saved;
        fill(saved, 2);
        CHECK(lc::test::exportDrawing(saved, out.file("control.dxf"), target.m_format));
        CHECK(out.entries() == std::vector<std::string>{"control.dxf"});
    }
}

// The DWG twin. A refusal reaches the DWG writer through finalizeDwgWrite()
// before anything is moved, so this guards what already works.
TEST_CASE("a refused DWG export creates no file and keeps an existing one", "[dwg][filter][export-commit]") {
    lc::test::ensureSettings();
    const lc::test::QuietLog quiet;
    for (const Target& target : {Target{RS2::FormatDWG2018, "dwg2018"}, Target{RS2::FormatDWG, "dwg2000"}}) {
        CAPTURE(target.m_name);
        const lc::test::OutputDirectory out(std::string("refused_dwg_") + target.m_name);

        RS_Graphic refused;
        fill(refused, 2);
        addUnreadableConversion(refused);
        CHECK_FALSE(lc::test::exportDrawing(refused, out.file("new.dwg"), target.m_format));
        CHECK(out.entries().empty());

        const auto file = out.file("drawing.dwg");
        const std::string before = putExistingContent(file);
        REQUIRE(!before.empty());
        CHECK_FALSE(lc::test::exportDrawing(refused, file, target.m_format));
        CHECK(sameBytes(file, before));
        CHECK(out.entries() == std::vector<std::string>{"drawing.dwg"});

        // The control: without the point the same drawing is saved, to a name
        // of its own.
        RS_Graphic saved;
        fill(saved, 2);
        CHECK(lc::test::exportDrawing(saved, out.file("control.dwg"), target.m_format));
        CHECK(out.entries() == std::vector<std::string>{"control.dwg", "drawing.dwg"});
    }
}
