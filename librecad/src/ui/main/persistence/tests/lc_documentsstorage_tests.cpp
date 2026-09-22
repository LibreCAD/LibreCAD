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

// Saving drawings through LC_DocumentsStorage, the way QC_MDIWindow does:
// files are opened into a graphic that starts as a new window's does, and the
// tests check the bytes that Save and autosave write.
//
// Save on a drawing opened from a file that was not DXF used to write DXF over
// that file: a new window's graphic starts as FormatDXFRW and loading a file
// never changed it, so a DWG, JWW, Shapefile, LFF or CXF file was replaced by
// DXF under its own name, and autosave wrote DXF under the file's extension.
// Save now writes the format the file was read from, and asks for a new name
// when LibreCAD cannot write that format.

#include <catch2/catch_test_macros.hpp>

#include <QApplication>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include "jwwdoc.h"
#include "lc_documentsstorage.h"
#include "lc_filedialogservice.h"
#include "rs_fileio.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_line.h"
#include "rs_settings.h"

namespace {

void ensureApp() {
    // saveDocument() sets an override cursor, which needs a QGuiApplication.
    // rs_mtext_bidi_tests.cpp creates one before any test runs, so this is not
    // a QApplication, and a failed import (it shows a QMessageBox) aborts:
    // every file these tests open must import.
    static int argc = 1;
    static char arg0[] = "librecad_tests";
    static char* argv[] = {arg0, nullptr};
    static QCoreApplication* app = QCoreApplication::instance() != nullptr
        ? QCoreApplication::instance()
        : new QApplication(argc, argv);
    static bool ready = [] {
        QCoreApplication::setOrganizationName("LibreCAD");
        QCoreApplication::setApplicationName("LibreCAD-tests");
        RS_Settings::init("LibreCAD", "LibreCAD-tests");
        return true;
    }();
    (void)app;
    (void)ready;
}

/** Answers the Save As dialog in place of a user: with m_answer, or cancels it when m_answer is empty. */
class TestStorage : public LC_DocumentsStorage {
public:
    using LC_DocumentsStorage::loadGraphicFromTemplate;

