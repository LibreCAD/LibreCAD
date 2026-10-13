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

// The attributes of a block reference through RS_FilterDXFRW. The drawing has
// no attribute of its own: an ATTRIB is imported as a text next to the block
// reference, a multi-line one (AutoCAD 2018 and later) as a multi-line text,
// and the export puts the texts back under their block reference.
//
// The fixtures are written here, in the group order and with the values that
// AutoCAD writes; no drawing of another program is copied. Every case has a
// directory of its own and gives every export a file name of its own.

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

#include <QString>

#include "lc_filtertestsupport.h"
#include "lc_sourcefonts.h"
#include "rs_block.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_mtext.h"
#include "rs_text.h"

namespace {

using lc::test::DxfRecord;
using lc::test::EditableDrawing;

// What every case starts with, in this order: settings, a quiet log, fonts
// with glyphs, a directory of its own.
struct Bench {
    explicit Bench(const std::string& name) : m_out(name) {}
    const bool m_settings = lc::test::ensureSettings();
    lc::test::QuietLog m_quiet;
    lc::test::SourceFonts m_fonts;
    lc::test::OutputDirectory m_out;
};

struct Target {
    RS2::FormatType m_format;
    const char* m_name;     // also the stem of the output file name
    const char* m_suffix;
    bool m_dxf;
    bool m_embedsMText;     // R2018 only
};
const Target kTargets[] = {
    {RS2::FormatDXFRW2018, "dxf2018", ".dxf", true, true},  {RS2::FormatDXFRW, "dxf2007", ".dxf", true, false},
    {RS2::FormatDXFRW2000, "dxf2000", ".dxf", true, false}, {RS2::FormatDWG2018, "dwg2018", ".dwg", false, true},
    {RS2::FormatDWG2013, "dwg2013", ".dwg", false, false},  {RS2::FormatDWG2010, "dwg2010", ".dwg", false, false},
    {RS2::FormatDWG2007, "dwg2007", ".dwg", false, false},  {RS2::FormatDWG2004, "dwg2004", ".dwg", false, false},
    {RS2::FormatDWG, "dwg2000", ".dwg", false, false},
};

const Target& target(const std::string& name) {
    const auto found =
        std::find_if(std::begin(kTargets), std::end(kTargets), [&](const Target& t) { return name == t.m_name; });
    REQUIRE(found != std::end(kTargets));
    return *found;
}

// A block reference with one attribute, in the group order and with the
// values AutoCAD 2018 writes for a multi-line attribute. Top left at (1,4),
// height 9: the attribute's own first point is the baseline start (1,-5), its
// alignment point (1,4), 74 = 3.
struct AttributeDxf {
    bool m_handles = false;                         // 5 on INSERT 1A0, ATTRIB 1A1, SEQEND 1A2
    std::string m_blockContent;                     // records inside the block, e.g. attdefDxf()
    std::string m_insertGroups = "10\n0\n20\n0\n30\n0\n";
    std::string m_firstPoint = "10\n1\n20\n-5\n30\n0\n";
    std::string m_height = "9";
    std::string m_value;                            // empty, as AutoCAD leaves it
    std::string m_textExtra;                        // after group 1 of the attribute itself: 50, 41, 51, 72
    std::string m_alignmentPoint = "11\n1\n21\n4\n31\n0\n";
    std::string m_textTail;                         // after the first alignment point: 210 220 230
    std::string m_tag = "LABEL";
    std::string m_verticalJustification = "3";
    std::string m_typeGroups = "71\n2\n72\n0\n";    // the attribute type; some files have none
    std::string m_marker = "101";                   // "100": what LibreCAD wrote before; "": a single-line attribute
    std::string m_mtextPoint = "10\n1\n20\n4\n30\n0\n";
    std::string m_mtextWidth = "0.0";
    std::string m_attachment = "1";
    std::string m_direction = "5";
    std::string m_text = "my text\\Psecond line";
    std::string m_style;                            // group 7 of the attribute and of its text; none: Standard
    std::string m_mtextExtra;                       // between text/style and 73: 210.., 11 21 31, 50
    std::string m_spacingStyle = "1";
    std::string m_spacingFactor = "1.0";
    std::string m_mtextTail;                        // after 44: 90 63 45 441, then application data

    // The values of a richer attribute: font codes, a width, a style, exact
    // spacing, a background and a stored plain value.
    static AttributeDxf rich() {
        AttributeDxf f;
        f.m_value = "fallback";
        f.m_mtextWidth = "80";
        f.m_direction = "1";
        f.m_text = "{\\fArial;ABC}\\PDEF";
        f.m_style = "iso3098";
        f.m_spacingStyle = "2";
        f.m_mtextTail = "90\n1\n63\n3\n45\n1.5\n441\n0\n";
        return f;
    }

