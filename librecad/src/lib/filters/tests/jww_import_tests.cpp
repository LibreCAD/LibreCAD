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

#include <memory>
#include <string>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>

#ifdef Q_OS_UNIX
#include <fcntl.h>
#endif

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

/**
 * Fills the attributes every JWW record carries: layer group 0, layer 1,
 * pen 1, and the file version @p version, by which the record is written and
 * read.
 */
template <typename Data>
void setCommon(Data& data, jwDWORD version = 600) {
    data.SetVersion(version);
    data.m_lGroup = 0;
    data.m_nPenStyle = 1;
    data.m_nPenColor = 1;
    data.m_nPenWidth = 0;
    data.m_nLayer = 1;
    data.m_nGLayer = 0;
    data.m_sFlg = 0;
}

CDataSen makeLine(jwDWORD version, DPoint start, DPoint end) {
    CDataSen line;
    setCommon(line, version);
    line.m_start = start;
    line.m_end = end;
    return line;
}

CDataMoji makeText(jwDWORD version, const std::string& text) {
    CDataMoji moji;
    setCommon(moji, version);
    moji.m_start = {1.0, 2.0};
    moji.m_end = {6.0, 2.0};
    moji.m_nMojiShu = 1;
    moji.m_dSizeX = 2.5;
    moji.m_dSizeY = 2.5;
    moji.m_dKankaku = 0.0;
    moji.m_degKakudo = 0.0;
    moji.m_strFontName = "Arial";
    moji.m_string = text;
    return moji;
}

CDataTen makePoint(jwDWORD version, DPoint at) {
    CDataTen point;
    setCommon(point, version);
    point.m_start = at;
    point.m_bKariten = 0;
    point.m_nCode = 0; // a plain point: no code, angle or scale follows
    point.m_radKaitenKaku = 0.0;
    point.m_dBairitsu = 1.0;
    return point;
}

/**
 * An arc of 90 degrees, or with @p full a whole circle, of radius 3 about
 * (5, 5); an ellipse's if @p ratio is not 1.
 */
CDataEnko makeArc(jwDWORD version, bool full = false, double ratio = 1.0) {
    CDataEnko arc;
    setCommon(arc, version);
    arc.m_start = {5.0, 5.0};
    arc.m_dHankei = 3.0;
    arc.m_radKaishiKaku = 0.0;
    arc.m_radEnkoKaku = full ? 6.283185307179586 : 1.5707963267948966;
    arc.m_radKatamukiKaku = 0.0;
    arc.m_dHenpeiRitsu = ratio;
    arc.m_bZenEnFlg = full ? 1 : 0;
    return arc;
}

CDataSolid makeSolid(jwDWORD version) {
    CDataSolid solid;
    setCommon(solid, version); // pen color 1: no RGB value follows
    solid.m_start = {20.0, 20.0};
    solid.m_end = {20.0, 30.0};
    solid.m_DPoint2 = {30.0, 20.0};
    solid.m_DPoint3 = {30.0, 30.0};
    solid.m_Color = 0;
    return solid;
}

/** An insert of block definition @p number at @p at. */
CDataBlock makeInsert(jwDWORD version, jwDWORD number, DPoint at) {
    CDataBlock insert;
    setCommon(insert, version);
    insert.m_DPKijunTen = at;
    insert.m_dBairitsuX = 1.0;
    insert.m_dBairitsuY = 1.0;
    insert.m_radKaitenKaku = 0.0;
    insert.m_n_Number = number;
    return insert;
}

CDataList makeDefinition(jwDWORD version, jwDWORD number, const std::string& name) {
    CDataList definition;
    setCommon(definition, version);
    definition.m_nNumber = number;
    definition.m_bReffered = 1;
    definition.m_time = 0x5F5E1000;
    definition.m_strName = name;
    return definition;
}

