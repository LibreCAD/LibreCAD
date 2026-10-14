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

// R12-R2004 DXF text is in the Windows codepage the header names, and
// ANSI_1252 when it names none or one the reader does not know. The codec had
// no branch for ANSI_1252 in a DXF, so such a file was read byte for byte: a
// degree sign (B0) became U+FFFD, and a drawing saved as R12-R2004 carried
// UTF-8 under an ANSI_1252 header.
//
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
#include <utility>
#include <vector>

#include <QCoreApplication>
#include <QFile>
#include <QString>
#include <QTemporaryDir>

#include "drw_textcodec.h"
#include "lc_dwgadvancedmetadata.h"
#include "rs.h"
#include "rs_block.h"
#include "rs_blocklist.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_settings.h"
#include "rs_text.h"

namespace {

// Every $ACADVER the codec treats as R12 or as R13-R2004.
const char* const kPre2007Versions[] = {"AC1006", "AC1009", "AC1012",
                                        "AC1014", "AC1015", "AC1018"};

// Configures a codec the way the DXF reader does: $ACADVER, then $DWGCODEPAGE.
void useDxfHeader(DRW_TextCodec& codec, const char* acadver, const char* codePage) {
    codec.setVersion(acadver, /*dxfFormat=*/true);
    codec.setCodePage(codePage, /*dxfFormat=*/true);
}

// U+00E9 is E9 and U+00B0 is B0 in Windows-1252.
void checkWindows1252(DRW_TextCodec& codec) {
    CHECK(codec.toUtf8("\xE9") == "\xC3\xA9");
    CHECK(codec.toUtf8("\xB0") == "\xC2\xB0");
    CHECK(codec.fromUtf8("\xC3\xA9") == "\xE9");
    CHECK(codec.fromUtf8("\xC2\xB0") == "\xB0");
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

// The layer "CAFE" with an acute on the E, and the TEXT "ete 108" with an
// acute on both e and a degree sign at the end.
const QString kLayerName = QString::fromUtf8("CAF\xC3\x89");
const QString kText = QString::fromUtf8("\xC3\xA9t\xC3\xA9 108\xC2\xB0");

// That drawing as R2000 writes it: Windows-1252, declared in the header.
std::string r2000Drawing() {
    return
        "0\nSECTION\n2\nHEADER\n"
        "9\n$ACADVER\n1\nAC1015\n"
        "9\n$DWGCODEPAGE\n3\nANSI_1252\n"
        "0\nENDSEC\n"
        "0\nSECTION\n2\nTABLES\n"
        "0\nTABLE\n2\nLAYER\n5\n2\n100\nAcDbSymbolTable\n70\n2\n"
        "0\nLAYER\n5\n10\n100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n"
        "2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n"
        "0\nLAYER\n5\n11\n100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n"
        "2\nCAF\xC9\n70\n0\n62\n3\n6\nCONTINUOUS\n"
        "0\nENDTAB\n"
        "0\nENDSEC\n"
        "0\nSECTION\n2\nENTITIES\n"
        "0\nTEXT\n5\n20\n100\nAcDbEntity\n8\nCAF\xC9\n"
        "100\nAcDbText\n10\n0.0\n20\n0.0\n30\n0.0\n40\n2.5\n"
        "1\n\xE9t\xE9 108\xB0\n100\nAcDbText\n"
        "0\nENDSEC\n0\nEOF\n";
}

// And as R12 writes it: the same bytes, no $DWGCODEPAGE and no handles.
std::string r12Drawing() {
    return
        "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1009\n0\nENDSEC\n"
        "0\nSECTION\n2\nTABLES\n"
        "0\nTABLE\n2\nLAYER\n70\n2\n"
        "0\nLAYER\n2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n"
        "0\nLAYER\n2\nCAF\xC9\n70\n0\n62\n3\n6\nCONTINUOUS\n"
        "0\nENDTAB\n0\nENDSEC\n"
        "0\nSECTION\n2\nENTITIES\n"
        "0\nTEXT\n8\nCAF\xC9\n10\n0.0\n20\n0.0\n30\n0.0\n40\n2.5\n"
        "1\n\xE9t\xE9 108\xB0\n"
        "0\nENDSEC\n0\nEOF\n";
}

// An R2007 drawing, so UTF-8: layer "L" and TEXT "x", each followed by U+4E2D,
// which Windows-1252 cannot hold. The header names the codepage a pre-R2007
// save of it uses.
std::string r2007DrawingBeyondWindows1252() {
    return
        "0\nSECTION\n2\nHEADER\n"
        "9\n$ACADVER\n1\nAC1021\n"
        "9\n$DWGCODEPAGE\n3\nANSI_1252\n"
        "0\nENDSEC\n"
        "0\nSECTION\n2\nTABLES\n"
        "0\nTABLE\n2\nLAYER\n5\n2\n100\nAcDbSymbolTable\n70\n2\n"
        "0\nLAYER\n5\n10\n100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n"
        "2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n"
        "0\nLAYER\n5\n11\n100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n"
        "2\nL\xE4\xB8\xAD\n70\n0\n62\n3\n6\nCONTINUOUS\n"
        "0\nENDTAB\n"
        "0\nENDSEC\n"
        "0\nSECTION\n2\nENTITIES\n"
        "0\nTEXT\n5\n20\n100\nAcDbEntity\n8\nL\xE4\xB8\xAD\n"
        "100\nAcDbText\n10\n0.0\n20\n0.0\n30\n0.0\n40\n2.5\n"
        "1\nx\xE4\xB8\xAD\n100\nAcDbText\n"
        "0\nENDSEC\n0\nEOF\n";
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

// The text of the drawing's one TEXT entity.
QString textOf(RS_Graphic& graphic) {
    QString text;
    for (RS_Entity* e : graphic) {
        if (e != nullptr && e->rtti() == RS2::EntityText)
            text = static_cast<RS_Text*>(e)->getText();
    }
    return text;
}

} // namespace

TEST_CASE("a pre-R2007 DXF is Windows-1252 text unless it names another codepage",
          "[dxf][codec][pre2007]") {
    for (const char* acadver : kPre2007Versions) {
        DYNAMIC_SECTION(acadver << " declaring ANSI_1252") {
            DRW_TextCodec codec;
            useDxfHeader(codec, acadver, "ANSI_1252");
            checkWindows1252(codec);
            CHECK(codec.getCodePage() == "ANSI_1252");
        }
        DYNAMIC_SECTION(acadver << " declaring no codepage") {
            DRW_TextCodec codec;
            codec.setVersion(acadver, /*dxfFormat=*/true);
            checkWindows1252(codec);
            CHECK(codec.getCodePage() == "ANSI_1252");
        }
        DYNAMIC_SECTION(acadver << " declaring a codepage the reader does not know") {
            DRW_TextCodec codec;
            useDxfHeader(codec, acadver, "NO_SUCH_CODEPAGE");
            checkWindows1252(codec);
            CHECK(codec.getCodePage() == "ANSI_1252");
        }
    }

    SECTION("$DWGCODEPAGE ahead of $ACADVER") {
        DRW_TextCodec codec;
        codec.setCodePage("ANSI_1252", /*dxfFormat=*/true);
        codec.setVersion("AC1015", /*dxfFormat=*/true);
        checkWindows1252(codec);
    }
}

TEST_CASE("a pre-R2007 DXF keeps every Windows-1252 byte through a read and a write",
          "[dxf][codec][pre2007]") {
    DRW_TextCodec codec;
    useDxfHeader(codec, "AC1015", "ANSI_1252");
    for (int byte = 0x80; byte <= 0xFF; ++byte) {
        // Windows-1252 assigns no character to these five.
        if (byte == 0x81 || byte == 0x8D || byte == 0x8F || byte == 0x90 ||
            byte == 0x9D)
            continue;
        INFO("byte " << byte);
        const std::string raw(1, static_cast<char>(byte));
        const std::string utf8 = codec.toUtf8(raw);
        CHECK(utf8.size() > 1);
        CHECK(codec.fromUtf8(utf8) == raw);
    }
}

TEST_CASE("a pre-R2007 DXF escapes what Windows-1252 cannot hold",
          "[dxf][codec][pre2007]") {
    DRW_TextCodec codec;
    useDxfHeader(codec, "AC1015", "ANSI_1252");
    // U+00E9, U+4E2D, U+00B0
    const std::string utf8 = "\xC3\xA9\xE4\xB8\xAD\xC2\xB0";
    const std::string dxf = "\xE9\\U+4E2D\xB0";
    CHECK(codec.fromUtf8(utf8) == dxf);
    CHECK(codec.toUtf8(dxf) == utf8);
}

TEST_CASE("a pre-R2007 DXF declaring UTF-8 keeps its bytes",
          "[dxf][codec][pre2007]") {
    for (const char* acadver : kPre2007Versions) {
        DYNAMIC_SECTION(acadver) {
            DRW_TextCodec codec;
            useDxfHeader(codec, acadver, "UTF-8");
            CHECK(codec.toUtf8("\xC3\xA9") == "\xC3\xA9");
            CHECK(codec.fromUtf8("\xC3\xA9") == "\xC3\xA9");
            // The header reports the codepage a reader would fall back to.
            CHECK(codec.getCodePage() == "ANSI_1252");
        }
    }
}

TEST_CASE("a DXF that declares no version keeps its bytes",
          "[dxf][codec][pre2007]") {
    SECTION("no header") {
        DRW_TextCodec codec;
        CHECK(codec.toUtf8("\xC3\xA9") == "\xC3\xA9");
        CHECK(codec.toUtf8("\xB0") == "\xB0");
        CHECK(codec.fromUtf8("\xC3\xA9") == "\xC3\xA9");
    }
    SECTION("$DWGCODEPAGE alone") {
        DRW_TextCodec codec;
        codec.setCodePage("ANSI_1252", /*dxfFormat=*/true);
        CHECK(codec.toUtf8("\xC3\xA9") == "\xC3\xA9");
        CHECK(codec.toUtf8("\xB0") == "\xB0");
        CHECK(codec.fromUtf8("\xC3\xA9") == "\xC3\xA9");
    }
}

TEST_CASE("a pre-R2007 DXF in Windows-1252 imports its names and text",
          "[dxf][filter][pre2007]") {
    ensureSettings();

    SECTION("R2000 declaring ANSI_1252") {
        RS_Graphic graphic;
        REQUIRE(importText(graphic, "import_r2000", r2000Drawing()));
        CHECK(graphic.findLayer(kLayerName) != nullptr);
        CHECK(textOf(graphic) == kText);
    }
    SECTION("R12 declaring no codepage") {
        RS_Graphic graphic;
        REQUIRE(importText(graphic, "import_r12", r12Drawing()));
        CHECK(graphic.findLayer(kLayerName) != nullptr);
        CHECK(textOf(graphic) == kText);
    }
}

TEST_CASE("a drawing is saved as Windows-1252 in every pre-R2007 DXF version",
          "[dxf][filter][pre2007]") {
    ensureSettings();

    for (const auto& [format, acadver] :
         std::initializer_list<std::pair<RS2::FormatType, const char*>>{
             {RS2::FormatDXFRW12, "AC1009"},
             {RS2::FormatDXFRW14, "AC1014"},
             {RS2::FormatDXFRW2000, "AC1015"},
             {RS2::FormatDXFRW2004, "AC1018"}}) {
        DYNAMIC_SECTION(acadver) {
            RS_Graphic graphic;
            REQUIRE(importText(graphic, "save_src", r2000Drawing()));

            const std::string out = tempPath("save_out");
            {
                RS_FilterDXFRW filter;
                REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                                          format));
            }
            const std::string saved = readFile(out);

            // The header is what makes the text bytes the test.
            REQUIRE(saved.find(acadver) != std::string::npos);
            REQUIRE(saved.find("ANSI_1252") != std::string::npos);

            CHECK(saved.find("CAF\xC9") != std::string::npos);
            CHECK(saved.find("\xE9t\xE9 108") != std::string::npos);
            // The drawing has neither A with tilde (C3) nor A with circumflex
            // (C2), so either byte is the lead of a UTF-8 sequence.
            CHECK(saved.find('\xC3') == std::string::npos);
            CHECK(saved.find('\xC2') == std::string::npos);

            RS_Graphic reloaded;
            REQUIRE(importDrawing(reloaded, out));
            std::filesystem::remove(out);
            CHECK(reloaded.findLayer(kLayerName) != nullptr);
            CHECK(textOf(reloaded) == kText);
        }
    }
}

