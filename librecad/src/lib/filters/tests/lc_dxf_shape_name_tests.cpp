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

// A DXF SHAPE names a shape of a shape file in group 2. A DWG holds the number
// of the shape instead; its name is in the .shx file, which a program that
// converts the DWG usually does not have. LibreDWG and ACadSharp write such a
// SHAPE with an empty name, and LibreCAD wrote it without group 2. The DXF
// reader refused the whole file over such a SHAPE, so a DWG with a shape,
// saved as DXF, could not be opened again. The reader now takes it, and the
// writer leaves it out and says so: a SHAPE without a name names no shape, and
// ODA drops it on reading.

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <string>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QTemporaryDir>

#include "drw_entities.h"
#include "lc_dwgadvancedmetadata.h"
#include "lc_filtertestsupport.h"
#include "lc_shxshapenames.h"
#include "rs_filterdxfrw.h"
#include "rs_block.h"
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

std::string tempPath(const std::string& name) {
    return (std::filesystem::temp_directory_path() / ("lc_dxf_shape_name_" + name))
        .string();
}

struct Format {
    RS2::FormatType m_type;
    const char* m_name;
};

const Format kDxfFormats[] = {
    {RS2::FormatDXFRW12, "dxf-r12"},       {RS2::FormatDXFRW14, "dxf-r14"},
    {RS2::FormatDXFRW2000, "dxf-2000"},    {RS2::FormatDXFRW2004, "dxf-2004"},
    {RS2::FormatDXFRW, "dxf-2007"},        {RS2::FormatDXFRW2010, "dxf-2010"},
    {RS2::FormatDXFRW2013, "dxf-2013"},    {RS2::FormatDXFRW2018, "dxf-2018"}};

std::size_t countLines(RS_Graphic& graphic) {
    std::size_t lines = 0;
    for (RS_Entity* e : graphic) {
        if (e != nullptr && e->rtti() == RS2::EntityLine)
            ++lines;
    }
    return lines;
}

QByteArray shapeFileBytes() {
    QByteArray bytes("AutoCAD-86 shapes 1.0\r\n\x1a");
    const auto word = [&bytes](unsigned value) {
        bytes.append(static_cast<char>(value & 0xff));
        bytes.append(static_cast<char>(value >> 8));
    };
    word(130); // First and last index, then directory size.
    word(130);
    word(1);
    word(130);
    word(10); // MY-SHAPE, NUL, bytecode terminator.
    bytes.append("MY-SHAPE\0\0", 10);
    bytes.append("EOF");
    return bytes;
}

void addShapeFileStyle(RS_Graphic &graphic, const std::string &filename,
                       std::uint32_t handle = 0x887) {
    DRW_Textstyle style;
    style.handle = handle;
    style.flags = 1;
    style.name.clear();
    style.font = filename;
    graphic.dwgAdvancedMetadata().addTextStyleName(style);
}

void addNumberedShape(RS_Graphic &graphic, std::uint32_t handle = 0x889,
                     std::uint32_t styleHandle = 0x887,
                     std::uint32_t ownerHandle = 0,
                     DRW::Space space = DRW::ModelSpace) {
    DRW_Shape shape;
    shape.handle = handle;
    shape.parentHandle = ownerHandle;
    shape.space = space;
    shape.m_shapeFileHandle = styleHandle;
    shape.m_shapeIndex = 130;
    shape.m_styleName = "NOT-THE-GLYPH-NAME";
    shape.m_insertionPoint = {15.25, -20.5, 1.0};
    shape.m_scale = 2.5;
    shape.m_rotation = 0.25;
    shape.m_oblique = 0.125;
    shape.m_widthFactor = 1.25;
    shape.m_thickness = 0.5;
    shape.color = 3;
    shape.ltypeScale = 2.0;
    shape.extData = {
        std::make_shared<DRW_Variant>(1001, std::string("SHAPE.TEST")),
        std::make_shared<DRW_Variant>(1000, std::string("shape payload"))};
    graphic.dwgAdvancedMetadata().addShape(shape);
}

// The number of SHAPE records in a written DXF.
std::size_t countShapeRecords(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    std::size_t shapes = 0;
    std::string code;
    std::string value;
    const auto trim = [](std::string& text) {
        while (!text.empty() && (text.back() == '\r' || text.back() == ' '))
            text.pop_back();
        text.erase(0, text.find_first_not_of(' '));
    };
    while (std::getline(in, code) && std::getline(in, value)) {
        trim(code);
        trim(value);
        if (code == "0" && value == "SHAPE")
            ++shapes;
    }
    return shapes;
}