    std::string str() const {
        const std::string style7 = m_style.empty() ? std::string() : "7\n" + m_style + "\n";
        std::string dxf = "0\nSECTION\n2\nBLOCKS\n0\nBLOCK\n8\n0\n2\nTITLE\n70\n";
        dxf += m_blockContent.empty() ? "0" : "2";
        dxf += "\n10\n0\n20\n0\n3\nTITLE\n" + m_blockContent + "0\nENDBLK\n8\n0\n0\nENDSEC\n"
               "0\nSECTION\n2\nENTITIES\n0\nINSERT\n";
        dxf += m_handles ? "5\n1A0\n" : "";
        dxf += "100\nAcDbEntity\n8\n0\n100\nAcDbBlockReference\n66\n1\n2\nTITLE\n" + m_insertGroups;
        dxf += "0\nATTRIB\n";
        dxf += m_handles ? "5\n1A1\n" : "";
        dxf += "100\nAcDbEntity\n8\n0\n100\nAcDbText\n" + m_firstPoint + "40\n" + m_height + "\n1\n" + m_value + "\n"
               + m_textExtra + style7 + m_alignmentPoint + m_textTail;
        dxf += "100\nAcDbAttribute\n280\n0\n2\n" + m_tag + "\n70\n0\n74\n" + m_verticalJustification + "\n280\n1\n";
        if (!m_marker.empty()) {
            dxf += m_typeGroups + m_alignmentPoint;
            dxf += m_marker + "\nEmbedded Object\n" + m_mtextPoint + "40\n" + m_height + "\n41\n" + m_mtextWidth
                   + "\n46\n0.0\n71\n" + m_attachment + "\n72\n" + m_direction + "\n1\n" + m_text + "\n" + style7
                   + m_mtextExtra + "73\n" + m_spacingStyle + "\n44\n" + m_spacingFactor + "\n" + m_mtextTail;
        }
        dxf += "0\nSEQEND\n";
        dxf += m_handles ? "5\n1A2\n" : "";
        dxf += "100\nAcDbEntity\n8\n0\n0\nENDSEC\n0\nEOF\n";
        return dxf;
    }
};

// A single-line attribute: no embedded object, the value in the attribute's
// own group 1, left on the baseline at (1,4).
AttributeDxf plainAttributeDxf(const std::string& value) {
    AttributeDxf f;
    f.m_marker.clear();
    f.m_value = value;
    f.m_firstPoint = "10\n1\n20\n4\n30\n0\n";
    f.m_alignmentPoint.clear();
    f.m_verticalJustification = "0";
    return f;
}

// A multi-line attribute definition for the block, in AutoCAD's group order:
// as the attribute, with the prompt before the tag and type 4. AutoCAD leaves
// the definition's own value empty too. This one has a value, which is what
// the drawing shows of a definition, and a height and a width that are not
// those of its MTEXT, so that a reader that takes the groups of the embedded
// object for the definition's own is seen.
std::string attdefDxf() {
    return "0\nATTDEF\n100\nAcDbEntity\n8\n0\n100\nAcDbText\n10\n2\n20\n-1.5\n30\n0\n40\n2.5\n1\nDEFAULT\n"
           "11\n2\n21\n1\n31\n0\n"
           "100\nAcDbAttributeDefinition\n280\n0\n3\nwhat the label says\n2\nLABEL\n70\n0\n74\n3\n280\n1\n"
           "71\n4\n72\n0\n11\n2\n21\n1\n31\n0\n"
           "101\nEmbedded Object\n10\n2\n20\n1\n30\n0\n40\n7\n41\n0.0\n46\n0.0\n71\n1\n72\n5\n"
           "1\ndefault text\\Psecond line\n73\n1\n44\n1.0\n";
}

// A drawing with one MTEXT that belongs to no block reference.
std::string standaloneMTextDxf(const std::string& spacingStyle) {
    return "0\nSECTION\n2\nENTITIES\n0\nMTEXT\n100\nAcDbEntity\n8\n0\n100\nAcDbMText\n"
           "10\n1\n20\n4\n30\n0\n40\n9\n41\n80\n71\n1\n72\n5\n1\nmy text\\Psecond line\n73\n"
           + spacingStyle + "\n44\n1.0\n0\nENDSEC\n0\nEOF\n";
}

// Writes a DXF text to <name>.dxf of the case and opens it. Returns the file.
std::filesystem::path loadDxf(const Bench& bench, const std::string& dxf, EditableDrawing& drawing,
                              const std::string& name = "source") {
    const std::filesystem::path file = bench.m_out.file(name + ".dxf");
    {
        std::ofstream out(file, std::ios::binary);
        out << dxf;
    }
    REQUIRE(lc::test::importDrawing(drawing.m_graphic, file, RS2::FormatDXFRW2018));
    return file;
}

// The one entity of a type that the drawing shows.
template <typename Entity>
const Entity* only(EditableDrawing& drawing, const RS2::EntityType type) {
    const std::vector<const Entity*> found = drawing.live<Entity>(type);
    REQUIRE(found.size() == 1);
    return found.front();
}

// The text an attribute was imported as: the one text of the drawing, of
// either kind.
const RS_Entity* attributeText(EditableDrawing& drawing) {
    std::vector<const RS_Entity*> found;
    for (const RS_Text* text : drawing.live<RS_Text>(RS2::EntityText)) {
        found.push_back(text);
    }
    for (const RS_MText* text : drawing.live<RS_MText>(RS2::EntityMText)) {
        found.push_back(text);
    }
    REQUIRE(found.size() == 1);
    return found.front();
}

// Opens the fixture. With handles it also makes sure that the drawing knows
// them, so that no check of a handle passes on zeros.
std::filesystem::path load(const Bench& bench, const AttributeDxf& fixture, EditableDrawing& drawing,
                           const std::string& name = "source") {
    const std::filesystem::path file = loadDxf(bench, fixture.str(), drawing, name);
    if (fixture.m_handles) {
        REQUIRE(only<RS_Insert>(drawing, RS2::EntityInsert)->sourceHandle() == 0x1A0u);
        REQUIRE(attributeText(drawing)->sourceHandle() == 0x1A1u);
    }
    return file;
}

// Saves the drawing to a file of the case named after the variant and the
// target. Returns the file; the export has to succeed.
std::filesystem::path save(const Bench& bench, EditableDrawing& drawing, const std::string& variant,
                           const Target& to) {
    const std::filesystem::path file = bench.m_out.file(variant + "_" + to.m_name + to.m_suffix);
    CAPTURE(file.filename().string());
    REQUIRE(lc::test::exportDrawing(drawing.m_graphic, file, to.m_format));
    return file;
}

// The one record of a type in a written DXF.
DxfRecord onlyRecord(const std::filesystem::path& file, const std::string& type) {
    const std::vector<DxfRecord> records = lc::test::dxfRecords(file, type);
    CAPTURE(file.filename().string(), type);
    REQUIRE(records.size() == 1);
    return records.front();
}

// What a text keeps of its ATTRIB beyond what a text can hold is application
// data on the text, under names that start like this. It is for the export
// to read and never belongs in a file.
const char* const kPrivateApplication = "LibreCAD_INSERT_ATTRIB";

// True when a record of the file has application data under such a name.
bool hasPrivateAttributeData(const std::filesystem::path& file) {
    for (const DxfRecord& record : lc::test::dxfRecords(file)) {
        for (const lc::test::DxfGroup& group : record.m_groups) {
            if (group.m_code == 1001 && group.m_value.rfind(kPrivateApplication, 0) == 0) {
                return true;
            }
        }
    }
    return false;
}

// What holds for every DXF written from a drawing that was opened from a
// DXF: no handle twice, and the private names nowhere, not even in the table
// of applications.
void checkWrittenDxf(const std::filesystem::path& file) {
    CAPTURE(file.filename().string());
    CHECK(lc::test::duplicateDxfHandles(file).empty());
    CHECK(lc::test::fileBytes(file).find(kPrivateApplication) == std::string::npos);
}

// Where the MTEXT sits in the record of a multi-line attribute, as AutoCAD
// writes it and ezdxf finds it: the attribute type and the copy of the
// alignment point, the marker 101, the MTEXT from its insertion point to its
// direction and then its text, and the application data at the very end,
// where it is the attribute's. The groups in between are not pinned here.
void checkEmbeddedMTextPlace(const DxfRecord& record) {
    const std::vector<int> codes = record.codes();
    const std::size_t at = record.embeddedAt();
    REQUIRE(at < codes.size());
    CHECK(record.embeddedMarker() == 101);
    CHECK(std::count_if(record.m_groups.begin(), record.m_groups.end(),
                        [](const lc::test::DxfGroup& group) { return group.m_value == "Embedded Object"; })
          == 1);
    // The codes of count groups from an index on.
    const auto codesFrom = [&codes](const std::size_t first, const std::size_t count) {
        const auto begin = codes.begin() + static_cast<std::ptrdiff_t>(first);
        return std::vector<int>(begin, begin + static_cast<std::ptrdiff_t>(count));
    };

    REQUIRE(at >= 5);
    CHECK(codesFrom(at - 5, 5) == std::vector<int>{71, 72, 11, 21, 31});
    CHECK(record.m_groups[at - 5].m_value == "2");
    CHECK(record.m_groups[at - 4].m_value == "0");

    REQUIRE(codes.size() >= at + 10);
    CHECK(codesFrom(at + 1, 8) == std::vector<int>{10, 20, 30, 40, 41, 46, 71, 72});
    std::size_t text = at + 9;
    while (text + 1 < codes.size() && codes[text] == 3) {
        ++text;
    }
    CHECK(codes[text] == 1);

    const auto firstApplicationData = std::find_if(codes.begin(), codes.end(), [](const int code) {
        return code >= 1000;
    });
    CHECK(std::all_of(firstApplicationData, codes.end(), [](const int code) { return code >= 1000; }));
    CHECK(static_cast<std::size_t>(firstApplicationData - codes.begin()) > at);
}

// Everything the drawing holds of a multi-line text, and where the text ends
// up on it: used after a save and a reopen, and after an edit of one
// property, which must not move another.
void checkSameMText(const RS_MText& expected, const RS_MText& actual) {
    CHECK(actual.getText().toStdString() == expected.getText().toStdString());
    CHECK(actual.getHeight() == Catch::Approx(expected.getHeight()).margin(1e-9));
    CHECK(actual.getWidth() == Catch::Approx(expected.getWidth()).margin(1e-9));
    CHECK(actual.getHAlign() == expected.getHAlign());
    CHECK(actual.getVAlign() == expected.getVAlign());
    CHECK(actual.getDrawingDirection() == expected.getDrawingDirection());
    CHECK(actual.getLineSpacingStyle() == expected.getLineSpacingStyle());
    CHECK(actual.getLineSpacingFactor() == Catch::Approx(expected.getLineSpacingFactor()).margin(1e-9));
    CHECK(actual.getInsertionPoint().distanceTo(expected.getInsertionPoint()) < 1e-9);
    CHECK(actual.getAngle() == Catch::Approx(expected.getAngle()).margin(1e-9));
    CAPTURE(actual.getStyle().toStdString(), expected.getStyle().toStdString());
    CHECK(actual.getStyle().compare(expected.getStyle(), Qt::CaseInsensitive) == 0);
    CHECK(actual.getMin().distanceTo(expected.getMin()) < 1e-6);
    CHECK(actual.getMax().distanceTo(expected.getMax()) < 1e-6);
}

// The groups of an untouched multi-line attribute that a save has to repeat.
// Of the attribute itself: value, tag, first point, height. Of its MTEXT:
// text, insertion point, height, width, attachment, direction, line spacing,
// and the style and the background where the source has them. Numbers are
// compared as numbers: AutoCAD writes 1.0 where LibreCAD writes 1.
void checkRepeatsSource(const DxfRecord& source, const std::filesystem::path& written) {
    const DxfRecord record = onlyRecord(written, "ATTRIB");
    CAPTURE(written.filename().string());
    CHECK(record.embeddedMarker() == 101);
    for (const int code : {1, 2, 10, 20, 40}) {
        CAPTURE(code);
        CHECK(lc::test::sameValues(source.own(code), record.own(code)));
    }
    for (const int code : {1, 10, 20, 40, 41, 71, 72, 73, 44}) {
        CAPTURE(code);
        CHECK(lc::test::sameValues(source.embedded(code), record.embedded(code)));
    }
    for (const int code : {7, 90, 45, 63}) {
        CAPTURE(code);
        if (!source.embedded(code).empty()) {
            CHECK(lc::test::sameValues(source.embedded(code), record.embedded(code)));
        }
    }
}

// A drawing of AutoCAD's own that is not in the repository; empty when the
// directory of such drawings is not given or does not have it.
std::filesystem::path externalSample(const char* name) {
    const char* directory = std::getenv("LIBRECAD_EXTERNAL_DWG_FIXTURE_DIR");
    if (directory == nullptr || *directory == '\0') {
        return {};
    }
    const std::filesystem::path file = std::filesystem::path(directory) / name;
    std::error_code error;
    return std::filesystem::is_regular_file(file, error) ? file : std::filesystem::path();
}

} // namespace