TEST_CASE("text beyond Windows-1252 survives a save as pre-R2007 DXF",
          "[dxf][filter][pre2007]") {
    ensureSettings();
    RS_Graphic graphic;
    REQUIRE(importText(graphic, "escape_src", r2007DrawingBeyondWindows1252()));

    const std::string out = tempPath("escape_out");
    {
        RS_FilterDXFRW filter;
        REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                                  RS2::FormatDXFRW2000));
    }
    const std::string saved = readFile(out);

    REQUIRE(saved.find("AC1015") != std::string::npos);
    REQUIRE(saved.find("ANSI_1252") != std::string::npos);
    CHECK(saved.find("L\\U+4E2D") != std::string::npos);
    CHECK(saved.find("x\\U+4E2D") != std::string::npos);
    CHECK(saved.find("\xE4\xB8\xAD") == std::string::npos);

    RS_Graphic reloaded;
    REQUIRE(importDrawing(reloaded, out));
    std::filesystem::remove(out);
    CHECK(reloaded.findLayer(QString::fromUtf8("L\xE4\xB8\xAD")) != nullptr);
    CHECK(textOf(reloaded) == QString::fromUtf8("x\xE4\xB8\xAD"));
}

TEST_CASE("a codepage codec writes bytes that are not UTF-8 as they are",
          "[dxf][codec][pre2007]") {
    // The DXF reader keeps some strings as the bytes of the file (extended
    // data, a multiline style name) and the writer encodes them like the rest.
    for (const char* codePage : {"ANSI_1250", "ANSI_1252", "ANSI_932", "ANSI_936"}) {
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

    for (const char* codePage : {"ANSI_1250", "ANSI_1252"}) {
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

TEST_CASE("legacy LibreCAD UTF-8 recovery is explicit and input-only in old DXF codecs",
          "[dxf][codec][pre2007][legacy-utf8]") {
    const std::string ambiguous = "\xC2\xB0";
    const std::string declared = "\xC3\x82\xC2\xB0"; // Windows-1252: U+00C2, U+00B0.
    for (const char* acadver : kPre2007Versions) {
        for (const bool codePageFirst : {false, true}) {
            DYNAMIC_SECTION(acadver << " codepage first=" << codePageFirst) {
                DRW_TextCodec normal;
                DRW_TextCodec recovered;
                recovered.setDxfTextEncoding(DRW::DxfTextEncoding::LegacyLibreCadUtf8);
                for (DRW_TextCodec* codec : {&normal, &recovered}) {
                    if (codePageFirst)
                        codec->setCodePage("ANSI_1252", true);
                    codec->setVersion(acadver, true);
                    if (!codePageFirst)
                        codec->setCodePage("ANSI_1252", true);
                }
                CHECK(normal.toUtf8(ambiguous) == declared);
                CHECK(recovered.toUtf8(ambiguous) == ambiguous);
                CHECK(recovered.toUtf8("\\U+4E2D") == "\xE4\xB8\xAD");
                CHECK(recovered.getCodePage() == "ANSI_1252");
                // Recovery does not choose an output encoding, even on this codec.
                CHECK(recovered.fromUtf8(ambiguous) == "\xB0");
                CHECK(recovered.fromUtf8("\xE4\xB8\xAD") == "\\U+4E2D");
                CHECK(recovered.fromUtf8(ambiguous) == normal.fromUtf8(ambiguous));
                recovered.setDxfTextEncoding(DRW::DxfTextEncoding::Declared);
                CHECK(recovered.toUtf8(ambiguous) == declared);
                CHECK(normal.toUtf8(ambiguous) == declared);
            }
        }
        DYNAMIC_SECTION(acadver << " default codepage") {
            DRW_TextCodec recovered;
            recovered.setDxfTextEncoding(DRW::DxfTextEncoding::LegacyLibreCadUtf8);
            recovered.setVersion(acadver, true);
            CHECK(recovered.toUtf8(ambiguous) == ambiguous);
            CHECK(recovered.fromUtf8(ambiguous) == "\xB0");
        }
    }
}

TEST_CASE("legacy UTF-8 choice does not override other codepages, unknown versions or DWG",
          "[dxf][codec][pre2007][legacy-utf8][guards]") {
    const std::string ambiguous = "\xC2\xB0";
    for (const char* codePage : {"ANSI_1250", "ANSI_932", "UTF-8"}) {
        for (const bool codePageFirst : {false, true}) {
            DYNAMIC_SECTION(codePage << " codepage first=" << codePageFirst) {
                DRW_TextCodec normal;
                DRW_TextCodec recovered;
                recovered.setDxfTextEncoding(DRW::DxfTextEncoding::LegacyLibreCadUtf8);
                for (DRW_TextCodec* codec : {&normal, &recovered}) {
                    if (codePageFirst)
                        codec->setCodePage(codePage, true);
                    codec->setVersion("AC1015", true);
                    if (!codePageFirst)
                        codec->setCodePage(codePage, true);
                }
                CHECK(recovered.toUtf8(ambiguous) == normal.toUtf8(ambiguous));
                CHECK(recovered.fromUtf8(ambiguous) == normal.fromUtf8(ambiguous));
                if (std::string(codePage) != "UTF-8")
                    CHECK(recovered.toUtf8(ambiguous) != ambiguous);
            }
        }
    }
    SECTION("no declared source version") {
        DRW_TextCodec codec;
        codec.setDxfTextEncoding(DRW::DxfTextEncoding::LegacyLibreCadUtf8);
        codec.setCodePage("ANSI_1252", true);
        CHECK_FALSE(codec.hasSourceVersion());
        CHECK(codec.toUtf8(ambiguous) == ambiguous);
        CHECK(codec.toUtf8("\xB0") == "\xB0");
    }
    SECTION("R2007 already uses UTF-8") {
        DRW_TextCodec normal;
        DRW_TextCodec recovered;
        recovered.setDxfTextEncoding(DRW::DxfTextEncoding::LegacyLibreCadUtf8);
        useDxfHeader(normal, "AC1021", "ANSI_1252");
        useDxfHeader(recovered, "AC1021", "ANSI_1252");
        CHECK(recovered.toUtf8(ambiguous) == ambiguous);
        CHECK(recovered.toUtf8(ambiguous) == normal.toUtf8(ambiguous));
        CHECK(recovered.fromUtf8(ambiguous) == normal.fromUtf8(ambiguous));
    }
    for (const char* acadver : {"AC1015", "AC1027"}) {
        DYNAMIC_SECTION("DWG " << acadver) {
            DRW_TextCodec normal;
            DRW_TextCodec recovered;
            recovered.setDxfTextEncoding(DRW::DxfTextEncoding::LegacyLibreCadUtf8);
            for (DRW_TextCodec* codec : {&normal, &recovered}) {
                codec->setVersion(acadver, false);
                codec->setByteCodePage("ANSI_1252");
            }
            CHECK(recovered.toUtf8(ambiguous) == normal.toUtf8(ambiguous));
            CHECK(recovered.fromUtf8(ambiguous) == normal.fromUtf8(ambiguous));
            CHECK(recovered.toUtf8CP8(ambiguous) == "\xC3\x82\xC2\xB0");
            CHECK(recovered.toUtf8CP8(ambiguous) == normal.toUtf8CP8(ambiguous));
        }
    }
}

TEST_CASE("legacy recovery follows the existing Windows-1252 aliases and unknown-codepage fallback",
          "[dxf][codec][pre2007][legacy-utf8][aliases]") {
    for (const char* codePage : {"CP1252", "latin1", "NO_SUCH_CODEPAGE"}) {
        for (const bool codePageFirst : {false, true}) {
            DYNAMIC_SECTION(codePage << " codepage first=" << codePageFirst) {
                DRW_TextCodec normal;
                DRW_TextCodec recovered;
                recovered.setDxfTextEncoding(DRW::DxfTextEncoding::LegacyLibreCadUtf8);
                for (DRW_TextCodec* codec : {&normal, &recovered}) {
                    if (codePageFirst)
                        codec->setCodePage(codePage, true);
                    codec->setVersion("AC1015", true);
                    if (!codePageFirst)
                        codec->setCodePage(codePage, true);
                }
                CHECK(normal.getCodePage() == "ANSI_1252");
                CHECK(recovered.getCodePage() == "ANSI_1252");
                CHECK(normal.toUtf8("\xC2\xB0") == "\xC3\x82\xC2\xB0");
                CHECK(recovered.toUtf8("\xC2\xB0") == "\xC2\xB0");
                CHECK(recovered.fromUtf8("\xC2\xB0") == "\xB0");
            }
        }
    }
}

namespace {

const std::string kLegacyRawXData = "RAW\xE9\xB0\\U+00E9";

std::vector<std::string> rawTextStrings(RS_Graphic& graphic) {
    std::vector<std::string> strings;
    for (RS_Entity* entity : graphic) {
        if (entity->rtti() != RS2::EntityText)
            continue;
        for (const auto& value : entity->getDrwExtData()) {
            if (value && value->code() == 1000 && value->type() == DRW_Variant::STRING && value->content.s)
                strings.push_back(*value->content.s);
        }
    }
    return strings;
}

// Typed names and text use UTF-8 under an ANSI_1252 declaration. The raw
// XDATA has invalid UTF-8 bytes and a literal DXF escape; neither is a typed
// text field. This does not promise lossless output for raw valid UTF-8 pairs.
std::string legacyUtf8Drawing(const char* acadver, const bool codePageFirst) {
    const std::string degree = "\xC2\xB0";
    const std::string layer = "L" + degree;
    const std::string style = "S" + degree;
    const std::string lineType = "T" + degree;
    const std::string block = "B" + degree;
    const auto text = [&](const std::string& handle, const std::string& value) {
        return "0\nTEXT\n5\n" + handle + "\n100\nAcDbEntity\n8\n" + layer +
               "\n6\n" + lineType + "\n100\nAcDbText\n10\n0\n20\n0\n30\n0\n40\n2.5\n1\n" +
               value + "\n7\n" + style + "\n100\nAcDbText\n73\n0\n";
    };
    const std::string version = std::string("9\n$ACADVER\n1\n") + acadver + "\n";
    const std::string codePage = "9\n$DWGCODEPAGE\n3\nANSI_1252\n";
    return "0\nSECTION\n2\nHEADER\n" +
        (codePageFirst ? codePage + version : version + codePage) + "0\nENDSEC\n"
        "0\nSECTION\n2\nTABLES\n"
        "0\nTABLE\n2\nLTYPE\n5\n2\n70\n2\n"
        "0\nLTYPE\n5\n3\n2\nCONTINUOUS\n70\n0\n3\nSolid\n72\n65\n73\n0\n40\n0\n"
        "0\nLTYPE\n5\n4\n2\n" + lineType + "\n70\n0\n3\nDash\n72\n65\n73\n2\n40\n1\n"
        "49\n0.5\n74\n0\n49\n-0.5\n74\n0\n0\nENDTAB\n"
        "0\nTABLE\n2\nLAYER\n5\n5\n70\n2\n"
        "0\nLAYER\n5\n6\n2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n"
        "0\nLAYER\n5\n7\n2\n" + layer + "\n70\n0\n62\n3\n6\n" + lineType + "\n0\nENDTAB\n"
        "0\nTABLE\n2\nSTYLE\n5\n8\n70\n1\n"
        "0\nSTYLE\n5\n9\n2\n" + style + "\n70\n0\n40\n0\n41\n1\n50\n0\n71\n0\n42\n1\n3\ntxt\n4\n\n0\nENDTAB\n"
        "0\nTABLE\n2\nAPPID\n5\nA\n70\n1\n0\nAPPID\n5\nB\n2\nMYAPP\n70\n0\n0\nENDTAB\n"
        "0\nENDSEC\n0\nSECTION\n2\nBLOCKS\n"
        "0\nBLOCK\n5\n10\n8\n0\n2\n" + block + "\n70\n0\n10\n0\n20\n0\n30\n0\n3\n" + block + "\n1\n\n" +
        text("11", "inside " + degree + " \xE4\xB8\xAD") +
        "0\nENDBLK\n5\n12\n8\n0\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"
        "0\nINSERT\n5\n13\n8\n" + layer + "\n2\n" + block + "\n10\n5\n20\n0\n30\n0\n" +
        text("14", "main " + degree + " \xE4\xB8\xAD") +
        "1001\nMYAPP\n1000\n" + kLegacyRawXData + "\n0\nENDSEC\n0\nEOF\n";
}

void checkRecoveredDrawing(RS_Graphic& graphic) {
    const QString degree = QString::fromUtf8("\xC2\xB0");
    const QString layerName = "L" + degree;
    const QString styleName = "S" + degree;
    const QString lineTypeName = "T" + degree;
    RS_Layer* layer = graphic.findLayer(layerName);
    REQUIRE(layer != nullptr);
    CHECK(layer->getPen().getLineTypeName() == lineTypeName);
    REQUIRE(graphic.dwgAdvancedMetadata().textStyleTableEntries().count(styleName.toUtf8().toStdString()) == 1);
    REQUIRE(graphic.dwgAdvancedMetadata().lineTypeTableEntries().count(lineTypeName.toUtf8().toStdString()) == 1);
    RS_Block* block = graphic.getBlockList()->find("B" + degree);
    REQUIRE(block != nullptr);
    int nestedTexts = 0;
    for (RS_Entity* entity : *block) {
        if (entity->rtti() == RS2::EntityText) {
            const auto* text = static_cast<RS_Text*>(entity);
            CHECK(text->getText() == "inside " + degree + QString::fromUtf8(" \xE4\xB8\xAD"));
            CHECK(text->getLayer() == layer);
            CHECK(text->getStyle() == styleName);
            ++nestedTexts;
        }
    }
    CHECK(nestedTexts == 1);
    int inserts = 0;
    int texts = 0;
    for (RS_Entity* entity : graphic) {
        if (entity->rtti() == RS2::EntityInsert) {
            CHECK(static_cast<RS_Insert*>(entity)->getName() == block->getName());
            CHECK(static_cast<RS_Insert*>(entity)->getBlockForInsert() == block);
            ++inserts;
        } else if (entity->rtti() == RS2::EntityText) {
            const auto* text = static_cast<RS_Text*>(entity);
            CHECK(text->getText() == "main " + degree + QString::fromUtf8(" \xE4\xB8\xAD"));
            CHECK(text->getLayer() == layer);
            CHECK(text->getStyle() == styleName);
            CHECK(text->getPen(false).getLineTypeName() == lineTypeName);
            ++texts;
        }
    }
    CHECK(inserts == 1);
    CHECK(texts == 1);
    CHECK(rawTextStrings(graphic) == std::vector<std::string>{kLegacyRawXData});
}

} // namespace

TEST_CASE("legacy LibreCAD DXF recovery fixes typed text and names without rewriting raw XDATA",
          "[dxf][filter][pre2007][legacy-utf8][roundtrip]") {
    ensureSettings();
    for (const char* acadver : kPre2007Versions) {
        for (const bool codePageFirst : {false, true}) {
            DYNAMIC_SECTION(acadver << " codepage first=" << codePageFirst) {
                const QTemporaryDir directory;
                REQUIRE(directory.isValid());
                const QString source = directory.filePath("legacy.dxf");
                const QString copy = directory.filePath("corrected-copy.dxf");
                const QByteArray bytes = QByteArray::fromStdString(legacyUtf8Drawing(acadver, codePageFirst));
                QFile input(source);
                REQUIRE(input.open(QIODevice::WriteOnly));
                REQUIRE(input.write(bytes) == bytes.size());
                input.close();

                RS_Graphic ordinary;
                ordinary.initForNewDocument();
                RS_FilterDXFRW normal;
                REQUIRE(normal.fileImport(ordinary, source, RS2::FormatDXFRW));
                CHECK(ordinary.findLayer(QString::fromUtf8("L\xC3\x82\xC2\xB0")) != nullptr);
                CHECK(textOf(ordinary) != QString::fromUtf8("main \xC2\xB0 \xE4\xB8\xAD"));
                CHECK(rawTextStrings(ordinary) == std::vector<std::string>{kLegacyRawXData});

                RS_Graphic recovered;
                recovered.initForNewDocument();
                RS_FilterDXFRW recovery;
                recovery.setDxfTextEncoding(RS2::DxfTextEncoding::LegacyLibreCadUtf8);
                REQUIRE(recovery.fileImport(recovered, source, RS2::FormatDXFRW));
                checkRecoveredDrawing(recovered);
                // The input policy on this very filter must not affect its output codec.
                REQUIRE(recovery.fileExport(recovered, copy, RS2::FormatDXFRW2000));
                QFile saved(copy);
                REQUIRE(saved.open(QIODevice::ReadOnly));
                QByteArray corrected = saved.readAll();
                corrected.replace("\r\n", "\n"); // Native Windows DXF line endings.
                CHECK(corrected.contains("ANSI_1252"));
                // TEXT's application exporter uses the standard %%D degree
                // escape; the layer-name oracle below checks the CP1252 byte.
                CHECK(corrected.contains("1\nmain %%D \\U+4E2D\n"));
                CHECK(corrected.contains("2\nL\xB0\n"));
                CHECK(corrected.contains(QByteArray::fromStdString("1000\n" + kLegacyRawXData + "\n")));

                RS_Graphic reloaded;
                reloaded.initForNewDocument();
                RS_FilterDXFRW declared;
                REQUIRE(declared.fileImport(reloaded, copy, RS2::FormatDXFRW));
                checkRecoveredDrawing(reloaded);
                REQUIRE(input.open(QIODevice::ReadOnly));
                CHECK(input.readAll() == bytes);
                input.close();
                RS_Graphic stillDeclared;
                stillDeclared.initForNewDocument();
                REQUIRE(declared.fileImport(stillDeclared, source, RS2::FormatDXFRW));
                CHECK(textOf(stillDeclared) != QString::fromUtf8("main \xC2\xB0 \xE4\xB8\xAD"));
            }
        }
    }
}

TEST_CASE("raw XDATA valid UTF-8 pairs are not decoded by either DXF input policy",
          "[dxf][filter][pre2007][legacy-utf8][raw-import]") {
    ensureSettings();
    const QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const QString source = directory.filePath("raw-pairs.dxf");
    QByteArray bytes = QByteArray::fromStdString(legacyUtf8Drawing("AC1015", false));
    bytes.replace(QByteArray::fromStdString(kLegacyRawXData), "AMB\xC2\xB0");
    QFile input(source);
    REQUIRE(input.open(QIODevice::WriteOnly));
    REQUIRE(input.write(bytes) == bytes.size());
    input.close();
    for (const auto encoding : {RS2::DxfTextEncoding::Declared, RS2::DxfTextEncoding::LegacyLibreCadUtf8}) {
        RS_Graphic graphic;
        graphic.initForNewDocument();
        RS_FilterDXFRW filter;
        filter.setDxfTextEncoding(encoding);
        REQUIRE(filter.fileImport(graphic, source, RS2::FormatDXFRW));
        CHECK(rawTextStrings(graphic) == std::vector<std::string>{"AMB\xC2\xB0"});
    }
    // Deliberately no raw-write oracle: valid UTF-8 pairs have pre-existing
    // output-codec ambiguity, outside this input-only recovery mode.
}
