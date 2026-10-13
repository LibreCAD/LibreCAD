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
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <QFile>
#include <QList>
#include <QString>

#include "drw_entities.h"
#include "lc_copyutils.h"
#include "lc_drwnullinterface.h"
#include "lc_filtertestsupport.h"
#include "lc_sourcefonts.h"
#include "libdwgr.h"
#include "rs_block.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_modification.h"
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

// A drawing with one MTEXT that belongs to no block reference. Without a
// spacing style the MTEXT has no group 73.
std::string standaloneMTextDxf(const std::string& spacingStyle) {
    const std::string group73 = spacingStyle.empty() ? std::string() : "73\n" + spacingStyle + "\n";
    return "0\nSECTION\n2\nENTITIES\n0\nMTEXT\n100\nAcDbEntity\n8\n0\n100\nAcDbMText\n"
           "10\n1\n20\n4\n30\n0\n40\n9\n41\n80\n71\n1\n72\n5\n1\nmy text\\Psecond line\n"
           + group73 + "44\n1.0\n0\nENDSEC\n0\nEOF\n";
}

// The line-spacing style of an MTEXT is its group 73: 1 is "at least", 2 is
// "exact", and without the group it is 1. A file may hold another value: 0
// is what earlier builds of LibreCAD stored in the MTEXT of an attribute.
struct SpacingStyleRow {
    const char* m_name;                             // also the start of the file names
    const char* m_group73;                          // empty: the MTEXT has no such group
    RS_MTextData::MTextLineSpacingStyle m_style;    // what the drawing shows
    const char* m_written;                          // group 73 of a file saved from the drawing
};
const SpacingStyleRow kSpacingStyleRows[] = {
    {"absent", "", RS_MTextData::AtLeast, "1"},
    {"one", "1", RS_MTextData::AtLeast, "1"},
    {"two", "2", RS_MTextData::Exact, "2"},
    {"zero", "0", RS_MTextData::AtLeast, "1"},
};

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

// A saved file and what the export said it left out, one item per line.
struct Saved {
    std::filesystem::path m_file;
    std::string m_report;
};

// Saves as save() does, with a filter that is asked for its report afterwards.
Saved saveWithReport(const Bench& bench, EditableDrawing& drawing, const std::string& variant, const Target& to) {
    const std::filesystem::path file = bench.m_out.file(variant + "_" + to.m_name + to.m_suffix);
    CAPTURE(file.filename().string());
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(drawing.m_graphic, lc::test::toQString(file), to.m_format));
    lc::test::keepForInspection(file);
    return {file, filter.exportReport().toStdString()};
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

// What libdxfrw reads of a DWG, without the filter in between: the block
// references with their attributes, and the handle of every registered
// application by its name.
struct DwgContent : lc::test::NullDrwInterface {
    std::vector<DRW_Insert> m_inserts;
    std::map<std::string, std::uint32_t> m_applications;
    void addInsert(const DRW_Insert& insert) override { m_inserts.push_back(insert); }
    void addAppId(const DRW_AppId& application) override { m_applications[application.name] = application.handle; }
};

void readDwg(const std::filesystem::path& file, DwgContent& content) {
    CAPTURE(file.filename().string());
    dwgRW reader(QFile::encodeName(lc::test::toQString(file)).constData());
    REQUIRE(reader.read(&content, false));
}

// The MTEXT of the one attribute of the one block reference of a DWG.
const DRW_MText& onlyEmbeddedMText(const DwgContent& content) {
    REQUIRE(content.m_inserts.size() == 1);
    REQUIRE(content.m_inserts.front().attlist.size() == 1);
    REQUIRE(content.m_inserts.front().attlist.front() != nullptr);
    REQUIRE(content.m_inserts.front().attlist.front()->mtext != nullptr);
    return *content.m_inserts.front().attlist.front()->mtext;
}

// The annotation-scale data of the attribute that AttributeDwgSource writes,
// and the name of the application the data belongs to. What the bytes mean
// is not known here; a DWG 2018 holds them next to the MTEXT of a multi-line
// attribute, and no other format has a place for them.
const std::vector<std::uint8_t> kScaleData{0x01, 0x7F, 0xFE};
constexpr std::uint16_t kScaleDataShort = 0x72;
const char* const kScaleApplication = "LC_SCALE_APP";
// What an export says when it left such data out.
const char* const kScaleDataLeftOut = "annotation scale data of 1 multi-line attribute(s)";

// Writes a DWG 2018 with one block reference and one multi-line attribute
// that has annotation-scale data, through libdxfrw: no DXF can hold the data,
// and no drawing of another program is copied. The value of the attribute
// itself is not empty, so that the cases on this source do not depend on
// what a format without the MTEXT gets as the value.
struct AttributeDwgSource : lc::test::NullDrwInterface {
    double m_angle = 0.0;                   // of the text, in radians
    bool m_dataNamesNoApplication = false;  // the data names a handle that no record of the file has
    std::uint32_t m_scaleApplication = 0;   // after write(): the handle of LC_SCALE_APP in the file

    void write(const std::filesystem::path& file) {
        CAPTURE(file.filename().string());
        dwgRW writer(QFile::encodeName(lc::test::toQString(file)).constData());
        m_writer = &writer;
        const bool written = writer.write(this, DRW::AC1032, false);
        m_writer = nullptr;
        REQUIRE(written);
        REQUIRE(m_insertWritten);
        REQUIRE(m_scaleApplication != 0);
    }

    // The callbacks only note what went wrong: libdxfrw calls them, and an
    // exception of a failed REQUIRE would not get past it.
    void writeAppId() override {
        DRW_AppId application;
        application.name = kScaleApplication;
        if (m_writer != nullptr && m_writer->addAppId(&application)) {
            m_scaleApplication = application.handle;
        }
    }
    void writeBlocks() override {
        if (m_writer != nullptr) {
            m_block = m_writer->defineBlock("TITLE", DRW_Coord{0.0, 0.0, 0.0});
        }
    }
    void writeEntities() override {
        if (m_writer == nullptr || m_block == 0) {
            return;
        }
        DRW_Insert insert;
        insert.basePoint = DRW_Coord{10.0, 20.0, 0.0};
        insert.extPoint = DRW_Coord{0.0, 0.0, 1.0};
        insert.blockRecH.ref = m_block;
        auto attribute = std::make_shared<DRW_Attrib>();
        attribute->basePoint = DRW_Coord{10.0, 20.0, 0.0};
        attribute->secPoint = attribute->basePoint;
        attribute->extPoint = DRW_Coord{0.0, 0.0, 1.0};
        attribute->height = 2.5;
        attribute->text = "line 1";
        attribute->tag = "NOTE";
        attribute->attVersion = 1;
        attribute->m_attributeType = 2;
        attribute->mtext = std::make_unique<DRW_MText>();
        DRW_MText& mtext = *attribute->mtext;
        mtext.basePoint = attribute->basePoint;
        mtext.extPoint = DRW_Coord{0.0, 0.0, 1.0};
        mtext.secPoint = DRW_Coord{std::cos(m_angle), std::sin(m_angle), 0.0};
        mtext.widthscale = 20.0;
        mtext.height = 2.5;
        mtext.textgen = 1;
        mtext.alignH = DRW_Text::HLeft;
        mtext.interlin = 1.0;
        mtext.text = "line 1\\Pline 2";
        mtext.m_r2018IsNotAnnotative = false;
        mtext.m_r2018AnnotativeData = kScaleData;
        mtext.m_r2018AnnotativeUnknown = kScaleDataShort;
        mtext.m_r2018AnnotativeAppHandle = m_dataNamesNoApplication ? 0x7F0u : m_scaleApplication;
        insert.attlist.push_back(std::move(attribute));
        m_insertWritten = m_writer->writeInsert(&insert);
    }

private:
    dwgRW* m_writer = nullptr;
    std::uint32_t m_block = 0;
    bool m_insertWritten = false;
};