// A SHAPE between two lines; nameGroup is what the file has for group 2.
std::string drawingWithShape(const std::string& nameGroup) {
    return
        "0\nSECTION\n2\nENTITIES\n"
        "0\nLINE\n5\n100\n8\n0\n10\n0.0\n20\n0.0\n11\n10.0\n21\n5.0\n"
        "0\nSHAPE\n5\n101\n330\n1F\n100\nAcDbEntity\n8\n0\n100\nAcDbShape\n"
        "10\n15.25\n20\n-20.5\n30\n0.0\n40\n2.5\n" +
        nameGroup +
        "50\n0.0\n"
        "0\nLINE\n5\n102\n8\n0\n10\n4.0\n20\n5.0\n11\n6.0\n21\n7.0\n"
        "0\nENDSEC\n0\nEOF\n";
}

} // namespace

TEST_CASE("a DXF with a SHAPE that has no name is read",
          "[dxf][filter][shape]") {
    ensureSettings();
    struct NameGroup {
        const char* m_label;
        const char* m_group;
        const char* m_expected;
    };
    const NameGroup nameGroup = GENERATE(
        NameGroup{"no group 2, as LibreCAD writes it", "", ""},
        NameGroup{"an empty name, as LibreDWG and ACadSharp write it", "2\n\n", ""},
        NameGroup{"the name AutoCAD writes", "2\nMY-SHAPE\n", "MY-SHAPE"});
    CAPTURE(nameGroup.m_label);

    const std::string src = tempPath("read.dxf");
    std::ofstream(src, std::ios::binary) << drawingWithShape(nameGroup.m_group);

    RS_Graphic graphic;
    graphic.initForNewDocument();
    {
        RS_FilterDXFRW filter;
        CHECK(filter.fileImport(graphic, QString::fromStdString(src), RS2::FormatDXFRW));
    }
    std::filesystem::remove(src);

    // The entities around the SHAPE, and what the SHAPE does say.
    CHECK(countLines(graphic) == 2);
    const auto& shapes = graphic.dwgAdvancedMetadata().shapes();
    REQUIRE(shapes.size() == 1);
    CHECK(shapes.front().handle == 0x101u);
    CHECK(shapes.front().styleName == nameGroup.m_expected);
    CHECK(shapes.front().insertionPoint.x == 15.25);
    CHECK(shapes.front().insertionPoint.y == -20.5);
    CHECK(shapes.front().scale == 2.5);
}

TEST_CASE("a SHAPE read from a DWG is left out of the DXF, which opens",
          "[dxf][filter][shape][roundtrip]") {
    ensureSettings();
    const Format dxf = GENERATE(from_range(std::begin(kDxfFormats), std::end(kDxfFormats)));
    CAPTURE(dxf.m_name);

    RS_Graphic graphic;
    graphic.initForNewDocument();
    graphic.addEntity(new RS_Line(&graphic, {RS_Vector(0., 0.), RS_Vector(10., 5.)}));
    {
        // What the DWG reader delivers: the number of the shape and the STYLE
        // record of its shape file, a record that has no name.
        DRW_Shape shape;
        shape.handle = 0x889;
        shape.parentHandle = 0x1F;
        shape.m_shapeFileHandle = 0x887;
        shape.m_shapeIndex = 130;
        shape.m_insertionPoint = DRW_Coord(15.25, -20.5, 0.0);
        shape.m_scale = 2.5;
        graphic.dwgAdvancedMetadata().addShape(shape);
    }

    const std::string out = tempPath(std::string(dxf.m_name) + ".dxf");
    {
        RS_FilterDXFRW filter;
        REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), dxf.m_type));
        CHECK(filter.exportReport().toStdString() == "1 × SHAPE without a name");
    }
    CHECK(countShapeRecords(out) == 0);

    RS_Graphic reopened;
    reopened.initForNewDocument();
    {
        RS_FilterDXFRW filter;
        CHECK(filter.fileImport(reopened, QString::fromStdString(out), dxf.m_type));
    }
    std::filesystem::remove(out);

    CHECK(countLines(reopened) == 1);
    CHECK(reopened.dwgAdvancedMetadata().shapes().empty());
}

