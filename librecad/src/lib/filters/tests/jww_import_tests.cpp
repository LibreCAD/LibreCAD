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

#include <functional>
#include <map>
#include <memory>
#include <string>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QStringList>

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

/** @p data on layer @p layer of layer group @p gLayer, which LibreCAD names "G-L" in hex. */
template <typename Data>
Data onLayer(Data data, jwWORD gLayer, jwWORD layer) {
    data.m_nGLayer = gLayer;
    data.m_nLayer = layer;
    return data;
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

/** A dimension of 100 along y = -5, with the parts Ver.4.20 added. */
CDataSunpou makeDimension(jwDWORD version) {
    CDataSunpou dimension;
    setCommon(dimension, version);
    dimension.m_Sen = makeLine(version, {0.0, -5.0}, {100.0, -5.0});
    dimension.m_Moji = makeText(version, "100");
    dimension.m_bSxfMode = 2;
    dimension.m_SenHo1 = makeLine(version, {0.0, 0.0}, {0.0, -6.0});
    dimension.m_SenHo2 = makeLine(version, {100.0, 0.0}, {100.0, -6.0});
    dimension.m_Ten1 = makePoint(version, {0.0, -5.0});
    dimension.m_Ten2 = makePoint(version, {100.0, -5.0});
    dimension.m_TenHo1 = makePoint(version, {0.0, 0.0});
    dimension.m_TenHo2 = makePoint(version, {100.0, 0.0});
    return dimension;
}

using JwwWriter = std::function<void(JWWDocument&)>;

/**
 * Writes a JWW file of @p version in the temporary directory with jwwlib's
 * writer and returns its path: Save() writes the header and the records
 * @p drawing puts in the document, then @p tail, if given, writes on. The
 * name goes through QFile::encodeName(), as it does when
 * RS_FilterJWW::fileImport() reads the file.
 */
QString writeJwwFile(const QString& name, jwDWORD version, const JwwWriter& drawing,
                     const JwwWriter& tail = {}) {
    const QString path = QDir::temp().filePath(
        QStringLiteral("librecad_%1_%2").arg(QCoreApplication::applicationPid()).arg(name));
    QFile::remove(path);
    {
        std::string noInput;
        std::string out = QFile::encodeName(path).toStdString();
        JWWDocument doc(noInput, out); // closes the file when it goes
        doc.Header = JWWHead();        // no constructor of its own: every field zero
        doc.Header.JW_DATA_VERSION = version;
        doc.objCode = version;
        drawing(doc);
        REQUIRE(doc.Save());
        if (tail) {
            tail(doc);
        }
        REQUIRE(doc.ofs->good()); // Save() cannot tell a failed write
    }
    REQUIRE(QFileInfo(path).size() > 0);
    return path;
}

/** Writes a JWW file with a line and, if @p text is not empty, a text. */
QString writeJww(const QString& name, const std::string& text) {
    return writeJwwFile(name, 600, [&text](JWWDocument& doc) {
        doc.vSen.push_back(makeLine(600, {0.0, 0.0}, {10.0, 5.0}));
        if (!text.empty()) {
            doc.vMoji.push_back(makeText(600, text));
        }
    });
}

/**
 * Save() ends the file with the number of block definitions as a DWORD, 0.
 * Jw_cad writes MFC's count, a WORD: this writes over it that @p count
 * definitions follow.
 */
void beginBlockDefinitions(JWWDocument& doc, jwWORD count) {
    doc.ofs->seekp(-4, std::ios::end);
    *doc.ofs << count;
}

/** Writes a block definition and the number of members the caller writes next. */
void writeBlockDefinition(JWWDocument& doc, const CDataList& definition, jwWORD members) {
    REQUIRE(doc.SaveDataList(definition));
    *doc.ofs << members;
}

/** The size of the header jwwlib writes for a file of @p version. */
qint64 jwwHeaderSize(jwDWORD version) {
    const QString path = QDir::temp().filePath(
        QStringLiteral("librecad_%1_jww_header.jww").arg(QCoreApplication::applicationPid()));
    QFile::remove(path);
    {
        std::string noInput;
        std::string out = QFile::encodeName(path).toStdString();
        JWWDocument doc(noInput, out);
        doc.Header = JWWHead();
        doc.Header.JW_DATA_VERSION = version;
        doc.objCode = version;
        REQUIRE(doc.WriteHeader());
    }
    const qint64 size = QFileInfo(path).size();
    QFile::remove(path);
    return size;
}

/** Reads a JWW file with jwwlib's reader. */
std::unique_ptr<JWWDocument> readJwwFile(const QString& path) {
    std::string input = QFile::encodeName(path).toStdString();
    std::string noOutput;
    auto doc = std::make_unique<JWWDocument>(input, noOutput);
    REQUIRE(doc->Read());
    doc->ifs->close(); // the records are read: on Windows an open file cannot be removed
    return doc;
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

TEST_CASE("JWW block definitions stay out of the drawing", "[jww][import]") {
    // Each block definition (CDataList) is followed by the number of its
    // members, then the members. The reader took that number from a field
    // nothing set, so members were read into the drawing. The last member
    // ends the file, as in a Jw_cad 6 file, and refers to its class by
    // number: the reader also read it a second time at the end of the file.
    const jwDWORD version = 600;
    const QString path = writeJwwFile(
        QStringLiteral("jww_blocks.jww"), version,
        [&](JWWDocument& doc) {
            doc.vSen.push_back(makeLine(version, {0.0, 0.0}, {10.0, 5.0}));
            doc.vMoji.push_back(makeText(version, "MODEL"));
            doc.vBlock.push_back(makeInsert(version, 2, {50.0, 50.0}));
        },
        [&](JWWDocument& doc) {
            beginBlockDefinitions(doc, 2);
            writeBlockDefinition(doc, makeDefinition(version, 1, "BLK1"), 3);
            doc.SaveSen(makeLine(version, {100.0, 100.0}, {200.0, 200.0}));
            doc.SaveTen(makePoint(version, {150.0, 150.0})); // a class the drawing does not use
            doc.SaveSen(makeLine(version, {111.0, 111.0}, {222.0, 222.0}));
            writeBlockDefinition(doc, makeDefinition(version, 2, "BLK2"), 3);
            doc.SaveMoji(makeText(version, "INBLOCK"));
            doc.SaveBlock(makeInsert(version, 1, {5.0, 5.0}));
            doc.SaveSen(makeLine(version, {300.0, 300.0}, {400.0, 400.0}));
        });

    auto doc = readJwwFile(path);
    CHECK(doc->vSen.size() == 1);
    CHECK(doc->vTen.empty());
    REQUIRE(doc->vMoji.size() == 1);
    CHECK(doc->vMoji.front().m_string == "MODEL");
    REQUIRE(doc->vBlock.size() == 1);
    CHECK(doc->vBlock.front().m_n_Number == 2);

    JWWBlockList& blocks = *doc->pBlockList;
    CHECK(blocks.getBlockListCount() == 2);
    // looked up by the definition's number, which reads right only when the
    // definition is read by the file's version
    REQUIRE(blocks.GetDataListCount(1) == 3);
    REQUIRE(blocks.GetDataListCount(2) == 3);
    CHECK(blocks.GetBlockList(2).m_strName == "BLK2");
    CHECK(blocks.GetDataType(1, 0) == Sen);
    REQUIRE(blocks.GetDataType(1, 1) == Ten);
    CHECK(blocks.GetCDataTen(1, 1).m_start.x == 150.0);
    REQUIRE(blocks.GetDataType(1, 2) == Sen);
    CHECK(blocks.GetCDataSen(1, 2).m_start.x == 111.0);
    REQUIRE(blocks.GetDataType(2, 0) == Moji);
    CHECK(blocks.GetCDataMoji(2, 0).m_string == "INBLOCK");
    REQUIRE(blocks.GetDataType(2, 1) == Block);
    CHECK(blocks.GetCDataBlock(2, 1).m_n_Number == 1);
    REQUIRE(blocks.GetDataType(2, 2) == Sen);
    CHECK(blocks.GetCDataSen(2, 2).m_end.y == 400.0);
    doc.reset();

    // and none of the members reaches the drawing LibreCAD imports
    ensureSettings();
    RS_Graphic graphic;
    RS_FilterJWW filter;
    REQUIRE(filter.fileImport(graphic, path, RS2::FormatJWW));
    QFile::remove(path);
    int lines = 0;
    int texts = 0;
    for (const RS_Entity* e : graphic) {
        if (e->rtti() == RS2::EntityLine) {
            ++lines;
            const auto* line = static_cast<const RS_Line*>(e);
            CHECK(line->getStartpoint() == RS_Vector(0.0, 0.0));
            CHECK(line->getEndpoint() == RS_Vector(10.0, 5.0));
        }
        else if (e->rtti() == RS2::EntityMText) {
            ++texts;
        }
    }
    CHECK(lines == 1);
    CHECK(texts == 1);
}

TEST_CASE("Images a Jw_cad 7 file embeds are not read as JWW records", "[jww][import]") {
    // From Ver.7.00 the block definitions are followed by the number of
    // images and, for each, its name, its size and its bytes. The reader went
    // on reading those bytes as records. These bytes are a line record.
    const jwDWORD version = 700;
    const QString path = writeJwwFile(
        QStringLiteral("jww_image.jww"), version,
        [&](JWWDocument& doc) { doc.vSen.push_back(makeLine(version, {0.0, 0.0}, {10.0, 5.0})); },
        [&](JWWDocument& doc) {
            beginBlockDefinitions(doc, 0);
            *doc.ofs << (jwDWORD)1; // one image
            // 1 + 13 bytes, so the old reader, which read two bytes at a
            // time, met the record's class tag where it looked for one
            doc.WriteString("image1.bmp.gz");
            const std::streampos sizeAt = doc.ofs->tellp();
            *doc.ofs << (jwDWORD)0;
            const std::streampos start = doc.ofs->tellp();
            doc.SaveSen(makeLine(version, {8e42, 1e-167}, {-3e232, 4.0}));
            const std::streampos end = doc.ofs->tellp();
            doc.ofs->seekp(sizeAt);
            *doc.ofs << (jwDWORD)(end - start);
            doc.ofs->seekp(0, std::ios::end);
        });
    auto doc = readJwwFile(path);
    CHECK(QFile::remove(path));
    REQUIRE(doc->vSen.size() == 1);
    CHECK(doc->vSen.front().m_end.x == 10.0);
}

TEST_CASE("A JWW record is read once, and not when the file ends inside it", "[jww][import]") {
    const jwDWORD version = 600;
    const QString path = writeJwwFile(QStringLiteral("jww_cut.jww"), version, [&](JWWDocument& doc) {
        doc.vSen.push_back(makeLine(version, {0.0, 0.0}, {10.0, 5.0}));
        doc.vSen.push_back(makeLine(version, {1.0, 1.0}, {2.0, 2.0}));
        doc.vSen.push_back(makeLine(version, {3.0, 3.0}, {4.0, 4.0}));
    });
    const qint64 size = QFileInfo(path).size();

    // without the block definition count, the file ends with the last line,
    // whose tag refers to its class: it was read twice
    REQUIRE(QFile::resize(path, size - 4));
    {
        auto doc = readJwwFile(path);
        REQUIRE(doc->vSen.size() == 3);
        CHECK(doc->vSen.back().m_end.y == 4.0);
    }
    // cut inside the last line: it was read, with the end point of the one
    // before it
    REQUIRE(QFile::resize(path, size - 4 - 8));
    {
        auto doc = readJwwFile(path);
        CHECK(doc->vSen.size() == 2);
    }
    CHECK(QFile::remove(path));
}

TEST_CASE("JWW dimensions and block definitions are read by the file's version", "[jww][import]") {
    // The reader gave the version to the dimension but not to the lines,
    // text and points inside it, nor to block definitions: their pen width,
    // written from Ver.3.51, was read or skipped by whatever the memory held.
    for (const jwDWORD version : {230u, 252u, 300u, 351u, 420u, 600u, 700u}) {
        INFO("version " << version);
        const QString path = writeJwwFile(
            QStringLiteral("jww_v%1.jww").arg(version), version,
            [&](JWWDocument& doc) {
                doc.vSen.push_back(makeLine(version, {0.0, 0.0}, {10.0, 5.0}));
                doc.vEnko.push_back(makeArc(version));
                doc.vMoji.push_back(makeText(version, "OLD"));
                doc.vSunpou.push_back(makeDimension(version));
                doc.vSolid.push_back(makeSolid(version)); // after the dimension
            },
            [&](JWWDocument& doc) {
                beginBlockDefinitions(doc, 1);
                writeBlockDefinition(doc, makeDefinition(version, 3, "OLDBLK"), 1);
                doc.SaveEnko(makeArc(version));
                if (version >= 700) {
                    *doc.ofs << (jwDWORD)0; // no images
                }
            });
        auto doc = readJwwFile(path);
        CHECK(QFile::remove(path));

        CHECK(doc->vSen.size() == 1);
        CHECK(doc->vEnko.size() == 1);
        REQUIRE(doc->vMoji.size() == 1);
        CHECK(doc->vMoji.front().m_string == "OLD");
        REQUIRE(doc->vSunpou.size() == 1);
        const CDataSunpou& dimension = doc->vSunpou.front();
        CHECK(dimension.m_Sen.m_end.x == 100.0);
        CHECK(dimension.m_Moji.m_string == "100");
        if (version >= 420) {
            CHECK(dimension.m_bSxfMode == 2);
            CHECK(dimension.m_SenHo2.m_start.x == 100.0);
            CHECK(dimension.m_TenHo2.m_start.x == 100.0);
        }
        REQUIRE(doc->vSolid.size() == 1);
        CHECK(doc->vSolid.front().m_DPoint3.x == 30.0);
        REQUIRE(doc->pBlockList->GetDataListCount(3) == 1);
        CHECK(doc->pBlockList->GetCDataEnko(3, 0).m_dHankei == 3.0);
    }
}

TEST_CASE("JWW class tags past number 0x7F7F are read as MFC writes them", "[jww][import]") {
    // MFC's CArchive numbers classes and objects together. A class numbered
    // n below 0x7FFF is tagged 0x8000 | n, so class 0x7F7F is tagged 0xFF7F,
    // which the reader took for the 0x7FFF tag and read a DWORD after it. A
    // class numbered 0x7FFF or more is tagged 0x7FFF and a DWORD, and a count
    // of 0x8000 or more (as jwwlib writes it) is 0xFFFF and a DWORD.
    const jwDWORD version = 600;
    const QString path = writeJwwFile(QStringLiteral("jww_tags.jww"), version, [&](JWWDocument& doc) {
        // class CDataSen is 1 and its lines 2 to 32638, so CDataEnko is 0x7F7F
        for (int k = 0; k < 32637; ++k) {
            doc.vSen.push_back(makeLine(version, {double(k), 0.0}, {double(k), 1.0}));
        }
        doc.vEnko.push_back(makeArc(version));
        doc.vEnko.push_back(makeArc(version)); // tagged 0xFF7F
        // CDataTen is 32642 and its points 32643 to 32842, so CDataMoji is 32843
        for (int k = 0; k < 200; ++k) {
            doc.vTen.push_back(makePoint(version, {double(k), 2.0}));
        }
        doc.vMoji.push_back(makeText(version, "FIRST"));
        doc.vMoji.push_back(makeText(version, "SECOND")); // tagged 0x7FFF, 0x8000804B
    });
    auto doc = readJwwFile(path);
    CHECK(QFile::remove(path));
    CHECK(doc->vSen.size() == 32637);
    CHECK(doc->vEnko.size() == 2);
    REQUIRE(doc->vTen.size() == 200);
    CHECK(doc->vTen.back().m_start.x == 199.0);
    REQUIRE(doc->vMoji.size() == 2);
    CHECK(doc->vMoji.back().m_string == "SECOND");
}

TEST_CASE("A JWW file older than Ver.2.30 is refused", "[jww][import]") {
    ensureSettings();
    // ReadHeader() read the rest of the header only from Ver.2.30 on, but
    // returned true for an older version too, and the records were then read
    // from the middle of the header. The header of Ver.2.29 and older is
    // shorter: it is refused instead. Ver.2.31 to 2.99 have the header of
    // Ver.2.30, and the version test above reads a Ver.2.52 file.
    const QString path = writeJww(QStringLiteral("jww_import_v225.jww"), "");
    {
        QFile file(path);
        REQUIRE(file.open(QIODevice::ReadWrite));
        REQUIRE(file.seek(8)); // after "JwwData."
        const jwDWORD version = 225;
        REQUIRE(file.write(reinterpret_cast<const char*>(&version), sizeof version) == sizeof version);
    }
    RS_Graphic graphic;
    RS_FilterJWW filter;
    CHECK_FALSE(filter.fileImport(graphic, path, RS2::FormatJWW));
    CHECK(QFile::remove(path));
}

TEST_CASE("A JWW file that ends with its header is refused", "[jww][import]") {
    ensureSettings();
    // ReadHeader() read its fields without ever looking at the stream, so a
    // file cut off inside its header, by a partial download or copy, was read
    // as a drawing with no records and opened as an empty drawing. A file
    // that ends where the drawing begins (the last cut) has no drawing
    // either: a file with an empty drawing still has its count.
    const jwDWORD version = 600;
    const qint64 header = jwwHeaderSize(version);
    // a whole file each time: resizing one up again would fill it with zeros
    for (const qint64 cut : {qint64(8), qint64(12), header / 2, header - 1, header}) {
        INFO("cut at " << cut);
        const QString path = writeJww(QStringLiteral("jww_import_cut_header.jww"), "");
        REQUIRE(QFileInfo(path).size() > header);
        REQUIRE(QFile::resize(path, cut));
        RS_Graphic graphic;
        RS_FilterJWW filter;
        CHECK_FALSE(filter.fileImport(graphic, path, RS2::FormatJWW));
        CHECK(QFile::remove(path));
    }
}

TEST_CASE("JWW records are imported on the layer their numbers name", "[jww][import]") {
    ensureSettings();
    // DL_Jww gave every record the layer name in values[8], a DXF parser
    // buffer that nothing fills, so the entities landed on layer "0" (or on
    // a layer named after whatever that memory held) and the layers the file
    // names stayed empty.
    const QString path = writeJwwFile(QStringLiteral("jww_import_layers.jww"), 600, [](JWWDocument& doc) {
        doc.vSen.push_back(onLayer(makeLine(600, {0.0, 0.0}, {10.0, 5.0}), 0x0, 0x1));
        doc.vEnko.push_back(onLayer(makeArc(600, true), 0x2, 0xA));      // circle
        doc.vEnko.push_back(onLayer(makeArc(600), 0x2, 0xB));            // arc
        doc.vEnko.push_back(onLayer(makeArc(600, true, 0.5), 0x2, 0xC)); // ellipse
        doc.vTen.push_back(onLayer(makePoint(600, {1.0, 1.0}), 0xF, 0xF));
        doc.vMoji.push_back(onLayer(makeText(600, "T"), 0x3, 0x4));
    });
    RS_Graphic graphic;
    graphic.initForNewDocument(); // layer "0", as when LibreCAD opens a file
    RS_FilterJWW filter;
    REQUIRE(filter.fileImport(graphic, path, RS2::FormatJWW));
    QFile::remove(path);

    const std::map<RS2::EntityType, QString> expected{
        {RS2::EntityLine, QStringLiteral("0-1")},  {RS2::EntityCircle, QStringLiteral("2-A")},
        {RS2::EntityArc, QStringLiteral("2-B")},   {RS2::EntityEllipse, QStringLiteral("2-C")},
        {RS2::EntityPoint, QStringLiteral("F-F")}, {RS2::EntityMText, QStringLiteral("3-4")}};
    std::map<RS2::EntityType, int> found;
    for (const RS_Entity* e : graphic) {
        INFO("entity type " << e->rtti());
        const auto it = expected.find(e->rtti());
        REQUIRE(it != expected.end());
        ++found[e->rtti()];
        REQUIRE(e->getLayer() != nullptr);
        CHECK(e->getLayer()->getName().toStdString() == it->second.toStdString());
        CHECK(e->getLayer() == graphic.findLayer(it->second));
    }
    CHECK(found.size() == expected.size());
    for (const auto& typeCount : found) {
        CHECK(typeCount.second == 1);
    }

    // the layers the file names, and the new document's "0", which is empty
    QStringList layers;
    for (const RS_Layer* layer : *graphic.getLayerList()) {
        layers << layer->getName();
    }
    layers.sort();
    CHECK(layers == QStringList{"0", "0-1", "2-A", "2-B", "2-C", "3-4", "F-F"});
}

TEST_CASE("A JWW layer's pen does not come from the record imported before it", "[jww][import]") {
    ensureSettings();
    // Pen colour 2 is ByLayer. RS_FilterJWW::addLayer() gave the layer the
    // pen of the record imported before, so once the records are on their
    // layers the second line took the first line's colour.
    const QString path = writeJwwFile(QStringLiteral("jww_import_layer_pen.jww"), 600, [](JWWDocument& doc) {
        doc.vSen.push_back(makeLine(600, {0.0, 0.0}, {10.0, 5.0})); // pen colour 1
        CDataSen byLayer = makeLine(600, {0.0, 10.0}, {10.0, 15.0});
        byLayer.m_nPenColor = 2;
        doc.vSen.push_back(byLayer);
    });
    RS_Graphic graphic;
    RS_FilterJWW filter;
    REQUIRE(filter.fileImport(graphic, path, RS2::FormatJWW));
    QFile::remove(path);

    // the filter makes no layer "0" of its own: the file names only "0-1"
    CHECK(graphic.getLayerList()->count() == 1);
    const RS_Layer* layer = graphic.findLayer("0-1");
    REQUIRE(layer != nullptr);
    CHECK(layer->getPen() == RS_Layer("0-1").getPen());
    int byLayerLines = 0;
    for (const RS_Entity* e : graphic) {
        if (e->rtti() == RS2::EntityLine && e->getPen(false).getColor().isByLayer()) {
            ++byLayerLines;
            CHECK(e->getPen(true).getColor() == RS_Color(Qt::black));
        }
    }
    CHECK(byLayerLines == 1);
}

TEST_CASE("A JWW dimension's line and text are imported on its layers", "[jww][import]") {
    ensureSettings();
    const QString path = writeJwwFile(QStringLiteral("jww_import_dimension_layers.jww"), 600, [](JWWDocument& doc) {
        CDataSunpou dimension = onLayer(makeDimension(600), 0x5, 0x6);
        dimension.m_Sen = onLayer(dimension.m_Sen, 0x5, 0x6);
        dimension.m_Moji = onLayer(dimension.m_Moji, 0x5, 0x6);
        doc.vSunpou.push_back(dimension);
    });
    RS_Graphic graphic;
    RS_FilterJWW filter;
    REQUIRE(filter.fileImport(graphic, path, RS2::FormatJWW));
    QFile::remove(path);

    int lines = 0;
    int texts = 0;
    for (const RS_Entity* e : graphic) {
        REQUIRE(e->getLayer() != nullptr);
        CHECK(e->getLayer()->getName().toStdString() == "5-6");
        if (e->rtti() == RS2::EntityLine) {
            ++lines;
        } else if (e->rtti() == RS2::EntityMText) {
            ++texts;
            CHECK(static_cast<const RS_MText*>(e)->getText().toStdString() == "100");
        }
    }
    CHECK(lines == 1);
    CHECK(texts == 1);
}