TEST_CASE("an untouched multi-line attribute is written back as it was read",
          "[attribute-export][attrib][filter][dxf][dwg][keep]") {
    const Bench bench("attr_untouched");
    AttributeDxf withHandles;
    withHandles.m_handles = true;
    struct Preset {
        const char* m_name;
        AttributeDxf m_fixture;
    };
    const std::vector<Preset> presets{
        {"autocad", AttributeDxf{}}, {"rich", AttributeDxf::rich()}, {"handles", withHandles}};

    for (const Preset& preset : presets) {
        CAPTURE(preset.m_name);
        EditableDrawing drawing;
        const DxfRecord source = onlyRecord(load(bench, preset.m_fixture, drawing, preset.m_name), "ATTRIB");
        const RS_MText* imported = only<RS_MText>(drawing, RS2::EntityMText);

        const auto direct = save(bench, drawing, preset.m_name, target("dxf2018"));
        checkWrittenDxf(direct);
        checkRepeatsSource(source, direct);
        EditableDrawing reopened;
        REQUIRE(lc::test::importDrawing(reopened.m_graphic, direct, RS2::FormatDXFRW2018));
        checkSameMText(*imported, *only<RS_MText>(reopened, RS2::EntityMText));

        // The same through a DWG: its content is read here as the drawing it
        // opens as, and as the DXF that drawing is saved to.
        const auto dwg = save(bench, drawing, preset.m_name, target("dwg2018"));
        EditableDrawing fromDwg;
        REQUIRE(lc::test::importDrawing(fromDwg.m_graphic, dwg, RS2::FormatDWG2018));
        checkSameMText(*imported, *only<RS_MText>(fromDwg, RS2::EntityMText));
        const auto viaDwg = save(bench, fromDwg, std::string(preset.m_name) + "_dwg2018", target("dxf2018"));
        // The DWG export registers every application name it finds on the
        // entities before the private data is taken off them, so the DWG
        // lists the two private names, without data, and the drawing opened
        // from it keeps the list. No record may have the data.
        CHECK(lc::test::duplicateDxfHandles(viaDwg).empty());
        CHECK_FALSE(hasPrivateAttributeData(viaDwg));
        checkRepeatsSource(source, viaDwg);
    }
}

