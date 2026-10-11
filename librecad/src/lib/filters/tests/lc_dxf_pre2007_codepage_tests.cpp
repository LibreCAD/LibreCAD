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

// The DXF reader keeps some strings as the bytes of the file, extended data
// among them, and the writer encodes every string as if it were UTF-8. Bytes
// that are not UTF-8 came out garbled or made the codec throw, and the throw
// made the save fail.

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <string>

#include <QCoreApplication>
#include <QString>

#include "drw_textcodec.h"
#include "rs.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_settings.h"

namespace {

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
            (std::string("lc_dxf_pre2007_codepage_") + suffix + ".dxf"))
        .string();
}

std::string readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), {});
}

// An R2000 drawing whose line carries three extended data strings with bytes
// above 7F: "45" then B0, "Caf" then E9, and "LECTRICIT" between two C9.
std::string r2000DrawingWithExtendedData(const std::string& codePage) {
    return
        "0\nSECTION\n2\nHEADER\n"
        "9\n$ACADVER\n1\nAC1015\n"
        "9\n$DWGCODEPAGE\n3\n" + codePage + "\n"
        "0\nENDSEC\n"
        "0\nSECTION\n2\nTABLES\n"
        "0\nTABLE\n2\nLAYER\n5\n2\n100\nAcDbSymbolTable\n70\n1\n"
        "0\nLAYER\n5\n10\n100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n"
        "2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n"
        "0\nENDTAB\n"
        "0\nTABLE\n2\nAPPID\n5\n9\n100\nAcDbSymbolTable\n70\n2\n"
        "0\nAPPID\n5\n12\n100\nAcDbSymbolTableRecord\n100\nAcDbRegAppTableRecord\n"
        "2\nACAD\n70\n0\n"
        "0\nAPPID\n5\n13\n100\nAcDbSymbolTableRecord\n100\nAcDbRegAppTableRecord\n"
        "2\nMYAPP\n70\n0\n"
        "0\nENDTAB\n"
        "0\nENDSEC\n"
        "0\nSECTION\n2\nENTITIES\n"
        "0\nLINE\n5\n20\n100\nAcDbEntity\n8\n0\n"
        "100\nAcDbLine\n10\n0.0\n20\n0.0\n30\n0.0\n11\n10.0\n21\n0.0\n31\n0.0\n"
        "1001\nMYAPP\n1000\n45\xB0\n1000\nCaf\xE9\n1000\n\xC9LECTRICIT\xC9\n"
        "0\nENDSEC\n0\nEOF\n";
}

bool importDrawing(RS_Graphic& graphic, const std::string& path) {
    graphic.initForNewDocument();
    RS_FilterDXFRW filter;
    return filter.fileImport(graphic, QString::fromStdString(path),
                             RS2::FormatDXFRW);
}

bool importText(RS_Graphic& graphic, const char* suffix, const std::string& drawing) {
    const std::string src = tempPath(suffix);
    std::ofstream(src, std::ios::binary) << drawing;
    const bool imported = importDrawing(graphic, src);
    std::filesystem::remove(src);
    return imported;
}

} // namespace

TEST_CASE("a codepage codec writes bytes that are not UTF-8 as they are",
          "[dxf][codec][pre2007]") {
    // The DXF reader keeps some strings as the bytes of the file (extended
    // data, a multiline style name) and the writer encodes them like the rest.
    for (const char* codePage : {"ANSI_1250", "ANSI_932", "ANSI_936"}) {
        DYNAMIC_SECTION(codePage) {
            DRW_TextCodec codec;
            useDxfHeader(codec, "AC1015", codePage);
            for (const char* raw : {
                     "45\xB0",              // a continuation byte with no lead
                     "Caf\xE9",             // a lead with nothing after it
                     "\xC9LECTRICIT\xC9",   // a lead followed by ASCII
                     "\xFF",                // never part of UTF-8
                     "\xC0\xAF",            // an overlong form
                     "\xED\xA0\x80",        // a surrogate
                     "\xF4\x90\x80\x80"}) { // past U+10FFFF
                INFO("raw " << raw);
                CHECK(codec.fromUtf8(raw) == raw);
            }
        }
    }
}

TEST_CASE("extended data in the codepage of the file survives a save as pre-R2007 DXF",
          "[dxf][filter][pre2007]") {
    ensureSettings();

    for (const char* codePage : {"ANSI_1250"}) {
        DYNAMIC_SECTION(codePage) {
            RS_Graphic graphic;
            REQUIRE(importText(graphic, "xdata_src",
                               r2000DrawingWithExtendedData(codePage)));

            const std::string out = tempPath("xdata_out");
            {
                RS_FilterDXFRW filter;
                REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                                          RS2::FormatDXFRW2000));
            }
            const std::string saved = readFile(out);
            std::filesystem::remove(out);

            REQUIRE(saved.find("AC1015") != std::string::npos);
            REQUIRE(saved.find(codePage) != std::string::npos);
            // The reader keeps these strings as the bytes of the file, so the
            // writer has to leave them alone.
            CHECK(saved.find("45\xB0") != std::string::npos);
            CHECK(saved.find("Caf\xE9") != std::string::npos);
            CHECK(saved.find("\xC9LECTRICIT\xC9") != std::string::npos);
        }
    }
}
