/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2026 LibreCAD.org

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

// JWW import hung on every file. RS_FilterJWW::toNativeString() decodes \U+XXXX
// and %%nnn escapes in loops that ran "while (!cap.isNull())"; the Qt 6 port
// dropped the line that assigned the match to cap, so cap stayed "" (empty but
// not null) and neither loop ever ended. Every entity passes its layer name
// through toNativeString(), so a file with a single line was enough.
//
// On the unfixed code these tests do not fail: they hang.

#include <catch2/catch_test_macros.hpp>

#include <string>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>

#include "jwwdoc.h"
#include "rs_filterjww.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_mtext.h"
#include "rs_settings.h"

namespace {

void ensureSettings() {
    static int argc = 1;
    static char arg0[] = "librecad_tests";
    static char* argv[] = {arg0, nullptr};
    static QCoreApplication* app =
        QCoreApplication::instance() ? QCoreApplication::instance() : new QCoreApplication(argc, argv);
    static bool ready = [] {
        QCoreApplication::setOrganizationName("LibreCAD");
        QCoreApplication::setApplicationName("LibreCAD-tests");
        RS_Settings::init("LibreCAD", "LibreCAD-tests");
        return true;
    }();
    (void)app;
    (void)ready;
}

/** Fills the attributes every JWW record carries: layer group 0, layer 1, pen 1. */
template <typename Data>
void setCommon(Data& data) {
    data.SetVersion(600);
    data.m_lGroup = 0;
    data.m_nPenStyle = 1;
    data.m_nPenColor = 1;
    data.m_nPenWidth = 0;
    data.m_nLayer = 1;
    data.m_nGLayer = 0;
    data.m_sFlg = 0;
}

/** Saves a JWW document to @p output: a line and, if @p text is not empty, a text. */
void saveJww(const std::string& output, const std::string& text) {
    std::string noInput;
    std::string out = output;
    JWWDocument doc(noInput, out); // closes the file when it goes
    doc.Header = JWWHead();        // no constructor of its own: every field zero
    doc.Header.JW_DATA_VERSION = 600;
    doc.objCode = 600;

    CDataSen line;
    setCommon(line);
    line.m_start = {0.0, 0.0};
    line.m_end = {10.0, 5.0};
    doc.vSen.push_back(line);

    if (!text.empty()) {
        CDataMoji moji;
        setCommon(moji);
        moji.m_start = {1.0, 2.0};
        moji.m_end = {6.0, 2.0};
        moji.m_nMojiShu = 1;
        moji.m_dSizeX = 2.5;
        moji.m_dSizeY = 2.5;
        moji.m_dKankaku = 0.0;
        moji.m_degKakudo = 0.0;
        moji.m_strFontName = "Arial";
        moji.m_string = text;
        doc.vMoji.push_back(moji);
    }
    REQUIRE(doc.Save());
}

/**
 * Writes a JWW file in the temporary directory and returns its path. The name
 * goes through QFile::encodeName(), as it does when RS_FilterJWW::fileImport()
 * reads the file.
 */
QString writeJww(const QString& name, const std::string& text) {
    const QString path = QDir::temp().filePath(
        QStringLiteral("librecad_%1_%2").arg(QCoreApplication::applicationPid()).arg(name));
    QFile::remove(path);
    saveJww(QFile::encodeName(path).toStdString(), text);
    REQUIRE(QFileInfo(path).size() > 0); // Save() cannot tell a failed write
    return path;
}

} // namespace

TEST_CASE("A JWW file with a line imports", "[jww][import]") {
    ensureSettings();
    const QString path = writeJww(QStringLiteral("jww_import_line.jww"), "");
    RS_Graphic graphic;
    RS_FilterJWW filter;
    REQUIRE(filter.fileImport(graphic, path, RS2::FormatJWW));
    QFile::remove(path);

    // the layer the file names, layer group 0 and layer 1, went through
    // toNativeString() on its way in
    CHECK(graphic.findLayer("0-1") != nullptr);
    int lines = 0;
    for (const RS_Entity* e : graphic) {
        if (e->rtti() == RS2::EntityLine) {
            ++lines;
            const auto* line = static_cast<const RS_Line*>(e);
            CHECK(line->getStartpoint() == RS_Vector(0.0, 0.0));
            CHECK(line->getEndpoint() == RS_Vector(10.0, 5.0));
        }
    }
    CHECK(lines == 1);
}

TEST_CASE("A JWW text's unicode and ASCII code escapes are decoded", "[jww][import]") {
    ensureSettings();
    // \U+00B0 is a degree sign, %%065 an A, and %%c a diameter sign; a code
    // that spells another escape once decoded is decoded in turn: \U+005C
    // before U+0041 makes \U+0041, and \U+0025 before %065 makes %%065.
    const QString path = writeJww(QStringLiteral("jww_import_text.jww"),
                                  "x\\U+00B0y %%065%%066 %%c \\U+005CU+0041 \\U+0025%065");
    RS_Graphic graphic;
    RS_FilterJWW filter;
    REQUIRE(filter.fileImport(graphic, path, RS2::FormatJWW));
    QFile::remove(path);

    QStringList texts;
    for (const RS_Entity* e : graphic) {
        if (e->rtti() == RS2::EntityMText) { // JWW text comes in as MText
            texts << static_cast<const RS_MText*>(e)->getText();
        }
    }
    REQUIRE(texts.size() == 1);
    CHECK(texts.front() == QString::fromUtf8("x\u00B0y AB \u2205 A A"));
}

TEST_CASE("A surrogate pair written as two unicode codes decodes to one character", "[jww][import]") {
    ensureSettings();
    RS_FilterJWW filter;
    // toDxfString() writes a character outside the BMP as one code per half:
    // U+20B9F, a kanji, as \U+D842\U+DF9F. Escapes after it are still decoded.
    CHECK(filter.toNativeString("a\\U+D842\\U+DF9Fb %%065 \\U+00B0", QStringLiteral("ANSI_1252")) ==
          QString::fromUcs4(U"a\U00020B9Fb A \u00B0"));
    // a half on its own stays as it is, and does not stop the decoding after it
    CHECK(filter.toNativeString("\\U+D800 %%065 \\U+00B0", QStringLiteral("ANSI_1252")) ==
          QString(QChar(0xD800)) + QString::fromUtf8(" A \u00B0"));
}