TEST_CASE("an untouched attribute at 75 and 150 degrees is not taken for edited",
          "[attribute-export][attrib][filter][dxf][keep]") {
    const Bench bench("attr_rotated_untouched");
    for (const char* degrees : {"75", "150"}) {
        CAPTURE(degrees);
        AttributeDxf fixture = AttributeDxf::rich();
        fixture.m_mtextExtra = std::string("50\n") + degrees + "\n";
        EditableDrawing drawing;
        load(bench, fixture, drawing, std::string("source_") + degrees);
        const RS_MText* imported = only<RS_MText>(drawing, RS2::EntityMText);
        const double angle = std::stod(degrees) * M_PI / 180.0;
        CHECK(imported->getAngle() == Catch::Approx(angle).margin(1e-9));

        // The angle of the drawing is the file's degrees turned into radians,
        // and turned back it is not always the same number: 75 comes back
        // as 75.00000000000001. An attribute taken for edited gets the
        // measured size of its text as groups 42 and 43; a text taken for
        // edited is written anew from the drawing, without its font code.
        const auto out = save(bench, drawing, degrees, target("dxf2018"));
        checkWrittenDxf(out);
        const DxfRecord record = onlyRecord(out, "ATTRIB");
        CHECK(record.embedded(1) == std::vector<std::string>{"{\\fArial;ABC}\\PDEF"});
        CHECK(record.embedded(42).empty());
        CHECK(record.embedded(43).empty());

        EditableDrawing reopened;
        REQUIRE(lc::test::importDrawing(reopened.m_graphic, out, RS2::FormatDXFRW2018));
        checkSameMText(*imported, *only<RS_MText>(reopened, RS2::EntityMText));
    }
}

