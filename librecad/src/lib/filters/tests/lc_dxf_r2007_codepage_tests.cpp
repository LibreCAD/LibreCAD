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

// R2007+ DXF text is UTF-8 whatever $DWGCODEPAGE says. A drawing opened from
// a CP1250 file keeps ANSI_1250 in its header, and LibreCAD 2.2 saves it as
// R2007 with that header over UTF-8 text. The codec used to apply the header
// codepage to such files, so a Czech layer name came back as mojibake and the
// text it wrote was CP1250 bytes inside an R2007 file.

#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include <QCoreApplication>
#include <QString>

#include "drw_textcodec.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_settings.h"
#include "rs_text.h"

namespace {

const std::string kUtf8CapitalRCaron = "\xC5\x98";   // U+0158
const std::string kCp1250CapitalRCaron = "\xD8";

// Configures a codec the way the DXF reader does: $ACADVER, then $DWGCODEPAGE.
void useDxfHeader(DRW_TextCodec& codec, const char* acadver, const char* codePage) {
    codec.setVersion(acadver, /*dxfFormat=*/true);
    codec.setCodePage(codePage, /*dxfFormat=*/true);
}

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

std::string tempPath(const char* suffix) {
    return (std::filesystem::temp_directory_path() /
            (std::string("lc_dxf_r2007_codepage_") + suffix + ".dxf"))
        .string();
}

std::string readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), {});
}

// An R2007 drawing whose header declares ANSI_1250 above UTF-8 text: layer
// "OREZ" with a caron on the R (C5 98) and the TEXT "rericha" with carons on
// both r (C5 99).
std::string r2007Drawing() {
    return
        "0\nSECTION\n2\nHEADER\n"
        "9\n$ACADVER\n1\nAC1021\n"
        "9\n$DWGCODEPAGE\n3\nANSI_1250\n"
        "0\nENDSEC\n"
        "0\nSECTION\n2\nTABLES\n"
        "0\nTABLE\n2\nLAYER\n5\n2\n100\nAcDbSymbolTable\n70\n2\n"
        "0\nLAYER\n5\n10\n100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n"
        "2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n"
        "0\nLAYER\n5\n11\n100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n"
        "2\nO\xC5\x98" "EZ\n70\n0\n62\n3\n6\nCONTINUOUS\n"
        "0\nENDTAB\n"
        "0\nENDSEC\n"
        "0\nSECTION\n2\nENTITIES\n"
        "0\nTEXT\n5\n20\n100\nAcDbEntity\n8\nO\xC5\x98" "EZ\n"
        "100\nAcDbText\n10\n0.0\n20\n0.0\n30\n0.0\n40\n2.5\n"
        "1\n\xC5\x99" "e\xC5\x99" "icha\n100\nAcDbText\n"
        "0\nENDSEC\n0\nEOF\n";
}

} // namespace

TEST_CASE("an R2007+ DXF ignores $DWGCODEPAGE and keeps its UTF-8 text",
          "[dxf][codec][r2007]") {
    for (const char* acadver : {"AC1021", "AC1024", "AC1027", "AC1032"}) {
        DYNAMIC_SECTION(acadver) {
            DRW_TextCodec codec;
            useDxfHeader(codec, acadver, "ANSI_1250");
            CHECK(codec.toUtf8(kUtf8CapitalRCaron) == kUtf8CapitalRCaron);
            CHECK(codec.fromUtf8(kUtf8CapitalRCaron) == kUtf8CapitalRCaron);
            // The header still reports what the file declared.
            CHECK(codec.getCodePage() == "ANSI_1250");
        }
    }

    SECTION("every table codepage is ignored") {
        // U+3042, U+56FE, U+B3C4, U+0416, U+0158
        const std::string text = "\xE3\x81\x82\xE5\x9B\xBE\xEB\x8F\x84\xD0\x96\xC5\x98";
        for (const char* codePage : {"ANSI_874", "ANSI_932", "ANSI_936", "ANSI_949",
                                     "ANSI_950", "ANSI_1251", "ANSI_1252"}) {
            DYNAMIC_SECTION(codePage) {
                DRW_TextCodec codec;
                useDxfHeader(codec, "AC1027", codePage);
                CHECK(codec.toUtf8(text) == text);
                CHECK(codec.fromUtf8(text) == text);
                CHECK(codec.getCodePage() == codePage);
            }
        }
    }

    SECTION("a header declaring UTF-8 is reported as ANSI_1252") {
        DRW_TextCodec codec;
        useDxfHeader(codec, "AC1032", "UTF-8");
        CHECK(codec.toUtf8(kUtf8CapitalRCaron) == kUtf8CapitalRCaron);
        CHECK(codec.getCodePage() == "ANSI_1252");
    }
}