TEST_CASE("a DXF SHAPE that has no name is left out when the drawing is saved",
          "[dxf][filter][shape][roundtrip]") {
    ensureSettings();
    struct NameGroup {
        const char* m_label;
        const char* m_group;
        std::size_t m_written;
        const char* m_report;
    };
    const NameGroup nameGroup = GENERATE(
        NameGroup{"no group 2", "", 0, "1 × SHAPE without a name"},
        NameGroup{"an empty name, as LibreDWG and ACadSharp write it", "2\n\n", 0,
                  "1 × SHAPE without a name"},
        NameGroup{"the name AutoCAD writes", "2\nMY-SHAPE\n", 1, ""});
    CAPTURE(nameGroup.m_label);

    const std::string src = tempPath("resave-src.dxf");
    std::ofstream(src, std::ios::binary) << drawingWithShape(nameGroup.m_group);
    RS_Graphic graphic;
    graphic.initForNewDocument();
    {
        RS_FilterDXFRW filter;
        REQUIRE(filter.fileImport(graphic, QString::fromStdString(src), RS2::FormatDXFRW));
    }
    std::filesystem::remove(src);

    const std::string out = tempPath("resave-out.dxf");
    {
        RS_FilterDXFRW filter;
        REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW));
        CHECK(filter.exportReport().toStdString() == nameGroup.m_report);
    }
    CHECK(countShapeRecords(out) == nameGroup.m_written);

    RS_Graphic reopened;
    reopened.initForNewDocument();
    {
        RS_FilterDXFRW filter;
        CHECK(filter.fileImport(reopened, QString::fromStdString(out), RS2::FormatDXFRW));
    }
    std::filesystem::remove(out);

    CHECK(countLines(reopened) == 2);
    const auto& shapes = reopened.dwgAdvancedMetadata().shapes();
    REQUIRE(shapes.size() == nameGroup.m_written);
    if (!shapes.empty()) {
        CHECK(shapes.front().styleName == "MY-SHAPE");
        CHECK(shapes.front().insertionPoint.x == 15.25);
        CHECK(shapes.front().scale == 2.5);
    }
}

TEST_CASE("SHX shape names are bounded by the indexed records",
          "[dxf][filter][shape][shx]") {
    const QByteArray bytes = shapeFileBytes();
    const auto names = LC_ShxShapeNames::parse(bytes);
    REQUIRE(names.has_value());
    REQUIRE(names->size() == 1);
    CHECK(names->at(130) == "MY-SHAPE");
    for (qsizetype size = 0; size < bytes.size(); ++size)
        CHECK_FALSE(LC_ShxShapeNames::parse(bytes.left(size)).has_value());
    QByteArray bad = bytes;
    bad[32] = static_cast<char>(0xff); // Record length exceeds the file.
    CHECK_FALSE(LC_ShxShapeNames::parse(bad).has_value());
    bad = bytes;
    bad[42] = 'X'; // Remove the name terminator.
    CHECK_FALSE(LC_ShxShapeNames::parse(bad).has_value());
    bad = bytes;
    bad[37] = '\n'; // A name cannot inject a DXF record.
    CHECK_FALSE(LC_ShxShapeNames::parse(bad).has_value());
    bad = bytes;
    bad[20] = '1'; // shapes 1.1 has the same directory format.
    CHECK(LC_ShxShapeNames::parse(bad).has_value());
    CHECK_FALSE(LC_ShxShapeNames::parse(QByteArray("not a compiled shape file")).has_value());
}

TEST_CASE("a DWG SHAPE gets its DXF glyph name from its SHX file",
          "[dxf][filter][shape][shx][roundtrip]") {
    ensureSettings();
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QFile shx(dir.filePath("symbols.shx"));
    REQUIRE(shx.open(QIODevice::WriteOnly));
    REQUIRE(shx.write(shapeFileBytes()) == shapeFileBytes().size());
    shx.close();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addShapeFileStyle(graphic, "symbols.shx");
    addNumberedShape(graphic);
    const QString out = dir.filePath("shape.dxf");
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, out, RS2::FormatDXFRW));
    CHECK(filter.exportReport().isEmpty());
    RS_Graphic reopened;
    reopened.initForNewDocument();
    RS_FilterDXFRW reader;
    REQUIRE(reader.fileImport(reopened, out, RS2::FormatDXFRW));
    const auto &shapes = reopened.dwgAdvancedMetadata().shapes();
    REQUIRE(shapes.size() == 1);
    CHECK(shapes.front().styleName == "MY-SHAPE");
    CHECK(shapes.front().color == 3);
    CHECK(shapes.front().scale == 2.5);

    REQUIRE(shx.remove());
    REQUIRE(filter.fileExport(graphic, out, RS2::FormatDXFRW));
    CHECK(filter.exportReport().contains("SHAPE without a name"));
    CHECK(countShapeRecords(out.toStdString()) == 0);
}

