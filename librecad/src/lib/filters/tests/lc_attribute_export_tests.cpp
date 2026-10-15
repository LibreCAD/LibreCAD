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
#include "rs_layer.h"
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

// The DXF formats that only the cases about the value of an attribute save
// to. None of them has an MTEXT in an attribute, and the oldest has no MTEXT
// at all.
const Target kOtherDxfTargets[] = {
    {RS2::FormatDXFRW12, "dxf12", ".dxf", true, false},     {RS2::FormatDXFRW14, "dxf14", ".dxf", true, false},
    {RS2::FormatDXFRW2004, "dxf2004", ".dxf", true, false}, {RS2::FormatDXFRW2010, "dxf2010", ".dxf", true, false},
    {RS2::FormatDXFRW2013, "dxf2013", ".dxf", true, false},
};

const Target& target(const std::string& name) {
    const auto named = [&](const Target& t) { return name == t.m_name; };
    const auto found = std::find_if(std::begin(kTargets), std::end(kTargets), named);
    if (found != std::end(kTargets)) {
        return *found;
    }
    const auto other = std::find_if(std::begin(kOtherDxfTargets), std::end(kOtherDxfTargets), named);
    REQUIRE(other != std::end(kOtherDxfTargets));
    return *other;
}

// Every format that has no MTEXT in an attribute: twelve, seven of DXF and
// five of DWG.
std::vector<Target> targetsWithoutMText() {
    std::vector<Target> targets;
    for (const Target& t : kOtherDxfTargets) {
        targets.push_back(t);
    }
    for (const Target& t : kTargets) {
        if (!t.m_embedsMText) {
            targets.push_back(t);
        }
    }
    return targets;
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
    std::string m_commonGroups;                     // after the layer of the attribute: 60
    std::string m_flags = "0";                      // group 70; 1: the attribute is invisible
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
        dxf += "100\nAcDbEntity\n8\n0\n" + m_commonGroups + "100\nAcDbText\n" + m_firstPoint + "40\n" + m_height
               + "\n1\n" + m_value + "\n" + m_textExtra + style7 + m_alignmentPoint + m_textTail;
        dxf += "100\nAcDbAttribute\n280\n0\n2\n" + m_tag + "\n70\n" + m_flags + "\n74\n" + m_verticalJustification
               + "\n280\n1\n";
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
// spacing style the MTEXT has no group 73. Its direction, where the case
// gives one, is where AutoCAD writes it: an x-axis (11, 21, 31) or an angle
// (50) after the text.
std::string standaloneMTextDxf(const std::string& spacingStyle, const std::string& direction = {}) {
    const std::string group73 = spacingStyle.empty() ? std::string() : "73\n" + spacingStyle + "\n";
    return "0\nSECTION\n2\nENTITIES\n0\nMTEXT\n100\nAcDbEntity\n8\n0\n100\nAcDbMText\n"
           "10\n1\n20\n4\n30\n0\n40\n9\n41\n80\n71\n1\n72\n5\n1\nmy text\\Psecond line\n"
           + direction + group73 + "44\n1.0\n0\nENDSEC\n0\nEOF\n";
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
    // A text with nothing to draw has no box, before and after.
    CHECK(actual.getMin().valid == expected.getMin().valid);
    CHECK(actual.getMax().valid == expected.getMax().valid);
    if (expected.getMin().valid && expected.getMax().valid) {
        CHECK(actual.getMin().distanceTo(expected.getMin()) < 1e-6);
        CHECK(actual.getMax().distanceTo(expected.getMax()) < 1e-6);
    }
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
    for (const int code : {7, 90, 45, 63, 421, 441}) {
        CAPTURE(code);
        if (!source.embedded(code).empty()) {
            CHECK(lc::test::sameValues(source.embedded(code), record.embedded(code)));
        }
    }
}

// What libdxfrw reads of a DWG, without the filter in between: the block
// references with their attributes, the texts of their own, and the handle
// of every registered application, layer, linetype and text style by its name.
struct DwgContent : lc::test::NullDrwInterface {
    std::vector<DRW_Insert> m_inserts;
    std::vector<DRW_Text> m_texts;
    std::vector<DRW_MText> m_mtexts;
    std::map<std::string, std::uint32_t> m_applications;
    std::map<std::string, std::uint32_t> m_layers;
    std::map<std::string, std::uint32_t> m_lineTypes;
    std::map<std::string, std::uint32_t> m_textStyles;
    void addInsert(const DRW_Insert& insert) override { m_inserts.push_back(insert); }
    void addText(const DRW_Text& text) override { m_texts.push_back(text); }
    void addMText(const DRW_MText& text) override { m_mtexts.push_back(text); }
    void addAppId(const DRW_AppId& application) override { m_applications[application.name] = application.handle; }
    void addLayer(const DRW_Layer& layer) override { m_layers[layer.name] = layer.handle; }
    void addLType(const DRW_LType& lineType) override { m_lineTypes[lineType.name] = lineType.handle; }
    void addTextStyle(const DRW_Textstyle& style) override { m_textStyles[style.name] = style.handle; }
};

// The handle of the record with a name, whatever its letter case; 0 when
// there is none.
std::uint32_t handleNamed(const std::map<std::string, std::uint32_t>& records, const std::string& name) {
    for (const auto& [recordName, handle] : records) {
        if (QString::fromStdString(recordName).compare(QString::fromStdString(name), Qt::CaseInsensitive) == 0) {
            return handle;
        }
    }
    return 0;
}

void readDwg(const std::filesystem::path& file, DwgContent& content) {
    CAPTURE(file.filename().string());
    dwgRW reader(QFile::encodeName(lc::test::toQString(file)).constData());
    REQUIRE(reader.read(&content, false));
}

// A DWG lists the applications that have data in it. The private names have
// none there, and a name in the list would be handed on to every file saved
// from the drawing that the DWG opens as.
void checkNoPrivateApplication(const DwgContent& content) {
    for (const auto& [name, handle] : content.m_applications) {
        CAPTURE(name, handle);
        const QString listed = QString::fromStdString(name);
        CHECK_FALSE(listed.startsWith(QLatin1String(kPrivateApplication), Qt::CaseInsensitive));
    }
}

// The one attribute of the one block reference of a DWG.
const DRW_Attrib& onlyDwgAttribute(const DwgContent& content) {
    REQUIRE(content.m_inserts.size() == 1);
    REQUIRE(content.m_inserts.front().attlist.size() == 1);
    REQUIRE(content.m_inserts.front().attlist.front() != nullptr);
    return *content.m_inserts.front().attlist.front();
}

// The MTEXT of that attribute.
const DRW_MText& onlyEmbeddedMText(const DwgContent& content) {
    const DRW_Attrib& attribute = onlyDwgAttribute(content);
    REQUIRE(attribute.mtext != nullptr);
    return *attribute.mtext;
}

// The attribute's own value in a saved file, as the file holds it: group 1 of
// the ATTRIB itself in a DXF, the text value of the ATTRIB in a DWG. It is
// read without the filter, which would decode it as it decodes a text.
std::string attributeValueIn(const std::filesystem::path& file, const Target& of) {
    CAPTURE(file.filename().string());
    if (of.m_dxf) {
        const std::vector<std::string> value = onlyRecord(file, "ATTRIB").own(1);
        REQUIRE(value.size() == 1);
        return value.front();
    }
    DwgContent content;
    readDwg(file, content);
    return onlyDwgAttribute(content).text;
}

// Two characters as UTF-8, which is what a text of a file or of the drawing
// is compared as here. The bytes are spelled out: a compiler need not encode
// a string literal that way.
const char* const kDegreeSign = "\xC2\xB0";
const char* const kCapitalOmega = "\xCE\xA9";

// What a saved file shows of its one attribute when it is opened.
struct ShownAttribute {
    bool m_multiLine = false;
    std::string m_text;
    double m_height = 0.0;
    RS_Vector m_at;
};

ShownAttribute shownAttribute(const std::filesystem::path& file, const Target& from) {
    CAPTURE(file.filename().string());
    EditableDrawing reopened;
    REQUIRE(lc::test::importDrawing(reopened.m_graphic, file, from.m_format));
    REQUIRE(reopened.live<RS_Insert>(RS2::EntityInsert).size() == 1);
    const RS_Entity* entity = attributeText(reopened);
    if (entity->rtti() == RS2::EntityMText) {
        const auto* text = static_cast<const RS_MText*>(entity);
        return {true, text->getText().toStdString(), text->getHeight(), text->getInsertionPoint()};
    }
    const auto* text = static_cast<const RS_Text*>(entity);
    return {false, text->getText().toStdString(), text->getHeight(), text->getInsertionPoint()};
}

// What a text keeps of its ATTRIB is application data on the text, in groups
// under the private names: of the attribute itself a row of numbers, then
// the tag and the value as strings; of its MTEXT, among others, the text
// style and then the content as strings. These put a value there that no
// DXF of these cases can: a part is found by its group, its code and its
// place among the values of that code.
bool setStoredAttributePart(RS_Entity& text, const std::string& group, const int code, const int place,
                            const std::shared_ptr<DRW_Variant>& part) {
    std::vector<std::shared_ptr<DRW_Variant>> data = text.getDrwExtData();
    bool inGroup = false;
    int seen = 0;
    for (auto& value : data) {
        if (value == nullptr) {
            continue;
        }
        if (value->code() == 1001) {
            inGroup = group == value->c_str();
        }
        else if (inGroup && value->code() == code && seen++ == place) {
            value = part;
            text.setDrwExtData(std::move(data));
            return true;
        }
    }
    return false;
}

// The attribute's own value as the file had it.
bool setStoredAttributeValue(RS_Entity& text, const std::string& stored) {
    return setStoredAttributePart(text, kPrivateApplication, 1000, 1, std::make_shared<DRW_Variant>(1000, stored));
}

// The attribute type: 2 for a multi-line attribute.
bool setStoredAttributeType(RS_Entity& text, const int type) {
    return setStoredAttributePart(text, kPrivateApplication, 1070, 4, std::make_shared<DRW_Variant>(1070, type));
}

// The content of the MTEXT as the file had it, for a content of one string.
bool setStoredMTextContent(RS_Entity& text, const std::string& stored) {
    return setStoredAttributePart(text, std::string(kPrivateApplication) + "_MTEXT", 1000, 1,
                                  std::make_shared<DRW_Variant>(1000, stored));
}

// The column layout of the MTEXT: 0 for none, 1 for columns of one height.
bool setStoredMTextColumnType(RS_Entity& text, const int type) {
    return setStoredAttributePart(text, std::string(kPrivateApplication) + "_MTEXT", 1070, 10,
                                  std::make_shared<DRW_Variant>(1070, type));
}

// The annotation-scale data of the attribute that AttributeDwgSource writes,
// and the name of the application the data belongs to. What the bytes mean
// is not known here; a DWG 2018 holds them next to the MTEXT of a multi-line
// attribute, and no other format has a place for them.
const std::vector<std::uint8_t> kScaleData{0x01, 0x7F, 0xFE};
constexpr std::uint16_t kScaleDataShort = 0x72;
const char* const kScaleApplication = "LC_SCALE_APP";
// The handle of the application ACAD, which is the same in every DWG.
constexpr std::uint32_t kAcadApplication = 0x14u;
// What an export says when it left such data out, or a column layout, which
// is kept and left out by the same rule.
const char* const kScaleDataLeftOut = "annotation scale or column data of 1 multi-line attribute(s)";

// The layer and the text style of the attribute that AttributeDwgSource
// writes with references, and the handles its MTEXT refers by. A DWG names
// every record by a handle, a number of that one file. The embedded layer
// and linetype are real, distinct records of the source; the other foreign
// references are not registered deliberately by this fixture. None grants an
// association with the same number in a file saved from the source.
const char* const kAttributeLayer = "LCATTR";
const char* const kAttributeStyle = "LCNOTE";
const char* const kEmbeddedSourceLayer = "LCBODY";
const char* const kEmbeddedSourceLinetype = "LCBODY_DASH";
constexpr std::uint32_t kForeignOwner = 0x3Fu;
const std::vector<std::uint32_t> kForeignReactors{0x31u, 0x32u};
constexpr std::uint32_t kForeignDictionary = 0x33u;
constexpr std::uint32_t kForeignColor = 0x35u;
constexpr std::uint32_t kForeignMaterial = 0x36u;
constexpr std::uint32_t kForeignShadow = 0x37u;
constexpr std::uint32_t kForeignPlotStyle = 0x38u;
constexpr std::uint32_t kForeignVisualStyle = 0x39u;    // and the two after it
constexpr std::uint32_t kForeignApplication = 0x3Eu;
constexpr int kMTextTransparency = 0x03000080;

// Writes a DWG 2018 with one block reference and one multi-line attribute
// through libdxfrw: no DXF can hold what the MTEXT of the attribute has
// here, and no drawing of another program is copied. That is annotation-scale
// data, or, with references, every reference to another record that the
// MTEXT inside an attribute can have. The value of the attribute itself is
// not empty, so that the cases on this source do not depend on what a format
// without the MTEXT gets as the value.
struct AttributeDwgSource : lc::test::NullDrwInterface {
    double m_angle = 0.0;                   // of the text, in radians
    bool m_dataNamesNoApplication = false;  // the data names a handle that no record of the file has
    bool m_dataNamesAcad = false;           // the data names ACAD, not the application of its own
    bool m_references = false;              // no scale data: a layer, a text style and the references
    std::uint32_t m_scaleApplication = 0;   // after write(): the handle of LC_SCALE_APP in the file
    std::uint32_t m_layer = 0;              // with references: the handle of the layer in the file
    std::uint32_t m_style = 0;              // and of the text style
    std::uint32_t m_embeddedLayer = 0;      // distinct source records used by the embedded MTEXT
    std::uint32_t m_embeddedLinetype = 0;

    void write(const std::filesystem::path& file) {
        CAPTURE(file.filename().string());
        dwgRW writer(QFile::encodeName(lc::test::toQString(file)).constData());
        m_writer = &writer;
        const bool written = writer.write(this, DRW::AC1032, false);
        m_writer = nullptr;
        REQUIRE(written);
        REQUIRE(m_insertWritten);
        REQUIRE(m_scaleApplication != 0);
        REQUIRE((m_layer != 0) == m_references);
        REQUIRE((m_style != 0) == m_references);
        REQUIRE((m_embeddedLayer != 0) == m_references);
        REQUIRE((m_embeddedLinetype != 0) == m_references);
        if (m_references) {
            REQUIRE(m_embeddedLayer != m_layer);
        }
    }

    // The callbacks only note what went wrong: libdxfrw calls them, and an
    // exception of a failed REQUIRE would not get past it.
    void writeLTypes() override {
        DRW_LType lineType;
        lineType.updateValues(kEmbeddedSourceLinetype, "Embedded MTEXT source", 2, 1.0, {0.5, -0.5});
        if (m_references && m_writer != nullptr && m_writer->addLType(&lineType)) {
            m_embeddedLinetype = lineType.handle;
        }
    }
    void writeLayers() override {
        DRW_Layer layer;
        layer.name = kAttributeLayer;
        if (m_references && m_writer != nullptr && m_writer->addLayer(&layer)) {
            m_layer = layer.handle;
        }
        DRW_Layer embeddedLayer;
        embeddedLayer.name = kEmbeddedSourceLayer;
        if (m_references && m_writer != nullptr && m_writer->addLayer(&embeddedLayer)) {
            m_embeddedLayer = embeddedLayer.handle;
        }
    }
    void writeTextstyles() override {
        DRW_Textstyle style;
        style.name = kAttributeStyle;
        style.font = "txt";
        if (m_references && m_writer != nullptr && m_writer->addTextstyle(&style)) {
            m_style = style.handle;
        }
    }
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
        if (m_references) {
            attribute->layer = kAttributeLayer;
            attribute->styleH.ref = m_style;
            mtext.styleH.ref = m_style;
            mtext.m_r2018IsNotAnnotative = true;
            mtext.m_r2018AppIdHandle = kForeignApplication;
            mtext.parentHandle = kForeignOwner;
            mtext.reactorHandles = kForeignReactors;
            mtext.xDictHandle = kForeignDictionary;
            mtext.setHasDataStorageBinaryData(true);
            // A colour of a colour book: the MTEXT names the record of the
            // colour, and the number in its place is no colour (256 is "by
            // layer").
            mtext.color = 256;
            mtext.colorName = "Book$Entry";
            mtext.setDwgAcDbColorHandle(true, kForeignColor);
            mtext.transparency = kMTextTransparency;
            mtext.ltypeScale = 1.25;
            mtext.lWeight = DRW_LW_Conv::width05;
            mtext.material = kForeignMaterial;
            mtext.shadow = DRW::IgnoreShadows;
            mtext.shadowHandle = kForeignShadow;
            mtext.plotStyle = static_cast<int>(kForeignPlotStyle);
            mtext.fullVisualStyleHandle = kForeignVisualStyle;
            mtext.faceVisualStyleHandle = kForeignVisualStyle + 1;
            mtext.edgeVisualStyleHandle = kForeignVisualStyle + 2;
            mtext.setDwgLayerHandle(m_embeddedLayer);
            mtext.lineType.clear();
            mtext.setDwgLinetypeHandle(m_embeddedLinetype);
        }
        else {
            mtext.m_r2018IsNotAnnotative = false;
            mtext.m_r2018AnnotativeData = kScaleData;
            mtext.m_r2018AnnotativeUnknown = kScaleDataShort;
            mtext.m_r2018AnnotativeAppHandle = m_scaleApplication;
            if (m_dataNamesNoApplication) {
                mtext.m_r2018AnnotativeAppHandle = 0x7F0u;
            }
            else if (m_dataNamesAcad) {
                mtext.m_r2018AnnotativeAppHandle = kAcadApplication;
            }
        }
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
    CHECK(saved.m_report.find("annotation scale") == std::string::npos);
}

// A save that left the data out says so, and has the block reference and
// its attribute all the same. In a DWG 2018 the MTEXT is then one without
// annotation scales, as AutoCAD writes such a one: the mark "annotative" with
// no data after it is no form that AutoCAD writes.
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
        CHECK(mtext.m_r2018IsNotAnnotative);
        CHECK(mtext.m_r2018Version == 4);
        CHECK(mtext.m_r2018DefaultFlag);
        CHECK(mtext.m_r2018AppIdHandle == 0u);
        CHECK(mtext.m_r2018ColumnType == 0);
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
// handles, every record has one, as in a file of another program. The first
// attribute can be given groups after its handle: its dictionary, its reactors.
std::string attributedInsertRecords(const bool handles, const std::string& ofFirstAttribute = {}) {
    const auto handle = [handles](const char* value) {
        return handles ? std::string("5\n") + value + "\n" : std::string();
    };
    return "0\nINSERT\n" + handle("1A0") + "8\n0\n2\nTAG\n10\n0\n20\n0\n66\n1\n"
           "0\nATTRIB\n" + handle("1A1") + ofFirstAttribute + "8\n0\n10\n1\n20\n2\n40\n1\n1\nfirst\n2\nNAME\n70\n0\n"
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
// taken and not used would lie inside the chain of a DWG 2000. A section of
// objects can follow the entities.
std::string attributedInsertDxf(const bool handles, const std::string& ofFirstAttribute = {},
                                const std::string& objects = {}) {
    return "0\nSECTION\n2\nBLOCKS\n" + tagBlockDxf(handles) + "0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"
           + attributedInsertRecords(handles, ofFirstAttribute) + "0\nLINE\n" + (handles ? "5\n1A6\n" : "")
           + "8\n0\n10\n20\n20\n0\n11\n30\n21\n0\n0\nENDSEC\n" + objects + "0\nEOF\n";
}

// The same block reference inside the block OUTER, which the drawing inserts once.
std::string nestedAttributedInsertDxf(const std::string& ofFirstAttribute = {}, const std::string& objects = {}) {
    return "0\nSECTION\n2\nBLOCKS\n" + tagBlockDxf(false)
           + "0\nBLOCK\n8\n0\n2\nOUTER\n70\n0\n10\n0\n20\n0\n3\nOUTER\n"
           + attributedInsertRecords(false, ofFirstAttribute)
           + "0\nENDBLK\n8\n0\n0\nENDSEC\n"
             "0\nSECTION\n2\nENTITIES\n0\nINSERT\n8\n0\n2\nOUTER\n10\n0\n20\n0\n0\nENDSEC\n"
           + objects + "0\nEOF\n";
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

// The record of an attribute of one line in AutoCAD's group order: the first
// point, the height and the value, then what the case puts into the text
// part of the record (71, 72 and the alignment point, as it needs them), and
// in the attribute part the tag and the vertical justification.
std::string singleLineAttrib(const std::string& tag, const std::string& x, const std::string& y,
                             const std::string& height, const std::string& value, const std::string& textGroups,
                             const std::string& verticalJustification = "0") {
    return "0\nATTRIB\n100\nAcDbEntity\n8\n0\n100\nAcDbText\n10\n" + x + "\n20\n" + y + "\n30\n0\n40\n" + height
           + "\n1\n" + value + "\n7\niso3098\n" + textGroups + "100\nAcDbAttribute\n280\n0\n2\n" + tag
           + "\n70\n0\n74\n" + verticalJustification + "\n280\n1\n";
}

// A block reference with such attributes.
std::string singleLineAttributesDxf(const std::vector<std::string>& attributes) {
    std::string dxf = "0\nSECTION\n2\nBLOCKS\n0\nBLOCK\n8\n0\n2\nTITLE\n70\n0\n10\n0\n20\n0\n3\nTITLE\n"
                      "0\nENDBLK\n8\n0\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"
                      "0\nINSERT\n100\nAcDbEntity\n8\n0\n100\nAcDbBlockReference\n66\n1\n2\nTITLE\n"
                      "10\n0\n20\n0\n30\n0\n";
    for (const std::string& attribute : attributes) {
        dxf += attribute;
    }
    return dxf + "0\nSEQEND\n100\nAcDbEntity\n8\n0\n0\nENDSEC\n0\nEOF\n";
}

// The text of one line the drawing shows with a value, wherever it is.
const RS_Text* textWithValue(EditableDrawing& drawing, const std::string& value) {
    const RS_Text* found = nullptr;
    for (const RS_Text* text : drawing.live<RS_Text>(RS2::EntityText)) {
        if (text->getText().toStdString() == value) {
            REQUIRE(found == nullptr);
            found = text;
        }
    }
    CAPTURE(value);
    REQUIRE(found != nullptr);
    return found;
}

// The groups of the attribute itself with these codes as numbers, one after
// another; a code the record does not have gives none. QString::toDouble()
// does not follow the locale.
std::vector<double> ownNumbers(const DxfRecord& record, const std::vector<int>& codes) {
    std::vector<double> numbers;
    for (const int code : codes) {
        for (const std::string& value : record.own(code)) {
            bool isNumber = false;
            numbers.push_back(QString::fromStdString(value).toDouble(&isNumber));
            CAPTURE(code, value);
            REQUIRE(isNumber);
        }
    }
    return numbers;
}

// The same of the groups of its MTEXT.
std::vector<double> embeddedNumbers(const DxfRecord& record, const std::vector<int>& codes) {
    std::vector<double> numbers;
    for (const int code : codes) {
        for (const std::string& value : record.embedded(code)) {
            bool isNumber = false;
            numbers.push_back(QString::fromStdString(value).toDouble(&isNumber));
            CAPTURE(code, value);
            REQUIRE(isNumber);
        }
    }
    return numbers;
}

// True when the lists are as long as each other and no pair is further apart
// than 1e-9: a point of a file is where it should be, not near it.
bool sameNumbers(const std::vector<double>& expected, const std::vector<double>& actual) {
    if (expected.size() != actual.size()) {
        return false;
    }
    for (std::size_t i = 0; i < expected.size(); ++i) {
        if (!(std::abs(expected[i] - actual[i]) <= 1e-9)) {
            return false;
        }
    }
    return true;
}

// True when two angles, in radians, are one direction.
bool sameDirection(const double a, const double b) {
    return std::abs(std::remainder(a - b, 2.0 * M_PI)) < 1e-9;
}

// The first point and the alignment point of the attribute itself in a
// record: 10, 20, 11, 21. Next to its MTEXT a record has the alignment point
// a second time, in front of the MTEXT, and the two have to be one point.
std::vector<double> ownPlace(const DxfRecord& record) {
    const std::vector<double> first = ownNumbers(record, {10, 20});
    const std::vector<double> x = ownNumbers(record, {11});
    const std::vector<double> y = ownNumbers(record, {21});
    REQUIRE(first.size() == 2);
    REQUIRE(!x.empty());
    REQUIRE(x.size() == y.size());
    for (std::size_t i = 1; i < x.size(); ++i) {
        CHECK(x[i] == x.front());
        CHECK(y[i] == y.front());
    }
    return {first[0], first[1], x.front(), y.front()};
}

// Group 72 of the attribute itself as a record has to have it: the
// horizontal justification when the text is not left-aligned, and next to an
// MTEXT a second 72, a 0 that goes with the attribute type.
std::vector<std::string> ownGroup72(const char* justification, const Target& to) {
    std::vector<std::string> values;
    if (*justification != '\0') {
        values.emplace_back(justification);
    }
    if (to.m_embedsMText) {
        values.emplace_back("0");
    }
    return values;
}

// How often a record names the application "LibreCad", under which the
// drawing notes a right-to-left text: the formats have no such direction.
int rightToLeftNotes(const DxfRecord& record) {
    return static_cast<int>(std::count_if(record.m_groups.begin(), record.m_groups.end(),
                                          [](const lc::test::DxfGroup& group) {
                                              return group.m_code == 1001 && group.m_value == "LibreCad";
                                          }));
}

// The same of the application data of an entity read from a DWG.
int rightToLeftNotes(const DRW_Entity& entity) {
    return static_cast<int>(std::count_if(entity.extData.begin(), entity.extData.end(),
                                          [](const std::shared_ptr<DRW_Variant>& value) {
                                              return value != nullptr && value->code() == 1001
                                                     && std::string("LibreCad") == value->c_str();
                                          }));
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
        // The file that LibreCAD wrote, opened and saved with nothing
        // changed, repeats the source as well.
        const auto again = save(bench, reopened, std::string(preset.m_name) + "_again", target("dxf2018"));
        checkWrittenDxf(again);
        checkRepeatsSource(source, again);

        // The same through a DWG: its content is read here as the drawing it
        // opens as, and as the DXF that drawing is saved to.
        const auto dwg = save(bench, drawing, preset.m_name, target("dwg2018"));
        DwgContent written;
        readDwg(dwg, written);
        checkNoPrivateApplication(written);
        EditableDrawing fromDwg;
        REQUIRE(lc::test::importDrawing(fromDwg.m_graphic, dwg, RS2::FormatDWG2018));
        checkSameMText(*imported, *only<RS_MText>(fromDwg, RS2::EntityMText));
        const auto viaDwg = save(bench, fromDwg, std::string(preset.m_name) + "_dwg2018", target("dxf2018"));
        checkWrittenDxf(viaDwg);
        checkRepeatsSource(source, viaDwg);
    }
}

// The background of an MTEXT has a colour by number (group 63) and can have
// a true colour next to it (group 421), which is the colour then. AutoCAD
// writes the nearest numbered colour with it; ezdxf writes 0.
TEST_CASE("the background of an attribute's MTEXT keeps its true colour",
          "[attribute-export][attrib][filter][dxf][keep]") {
    const Bench bench("attr_background");
    struct Variant {
        const char* m_name;
        const char* m_background;
        std::vector<std::string> m_number;
        std::vector<std::string> m_trueColour;
    };
    const std::vector<Variant> variants{
        {"autocad", "90\n1\n63\n97\n421\n3568962\n45\n1.25\n441\n0\n", {"97"}, {"3568962"}},
        {"ezdxf", "45\n1.5\n90\n1\n63\n0\n421\n16711680\n", {"0"}, {"16711680"}},
        // Without a true colour none is written, and the attribute is as
        // much a multi-line one as the others.
        {"number_only", "90\n1\n63\n97\n45\n1.25\n441\n0\n", {"97"}, {}},
    };
    const Target to = target("dxf2018");
    for (const Variant& variant : variants) {
        const std::string name = variant.m_name;
        CAPTURE(name);
        AttributeDxf fixture;
        fixture.m_mtextTail = variant.m_background;
        EditableDrawing drawing;
        const DxfRecord source = onlyRecord(load(bench, fixture, drawing, name + "_source"), "ATTRIB");
        REQUIRE(source.embedded(421) == variant.m_trueColour);
        const auto checkBackground = [&](const std::filesystem::path& file) {
            checkWrittenDxf(file);
            const DxfRecord record = onlyRecord(file, "ATTRIB");
            CAPTURE(file.filename().string());
            CHECK(record.embedded(90) == std::vector<std::string>{"1"});
            CHECK(record.embedded(63) == variant.m_number);
            CHECK(record.embedded(421) == variant.m_trueColour);
        };

        const auto untouched = save(bench, drawing, name, to);
        checkBackground(untouched);
        checkRepeatsSource(source, untouched);
        // The file that LibreCAD wrote keeps it through another save.
        EditableDrawing reopened;
        REQUIRE(lc::test::importDrawing(reopened.m_graphic, untouched, to.m_format));
        checkBackground(save(bench, reopened, name + "_again", to));
        // A DWG is saved all the same; what it holds of the background is
        // not this case's.
        const Target dwg = target("dwg2018");
        CHECK(shownAttribute(save(bench, drawing, name, dwg), dwg).m_multiLine);

        // The background is no part of the text: an edit leaves it alone.
        drawing.edit(only<RS_MText>(drawing, RS2::EntityMText), [](RS_MText& text) { text.setText("changed"); });
        const auto edited = save(bench, drawing, name + "_edited", to);
        checkBackground(edited);
        CHECK(onlyRecord(edited, "ATTRIB").embedded(1) == std::vector<std::string>{"changed"});
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

// A DWG names the text style of a text by the handle of the style's record,
// and a text that names none is in the style Standard. Every kind of text
// names the record of its own style: an attribute of one line and one of
// several, a text and a multi-line text of their own.
TEST_CASE("every kind of text keeps its text style in a DWG",
          "[attribute-export][attrib][mtext][filter][dwg][keep]") {
    const Bench bench("text_style_dwg");
    const char* const style = "iso3098";
    const std::string seqend = "0\nSEQEND\n";
    const std::string end = "0\nENDSEC\n0\nEOF\n";
    std::string dxf = AttributeDxf::rich().str();
    REQUIRE(dxf.find(seqend) != std::string::npos);
    dxf.insert(dxf.find(seqend), singleLineAttrib("PATH", "1", "20", "9", "one line", ""));
    REQUIRE(dxf.size() > end.size());
    REQUIRE(dxf.compare(dxf.size() - end.size(), end.size(), end) == 0);
    dxf.insert(dxf.size() - end.size(),
               "0\nTEXT\n100\nAcDbEntity\n8\n0\n100\nAcDbText\n10\n100\n20\n20\n30\n0\n40\n9\n1\nof its own\n"
               "7\niso3098\n100\nAcDbText\n"
               "0\nMTEXT\n100\nAcDbEntity\n8\n0\n100\nAcDbMText\n10\n100\n20\n4\n30\n0\n40\n9\n41\n80\n71\n1\n72\n5\n"
               "1\nseveral\\Plines\n7\niso3098\n73\n1\n44\n1.0\n");
    // How many texts of either kind a drawing shows, after a check of the
    // style of each.
    const auto textsInStyle = [style](EditableDrawing& drawing) {
        int texts = 0;
        for (const RS_Text* text : drawing.live<RS_Text>(RS2::EntityText)) {
            CAPTURE(text->getText().toStdString(), text->getStyle().toStdString());
            CHECK(text->getStyle().compare(style, Qt::CaseInsensitive) == 0);
            ++texts;
        }
        for (const RS_MText* text : drawing.live<RS_MText>(RS2::EntityMText)) {
            CAPTURE(text->getText().toStdString(), text->getStyle().toStdString());
            CHECK(text->getStyle().compare(style, Qt::CaseInsensitive) == 0);
            ++texts;
        }
        return texts;
    };

    for (const Target& to : kTargets) {
        if (to.m_dxf) {
            continue;
        }
        CAPTURE(to.m_name);
        EditableDrawing drawing;
        loadDxf(bench, dxf, drawing, std::string(to.m_name) + "_source");
        REQUIRE(drawing.live<RS_Insert>(RS2::EntityInsert).size() == 1);
        REQUIRE(drawing.live<RS_Text>(RS2::EntityText).size() == 2);
        REQUIRE(drawing.live<RS_MText>(RS2::EntityMText).size() == 2);
        REQUIRE(textsInStyle(drawing) == 4);
        const auto out = save(bench, drawing, "untouched", to);

        // In the file: a record of the style, and its handle in every text.
        DwgContent written;
        readDwg(out, written);
        const std::uint32_t handle = handleNamed(written.m_textStyles, style);
        const std::uint32_t standard = handleNamed(written.m_textStyles, "Standard");
        CAPTURE(handle, standard);
        REQUIRE(handle != 0);
        REQUIRE(handle != standard);
        REQUIRE(written.m_inserts.size() == 1);
        REQUIRE(written.m_inserts.front().attlist.size() == 2);
        for (const auto& attribute : written.m_inserts.front().attlist) {
            REQUIRE(attribute != nullptr);
            CAPTURE(attribute->tag);
            CHECK(attribute->styleH.ref == handle);
            if (attribute->mtext != nullptr) {
                CHECK(attribute->mtext->styleH.ref == handle);
            }
        }
        REQUIRE(written.m_texts.size() == 1);
        CHECK(written.m_texts.front().styleH.ref == handle);
        REQUIRE(written.m_mtexts.size() == 1);
        CHECK(written.m_mtexts.front().styleH.ref == handle);

        // And the file opens with the four texts in their style.
        EditableDrawing reopened;
        REQUIRE(lc::test::importDrawing(reopened.m_graphic, out, to.m_format));
        CHECK(reopened.live<RS_Insert>(RS2::EntityInsert).size() == 1);
        CHECK(textsInStyle(reopened) == 4);
    }
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

TEST_CASE("a rotated attribute read from DXF keeps its angle in the formats of 2018",
          "[attribute-export][attrib][filter][dxf][dwg][attrib-geometry]") {
    const Bench bench("attr_angle_2018");
    // The direction of an MTEXT is its x-axis (11, 21, 31) or its angle (50),
    // and a DWG has the x-axis only. A DXF is saved with the one its file
    // had, so that the record of an untouched attribute stays as it was.
    struct Variant {
        const char* m_name;
        const char* m_ownAngle;                 // group 50 of the attribute itself
        const char* m_direction;                // of the MTEXT
        double m_degrees;
        std::vector<std::string> m_axis;        // 11, 21 and 31 of the MTEXT in a saved DXF
        std::vector<std::string> m_angle;       // and its 50
        const char* m_marker;                   // of the embedded object
    };
    const std::vector<Variant> variants{
        // As AutoCAD writes it: the x-axis, and the angle on the attribute.
        {"x_axis", "50\n30\n", "11\n0.8660254037844387\n21\n0.5\n31\n0\n", 30.0,
         {"0.8660254037844387", "0.5", "0"}, {}, "101"},
        // An x-axis need not have the length 1; it is the file's all the same.
        {"long_x_axis", "50\n45\n", "11\n2\n21\n2\n31\n0\n", 45.0, {"2", "2", "0"}, {}, "101"},
        // Nor a part in the drawing plane: a text that runs up a wall, the
        // plane across X, has an x-axis along Z. In the drawing it reads as
        // the angle 0, and the direction of that angle is not its x-axis.
        {"z_axis", "", "210\n1\n220\n0\n230\n0\n11\n0\n21\n0\n31\n1\n", 0.0, {"0", "0", "1"}, {}, "101"},
        {"angle", "", "50\n75\n", 75.0, {}, {"75"}, "101"},
        // As earlier versions of LibreCAD wrote a text that was rotated in
        // the drawing: the x-axis the file came with, and after it the angle.
        // The last of the two counts, and the axis is not written again.
        {"stale_x_axis", "", "11\n1\n21\n0\n31\n0\n50\n30\n", 30.0, {}, {"30"}, "101"},
        // And as they wrote a rotated attribute that came from a DWG, behind
        // the marker of then: the x-axis of the text itself, and after it
        // the angle all the same.
        {"x_axis_and_angle", "", "11\n0.8660254037844387\n21\n0.5\n31\n0\n50\n30\n", 30.0, {}, {"30"}, "100"},
        {"x_axis_and_angle_150", "", "11\n-0.8660254037844387\n21\n0.5\n31\n0\n50\n150\n", 150.0, {}, {"150"},
         "100"},
    };
    const auto checkDirection = [](const DxfRecord& record, const std::vector<std::string>& axis,
                                   const std::vector<std::string>& angle) {
        std::vector<std::string> written = record.embedded(11);
        for (const int code : {21, 31}) {
            for (const std::string& value : record.embedded(code)) {
                written.push_back(value);
            }
        }
        CAPTURE(written, record.embedded(50));
        CHECK(lc::test::sameValues(axis, written));
        CHECK(lc::test::sameValues(angle, record.embedded(50)));
    };
    for (const Variant& variant : variants) {
        CAPTURE(variant.m_name);
        AttributeDxf fixture;
        fixture.m_marker = variant.m_marker;
        fixture.m_textExtra = variant.m_ownAngle;
        fixture.m_mtextExtra = variant.m_direction;
        EditableDrawing drawing;
        load(bench, fixture, drawing, std::string(variant.m_name) + "_source");
        const double angle = variant.m_degrees * M_PI / 180.0;
        const RS_MText* shown = only<RS_MText>(drawing, RS2::EntityMText);
        CHECK(shown->getAngle() == Catch::Approx(angle).margin(1e-9));

        const auto dxf = save(bench, drawing, variant.m_name, target("dxf2018"));
        checkWrittenDxf(dxf);
        checkDirection(onlyRecord(dxf, "ATTRIB"), variant.m_axis, variant.m_angle);
        EditableDrawing fromDxf;
        REQUIRE(lc::test::importDrawing(fromDxf.m_graphic, dxf, RS2::FormatDXFRW2018));
        CHECK(only<RS_MText>(fromDxf, RS2::EntityMText)->getAngle() == Catch::Approx(angle).margin(1e-9));

        const auto out = save(bench, drawing, variant.m_name, target("dwg2018"));
        EditableDrawing reopened;
        REQUIRE(lc::test::importDrawing(reopened.m_graphic, out, RS2::FormatDWG2018));
        CHECK(only<RS_MText>(reopened, RS2::EntityMText)->getAngle() == Catch::Approx(angle).margin(1e-9));

        // In the file the direction is the x-axis. And the MTEXT is marked
        // as not annotative: a DXF has no annotation scales for it.
        DwgContent content;
        readDwg(out, content);
        const DRW_MText& mtext = onlyEmbeddedMText(content);
        CAPTURE(mtext.secPoint.x, mtext.secPoint.y, mtext.secPoint.z);
        if (std::string(variant.m_name) == "z_axis") {
            CHECK(sameNumbers({0.0, 0.0, 1.0}, {mtext.secPoint.x, mtext.secPoint.y, mtext.secPoint.z}));
        }
        else {
            CHECK(std::hypot(mtext.secPoint.x, mtext.secPoint.y) >= 1.0 - 1e-9);
        }
        CHECK(std::atan2(mtext.secPoint.y, mtext.secPoint.x) == Catch::Approx(angle).margin(1e-9));
        CHECK(mtext.m_r2018IsNotAnnotative);

        // Rotated in the drawing, the text has one direction, the new one,
        // whichever way its file had the old one.
        const RS_MText* turned = drawing.edit(shown, [](RS_MText& text) { text.setAngle(1.0); });
        const auto turnedDxf = save(bench, drawing, std::string(variant.m_name) + "_turned", target("dxf2018"));
        const DxfRecord turnedRecord = onlyRecord(turnedDxf, "ATTRIB");
        checkDirection(turnedRecord,
                       {QString::number(std::cos(1.0), 'g', 17).toStdString(),
                        QString::number(std::sin(1.0), 'g', 17).toStdString(), "0"},
                       {});
        // The attribute itself turns with its MTEXT, about the alignment
        // point (1,4): its baseline starts one height below along the
        // turned text.
        CHECK(sameNumbers({1.0 + 9.0 * std::sin(1.0), 4.0 - 9.0 * std::cos(1.0), 1.0, 4.0}, ownPlace(turnedRecord)));
        CHECK(sameNumbers({1.0 * 180.0 / M_PI}, ownNumbers(turnedRecord, {50})));
        for (const char* format : {"dxf2018", "dwg2018"}) {
            const Target to = target(format);
            CAPTURE(to.m_name);
            const auto file = to.m_dxf ? turnedDxf
                                       : save(bench, drawing, std::string(variant.m_name) + "_turned", to);
            EditableDrawing again;
            REQUIRE(lc::test::importDrawing(again.m_graphic, file, to.m_format));
            checkSameMText(*turned, *only<RS_MText>(again, RS2::EntityMText));
        }
    }
}

// The same of an MTEXT of its own. Its record has the angle in any case, and
// the x-axis its file had next to it: a reader takes the x-axis where a
// record has both, so the two have to be one direction, and a text that was
// turned in the drawing cannot keep the x-axis of its file. In a DWG the
// x-axis is all there is, also for a text that never had one. A text that
// runs up a wall has an x-axis along Z: no direction in the drawing, where
// it reads as the angle 0, and the x-axis of its file as much as any other.
TEST_CASE("a standalone MTEXT keeps a rotation made in the drawing",
          "[attribute-export][mtext][filter][dxf][dwg][attrib-geometry]") {
    const Bench bench("mtext_turned");
    // A text at 30 degrees: as AutoCAD writes it, with an x-axis that is
    // longer than 1, and with the angle alone. And one on a wall, the plane
    // across X, that runs up or down it.
    const std::string xAxis = "11\n0.8660254037844335\n21\n0.5000000000000089\n31\n0.0\n";
    const std::string longAxis = "11\n1.73205080756887\n21\n1.0\n31\n0.0\n";
    const std::string angleAlone = "50\n30\n";
    const std::string upAWall = "210\n1\n220\n0\n230\n0\n11\n0\n21\n0\n31\n1\n";
    const std::string downAWall = "210\n1\n220\n0\n230\n0\n11\n0\n21\n0\n31\n-1\n";
    using Turn = std::function<bool(EditableDrawing&, const RS_MText*)>;
    const Turn rotate = [](EditableDrawing& drawing, const RS_MText* text) {
        RS_RotateData data;
        data.center = text->getInsertionPoint();
        data.angle = M_PI / 6.0;
        bool done = false;
        drawing.modify([&](LC_DocumentModificationBatch& ctx) {
            done = RS_Modification::rotate(data, selection({text}), false, ctx);
        });
        return done;
    };
    const Turn mirror = [](EditableDrawing& drawing, const RS_MText* text) {
        RS_MirrorData data;
        data.axisPoint1 = RS_Vector(0.0, 0.0);
        data.axisPoint2 = RS_Vector(0.0, 10.0);
        bool done = false;
        drawing.modify([&](LC_DocumentModificationBatch& ctx) {
            done = RS_Modification::mirror(data, selection({text}), false, ctx);
        });
        return done;
    };
    const Turn setAngle = [](EditableDrawing& drawing, const RS_MText* text) {
        drawing.edit(text, [](RS_MText& clone) { clone.setAngle(1.0); });
        return true;
    };
    struct Row {
        const char* m_name;
        std::string m_direction;    // of the MTEXT in the source
        double m_degrees;           // the angle the drawing reads it at
        Turn m_turn;                // none: the text is left alone
        bool m_hasAxis;             // a DXF saved from the drawing has an x-axis
        std::vector<double> m_sameAxis;     // and it is this one, the source's (11, 21, 31)
    };
    const std::vector<Row> rows{
        {"untouched", xAxis, 30.0, nullptr, true, {0.8660254037844335, 0.5000000000000089, 0.0}},
        // The x-axis of a file is its own, whatever its length, until the
        // text is turned.
        {"long_axis", longAxis, 30.0, nullptr, true, {1.73205080756887, 1.0, 0.0}},
        {"rotated", xAxis, 30.0, rotate, true, {}},
        {"mirrored", xAxis, 30.0, mirror, true, {}},
        {"angle_set", xAxis, 30.0, setAngle, true, {}},
        {"angle_alone", angleAlone, 30.0, nullptr, false, {}},
        {"angle_alone_rotated", angleAlone, 30.0, rotate, false, {}},
        // Also the one along Z, in a DWG too: the direction of the angle 0
        // in its place would lie along the normal of this text.
        {"up_a_wall", upAWall, 0.0, nullptr, true, {0.0, 0.0, 1.0}},
        {"down_a_wall", downAWall, 0.0, nullptr, true, {0.0, 0.0, -1.0}},
        {"up_a_wall_rotated", upAWall, 0.0, rotate, true, {}},
    };
    for (const Row& row : rows) {
        for (const char* format : {"dxf2018", "dxf2007", "dxf2000", "dwg2018", "dwg2000"}) {
            const Target to = target(format);
            CAPTURE(row.m_name, to.m_name);
            EditableDrawing drawing;
            loadDxf(bench, standaloneMTextDxf("1", row.m_direction), drawing,
                    std::string(row.m_name) + "_" + to.m_name + "_source");
            const RS_MText* read = only<RS_MText>(drawing, RS2::EntityMText);
            const double sourceAngle = row.m_degrees * M_PI / 180.0;
            REQUIRE(sameDirection(read->getAngle(), sourceAngle));
            if (row.m_turn) {
                REQUIRE(row.m_turn(drawing, read));
            }
            const RS_MText* shown = only<RS_MText>(drawing, RS2::EntityMText);
            const double angle = shown->getAngle();
            REQUIRE(sameDirection(angle, sourceAngle) == !row.m_turn);
            // An x-axis that is kept and has no part in the drawing plane
            // gives no direction to compare.
            const bool alongZ = !row.m_sameAxis.empty() && row.m_sameAxis[0] == 0.0 && row.m_sameAxis[1] == 0.0;

            const auto out = save(bench, drawing, row.m_name, to);
            if (to.m_dxf) {
                checkWrittenDxf(out);
                const DxfRecord record = onlyRecord(out, "MTEXT");
                const std::vector<double> written = ownNumbers(record, {50});
                REQUIRE(written.size() == 1);
                CHECK(sameDirection(written.front() * M_PI / 180.0, angle));
                const std::vector<double> axis = ownNumbers(record, {11, 21, 31});
                CAPTURE(written, axis);
                if (row.m_hasAxis) {
                    REQUIRE(axis.size() == 3);
                    CHECK(sameDirection(std::atan2(axis[1], axis[0]), angle));
                }
                else {
                    CHECK(axis.empty());
                }
                if (!row.m_sameAxis.empty()) {
                    // The same numbers to the last bit, however they are
                    // spelled.
                    CHECK(axis == row.m_sameAxis);
                }
            }
            else {
                DwgContent written;
                readDwg(out, written);
                REQUIRE(written.m_mtexts.size() == 1);
                const DRW_Coord& axis = written.m_mtexts.front().secPoint;
                CAPTURE(axis.x, axis.y, axis.z);
                if (!alongZ) {
                    CHECK(std::hypot(axis.x, axis.y) >= 1.0 - 1e-9);
                }
                CHECK(sameDirection(std::atan2(axis.y, axis.x), angle));
                if (!row.m_sameAxis.empty()) {
                    CHECK(std::vector<double>{axis.x, axis.y, axis.z} == row.m_sameAxis);
                }
            }

            EditableDrawing reopened;
            REQUIRE(lc::test::importDrawing(reopened.m_graphic, out, to.m_format));
            const RS_MText* again = only<RS_MText>(reopened, RS2::EntityMText);
            CAPTURE(again->getAngle(), angle);
            CHECK(sameDirection(again->getAngle(), angle));
            CHECK(again->getInsertionPoint().distanceTo(shown->getInsertionPoint()) < 1e-9);
        }
    }
}

// A hidden attribute. The record of an attribute can be invisible like that
// of any entity (group 60 of a DXF, a bit of its own in a DWG), and an
// attribute has a flag "invisible" (bit 1 of group 70): that is how AutoCAD
// hides an attribute, and what its ATTDISP shows again. The drawing has a
// text that shows or does not. A save gives the file back which of the two
// it had: a record that is invisible itself stays hidden whatever the flag
// says.
TEST_CASE("a hidden attribute is saved hidden the way its file had it",
          "[attribute-export][attrib][filter][dxf][dwg][keep]") {
    const Bench bench("attr_hidden");
    const std::vector<std::string> none;
    const std::vector<std::string> invisible{"1"};
    // The values of the texts that a drawing does not show, sorted.
    const auto hiddenTexts = [](EditableDrawing& drawing) {
        std::vector<std::string> values;
        for (const RS_Text* text : drawing.live<RS_Text>(RS2::EntityText)) {
            if (!text->getFlag(RS2::FlagVisible)) {
                values.push_back(text->getText().toStdString());
            }
        }
        std::sort(values.begin(), values.end());
        return values;
    };

    SECTION("attributes of one line") {
        struct Row {
            const char* m_tag;
            const char* m_value;
            bool m_invisible;       // the record has 60 = 1
            const char* m_flags;    // group 70
        };
        const std::vector<Row> rows{
            {"SHOWN", "shown", false, "0"},
            {"FLAGGED", "flagged", false, "1"},
            {"UNSEEN", "unseen", true, "0"},
            {"BOTH", "both", true, "1"},
        };
        std::vector<std::string> attributes;
        for (std::size_t i = 0; i < rows.size(); ++i) {
            const Row& row = rows[i];
            attributes.push_back(std::string("0\nATTRIB\n100\nAcDbEntity\n8\n0\n") + (row.m_invisible ? "60\n1\n" : "")
                                 + "100\nAcDbText\n10\n1\n20\n" + std::to_string(2 * i) + "\n30\n0\n40\n1\n1\n"
                                 + row.m_value + "\n100\nAcDbAttribute\n280\n0\n2\n" + row.m_tag + "\n70\n"
                                 + row.m_flags + "\n280\n1\n");
        }
        EditableDrawing drawing;
        loadDxf(bench, singleLineAttributesDxf(attributes), drawing);
        const std::vector<std::string> hidden{"both", "flagged", "unseen"};
        REQUIRE(hiddenTexts(drawing) == hidden);

        // What a saved file has of the attribute with a tag: whether its
        // record is invisible, and its flags.
        const auto checkSaved = [&](const std::filesystem::path& file, const Target& to, const std::string& tag,
                                    const bool recordInvisible, const std::string& flags) {
            CAPTURE(file.filename().string(), tag);
            if (to.m_dxf) {
                const DxfRecord record = attributeRecord(file, tag);
                CHECK(record.own(60) == (recordInvisible ? invisible : none));
                CHECK(record.own(70) == std::vector<std::string>{flags});
                return;
            }
            DwgContent content;
            readDwg(file, content);
            REQUIRE(content.m_inserts.size() == 1);
            int found = 0;
            for (const auto& attribute : content.m_inserts.front().attlist) {
                REQUIRE(attribute != nullptr);
                if (attribute->tag == tag) {
                    ++found;
                    CHECK(attribute->visible == !recordInvisible);
                    CHECK(std::to_string(static_cast<int>(attribute->attribFlags)) == flags);
                }
            }
            CHECK(found == 1);
        };

        for (const Target& to : kTargets) {
            CAPTURE(to.m_name);
            const auto out = save(bench, drawing, "untouched", to);
            if (to.m_dxf) {
                checkWrittenDxf(out);
            }
            else {
                // A text of one line keeps what it has of its attribute
                // under the private names, as a multi-line text does.
                DwgContent content;
                readDwg(out, content);
                checkNoPrivateApplication(content);
            }
            for (const Row& row : rows) {
                checkSaved(out, to, row.m_tag, row.m_invisible, row.m_flags);
            }
            EditableDrawing reopened;
            REQUIRE(lc::test::importDrawing(reopened.m_graphic, out, to.m_format));
            CHECK(hiddenTexts(reopened) == hidden);
        }

        // A text that the drawing itself has hidden since is an invisible
        // record: its attribute has no flag for it.
        drawing.edit(textWithValue(drawing, "shown"), [](RS_Text& text) { text.setVisible(false); });
        for (const char* format : {"dxf2018", "dxf2007", "dwg2018", "dwg2000"}) {
            const Target to = target(format);
            CAPTURE(to.m_name);
            const auto out = save(bench, drawing, "hidden_since", to);
            checkSaved(out, to, "SHOWN", true, "0");
            checkSaved(out, to, "FLAGGED", false, "1");
        }
    }

    // In a DWG 2018 the MTEXT of an attribute has the bit of an entity too,
    // and the drawing does not show a text whose MTEXT is invisible. No other
    // format has a place for that bit: there the record of the attribute is
    // invisible in its stead, so that the text stays hidden.
    SECTION("a multi-line attribute") {
        struct Variant {
            const char* m_name;
            const char* m_commonGroups;
            const char* m_flags;
            const char* m_mtextTail;
        };
        const std::vector<Variant> variants{
            {"flagged", "", "1", ""},
            {"unseen", "60\n1\n", "0", ""},
            {"mtext_unseen", "", "0", "60\n1\n"},
        };
        for (const Variant& variant : variants) {
            const std::string name = variant.m_name;
            const bool recordInvisible = name == "unseen";
            const bool mtextInvisible = name == "mtext_unseen";
            AttributeDxf fixture;
            fixture.m_commonGroups = variant.m_commonGroups;
            fixture.m_flags = variant.m_flags;
            fixture.m_mtextTail = variant.m_mtextTail;
            EditableDrawing drawing;
            load(bench, fixture, drawing, name + "_source");
            REQUIRE_FALSE(only<RS_MText>(drawing, RS2::EntityMText)->getFlag(RS2::FlagVisible));

            for (const Target& to : kTargets) {
                CAPTURE(name, to.m_name);
                const auto out = save(bench, drawing, name, to);
                const bool hasMTextBit = to.m_embedsMText && !to.m_dxf;
                const bool expectInvisible = recordInvisible || (mtextInvisible && !hasMTextBit);
                if (to.m_dxf) {
                    checkWrittenDxf(out);
                    const DxfRecord record = onlyRecord(out, "ATTRIB");
                    CHECK(record.own(60) == (expectInvisible ? invisible : none));
                    CHECK(record.own(70) == std::vector<std::string>{variant.m_flags});
                }
                else {
                    DwgContent content;
                    readDwg(out, content);
                    const DRW_Attrib& attribute = onlyDwgAttribute(content);
                    CHECK(attribute.visible == !expectInvisible);
                    CHECK(std::to_string(static_cast<int>(attribute.attribFlags)) == variant.m_flags);
                    if (hasMTextBit) {
                        CHECK(onlyEmbeddedMText(content).visible == !mtextInvisible);
                    }
                }
                EditableDrawing reopened;
                REQUIRE(lc::test::importDrawing(reopened.m_graphic, out, to.m_format));
                CHECK_FALSE(attributeText(reopened)->getFlag(RS2::FlagVisible));
            }
        }
    }
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

// A tool does not replace an entity on a locked layer: the original stays
// and each clone of it is one more entity. Of a block reference and a text
// of it, one may be replaced and the other not. Clone number n of the text
// belongs to clone number n of the block reference all the same, whichever of
// the two took the place of its original.
TEST_CASE("copies of a block reference and its text stay pairs when one of the two is on a locked layer",
          "[attribute-export][attrib][filter][dxf][dwg][attrib-owner]") {
    const Bench bench("attr_owner_locked");
    const std::vector<const char*> formats{"dxf2018", "dwg2018", "dwg2000"};
    struct Row {
        const char* m_name;
        bool m_textLocked;      // else the block reference
        const char* m_expected;
    };
    const std::vector<Row> rows{
        // The block reference moved and has what it had, the text that
        // stayed too. The first clone of the text is a text of its own.
        {"text_locked", true, "INSERT@50[first|second||secret] INSERT@100[far] TEXT[near] hidden=0 LINE=1"},
        // The block reference stayed, and the text that moved is still its,
        // in its place among the attributes. The first clone of the block
        // reference has no text.
        {"reference_locked", false,
         "INSERT@0[near|second||secret] INSERT@50[] INSERT@100[far] TEXT[] hidden=0 LINE=1"},
    };
    for (const Row& row : rows) {
        CAPTURE(row.m_name);
        EditableDrawing drawing;
        loadDxf(bench, attributedInsertDxf(false), drawing, row.m_name);
        const RS_Insert* insert = referenceAt(drawing, 0);
        const RS_Text* nameText = lineText(drawing, "first");
        drawing.m_graphic.addLayer(new RS_Layer("LOCKED"));
        RS_Layer* locked = drawing.m_graphic.findLayer("LOCKED");
        REQUIRE(locked != nullptr);
        const RS_Entity* kept = row.m_textLocked ? static_cast<const RS_Entity*>(nameText) : insert;
        const_cast<RS_Entity*>(kept)->setLayer(locked);
        locked->lock(true);

        // The Move tool with two copies and without the originals.
        RS_MoveData data;
        data.offset = RS_Vector(50.0, 0.0);
        data.multipleCopies = true;
        data.number = 2;
        drawing.modify([&](LC_DocumentModificationBatch& ctx) {
            RS_Modification::move(data, selection({insert, nameText}), false, ctx);
        });
        locked->lock(false);
        CHECK(kept->isDeleted() == false);
        CHECK(drawing.live<RS_Insert>(RS2::EntityInsert).size() == (row.m_textLocked ? 2u : 3u));

        const RS_Insert* second = referenceAt(drawing, 100);
        CHECK(lineText(drawing, "first", 101.0)->ownerLineage() == second->lineage());
        // The clones of the text have one value; a value of its own for each
        // shows in every format which block reference has which.
        drawing.edit(lineText(drawing, "first", 51.0), [](RS_Text& text) { text.setText("near"); });
        drawing.edit(lineText(drawing, "first", 101.0), [](RS_Text& text) { text.setText("far"); });
        for (const auto& file : saveAll(bench, drawing, row.m_name, formats, row.m_expected)) {
            CHECK(lc::test::dxfRecords(file, "INSERT").size() == (row.m_textLocked ? 2u : 3u));
            CHECK(lc::test::dxfRecords(file, "TEXT").size() == (row.m_textLocked ? 1u : 0u));
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

    // A text has neither an elevation nor a thickness, and it lies in the
    // plane of the drawing. Its attribute had all of that, and the record of
    // the text has a place for each: 30 and 31 for the two points, 39, and
    // 210 to 230 for the direction the plane faces.
    SECTION("a text of one line stays in the plane of its attribute") {
        EditableDrawing drawing;
        loadDxf(bench,
                singleLineAttributesDxf({"0\nATTRIB\n100\nAcDbEntity\n8\n0\n100\nAcDbText\n39\n2\n"
                                         "10\n1\n20\n2\n30\n5\n40\n1\n1\nlone\n72\n1\n11\n3\n21\n2\n31\n5\n"
                                         "210\n0\n220\n0\n230\n-1\n"
                                         "100\nAcDbAttribute\n280\n0\n2\nNOTE\n70\n0\n280\n1\n"}),
                drawing, "plane");
        drawing.erase(only<RS_Insert>(drawing, RS2::EntityInsert));
        for (const char* format : {"dxf2018", "dxf2007"}) {
            const Target to = target(format);
            CAPTURE(to.m_name);
            const auto file = save(bench, drawing, "plane_erased", to);
            checkWrittenDxf(file);
            CHECK(lc::test::dxfRecords(file, "ATTRIB").empty());
            const DxfRecord record = onlyRecord(file, "TEXT");
            CHECK(record.own(1) == std::vector<std::string>{"lone"});
            CHECK(lc::test::sameValues({"5"}, record.own(30)));
            CHECK(lc::test::sameValues({"5"}, record.own(31)));
            CHECK(lc::test::sameValues({"2"}, record.own(39)));
            CHECK(lc::test::sameValues({"0", "0", "-1"}, {record.own(210).front(), record.own(220).front(),
                                                           record.own(230).front()}));
        }
    }

    // A multi-line text is written as an MTEXT, which has an elevation, a
    // direction its plane faces, and the direction of its text as an x-axis.
    // The MTEXT of the attribute had them, and they are not those of the
    // attribute's own record: here that one lies flat at elevation 0.
    SECTION("a multi-line text stays in the plane of its MTEXT") {
        AttributeDxf fixture;
        fixture.m_handles = true;
        fixture.m_mtextPoint = "10\n1\n20\n4\n30\n7\n";
        fixture.m_mtextExtra = "210\n0\n220\n0\n230\n-1\n";
        EditableDrawing drawing;
        load(bench, fixture, drawing, "mtext_plane");
        drawing.erase(only<RS_Insert>(drawing, RS2::EntityInsert));
        const auto checkPlane = [](const std::filesystem::path& file) {
            checkWrittenDxf(file);
            CHECK(lc::test::dxfRecords(file, "ATTRIB").empty());
            const DxfRecord record = onlyRecord(file, "MTEXT");
            CAPTURE(file.filename().string());
            CHECK(lc::test::sameValues({"1", "4", "7"}, {record.own(10).front(), record.own(20).front(),
                                                        record.own(30).front()}));
            CHECK(lc::test::sameValues({"0", "0", "-1"}, {record.own(210).front(), record.own(220).front(),
                                                           record.own(230).front()}));
        };
        checkPlane(save(bench, drawing, "mtext_plane_erased", target("dxf2018")));
        checkPlane(save(bench, drawing, "mtext_plane_erased", target("dxf2007")));
        // A DWG has the same fields: the drawing opened from it is saved
        // with what they held.
        const auto dwg = save(bench, drawing, "mtext_plane_erased", target("dwg2018"));
        EditableDrawing fromDwg;
        REQUIRE(lc::test::importDrawing(fromDwg.m_graphic, dwg, RS2::FormatDWG2018));
        CHECK(only<RS_MText>(fromDwg, RS2::EntityMText)->ownerLineage() == 0);
        checkPlane(save(bench, fromDwg, "mtext_plane_dwg2018", target("dxf2018")));
    }

    // A DWG has the direction of an MTEXT as its x-axis and nothing else. The
    // MTEXT of an attribute can have its direction as an angle alone (group
    // 50): a DXF is saved with the angle, as its file was, and a DWG with the
    // x-axis that says the same.
    SECTION("a turned multi-line text keeps its direction") {
        struct Variant {
            const char* m_name;
            const char* m_direction;            // of the MTEXT of the attribute
            std::vector<std::string> m_axis;    // 11 and 21 of the MTEXT in a saved DXF
        };
        const std::vector<Variant> variants{
            {"x_axis", "11\n0\n21\n1\n31\n0\n", {"0", "1"}},
            {"angle", "50\n90\n", {}},
        };
        for (const Variant& variant : variants) {
            CAPTURE(variant.m_name);
            const std::string name = std::string("mtext_turned_") + variant.m_name;
            AttributeDxf fixture;
            fixture.m_handles = true;
            fixture.m_mtextExtra = variant.m_direction;
            EditableDrawing drawing;
            load(bench, fixture, drawing, name);
            REQUIRE(only<RS_MText>(drawing, RS2::EntityMText)->getAngle() == Catch::Approx(M_PI / 2.0).margin(1e-9));
            drawing.erase(only<RS_Insert>(drawing, RS2::EntityInsert));
            for (const char* format : {"dwg2018", "dwg2000", "dxf2018", "dxf2007"}) {
                const Target to = target(format);
                CAPTURE(to.m_name);
                const auto file = save(bench, drawing, name + "_erased", to);
                if (to.m_dxf) {
                    const DxfRecord record = onlyRecord(file, "MTEXT");
                    std::vector<std::string> axis = record.own(11);
                    for (const std::string& y : record.own(21)) {
                        axis.push_back(y);
                    }
                    CAPTURE(axis, record.own(50));
                    CHECK(lc::test::sameValues(variant.m_axis, axis));
                    CHECK(lc::test::sameValues({"90"}, record.own(50)));
                }
                EditableDrawing reopened;
                REQUIRE(lc::test::importDrawing(reopened.m_graphic, file, to.m_format));
                const RS_MText* text = only<RS_MText>(reopened, RS2::EntityMText);
                CHECK(text->getAngle() == Catch::Approx(M_PI / 2.0).margin(1e-9));
                CHECK(text->getInsertionPoint().distanceTo(RS_Vector(1.0, 4.0)) < 1e-9);
            }
            // Turned in the drawing since, it has the new direction and not
            // the one of its file.
            drawing.edit(only<RS_MText>(drawing, RS2::EntityMText), [](RS_MText& text) { text.setAngle(1.0); });
            for (const char* format : {"dwg2018", "dwg2000", "dxf2018"}) {
                const Target to = target(format);
                CAPTURE(to.m_name);
                const auto file = save(bench, drawing, name + "_again", to);
                EditableDrawing reopened;
                REQUIRE(lc::test::importDrawing(reopened.m_graphic, file, to.m_format));
                CHECK(only<RS_MText>(reopened, RS2::EntityMText)->getAngle() == Catch::Approx(1.0).margin(1e-9));
            }
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

// An attribute can have what every entity can: an extension dictionary and
// reactors. AutoCAD gives a multi-line attribute a dictionary in every file
// older than 2018, which has no MTEXT for it. A DWG names every record by a
// handle, so such an attribute is given its handle before its block
// reference is written, and in a DWG 2000 that has to be the handle its
// place after the block reference comes with.
TEST_CASE("a block reference whose attribute has a dictionary or a reactor saves to DWG",
          "[attribute-export][attrib][filter][dxf][dwg][attrib-owner]") {
    const Bench bench("attr_owner_references");
    const std::vector<const char*> formats{"dwg2000", "dwg2004", "dwg2007", "dwg2010",
                                           "dwg2013", "dwg2018", "dxf2018", "dxf2007"};
    // The dictionary as AutoCAD writes it: owned by the attribute, with a
    // record of what a newer file would have.
    const std::string dictionary = "0\nSECTION\n2\nOBJECTS\n0\nDICTIONARY\n5\nC\n330\n0\n100\nAcDbDictionary\n281\n1\n"
                                   "0\nDICTIONARY\n5\n1C0\n330\n1A1\n100\nAcDbDictionary\n281\n1\n"
                                   "3\nACAD_XREC_ROUNDTRIP\n360\n1C1\n"
                                   "0\nXRECORD\n5\n1C1\n330\n1C0\n100\nAcDbXrecord\n280\n1\n0\nENDSEC\n";
    // An object that anything can refer to: a dictionary of the drawing's own.
    const std::string listed = "0\nSECTION\n2\nOBJECTS\n0\nDICTIONARY\n5\nC\n330\n0\n100\nAcDbDictionary\n281\n1\n"
                               "3\nLC_ATTRIBUTE_TEST\n350\n1C0\n"
                               "0\nDICTIONARY\n5\n1C0\n330\nC\n100\nAcDbDictionary\n281\n1\n0\nENDSEC\n";
    struct Variant {
        const char* m_name;
        const char* m_groups;       // of the first attribute, after its handle
        std::string m_objects;
        bool m_handles;
    };
    const std::vector<Variant> variants{
        {"dictionary", "102\n{ACAD_XDICTIONARY\n360\n1C0\n102\n}\n", dictionary, true},
        {"reactor", "102\n{ACAD_REACTORS\n330\n1C0\n102\n}\n", listed, true},
        // The entities of a file need not have handles: then no handle of
        // the saved file is taken ahead for them, and those of the
        // attributes are taken when their block reference is written.
        {"reactor_no_handles", "102\n{ACAD_REACTORS\n330\n1C0\n102\n}\n", listed, false},
        // A material is a reference like these, and one that a copy keeps.
        {"material", "347\n1C0\n", listed, true},
    };
    for (const Variant& variant : variants) {
        const std::string name = variant.m_name;
        CAPTURE(name);
        EditableDrawing drawing;
        loadDxf(bench, attributedInsertDxf(variant.m_handles, variant.m_groups, variant.m_objects), drawing,
                name + "_source");
        // The text has what this case is about.
        const RS_Text* first = lineText(drawing, "first");
        REQUIRE((first->xDictHandle() == 0x1C0u || first->reactorHandles() == std::vector<quint32>{0x1C0u}
                 || first->materialHandle() == 0x1C0u));
        REQUIRE((first->sourceHandle() != 0) == variant.m_handles);

        for (const auto& file :
             saveAll(bench, drawing, name, formats, "INSERT@0[first|second||secret] TEXT[] hidden=0 LINE=1")) {
            CHECK(attributeTags(file) == std::vector<std::string>{"NAME", "NOTE", "BLANK", "CODE"});
        }

        // And so does the clone that an edit puts in the place of the text,
        // and a copy of the block reference with its texts next to it.
        drawing.edit(first, [](RS_Text& text) { text.setText("edited"); });
        saveAll(bench, drawing, name + "_edited", formats, "INSERT@0[edited|second||secret] TEXT[] hidden=0 LINE=1");
        moveEntities(drawing, {referenceAt(drawing, 0), lineText(drawing, "edited"), lineText(drawing, "second")},
                     RS_Vector(50.0, 0.0), 1);
        saveAll(bench, drawing, name + "_copied", formats,
                "INSERT@0[edited|second||secret] INSERT@50[edited|second] TEXT[] hidden=0 LINE=1");
    }

    // The same inside a block definition, whose entities are a chain of
    // their own.
    EditableDrawing nested;
    loadDxf(bench, nestedAttributedInsertDxf("102\n{ACAD_REACTORS\n330\n1C0\n102\n}\n", listed), nested,
            "nested_source");
    saveAll(bench, nested, "nested", formats, "INSERT@0[first|second||secret] TEXT[] hidden=0 LINE=0", "OUTER");
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

TEST_CASE("of two block references with one lineage the first has the attribute texts",
          "[attribute-export][attrib][filter][dxf][dwg][attrib-owner]") {
    const Bench bench("attr_owner_shared_lineage");
    EditableDrawing drawing;
    loadWithHandles(bench, drawing, "fixture");
    const RS_Insert* insert = referenceAt(drawing, 0);

    // A clone put into the drawing without an undo step: two block
    // references that show, and the texts name both by their one lineage.
    RS_Entity* twin = insert->clone();
    twin->clearDwgProvenance(RS_Entity::Identity);
    twin->move(RS_Vector(50.0, 0.0));
    drawing.m_graphic.addEntity(twin);
    REQUIRE(twin->lineage() == insert->lineage());

    for (const auto& file : saveAll(bench, drawing, "twin", {"dxf2018", "dxf2007", "dwg2018", "dwg2000"},
                                    "INSERT@0[first|second||secret] INSERT@50[] TEXT[] hidden=0 LINE=1")) {
        CHECK(attributeTags(file) == std::vector<std::string>{"NAME", "NOTE", "BLANK", "CODE"});
        CHECK(lc::test::dxfRecords(file, "TEXT").empty());
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

    // The data can name ACAD, the application every drawing has. A save
    // leaves that record to libdxfrw, which gives it the handle it has in
    // every DWG: the data names it by that handle and is kept.
    AttributeDwgSource source;
    source.m_dataNamesAcad = true;
    const std::filesystem::path file = bench.m_out.file("acad_source.dwg");
    source.write(file);
    {
        DwgContent content;
        readDwg(file, content);
        REQUIRE(handleNamed(content.m_applications, "ACAD") == kAcadApplication);
        REQUIRE(onlyEmbeddedMText(content).m_r2018AnnotativeAppHandle == kAcadApplication);
    }
    EditableDrawing drawing;
    REQUIRE(lc::test::importDrawing(drawing.m_graphic, file, RS2::FormatDWG2018));
    const Saved saved = saveWithReport(bench, drawing, "acad", target("dwg2018"));
    DwgContent written;
    readDwg(saved.m_file, written);
    const DRW_MText& mtext = onlyEmbeddedMText(written);
    CHECK(mtext.m_r2018AnnotativeData == kScaleData);
    CHECK(mtext.m_r2018AnnotativeUnknown == kScaleDataShort);
    CHECK_FALSE(mtext.m_r2018IsNotAnnotative);
    CHECK(handleNamed(written.m_applications, "ACAD") == kAcadApplication);
    CHECK(mtext.m_r2018AnnotativeAppHandle == kAcadApplication);
    CAPTURE(saved.m_report);
    CHECK(saved.m_report.find("annotation scale") == std::string::npos);
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

// A column layout of the MTEXT is kept and left out by the rule of the
// annotation-scale data: only a DWG 2018 has a place for it, and the drawing
// shows one layout of the text, without columns. No file that these cases
// can write has columns in the MTEXT of an attribute, so the layout is put
// here straight into what the text keeps of its attribute.
TEST_CASE("a column layout in the MTEXT of an attribute is saved with the untouched text or reported",
          "[attribute-export][attrib][filter][dxf][dwg][annotative]") {
    const Bench bench("attr_columns");
    AttributeDxf fixture;
    fixture.m_handles = true;
    for (const bool edited : {false, true}) {
        for (const char* format : kScaleDataFormats) {
            const Target to = target(format);
            const std::string name = edited ? "edited" : "untouched";
            CAPTURE(name, to.m_name);
            EditableDrawing drawing;
            load(bench, fixture, drawing, name + "_" + to.m_name + "_source");
            const RS_MText* shown = only<RS_MText>(drawing, RS2::EntityMText);
            REQUIRE(setStoredMTextColumnType(*const_cast<RS_MText*>(shown), 1));
            if (edited) {
                drawing.edit(shown, [](RS_MText& text) { text.setText("changed\ntext"); });
            }
            const Saved saved = saveWithReport(bench, drawing, name, to);
            CAPTURE(saved.m_report);
            const bool kept = !edited && to.m_embedsMText && !to.m_dxf;
            CHECK((saved.m_report.find(kScaleDataLeftOut) == std::string::npos) == kept);
        }
    }

    // Without columns there is nothing to report.
    EditableDrawing drawing;
    load(bench, fixture, drawing, "no_columns_source");
    CHECK(saveWithReport(bench, drawing, "no_columns", target("dxf2007")).m_report.find("column")
          == std::string::npos);
}

// A DWG names a record by its handle, a number that means something in the
// file it is in only: a save gives every record a handle of its own. What
// the MTEXT inside an attribute refers to in a saved file has to be a record
// of that file: the layer and the text style of its attribute, and the
// linetype "by layer", which needs no record. A reference that the drawing
// knows nothing about is not saved at all: the number it had in the file it
// was read from is another record in the saved file, or none.
TEST_CASE("the MTEXT of an attribute saved to DWG 2018 refers to records of the saved file",
          "[attribute-export][attrib][filter][dwg][embedded-mtext]") {
    const Bench bench("attr_dwg_references");
    AttributeDwgSource source;
    source.m_references = true;
    const std::filesystem::path file = bench.m_out.file("source.dwg");
    source.write(file);
    {
        // The source has what this case is about.
        DwgContent content;
        readDwg(file, content);
        const DRW_MText& mtext = onlyEmbeddedMText(content);
        REQUIRE(onlyDwgAttribute(content).dwgLayerHandle() == source.m_layer);
        REQUIRE(mtext.styleH.ref == source.m_style);
        REQUIRE(handleNamed(content.m_layers, kEmbeddedSourceLayer) == source.m_embeddedLayer);
        REQUIRE(handleNamed(content.m_lineTypes, kEmbeddedSourceLinetype) == source.m_embeddedLinetype);
        REQUIRE(mtext.dwgLayerHandle() == source.m_embeddedLayer);
        REQUIRE(mtext.dwgLayerHandle() != onlyDwgAttribute(content).dwgLayerHandle());
        REQUIRE(mtext.dwgLinetypeHandle() == source.m_embeddedLinetype);
        REQUIRE(mtext.reactorHandles == kForeignReactors);
        REQUIRE(mtext.xDictHandle == kForeignDictionary);
        REQUIRE(mtext.dwgAcDbColorHandle() == kForeignColor);
        REQUIRE(mtext.m_r2018AppIdHandle == kForeignApplication);
    }

    struct Variant {
        const char* m_name;
        std::function<void(RS_MText&)> m_edit;  // none: untouched
    };
    const std::vector<Variant> variants{
        {"untouched", nullptr},
        {"edited",
         [](RS_MText& text) {
             text.move(RS_Vector(2.0, 1.0));
             text.setAngle(0.5);
             text.setVAlign(RS_MTextData::VABottom);
             text.setHAlign(RS_MTextData::HARight);
         }},
    };
    for (const Variant& variant : variants) {
        CAPTURE(variant.m_name);
        EditableDrawing drawing;
        REQUIRE(lc::test::importDrawing(drawing.m_graphic, file, RS2::FormatDWG2018));
        const RS_MText* shown = only<RS_MText>(drawing, RS2::EntityMText);
        REQUIRE(shown->getStyle().compare(kAttributeStyle, Qt::CaseInsensitive) == 0);
        REQUIRE(shown->getLayer() != nullptr);
        REQUIRE(shown->getLayer()->getName().compare(kAttributeLayer, Qt::CaseInsensitive) == 0);
        if (variant.m_edit) {
            shown = drawing.edit(shown, variant.m_edit);
        }
        const auto out = save(bench, drawing, variant.m_name, target("dwg2018"));

        DwgContent written;
        readDwg(out, written);
        const DRW_Attrib& attribute = onlyDwgAttribute(written);
        const DRW_MText& mtext = onlyEmbeddedMText(written);
        const std::uint32_t layer = handleNamed(written.m_layers, kAttributeLayer);
        const std::uint32_t style = handleNamed(written.m_textStyles, kAttributeStyle);
        const std::uint32_t acad = handleNamed(written.m_applications, "ACAD");
        CAPTURE(layer, style, acad, source.m_layer, source.m_style);
        REQUIRE(layer != 0);
        REQUIRE(style != 0);
        REQUIRE(acad != 0);
        // A handle copied from the source would pass for the right one if
        // the two files gave the record the same handle.
        REQUIRE(layer != source.m_layer);
        REQUIRE(style != source.m_style);
        REQUIRE(layer != source.m_embeddedLayer);
        REQUIRE(acad != kForeignApplication);
        checkNoPrivateApplication(written);

        // The layer and the text style of the attribute, by their handles in
        // this file, for the attribute and for its MTEXT alike.
        CHECK(attribute.dwgLayerHandle() == layer);
        CHECK(mtext.dwgLayerHandle() == layer);
        CHECK(attribute.styleH.ref == style);
        CHECK(mtext.styleH.ref == style);
        CHECK(mtext.dwgLinetypeHandle() == 0u);
        CHECK(mtext.lineType == "BYLAYER");
        // The full-form R2018 fields name no application: the filter's fresh
        // MTEXT does not copy that source reference, and the Full writer
        // keeps its null value instead of the legacy Short default ACAD.
        CAPTURE(mtext.m_r2018AppIdHandle);
        CHECK(mtext.m_r2018AppIdHandle == 0u);
        CHECK(mtext.m_r2018IsNotAnnotative);

        // No other reference, and no flag that says there is one.
        CHECK(mtext.parentHandle == DRW::NoHandle);
        CHECK(mtext.reactorHandles.empty());
        CHECK(mtext.xDictHandle == 0u);
        CHECK_FALSE(mtext.hasDataStorageBinaryData());
        CHECK_FALSE(mtext.hasDwgAcDbColorHandle());
        CHECK(mtext.colorName.empty());
        CHECK(mtext.color == 256);
        CHECK(mtext.material == DRW::MaterialByLayer);
        CHECK(mtext.shadow == DRW::CastAndReceieveShadows);
        CHECK(mtext.shadowHandle == 0u);
        CHECK(mtext.plotStyle == DRW::DefaultPlotStyle);
        CHECK(mtext.fullVisualStyleHandle == 0u);
        CHECK(mtext.faceVisualStyleHandle == 0u);
        CHECK(mtext.edgeVisualStyleHandle == 0u);

        // What is a value and no reference is kept.
        CHECK(mtext.transparency == kMTextTransparency);
        CHECK(mtext.ltypeScale == 1.25);
        CHECK(mtext.lWeight == DRW_LW_Conv::width05);

        if (variant.m_edit) {
            CHECK(mtext.textgen == 9);
            CHECK(mtext.basePoint.x == Catch::Approx(shown->getInsertionPoint().x).margin(1e-9));
            CHECK(mtext.basePoint.y == Catch::Approx(shown->getInsertionPoint().y).margin(1e-9));
            CHECK(mtext.secPoint.x == Catch::Approx(std::cos(0.5)).margin(1e-9));
            CHECK(mtext.secPoint.y == Catch::Approx(std::sin(0.5)).margin(1e-9));
        }

        // And the file opens as what the drawing shows.
        EditableDrawing reopened;
        REQUIRE(lc::test::importDrawing(reopened.m_graphic, out, RS2::FormatDWG2018));
        const RS_MText* again = only<RS_MText>(reopened, RS2::EntityMText);
        checkSameMText(*shown, *again);
        REQUIRE(again->getLayer() != nullptr);
        CHECK(again->getLayer()->getName().compare(kAttributeLayer, Qt::CaseInsensitive) == 0);
    }
}

// The attribute's own value. Next to its MTEXT it is not shown: AutoCAD
// leaves it empty, and another program may have put a line there. A format
// before 2018 has no MTEXT in an attribute, and the value is all that is
// left of the text there: the text on one line, without the codes of an
// MTEXT.
//
// A value that is read back as a text of one line is compared here only
// where it has no backslash. Such a text is still read as if it were the
// content of an MTEXT, which turns the "\P" of "C:\Plans" into a line break
// (finding 8 of the review of pull request 3059, corrected separately). The
// cases with a backslash look at the value in the file.

TEST_CASE("a multi-line attribute keeps its text in a format without the embedded MTEXT",
          "[attribute-export][attrib][filter][dxf][dwg][plain-value]") {
    const Bench bench("attr_value_no_mtext");

    // A font with its options, a height, a tab, a degree sign and a stack.
    AttributeDxf coded;
    coded.m_text = R"({\fArial|b0|i0|c0|p34;ABC}\PDEF \H2x;tall ^Itab 50%%d \S1#2;)";
    const std::string codedInFile = "ABC DEF tall  tab 50%%D 1#2";
    const std::string codedShown = std::string("ABC DEF tall  tab 50") + kDegreeSign + " 1#2";

    SECTION("an empty value as AutoCAD writes it") {
        for (const Target& to : targetsWithoutMText()) {
            CAPTURE(to.m_name);
            EditableDrawing drawing;
            load(bench, coded, drawing, std::string(to.m_name) + "_source");
            const auto out = save(bench, drawing, "untouched", to);
            CHECK(attributeValueIn(out, to) == codedInFile);
            if (to.m_dxf) {
                checkWrittenDxf(out);
                CHECK(lc::test::fileBytes(out).find("Embedded Object") == std::string::npos);
            }
            const ShownAttribute shown = shownAttribute(out, to);
            CHECK_FALSE(shown.m_multiLine);
            CHECK(shown.m_text == codedShown);
            CHECK(shown.m_height == 9.0);
        }
    }

    // A change of the layout is no change of the text.
    SECTION("after the attribute was only moved") {
        for (const Target& to : targetsWithoutMText()) {
            CAPTURE(to.m_name);
            EditableDrawing drawing;
            load(bench, coded, drawing, std::string(to.m_name) + "_source");
            drawing.edit(only<RS_MText>(drawing, RS2::EntityMText), [](RS_MText& text) {
                text.move(RS_Vector(5.0, 0.0));
            });
            const auto out = save(bench, drawing, "moved", to);
            CHECK(attributeValueIn(out, to) == codedInFile);
            const ShownAttribute shown = shownAttribute(out, to);
            CHECK_FALSE(shown.m_multiLine);
            CHECK(shown.m_text == codedShown);
            CHECK(shown.m_at.x == Catch::Approx(6.0).margin(1e-9));
        }
    }

    // A line that another program left as the value is its stand-in for the
    // MTEXT, not a part of the text.
    SECTION("a value another program left there gives way to the text") {
        for (const char* format : {"dxf2007", "dwg2000", "dwg2013"}) {
            const Target to = target(format);
            CAPTURE(to.m_name);
            EditableDrawing drawing;
            load(bench, AttributeDxf::rich(), drawing, std::string(to.m_name) + "_source");
            const auto out = save(bench, drawing, "stand_in", to);
            CHECK(attributeValueIn(out, to) == "ABC DEF");
            CHECK(shownAttribute(out, to).m_text == "ABC DEF");
        }
    }

    SECTION("a value stays where the MTEXT has no text") {
        AttributeDxf fixture = AttributeDxf::rich();
        fixture.m_value = "first line";
        fixture.m_text = R"({\fArial;})";
        for (const char* format : {"dxf2007", "dwg2013"}) {
            const Target to = target(format);
            CAPTURE(to.m_name);
            EditableDrawing drawing;
            load(bench, fixture, drawing, std::string(to.m_name) + "_source");
            const auto out = save(bench, drawing, "only_codes", to);
            CHECK(attributeValueIn(out, to) == "first line");
        }
    }

    // "\\" is a backslash in an MTEXT, so the text here ends in C:\Plans\x.
    // The value is looked at in the file only: see the note on finding 8
    // above these cases.
    SECTION("a backslash of the text is a backslash of the value") {
        AttributeDxf fixture;
        fixture.m_text = R"({\fArial;ABC}\PC:\\Plans\\x)";
        for (const char* format : {"dxf2007", "dwg2013"}) {
            const Target to = target(format);
            CAPTURE(to.m_name);
            EditableDrawing drawing;
            load(bench, fixture, drawing, std::string(to.m_name) + "_source");
            const auto out = save(bench, drawing, "backslash", to);
            CHECK(attributeValueIn(out, to) == R"(ABC C:\Plans\x)");
        }
    }
}

TEST_CASE("an untouched multi-line attribute keeps the value it came with next to its MTEXT",
          "[attribute-export][attrib][filter][dxf][dwg][plain-value]") {
    const Bench bench("attr_value_kept");
    for (const char* value : {"", "first line"}) {
        const std::string name = *value == '\0' ? "empty" : "line";
        AttributeDxf fixture = AttributeDxf::rich();
        fixture.m_value = value;
        for (const char* format : {"dxf2018", "dwg2018"}) {
            const Target to = target(format);
            CAPTURE(name, to.m_name);
            EditableDrawing drawing;
            load(bench, fixture, drawing, name + "_" + to.m_name + "_source");
            const auto out = save(bench, drawing, name, to);
            CHECK(attributeValueIn(out, to) == value);
            const ShownAttribute shown = shownAttribute(out, to);
            CHECK(shown.m_multiLine);
            CHECK(shown.m_text == "ABC\nDEF");
        }
    }
}

TEST_CASE("the value of an edited multi-line attribute follows its text",
          "[attribute-export][attrib][filter][dxf][dwg][plain-value]") {
    const Bench bench("attr_value_edited");
    // What is typed into the text, and the value it gives in a DXF and in a
    // DWG: a caret is a caret and a blank in a DXF, where a caret starts a
    // code. The last column is what the file shows when it is opened; it is
    // empty where that cannot be compared: for a backslash see the note
    // above, and a DWG is not read back for a caret either, which is taken
    // for a code there.
    struct Row {
        const char* m_name;
        QString m_typed;
        std::string m_inDxf;
        std::string m_inDwg;
        std::string m_shown;
    };
    const std::string omega = std::string(kCapitalOmega) + " ABC";
    const std::vector<Row> rows{
        {"codes", QStringLiteral(R"({\fArial;big} \H2x;tall\Pnext 50%%d \S1/2;)"), "big tall next 50%%D 1/2",
         "big tall next 50%%D 1/2", std::string("big tall next 50") + kDegreeSign + " 1/2"},
        {"caret", QStringLiteral(R"({\fArial;big} \H2x;tall\Pnext 50%%d \S1^2;)"), "big tall next 50%%D 1^ 2",
         "big tall next 50%%D 1^2", ""},
        {"backslash", QStringLiteral(R"(a\\Path)"), R"(a\Path)", R"(a\Path)", ""},
        {"blanks", QStringLiteral("    ABC\n\tDEF"), "    ABC  DEF", "    ABC  DEF", "    ABC  DEF"},
        {"greek", QStringLiteral("\u03a9 ABC"), omega, omega, omega},
        {"nothing", QString(), "", "", ""},
    };

    for (const char* value : {"fallback", ""}) {
        const std::string stored = value;
        const std::string prefix = stored.empty() ? "empty_" : "line_";
        AttributeDxf fixture;
        fixture.m_value = stored;
        fixture.m_text = "old";
        for (const Row& row : rows) {
            for (const char* format : {"dxf2000", "dxf2007", "dxf2018", "dwg2000", "dwg2013", "dwg2018"}) {
                const Target to = target(format);
                // The two formats of 2000 have a code page, and none has
                // Greek: what they hold of the letter is the codec's matter.
                if (std::string(row.m_name) == "greek"
                    && (to.m_format == RS2::FormatDXFRW2000 || to.m_format == RS2::FormatDWG)) {
                    continue;
                }
                const std::string name = prefix + row.m_name;
                CAPTURE(name, to.m_name);
                EditableDrawing drawing;
                load(bench, fixture, drawing, name + "_" + to.m_name + "_source");
                drawing.edit(only<RS_MText>(drawing, RS2::EntityMText), [&](RS_MText& text) {
                    text.setText(row.m_typed);
                });
                const auto out = save(bench, drawing, name, to);
                const std::string plain = to.m_dxf ? row.m_inDxf : row.m_inDwg;
                if (to.m_dxf) {
                    checkWrittenDxf(out);
                }

                // Next to the MTEXT a value that was empty stays empty, as
                // AutoCAD writes it; one that was there follows the text.
                if (to.m_embedsMText) {
                    CHECK(attributeValueIn(out, to) == (stored.empty() ? std::string() : plain));
                    continue;
                }
                CHECK(attributeValueIn(out, to) == plain);
                const bool caretInDwg = !to.m_dxf && plain.find('^') != std::string::npos;
                if (!row.m_shown.empty() && !caretInDwg) {
                    const ShownAttribute shown = shownAttribute(out, to);
                    CHECK_FALSE(shown.m_multiLine);
                    CHECK(shown.m_text == row.m_shown);
                }
            }
        }
    }
}

TEST_CASE("a stored attribute value that a DXF line cannot hold is not written to DXF",
          "[attribute-export][attrib][filter][dxf][dwg][plain-value]") {
    const Bench bench("attr_value_unfit");
    EditableDrawing drawing;
    load(bench, AttributeDxf::rich(), drawing);
    // A text value of a DWG can hold a line break. No DXF can put one into
    // the value the text remembers, so it is put there by hand; the clone in
    // the place of the text has the text of the file.
    drawing.edit(only<RS_MText>(drawing, RS2::EntityMText), [](RS_MText& text) {
        REQUIRE(setStoredAttributeValue(text, "two\nlines"));
    });

    // To a DXF the text goes in its place: the export is not refused over
    // a value that is not shown.
    const Target dxf = target("dxf2018");
    const auto direct = save(bench, drawing, "unfit", dxf);
    checkWrittenDxf(direct);
    CHECK(attributeValueIn(direct, dxf) == "ABC DEF");
    CHECK(onlyRecord(direct, "ATTRIB").embedded(1) == std::vector<std::string>{"{\\fArial;ABC}\\PDEF"});

    // A DWG keeps the value as it is, and the drawing opened from that DWG
    // is saved to DXF like the one above.
    const Target dwg = target("dwg2018");
    const auto kept = save(bench, drawing, "unfit", dwg);
    CHECK(attributeValueIn(kept, dwg) == "two\nlines");
    EditableDrawing fromDwg;
    REQUIRE(lc::test::importDrawing(fromDwg.m_graphic, kept, RS2::FormatDWG2018));
    const auto viaDwg = save(bench, fromDwg, "unfit_dwg2018", dxf);
    CHECK(attributeValueIn(viaDwg, dxf) == "ABC DEF");
}

TEST_CASE("a format without the MTEXT of an attribute is not refused over what the MTEXT holds",
          "[attribute-export][attrib][filter][dxf][plain-value]") {
    const Bench bench("attr_value_mtext_unfit");
    EditableDrawing drawing;
    load(bench, AttributeDxf{}, drawing);
    // The content of an MTEXT in a DWG can hold a line break as it is, where
    // a DXF has the code "\P". No DXF can put one into the content the text
    // remembers, so it is put there by hand; the text of the drawing is the
    // same for both, and the attribute passes for untouched.
    drawing.edit(only<RS_MText>(drawing, RS2::EntityMText), [](RS_MText& text) {
        REQUIRE(setStoredMTextContent(text, "my text\nsecond line"));
    });

    // Untouched, the content goes back into the MTEXT as it was read, and
    // no line of a DXF holds this one. A DXF before 2018 writes no MTEXT
    // for an attribute, so there is nothing to refuse.
    for (const char* format : {"dxf2007", "dxf2000"}) {
        const Target to = target(format);
        CAPTURE(to.m_name);
        const auto out = save(bench, drawing, "unfit_content", to);
        checkWrittenDxf(out);
        CHECK(attributeValueIn(out, to) == "my text second line");
    }
}

TEST_CASE("a stored MTEXT content that a DXF line cannot hold is not written to DXF",
          "[attribute-export][attrib][filter][dxf][dwg][plain-value]") {
    const Bench bench("attr_content_unfit");
    // The content of an MTEXT in a DWG can hold a line break or a carriage
    // return as it is, where a DXF has the codes "\P" and "^M". The fixture
    // has the codes, and the character itself is put by hand into the content
    // the text remembers: the text of the drawing is the same for both, and
    // the attribute passes for untouched.
    struct Row {
        const char* m_name;
        const char* m_inDxf;
        const char* m_inDwg;
    };
    const std::vector<Row> rows{
        {"line_feed", "my text\\Psecond line", "my text\nsecond line"},
        {"carriage_return", "one^Mtwo", "one\rtwo"},
    };
    const Target dxf = target("dxf2018");
    const Target dwg = target("dwg2018");
    for (const Row& row : rows) {
        const std::string name = row.m_name;
        CAPTURE(name);
        const std::vector<std::string> inDxf{row.m_inDxf};
        AttributeDxf fixture;
        fixture.m_text = row.m_inDxf;
        EditableDrawing drawing;
        load(bench, fixture, drawing, name + "_source");
        drawing.edit(only<RS_MText>(drawing, RS2::EntityMText), [&](RS_MText& text) {
            REQUIRE(setStoredMTextContent(text, row.m_inDwg));
        });

        // To a DXF the text of the drawing goes in its place, spelled as a
        // DXF spells it: the export is not refused over a content that the
        // drawing shows all the same.
        const auto direct = save(bench, drawing, name, dxf);
        checkWrittenDxf(direct);
        CHECK(onlyRecord(direct, "ATTRIB").embedded(1) == inDxf);

        // A DWG keeps the content as it is, and the drawing opened from that
        // DWG is saved to DXF like the one above, also when the text was
        // only moved, which leaves its content alone.
        const auto kept = save(bench, drawing, name, dwg);
        DwgContent content;
        readDwg(kept, content);
        CHECK(onlyEmbeddedMText(content).text == row.m_inDwg);
        EditableDrawing fromDwg;
        REQUIRE(lc::test::importDrawing(fromDwg.m_graphic, kept, RS2::FormatDWG2018));
        const auto viaDwg = save(bench, fromDwg, name + "_dwg2018", dxf);
        checkWrittenDxf(viaDwg);
        CHECK(onlyRecord(viaDwg, "ATTRIB").embedded(1) == inDxf);
        fromDwg.edit(only<RS_MText>(fromDwg, RS2::EntityMText), [](RS_MText& text) {
            text.move(RS_Vector(5.0, 0.0));
        });
        const auto moved = save(bench, fromDwg, name + "_dwg2018_moved", dxf);
        checkWrittenDxf(moved);
        CHECK(onlyRecord(moved, "ATTRIB").embedded(1) == inDxf);
        // And saved to a DWG again, it has the content of the DWG.
        const auto again = save(bench, fromDwg, name + "_dwg2018_moved", dwg);
        DwgContent contentAgain;
        readDwg(again, contentAgain);
        CHECK(onlyEmbeddedMText(contentAgain).text == row.m_inDwg);
    }
}

TEST_CASE("a multi-line attribute keeps its MTEXT in the formats of 2018 whatever its type says",
          "[attribute-export][attrib][filter][dxf][dwg][plain-value]") {
    const Bench bench("attr_value_type");
    // The type of an attribute is its group 71 before the embedded object: 2
    // for a multi-line attribute. A file may have none, or a 0 or a 1 there.
    // The reader makes a 2 of each, and the export does not rely on that:
    // the last two variants have the 0 and the 1 in what the text remembers
    // of its ATTRIB. A DWG has the MTEXT only in an attribute of type 2.
    struct Variant {
        const char* m_name;
        const char* m_typeGroups;
        int m_storedType;   // -1: as read
    };
    const std::vector<Variant> variants{
        {"none", "", -1},
        {"zero", "71\n0\n72\n0\n", -1},
        {"one", "71\n1\n72\n0\n", -1},
        {"stored_zero", "71\n2\n72\n0\n", 0},
        {"stored_one", "71\n2\n72\n0\n", 1},
    };
    for (const Variant& variant : variants) {
        AttributeDxf fixture;
        fixture.m_typeGroups = variant.m_typeGroups;
        for (const char* format : {"dwg2018", "dxf2018"}) {
            const Target to = target(format);
            CAPTURE(variant.m_name, to.m_name);
            EditableDrawing drawing;
            load(bench, fixture, drawing, std::string(variant.m_name) + "_" + to.m_name + "_source");
            if (variant.m_storedType >= 0) {
                drawing.edit(only<RS_MText>(drawing, RS2::EntityMText), [&](RS_MText& text) {
                    REQUIRE(setStoredAttributeType(text, variant.m_storedType));
                });
            }
            const auto out = save(bench, drawing, variant.m_name, to);
            const ShownAttribute shown = shownAttribute(out, to);
            CHECK(shown.m_multiLine);
            CHECK(shown.m_text == "my text\nsecond line");
            if (to.m_dxf) {
                checkWrittenDxf(out);
                // Of the attribute itself: the text generation, then the type.
                CHECK(onlyRecord(out, "ATTRIB").own(71) == std::vector<std::string>{"0", "2"});
            }
            else {
                DwgContent content;
                readDwg(out, content);
                CHECK(onlyDwgAttribute(content).m_attributeType == 2);
            }
        }
    }
}

// A justified text is placed by its alignment point (group 11) and its
// justification (72, and 74 in an attribute); its first point (group 10) is
// where its baseline starts. The drawing has the alignment point as the
// insertion point of the text and works the first point out when it lays the
// text out. An aligned text has its two ends in the two points.

TEST_CASE("a justified single-line attribute does not move when it is saved",
          "[attribute-export][attrib][filter][dxf][dwg][attrib-geometry]") {
    const Bench bench("attr_single_line_points");
    // TOP has the numbers of an attribute that AutoCAD wrote: top left, its
    // first point one height below its alignment point.
    const std::string source = singleLineAttributesDxf({
        singleLineAttrib("TOP", "994.5582906791903", "41.70948425675614", "1.720654456209621", "hello",
                         "11\n994.5582906791903\n21\n43.43013871296577\n31\n0\n", "3"),
        singleLineAttrib("CENTER", "6.95", "0.45", "0.1", "A", "72\n1\n11\n7\n21\n0.45\n31\n0\n"),
        singleLineAttrib("LEFT", "3", "5", "1", "left", ""),
        singleLineAttrib("ALIGNED", "10", "20", "1", "ends", "72\n3\n11\n14\n21\n20\n31\n0\n"),
    });
    const auto checkPlaces = [](EditableDrawing& drawing) {
        const RS_Vector top(994.5582906791903, 43.43013871296577);
        CHECK(textWithValue(drawing, "hello")->getInsertionPoint().distanceTo(top) < 1e-9);
        CHECK(textWithValue(drawing, "A")->getInsertionPoint().distanceTo(RS_Vector(7.0, 0.45)) < 1e-9);
        CHECK(textWithValue(drawing, "left")->getInsertionPoint().distanceTo(RS_Vector(3.0, 5.0)) < 1e-9);
        const RS_Text* aligned = textWithValue(drawing, "ends");
        CHECK(aligned->getInsertionPoint().distanceTo(RS_Vector(10.0, 20.0)) < 1e-9);
        CHECK(aligned->getSecondPoint().distanceTo(RS_Vector(14.0, 20.0)) < 1e-9);
    };

    for (const char* format : {"dxf2018", "dxf2007", "dwg2013"}) {
        const Target to = target(format);
        auto drawing = std::make_unique<EditableDrawing>();
        loadDxf(bench, source, *drawing, std::string(to.m_name) + "_source");
        checkPlaces(*drawing);

        // The first points that the first save wrote, by tag: a point that is
        // worked out has to come out the same from the file it was saved to.
        std::map<std::string, std::vector<double>> firstSave;
        for (int round = 1; round <= 3; ++round) {
            CAPTURE(to.m_name, round);
            const auto out = save(bench, *drawing, "round" + std::to_string(round), to);
            auto reopened = std::make_unique<EditableDrawing>();
            REQUIRE(lc::test::importDrawing(reopened->m_graphic, out, to.m_format));
            checkPlaces(*reopened);
            drawing = std::move(reopened);
            if (!to.m_dxf) {
                continue;
            }
            checkWrittenDxf(out);
            {
                const DxfRecord record = attributeRecord(out, "TOP");
                const std::vector<double> points = ownNumbers(record, {10, 20, 11, 21});
                CAPTURE(points);
                CHECK(sameNumbers({994.5582906791903, 43.43013871296577 - 1.720654456209621, 994.5582906791903,
                                   43.43013871296577},
                                  points));
                CHECK(record.own(72).empty());
                CHECK(record.own(74) == std::vector<std::string>{"3"});
            }
            {
                // Half the width of the letter to the left of the point it is
                // centred on, and on its line.
                const DxfRecord record = attributeRecord(out, "CENTER");
                const std::vector<double> points = ownNumbers(record, {10, 20, 11, 21});
                CAPTURE(points);
                REQUIRE(points.size() == 4);
                CHECK(points[0] < 7.0);
                CHECK(points[0] > 6.9);
                CHECK(sameNumbers({0.45, 7.0, 0.45}, {points[1], points[2], points[3]}));
                CHECK(record.own(72) == std::vector<std::string>{"1"});
                CHECK(record.own(74).empty());
            }
            {
                const DxfRecord record = attributeRecord(out, "LEFT");
                CHECK(sameNumbers({3.0, 5.0}, ownNumbers(record, {10, 20})));
                CHECK(record.own(11).empty());
                CHECK(record.own(72).empty());
                CHECK(record.own(74).empty());
            }
            {
                const DxfRecord record = attributeRecord(out, "ALIGNED");
                const std::vector<double> points = ownNumbers(record, {10, 20, 11, 21});
                CAPTURE(points);
                CHECK(sameNumbers({10.0, 20.0, 14.0, 20.0}, points));
                CHECK(record.own(72) == std::vector<std::string>{"3"});
            }
            for (const char* tag : {"TOP", "CENTER", "LEFT", "ALIGNED"}) {
                const std::vector<double> first = ownNumbers(attributeRecord(out, tag), {10, 20});
                if (round == 1) {
                    firstSave[tag] = first;
                }
                CAPTURE(tag, first, firstSave[tag]);
                CHECK(sameNumbers(firstSave[tag], first));
            }
        }
    }
}

TEST_CASE("an attribute keeps its own width factor and text generation flags",
          "[attribute-export][attrib][filter][dxf][dwg][attrib-geometry]") {
    const Bench bench("attr_width_generation");

    // A multi-line text has neither. The 41 and the 71 of the attribute
    // itself are for the value of the attribute, where that is what is shown.
    SECTION("the width factor and the generation flags of a multi-line attribute") {
        AttributeDxf fixture;
        fixture.m_textExtra = "41\n0.8\n71\n2\n";
        for (const char* format : {"dxf2018", "dxf2007", "dwg2018", "dwg2013"}) {
            const Target to = target(format);
            CAPTURE(to.m_name);
            EditableDrawing drawing;
            load(bench, fixture, drawing, std::string(to.m_name) + "_source");
            REQUIRE(attributeText(drawing)->rtti() == RS2::EntityMText);
            const auto out = save(bench, drawing, "width", to);
            if (to.m_dxf) {
                checkWrittenDxf(out);
                const DxfRecord record = onlyRecord(out, "ATTRIB");
                CHECK(sameNumbers({0.8}, ownNumbers(record, {41})));
                // Next to the MTEXT the attribute has a second 71, its type.
                const std::vector<std::string> generation = record.own(71);
                REQUIRE(generation.size() == (to.m_embedsMText ? 2u : 1u));
                CHECK(generation.front() == "2");
            }
            else {
                DwgContent content;
                readDwg(out, content);
                CHECK(onlyDwgAttribute(content).widthscale == 0.8);
                CHECK(onlyDwgAttribute(content).textgen == 2);
            }
        }
    }

    // Group 71 of a text: 2 for backward, 4 for upside down, 6 for both. A
    // text of the drawing is one of the two at most, and a 6 is shown as
    // neither: it is kept as long as the drawing does not say otherwise. A
    // value that means nothing, such as a 3, is shown as neither too and is
    // written as 0, so that the text is shown the same when it is read back.
    SECTION("the generation flags of a single-line attribute") {
        struct Row {
            const char* m_name;
            const char* m_stored;                       // group 71 of the source
            bool m_edited;
            RS_TextData::TextGeneration m_generation;   // what an edit makes of the text
            int m_written;
        };
        const std::vector<Row> rows{
            {"both", "6", false, RS_TextData::None, 6},
            {"both_to_backward", "6", true, RS_TextData::Backward, 2},
            {"backward", "2", false, RS_TextData::None, 2},
            {"backward_to_none", "2", true, RS_TextData::None, 0},
            {"none_to_upside_down", "0", true, RS_TextData::UpsideDown, 4},
            {"another_value", "3", false, RS_TextData::None, 0},
        };
        for (const Row& row : rows) {
            AttributeDxf fixture = plainAttributeDxf("FLAGS");
            fixture.m_textExtra = std::string("71\n") + row.m_stored + "\n";
            for (const char* format : {"dxf2007", "dxf2018", "dwg2013"}) {
                const Target to = target(format);
                CAPTURE(row.m_name, to.m_name);
                EditableDrawing drawing;
                load(bench, fixture, drawing, std::string(row.m_name) + "_" + to.m_name + "_source");
                if (row.m_edited) {
                    drawing.edit(only<RS_Text>(drawing, RS2::EntityText), [&](RS_Text& text) {
                        text.setTextGeneration(row.m_generation);
                    });
                }
                const auto out = save(bench, drawing, row.m_name, to);
                if (to.m_dxf) {
                    checkWrittenDxf(out);
                    CHECK(onlyRecord(out, "ATTRIB").own(71) == std::vector<std::string>{std::to_string(row.m_written)});
                }
                else {
                    DwgContent content;
                    readDwg(out, content);
                    CHECK(onlyDwgAttribute(content).textgen == row.m_written);
                }
            }
        }
    }
}

// A text with no letter to draw has no size, and the first point of a
// justified one cannot be worked out from its letters: it is the alignment
// point, lowered as a capital letter would be. A value of blanks is such a
// text, and so is the empty value of an attribute that was not filled in.
TEST_CASE("the first point of a single-line attribute with nothing to draw stays at its alignment point",
          "[attribute-export][attrib][filter][dxf][dwg][attrib-geometry]") {
    const Bench bench("attr_blank_first_point");
    // Height 2, aligned to (50,20): to the right on the baseline, in the
    // middle, centred with the vertical middle, at the top, centred at the
    // bottom, and in the middle of a text turned by 90 degrees. The last
    // columns are where the baseline of a capital letter would start: on the
    // alignment point, half a height below it, a whole height below it, a
    // third of a height above it (so far the line below the baseline hangs),
    // and for the turned text half a height to the right.
    struct Kind {
        const char* m_name;
        const char* m_textGroups;   // 50 and 72
        const char* m_vertical;
        double m_firstX;
        double m_firstY;
    };
    const std::vector<Kind> kinds{
        {"RIGHT", "72\n2\n", "0", 50.0, 20.0},
        {"MIDDLE", "72\n4\n", "0", 50.0, 19.0},
        {"CENTER", "72\n1\n", "2", 50.0, 19.0},
        {"TOP", "", "3", 50.0, 18.0},
        {"BOTTOM", "72\n1\n", "1", 50.0, 20.0 + 2.0 / 3.0},
        {"TURNED", "50\n90\n72\n4\n", "0", 51.0, 20.0},
    };
    std::vector<std::string> attributes;
    for (const char* value : {"", "   "}) {
        for (const Kind& kind : kinds) {
            const std::string tag = std::string(*value == '\0' ? "EMPTY_" : "BLANK_") + kind.m_name;
            attributes.push_back(singleLineAttrib(tag, "50", "20", "2", value,
                                                  std::string(kind.m_textGroups) + "11\n50\n21\n20\n31\n0\n",
                                                  kind.m_vertical));
        }
    }
    const auto checkPlaces = [&kinds](EditableDrawing& drawing) {
        const std::vector<const RS_Text*> texts = drawing.live<RS_Text>(RS2::EntityText);
        CHECK(texts.size() == 2 * kinds.size());
        int turned = 0;
        for (const RS_Text* text : texts) {
            CAPTURE(text->getInsertionPoint().x, text->getInsertionPoint().y);
            CHECK(text->getInsertionPoint().distanceTo(RS_Vector(50.0, 20.0)) < 1e-9);
            turned += std::abs(text->getAngle() - M_PI / 2.0) < 1e-9 ? 1 : 0;
        }
        CHECK(turned == 2);
    };
    const auto checkRecords = [&kinds](const std::filesystem::path& file) {
        for (const char* prefix : {"EMPTY_", "BLANK_"}) {
            for (const Kind& kind : kinds) {
                const std::string tag = std::string(prefix) + kind.m_name;
                const std::vector<double> points = ownNumbers(attributeRecord(file, tag), {10, 20, 11, 21});
                CAPTURE(file.filename().string(), tag, points);
                CHECK(sameNumbers({kind.m_firstX, kind.m_firstY, 50.0, 20.0}, points));
            }
        }
    };

    EditableDrawing drawing;
    loadDxf(bench, singleLineAttributesDxf(attributes), drawing);
    checkPlaces(drawing);

    const Target dxf = target("dxf2007");
    const auto direct = save(bench, drawing, "untouched", dxf);
    checkWrittenDxf(direct);
    checkRecords(direct);
    EditableDrawing reopened;
    REQUIRE(lc::test::importDrawing(reopened.m_graphic, direct, dxf.m_format));
    checkPlaces(reopened);

    // The same through a DWG, whose content is read as the DXF that the
    // drawing it opens as is saved to.
    const Target dwg = target("dwg2013");
    const auto viaDwg = save(bench, drawing, "untouched", dwg);
    EditableDrawing fromDwg;
    REQUIRE(lc::test::importDrawing(fromDwg.m_graphic, viaDwg, dwg.m_format));
    checkPlaces(fromDwg);
    const auto again = save(bench, fromDwg, "untouched_dwg2013", dxf);
    checkWrittenDxf(again);
    checkRecords(again);
}

// The attribute itself is a text of one line: its value, which a format
// without its MTEXT shows, with a first point (10), an alignment point (11)
// and a justification (72, 74) of its own. AutoCAD keeps them in step with
// the MTEXT: the alignment point is the insertion point of the MTEXT, the
// justification follows its attachment point, and the first point is where
// the baseline of the value starts. In the sample it wrote, a top-left
// attribute of height 1.72 at y 28.75 has its first point at y 27.03.

// A multi-line attribute, top left at (1,4) with a text of one line in a
// font of the drawing. The first point of its file is not where it is worked
// out here, a quarter above: kept and worked out give different numbers.
AttributeDxf offsetFirstPointDxf() {
    AttributeDxf fixture;
    fixture.m_firstPoint = "10\n1\n20\n-4.75\n30\n0\n";
    fixture.m_text = "ABC";
    fixture.m_style = "iso3098";
    fixture.m_direction = "1";
    return fixture;
}

TEST_CASE("an edited multi-line attribute gets its own points and justification from its MTEXT",
          "[attribute-export][attrib][filter][dxf][dwg][attrib-geometry]") {
    const Bench bench("attr_outer_edited");

    SECTION("a change of the text") {
        EditableDrawing drawing;
        load(bench, offsetFirstPointDxf(), drawing);
        drawing.edit(only<RS_MText>(drawing, RS2::EntityMText), [](RS_MText& text) { text.setText("ABD"); });
        std::filesystem::path saved2018;
        for (const char* format : {"dxf2018", "dxf2007"}) {
            const Target to = target(format);
            CAPTURE(to.m_name);
            const auto out = save(bench, drawing, "text", to);
            checkWrittenDxf(out);
            const DxfRecord record = onlyRecord(out, "ATTRIB");
            const std::vector<double> place = ownPlace(record);
            CAPTURE(place);
            // The first point is worked out: one height below the alignment
            // point, and no longer the -4.75 of the file.
            CHECK(sameNumbers({1.0, -5.0, 1.0, 4.0}, place));
            CHECK(record.own(74) == std::vector<std::string>{"3"});
            CHECK(record.own(72) == ownGroup72("", to));
            if (to.m_embedsMText) {
                saved2018 = out;
            }
        }

        // The saved file opened and saved again, with nothing changed, to a
        // format that shows the value: the same record, and a text of one
        // line where the multi-line text was.
        EditableDrawing reopened;
        REQUIRE(lc::test::importDrawing(reopened.m_graphic, saved2018, RS2::FormatDXFRW2018));
        const Target older = target("dxf2007");
        const auto again = save(bench, reopened, "text_reopened", older);
        const DxfRecord record = onlyRecord(again, "ATTRIB");
        CHECK(sameNumbers({1.0, -5.0, 1.0, 4.0}, ownPlace(record)));
        CHECK(record.own(74) == std::vector<std::string>{"3"});
        CHECK(record.own(72).empty());
        EditableDrawing shown;
        REQUIRE(lc::test::importDrawing(shown.m_graphic, again, older.m_format));
        const RS_Text* text = only<RS_Text>(shown, RS2::EntityText);
        CHECK(text->getText().toStdString() == "ABD");
        CHECK(text->getInsertionPoint().distanceTo(RS_Vector(1.0, 4.0)) < 1e-9);
        CHECK(text->getVAlign() == RS_TextData::VATop);
        CHECK(text->getHAlign() == RS_TextData::HALeft);
    }

    SECTION("another attachment point and another place") {
        EditableDrawing drawing;
        load(bench, offsetFirstPointDxf(), drawing);
        const RS_MText* moved = drawing.edit(only<RS_MText>(drawing, RS2::EntityMText), [](RS_MText& text) {
            text.setHAlign(RS_MTextData::HARight);
            text.setVAlign(RS_MTextData::VAMiddle);
            text.move(RS_Vector(6.0, 4.0));
        });
        REQUIRE(moved->getInsertionPoint().distanceTo(RS_Vector(7.0, 8.0)) < 1e-12);
        for (const char* format : {"dxf2018", "dxf2007", "dwg2013"}) {
            const Target to = target(format);
            CAPTURE(to.m_name);
            const auto out = save(bench, drawing, "right_middle", to);
            std::vector<double> place;
            if (to.m_dxf) {
                checkWrittenDxf(out);
                const DxfRecord record = onlyRecord(out, "ATTRIB");
                place = ownPlace(record);
                CAPTURE(place);
                CHECK(sameNumbers({7.0, 8.0}, {place[2], place[3]}));
                CHECK(record.own(74) == std::vector<std::string>{"2"});
                CHECK(record.own(72) == ownGroup72("2", to));
            }
            if (to.m_embedsMText) {
                // The value next to the MTEXT is empty, and nothing has a
                // width: half a height below the alignment point.
                CHECK(sameNumbers({7.0, 3.5}, {place[0], place[1]}));
                continue;
            }
            // Without the MTEXT the value is the text. Its first point is
            // where a text of one line, right-aligned to (7,8) by its
            // middle, starts: the very point that the text opened from the
            // file works out for itself.
            EditableDrawing shown;
            REQUIRE(lc::test::importDrawing(shown.m_graphic, out, to.m_format));
            const RS_Text* text = only<RS_Text>(shown, RS2::EntityText);
            CHECK(text->getText().toStdString() == "ABC");
            CHECK(text->getInsertionPoint().distanceTo(RS_Vector(7.0, 8.0)) < 1e-9);
            CHECK(text->getHAlign() == RS_TextData::HARight);
            CHECK(text->getVAlign() == RS_TextData::VAMiddle);
            if (to.m_dxf) {
                CHECK(place[0] < 7.0);
                CHECK(place[1] > -1.0);
                CHECK(place[1] < 8.0);
                CHECK(text->getSecondPoint().distanceTo(RS_Vector(place[0], place[1])) < 1e-6);
            }
        }
    }

    // The value is a text of one line: "\P", "\~" and a font in braces are
    // characters of it like any other, and each has its width. The first
    // point of a text that ends at its alignment point is where all of them
    // start, which is again the point that the text opened from the file
    // works out for itself, and the point it is saved with from there. A
    // DWG has the characters themselves where a DXF has a code for a caret,
    // and the same first point: what the value shows is one text.
    SECTION("a value with a backslash or a caret is laid out as the characters it has") {
        struct Row {
            const char* m_name;
            const char* m_typed;    // in an MTEXT "\\" is a backslash, "\{" a brace
            const char* m_inDxf;
            const char* m_shown;    // also the value in a DWG
        };
        const std::vector<Row> rows{
            {"paragraph", R"(C:\\Plans\\x)", R"(C:\Plans\x)", R"(C:\Plans\x)"},
            {"hard_space", R"(a\\~b)", R"(a\~b)", R"(a\~b)"},
            {"font", R"(\{\\fArial|b0;T\})", R"({\fArial|b0;T})", R"({\fArial|b0;T})"},
            {"caret", "a^Jb", "a^ Jb", "a^Jb"},
        };
        const Target dxf = target("dxf2007");
        const Target dwg = target("dwg2013");
        for (const Row& row : rows) {
            const std::string name = std::string("coded_") + row.m_name;
            CAPTURE(name);
            EditableDrawing drawing;
            load(bench, offsetFirstPointDxf(), drawing, name + "_source");
            drawing.edit(only<RS_MText>(drawing, RS2::EntityMText), [&](RS_MText& text) {
                text.setText(QString::fromLatin1(row.m_typed));
                text.setHAlign(RS_MTextData::HARight);
            });
            // The first point of the attribute in a file, of either kind.
            const auto firstPoint = [](const std::filesystem::path& file, const Target& of) {
                if (of.m_dxf) {
                    const std::vector<double> place = ownPlace(onlyRecord(file, "ATTRIB"));
                    return RS_Vector(place[0], place[1]);
                }
                DwgContent content;
                readDwg(file, content);
                const DRW_Coord& point = onlyDwgAttribute(content).basePoint;
                return RS_Vector(point.x, point.y);
            };
            // The text a file opens with has the value, ends at (1,4) and
            // starts where the file says; saved from there, the file says
            // the same again.
            const auto checkShown = [&](const std::filesystem::path& file, const Target& of, const RS_Vector& first) {
                CAPTURE(of.m_name);
                EditableDrawing shown;
                REQUIRE(lc::test::importDrawing(shown.m_graphic, file, of.m_format));
                const RS_Text* text = only<RS_Text>(shown, RS2::EntityText);
                CHECK(text->getText().toStdString() == row.m_shown);
                CHECK(text->getInsertionPoint().distanceTo(RS_Vector(1.0, 4.0)) < 1e-9);
                CHECK(text->getHAlign() == RS_TextData::HARight);
                CHECK(text->getVAlign() == RS_TextData::VATop);
                CAPTURE(first.x, first.y, text->getSecondPoint().x, text->getSecondPoint().y);
                CHECK(text->getSecondPoint().distanceTo(first) < 1e-6);
                const auto again = save(bench, shown, name + "_reopened", of);
                CHECK(attributeValueIn(again, of) == attributeValueIn(file, of));
                CHECK(firstPoint(again, of).distanceTo(first) < 1e-6);
            };

            const auto inDxf = save(bench, drawing, name, dxf);
            checkWrittenDxf(inDxf);
            CHECK(attributeValueIn(inDxf, dxf) == row.m_inDxf);
            const RS_Vector first = firstPoint(inDxf, dxf);
            CHECK(first.x < 1.0);
            checkShown(inDxf, dxf, first);

            const auto inDwg = save(bench, drawing, name, dwg);
            CHECK(attributeValueIn(inDwg, dwg) == row.m_shown);
            CHECK(firstPoint(inDwg, dwg).distanceTo(first) < 1e-6);
            // A caret of a DWG is read as the start of a code.
            if (std::string(row.m_shown).find('^') == std::string::npos) {
                checkShown(inDwg, dwg, first);
            }
        }
    }

    // The value that a file came with stays next to the MTEXT while the text
    // is as it was. It is a text of one line too: once the attribute ends at
    // another point, its first point is where a text of one line with that
    // value starts, the backslashes and the letters after them included.
    SECTION("a stored value with a backslash is laid out as the characters it has") {
        const std::string value = R"(C:\Plans\x)";
        AttributeDxf fixture = offsetFirstPointDxf();
        fixture.m_value = value;
        EditableDrawing drawing;
        load(bench, fixture, drawing, "stored_source");
        drawing.edit(only<RS_MText>(drawing, RS2::EntityMText), [](RS_MText& text) {
            text.setHAlign(RS_MTextData::HARight);
            text.move(RS_Vector(6.0, 0.0));
        });
        const RS_Vector end(7.0, 4.0);
        const RS_TextData data(end, end, 9.0, 1.0, RS_TextData::VATop, RS_TextData::HARight, RS_TextData::None,
                               QString::fromStdString(value), QStringLiteral("iso3098"), 0.0, RS2::Update);
        const RS_Text line(nullptr, data);
        const RS_Vector start = line.getSecondPoint();
        CAPTURE(start.x, start.y);
        REQUIRE(start.x < 7.0 - 9.0);

        const Target dxf = target("dxf2018");
        const auto inDxf = save(bench, drawing, "stored", dxf);
        checkWrittenDxf(inDxf);
        CHECK(attributeValueIn(inDxf, dxf) == value);
        const std::vector<double> place = ownPlace(onlyRecord(inDxf, "ATTRIB"));
        CAPTURE(place);
        CHECK(sameNumbers({7.0, 4.0}, {place[2], place[3]}));
        CHECK(start.distanceTo(RS_Vector(place[0], place[1])) < 1e-6);

        const Target dwg = target("dwg2018");
        const auto inDwg = save(bench, drawing, "stored", dwg);
        DwgContent content;
        readDwg(inDwg, content);
        const DRW_Attrib& attribute = onlyDwgAttribute(content);
        CHECK(attribute.text == value);
        CAPTURE(attribute.basePoint.x, attribute.basePoint.y);
        CHECK(start.distanceTo(RS_Vector(attribute.basePoint.x, attribute.basePoint.y)) < 1e-6);
    }
}

TEST_CASE("an untouched multi-line attribute keeps its first point while its record is written as it was",
          "[attribute-export][attrib][filter][dxf][dwg][attrib-geometry]") {
    const Bench bench("attr_outer_untouched");

    SECTION("the numbers of an attribute that AutoCAD wrote") {
        AttributeDxf fixture;
        fixture.m_firstPoint = "10\n977.2814835151885\n20\n27.03015976727258\n30\n0\n";
        fixture.m_height = "1.720654456209621";
        fixture.m_alignmentPoint = "11\n977.2814835151885\n21\n28.7508142234822\n31\n0\n";
        fixture.m_mtextPoint = "10\n977.2814835151885\n20\n28.7508142234822\n30\n0\n";
        fixture.m_text = "my multi line text";
        const std::vector<double> points{977.2814835151885, 27.03015976727258, 977.2814835151885, 28.7508142234822};
        EditableDrawing drawing;
        load(bench, fixture, drawing);

        // Number for number: nothing is worked out for a record that is
        // written as it was read.
        const auto direct = save(bench, drawing, "untouched", target("dxf2018"));
        CHECK(ownPlace(onlyRecord(direct, "ATTRIB")) == points);
        CHECK(onlyRecord(direct, "ATTRIB").own(74) == std::vector<std::string>{"3"});

        // The same through a DWG, which is read here as the DXF that the
        // drawing it opens as is saved to.
        const auto dwg = save(bench, drawing, "untouched", target("dwg2018"));
        EditableDrawing fromDwg;
        REQUIRE(lc::test::importDrawing(fromDwg.m_graphic, dwg, RS2::FormatDWG2018));
        const auto viaDwg = save(bench, fromDwg, "untouched_dwg2018", target("dxf2018"));
        CHECK(ownPlace(onlyRecord(viaDwg, "ATTRIB")) == points);
        CHECK(onlyRecord(viaDwg, "ATTRIB").own(74) == std::vector<std::string>{"3"});

        // A format without the MTEXT gets the text as the value, which is
        // not the empty value the first point was read with. Worked out, the
        // point is AutoCAD's: one height below the alignment point.
        const auto older = save(bench, drawing, "untouched", target("dxf2007"));
        const std::vector<double> place = ownPlace(onlyRecord(older, "ATTRIB"));
        CAPTURE(place);
        REQUIRE(place.size() == 4);
        CHECK(place[2] == points[2]);
        CHECK(place[3] == points[3]);
        CHECK(sameNumbers({points[0], points[1]}, {place[0], place[1]}));
        CHECK(onlyRecord(older, "ATTRIB").own(74) == std::vector<std::string>{"3"});
    }

    SECTION("a first point that is not where it would be worked out") {
        const auto firstPoint = [](const std::filesystem::path& file) {
            return ownNumbers(onlyRecord(file, "ATTRIB"), {10, 20});
        };
        const std::vector<double> kept{1.0, -4.75};
        const std::vector<double> workedOut{1.0, -5.0};
        {
            EditableDrawing drawing;
            load(bench, offsetFirstPointDxf(), drawing, "kept_source");
            CHECK(firstPoint(save(bench, drawing, "kept", target("dxf2018"))) == kept);
            const auto dwg = save(bench, drawing, "kept", target("dwg2018"));
            EditableDrawing fromDwg;
            REQUIRE(lc::test::importDrawing(fromDwg.m_graphic, dwg, RS2::FormatDWG2018));
            CHECK(firstPoint(save(bench, fromDwg, "kept_dwg2018", target("dxf2018"))) == kept);
            // The value changes with the format, and the point with it.
            CHECK(sameNumbers(workedOut, firstPoint(save(bench, drawing, "kept", target("dxf2007")))));

            // The line spacing moves nothing of the attribute itself.
            drawing.edit(only<RS_MText>(drawing, RS2::EntityMText), [](RS_MText& text) {
                text.setLineSpacingFactor(1.25);
            });
            CHECK(firstPoint(save(bench, drawing, "spacing", target("dxf2018"))) == kept);
        }
        {
            // With the text as the value in the file already, a format
            // without the MTEXT writes the record as it was too.
            AttributeDxf fixture = offsetFirstPointDxf();
            fixture.m_value = "ABC";
            EditableDrawing drawing;
            load(bench, fixture, drawing, "value_source");
            CHECK(firstPoint(save(bench, drawing, "value", target("dxf2007"))) == kept);
            CHECK(firstPoint(save(bench, drawing, "value", target("dxf2018"))) == kept);
        }
        {
            // Another height: the point is one new height below.
            EditableDrawing drawing;
            load(bench, offsetFirstPointDxf(), drawing, "height_source");
            drawing.edit(only<RS_MText>(drawing, RS2::EntityMText), [](RS_MText& text) { text.setHeight(3.0); });
            CHECK(sameNumbers({1.0, 1.0}, firstPoint(save(bench, drawing, "height", target("dxf2018")))));
        }
    }

    // A record as earlier versions of LibreCAD wrote it: the attribute
    // itself at the place of the MTEXT without a justification, and only the
    // copy of the alignment point in front of the MTEXT. Its first point
    // belongs to no justification that is written now.
    SECTION("a record without a justification gets one") {
        AttributeDxf fixture = offsetFirstPointDxf();
        fixture.m_firstPoint = "10\n1\n20\n4\n30\n0\n";
        fixture.m_alignmentPoint.clear();
        fixture.m_verticalJustification = "0";
        fixture.m_typeGroups = "71\n2\n72\n0\n11\n1\n21\n4\n31\n0\n";
        for (const char* format : {"dxf2018", "dxf2007"}) {
            const Target to = target(format);
            CAPTURE(to.m_name);
            EditableDrawing drawing;
            load(bench, fixture, drawing, std::string(to.m_name) + "_source");
            const DxfRecord record = onlyRecord(save(bench, drawing, "untouched", to), "ATTRIB");
            CHECK(sameNumbers({1.0, -5.0, 1.0, 4.0}, ownPlace(record)));
            CHECK(record.own(74) == std::vector<std::string>{"3"});
            CHECK(record.own(72) == ownGroup72("", to));
        }
    }
}

// A block reference seen from below has the extrusion (0,0,-1), and so have
// its attributes. The points of the attribute itself are in the coordinate
// system of that extrusion, in which x is mirrored; those of its MTEXT are
// in the coordinate system of the drawing.
TEST_CASE("the points of an extruded multi-line attribute stay in its own coordinate system",
          "[attribute-export][attrib][filter][dxf][attrib-geometry]") {
    const Bench bench("attr_outer_extruded");
    const std::string down = "210\n0\n220\n0\n230\n-1\n";
    AttributeDxf fixture;
    fixture.m_insertGroups = "10\n0\n20\n0\n30\n0\n" + down;
    fixture.m_firstPoint = "10\n-5\n20\n4\n30\n0\n";
    fixture.m_value = "fallback";
    fixture.m_alignmentPoint.clear();
    fixture.m_verticalJustification = "0";
    fixture.m_textTail = down;
    fixture.m_typeGroups = "71\n2\n72\n0\n11\n5\n21\n4\n31\n0\n";
    fixture.m_mtextPoint = "10\n5\n20\n4\n30\n0\n";
    fixture.m_text = "ABC";
    fixture.m_style = "iso3098";
    fixture.m_direction = "1";
    fixture.m_mtextExtra = down;

    struct Variant {
        const char* m_name;
        std::function<void(RS_MText&)> m_edit;  // none: untouched
        std::vector<double> m_own;              // first point and alignment point of the attribute itself
        std::vector<double> m_mtext;            // insertion point of the MTEXT
    };
    const std::vector<Variant> variants{
        {"untouched", nullptr, {-5.0, 4.0, -5.0, 4.0}, {5.0, 4.0}},
        {"text", [](RS_MText& text) { text.setText("ABD"); }, {-5.0, 4.0, -5.0, 4.0}, {5.0, 4.0}},
        {"moved", [](RS_MText& text) { text.move(RS_Vector(2.0, 1.0)); }, {-7.0, 5.0, -7.0, 5.0}, {7.0, 5.0}},
    };
    for (const Variant& variant : variants) {
        for (const char* format : {"dxf2018", "dxf2007"}) {
            const Target to = target(format);
            CAPTURE(variant.m_name, to.m_name);
            EditableDrawing drawing;
            load(bench, fixture, drawing, std::string(variant.m_name) + "_" + to.m_name + "_source");
            const RS_MText* shown = only<RS_MText>(drawing, RS2::EntityMText);
            REQUIRE(shown->getInsertionPoint().distanceTo(RS_Vector(5.0, 4.0)) < 1e-12);
            if (variant.m_edit) {
                shown = drawing.edit(shown, variant.m_edit);
            }
            const auto out = save(bench, drawing, variant.m_name, to);
            checkWrittenDxf(out);
            const DxfRecord record = onlyRecord(out, "ATTRIB");
            const std::vector<double> place = ownPlace(record);
            CAPTURE(place);
            CHECK(sameNumbers(variant.m_own, place));
            CHECK(record.own(74) == std::vector<std::string>{"3"});
            CHECK(lc::test::sameValues({"-1"}, record.own(230)));

            EditableDrawing reopened;
            REQUIRE(lc::test::importDrawing(reopened.m_graphic, out, to.m_format));
            if (!to.m_embedsMText) {
                // A text of one line is shown at the point of its file.
                const RS_Text* text = only<RS_Text>(reopened, RS2::EntityText);
                CHECK(text->getInsertionPoint().distanceTo(RS_Vector(variant.m_own[2], variant.m_own[3])) < 1e-9);
                CHECK(text->getVAlign() == RS_TextData::VATop);
                continue;
            }
            CHECK(sameNumbers(variant.m_mtext, embeddedNumbers(record, {10, 20})));
            checkSameMText(*shown, *only<RS_MText>(reopened, RS2::EntityMText));
            // Opened and saved again with nothing changed: the same points.
            const auto again = save(bench, reopened, std::string(variant.m_name) + "_reopened", to);
            CHECK(sameNumbers(variant.m_own, ownPlace(onlyRecord(again, "ATTRIB"))));
        }
    }

    // A record whose attribute has the points of its MTEXT as they are, in
    // the coordinate system of the drawing, as a version of LibreCAD wrote
    // it after an edit. Its first point belongs to an alignment point that
    // is not written again.
    AttributeDxf mirrored = fixture;
    mirrored.m_firstPoint = "10\n5\n20\n4\n30\n0\n";
    mirrored.m_alignmentPoint = "11\n5\n21\n4\n31\n0\n";
    mirrored.m_verticalJustification = "3";
    mirrored.m_typeGroups = "71\n2\n72\n0\n";
    EditableDrawing drawing;
    load(bench, mirrored, drawing, "mirrored_source");
    REQUIRE(only<RS_MText>(drawing, RS2::EntityMText)->getInsertionPoint().distanceTo(RS_Vector(5.0, 4.0)) < 1e-12);
    const DxfRecord record = onlyRecord(save(bench, drawing, "mirrored", target("dxf2018")), "ATTRIB");
    CHECK(sameNumbers({-5.0, 4.0, -5.0, 4.0}, ownPlace(record)));
    CHECK(sameNumbers({5.0, 4.0}, embeddedNumbers(record, {10, 20})));
}

// A text with no letter to draw has no size. Next to its MTEXT the value of
// an attribute is empty as a rule, and the first point of a right-aligned or
// a centred empty value must not be a width away that is no width: it is the
// alignment point, lowered or raised as for a capital letter.
TEST_CASE("the first point of a multi-line attribute with nothing to draw stays at its alignment point",
          "[attribute-export][attrib][filter][dxf][attrib-geometry]") {
    const Bench bench("attr_outer_blank");
    struct Kind {
        const char* m_name;
        const char* m_attachment;       // group 71 of the MTEXT
        const char* m_horizontal;       // group 72 of the attribute itself
        const char* m_vertical;         // and its group 74
        double m_below;                 // the baseline of a capital letter below the alignment point
    };
    const std::vector<Kind> kinds{{"top_right", "3", "2", "3", 9.0}, {"middle_right", "6", "2", "2", 4.5},
                                  {"bottom_centre", "8", "1", "1", -3.0}};
    for (const Kind& kind : kinds) {
        AttributeDxf fixture = offsetFirstPointDxf();
        fixture.m_attachment = kind.m_attachment;
        for (const bool emptied : {false, true}) {
            for (const char* format : {"dxf2018", "dxf2007"}) {
                const Target to = target(format);
                const std::string name = std::string(kind.m_name) + (emptied ? "_emptied" : "_untouched");
                CAPTURE(name, to.m_name);
                EditableDrawing drawing;
                load(bench, fixture, drawing, name + "_" + to.m_name + "_source");
                if (emptied) {
                    drawing.edit(only<RS_MText>(drawing, RS2::EntityMText), [](RS_MText& text) { text.setText(""); });
                }
                const DxfRecord record = onlyRecord(save(bench, drawing, name, to), "ATTRIB");
                const std::vector<double> place = ownPlace(record);
                CAPTURE(place);
                CHECK(record.own(74) == std::vector<std::string>{kind.m_vertical});
                CHECK(record.own(72) == ownGroup72(kind.m_horizontal, to));
                CHECK(sameNumbers({1.0, 4.0}, {place[2], place[3]}));
                if (emptied || to.m_embedsMText) {
                    // The value is empty.
                    CHECK(sameNumbers({1.0, 4.0 - kind.m_below}, {place[0], place[1]}));
                }
                else {
                    // The value is the text, which has a width.
                    CHECK(place[0] < 1.0);
                    CHECK(place[0] > 1.0 - 9.0 * 3.0);
                    CHECK(std::abs(place[1] - (4.0 - kind.m_below)) < 9.0);
                }
            }
        }
    }
}

// A multi-line text takes a "\A0;", "\A1;" or "\A2;" at the start of its
// content for its vertical alignment: the drawing has the content without it
// and the alignment it names, whatever the attachment point (group 71) of the
// file says. An untouched attribute is saved with the content and the
// attachment point it was read with. Once one of the two is changed, a
// content with such a start cannot go out next to another vertical alignment:
// the file would open with the alignment of the start again.
TEST_CASE("the content and the attachment point of a multi-line attribute are saved as a pair",
          "[attribute-export][attrib][filter][dxf][dwg][attrib-geometry]") {
    const Bench bench("attr_alignment_start");
    struct Variant {
        const char* m_name;
        std::function<void(RS_MText&)> m_edit;      // none: untouched
        const char* m_attachment;                   // group 71 of the MTEXT in a DXF
        const char* m_content;                      // its group 1
    };

    // Top left by its attachment point, the vertical middle by its content.
    SECTION("a content that starts with an alignment") {
        AttributeDxf fixture;
        fixture.m_text = R"(\A1;Line1\PLine2)";
        const std::vector<Variant> variants{
            {"untouched", nullptr, "1", R"(\A1;Line1\PLine2)"},
            {"text", [](RS_MText& text) { text.setText(QStringLiteral("Line1\nChanged")); }, "4",
             R"(Line1\PChanged)"},
            {"bottom", [](RS_MText& text) { text.setVAlign(RS_MTextData::VABottom); }, "7", R"(Line1\PLine2)"},
            // The vertical alignment is still the one the content starts with.
            {"centre", [](RS_MText& text) { text.setHAlign(RS_MTextData::HACenter); }, "5", R"(\A1;Line1\PLine2)"},
        };
        for (const Variant& variant : variants) {
            for (const char* format : {"dxf2018", "dwg2018"}) {
                const Target to = target(format);
                CAPTURE(variant.m_name, to.m_name);
                EditableDrawing drawing;
                load(bench, fixture, drawing, std::string(variant.m_name) + "_" + to.m_name + "_source");
                const RS_MText* shown = only<RS_MText>(drawing, RS2::EntityMText);
                REQUIRE(shown->getText().toStdString() == "Line1\nLine2");
                REQUIRE(shown->getVAlign() == RS_MTextData::VAMiddle);
                if (variant.m_edit) {
                    shown = drawing.edit(shown, variant.m_edit);
                }
                const auto out = save(bench, drawing, variant.m_name, to);
                if (to.m_dxf) {
                    checkWrittenDxf(out);
                    const DxfRecord record = onlyRecord(out, "ATTRIB");
                    CHECK(record.embedded(71) == std::vector<std::string>{variant.m_attachment});
                    CHECK(record.embedded(1) == std::vector<std::string>{variant.m_content});
                }
                // The file opens as what the drawing shows, where it shows it.
                EditableDrawing reopened;
                REQUIRE(lc::test::importDrawing(reopened.m_graphic, out, to.m_format));
                checkSameMText(*shown, *only<RS_MText>(reopened, RS2::EntityMText));
            }
        }
    }

    // The content of the file has codes that the text of the drawing does
    // not keep, a font here. A change of the alignment alone does not cost them.
    SECTION("a content without one keeps its codes when the alignment changes") {
        const std::vector<Variant> variants{
            {"bottom", [](RS_MText& text) { text.setVAlign(RS_MTextData::VABottom); }, "7", R"({\fArial;ABC}\PDEF)"},
            {"right", [](RS_MText& text) { text.setHAlign(RS_MTextData::HARight); }, "3", R"({\fArial;ABC}\PDEF)"},
        };
        for (const Variant& variant : variants) {
            for (const char* format : {"dxf2018", "dwg2018"}) {
                const Target to = target(format);
                CAPTURE(variant.m_name, to.m_name);
                EditableDrawing drawing;
                load(bench, AttributeDxf::rich(), drawing, std::string(variant.m_name) + "_" + to.m_name + "_source");
                const RS_MText* shown = drawing.edit(only<RS_MText>(drawing, RS2::EntityMText), variant.m_edit);
                const auto out = save(bench, drawing, variant.m_name, to);
                EditableDrawing reopened;
                REQUIRE(lc::test::importDrawing(reopened.m_graphic, out, to.m_format));
                checkSameMText(*shown, *only<RS_MText>(reopened, RS2::EntityMText));
                // What a DWG holds is read as the DXF that the drawing it
                // opens as is saved to.
                const auto dxf = to.m_dxf ? out : save(bench, reopened, std::string(variant.m_name) + "_" + to.m_name,
                                                       target("dxf2018"));
                const DxfRecord record = onlyRecord(dxf, "ATTRIB");
                CHECK(record.embedded(71) == std::vector<std::string>{variant.m_attachment});
                CHECK(record.embedded(1) == std::vector<std::string>{variant.m_content});
            }
        }
    }
}

// The note belongs to the attribute: the application data of a record is
// its own, and the MTEXT inside the attribute of a DWG has none.
TEST_CASE("a right-to-left attribute is noted once in its record",
          "[attribute-export][attrib][filter][dxf][dwg][attrib-geometry]") {
    const Bench bench("attr_right_to_left");
    AttributeDxf fixture;
    fixture.m_mtextTail = "1001\nLibreCad\n1071\n2\n";

    SECTION("three saves to DXF 2018") {
        const Target to = target("dxf2018");
        auto drawing = std::make_unique<EditableDrawing>();
        load(bench, fixture, *drawing);
        for (int round = 1; round <= 3; ++round) {
            CAPTURE(round);
            CHECK(only<RS_MText>(*drawing, RS2::EntityMText)->getDrawingDirection() == RS_MTextData::RightToLeft);
            const auto out = save(bench, *drawing, "round" + std::to_string(round), to);
            checkWrittenDxf(out);
            CHECK(rightToLeftNotes(onlyRecord(out, "ATTRIB")) == 1);
            auto reopened = std::make_unique<EditableDrawing>();
            REQUIRE(lc::test::importDrawing(reopened->m_graphic, out, to.m_format));
            drawing = std::move(reopened);
        }
        CHECK(only<RS_MText>(*drawing, RS2::EntityMText)->getDrawingDirection() == RS_MTextData::RightToLeft);
    }

    SECTION("every format") {
        for (const Target& to : kTargets) {
            CAPTURE(to.m_name);
            EditableDrawing drawing;
            load(bench, fixture, drawing, std::string(to.m_name) + "_source");
            const auto out = save(bench, drawing, "untouched", to);
            if (to.m_dxf) {
                checkWrittenDxf(out);
                CHECK(rightToLeftNotes(onlyRecord(out, "ATTRIB")) == 1);
            }
            else {
                DwgContent content;
                readDwg(out, content);
                CHECK(rightToLeftNotes(onlyDwgAttribute(content)) == 1);
                // The note is data of the file and has its application
                // there; what the text keeps of its attribute has none.
                CHECK(handleNamed(content.m_applications, "LibreCad") != 0);
                checkNoPrivateApplication(content);
            }
            if (!to.m_embedsMText) {
                continue;   // a text of one line has no direction
            }
            EditableDrawing reopened;
            REQUIRE(lc::test::importDrawing(reopened.m_graphic, out, to.m_format));
            CHECK(only<RS_MText>(reopened, RS2::EntityMText)->getDrawingDirection() == RS_MTextData::RightToLeft);
            const auto again = save(bench, reopened, std::string("again_") + to.m_name, target("dxf2018"));
            CHECK(rightToLeftNotes(onlyRecord(again, "ATTRIB")) == 1);
        }
    }

    // The note says how the text is laid out: 2 by the rules of Unicode, 1
    // the way older versions of LibreCAD did, which a drawing of theirs keeps.
    SECTION("the note of an old drawing stays the old one") {
        AttributeDxf old;
        old.m_mtextTail = "1001\nLibreCad\n1071\n1\n";
        const auto noted = [](const DxfRecord& record) {
            std::vector<std::string> values;
            for (const lc::test::DxfGroup& group : record.m_groups) {
                if (group.m_code == 1071) {
                    values.push_back(group.m_value);
                }
            }
            return values;
        };
        for (const char* format : {"dxf2018", "dxf2007", "dwg2018"}) {
            const Target to = target(format);
            CAPTURE(to.m_name);
            EditableDrawing drawing;
            load(bench, old, drawing, std::string(to.m_name) + "_source");
            const RS_MText* shown = only<RS_MText>(drawing, RS2::EntityMText);
            REQUIRE(shown->getDrawingDirection() == RS_MTextData::RightToLeft);
            REQUIRE(shown->getData().legacyRtlLayout);
            const auto out = save(bench, drawing, "old", to);
            if (to.m_dxf) {
                CHECK(noted(onlyRecord(out, "ATTRIB")) == std::vector<std::string>{"1"});
            }
            if (to.m_embedsMText) {
                EditableDrawing reopened;
                REQUIRE(lc::test::importDrawing(reopened.m_graphic, out, to.m_format));
                CHECK(only<RS_MText>(reopened, RS2::EntityMText)->getData().legacyRtlLayout);
            }
        }
    }

    SECTION("no note once the text is left-to-right") {
        for (const Target& to : kTargets) {
            CAPTURE(to.m_name);
            EditableDrawing drawing;
            load(bench, fixture, drawing, std::string(to.m_name) + "_source");
            drawing.edit(only<RS_MText>(drawing, RS2::EntityMText), [](RS_MText& text) {
                text.setDrawingDirection(RS_MTextData::LeftToRight);
            });
            const auto out = save(bench, drawing, "left_to_right", to);
            if (to.m_dxf) {
                checkWrittenDxf(out);
                const DxfRecord record = onlyRecord(out, "ATTRIB");
                CHECK(rightToLeftNotes(record) == 0);
                if (to.m_embedsMText) {
                    CHECK(record.embedded(72) == std::vector<std::string>{"1"});
                }
            }
            else {
                DwgContent content;
                readDwg(out, content);
                CHECK(rightToLeftNotes(onlyDwgAttribute(content)) == 0);
            }
        }
    }
}

// The direction and the line spacing of an untouched attribute are those of
// its file, also where the drawing has them differently: a right-to-left
// text has its direction in the note and not in group 72, and the line
// spacing of a file that an old LibreCAD wrote is read a little smaller.
TEST_CASE("an untouched multi-line attribute keeps the direction and the line spacing of its file",
          "[attribute-export][attrib][filter][dxf][attrib-geometry]") {
    const Bench bench("attr_layout_kept");
    const Target to = target("dxf2018");

    SECTION("the direction next to the right-to-left note") {
        AttributeDxf fixture;   // direction 5: by the text style
        fixture.m_mtextTail = "1001\nLibreCad\n1071\n2\n";
        EditableDrawing drawing;
        load(bench, fixture, drawing);
        REQUIRE(only<RS_MText>(drawing, RS2::EntityMText)->getDrawingDirection() == RS_MTextData::RightToLeft);
        const auto out = save(bench, drawing, "untouched", to);
        CHECK(onlyRecord(out, "ATTRIB").embedded(72) == std::vector<std::string>{"5"});
    }

    SECTION("the line spacing of a file of an old LibreCAD") {
        AttributeDxf fixture;
        fixture.m_spacingFactor = "1.5";
        EditableDrawing drawing;
        // The comment at the start names the library that wrote the file; it
        // is looked at when the header of the file is read.
        loadDxf(bench, "999\ndxflib 2.5.0.0\n0\nSECTION\n2\nHEADER\n0\nENDSEC\n" + fixture.str(), drawing);
        const RS_MText* shown = only<RS_MText>(drawing, RS2::EntityMText);
        REQUIRE(std::abs(shown->getLineSpacingFactor() - 1.44) < 1e-9);
        const auto out = save(bench, drawing, "untouched", to);
        CHECK(lc::test::sameValues({"1.5"}, onlyRecord(out, "ATTRIB").embedded(44)));

        // Changed in the drawing, it is the spacing of the drawing.
        drawing.edit(shown, [](RS_MText& text) { text.setLineSpacingFactor(2.0); });
        const auto edited = save(bench, drawing, "edited", to);
        CHECK(lc::test::sameValues({"2"}, onlyRecord(edited, "ATTRIB").embedded(44)));
    }

    // Such a file has a bottom-aligned text by the lowest point of its last
    // line, and the drawing shows it lower by what hangs below the baseline
    // there. The filter that saves is not the one that read the file: it
    // cannot work that out again, and the text is not moved for it.
    SECTION("the place of a bottom-aligned text of a file of an old LibreCAD") {
        AttributeDxf fixture;
        fixture.m_attachment = "7";
        fixture.m_mtextPoint = "10\n1\n20\n14\n30\n0\n";
        fixture.m_text = "first\\Pgyp";
        EditableDrawing drawing;
        loadDxf(bench, "999\ndxflib 2.5.0.0\n0\nSECTION\n2\nHEADER\n0\nENDSEC\n" + fixture.str(), drawing);
        const RS_MText* shown = only<RS_MText>(drawing, RS2::EntityMText);
        REQUIRE(shown->getVAlign() == RS_MTextData::VABottom);
        const RS_Vector place = shown->getInsertionPoint();
        REQUIRE(std::abs(place.x - 1.0) < 1e-9);
        REQUIRE(place.y < 14.0 - 1e-3);
        const auto out = save(bench, drawing, "untouched", to);
        const DxfRecord record = onlyRecord(out, "ATTRIB");
        CHECK(sameNumbers({1.0, 14.0}, embeddedNumbers(record, {10, 20})));
        // Nor is it taken for edited, which would give it its measured size.
        CHECK(record.embedded(42).empty());
        CHECK(record.embedded(43).empty());

        // Moved in the drawing, it is where the drawing has it.
        drawing.edit(shown, [](RS_MText& text) { text.move(RS_Vector(5.0, 0.0)); });
        const auto moved = save(bench, drawing, "moved", to);
        CHECK(sameNumbers({place.x + 5.0, place.y}, embeddedNumbers(onlyRecord(moved, "ATTRIB"), {10, 20})));
    }
}

// Group 71 of an MTEXT is its attachment point, 72 its direction, 73 and 44
// its line spacing. A right-to-left text is written as left-to-right, with
// the note.
TEST_CASE("a standalone MTEXT is saved with the alignment and the direction and the spacing of the drawing",
          "[attribute-export][mtext][filter][dxf][dwg][attrib-geometry]") {
    const Bench bench("mtext_layout_groups");
    struct Row {
        const char* m_name;
        RS_MTextData::MTextDrawingDirection m_direction;
        const char* m_group72;
        int m_notes;
    };
    const std::vector<Row> rows{
        {"by_style", RS_MTextData::ByStyle, "5", 0},
        {"top_to_bottom", RS_MTextData::TopToBottom, "3", 0},
        {"left_to_right", RS_MTextData::LeftToRight, "1", 0},
        {"right_to_left", RS_MTextData::RightToLeft, "1", 1},
    };
    for (const Row& row : rows) {
        for (const Target& to : kTargets) {
            CAPTURE(row.m_name, to.m_name);
            EditableDrawing drawing;
            // Top left, by style, exact spacing of 1.
            loadDxf(bench, standaloneMTextDxf("2"), drawing, std::string(row.m_name) + "_" + to.m_name + "_source");
            const RS_MText* shown = drawing.edit(only<RS_MText>(drawing, RS2::EntityMText), [&](RS_MText& text) {
                text.setVAlign(RS_MTextData::VAMiddle);
                text.setHAlign(RS_MTextData::HARight);
                text.setDrawingDirection(row.m_direction);
                text.setLineSpacingFactor(1.25);
            });
            const auto out = save(bench, drawing, row.m_name, to);
            if (to.m_dxf) {
                checkWrittenDxf(out);
                const DxfRecord record = onlyRecord(out, "MTEXT");
                CHECK(record.own(71) == std::vector<std::string>{"6"});
                CHECK(record.own(72) == std::vector<std::string>{row.m_group72});
                CHECK(record.own(73) == std::vector<std::string>{"2"});
                CHECK(sameNumbers({1.25}, ownNumbers(record, {44})));
                CHECK(rightToLeftNotes(record) == row.m_notes);
            }
            EditableDrawing reopened;
            REQUIRE(lc::test::importDrawing(reopened.m_graphic, out, to.m_format));
            const RS_MText* again = only<RS_MText>(reopened, RS2::EntityMText);
            CHECK(again->getVAlign() == shown->getVAlign());
            CHECK(again->getHAlign() == shown->getHAlign());
            CHECK(again->getDrawingDirection() == shown->getDrawingDirection());
            CHECK(again->getLineSpacingStyle() == shown->getLineSpacingStyle());
            CHECK(again->getLineSpacingFactor() == Catch::Approx(1.25).margin(1e-9));
            CHECK(again->getInsertionPoint().distanceTo(shown->getInsertionPoint()) < 1e-9);
        }
    }
}

// The tools change one thing of a text at a time: the property sheet a
// number or a choice, Move its place, Rotate its angle. The text editor can
// change several in one step. Each time a changed clone takes the place of
// the text. The cases below make such a change and save the drawing to every
// format.

namespace {

// One change, and where a written DXF has it: the codes of the groups of the
// attribute itself that follow the change, the codes of the groups of its
// MTEXT that do, and what the MTEXT holds of the property afterwards.
struct OneEdit {
    const char* m_name;
    std::function<void(RS_MText&)> m_change;
    std::vector<int> m_own;
    std::vector<int> m_mtext;
    std::vector<std::pair<int, std::vector<std::string>>> m_written;
    const char* m_plain = nullptr;      // of a change of the content: the new text on one line
};

// The groups of the attribute itself in a record, or those of its MTEXT, as
// "code=value" in the order of the file, without the groups of some codes.
std::vector<std::string> groupsWithout(const DxfRecord& record, const bool ofMText, const std::vector<int>& codes) {
    const std::size_t at = record.embeddedAt();
    const std::size_t first = ofMText ? at + 1 : 1;
    const std::size_t last = ofMText ? record.m_groups.size() : at;
    std::vector<std::string> groups;
    for (std::size_t i = first; i < last; ++i) {
        const lc::test::DxfGroup& group = record.m_groups[i];
        if (std::find(codes.begin(), codes.end(), group.m_code) == codes.end()) {
            groups.push_back(std::to_string(group.m_code) + "=" + group.m_value);
        }
    }
    return groups;
}

} // namespace

TEST_CASE("an edit of one property of a multi-line attribute is saved",
          "[attribute-export][attrib][filter][dxf][dwg][attrib-edit]") {
    const Bench bench("attr_edit_one");
    const QString typed = QStringLiteral("    ABC\n\tDEF \u03a9");
    // In an MTEXT a line break is "\P" and a tab "^I"; on one line each of
    // them is a blank.
    const std::string typedInMText = std::string("    ABC\\P^IDEF ") + kCapitalOmega;
    const std::string typedOnOneLine = std::string("    ABC  DEF ") + kCapitalOmega;
    const std::vector<OneEdit> edits{
        {"text", [&](RS_MText& text) { text.setText(typed); }, {1}, {1}, {{1, {typedInMText}}}, typedOnOneLine.c_str()},
        {"no_text", [](RS_MText& text) { text.setText(QString()); }, {1}, {1}, {{1, {""}}}, ""},
        {"width", [](RS_MText& text) { text.setWidth(120.0); }, {}, {41}, {{41, {"120"}}}},
        // The attribute itself is justified as its MTEXT is attached, and
        // its first point is where the baseline of its value then starts.
        {"right", [](RS_MText& text) { text.setHAlign(RS_MTextData::HARight); }, {10, 72}, {71}, {{71, {"3"}}}},
        {"middle", [](RS_MText& text) { text.setVAlign(RS_MTextData::VAMiddle); }, {20, 74}, {71}, {{71, {"4"}}}},
        {"spacing", [](RS_MText& text) { text.setLineSpacingFactor(1.25); }, {}, {44}, {{44, {"1.25"}}}},
        // The setter is an overload of the getter.
        {"exact", [](RS_MText& text) { text.getLineSpacingStyle(RS_MTextData::Exact); }, {}, {73}, {{73, {"2"}}}},
        // A right-to-left text is written as left-to-right, with the note.
        {"right_to_left", [](RS_MText& text) { text.setDrawingDirection(RS_MTextData::RightToLeft); }, {}, {72},
         {{72, {"1"}}}},
        {"place", [](RS_MText& text) { text.setInsertionPoint(RS_Vector(7.0, 8.0)); }, {10, 20, 11, 21}, {10, 20},
         {{10, {"7"}}, {20, {"8"}}}},
        // A rotated MTEXT has its direction as an x-axis, not as an angle.
        {"angle", [](RS_MText& text) { text.setAngle(0.5); }, {10, 20, 50}, {11, 21, 31},
         {{11, {QString::number(std::cos(0.5), 'g', 17).toStdString()}},
          {21, {QString::number(std::sin(0.5), 'g', 17).toStdString()}},
          {50, {}}}},
        {"height", [](RS_MText& text) { text.setHeight(12.0); }, {20, 40}, {40}, {{40, {"12"}}}},
        {"style", [](RS_MText& text) { text.setStyle("unicode"); }, {7}, {7}, {{7, {"unicode"}}}},
        // The text editor changes several things in one step. Middle right
        // is attachment point 6.
        {"all_at_once",
         [&](RS_MText& text) {
             text.setText(typed);
             text.setWidth(120.0);
             text.setHAlign(RS_MTextData::HARight);
             text.setVAlign(RS_MTextData::VAMiddle);
             text.setLineSpacingFactor(1.25);
             text.getLineSpacingStyle(RS_MTextData::Exact);
             text.setDrawingDirection(RS_MTextData::RightToLeft);
             text.setInsertionPoint(RS_Vector(7.0, 8.0));
             text.setAngle(0.5);
             text.setHeight(12.0);
             text.setStyle("unicode");
         },
         {1, 7, 10, 20, 11, 21, 40, 50, 72, 74},
         {1, 7, 10, 20, 11, 21, 31, 40, 41, 44, 71, 72, 73},
         {{1, {typedInMText}},
          {41, {"120"}},
          {71, {"6"}},
          {44, {"1.25"}},
          {73, {"2"}},
          {72, {"1"}},
          {10, {"7"}},
          {20, {"8"}},
          {11, {QString::number(std::cos(0.5), 'g', 17).toStdString()}},
          {21, {QString::number(std::sin(0.5), 'g', 17).toStdString()}},
          {50, {}},
          {40, {"12"}},
          {7, {"unicode"}}},
         typedOnOneLine.c_str()},
    };
    // Not a matter of any change: the handles, which a new text style
    // shifts; the right-to-left note, which is counted instead; and the
    // size of the text as it was measured, which only a changed MTEXT gets.
    const auto besides = [](std::vector<int> codes, const std::vector<int>& more) {
        codes.insert(codes.end(), more.begin(), more.end());
        return codes;
    };
    const std::vector<int> neverOwn{5, 330, 1001, 1071};
    const std::vector<int> neverMText{42, 43, 1001, 1071};

    struct Preset {
        const char* m_name;
        AttributeDxf m_fixture;
        const char* m_plain;    // the text of the fixture on one line
    };
    const std::vector<Preset> presets{{"autocad", AttributeDxf{}, "my text second line"},
                                      {"rich", AttributeDxf::rich(), "ABC DEF"}};

    for (const Preset& preset : presets) {
        // The file that every drawing below is opened from, and its
        // attribute as each DXF format has it with nothing changed.
        EditableDrawing untouched;
        const auto source = load(bench, preset.m_fixture, untouched, std::string(preset.m_name) + "_source");
        std::map<std::string, DxfRecord> asRead;
        for (const Target& to : kTargets) {
            if (to.m_dxf) {
                asRead[to.m_name] =
                    onlyRecord(save(bench, untouched, std::string(preset.m_name) + "_untouched", to), "ATTRIB");
            }
        }

        for (const OneEdit& edit : edits) {
            for (const Target& to : kTargets) {
                const std::string name = std::string(preset.m_name) + "_" + edit.m_name;
                CAPTURE(name, to.m_name);
                EditableDrawing drawing;
                REQUIRE(lc::test::importDrawing(drawing.m_graphic, source, RS2::FormatDXFRW2018));
                const RS_MText* model = drawing.edit(only<RS_MText>(drawing, RS2::EntityMText), edit.m_change);

                // A row that fails is not to hide the rows after it.
                const auto out = bench.m_out.file(name + "_" + to.m_name + to.m_suffix);
                const bool saved = lc::test::exportDrawing(drawing.m_graphic, out, to.m_format);
                CHECK(saved);
                if (!saved) {
                    continue;
                }

                std::vector<double> place;  // of the attribute itself in a DXF: 10, 20, 11, 21
                if (to.m_dxf) {
                    // The attribute is in the file once, as an attribute.
                    checkWrittenDxf(out);
                    CHECK(lc::test::dxfRecords(out, "TEXT").empty());
                    CHECK(lc::test::dxfRecords(out, "MTEXT").empty());
                    const std::vector<DxfRecord> records = lc::test::dxfRecords(out, "ATTRIB");
                    CHECK(records.size() == 1);
                    if (records.size() != 1) {
                        continue;
                    }
                    // The change is in the groups it belongs to and in no
                    // other group of the record.
                    const DxfRecord& record = records.front();
                    const DxfRecord& before = asRead.at(to.m_name);
                    const std::vector<int> own = besides(edit.m_own, neverOwn);
                    const std::vector<int> ofMText = besides(edit.m_mtext, neverMText);
                    CHECK(groupsWithout(record, false, own) == groupsWithout(before, false, own));
                    CHECK(groupsWithout(record, true, ofMText) == groupsWithout(before, true, ofMText));
                    CHECK(rightToLeftNotes(record)
                          == (model->getDrawingDirection() == RS_MTextData::RightToLeft ? 1 : 0));
                    // The attribute itself goes with its MTEXT, also where
                    // the MTEXT is written and no program shows the
                    // attribute's own text: as high, as turned, in the same
                    // style, with the insertion point as its alignment
                    // point and justified as the MTEXT is attached.
                    place = ownPlace(record);
                    const RS_Vector at = model->getInsertionPoint();
                    CHECK(sameNumbers({at.x, at.y}, {place[2], place[3]}));
                    CHECK(sameNumbers({model->getHeight()}, ownNumbers(record, {40})));
                    CHECK(sameNumbers({model->getAngle() * 180.0 / M_PI}, ownNumbers(record, {50})));
                    // Left is no group 72 at all.
                    const std::string horizontal = model->getHAlign() == RS_MTextData::HALeft
                                                       ? std::string()
                                                       : std::to_string(static_cast<int>(model->getHAlign()));
                    CHECK(record.own(72) == ownGroup72(horizontal.c_str(), to));
                    CHECK(record.own(74)
                          == std::vector<std::string>{std::to_string(3 - static_cast<int>(model->getVAlign()))});
                    const std::vector<std::string> ownStyle = record.own(7);
                    CAPTURE(ownStyle, model->getStyle().toStdString());
                    CHECK(ownStyle.size() == 1);
                    CHECK((ownStyle.size() == 1
                           && model->getStyle().compare(QString::fromStdString(ownStyle.front()), Qt::CaseInsensitive)
                                  == 0));
                    if (to.m_embedsMText) {
                        checkEmbeddedMTextPlace(record);
                        for (const auto& [code, values] : edit.m_written) {
                            CAPTURE(code, record.embedded(code));
                            CHECK(lc::test::sameValues(values, record.embedded(code)));
                        }
                    }
                    // An empty value, which AutoCAD leaves next to the
                    // MTEXT, has no width: its first point is the alignment
                    // point, lowered along the turned text as for a capital
                    // letter. No text here is attached by its bottom.
                    if (to.m_embedsMText && record.own(1) == std::vector<std::string>{""}) {
                        REQUIRE(model->getVAlign() != RS_MTextData::VABottom);
                        const double below =
                            model->getHeight() * (model->getVAlign() == RS_MTextData::VATop ? 1.0 : 0.5);
                        CAPTURE(place);
                        CHECK(sameNumbers({at.x + below * std::sin(model->getAngle()),
                                           at.y - below * std::cos(model->getAngle())},
                                          {place[0], place[1]}));
                    }
                }

                EditableDrawing reopened;
                const bool opened = lc::test::importDrawing(reopened.m_graphic, out, to.m_format);
                CHECK(opened);
                if (!opened) {
                    continue;
                }
                CHECK(reopened.live<RS_Insert>(RS2::EntityInsert).size() == 1);
                const std::vector<const RS_MText*> multiLine = reopened.live<RS_MText>(RS2::EntityMText);
                const std::vector<const RS_Text*> oneLine = reopened.live<RS_Text>(RS2::EntityText);
                if (to.m_embedsMText) {
                    // The file opens as what the drawing shows, where it
                    // shows it.
                    CHECK(oneLine.empty());
                    CHECK(multiLine.size() == 1);
                    if (multiLine.size() == 1) {
                        checkSameMText(*model, *multiLine.front());
                    }
                    continue;
                }
                // A format without the MTEXT shows the attribute as a text
                // of one line: the text without line breaks and codes, as
                // high, as turned and in the style of the multi-line text,
                // and justified at its insertion point as it is attached.
                CHECK(multiLine.empty());
                CHECK(oneLine.size() == 1);
                if (oneLine.size() != 1) {
                    continue;
                }
                const RS_Text* text = oneLine.front();
                CHECK(text->getText().toStdString() == (edit.m_plain != nullptr ? edit.m_plain : preset.m_plain));
                CHECK(text->getHeight() == Catch::Approx(model->getHeight()).margin(1e-9));
                CHECK(text->getAngle() == Catch::Approx(model->getAngle()).margin(1e-9));
                CAPTURE(text->getStyle().toStdString(), model->getStyle().toStdString());
                CHECK(text->getStyle().compare(model->getStyle(), Qt::CaseInsensitive) == 0);
                CHECK(text->getInsertionPoint().distanceTo(model->getInsertionPoint()) < 1e-9);
                // The two kinds of text count the vertical alignments in
                // opposite orders: top is 0 of three there and 3 of four here.
                CHECK(static_cast<int>(text->getHAlign()) == static_cast<int>(model->getHAlign()));
                CHECK(static_cast<int>(text->getVAlign()) == 3 - static_cast<int>(model->getVAlign()));
                // The first point of the attribute in a DXF is the point
                // that the text works out for itself as the start of its
                // baseline. A text with nothing to draw works none out.
                if (to.m_dxf && !text->getText().isEmpty()) {
                    CAPTURE(place, text->getSecondPoint().x, text->getSecondPoint().y);
                    CHECK(text->getSecondPoint().distanceTo(RS_Vector(place[0], place[1])) < 1e-6);
                }
            }
        }
    }
}

// An undo puts the text of the file back in the place of the clone, which
// stays in the drawing, flagged deleted. The save is then the save of the
// untouched drawing: the record of the file, with the codes of its content
// that the text of the drawing does not keep.
TEST_CASE("an undone edit of a multi-line attribute leaves its record as the file had it",
          "[attribute-export][attrib][filter][dxf][dwg][attrib-edit]") {
    const Bench bench("attr_edit_undone");
    struct Preset {
        const char* m_name;
        AttributeDxf m_fixture;
    };
    const std::vector<Preset> presets{{"autocad", AttributeDxf{}}, {"rich", AttributeDxf::rich()}};
    // But for the handles: the clone took none, and the test is not about
    // which numbers the records get.
    const std::vector<int> handles{5, 330};
    for (const Preset& preset : presets) {
        const std::string name = preset.m_name;
        EditableDrawing untouched;
        const auto source = load(bench, preset.m_fixture, untouched, name + "_source");
        EditableDrawing drawing;
        REQUIRE(lc::test::importDrawing(drawing.m_graphic, source, RS2::FormatDXFRW2018));
        const RS_MText* imported = only<RS_MText>(drawing, RS2::EntityMText);
        drawing.edit(imported, [](RS_MText& text) {
            text.setText(QStringLiteral("XYZ"));
            text.setHeight(12.0);
        });
        REQUIRE(only<RS_MText>(drawing, RS2::EntityMText) != imported);
        REQUIRE(drawing.m_graphic.undo());
        REQUIRE(only<RS_MText>(drawing, RS2::EntityMText) == imported);

        for (const Target& to : kTargets) {
            CAPTURE(name, to.m_name);
            const auto out = save(bench, drawing, name + "_undone", to);
            if (to.m_dxf) {
                checkWrittenDxf(out);
                CHECK(lc::test::dxfRecords(out, "TEXT").empty());
                CHECK(lc::test::dxfRecords(out, "MTEXT").empty());
                const DxfRecord record = onlyRecord(out, "ATTRIB");
                const DxfRecord asRead = onlyRecord(save(bench, untouched, name + "_untouched", to), "ATTRIB");
                CHECK(groupsWithout(record, false, handles) == groupsWithout(asRead, false, handles));
                CHECK(groupsWithout(record, true, {}) == groupsWithout(asRead, true, {}));
            }
            EditableDrawing reopened;
            REQUIRE(lc::test::importDrawing(reopened.m_graphic, out, to.m_format));
            CHECK(reopened.live<RS_Insert>(RS2::EntityInsert).size() == 1);
            const RS_Entity* shown = attributeText(reopened);
            REQUIRE((shown->rtti() == RS2::EntityMText) == to.m_embedsMText);
            if (to.m_embedsMText) {
                checkSameMText(*imported, *static_cast<const RS_MText*>(shown));
            }
        }
        // The file itself, for the preset whose content has a code.
        if (name == "rich") {
            const DxfRecord record = onlyRecord(bench.m_out.file("rich_undone_dxf2018.dxf"), "ATTRIB");
            CHECK(record.embedded(1) == std::vector<std::string>{"{\\fArial;ABC}\\PDEF"});
        }
    }
}

// A file that LibreCAD saved has the attribute as LibreCAD writes it, and
// its records have handles. Opened and changed again, the attribute is in
// the next file once, with the second change.
TEST_CASE("an attribute edited again after a save and a reopen is saved with the second edit",
          "[attribute-export][attrib][filter][dxf][dwg][attrib-edit]") {
    const Bench bench("attr_edit_again");
    struct First {
        const char* m_name;
        QString m_text;
    };
    const std::vector<First> firsts{{"text", QStringLiteral("    ABC\n\tDEF \u03a9")}, {"no_text", QString()}};
    for (const First& first : firsts) {
        for (const Target& to : kTargets) {
            const std::string name = first.m_name;
            CAPTURE(name, to.m_name);
            EditableDrawing drawing;
            load(bench, AttributeDxf{}, drawing, name + "_" + to.m_name + "_source");
            drawing.edit(only<RS_MText>(drawing, RS2::EntityMText), [&](RS_MText& text) {
                text.setText(first.m_text);
                text.setDrawingDirection(RS_MTextData::RightToLeft);
            });
            const auto saved = save(bench, drawing, name + "_first", to);

            // The file of the first save, opened: a multi-line text where
            // the format has the MTEXT, a text of one line where it has not.
            EditableDrawing second;
            REQUIRE(lc::test::importDrawing(second.m_graphic, saved, to.m_format));
            const RS_Entity* shown = attributeText(second);
            REQUIRE((shown->rtti() == RS2::EntityMText) == to.m_embedsMText);
            if (to.m_embedsMText) {
                const auto* text = static_cast<const RS_MText*>(shown);
                REQUIRE(text->getDrawingDirection() == RS_MTextData::RightToLeft);
                second.edit(text, [](RS_MText& changed) {
                    changed.setText(QStringLiteral("new text"));
                    changed.setDrawingDirection(RS_MTextData::LeftToRight);
                });
            }
            else {
                second.edit(static_cast<const RS_Text*>(shown),
                            [](RS_Text& changed) { changed.setText(QStringLiteral("new text")); });
            }
            const auto again = save(bench, second, name + "_second", to);

            if (to.m_dxf) {
                checkWrittenDxf(again);
                CHECK(lc::test::dxfRecords(again, "TEXT").empty());
                CHECK(lc::test::dxfRecords(again, "MTEXT").empty());
                const DxfRecord record = onlyRecord(again, "ATTRIB");
                if (to.m_embedsMText) {
                    CHECK(record.embedded(1) == std::vector<std::string>{"new text"});
                    CHECK(record.embedded(72) == std::vector<std::string>{"1"});
                    CHECK(rightToLeftNotes(record) == 0);
                }
                else {
                    CHECK(record.own(1) == std::vector<std::string>{"new text"});
                }
            }
            else if (to.m_embedsMText) {
                DwgContent content;
                readDwg(again, content);
                CHECK(rightToLeftNotes(onlyDwgAttribute(content)) == 0);
            }

            EditableDrawing reopened;
            REQUIRE(lc::test::importDrawing(reopened.m_graphic, again, to.m_format));
            CHECK(reopened.live<RS_Insert>(RS2::EntityInsert).size() == 1);
            const RS_Entity* last = attributeText(reopened);
            REQUIRE((last->rtti() == RS2::EntityMText) == to.m_embedsMText);
            if (to.m_embedsMText) {
                const auto* text = static_cast<const RS_MText*>(last);
                CHECK(text->getText().toStdString() == "new text");
                CHECK(text->getDrawingDirection() == RS_MTextData::LeftToRight);
            }
            else {
                CHECK(static_cast<const RS_Text*>(last)->getText().toStdString() == "new text");
            }
        }
    }
}

// The spacing of the lines of an MTEXT is a factor (group 44) and the way it
// is applied (73): 2 where a taller letter does not push the lines apart.
// The drawing has both, and a change of the one does not reset the other.
TEST_CASE("a change of the line-spacing factor alone keeps exact line spacing",
          "[attribute-export][attrib][filter][dxf][dwg][keep]") {
    const Bench bench("attr_edit_spacing");
    for (const char* format : {"dxf2018", "dwg2018"}) {
        const Target to = target(format);
        CAPTURE(to.m_name);
        EditableDrawing drawing;
        load(bench, AttributeDxf::rich(), drawing, std::string(to.m_name) + "_source");
        const RS_MText* imported = only<RS_MText>(drawing, RS2::EntityMText);
        REQUIRE(imported->getLineSpacingStyle() == RS_MTextData::Exact);
        REQUIRE(imported->getLineSpacingFactor() == 1.0);
        const RS_MText* model = drawing.edit(imported, [](RS_MText& text) { text.setLineSpacingFactor(1.25); });

        const auto out = save(bench, drawing, "factor", to);
        if (to.m_dxf) {
            checkWrittenDxf(out);
            const DxfRecord record = onlyRecord(out, "ATTRIB");
            CHECK(record.embedded(73) == std::vector<std::string>{"2"});
            CHECK(lc::test::sameValues({"1.25"}, record.embedded(44)));
            // Nor does it cost the content the codes that the drawing does
            // not keep.
            CHECK(record.embedded(1) == std::vector<std::string>{"{\\fArial;ABC}\\PDEF"});
        }
        else {
            DwgContent content;
            readDwg(out, content);
            const DRW_MText& mtext = onlyEmbeddedMText(content);
            CHECK(mtext.linespacingStyle == 2);
            CHECK(mtext.interlin == Catch::Approx(1.25).margin(1e-12));
        }

        EditableDrawing reopened;
        REQUIRE(lc::test::importDrawing(reopened.m_graphic, out, to.m_format));
        const RS_MText* again = only<RS_MText>(reopened, RS2::EntityMText);
        CHECK(again->getLineSpacingStyle() == RS_MTextData::Exact);
        checkSameMText(*model, *again);
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
        // AutoCAD leaves the attribute's own value empty next to the MTEXT.
        // Without the MTEXT the value is the text.
        if (to.m_embedsMText) {
            checkEmbeddedMTextPlace(records.front());
            CHECK(records.front().embedded(1) == std::vector<std::string>{text});
            CHECK(records.front().own(1) == std::vector<std::string>{""});
        }
        else {
            CHECK(records.front().embeddedMarker() == 0);
            CHECK(records.front().own(1) == std::vector<std::string>{text});
        }
    }
    // sample_AC1032.dwg is not opened here: the DWG layout of an attribute's
    // embedded MTEXT is not AutoCAD's yet, which is corrected separately.
}