/** Saves a JWW document to @p output: a line and, if @p text is not empty, a text. */
void saveJww(const std::string& output, const std::string& text) {
    std::string noInput;
    std::string out = output;
    JWWDocument doc(noInput, out); // closes the file when it goes
    doc.Header = JWWHead();        // no constructor of its own: every field zero
    doc.Header.JW_DATA_VERSION = 600;
    doc.objCode = 600;
    doc.vSen.push_back(makeLine(600, {0.0, 0.0}, {10.0, 5.0}));
    if (!text.empty()) {
        doc.vMoji.push_back(makeText(600, text));
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

#ifdef Q_OS_UNIX
/** How many file descriptors below 1024 this process has open. */
int openDescriptors() {
    int count = 0;
    for (int fd = 0; fd < 1024; ++fd) {
        if (fcntl(fd, F_GETFD) != -1) {
            ++count;
        }
    }
    return count;
}
#endif

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

TEST_CASE("A file that is not a JWW file is refused and closed", "[jww][import]") {
    ensureSettings();
    const QString path = QDir::temp().filePath(
        QStringLiteral("librecad_%1_jww_import_not_jww.jww").arg(QCoreApplication::applicationPid()));
    {
        QFile file(path);
        REQUIRE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        REQUIRE(file.write("not a Jw_cad drawing\n") > 0);
    }
    const auto import = [&path] {
        RS_Graphic graphic;
        RS_FilterJWW filter;
        return filter.fileImport(graphic, path, RS2::FormatJWW);
    };
    CHECK_FALSE(import()); // the first import may set up what stays open
#ifdef Q_OS_UNIX
    // DL_Jww::in() did not delete the JWWDocument when Read() failed, and
    // the document's ifstream kept the file open
    const int before = openDescriptors();
    CHECK_FALSE(import());
    CHECK(openDescriptors() == before);
#endif
    CHECK(QFile::remove(path)); // on Windows an open stream keeps the file
}

TEST_CASE("A JWW block list deletes its records as the types they were made as", "[jww][import]") {
    // JWWBlockList keeps each block definition (CDataList) and its members
    // (CDataSen, CDataMoji, ...). It deleted the definitions as CDataBlock,
    // deleted the members as CDataList in Init(), and never deleted them in
    // its destructor. CData has no virtual destructor, so each record has to
    // be deleted as its own type. A plain build can only check that this
    // runs; AddressSanitizer and leak checkers see the difference.
    auto blocks = std::make_unique<JWWBlockList>();
    const auto addBlock = [&blocks] {
        CDataList block = makeDefinition(600, 1, std::string(100, 'B')); // too long for a short string
        block.Count = 7;
        blocks->AddBlockList(block);
        CDataSen line = makeLine(600, {0.0, 0.0}, {10.0, 5.0});
        blocks->AddDataListSen(line);
        CDataEnko arc = makeArc(600);
        blocks->AddDataListEnko(arc);
        CDataTen point = makePoint(600, {1.0, 1.0});
        blocks->AddDataListTen(point);
        CDataMoji text = makeText(600, std::string(100, 'x'));
        blocks->AddDataListMoji(text);
        CDataSolid solid = makeSolid(600);
        blocks->AddDataListSolid(solid);
        CDataSunpou dimension{};
        setCommon(dimension);
        dimension.m_Moji = makeText(600, std::string(100, 'd'));
        blocks->AddDataListSunpou(dimension);
        CDataBlock insert = makeInsert(600, 1, {0.0, 0.0});
        blocks->AddDataListBlock(insert);
    };
    addBlock();
    REQUIRE(blocks->getBlockListCount() == 1);
    REQUIRE(blocks->GetDataListCount(1) == 7);
    CHECK(blocks->GetBlockList(1).m_strName == std::string(100, 'B'));
    CHECK(blocks->GetCDataMoji(1, 3).m_string == std::string(100, 'x'));
    CHECK(blocks->GetCDataSunpou(1, 5).m_Moji.m_string == std::string(100, 'd'));

    blocks->Init(); // JWWDocument::Read() starts with this
    CHECK(blocks->getBlockListCount() == 0);

    addBlock();
    blocks.reset();
}