#ifdef DWGSUPPORT
TEST_CASE("native DWG line-only interoperability controls round-trip",
          "[dwg][filter][container][roundtrip]") {
    ensureSettings();
    const auto format = GENERATE(RS2::FormatDWG, RS2::FormatDWG2004,
                                 RS2::FormatDWG2007, RS2::FormatDWG2010,
                                 RS2::FormatDWG2013, RS2::FormatDWG2018);
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const QString out = QDir(qEnvironmentVariable("LIBRECAD_SHAPE_OUTPUT", directory.path()))
                            .filePath(QStringLiteral("control-%1.dwg").arg(format));
    RS_Graphic graphic;
    graphic.initForNewDocument();
    graphic.addEntity(new RS_Line(&graphic, {RS_Vector(0., 0.), RS_Vector(10., 5.)}));
    RS_FilterDXFRW writer;
    REQUIRE(writer.fileExport(graphic, out, format));
    RS_Graphic reopened;
    RS_FilterDXFRW reader;
    REQUIRE(reader.fileImport(reopened, out, format));
    CHECK(countLines(reopened) == 1);
}

TEST_CASE("ODA-audited native DWG exports retain lines and SHAPEs",
          "[dwg][filter][shape][oda][external-fixture]") {
    const QString directory = qEnvironmentVariable("LIBRECAD_ODA_OUTPUT");
    if (directory.isEmpty())
        SKIP("Set LIBRECAD_ODA_OUTPUT to the audited native-DWG output directory");
    ensureSettings();
    const auto format = GENERATE(RS2::FormatDWG, RS2::FormatDWG2004,
                                 RS2::FormatDWG2007, RS2::FormatDWG2010,
                                 RS2::FormatDWG2013, RS2::FormatDWG2018);
    const bool shapes = GENERATE(false, true);
    const QString path = QDir(directory).filePath(
        QStringLiteral("%1-%2.dwg").arg(shapes ? "shapes" : "control").arg(format));
    CAPTURE(path);
    REQUIRE_FALSE(QFileInfo::exists(path + ".err"));
    REQUIRE(QFileInfo::exists(path));
    RS_Graphic graphic;
    RS_FilterDXFRW reader;
    REQUIRE(reader.fileImport(graphic, path, RS2::FormatDWG2018));
    CHECK(countLines(graphic) == 1);
    REQUIRE(graphic.dwgAdvancedMetadata().shapes().size() == (shapes ? 2u : 0u));
    std::set<std::string> fonts;
    for (const auto& shape : graphic.dwgAdvancedMetadata().shapes()) {
        CHECK(shape.shapeIndex == 130);
        CHECK(shape.color == 3);
        CHECK(shape.scale == 2.5);
        CHECK(shape.widthFactor == 1.25);
        CHECK(shape.thickness == 0.5);
        CHECK(shape.rotation == 0.25);
        CHECK(shape.oblique == 0.125);
        CHECK(shape.insertionPoint.x == 15.25);
        CHECK(shape.insertionPoint.y == -20.5);
        const auto& styles = graphic.dwgAdvancedMetadata().textStyles();
        const auto style = styles.find(shape.shapeFileHandle);
        REQUIRE(style != styles.end());
        fonts.insert(style->second.font);
    }
    if (shapes)
        CHECK(fonts == std::set<std::string>{"missing-first.shx", "missing-second.shx"});
}