// The formats the cases on that source save to; the one that holds the data
// comes first.
const std::vector<const char*> kScaleDataFormats{"dwg2018", "dxf2018", "dxf2007", "dxf2000", "dwg2013", "dwg2000"};

// A DWG 2018 that was saved with the data: the bytes and the number after
// them are the source's, and nothing is reported. The data names its
// application by handle, and a save gives every record a handle of its own,
// so the handle is the one of the application in the saved file, which is
// not the number the source had.
void checkScaleDataKept(const Saved& saved, const AttributeDwgSource& source) {
    CAPTURE(saved.m_file.filename().string());
    DwgContent written;
    readDwg(saved.m_file, written);
    const DRW_MText& mtext = onlyEmbeddedMText(written);
    CHECK(mtext.m_r2018AnnotativeData == kScaleData);
    CHECK(mtext.m_r2018AnnotativeUnknown == kScaleDataShort);
    CHECK_FALSE(mtext.m_r2018IsNotAnnotative);
    REQUIRE(written.m_applications.count(kScaleApplication) == 1);
    const std::uint32_t application = written.m_applications[kScaleApplication];
    // A handle copied from the source would pass for the right one if the
    // two files gave the application the same handle.
    REQUIRE(application != source.m_scaleApplication);
    CHECK(mtext.m_r2018AnnotativeAppHandle == application);
    CHECK(saved.m_report.find("annotation scale data") == std::string::npos);
}

// A save that left the data out says so, and has the block reference and
// its attribute all the same. In a DWG 2018 the attribute is still marked as
// annotative.
void checkScaleDataLeftOut(const Saved& saved, const Target& to) {
    CAPTURE(saved.m_file.filename().string(), saved.m_report);
    CHECK(saved.m_report.find(kScaleDataLeftOut) != std::string::npos);
    if (to.m_dxf) {
        CHECK(lc::test::duplicateDxfHandles(saved.m_file).empty());
        CHECK(lc::test::dxfRecords(saved.m_file, "INSERT").size() == 1);
        CHECK(lc::test::dxfRecords(saved.m_file, "ATTRIB").size() == 1);
    }
    else if (to.m_embedsMText) {
        DwgContent written;
        readDwg(saved.m_file, written);
        const DRW_MText& mtext = onlyEmbeddedMText(written);
        CHECK(mtext.m_r2018AnnotativeData.empty());
        CHECK_FALSE(mtext.m_r2018IsNotAnnotative);
    }
}