    QString m_answer;
    RS2::FormatType m_answerType = RS2::FormatDXFRW;
    int m_asked = 0;
    QString m_askedName;
    RS2::FormatType m_askedType = RS2::FormatUnknown;

protected:
    LC_FileDialogService::FileDialogResult askSaveFileDetails(const QString& currentFileName,
                                                              const RS2::FormatType preferredType) override {
        ++m_asked;
        m_askedName = currentFileName;
        m_askedType = preferredType;
        LC_FileDialogService::FileDialogResult result;
        if (!m_answer.isEmpty()) {
            result.filePath = m_answer;
            result.fileType = m_answerType;
        }
        return result;
    }
};

QByteArray contents(const QString& path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

/** Opens @p path in @p graphic the way QC_MDIWindow opens a file in a new window. */
void open(LC_DocumentsStorage& storage, RS_Graphic& graphic, const QString& path,
          const RS2::FormatType type = RS2::FormatUnknown) {
    graphic.initForNewDocument();
    graphic.setFormatType(RS2::FormatDXFRW); // what a new window starts with
    REQUIRE(storage.loadDocument(&graphic, path, type));
}

void edit(RS_Graphic& graphic) {
    graphic.addEntity(new RS_Line(&graphic, RS_Vector(0.0, 0.0), RS_Vector(100.0, 50.0)));
    graphic.setModified(true);
}

bool hasEditedLine(const RS_Graphic& graphic) {
    for (const RS_Entity* e : graphic) {
        if (e->rtti() == RS2::EntityLine) {
            const auto* line = static_cast<const RS_Line*>(e);
            if (line->getStartpoint() == RS_Vector(0.0, 0.0) && line->getEndpoint() == RS_Vector(100.0, 50.0)) {
                return true;
            }
        }
    }
    return false;
}

bool save(LC_DocumentsStorage& storage, RS_Graphic& graphic, bool& cancelled) {
    return storage.saveDocument(&graphic, nullptr, cancelled);
}

bool isDxf(const QByteArray& bytes) {
    // LibreCAD's DXF starts with a 999 comment naming dxfrw
    return bytes.startsWith("999\ndxfrw ") || bytes.startsWith("999\r\ndxfrw ");
}

/** Copies a fixture to where a user's file would be; checked-out fixtures may be read-only. */
void copyFixture(const QString& from, const QString& to) {
    REQUIRE(QFile::copy(from, to));
    REQUIRE(QFile::setPermissions(to, QFile::ReadOwner | QFile::WriteOwner));
}

/** Copies a Shapefile (.shp, .shx, .dbf) to drawing.* in @p dir. */
void copyShapefile(const QTemporaryDir& dir) {
    const QString source = QString(LIBRECAD_SOURCE_DIR) + "/test_data/shp/polylines";
    for (const char* extension : {".shp", ".shx", ".dbf"}) {
        copyFixture(source + extension, dir.filePath(QString("drawing") + extension));
    }
}

/** Writes a JWW file holding one line with jwwlib's own writer. */
void writeJww(const QString& path) {
    std::string noInput;
    std::string output = path.toStdString();
    JWWDocument doc(noInput, output);
    doc.Header = JWWHead(); // no constructor of its own: every field zero
    doc.Header.JW_DATA_VERSION = 600;
    doc.objCode = 600;
    CDataSen line;
    line.SetVersion(600);
    line.m_lGroup = 0;
    line.m_nPenStyle = 1;
    line.m_nPenColor = 1;
    line.m_nPenWidth = 0;
    line.m_nLayer = 1;
    line.m_nGLayer = 0;
    line.m_sFlg = 0;
    line.m_start = {0.0, 0.0};
    line.m_end = {10.0, 5.0};
    doc.vSen.push_back(line);
    REQUIRE(doc.Save());
}

/** Writes a DWG drawing holding one line with LibreCAD's own writer. */
void writeDwg(const QString& path, const RS2::FormatType type) {
    RS_Graphic graphic;
    graphic.initForNewDocument();
    graphic.addEntity(new RS_Line(&graphic, RS_Vector(1.0, 2.0), RS_Vector(3.0, 4.0)));
    REQUIRE(RS_FileIO::instance()->fileExport(graphic, path, type));
}

/** Writes an empty DXF drawing to @p path. */
void writeDxf(const QString& path) {
    RS_Graphic graphic;
    graphic.initForNewDocument();
    REQUIRE(RS_FileIO::instance()->fileExport(graphic, path, RS2::FormatDXFRW));
}

/** Makes a directory read-only for as long as it lives. */
class ReadOnlyDirectory {
public:
    explicit ReadOnlyDirectory(const QString& path)
        : m_path(path), m_permissions(QFile::permissions(path)) {
        QFile::setPermissions(path, QFile::ReadOwner | QFile::ExeOwner);
    }
    ~ReadOnlyDirectory() {
        QFile::setPermissions(m_path, m_permissions);
    }
    ReadOnlyDirectory(const ReadOnlyDirectory&) = delete;
    ReadOnlyDirectory& operator=(const ReadOnlyDirectory&) = delete;

private:
    QString m_path;
    QFile::Permissions m_permissions;
};

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A drawing that failed to save stays modified", "[documentsstorage][save]") {
#ifdef Q_OS_WIN
    SKIP("the export filters check the directory's permissions on POSIX systems only");
#endif
    ensureApp();
    QTemporaryDir dir;
    const QString path = dir.filePath("drawing.dxf");
    writeDxf(path);

    LC_DocumentsStorage storage;
    RS_Graphic graphic;
    open(storage, graphic, path);
    edit(graphic);

    // the file itself stays writable; the DXF filter cannot write next to it
    const ReadOnlyDirectory readOnly(dir.path());
    if (QFileInfo(dir.path()).isWritable()) {
        SKIP("cannot make a directory read-only here (running as root?)");
    }
    const QByteArray original = contents(path);
    bool cancelled = false;
    CHECK_FALSE(storage.saveDocument(&graphic, nullptr, cancelled));
    CHECK(contents(path) == original);
    CHECK(graphic.isModified());
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A drawing opened from DWG is saved as DWG of the same version", "[documentsstorage][save]") {
    ensureApp();
    struct Case {
        RS2::FormatType m_type;
        const char* m_magic;
    };
    for (const Case& c : {Case{RS2::FormatDWG2004, "AC1018"}, Case{RS2::FormatDWG2007, "AC1021"},
                          Case{RS2::FormatDWG2010, "AC1024"}, Case{RS2::FormatDWG2013, "AC1027"},
                          Case{RS2::FormatDWG2018, "AC1032"}}) {
        DYNAMIC_SECTION(c.m_magic) {
            QTemporaryDir dir;
            const QString path = dir.filePath("drawing.dwg");
            writeDwg(path, c.m_type);
            const QByteArray original = contents(path);
            REQUIRE(original.startsWith(c.m_magic));

            TestStorage storage;
            RS_Graphic graphic;
            open(storage, graphic, path);
            CHECK(graphic.getFormatType() == c.m_type);

            edit(graphic);
            bool cancelled = false;
            REQUIRE(save(storage, graphic, cancelled));
            CHECK_FALSE(cancelled);
            CHECK(storage.m_asked == 0);
            CHECK_FALSE(graphic.isModified());
            CHECK(contents(path).startsWith(c.m_magic));
            CHECK(contents(path + "~") == original); // the backup

            RS_Graphic reopened;
            open(storage, reopened, path);
            CHECK(hasEditedLine(reopened));
        }
    }

    SECTION("AC1015") {
        QTemporaryDir dir;
        const QString path = dir.filePath("drawing.dwg");
        writeDwg(path, RS2::FormatDWG);
        const QByteArray original = contents(path);
        TestStorage storage;
        RS_Graphic graphic;
        open(storage, graphic, path, RS2::FormatDWG); // as the Open dialog's DWG filter does
        CHECK(graphic.getFormatType() == RS2::FormatDWG);

        edit(graphic);
        bool cancelled = false;
        if (save(storage, graphic, cancelled)) {
            CHECK(contents(path).startsWith("AC1015"));
        } else {
            // The DWG writer cannot write back a drawing read from R2000 yet,
            // not even its own output. A failed Save leaves the file alone and
            // the drawing modified; QC_ApplicationWindow then offers Save As.
            CHECK(contents(path) == original);
            CHECK(graphic.isModified());
        }
        CHECK(storage.m_asked == 0);
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A DWG version LibreCAD cannot write leaves no format to save in", "[documentsstorage][save]") {
    CHECK(RS_FilterDXFRW::formatForDwgVersion(DRW::AC1015) == RS2::FormatDWG);
    CHECK(RS_FilterDXFRW::formatForDwgVersion(DRW::AC1018) == RS2::FormatDWG2004);
    CHECK(RS_FilterDXFRW::formatForDwgVersion(DRW::AC1021) == RS2::FormatDWG2007);
    CHECK(RS_FilterDXFRW::formatForDwgVersion(DRW::AC1024) == RS2::FormatDWG2010);
    CHECK(RS_FilterDXFRW::formatForDwgVersion(DRW::AC1027) == RS2::FormatDWG2013);
    CHECK(RS_FilterDXFRW::formatForDwgVersion(DRW::AC1032) == RS2::FormatDWG2018);
    for (const DRW::Version version : {DRW::AC1014, DRW::AC1012, DRW::AC1009, DRW::AC1006, DRW::MC00,
                                       DRW::UNKNOWNV}) {
        CHECK(RS_FilterDXFRW::formatForDwgVersion(version) == RS2::FormatUnknown);
    }
    CHECK_FALSE(RS_FileIO::instance()->canExport(RS2::FormatUnknown));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Save asks for a new name for a drawing opened from a Shapefile", "[documentsstorage][save]") {
    ensureApp();
    QTemporaryDir dir;
    copyShapefile(dir);
    const QString path = dir.filePath("drawing.shp");
    const QByteArray original = contents(path);

    TestStorage storage;
    RS_Graphic graphic;
    open(storage, graphic, path);
    CHECK(graphic.getFormatType() == RS2::FormatSHP);
    edit(graphic);

    SECTION("cancelling the dialog leaves the file and the drawing as they were") {
        bool cancelled = false;
        CHECK(save(storage, graphic, cancelled));
        CHECK(cancelled);
        CHECK(storage.m_asked == 1);
        CHECK(storage.m_askedName == path); // the dialog starts at the file, offering DXF
        CHECK(storage.m_askedType == RS2::FormatDXFRW);
        CHECK(contents(path) == original);
        CHECK_FALSE(QFile::exists(path + "~"));
        CHECK(graphic.isModified());
        CHECK(graphic.getFilename() == path);
    }

    SECTION("the drawing is saved under the new name, and from then on in place") {
        const QString dxfPath = dir.filePath("drawing.dxf");
        storage.m_answer = dxfPath;
        bool cancelled = false;
        REQUIRE(save(storage, graphic, cancelled));
        CHECK_FALSE(cancelled);
        CHECK(storage.m_asked == 1);
        CHECK(contents(path) == original);
        CHECK(isDxf(contents(dxfPath)));
        CHECK(graphic.getFilename() == dxfPath);
        CHECK(graphic.getFormatType() == RS2::FormatDXFRW);
        CHECK_FALSE(graphic.isModified());

        graphic.setModified(true);
        REQUIRE(save(storage, graphic, cancelled));
        CHECK(storage.m_asked == 1);
        CHECK(QFile::exists(dxfPath + "~"));

        RS_Graphic reopened;
        open(storage, reopened, dxfPath);
        CHECK(hasEditedLine(reopened));
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Save asks for a new name for a drawing read from JWW", "[documentsstorage][save][jww]") {
    ensureApp();
    if (RS_FileIO::instance()->canExport(RS2::FormatJWW)) {
        SKIP("JWW is still offered as an export format, though nothing writes it");
    }
    QTemporaryDir dir;
    const QString path = dir.filePath("drawing.jww");
    writeJww(path);
    const QByteArray original = contents(path);

    // what loading a JWW file leaves; the JWW reader is not part of this test
    TestStorage storage;
    RS_Graphic graphic;
    graphic.initForNewDocument();
    graphic.setFormatType(RS2::FormatJWW);
    graphic.setFilename(path);
    graphic.markSaved(QFileInfo(path).lastModified());
    edit(graphic);

    bool cancelled = false;
    CHECK(save(storage, graphic, cancelled));
    CHECK(cancelled);
    CHECK(storage.m_asked == 1);
    CHECK(storage.m_askedType == RS2::FormatDXFRW);
    CHECK(contents(path) == original);
    CHECK(graphic.isModified());
}

TEST_CASE("JWW remains registered as an export format", "[documentsstorage][save][jww]") {
    ensureApp();
    CHECK(RS_FileIO::instance()->canExport(RS2::FormatJWW));
    CHECK(RS_FileIO::instance()->getExportFilter("drawing.jww", RS2::FormatJWW) != nullptr);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Save asks for a new name for a drawing opened as a QCad 1 file", "[documentsstorage][save]") {
    ensureApp();
    QTemporaryDir dir;
    const QString path = dir.filePath("drawing.dxf");
    writeDxf(path);
    const QByteArray original = contents(path);

    TestStorage storage;
    RS_Graphic graphic;
    open(storage, graphic, path, RS2::FormatDXF1);
    CHECK(graphic.getFormatType() == RS2::FormatDXF1);
    edit(graphic);
    bool cancelled = false;
    CHECK(save(storage, graphic, cancelled));
    CHECK(storage.m_asked == 1);
    CHECK(contents(path) == original);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A font opened from LFF or CXF is saved in its own format", "[documentsstorage][save]") {
    ensureApp();
    QTemporaryDir dir;
    TestStorage storage;

    SECTION("LFF") {
        const QString path = dir.filePath("font.lff");
        copyFixture(QString(LIBRECAD_SOURCE_DIR) + "/librecad/support/fonts/greekp.lff", path);
        RS_Graphic graphic;
        open(storage, graphic, path);
        CHECK(graphic.getFormatType() == RS2::FormatLFF);
        const unsigned letters = graphic.countBlocks();
        CHECK(letters > 90);
        graphic.setModified(true);
        bool cancelled = false;
        REQUIRE(save(storage, graphic, cancelled));
        CHECK(storage.m_asked == 0);
        CHECK(contents(path).startsWith("# Format:            LibreCAD Font 1\n"));

        RS_Graphic reopened;
        open(storage, reopened, path);
        CHECK(reopened.countBlocks() == letters);
    }

    SECTION("CXF") {
        const QString path = dir.filePath("font.cxf");
        {
            QFile file(path);
            REQUIRE(file.open(QIODevice::WriteOnly));
            file.write("# Format:            QCad II Font\n"
                       "# Name:              Test\n"
                       "\n"
                       "[0041] A\n"
                       "L 0.0,0.0,3.0,9.0\n"
                       "L 3.0,9.0,6.0,0.0\n"
                       "\n");
        }
        RS_Graphic graphic;
        open(storage, graphic, path);
        CHECK(graphic.getFormatType() == RS2::FormatCXF);
        const unsigned blocks = graphic.countBlocks();
        graphic.setModified(true);
        bool cancelled = false;
        REQUIRE(save(storage, graphic, cancelled));
        CHECK(storage.m_asked == 0);
        CHECK(contents(path).startsWith("# Format:            QCad II Font\n"));
        CHECK(contents(path).contains("[0041] A"));

        RS_Graphic reopened;
        open(storage, reopened, path);
        CHECK(reopened.countBlocks() == blocks);
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A drawing opened from DXF is saved as DXF", "[documentsstorage][save]") {
    ensureApp();
    QTemporaryDir dir;
    const QString path = dir.filePath("drawing.dxf");
    writeDxf(path);
    const QByteArray original = contents(path);

    TestStorage storage;
    RS_Graphic graphic;
    open(storage, graphic, path);
    CHECK(graphic.getFormatType() == RS2::FormatDXFRW);
    edit(graphic);
    bool cancelled = false;
    REQUIRE(save(storage, graphic, cancelled));
    CHECK(storage.m_asked == 0);
    CHECK(isDxf(contents(path)));
    CHECK(contents(path + "~") == original);

    RS_Graphic reopened;
    open(storage, reopened, path);
    CHECK(hasEditedLine(reopened));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Autosave writes DXF to a file named *.dxf", "[documentsstorage][autosave]") {
    ensureApp();
    QTemporaryDir dir;
    TestStorage storage;

    SECTION("a drawing opened from a Shapefile") {
        copyShapefile(dir);
        const QString path = dir.filePath("drawing.shp");
        const QByteArray original = contents(path);
        RS_Graphic graphic;
        open(storage, graphic, path);
        const QString autosaveName = graphic.getAutoSaveFileName();
        CHECK(QFileInfo(autosaveName).absolutePath() == QFileInfo(path).absolutePath());
        CHECK(autosaveName.endsWith("drawing.shp.dxf"));

        edit(graphic);
        QString written;
        REQUIRE(storage.autoSaveDocument(&graphic, nullptr, written));
        CHECK(written == autosaveName);
        CHECK(isDxf(contents(autosaveName)));
        CHECK(contents(path) == original);

        RS_Graphic recovered;
        open(storage, recovered, autosaveName);
        CHECK(hasEditedLine(recovered));

        // saving under a new name removes the autosave file
        storage.m_answer = dir.filePath("drawing.dxf");
        bool cancelled = false;
        REQUIRE(save(storage, graphic, cancelled));
        CHECK_FALSE(QFile::exists(autosaveName));
        CHECK(graphic.getAutoSaveFileName().endsWith("drawing.dxf"));
        CHECK_FALSE(graphic.getAutoSaveFileName().endsWith("drawing.dxf.dxf"));
    }

    SECTION("a drawing opened from DWG") {
        const QString path = dir.filePath("drawing.dwg");
        writeDwg(path, RS2::FormatDWG2004);
        const QByteArray original = contents(path);
        RS_Graphic graphic;
        open(storage, graphic, path);
        const QString autosaveName = graphic.getAutoSaveFileName();
        CHECK(autosaveName.endsWith("drawing.dwg.dxf"));

        edit(graphic);
        QString written;
        REQUIRE(storage.autoSaveDocument(&graphic, nullptr, written));
        CHECK(isDxf(contents(autosaveName)));
        CHECK(contents(path) == original);
    }

    SECTION("a drawing opened from DXF") {
        const QString path = dir.filePath("drawing.dxf");
        writeDxf(path);
        RS_Graphic graphic;
        open(storage, graphic, path);
        const QString autosaveName = graphic.getAutoSaveFileName();
        CHECK(autosaveName.endsWith("drawing.dxf"));
        CHECK(autosaveName != path);

        edit(graphic);
        QString written;
        REQUIRE(storage.autoSaveDocument(&graphic, nullptr, written));
        CHECK(isDxf(contents(autosaveName)));
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A new drawing from a DWG template is saved as DXF", "[documentsstorage][save]") {
    ensureApp();
    QTemporaryDir dir;
    const QString templatePath = dir.filePath("template.dwg");
    writeDwg(templatePath, RS2::FormatDWG2004);
    TestStorage storage;
    RS_Graphic graphic;
    graphic.setFormatType(RS2::FormatDXFRW); // what a new window starts with
    REQUIRE(storage.loadGraphicFromTemplate(&graphic, templatePath, RS2::FormatUnknown));
    CHECK(graphic.getFilename().isEmpty());
    CHECK(graphic.getFormatType() == RS2::FormatDXFRW);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Save still refuses a file changed on disk since it was opened", "[documentsstorage][save]") {
    ensureApp();
    QTemporaryDir dir;
    const QString path = dir.filePath("drawing.dwg");
    writeDwg(path, RS2::FormatDWG2004);

    TestStorage storage;
    RS_Graphic graphic;
    open(storage, graphic, path);
    {
        QFile file(path);
        REQUIRE(file.open(QIODevice::ReadWrite));
        REQUIRE(file.setFileTime(QFileInfo(path).lastModified().addSecs(60), QFileDevice::FileModificationTime));
    }
    const QByteArray changed = contents(path);
    edit(graphic);
    bool cancelled = false;
    CHECK_FALSE(save(storage, graphic, cancelled));
    CHECK(storage.m_asked == 0);
    CHECK(contents(path) == changed);
    CHECK(graphic.isModified());
}