TEST_CASE("SHAPE export keeps a local SHX reachable from a different directory",
          "[dwg][dxf][filter][shape][shx][roundtrip]") {
    ensureSettings();
    const auto format = GENERATE(RS2::FormatDXFRW, RS2::FormatDWG2018);
    const bool named = GENERATE(false, true);
    const bool symlinkedOutput = GENERATE(false, true);
    QTemporaryDir source;
    QTemporaryDir destination;
    REQUIRE(source.isValid());
    REQUIRE(destination.isValid());
    QString outputDirectory = destination.path();
    if (symlinkedOutput) {
        outputDirectory = source.filePath("output-link");
        std::error_code error;
        std::filesystem::create_directory_symlink(destination.path().toStdString(),
                                                  outputDirectory.toStdString(), error);
        if (error)
            SKIP("Directory symlinks are unavailable on this system");
    }
    QFile shx(source.filePath("symbols.shx"));
    REQUIRE(shx.open(QIODevice::WriteOnly));
    REQUIRE(shx.write(shapeFileBytes()) == shapeFileBytes().size());
    shx.close();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    graphic.setFilename(source.filePath("source.dxf"));
    addShapeFileStyle(graphic, "symbols.shx");
    if (named) {
        DRW_Shape shape;
        shape.handle = 0x889;
        shape.m_styleName = "MY-SHAPE";
        graphic.dwgAdvancedMetadata().addShape(shape);
    } else {
        addNumberedShape(graphic);
    }
    const QString out = QDir(outputDirectory).filePath(format == RS2::FormatDXFRW
                                                         ? "shape.dxf" : "shape.dwg");
    RS_FilterDXFRW writer;
    REQUIRE(writer.fileExport(graphic, out, format));
    CHECK(writer.exportReport().isEmpty());
    RS_Graphic reopened;
    RS_FilterDXFRW reader;
    REQUIRE(reader.fileImport(reopened, out, format));
    REQUIRE(reopened.dwgAdvancedMetadata().shapes().size() == 1);
    bool foundStyle = false;
    for (const auto &[handle, style] : reopened.dwgAdvancedMetadata().textStyles()) {
        if ((style.flags & 1) == 0)
            continue;
        foundStyle = true;
        CHECK(QFileInfo(QString::fromStdString(style.font)).isAbsolute());
        CHECK(QFileInfo(QDir(destination.path()).filePath(QString::fromStdString(style.font)))
                  .canonicalFilePath() == QFileInfo(shx.fileName()).canonicalFilePath());
    }
    CHECK(foundStyle);
}

TEST_CASE("SHAPE space ownership is resolved before structural handle aliases",
          "[dwg][dxf][filter][shape][roundtrip]") {
    ensureSettings();
    const auto format = GENERATE(RS2::FormatDXFRW, RS2::FormatDWG,
                                 RS2::FormatDWG2018);
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QFile shx(dir.filePath("symbols.shx"));
    REQUIRE(shx.open(QIODevice::WriteOnly));
    REQUIRE(shx.write(shapeFileBytes()) == shapeFileBytes().size());
    shx.close();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    auto *block = new RS_Block(&graphic, RS_BlockData("Symbols", RS_Vector(0., 0.), false));
    block->setSourceHandle(DRW::DwgModelSpaceBlockRecordHandle);
    graphic.addBlock(block);
    graphic.dwgAdvancedMetadata().addSpaceBlockRecord(0x700, "*Model_Space");
    addShapeFileStyle(graphic, "symbols.shx");
    addNumberedShape(graphic, 0x889, 0x887, 0x700);
    const QString out = dir.filePath(format == RS2::FormatDXFRW ? "shape.dxf" : "shape.dwg");
    RS_FilterDXFRW writer;
    REQUIRE(writer.fileExport(graphic, out, format));
    CHECK(writer.exportReport().isEmpty());
    RS_Graphic reopened;
    RS_FilterDXFRW reader;
    REQUIRE(reader.fileImport(reopened, out, format));
    const auto &shapes = reopened.dwgAdvancedMetadata().shapes();
    REQUIRE(shapes.size() == 1);
    CHECK(shapes.front().ownerBlockName.empty());
    CHECK(shapes.front().space == DRW::ModelSpace);
}

TEST_CASE("DWG export preserves SHAPE indices and unnamed shape-file STYLEs",
          "[dwg][filter][shape][roundtrip]") {
    ensureSettings();
    const auto format = GENERATE(RS2::FormatDWG, RS2::FormatDWG2004,
                                 RS2::FormatDWG2007, RS2::FormatDWG2010,
                                 RS2::FormatDWG2013, RS2::FormatDWG2018);
    CAPTURE(format);
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    RS_Graphic graphic;
    graphic.initForNewDocument();
    graphic.addEntity(new RS_Line(&graphic, {RS_Vector(0., 0.), RS_Vector(10., 5.)}));
    addShapeFileStyle(graphic, "missing-first.shx");
    addShapeFileStyle(graphic, "missing-second.shx", 0x987);
    addNumberedShape(graphic);
    addNumberedShape(graphic, 0x989, 0x987);
    const QString out = QDir(qEnvironmentVariable("LIBRECAD_SHAPE_OUTPUT", dir.path()))
                            .filePath(QStringLiteral("shapes-%1.dwg").arg(format));
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, out, format));
    CHECK(filter.exportReport().isEmpty());
    RS_Graphic reopened;
    reopened.initForNewDocument();
    RS_FilterDXFRW reader;
    REQUIRE(reader.fileImport(reopened, out, format));
    CHECK(countLines(reopened) == 1);
    const auto &metadata = reopened.dwgAdvancedMetadata();
    REQUIRE(metadata.shapes().size() == 2);
    CHECK(metadata.textStyles().size() >= 3);
    CHECK(metadata.shapes()[0].shapeFileHandle != metadata.shapes()[1].shapeFileHandle);
    for (const auto &shape : metadata.shapes()) {
        CHECK(shape.shapeIndex == 130);
        CHECK(shape.insertionPoint.x == 15.25);
        CHECK(shape.insertionPoint.y == -20.5);
        CHECK(shape.insertionPoint.z == 1.0);
        CHECK(shape.scale == 2.5);
        CHECK(shape.rotation == 0.25);
        CHECK(shape.widthFactor == 1.25);
        CHECK(shape.oblique == 0.125);
        CHECK(shape.thickness == 0.5);
        CHECK(shape.color == 3);
        CHECK(shape.lineTypeScale == 2.0);
        REQUIRE(shape.extData.size() == 2);
        CHECK(std::string(shape.extData.back()->c_str()) == "shape payload");
        const auto style = metadata.textStyles().find(shape.shapeFileHandle);
        REQUIRE(style != metadata.textStyles().end());
        CHECK(style->second.name.empty());
        CHECK((style->second.flags & 1) != 0);
    }
}