TEST_CASE("a drawing with a multi-line attribute saves to DWG 2010 and 2013",
          "[attribute-export][attrib][filter][dwg][keep]") {
    const Bench bench("attr_dwg2010_2013");
    struct Preset {
        const char* m_name;
        AttributeDxf m_fixture;
    };
    const std::vector<Preset> presets{{"autocad", AttributeDxf{}}, {"rich", AttributeDxf::rich()}};
    for (const Preset& preset : presets) {
        for (const char* format : {"dwg2013", "dwg2010"}) {
            CAPTURE(preset.m_name, format);
            EditableDrawing drawing;
            load(bench, preset.m_fixture, drawing, std::string(preset.m_name) + "_" + format + "_source");
            REQUIRE(attributeText(drawing)->rtti() == RS2::EntityMText);
            const auto out = save(bench, drawing, preset.m_name, target(format));

            // These formats have no MTEXT in an attribute: it comes back as a
            // text of one line, next to its block reference.
            EditableDrawing reopened;
            REQUIRE(lc::test::importDrawing(reopened.m_graphic, out, target(format).m_format));
            CHECK(reopened.live<RS_Insert>(RS2::EntityInsert).size() == 1);
            CHECK(attributeText(reopened)->rtti() == RS2::EntityText);
        }
    }
}