// One change of an annotative attribute at a time, saved to every format.
// The drawing shows one layout of the text, so it cannot vouch for the data
// next to a text the data was not read with: the save succeeds without it.
// The change is made the way the tools make it, by a clone in the place of
// the text, or on the text itself, as a plugin makes it.
void checkEditedAnnotativeAttribute(const Bench& bench, const bool onTheTextItself) {
    AttributeDwgSource source;
    source.m_angle = 0.7;
    const std::filesystem::path file = bench.m_out.file("source.dwg");
    source.write(file);

    struct Edit {
        const char* m_name;
        std::function<void(RS_MText&)> m_change;
    };
    const std::vector<Edit> edits{
        {"text", [](RS_MText& text) { text.setText("changed\ntext"); }},
        {"move", [](RS_MText& text) { text.move(RS_Vector(5.0, 0.0)); }},
        {"height", [](RS_MText& text) { text.setHeight(12.0); }},
        {"angle", [](RS_MText& text) { text.setAngle(1.0); }},
        {"style", [](RS_MText& text) { text.setStyle("iso3098"); }},
    };
    for (const Edit& edit : edits) {
        for (const char* format : kScaleDataFormats) {
            const Target to = target(format);
            CAPTURE(edit.m_name, to.m_name);
            EditableDrawing drawing;
            REQUIRE(lc::test::importDrawing(drawing.m_graphic, file, RS2::FormatDWG2018));
            const RS_MText* imported = only<RS_MText>(drawing, RS2::EntityMText);
            if (onTheTextItself) {
                auto* text = const_cast<RS_MText*>(imported);
                edit.m_change(*text);
                text->update();
            }
            else {
                drawing.edit(imported, edit.m_change);
            }

            const Saved saved = saveWithReport(bench, drawing, edit.m_name, to);
            checkScaleDataLeftOut(saved, to);
            if (to.m_embedsMText && std::string(edit.m_name) == "text") {
                EditableDrawing reopened;
                REQUIRE(lc::test::importDrawing(reopened.m_graphic, saved.m_file, to.m_format));
                CHECK(reopened.live<RS_Insert>(RS2::EntityInsert).size() == 1);
                CHECK(only<RS_MText>(reopened, RS2::EntityMText)->getText().toStdString() == "changed\ntext");
            }
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

// A block reference with four attributes of one line: NAME and NOTE show,
// the value of BLANK is empty and CODE is hidden. (An ATTRIB without a group
// 1 at all is not read: the DXF reader gives the whole file up.) With
// handles, every record has one, as in a file of another program.
std::string attributedInsertRecords(const bool handles) {
    const auto handle = [handles](const char* value) {
        return handles ? std::string("5\n") + value + "\n" : std::string();
    };
    return "0\nINSERT\n" + handle("1A0") + "8\n0\n2\nTAG\n10\n0\n20\n0\n66\n1\n"
           "0\nATTRIB\n" + handle("1A1") + "8\n0\n10\n1\n20\n2\n40\n1\n1\nfirst\n2\nNAME\n70\n0\n"
           "0\nATTRIB\n" + handle("1A2") + "8\n0\n10\n1\n20\n6\n40\n1\n1\nsecond\n2\nNOTE\n70\n0\n"
           "0\nATTRIB\n" + handle("1A3") + "8\n0\n10\n1\n20\n8\n40\n1\n1\n\n2\nBLANK\n70\n0\n"
           "0\nATTRIB\n" + handle("1A4") + "8\n0\n10\n1\n20\n10\n40\n1\n1\nsecret\n2\nCODE\n70\n1\n"
           "0\nSEQEND\n" + handle("1A5") + "8\n0\n";
}

// The block of that reference: a line, and no attribute definition, which
// would open as one more text.
std::string tagBlockDxf(const bool handles) {
    return std::string("0\nBLOCK\n8\n0\n2\nTAG\n70\n0\n10\n0\n20\n0\n3\nTAG\n0\nLINE\n")
           + (handles ? "5\n1B0\n" : "") + "8\n0\n10\n0\n20\n0\n11\n5\n21\n0\n0\nENDBLK\n8\n0\n";
}

// The block reference in the drawing itself, with a line after it: a handle
// taken and not used would lie inside the chain of a DWG 2000.
std::string attributedInsertDxf(const bool handles) {
    return "0\nSECTION\n2\nBLOCKS\n" + tagBlockDxf(handles) + "0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"
           + attributedInsertRecords(handles) + "0\nLINE\n" + (handles ? "5\n1A6\n" : "")
           + "8\n0\n10\n20\n20\n0\n11\n30\n21\n0\n0\nENDSEC\n0\nEOF\n";
}

// The same block reference inside the block OUTER, which the drawing inserts once.
std::string nestedAttributedInsertDxf() {
    return "0\nSECTION\n2\nBLOCKS\n" + tagBlockDxf(false)
           + "0\nBLOCK\n8\n0\n2\nOUTER\n70\n0\n10\n0\n20\n0\n3\nOUTER\n" + attributedInsertRecords(false)
           + "0\nENDBLK\n8\n0\n0\nENDSEC\n"
             "0\nSECTION\n2\nENTITIES\n0\nINSERT\n8\n0\n2\nOUTER\n10\n0\n20\n0\n0\nENDSEC\n0\nEOF\n";
}

// What a drawing, or a block of it, shows of block references and texts, on
// one line: each block reference by its insertion x with the values of the
// texts that belong to it, in their order; the values of the other texts,
// sorted; how many of those are hidden; and the number of lines.
std::string contentOf(const RS_EntityContainer& container) {
    struct Reference {
        long m_x;
        unsigned long long m_lineage;
        std::vector<std::string> m_values;
    };
    std::vector<Reference> references;
    for (const RS_Entity* entity : container) {
        if (entity != nullptr && !entity->isDeleted() && entity->rtti() == RS2::EntityInsert) {
            const double x = static_cast<const RS_Insert*>(entity)->getInsertionPoint().x;
            references.push_back({std::lround(x), entity->lineage(), {}});
        }
    }
    std::stable_sort(references.begin(), references.end(),
                     [](const Reference& lhs, const Reference& rhs) { return lhs.m_x < rhs.m_x; });
    std::vector<std::string> others;
    int hidden = 0;
    int lines = 0;
    for (const RS_Entity* entity : container) {
        if (entity == nullptr || entity->isDeleted()) {
            continue;
        }
        std::string value;
        if (entity->rtti() == RS2::EntityLine) {
            ++lines;
            continue;
        }
        if (entity->rtti() == RS2::EntityText) {
            value = static_cast<const RS_Text*>(entity)->getText().toStdString();
        }
        else if (entity->rtti() == RS2::EntityMText) {
            value = static_cast<const RS_MText*>(entity)->getText().toStdString();
        }
        else {
            continue;
        }
        const auto owner = std::find_if(references.begin(), references.end(), [entity](const Reference& reference) {
            return reference.m_lineage == entity->ownerLineage();
        });
        if (owner != references.end()) {
            owner->m_values.push_back(value);
        }
        else {
            others.push_back(value);
            hidden += entity->getFlag(RS2::FlagVisible) ? 0 : 1;
        }
    }
    std::sort(others.begin(), others.end());
    const auto joined = [](const std::vector<std::string>& values) {
        std::string all;
        for (std::size_t i = 0; i < values.size(); ++i) {
            all += (i == 0 ? "" : "|") + values[i];
        }
        return all;
    };
    std::string line;
    for (const Reference& reference : references) {
        line += "INSERT@" + std::to_string(reference.m_x) + "[" + joined(reference.m_values) + "] ";
    }
    return line + "TEXT[" + joined(others) + "] hidden=" + std::to_string(hidden) + " LINE=" + std::to_string(lines);
}

// The same of a saved file, opened through the filter: of the drawing, or of
// one of its blocks.
std::string readBack(const std::filesystem::path& file, const Target& from, const char* block = nullptr) {
    CAPTURE(file.filename().string());
    RS_Graphic reopened;
    REQUIRE(lc::test::importDrawing(reopened, file, from.m_format));
    if (block == nullptr) {
        return contentOf(reopened);
    }
    const RS_Block* found = reopened.findBlock(block);
    REQUIRE(found != nullptr);
    return contentOf(*found);
}

// Saves the drawing to each of the formats and expects every file to open
// with the same content. Returns the DXF files, for a look at their records.
std::vector<std::filesystem::path> saveAll(const Bench& bench, EditableDrawing& drawing, const std::string& variant,
                                           const std::vector<const char*>& formats, const std::string& expected,
                                           const char* block = nullptr) {
    std::vector<std::filesystem::path> dxfFiles;
    for (const char* format : formats) {
        const Target to = target(format);
        CAPTURE(variant, to.m_name);
        const auto file = save(bench, drawing, variant, to);
        CHECK(readBack(file, to, block) == expected);
        if (to.m_dxf) {
            checkWrittenDxf(file);
            dxfFiles.push_back(file);
        }
    }
    return dxfFiles;
}

// The tags of the attributes of a written DXF, in file order.
std::vector<std::string> attributeTags(const std::filesystem::path& file) {
    std::vector<std::string> tags;
    for (const DxfRecord& record : lc::test::dxfRecords(file, "ATTRIB")) {
        const std::vector<std::string> tag = record.own(2);
        tags.push_back(tag.empty() ? std::string() : tag.front());
    }
    return tags;
}

// The one attribute with a tag in a written DXF.
DxfRecord attributeRecord(const std::filesystem::path& file, const std::string& tag) {
    std::vector<DxfRecord> found;
    for (const DxfRecord& record : lc::test::dxfRecords(file, "ATTRIB")) {
        if (record.own(2) == std::vector<std::string>{tag}) {
            found.push_back(record);
        }
    }
    CAPTURE(file.filename().string(), tag);
    REQUIRE(found.size() == 1);
    return found.front();
}

// Opens the fixture with handles and makes sure that the drawing knows one
// for each of its entities. (A drawing opened from a DXF that LibreCAD saved
// would know them too, but no DWG can be saved from one: its VPORT *ACTIVE
// is found twice, which is not a matter of attributes.)
void loadWithHandles(const Bench& bench, EditableDrawing& drawing, const std::string& name) {
    loadDxf(bench, attributedInsertDxf(true), drawing, name);
    REQUIRE(drawing.m_graphic.count() == 6);
    for (const RS_Entity* entity : drawing.m_graphic) {
        REQUIRE(entity != nullptr);
        REQUIRE(entity->sourceHandle() != 0);
    }
}

// The block reference the drawing shows at an insertion x.
const RS_Insert* referenceAt(EditableDrawing& drawing, const double x) {
    const RS_Insert* found = nullptr;
    for (const RS_Insert* insert : drawing.live<RS_Insert>(RS2::EntityInsert)) {
        if (std::abs(insert->getInsertionPoint().x - x) < 1e-9) {
            REQUIRE(found == nullptr);
            found = insert;
        }
    }
    REQUIRE(found != nullptr);
    return found;
}

// The text of one line the drawing shows with a value at an insertion x; the
// attributes of the fixture are at 1.
const RS_Text* lineText(EditableDrawing& drawing, const std::string& value, const double x = 1.0) {
    const RS_Text* found = nullptr;
    for (const RS_Text* text : drawing.live<RS_Text>(RS2::EntityText)) {
        if (text->getText().toStdString() == value && std::abs(text->getInsertionPoint().x - x) < 1e-9) {
            REQUIRE(found == nullptr);
            found = text;
        }
    }
    REQUIRE(found != nullptr);
    return found;
}

// What a tool is given: the entities of a selection.
QList<RS_Entity*> selection(const std::vector<const RS_Entity*>& entities) {
    QList<RS_Entity*> list;
    for (const RS_Entity* entity : entities) {
        list.append(const_cast<RS_Entity*>(entity));
    }
    return list;
}

// What the Move tool does in one step: without copies the entities are
// replaced by moved clones; with copies they stay and the clones are added.
void moveEntities(EditableDrawing& drawing, const std::vector<const RS_Entity*>& entities, const RS_Vector& offset,
                  const int copies = 0) {
    RS_MoveData data;
    data.offset = offset;
    data.keepOriginals = copies > 0;
    data.multipleCopies = copies > 1;
    data.number = copies;
    drawing.modify([&](LC_DocumentModificationBatch& ctx) {
        RS_Modification::move(data, selection(entities), false, ctx);
    });
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

TEST_CASE("a standalone MTEXT keeps its line-spacing style",
          "[attribute-export][mtext][filter][dxf][dwg][mtext-spacing]") {
    const Bench bench("mtext_spacing_style");
    for (const SpacingStyleRow& row : kSpacingStyleRows) {
        CAPTURE(row.m_name);
        EditableDrawing drawing;
        loadDxf(bench, standaloneMTextDxf(row.m_group73), drawing, std::string(row.m_name) + "_source");
        CHECK(only<RS_MText>(drawing, RS2::EntityMText)->getLineSpacingStyle() == row.m_style);

        for (const Target& to : kTargets) {
            CAPTURE(to.m_name);
            const auto out = save(bench, drawing, row.m_name, to);
            EditableDrawing reopened;
            REQUIRE(lc::test::importDrawing(reopened.m_graphic, out, to.m_format));
            CHECK(only<RS_MText>(reopened, RS2::EntityMText)->getLineSpacingStyle() == row.m_style);

            if (to.m_dxf) {
                checkWrittenDxf(out);
            }
            // What a DWG holds is read here as the DXF that the drawing it
            // opens as is saved to.
            const auto dxf =
                to.m_dxf ? out : save(bench, reopened, std::string(row.m_name) + "_" + to.m_name, target("dxf2007"));
            CHECK(onlyRecord(dxf, "MTEXT").own(73) == std::vector<std::string>{row.m_written});
        }
    }
}

TEST_CASE("the MTEXT of an attribute keeps line-spacing style 2 and never stores 0",
          "[attribute-export][attrib][filter][dxf][dwg][mtext-spacing]") {
    const Bench bench("attr_spacing_style");
    for (const SpacingStyleRow& row : kSpacingStyleRows) {
        if (std::string(row.m_group73).empty()) {
            continue;   // AutoCAD always writes the group in the MTEXT of an attribute
        }
        CAPTURE(row.m_name);
        AttributeDxf fixture = AttributeDxf::rich();
        fixture.m_spacingStyle = row.m_group73;
        EditableDrawing drawing;
        const DxfRecord source =
            onlyRecord(load(bench, fixture, drawing, std::string(row.m_name) + "_source"), "ATTRIB");
        REQUIRE(source.embedded(73) == std::vector<std::string>{row.m_group73});
        CHECK(only<RS_MText>(drawing, RS2::EntityMText)->getLineSpacingStyle() == row.m_style);

        for (const char* format : {"dxf2018", "dwg2018"}) {
            const Target to = target(format);
            CAPTURE(to.m_name);
            const auto out = save(bench, drawing, row.m_name, to);
            EditableDrawing reopened;
            REQUIRE(lc::test::importDrawing(reopened.m_graphic, out, to.m_format));
            CHECK(only<RS_MText>(reopened, RS2::EntityMText)->getLineSpacingStyle() == row.m_style);

            if (to.m_dxf) {
                checkWrittenDxf(out);
            }
            else {
                // The value in the DWG itself: the drawing shows a 0 as it
                // shows a 1.
                DwgContent content;
                readDwg(out, content);
                REQUIRE(content.m_inserts.size() == 1);
                REQUIRE(content.m_inserts.front().attlist.size() == 1);
                const auto& attribute = content.m_inserts.front().attlist.front();
                REQUIRE(attribute != nullptr);
                REQUIRE(attribute->mtext != nullptr);
                CHECK(std::to_string(attribute->mtext->linespacingStyle) == row.m_written);
            }
            // The MTEXT in a DWG is read as the DXF of the drawing it opens as.
            const auto dxf =
                to.m_dxf ? out : save(bench, reopened, std::string(row.m_name) + "_" + to.m_name, target("dxf2018"));
            const DxfRecord record = onlyRecord(dxf, "ATTRIB");
            CHECK(record.embedded(73) == std::vector<std::string>{row.m_written});
            // A value that reads as the default is no edit of the attribute:
            // the factor and the text with its font code are the file's, and
            // no measured size (42, 43) is added.
            CHECK(lc::test::sameValues(source.embedded(44), record.embedded(44)));
            CHECK(record.embedded(1) == source.embedded(1));
            CHECK(record.embedded(42).empty());
            CHECK(record.embedded(43).empty());
        }
    }
}

// The tools never change an entity: they put a changed clone in its place and
// flag the original deleted, and an undo flags the clone instead. A copy is a
// clone too. What is an attribute of which block reference therefore cannot
// hang on the objects themselves.

TEST_CASE("an attribute text replaced or undone or erased is written once",
          "[attribute-export][attrib][filter][dxf][dwg][attrib-owner]") {
    const Bench bench("attr_owner_edit");
    const std::vector<const char*> formats{"dxf2018", "dxf2007", "dwg2018", "dwg2013", "dwg2000"};
    const std::vector<std::string> allTags{"NAME", "NOTE", "BLANK", "CODE"};
    EditableDrawing drawing;
    loadWithHandles(bench, drawing, "fixture");
    const RS_Insert* insert = referenceAt(drawing, 0);
    for (const RS_Text* text : drawing.live<RS_Text>(RS2::EntityText)) {
        REQUIRE(text->ownerLineage() == insert->lineage());
    }

    for (const auto& file :
         saveAll(bench, drawing, "untouched", formats, "INSERT@0[first|second||secret] TEXT[] hidden=0 LINE=1")) {
        CHECK(attributeTags(file) == allTags);
    }

    drawing.edit(lineText(drawing, "first"), [](RS_Text& text) { text.setText("edited"); });
    for (const auto& file :
         saveAll(bench, drawing, "edited", formats, "INSERT@0[edited|second||secret] TEXT[] hidden=0 LINE=1")) {
        CHECK(attributeTags(file) == allTags);
    }

    REQUIRE(drawing.m_graphic.undo());
    for (const auto& file :
         saveAll(bench, drawing, "undone", formats, "INSERT@0[first|second||secret] TEXT[] hidden=0 LINE=1")) {
        CHECK(attributeTags(file) == allTags);
    }

    REQUIRE(drawing.m_graphic.redo());
    saveAll(bench, drawing, "redone", formats, "INSERT@0[edited|second||secret] TEXT[] hidden=0 LINE=1");
    REQUIRE(drawing.m_graphic.undo());

    drawing.erase(lineText(drawing, "second"));
    for (const auto& file :
         saveAll(bench, drawing, "erased", formats, "INSERT@0[first||secret] TEXT[] hidden=0 LINE=1")) {
        CHECK(attributeTags(file) == std::vector<std::string>{"NAME", "BLANK", "CODE"});
    }
}

TEST_CASE("a multi-line attribute edited the way the application edits is written once",
          "[attribute-export][attrib][filter][dxf][dwg][attrib-owner]") {
    const Bench bench("attr_owner_multiline");
    for (const bool handles : {false, true}) {
        const std::string name = handles ? "handles" : "plain";
        CAPTURE(name);
        AttributeDxf fixture;
        fixture.m_handles = handles;
        EditableDrawing drawing;
        load(bench, fixture, drawing, name + "_source");
        const RS_MText* imported = only<RS_MText>(drawing, RS2::EntityMText);

        // The text of the attribute in a saved file. The formats before 2018
        // hold it as a text of one line.
        const auto savedText = [&](const std::string& variant, const Target& to) {
            CAPTURE(variant, to.m_name);
            const auto file = save(bench, drawing, name + "_" + variant, to);
            if (to.m_dxf) {
                checkWrittenDxf(file);
                CHECK(lc::test::dxfRecords(file, "ATTRIB").size() == 1);
                CHECK(lc::test::dxfRecords(file, "TEXT").empty());
                CHECK(lc::test::dxfRecords(file, "MTEXT").empty());
            }
            EditableDrawing reopened;
            REQUIRE(lc::test::importDrawing(reopened.m_graphic, file, to.m_format));
            CHECK(reopened.live<RS_Insert>(RS2::EntityInsert).size() == 1);
            const RS_Entity* text = attributeText(reopened);
            CHECK((text->rtti() == RS2::EntityMText) == to.m_embedsMText);
            return text->rtti() == RS2::EntityMText ? static_cast<const RS_MText*>(text)->getText().toStdString()
                                                    : static_cast<const RS_Text*>(text)->getText().toStdString();
        };

        drawing.edit(imported, [](RS_MText& text) { text.setText("XYZ"); });
        for (const Target& to : kTargets) {
            CHECK(savedText("edited", to) == "XYZ");
        }

        // Undone, the attribute is the one of the file again; what a format
        // before 2018 shows of it is not this case's.
        REQUIRE(drawing.m_graphic.undo());
        for (const Target& to : kTargets) {
            const std::string text = savedText("undone", to);
            if (to.m_embedsMText) {
                CHECK(text == "my text\nsecond line");
            }
        }

        REQUIRE(drawing.m_graphic.redo());
        for (const Target& to : kTargets) {
            CHECK(savedText("redone", to) == "XYZ");
        }
    }
}

TEST_CASE("a block reference replaced by an edit keeps its attributes",
          "[attribute-export][attrib][filter][dxf][dwg][attrib-owner]") {
    const Bench bench("attr_owner_reference_edit");
    const std::vector<const char*> formats{"dxf2018", "dwg2018", "dwg2000"};
    EditableDrawing drawing;
    loadWithHandles(bench, drawing, "fixture");
    const RS_Insert* insert = referenceAt(drawing, 0);

    // The block reference alone: its attributes stay where their texts are.
    moveEntities(drawing, {insert}, RS_Vector(50.0, 0.0));
    for (const auto& file : saveAll(bench, drawing, "reference_moved", formats,
                                    "INSERT@50[first|second||secret] TEXT[] hidden=0 LINE=1")) {
        CHECK(attributeTags(file).size() == 4);
        CHECK(lc::test::dxfRecords(file, "TEXT").empty());
        CHECK(lc::test::sameValues({"1"}, attributeRecord(file, "NAME").own(10)));
    }

    REQUIRE(drawing.m_graphic.undo());
    saveAll(bench, drawing, "move_undone", formats, "INSERT@0[first|second||secret] TEXT[] hidden=0 LINE=1");

    // With the texts a selection takes. A hidden text is in no selection: it
    // stays an attribute of the block reference, where it was.
    moveEntities(drawing, {insert, lineText(drawing, "first"), lineText(drawing, "second")}, RS_Vector(50.0, 0.0));
    for (const auto& file : saveAll(bench, drawing, "all_moved", formats,
                                    "INSERT@50[first|second||secret] TEXT[] hidden=0 LINE=1")) {
        CHECK(attributeTags(file).size() == 4);
        CHECK(lc::test::dxfRecords(file, "TEXT").empty());
        CHECK(lc::test::sameValues({"51"}, attributeRecord(file, "NAME").own(10)));
        CHECK(lc::test::sameValues({"51"}, attributeRecord(file, "NOTE").own(10)));
        CHECK(lc::test::sameValues({"1"}, attributeRecord(file, "CODE").own(10)));
    }
    REQUIRE(drawing.m_graphic.undo());

    // Any other edit of the block reference is a clone in its place as well.
    drawing.edit(insert, [](RS_Insert& reference) { reference.rotate(RS_Vector(0.0, 0.0), M_PI / 2.0); });
    for (const auto& file : saveAll(bench, drawing, "reference_rotated", formats,
                                    "INSERT@0[first|second||secret] TEXT[] hidden=0 LINE=1")) {
        CHECK(attributeTags(file).size() == 4);
        CHECK(lc::test::sameValues({"90"}, onlyRecord(file, "INSERT").own(50)));
    }
}

TEST_CASE("each copy of a block reference writes its own attribute texts",
          "[attribute-export][attrib][filter][dxf][dwg][attrib-owner]") {
    const Bench bench("attr_owner_copies");
    const std::vector<const char*> formats{"dxf2018", "dwg2018", "dwg2000"};

    for (const bool handles : {true, false}) {
        const std::string name = handles ? "handles" : "plain";
        CAPTURE(name);
        EditableDrawing drawing;
        if (handles) {
            loadWithHandles(bench, drawing, name);
        }
        else {
            loadDxf(bench, attributedInsertDxf(false), drawing, name);
        }
        const RS_Insert* insert = referenceAt(drawing, 0);
        const RS_Text* nameText = lineText(drawing, "first");
        const RS_Text* noteText = lineText(drawing, "second");

        // Two copies of the block reference and of the two texts a selection
        // takes; the originals stay.
        moveEntities(drawing, {insert, nameText, noteText}, RS_Vector(50.0, 0.0), 2);
        std::vector<unsigned long long> lineages{insert->lineage()};
        for (const double x : {50.0, 100.0}) {
            CAPTURE(x);
            const RS_Insert* copy = referenceAt(drawing, x);
            CHECK(std::find(lineages.begin(), lineages.end(), copy->lineage()) == lineages.end());
            lineages.push_back(copy->lineage());
            CHECK(copy->sourceHandle() == 0);
            for (const char* value : {"first", "second"}) {
                const RS_Text* text = lineText(drawing, value, x + 1.0);
                CHECK(text->ownerLineage() == copy->lineage());
                CHECK(text->sourceHandle() == 0);
            }
        }
        CHECK(nameText->ownerLineage() == insert->lineage());
        CHECK(noteText->ownerLineage() == insert->lineage());

        // An edit of a copy's text, then of the original's: each stays with
        // its own block reference, whatever the order of the texts in the drawing.
        drawing.edit(lineText(drawing, "first", 51.0), [](RS_Text& text) { text.setText("one"); });
        for (const auto& file :
             saveAll(bench, drawing, name + "_copy_edited", formats,
                     "INSERT@0[first|second||secret] INSERT@50[one|second] INSERT@100[first|second] "
                     "TEXT[] hidden=0 LINE=1")) {
            CHECK(lc::test::dxfRecords(file, "INSERT").size() == 3);
            CHECK(attributeTags(file).size() == 8);
            CHECK(lc::test::dxfRecords(file, "TEXT").empty());
        }
        drawing.edit(nameText, [](RS_Text& text) { text.setText("zero"); });
        saveAll(bench, drawing, name + "_original_edited", formats,
                "INSERT@0[zero|second||secret] INSERT@50[one|second] INSERT@100[first|second] "
                "TEXT[] hidden=0 LINE=1");
    }

    // The Rotate and the Scale tool number their copies as the Move tool does.
    for (const char* tool : {"rotate", "scale"}) {
        CAPTURE(tool);
        EditableDrawing drawing;
        loadDxf(bench, attributedInsertDxf(false), drawing, tool);
        const QList<RS_Entity*> selected =
            selection({referenceAt(drawing, 0), lineText(drawing, "first"), lineText(drawing, "second")});
        drawing.modify([&](LC_DocumentModificationBatch& ctx) {
            if (std::string(tool) == "rotate") {
                RS_RotateData data;
                data.center = RS_Vector(0.0, -50.0);
                data.angle = M_PI / 6.0;
                data.keepOriginals = true;
                data.multipleCopies = true;
                data.number = 2;
                REQUIRE(RS_Modification::rotate(data, selected, false, ctx));
            }
            else {
                RS_ScaleData data;
                data.referencePoint = RS_Vector(-10.0, 0.0);
                data.factor = RS_Vector(2.0, 2.0);
                data.keepOriginals = true;
                data.multipleCopies = true;
                data.number = 2;
                REQUIRE(RS_Modification::scale(data, selected, false, ctx));
            }
        });

        // In the order of the drawing: the two copies of the block reference,
        // those of NAME, those of NOTE, after what the file had.
        const std::vector<const RS_Insert*> references = drawing.live<RS_Insert>(RS2::EntityInsert);
        const std::vector<const RS_Text*> texts = drawing.live<RS_Text>(RS2::EntityText);
        REQUIRE(references.size() == 3);
        REQUIRE(texts.size() == 8);
        CHECK(references[1]->lineage() != references[0]->lineage());
        CHECK(references[2]->lineage() != references[0]->lineage());
        CHECK(references[2]->lineage() != references[1]->lineage());
        for (const std::size_t copy : {std::size_t{1}, std::size_t{2}}) {
            CAPTURE(copy);
            CHECK(texts[3 + copy]->getText().toStdString() == "first");
            CHECK(texts[3 + copy]->ownerLineage() == references[copy]->lineage());
            CHECK(texts[5 + copy]->getText().toStdString() == "second");
            CHECK(texts[5 + copy]->ownerLineage() == references[copy]->lineage());
        }
    }
}

TEST_CASE("an attribute text without its block reference is written as text",
          "[attribute-export][attrib][filter][dxf][dwg][attrib-owner]") {
    const Bench bench("attr_owner_orphans");
    const std::vector<const char*> formats{"dxf2018", "dxf2007", "dwg2018", "dwg2000"};
    const std::vector<std::string> shown{"first", "second"};

    // A text that shows is kept as a text of its own. The hidden one and the
    // empty one were data of the block reference only: nothing is written
    // for them, and in a DWG 2000 no handle is spent on them.
    SECTION("the block reference erased, then the erase undone") {
        EditableDrawing drawing;
        loadWithHandles(bench, drawing, "erase");
        drawing.erase(referenceAt(drawing, 0));
        for (const auto& file : saveAll(bench, drawing, "erased", formats, "TEXT[first|second] hidden=0 LINE=1")) {
            CHECK(lc::test::dxfRecords(file, "INSERT").empty());
            CHECK(lc::test::dxfRecords(file, "ATTRIB").empty());
            std::vector<std::string> values;
            for (const DxfRecord& record : lc::test::dxfRecords(file, "TEXT")) {
                REQUIRE(record.own(1).size() == 1);
                values.push_back(record.own(1).front());
            }
            CHECK(values == shown);
        }

        REQUIRE(drawing.m_graphic.undo());
        for (const auto& file :
             saveAll(bench, drawing, "erase_undone", formats, "INSERT@0[first|second||secret] TEXT[] hidden=0 LINE=1")) {
            CHECK(attributeTags(file).size() == 4);
            CHECK(lc::test::dxfRecords(file, "TEXT").empty());
        }
    }

    SECTION("the block reference exploded") {
        EditableDrawing drawing;
        loadWithHandles(bench, drawing, "explode");
        const RS_Insert* insert = referenceAt(drawing, 0);
        const_cast<RS_Insert*>(insert)->update();
        drawing.modify([&](LC_DocumentModificationBatch& ctx) {
            REQUIRE(RS_Modification::explode(selection({insert}), ctx));
        });
        // The line of the block is in the drawing now.
        for (const auto& file : saveAll(bench, drawing, "exploded", formats, "TEXT[first|second] hidden=0 LINE=2")) {
            CHECK(lc::test::dxfRecords(file, "INSERT").empty());
            CHECK(lc::test::dxfRecords(file, "ATTRIB").empty());
            CHECK(lc::test::dxfRecords(file, "TEXT").size() == 2);
        }
    }

    SECTION("a text copied without its block reference") {
        EditableDrawing drawing;
        loadWithHandles(bench, drawing, "lone_copy");
        const RS_Insert* insert = referenceAt(drawing, 0);
        const RS_Text* nameText = lineText(drawing, "first");
        moveEntities(drawing, {nameText}, RS_Vector(0.0, -30.0), 1);
        // A later copy of the block reference with its texts does not take
        // the lone copy for one of its own.
        moveEntities(drawing, {insert, nameText, lineText(drawing, "second")}, RS_Vector(50.0, 0.0), 1);
        for (const auto& file : saveAll(bench, drawing, "lone_copy", formats,
                                        "INSERT@0[first|second||secret] INSERT@50[first|second] "
                                        "TEXT[first] hidden=0 LINE=1")) {
            CHECK(attributeTags(file).size() == 6);
            CHECK(lc::test::dxfRecords(file, "TEXT").size() == 1);
        }
    }

    SECTION("a text whose note of its attribute cannot be read") {
        EditableDrawing drawing;
        loadWithHandles(bench, drawing, "unreadable");
        // The first number of the note is the place of the attribute among
        // those of its block reference; there is no place -1.
        drawing.edit(lineText(drawing, "second"), [](RS_Text& text) {
            std::vector<std::shared_ptr<DRW_Variant>> data = text.getDrwExtData();
            bool inNote = false;
            bool changed = false;
            for (auto& value : data) {
                REQUIRE(value != nullptr);
                if (value->code() == 1001) {
                    inNote = std::string(value->c_str()) == kPrivateApplication;
                }
                else if (inNote && value->code() == 1070 && !changed) {
                    value = std::make_shared<DRW_Variant>(1070, -1);
                    changed = true;
                }
            }
            REQUIRE(changed);
            text.setDrwExtData(std::move(data));
        });
        for (const auto& file : saveAll(bench, drawing, "unreadable", formats,
                                        "INSERT@0[first||secret] TEXT[second] hidden=0 LINE=1")) {
            CHECK(attributeTags(file) == std::vector<std::string>{"NAME", "BLANK", "CODE"});
            CHECK(lc::test::dxfRecords(file, "TEXT").size() == 1);
        }
    }

    SECTION("a multi-line attribute text") {
        AttributeDxf fixture;
        fixture.m_handles = true;
        EditableDrawing drawing;
        load(bench, fixture, drawing, "multiline");
        drawing.erase(only<RS_Insert>(drawing, RS2::EntityInsert));
        for (const char* format : formats) {
            const Target to = target(format);
            CAPTURE(to.m_name);
            const auto file = save(bench, drawing, "multiline_erased", to);
            if (to.m_dxf) {
                checkWrittenDxf(file);
                CHECK(lc::test::dxfRecords(file, "INSERT").empty());
                CHECK(lc::test::dxfRecords(file, "ATTRIB").empty());
                CHECK(onlyRecord(file, "MTEXT").own(1) == std::vector<std::string>{"my text\\Psecond line"});
            }
            EditableDrawing reopened;
            REQUIRE(lc::test::importDrawing(reopened.m_graphic, file, to.m_format));
            CHECK(reopened.live<RS_Insert>(RS2::EntityInsert).empty());
            const RS_MText* text = only<RS_MText>(reopened, RS2::EntityMText);
            CHECK(text->ownerLineage() == 0);
            CHECK(text->getText().toStdString() == "my text\nsecond line");
            CHECK(text->getInsertionPoint().distanceTo(RS_Vector(1.0, 4.0)) < 1e-9);
            CHECK(text->getHeight() == 9.0);
            CHECK(text->getVAlign() == RS_MTextData::VATop);
        }
    }

    // Application data of a file is kept as it is read, so a text can carry
    // the private name without ever having been an attribute here. It is a
    // text like any other, and the data stays out of the file all the same.
    SECTION("a text of a file that has data under the private name") {
        EditableDrawing drawing;
        loadDxf(bench,
                "0\nSECTION\n2\nENTITIES\n0\nTEXT\n8\n0\n10\n1\n20\n2\n40\n1\n1\nplain text\n"
                "1001\nLibreCAD_INSERT_ATTRIB\n1070\n0\n0\nENDSEC\n0\nEOF\n",
                drawing, "foreign");
        const RS_Text* text = only<RS_Text>(drawing, RS2::EntityText);
        REQUIRE(text->ownerLineage() == 0);
        REQUIRE(text->hasDrwExtData());
        for (const auto& file : saveAll(bench, drawing, "foreign", formats, "TEXT[plain text] hidden=0 LINE=0")) {
            CHECK(lc::test::dxfRecords(file, "TEXT").size() == 1);
        }
    }
}

TEST_CASE("attributes of a block reference inside a block definition",
          "[attribute-export][attrib][filter][dxf][dwg][attrib-owner]") {
    const Bench bench("attr_owner_nested");
    EditableDrawing drawing;
    loadDxf(bench, nestedAttributedInsertDxf(), drawing);
    RS_Block* outer = drawing.m_graphic.findBlock("OUTER");
    REQUIRE(outer != nullptr);
    REQUIRE(contentOf(*outer) == "INSERT@0[first|second||secret] TEXT[] hidden=0 LINE=0");

    // An edit in the block editor is an undo step of the block.
    RS_Text* nameText = nullptr;
    for (RS_Entity* entity : *outer) {
        if (entity != nullptr && entity->rtti() == RS2::EntityText
            && static_cast<RS_Text*>(entity)->getText() == "first") {
            nameText = static_cast<RS_Text*>(entity);
        }
    }
    REQUIRE(nameText != nullptr);
    auto* edited = static_cast<RS_Text*>(nameText->clone());
    edited->setText("edited");
    edited->update();
    REQUIRE(outer->undoableModify(&drawing.m_viewport, [&](LC_DocumentModificationBatch& ctx) {
        ctx.replace(nameText, edited);
        ctx.dontSetActiveLayerAndPen();
        return true;
    }));
    for (const auto& file : saveAll(bench, drawing, "block_edit", {"dxf2018", "dwg2018"},
                                    "INSERT@0[edited|second||secret] TEXT[] hidden=0 LINE=0", "OUTER")) {
        CHECK(attributeTags(file) == std::vector<std::string>{"NAME", "NOTE", "BLANK", "CODE"});
    }

    // Exploding the reference of OUTER brings copies of the texts into the
    // drawing and no block reference with them. Whether the hidden text of
    // the block shows there is not this case's.
    const RS_Insert* reference = only<RS_Insert>(drawing, RS2::EntityInsert);
    const_cast<RS_Insert*>(reference)->update();
    drawing.modify([&](LC_DocumentModificationBatch& ctx) {
        REQUIRE(RS_Modification::explode(selection({reference}), ctx));
    });
    const auto exploded = save(bench, drawing, "exploded", target("dxf2018"));
    checkWrittenDxf(exploded);
    RS_Graphic reopened;
    REQUIRE(lc::test::importDrawing(reopened, exploded, RS2::FormatDXFRW2018));
    const std::string model = contentOf(reopened);
    CAPTURE(model);
    CHECK(model.find("INSERT@") == std::string::npos);
    std::vector<std::string> values;
    for (const RS_Entity* entity : reopened) {
        if (entity != nullptr && entity->rtti() == RS2::EntityText) {
            values.push_back(static_cast<const RS_Text*>(entity)->getText().toStdString());
        }
    }
    CHECK(std::count(values.begin(), values.end(), "edited") == 1);
    CHECK(std::count(values.begin(), values.end(), "second") == 1);
    CHECK(std::count(values.begin(), values.end(), "first") == 0);
}

TEST_CASE("texts sharing a source handle get handles of their own",
          "[attribute-export][attrib][filter][dxf][dwg][attrib-owner]") {
    const Bench bench("attr_owner_shared_handle");
    EditableDrawing drawing;
    loadWithHandles(bench, drawing, "fixture");
    const RS_Text* nameText = lineText(drawing, "first");

    // A clone put into the drawing without an undo step: two texts that show,
    // with one handle and one place among the attributes.
    RS_Entity* twin = nameText->clone();
    drawing.m_graphic.addEntity(twin);
    REQUIRE(twin->sourceHandle() == nameText->sourceHandle());

    // checkWrittenDxf() looks for a handle that two records have.
    for (const auto& file : saveAll(bench, drawing, "twin", {"dxf2018", "dxf2007", "dwg2018"},
                                    "INSERT@0[first|first|second||secret] TEXT[] hidden=0 LINE=1")) {
        CHECK(attributeTags(file) == std::vector<std::string>{"NAME", "NAME", "NOTE", "BLANK", "CODE"});
    }
}

TEST_CASE("pasted attribute texts stay with the pasted block reference",
          "[attribute-export][attrib][filter][dxf][dwg][attrib-owner]") {
    const Bench bench("attr_owner_paste");
    EditableDrawing source;
    loadWithHandles(bench, source, "fixture");
    QList<RS_Entity*> copied =
        selection({referenceAt(source, 0), lineText(source, "first"), lineText(source, "second")});
    source.m_graphic.select(copied, true);
    LC_CopyUtils::copy(RS_Vector(0.0, 0.0), copied, &source.m_graphic);
    source.m_graphic.select(copied, false);

    // Twice into another drawing: each paste is a block reference with its
    // two texts, new to that drawing.
    EditableDrawing drawing;
    for (const double x : {0.0, 50.0}) {
        drawing.modify([&](LC_DocumentModificationBatch& ctx) {
            LC_CopyUtils::paste(LC_CopyUtils::RS_PasteData(RS_Vector(x, 0.0)), &drawing.m_graphic, ctx);
        });
    }
    REQUIRE(drawing.live<RS_Insert>(RS2::EntityInsert).size() == 2);
    REQUIRE(drawing.live<RS_Text>(RS2::EntityText).size() == 4);
    for (const RS_Entity* entity : drawing.m_graphic) {
        REQUIRE(entity != nullptr);
        CHECK(entity->sourceHandle() == 0);
    }
    CHECK(referenceAt(drawing, 0)->lineage() != referenceAt(drawing, 50)->lineage());

    drawing.edit(lineText(drawing, "first", 51.0), [](RS_Text& text) { text.setText("pasted"); });
    for (const auto& file : saveAll(bench, drawing, "pasted", {"dxf2018", "dwg2018"},
                                    "INSERT@0[first|second] INSERT@50[pasted|second] TEXT[] hidden=0 LINE=0")) {
        CHECK(attributeTags(file).size() == 4);
        CHECK(lc::test::dxfRecords(file, "TEXT").empty());
    }
}

// An attribute can be annotative. Its MTEXT then has data for the annotation
// scales, which only a DWG 2018 holds and which is kept here as it was read.
// It goes back into a DWG 2018 next to a text that is as it was read, and no
// save is refused over it: what stays behind is named in the export report.

TEST_CASE("an untouched annotative attribute keeps its scale data in DWG 2018",
          "[attribute-export][attrib][filter][dxf][dwg][annotative]") {
    const Bench bench("attr_scale_untouched");
    int variant = 0;
    // A turned text too: the angle the drawing shows of it is not always
    // the number the file has, and must not pass for a change.
    for (const double angle : {0.0, 0.7}) {
        const std::string name = "angle" + std::to_string(variant++);
        CAPTURE(name);
        AttributeDwgSource source;
        source.m_angle = angle;
        const std::filesystem::path file = bench.m_out.file(name + "_source.dwg");
        source.write(file);

        for (const char* format : kScaleDataFormats) {
            const Target to = target(format);
            CAPTURE(to.m_name);
            EditableDrawing drawing;
            REQUIRE(lc::test::importDrawing(drawing.m_graphic, file, RS2::FormatDWG2018));
            REQUIRE(only<RS_MText>(drawing, RS2::EntityMText)->getAngle() == Catch::Approx(angle).margin(1e-9));
            const Saved saved = saveWithReport(bench, drawing, name, to);
            if (to.m_embedsMText && !to.m_dxf) {
                checkScaleDataKept(saved, source);
            }
            else {
                checkScaleDataLeftOut(saved, to);
            }
        }
    }
}

TEST_CASE("annotative attribute data that names no application of the drawing is left out",
          "[attribute-export][attrib][filter][dwg][annotative]") {
    const Bench bench("attr_scale_no_app");
    AttributeDwgSource source;
    source.m_dataNamesNoApplication = true;
    const std::filesystem::path file = bench.m_out.file("source.dwg");
    source.write(file);

    // The data names its application by a handle of the file it was read
    // from. Where that is no application the drawing has, there is nothing
    // the data could name in the saved file.
    EditableDrawing drawing;
    REQUIRE(lc::test::importDrawing(drawing.m_graphic, file, RS2::FormatDWG2018));
    const Target to = target("dwg2018");
    checkScaleDataLeftOut(saveWithReport(bench, drawing, "untouched", to), to);
}

TEST_CASE("an annotative attribute changed on the text itself is saved without its scale data",
          "[attribute-export][attrib][filter][dxf][dwg][annotative]") {
    const Bench bench("attr_scale_in_place");
    checkEditedAnnotativeAttribute(bench, true);
}

TEST_CASE("an annotative attribute edited the way the application edits is saved without its scale data",
          "[attribute-export][attrib][filter][dxf][dwg][annotative]") {
    const Bench bench("attr_scale_edited");
    checkEditedAnnotativeAttribute(bench, false);
}

TEST_CASE("undoing the edit of an annotative attribute restores its scale data",
          "[attribute-export][attrib][filter][dxf][dwg][annotative]") {
    const Bench bench("attr_scale_undone");
    AttributeDwgSource source;
    source.m_angle = 0.7;
    const std::filesystem::path file = bench.m_out.file("source.dwg");
    source.write(file);

    EditableDrawing drawing;
    REQUIRE(lc::test::importDrawing(drawing.m_graphic, file, RS2::FormatDWG2018));
    drawing.edit(only<RS_MText>(drawing, RS2::EntityMText), [](RS_MText& text) { text.setText("changed\ntext"); });
    REQUIRE(drawing.m_graphic.undo());
    REQUIRE(only<RS_MText>(drawing, RS2::EntityMText)->getText().toStdString() == "line 1\nline 2");

    for (const char* format : kScaleDataFormats) {
        const Target to = target(format);
        CAPTURE(to.m_name);
        const Saved saved = saveWithReport(bench, drawing, "undone", to);
        if (to.m_embedsMText && !to.m_dxf) {
            checkScaleDataKept(saved, source);
        }
        else {
            checkScaleDataLeftOut(saved, to);
        }
    }
}

TEST_CASE("a pasted annotative attribute is saved without its scale data",
          "[attribute-export][attrib][filter][dwg][annotative]") {
    const Bench bench("attr_scale_pasted");
    AttributeDwgSource source;
    const std::filesystem::path file = bench.m_out.file("source.dwg");
    source.write(file);

    EditableDrawing from;
    REQUIRE(lc::test::importDrawing(from.m_graphic, file, RS2::FormatDWG2018));
    QList<RS_Entity*> copied =
        selection({only<RS_Insert>(from, RS2::EntityInsert), only<RS_MText>(from, RS2::EntityMText)});
    from.m_graphic.select(copied, true);
    LC_CopyUtils::copy(RS_Vector(0.0, 0.0), copied, &from.m_graphic);
    from.m_graphic.select(copied, false);

    // Pasted where it was copied from, in place of the attribute of a drawing
    // that was opened from the same file: the text is as it was read, and the
    // handle its data names is the same application here. But a copy may as
    // well come from a drawing in which that handle is something else, and
    // nothing tells the two apart: a copy has no handle of a file.
    EditableDrawing drawing;
    REQUIRE(lc::test::importDrawing(drawing.m_graphic, file, RS2::FormatDWG2018));
    const RS_Vector place = only<RS_MText>(drawing, RS2::EntityMText)->getInsertionPoint();
    drawing.erase(only<RS_MText>(drawing, RS2::EntityMText));
    drawing.erase(only<RS_Insert>(drawing, RS2::EntityInsert));
    drawing.modify([&](LC_DocumentModificationBatch& ctx) {
        LC_CopyUtils::paste(LC_CopyUtils::RS_PasteData(RS_Vector(0.0, 0.0)), &drawing.m_graphic, ctx);
    });
    const RS_MText* pasted = only<RS_MText>(drawing, RS2::EntityMText);
    REQUIRE(pasted->sourceHandle() == 0);
    REQUIRE(pasted->ownerLineage() == only<RS_Insert>(drawing, RS2::EntityInsert)->lineage());
    REQUIRE(pasted->getInsertionPoint().distanceTo(place) < 1e-12);
    REQUIRE(pasted->getText().toStdString() == "line 1\nline 2");

    const Target to = target("dwg2018");
    checkScaleDataLeftOut(saveWithReport(bench, drawing, "pasted", to), to);
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