TEST_CASE("DWG export reports an unresolved SHAPE and resets the report",
          "[dwg][filter][shape]") {
    ensureSettings();
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    RS_Graphic graphic;
    graphic.initForNewDocument();
    graphic.addEntity(new RS_Line(&graphic, {RS_Vector(0., 0.), RS_Vector(10., 5.)}));
    addNumberedShape(graphic); // The STYLE record is missing.
    RS_FilterDXFRW filter;
    const QString out = dir.filePath("shape.dwg");
    REQUIRE(filter.fileExport(graphic, out, RS2::FormatDWG2018));
    CHECK(filter.exportReport() ==
          QString::fromUtf8("1 × SHAPE without a resolved shape-file STYLE or index"));
    addShapeFileStyle(graphic, "missing.shx");
    REQUIRE(filter.fileExport(graphic, out, RS2::FormatDWG2018));
    CHECK(filter.exportReport().isEmpty());
}

TEST_CASE("a named DXF SHAPE resolves its DWG index through SHX",
          "[dwg][dxf][filter][shape][shx][roundtrip]") {
    ensureSettings();
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QFile shx(dir.filePath("symbols.shx"));
    REQUIRE(shx.open(QIODevice::WriteOnly));
    REQUIRE(shx.write(shapeFileBytes()) == shapeFileBytes().size());
    shx.close();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addShapeFileStyle(graphic, "symbols.shx");
    DRW_Shape shape;
    shape.handle = 0x889;
    shape.m_styleName = "MY-SHAPE";
    graphic.dwgAdvancedMetadata().addShape(shape);
    RS_FilterDXFRW filter;
    const QString out = dir.filePath("shape.dwg");
    REQUIRE(filter.fileExport(graphic, out, RS2::FormatDWG2018));
    CHECK(filter.exportReport().isEmpty());
    RS_Graphic reopened;
    reopened.initForNewDocument();
    RS_FilterDXFRW reader;
    REQUIRE(reader.fileImport(reopened, out, RS2::FormatDWG2018));
    REQUIRE(reopened.dwgAdvancedMetadata().shapes().size() == 1);
    CHECK(reopened.dwgAdvancedMetadata().shapes().front().shapeIndex == 130);
}

TEST_CASE("DWG SHAPE export remaps a block owner and a structural source handle",
          "[dwg][filter][shape][roundtrip]") {
    ensureSettings();
    const auto format = GENERATE(RS2::FormatDWG, RS2::FormatDWG2013);
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    RS_Graphic graphic;
    graphic.initForNewDocument();
    auto *block = new RS_Block(&graphic, RS_BlockData("Symbols", RS_Vector(0., 0.), false));
    block->setSourceHandle(0xA50);
    graphic.addBlock(block);
    addShapeFileStyle(graphic, "missing.shx");
    addNumberedShape(graphic, DRW::DwgModelSpaceBlockRecordHandle, 0x887, block->sourceHandle());
    RS_FilterDXFRW filter;
    const QString out = dir.filePath("block-shape.dwg");
    REQUIRE(filter.fileExport(graphic, out, format));
    CHECK(filter.exportReport().isEmpty());
    RS_Graphic reopened;
    reopened.initForNewDocument();
    RS_FilterDXFRW reader;
    REQUIRE(reader.fileImport(reopened, out, format));
    const auto &shapes = reopened.dwgAdvancedMetadata().shapes();
    REQUIRE(shapes.size() == 1);
    CHECK(shapes.front().handle != DRW::DwgModelSpaceBlockRecordHandle);
    CHECK(shapes.front().parentHandle != 0);
    CHECK(shapes.front().parentHandle != 0xA50);
}