TEST_CASE("a pre-R2007 DXF still decodes with its $DWGCODEPAGE",
          "[dxf][codec][r2007]") {
    for (const char* acadver : {"AC1009", "AC1014", "AC1015", "AC1018"}) {
        DYNAMIC_SECTION(acadver) {
            DRW_TextCodec codec;
            useDxfHeader(codec, acadver, "ANSI_1250");
            CHECK(codec.toUtf8(kCp1250CapitalRCaron) == kUtf8CapitalRCaron);
            CHECK(codec.fromUtf8(kUtf8CapitalRCaron) == kCp1250CapitalRCaron);
        }
    }

    SECTION("a DXF that declares no version keeps the codepage too") {
        DRW_TextCodec codec;
        codec.setCodePage("ANSI_1250", /*dxfFormat=*/true);
        CHECK(codec.toUtf8(kCp1250CapitalRCaron) == kUtf8CapitalRCaron);
    }
}

TEST_CASE("a DWG reader's codepage is not touched by the R2007+ DXF rule",
          "[dxf][codec][r2007]") {
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1021, /*dxfFormat=*/false);
    codec.setCodePage("ANSI_1250", /*dxfFormat=*/false);
    CHECK(codec.toUtf8(kCp1250CapitalRCaron) == kUtf8CapitalRCaron);
}

TEST_CASE("an R2007 DXF declaring ANSI_1250 imports its UTF-8 names",
          "[dxf][filter][r2007]") {
    ensureSettings();
    const std::string src = tempPath("import");
    std::ofstream(src, std::ios::binary) << r2007Drawing();

    RS_Graphic graphic;
    graphic.initForNewDocument();
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
    std::filesystem::remove(src);

    CHECK(graphic.findLayer(QString::fromUtf8("O\xC5\x98" "EZ")) != nullptr);

    QString text;
    for (RS_Entity* e : graphic) {
        if (e != nullptr && e->rtti() == RS2::EntityText)
            text = static_cast<RS_Text*>(e)->getText();
    }
    CHECK(text == QString::fromUtf8("\xC5\x99" "e\xC5\x99" "icha"));
}

TEST_CASE("an R2007 DXF opened with ANSI_1250 is saved as UTF-8",
          "[dxf][filter][r2007]") {
    ensureSettings();
    const std::string src = tempPath("save_src");
    const std::string out = tempPath("save_out");
    std::ofstream(src, std::ios::binary) << r2007Drawing();

    RS_Graphic graphic;
    graphic.initForNewDocument();
    {
        RS_FilterDXFRW filter;
        REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                                  RS2::FormatDXFRW));
    }
    {
        RS_FilterDXFRW filter;
        REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                                  RS2::FormatDXFRW));
    }
    const std::string saved = readFile(out);
    std::filesystem::remove(src);
    std::filesystem::remove(out);

    // The header is kept, which is what makes the text bytes the test.
    REQUIRE(saved.find("AC1021") != std::string::npos);
    REQUIRE(saved.find("ANSI_1250") != std::string::npos);

    CHECK(saved.find("O" + kUtf8CapitalRCaron + "EZ") != std::string::npos);
    CHECK(saved.find("O" + kCp1250CapitalRCaron + "EZ") == std::string::npos);
}