TEST_CASE("an attribute keeps its text style name through DWG 2000",
          "[attribute-export][attrib][filter][dwg][keep]") {
    const Bench bench("attr_style_dwg2000");
    EditableDrawing drawing;
    load(bench, AttributeDxf::rich(), drawing);
    REQUIRE(only<RS_MText>(drawing, RS2::EntityMText)->getStyle().compare("iso3098", Qt::CaseInsensitive) == 0);
    const auto out = save(bench, drawing, "untouched", target("dwg2000"));

    EditableDrawing reopened;
    REQUIRE(lc::test::importDrawing(reopened.m_graphic, out, RS2::FormatDWG));
    const RS_Text* text = only<RS_Text>(reopened, RS2::EntityText);
    CAPTURE(text->getStyle().toStdString());
    CHECK(text->getStyle().compare("iso3098", Qt::CaseInsensitive) == 0);
}

TEST_CASE("an untouched attribute keeps the value another program stored",
          "[attribute-export][attrib][filter][dxf][plain-value]") {
    const Bench bench("attr_stored_value");
    EditableDrawing drawing;
    load(bench, AttributeDxf::rich(), drawing);
    const auto out = save(bench, drawing, "untouched", target("dxf2018"));
    checkWrittenDxf(out);

    // Next to its MTEXT the attribute's own value is not shown; it is what
    // the program that wrote the file put there, and it is not ours to change.
    const DxfRecord record = onlyRecord(out, "ATTRIB");
    CHECK(record.own(1) == std::vector<std::string>{"fallback"});
    CHECK(record.embedded(1) == std::vector<std::string>{"{\\fArial;ABC}\\PDEF"});
}

TEST_CASE("an attribute in AutoCAD's group order opens as one multi-line text",
          "[attribute-export][attrib][filter][dxf][embedded-mtext]") {
    const Bench bench("attr_marker_101");
    EditableDrawing drawing;
    load(bench, AttributeDxf{}, drawing);

    // One multi-line text, not a text of one line that took the groups of the
    // embedded object for its own: the width there is 0.
    CHECK(drawing.live<RS_Text>(RS2::EntityText).empty());
    const RS_MText* text = only<RS_MText>(drawing, RS2::EntityMText);
    CHECK(text->getText().toStdString() == "my text\nsecond line");
    CHECK(text->getInsertionPoint().distanceTo(RS_Vector(1.0, 4.0)) < 1e-9);
    CHECK(text->getHeight() == 9.0);
    CHECK(text->getWidth() == 0.0);
    CHECK(text->getHAlign() == RS_MTextData::HALeft);
    CHECK(text->getVAlign() == RS_MTextData::VATop);
    CHECK(text->getDrawingDirection() == RS_MTextData::ByStyle);
    CHECK(text->getLineSpacingStyle() == RS_MTextData::AtLeast);
    CHECK(text->getLineSpacingFactor() == 1.0);
    CHECK(text->getAngle() == 0.0);

    const auto out = save(bench, drawing, "untouched", target("dxf2018"));
    checkWrittenDxf(out);
    const DxfRecord record = onlyRecord(out, "ATTRIB");
    checkEmbeddedMTextPlace(record);
    // The copy of the alignment point before the marker.
    const std::size_t at = record.embeddedAt();
    CHECK(lc::test::sameValues({"1", "4", "0"}, {record.m_groups[at - 3].m_value, record.m_groups[at - 2].m_value,
                                                 record.m_groups[at - 1].m_value}));
    CHECK(record.embedded(3).empty());
    CHECK(record.embedded(1) == std::vector<std::string>{"my text\\Psecond line"});
    CHECK(record.embedded(1001).empty());

    EditableDrawing reopened;
    REQUIRE(lc::test::importDrawing(reopened.m_graphic, out, RS2::FormatDXFRW2018));
    CHECK(reopened.live<RS_Text>(RS2::EntityText).empty());
    checkSameMText(*text, *only<RS_MText>(reopened, RS2::EntityMText));
}