TEST_CASE("DWG SHAPE export respects ownerless paper-space metadata",
          "[dwg][filter][shape][roundtrip]") {
    ensureSettings();
    const auto format = GENERATE(RS2::FormatDWG, RS2::FormatDWG2018);
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    RS_Graphic graphic;
    graphic.initForNewDocument();
    graphic.addEntity(new RS_Line(&graphic, {RS_Vector(0., 0.), RS_Vector(10., 5.)}));
    addShapeFileStyle(graphic, "missing.shx");
    addNumberedShape(graphic, 0x889, 0x887, 0, DRW::PaperSpace);
    RS_FilterDXFRW filter;
    const QString out = dir.filePath("paper-shape.dwg");
    REQUIRE(filter.fileExport(graphic, out, format));
    RS_Graphic reopened;
    reopened.initForNewDocument();
    RS_FilterDXFRW reader;
    REQUIRE(reader.fileImport(reopened, out, format));
    const auto &shapes = reopened.dwgAdvancedMetadata().shapes();
    REQUIRE(shapes.size() == 1);
    CHECK(shapes.front().space == DRW::PaperSpace);
}

TEST_CASE("DXF block SHAPEs stay inside their BLOCK with a new owner handle",
          "[dxf][filter][shape][roundtrip]") {
    ensureSettings();
    const auto format = GENERATE(RS2::FormatDXFRW12, RS2::FormatDXFRW,
                                 RS2::FormatDXFRW2018);
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    const auto source = dir.filePath("block-source.dxf").toStdString();
    std::ofstream(source) <<
        "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1027\n0\nENDSEC\n"
        "0\nSECTION\n2\nTABLES\n0\nTABLE\n2\nBLOCK_RECORD\n70\n1\n"
        "0\nBLOCK_RECORD\n5\nA50\n2\nSymbols\n70\n0\n"
        "0\nENDTAB\n0\nENDSEC\n0\nSECTION\n2\nBLOCKS\n"
        "0\nBLOCK\n5\nA51\n330\nA50\n8\n0\n2\nSymbols\n70\n0\n"
        "10\n0\n20\n0\n3\nSymbols\n"
        "0\nSHAPE\n5\nA52\n330\nA50\n8\n0\n2\nMY-SHAPE\n"
        "10\n1\n20\n2\n40\n1\n"
        "0\nENDBLK\n5\nA53\n330\nA50\n8\n0\n0\nENDSEC\n"
        "0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n";
    RS_Graphic graphic;
    RS_FilterDXFRW reader;
    REQUIRE(reader.fileImport(graphic, QString::fromStdString(source), RS2::FormatDXFRW));
    REQUIRE(graphic.dwgAdvancedMetadata().shapes().size() == 1);
    const QString out = dir.filePath("block-out.dxf");
    RS_FilterDXFRW writer;
    REQUIRE(writer.fileExport(graphic, out, format));
    CHECK(writer.exportReport().isEmpty());
    std::string block;
    std::size_t shapes = 0;
    for (const auto &record : lc::test::dxfRecords(out.toStdString())) {
        if (record.type() == "BLOCK")
            block = record.own(2).front();
        else if (record.type() == "ENDBLK")
            block.clear();
        else if (record.type() == "SHAPE") {
            ++shapes;
            CHECK((block == "Symbols" || block == "SYMBOLS"));
            if (format != RS2::FormatDXFRW12)
                CHECK(record.own(330) != std::vector<std::string>{"A50"});
        }
    }
    CHECK(shapes == 1);
    RS_Graphic reopened;
    RS_FilterDXFRW reReader;
    REQUIRE(reReader.fileImport(reopened, out, format));
    REQUIRE(reopened.dwgAdvancedMetadata().shapes().size() == 1);
    CHECK_FALSE(reopened.dwgAdvancedMetadata().shapes().front().ownerBlockName.empty());
}

TEST_CASE("external SHAPE sample exports with registered XDATA applications",
          "[dxf][filter][shape][external-fixture]") {
    const QString fixture = qEnvironmentVariable("LIBRECAD_SHAPE_FIXTURE");
    if (fixture.isEmpty())
        SKIP("Set LIBRECAD_SHAPE_FIXTURE to exercise a corpus DWG");
    ensureSettings();
    QTemporaryDir temporary;
    REQUIRE(temporary.isValid());
    const QString output = qEnvironmentVariable("LIBRECAD_SHAPE_OUTPUT", temporary.path());
    RS_Graphic graphic;
    graphic.initForNewDocument();
    RS_FilterDXFRW reader;
    REQUIRE(reader.fileImport(graphic, fixture, RS2::FormatDWG));
    REQUIRE_FALSE(graphic.dwgAdvancedMetadata().shapes().empty());
    const QString out = output + "/sample.dxf";
    RS_FilterDXFRW writer;
    REQUIRE(writer.fileExport(graphic, out, RS2::FormatDXFRW));
    INFO(writer.exportReport().toStdString());
    RS_Graphic reopened;
    reopened.initForNewDocument();
    RS_FilterDXFRW reReader;
    REQUIRE(reReader.fileImport(reopened, out, RS2::FormatDXFRW));
    CHECK(countLines(reopened) == countLines(graphic));
    const QString expectedName = qEnvironmentVariable("LIBRECAD_SHAPE_EXPECT_NAME");
    if (!expectedName.isEmpty()) {
        REQUIRE(reopened.dwgAdvancedMetadata().shapes().size() == 1);
        CHECK(reopened.dwgAdvancedMetadata().shapes().front().styleName == expectedName.toStdString());
    }
    std::set<std::string> applications;
    for (const auto &record : lc::test::dxfRecords(out.toStdString(), "APPID"))
        applications.insert(QString::fromStdString(record.own(2).front()).toUpper().toStdString());
    for (const auto &record : lc::test::dxfRecords(out.toStdString())) {
        for (const auto &name : record.own(1001)) {
            CAPTURE(name);
            CHECK(applications.count(QString::fromStdString(name).toUpper().toStdString()) == 1);
        }
    }
}

TEST_CASE("external SHAPE geometry exports native R2018 DWG",
          "[dwg][filter][shape][external-fixture]") {
    const QString fixture = qEnvironmentVariable("LIBRECAD_SHAPE_FIXTURE");
    if (fixture.isEmpty())
        SKIP("Set LIBRECAD_SHAPE_FIXTURE to exercise a corpus DWG");
    ensureSettings();
    QTemporaryDir temporary;
    REQUIRE(temporary.isValid());
    RS_Graphic source;
    RS_FilterDXFRW reader;
    REQUIRE(reader.fileImport(source, fixture, RS2::FormatDWG));
    const auto& metadata = source.dwgAdvancedMetadata();
    REQUIRE_FALSE(metadata.shapes().empty());
    // Isolate actual corpus geometry from unrelated DataStorage replay policy.
    RS_Graphic graphic;
    graphic.initForNewDocument();
    graphic.addEntity(new RS_Line(&graphic, {RS_Vector(0., 0.), RS_Vector(10., 5.)}));
    for (const auto& record : metadata.shapes()) {
        const auto style = metadata.textStyles().find(record.shapeFileHandle);
        REQUIRE(style != metadata.textStyles().end());
        graphic.dwgAdvancedMetadata().addTextStyleName(style->second);
        DRW_Shape shape;
        shape.handle = record.handle;
        shape.color = record.color;
        shape.m_shapeFileHandle = record.shapeFileHandle;
        shape.m_shapeIndex = record.shapeIndex;
        shape.m_insertionPoint = record.insertionPoint;
        shape.m_extrusion = record.extrusion;
        shape.m_scale = record.scale;
        shape.m_rotation = record.rotation;
        shape.m_oblique = record.oblique;
        shape.m_widthFactor = record.widthFactor;
        shape.m_thickness = record.thickness;
        graphic.dwgAdvancedMetadata().addShape(shape, record.sourceDirectory);
    }
    const QString path = QDir(qEnvironmentVariable("LIBRECAD_SHAPE_OUTPUT", temporary.path()))
                             .filePath("sample-shape-geometry.dwg");
    RS_FilterDXFRW writer;
    REQUIRE(writer.fileExport(graphic, path, RS2::FormatDWG2018));
    INFO(writer.exportReport().toStdString());
    RS_Graphic reopened;
    RS_FilterDXFRW reReader;
    REQUIRE(reReader.fileImport(reopened, path, RS2::FormatDWG2018));
    CHECK(countLines(reopened) == countLines(graphic));
    REQUIRE(reopened.dwgAdvancedMetadata().shapes().size()
            == graphic.dwgAdvancedMetadata().shapes().size());
    CHECK(reopened.dwgAdvancedMetadata().shapes().front().shapeIndex
          == graphic.dwgAdvancedMetadata().shapes().front().shapeIndex);
}
#endif