TEST_CASE("an attribute with the marker LibreCAD wrote before still opens",
          "[attribute-export][attrib][filter][dxf][embedded-mtext]") {
    const Bench bench("attr_marker_100");

    SECTION("the marker alone") {
        AttributeDxf fixture;
        fixture.m_marker = "100";
        EditableDrawing drawing;
        load(bench, fixture, drawing);
        CHECK(drawing.live<RS_Text>(RS2::EntityText).empty());
        const RS_MText* text = only<RS_MText>(drawing, RS2::EntityMText);
        CHECK(text->getText().toStdString() == "my text\nsecond line");
        CHECK(text->getInsertionPoint().distanceTo(RS_Vector(1.0, 4.0)) < 1e-9);
        CHECK(text->getHeight() == 9.0);
        CHECK(text->getWidth() == 0.0);
        CHECK(text->getVAlign() == RS_MTextData::VATop);

        // It is saved with the marker of today.
        const auto out = save(bench, drawing, "marker", target("dxf2018"));
        checkWrittenDxf(out);
        checkEmbeddedMTextPlace(onlyRecord(out, "ATTRIB"));
    }

    // A rotated right-to-left attribute as earlier builds wrote it: the
    // direction of the text twice, as an x-axis that was never updated and as
    // the angle, and the right-to-left note twice, once for the MTEXT and
    // once for the attribute.
    SECTION("with a stale x-axis and two lists of application data") {
        AttributeDxf fixture = AttributeDxf::rich();
        fixture.m_marker = "100";
        fixture.m_mtextExtra = "11\n1\n21\n0\n31\n0\n50\n30\n";
        fixture.m_mtextTail = "1001\nLibreCad\n1071\n2\n1001\nLibreCad\n1071\n2\n";
        EditableDrawing drawing;
        load(bench, fixture, drawing);
        const RS_MText* text = only<RS_MText>(drawing, RS2::EntityMText);
        const double angle = 30.0 * M_PI / 180.0;
        CHECK(text->getText().toStdString() == "ABC\nDEF");
        CHECK(text->getDrawingDirection() == RS_MTextData::RightToLeft);
        CHECK(text->getAngle() == Catch::Approx(angle).margin(1e-9));

        const auto out = save(bench, drawing, "right_to_left", target("dxf2018"));
        checkWrittenDxf(out);
        const DxfRecord record = onlyRecord(out, "ATTRIB");
        checkEmbeddedMTextPlace(record);
        CHECK(record.embedded(1001) == std::vector<std::string>{"LibreCad"});
        CHECK(record.own(1001).empty());

        EditableDrawing reopened;
        REQUIRE(lc::test::importDrawing(reopened.m_graphic, out, RS2::FormatDXFRW2018));
        const RS_MText* again = only<RS_MText>(reopened, RS2::EntityMText);
        CHECK(again->getDrawingDirection() == RS_MTextData::RightToLeft);
        CHECK(again->getAngle() == Catch::Approx(angle).margin(1e-9));
    }
}

TEST_CASE("a block with a multi-line attribute definition keeps the definition's own text fields",
          "[attribute-export][attrib][filter][dxf][embedded-mtext]") {
    const Bench bench("attr_definition");
    AttributeDxf fixture;
    fixture.m_blockContent = attdefDxf();
    EditableDrawing drawing;
    load(bench, fixture, drawing);

    // The drawing shows a definition as a text of one line in the block.
    const RS_Block* block = drawing.m_graphic.findBlock("TITLE");
    REQUIRE(block != nullptr);
    std::vector<const RS_Text*> definitions;
    for (const RS_Entity* entity : *block) {
        REQUIRE(entity != nullptr);
        REQUIRE(entity->rtti() == RS2::EntityText);
        definitions.push_back(static_cast<const RS_Text*>(entity));
    }
    REQUIRE(definitions.size() == 1);
    CHECK(definitions.front()->getText().toStdString() == "DEFAULT");
    // Its own group 40, not the 7 of the embedded MTEXT; and no group 41 of
    // its own, not the 0 of the MTEXT, which is a width there.
    CHECK(definitions.front()->getHeight() == 2.5);
    CHECK(definitions.front()->getWidthRel() == 1.0);

    // The attribute of the block reference is not disturbed by it.
    CHECK(only<RS_MText>(drawing, RS2::EntityMText)->getText().toStdString() == "my text\nsecond line");
}

TEST_CASE("a rotated attribute read from DXF keeps its angle through DWG 2018",
          "[attribute-export][attrib][filter][dxf][dwg][attrib-geometry]") {
    const Bench bench("attr_angle_dwg2018");
    // The direction as AutoCAD writes it: the x-axis of the MTEXT, and the
    // angle on the attribute itself.
    AttributeDxf fixture;
    fixture.m_textExtra = "50\n30\n";
    fixture.m_mtextExtra = "11\n0.8660254037844387\n21\n0.5\n31\n0\n";
    EditableDrawing drawing;
    load(bench, fixture, drawing);
    const double angle = 30.0 * M_PI / 180.0;
    CHECK(only<RS_MText>(drawing, RS2::EntityMText)->getAngle() == Catch::Approx(angle).margin(1e-9));

    const auto out = save(bench, drawing, "untouched", target("dwg2018"));
    EditableDrawing reopened;
    REQUIRE(lc::test::importDrawing(reopened.m_graphic, out, RS2::FormatDWG2018));
    CHECK(only<RS_MText>(reopened, RS2::EntityMText)->getAngle() == Catch::Approx(angle).margin(1e-9));
}

// The fixtures that the cases above do not use open as what their names say.
TEST_CASE("the single-line and the standalone fixture open as the texts they describe",
          "[attribute-export][attrib][filter][dxf][keep]") {
    const Bench bench("attr_plain_fixtures");

    SECTION("a single-line attribute") {
        EditableDrawing drawing;
        load(bench, plainAttributeDxf("PLAIN VALUE"), drawing);
        CHECK(drawing.live<RS_MText>(RS2::EntityMText).empty());
        const RS_Text* text = only<RS_Text>(drawing, RS2::EntityText);
        CHECK(text->getText().toStdString() == "PLAIN VALUE");
        CHECK(text->getInsertionPoint().distanceTo(RS_Vector(1.0, 4.0)) < 1e-9);
        CHECK(text->getHeight() == 9.0);

        for (const char* format : {"dxf2018", "dxf2007"}) {
            CAPTURE(format);
            const auto out = save(bench, drawing, "single_line", target(format));
            checkWrittenDxf(out);
            const DxfRecord record = onlyRecord(out, "ATTRIB");
            CHECK(record.embeddedMarker() == 0);
            CHECK(record.own(1) == std::vector<std::string>{"PLAIN VALUE"});
            CHECK(record.own(2) == std::vector<std::string>{"LABEL"});
        }
    }

    SECTION("an MTEXT of its own") {
        EditableDrawing drawing;
        loadDxf(bench, standaloneMTextDxf("1"), drawing);
        CHECK(drawing.live<RS_Insert>(RS2::EntityInsert).empty());
        const RS_MText* text = only<RS_MText>(drawing, RS2::EntityMText);
        CHECK(text->getText().toStdString() == "my text\nsecond line");
        CHECK(text->getWidth() == 80.0);
        CHECK(text->getLineSpacingStyle() == RS_MTextData::AtLeast);

        const auto out = save(bench, drawing, "standalone", target("dxf2018"));
        checkWrittenDxf(out);
        CHECK(lc::test::dxfRecords(out, "ATTRIB").empty());
        const DxfRecord record = onlyRecord(out, "MTEXT");
        CHECK(record.own(1) == std::vector<std::string>{"my text\\Psecond line"});
        CHECK(record.own(73) == std::vector<std::string>{"1"});
    }
}

// Local only: the drawing is AutoCAD's and is not in the repository.
TEST_CASE("AutoCAD's sample attribute opens with its text",
          "[attribute-export][attrib][filter][dxf][embedded-mtext][fixture]") {
    const std::filesystem::path sample = externalSample("sample_AC1032_ascii.dxf");
    if (sample.empty()) {
        SKIP("sample_AC1032_ascii.dxf is an external fixture; set "
             "LIBRECAD_EXTERNAL_DWG_FIXTURE_DIR to run this case");
    }
    const Bench bench("attr_acad_sample");
    const std::string text = "my multi line text for the attrrib";

    EditableDrawing drawing;
    REQUIRE(lc::test::importDrawing(drawing.m_graphic, sample, RS2::FormatDXFRW2018));
    std::vector<const RS_MText*> found;
    for (const RS_MText* candidate : drawing.live<RS_MText>(RS2::EntityMText)) {
        if (candidate->getText().toStdString() == text) {
            found.push_back(candidate);
        }
    }
    REQUIRE(found.size() == 1);
    CHECK(found.front()->getStyle().compare("Standard", Qt::CaseInsensitive) == 0);
    CHECK(found.front()->getInsertionPoint().distanceTo(RS_Vector(977.2814835151885, 28.7508142234822)) < 1e-9);
    CHECK(found.front()->getHeight() == Catch::Approx(1.720654456209621));
    CHECK(found.front()->getWidth() == 0.0);

    // To the DXF formats only: this drawing is not saved as DWG at all, for
    // reasons that have nothing to do with its attributes.
    for (const Target& to : kTargets) {
        if (!to.m_dxf) {
            continue;
        }
        CAPTURE(to.m_name);
        const auto out = save(bench, drawing, "untouched", to);
        std::vector<DxfRecord> records;
        for (const DxfRecord& record : lc::test::dxfRecords(out, "ATTRIB")) {
            if (record.own(2) == std::vector<std::string>{"MULTI_LINE_ATT"}) {
                records.push_back(record);
            }
        }
        REQUIRE(records.size() == 1);
        // The position of the attribute is locked in the sample (its second 280).
        CHECK(records.front().own(280) == std::vector<std::string>{"1"});
        if (to.m_embedsMText) {
            checkEmbeddedMTextPlace(records.front());
            CHECK(records.front().embedded(1) == std::vector<std::string>{text});
        }
    }
    // sample_AC1032.dwg is not opened here: the DWG layout of an attribute's
    // embedded MTEXT is not AutoCAD's yet, which is corrected separately.
}
