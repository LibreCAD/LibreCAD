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

#include <filesystem>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QGridLayout>
#include <QTemporaryDir>
#include <QTranslator>

#include "jwwdoc.h"
#include "lc_actiontestsupport.h"
#include "lc_documentsstorage.h"
#include "lc_filedialogservice.h"
#include "lc_settingguard.h"
#include "qg_dlgoptionsgeneral.h"
#include "rs_fileio.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_line.h"
#include "rs_settings.h"
#include "rs_text.h"

namespace {

using lc::test::SettingGuard;

void ensureApp() {
    // saveDocument() sets an override cursor, which needs a QGuiApplication.
    // rs_mtext_bidi_tests.cpp creates one before any test runs, so this is not
    // a QApplication, and a failed import (it shows a QMessageBox) aborts:
    // Ordinary opens must import. Intentional partial failures below call the
    // native filter or supply an error callback, avoiding that default dialog.
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

/**
 * Answers the Save As dialog in place of a user: with m_answer, or cancels it when m_answer is empty.
 * Pins the backup settings, which other tests of the process change.
 */
class TestStorage : public LC_DocumentsStorage {
public:
    using LC_DocumentsStorage::loadGraphicFromTemplate;
    using LC_DocumentsStorage::backupDrawingFile;
    using LC_DocumentsStorage::createAutoSaveFileName;

    TestStorage() {
        m_backup.set(true);
        m_backupSuffix.set(QStringLiteral("~"));
        m_hideBackups.set(false);
        m_autosavePrefix.set(QStringLiteral("#"));
    }

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

private:
    SettingGuard m_backup{RS_SETTINGS, "Defaults", "AutoBackupDocument"};
    SettingGuard m_backupSuffix{RS_SETTINGS, "Defaults", "BackupFileSuffix"};
    SettingGuard m_hideBackups{RS_SETTINGS, "Defaults", "HideBackupFiles"};
    SettingGuard m_autosavePrefix{RS_SETTINGS, "Defaults", "AutosaveFilePrefix"};
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

void writeLegacyUtf8Dxf(const QString& path) {
    const QByteArray drawing(
        "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1015\n"
        "9\n$DWGCODEPAGE\n3\nANSI_1252\n0\nENDSEC\n"
        "0\nSECTION\n2\nENTITIES\n0\nTEXT\n5\n20\n100\nAcDbEntity\n8\n0\n100\nAcDbText\n"
        "10\n0\n20\n0\n40\n2.5\n1\n45\xC2\xB0\n"
        "0\nENDSEC\n0\nEOF\n");
    QFile file(path);
    REQUIRE(file.open(QIODevice::WriteOnly));
    REQUIRE(file.write(drawing) == drawing.size());
}

QString soleText(const RS_Graphic& graphic) {
    for (const auto* entity : graphic) {
        if (entity->rtti() == RS2::EntityText)
            return static_cast<const RS_Text*>(entity)->getText();
    }
    return {};
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

TEST_CASE("Legacy DXF recovery saves a corrected copy and protects its original",
          "[documentsstorage][save][legacyutf8]") {
    ensureApp();
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    const QString source = dir.filePath("legacy.dxf");
    const QString copy = dir.filePath("corrected.dxf");
    writeLegacyUtf8Dxf(source);
    const auto original = contents(source);
    TestStorage storage;
    RS_Graphic graphic;
    REQUIRE(storage.loadDocument(&graphic, source, RS2::FormatUnknown,
                                  RS2::DxfTextEncoding::LegacyLibreCadUtf8));
    CHECK(soleText(graphic) == QString::fromUtf8("45\xC2\xB0"));
    CHECK(graphic.isModified());
    CHECK(graphic.isDxfRecoverySourceFile(source));
    CHECK(graphic.getFilename() == source);

    storage.m_answer = copy;
    storage.m_answerType = RS2::FormatDXFRW2000;
    bool cancelled = false;
    REQUIRE(save(storage, graphic, cancelled));
    CHECK_FALSE(cancelled);
    CHECK(storage.m_asked == 1);
    CHECK(storage.m_askedName == dir.filePath("legacy_recovered.dxf"));
    CHECK(graphic.getFilename() == copy);
    CHECK_FALSE(graphic.isModified());
    CHECK(contents(source) == original);
    CHECK(contents(copy).contains("45%%D")); // TEXT uses the standard DXF degree escape.
    CHECK_FALSE(contents(copy).contains("45\xC2\xB0"));
    CHECK_FALSE(QFile::exists(source + "~"));

    RS_Graphic reopened;
    open(storage, reopened, copy);
    CHECK(soleText(reopened) == QString::fromUtf8("45\xC2\xB0"));
    CHECK(reopened.dxfRecoverySourceFile().isEmpty());
    CHECK_FALSE(reopened.isModified());

    // Protection lasts for the recovery document, not just its first Save As.
    edit(graphic);
    REQUIRE(save(storage, graphic, cancelled));
    CHECK(storage.m_asked == 1);
    storage.m_answer = source;
    CHECK_FALSE(storage.saveDocumentAs(&graphic, nullptr, cancelled));
    CHECK_FALSE(cancelled);
    CHECK(graphic.getFilename() == copy);
    CHECK(graphic.isDxfRecoverySourceFile(source));
    CHECK(contents(source) == original);
    CHECK_FALSE(QFile::exists(source + "~"));
}

TEST_CASE("Cancelled and failed recovery saves preserve source and session state",
          "[documentsstorage][save][legacyutf8]") {
    ensureApp();
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    const QString source = dir.filePath("legacy.dxf");
    writeLegacyUtf8Dxf(source);
    const auto original = contents(source);
    TestStorage storage;
    RS_Graphic graphic;
    REQUIRE(storage.loadDocument(&graphic, source, RS2::FormatUnknown,
                                  RS2::DxfTextEncoding::LegacyLibreCadUtf8));
    const auto autosave = graphic.getAutoSaveFileName();
    bool cancelled = false;
    CHECK(save(storage, graphic, cancelled));
    CHECK(cancelled);
    CHECK(graphic.isModified());
    CHECK(contents(source) == original);

    storage.m_answer = dir.filePath("missing/copy.dxf");
    CHECK_FALSE(save(storage, graphic, cancelled));
    CHECK_FALSE(cancelled);
    CHECK(graphic.getFilename() == source);
    CHECK(graphic.getAutoSaveFileName() == autosave);
    CHECK(graphic.getFormatType() == RS2::FormatDXFRW2000);
    CHECK(graphic.isModified());
    CHECK(graphic.isDxfRecoverySourceFile(source));
    CHECK(contents(source) == original);

    // A subsequent ordinary open must not inherit either policy or protection.
    open(storage, graphic, source);
    CHECK(soleText(graphic) == QString::fromUtf8("45\xC3\x82\xC2\xB0"));
    CHECK(graphic.dxfRecoverySourceFile().isEmpty());
    CHECK_FALSE(graphic.isModified());
}

TEST_CASE("Recovery export refuses original paths and filesystem aliases",
          "[documentsstorage][save][legacyutf8]") {
    ensureApp();
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    const QString source = dir.filePath("legacy.dxf");
    writeLegacyUtf8Dxf(source);
    const auto original = contents(source);
    TestStorage storage;
    RS_Graphic graphic;
    REQUIRE(storage.loadDocument(&graphic, source, RS2::FormatUnknown,
                                  RS2::DxfTextEncoding::LegacyLibreCadUtf8));

    QString target = source;
    SECTION("same file") {}
    SECTION("relative path components") { target = dir.filePath("./legacy.dxf"); }
    SECTION("symbolic link") {
        target = dir.filePath("alias.dxf");
        std::error_code error;
        std::filesystem::create_symlink(std::filesystem::u8path(source.toStdString()),
                                        std::filesystem::u8path(target.toStdString()), error);
        if (error)
            SKIP("filesystem does not permit symbolic links");
    }
    SECTION("hard link") {
        target = dir.filePath("alias.dxf");
        std::error_code error;
        std::filesystem::create_hard_link(std::filesystem::u8path(source.toStdString()),
                                          std::filesystem::u8path(target.toStdString()), error);
        if (error)
            SKIP("filesystem does not permit hard links");
    }
    CHECK(graphic.isDxfRecoverySourceFile(target));
    CHECK_FALSE(RS_FileIO::instance()->fileExport(graphic, target, RS2::FormatDXFRW2000));
    RS_FilterDXFRW direct;
    CHECK_FALSE(direct.fileExport(graphic, target, RS2::FormatDXFRW2000));
    storage.m_answer = target;
    bool cancelled = false;
    CHECK_FALSE(storage.saveDocumentAs(&graphic, nullptr, cancelled));
    CHECK(graphic.getFilename() == source);
    CHECK(graphic.isModified());
    CHECK(contents(source) == original);
    CHECK(contents(target) == original);
    CHECK_FALSE(QFile::exists(source + "~"));
}

TEST_CASE("Recovery Save As never cleans up or autosaves over a prefix-named original",
          "[documentsstorage][save][legacyutf8]") {
    ensureApp();
    for (bool hideBackups : {false, true}) {
        for (const QString& sourceName : {QStringLiteral("#copy.dxf"), QStringLiteral(".#copy.dxf")}) {
            CAPTURE(hideBackups, sourceName.toStdString());
            QTemporaryDir dir;
            REQUIRE(dir.isValid());
            TestStorage storage;
            SettingGuard hidden{RS_SETTINGS, "Defaults", "HideBackupFiles"};
            hidden.set(hideBackups);
            const QString source = dir.filePath(sourceName);
            const QString copy = dir.filePath("copy.dxf");
            writeLegacyUtf8Dxf(source);
            const QByteArray original = contents(source);
            REQUIRE_FALSE(original.isEmpty());
            RS_Graphic graphic;
            REQUIRE(storage.loadDocument(&graphic, source, RS2::FormatUnknown,
                                          RS2::DxfTextEncoding::LegacyLibreCadUtf8));
#ifdef Q_OS_WIN
            const QString preferredAutosave = dir.filePath("#copy.dxf");
#else
            const QString preferredAutosave = dir.filePath(hideBackups ? ".#copy.dxf" : "#copy.dxf");
#endif
            // This literal expectation is independent of the new collision loop.
            REQUIRE(storage.createAutoSaveFileName(QFileInfo(copy)) == preferredAutosave);
            storage.m_answer = copy;
            storage.m_answerType = RS2::FormatDXFRW2000;
            bool cancelled = false;
            REQUIRE(save(storage, graphic, cancelled));
            CHECK_FALSE(cancelled);
            CHECK(graphic.getFilename() == copy);
            CHECK_FALSE(graphic.isModified());
            CHECK(contents(source) == original);
            CHECK(contents(copy).contains("45%%D"));
            const QString autosave = graphic.getAutoSaveFileName();
            REQUIRE_FALSE(autosave.isEmpty());
            CHECK_FALSE(graphic.isDxfRecoverySourceFile(autosave));
            if (preferredAutosave == source)
                CHECK(autosave != preferredAutosave);
            else
                CHECK(autosave == preferredAutosave);

            // Subsequent ordinary autosave must use the safe alternate, not
            // merely avoid removing the source during the first Save As.
            edit(graphic);
            QString writtenAutosave;
            REQUIRE(storage.autoSaveDocument(&graphic, nullptr, writtenAutosave));
            CHECK(writtenAutosave == autosave);
            CHECK(isDxf(contents(autosave)));
            CHECK(contents(source) == original);
            RS_Graphic recoveredAutosave;
            open(storage, recoveredAutosave, autosave);
            CHECK(hasEditedLine(recoveredAutosave));
            REQUIRE(save(storage, graphic, cancelled));
            CHECK_FALSE(QFile::exists(autosave));
            CHECK(contents(source) == original);
        }
    }
}

TEST_CASE("Recovery Save As retains an old autosave path that aliases its original",
          "[documentsstorage][save][legacyutf8]") {
    ensureApp();
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    TestStorage storage;
    const QString source = dir.filePath("legacy.dxf");
    const QString copy = dir.filePath("corrected.dxf");
    writeLegacyUtf8Dxf(source);
    const QByteArray original = contents(source);
    RS_Graphic graphic;
    REQUIRE(storage.loadDocument(&graphic, source, RS2::FormatUnknown,
                                  RS2::DxfTextEncoding::LegacyLibreCadUtf8));
    QString oldAutosave = source;
    SECTION("literal source path") {}
    SECTION("symbolic alias") {
        oldAutosave = dir.filePath("old-autosave.dxf");
        std::error_code error;
        std::filesystem::create_symlink(std::filesystem::u8path(source.toStdString()),
                                        std::filesystem::u8path(oldAutosave.toStdString()), error);
        if (error)
            SKIP("filesystem does not permit symbolic links");
    }
    SECTION("hard-link alias") {
        oldAutosave = dir.filePath("old-autosave.dxf");
        std::error_code error;
        std::filesystem::create_hard_link(std::filesystem::u8path(source.toStdString()),
                                          std::filesystem::u8path(oldAutosave.toStdString()), error);
        if (error)
            SKIP("filesystem does not permit hard links");
    }
    REQUIRE(graphic.isDxfRecoverySourceFile(oldAutosave));
    graphic.setAutosaveFileName(oldAutosave);
    storage.m_answer = copy;
    storage.m_answerType = RS2::FormatDXFRW2000;
    bool cancelled = false;
    REQUIRE(storage.saveDocumentAs(&graphic, nullptr, cancelled));
    CHECK_FALSE(cancelled);
    CHECK(contents(source) == original);
    CHECK(contents(oldAutosave) == original);
    CHECK(graphic.getFilename() == copy);
    CHECK_FALSE(graphic.isDxfRecoverySourceFile(graphic.getAutoSaveFileName()));
    edit(graphic);
    QString autosave;
    REQUIRE(storage.autoSaveDocument(&graphic, nullptr, autosave));
    CHECK(isDxf(contents(autosave)));
    REQUIRE(save(storage, graphic, cancelled));
    CHECK_FALSE(QFile::exists(autosave));
    CHECK(contents(source) == original);
    CHECK(contents(oldAutosave) == original);
}

TEST_CASE("Recovery backup destinations cannot replace the original or its aliases",
          "[documentsstorage][backup][legacyutf8]") {
    ensureApp();
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    TestStorage storage;
    SettingGuard suffixSetting{RS_SETTINGS, "Defaults", "BackupFileSuffix"};
    const QString suffix = QStringLiteral(".original.dxf");
    suffixSetting.set(suffix);
    const QString copy = dir.filePath("copy.dxf");
    const QString backup = copy + suffix;
    QString source;
    enum class Alias { None, Symbolic, HardLink };
    Alias alias = Alias::None;
    SECTION("literal custom-suffix source") { source = backup; }
    SECTION("symbolic backup alias") { source = dir.filePath("legacy.dxf"); alias = Alias::Symbolic; }
    SECTION("hard-link backup alias") { source = dir.filePath("legacy.dxf"); alias = Alias::HardLink; }
    REQUIRE_FALSE(source.isEmpty());
    writeLegacyUtf8Dxf(source);
    const QByteArray original = contents(source);
    writeDxf(copy);
    const QByteArray priorCopy = contents(copy);
    if (alias != Alias::None) {
        std::error_code error;
        if (alias == Alias::Symbolic)
            std::filesystem::create_symlink(std::filesystem::u8path(source.toStdString()),
                                            std::filesystem::u8path(backup.toStdString()), error);
        else
            std::filesystem::create_hard_link(std::filesystem::u8path(source.toStdString()),
                                              std::filesystem::u8path(backup.toStdString()), error);
        if (error)
            SKIP("filesystem does not permit the requested link");
    }
    RS_Graphic graphic;
    REQUIRE(storage.loadDocument(&graphic, source, RS2::FormatUnknown,
                                  RS2::DxfTextEncoding::LegacyLibreCadUtf8));
    REQUIRE(graphic.isDxfRecoverySourceFile(backup));
    CHECK_FALSE(storage.backupDrawingFile(copy, suffix, &graphic));
    CHECK(contents(source) == original);
    CHECK(contents(backup) == original);
    CHECK(contents(copy) == priorCopy);

    // Skipping this unsafe optional backup must not prevent writing the
    // corrected destination or leave the recovered document unsaved.
    storage.m_answer = copy;
    storage.m_answerType = RS2::FormatDXFRW2000;
    bool cancelled = false;
    REQUIRE(storage.saveDocumentAs(&graphic, nullptr, cancelled));
    CHECK_FALSE(cancelled);
    CHECK(graphic.getFilename() == copy);
    CHECK_FALSE(graphic.isModified());
    CHECK(contents(copy).contains("45%%D"));
    CHECK(contents(source) == original);
    CHECK(contents(backup) == original);
    edit(graphic);
    QString autosave;
    REQUIRE(storage.autoSaveDocument(&graphic, nullptr, autosave));
    CHECK(isDxf(contents(autosave)));
    REQUIRE(save(storage, graphic, cancelled));
    CHECK(contents(source) == original);
    CHECK(contents(backup) == original);
}

TEST_CASE("An accepted partial legacy DXF import still protects its original",
          "[documentsstorage][save][legacyutf8]") {
    ensureApp();
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    TestStorage storage;
    const QString source = dir.filePath("partial.dxf");
    writeLegacyUtf8Dxf(source);
    QByteArray malformed = contents(source);
    const QByteArray end("0\nENDSEC\n0\nEOF\n");
    REQUIRE(malformed.endsWith(end));
    malformed.chop(end.size());
    // The first TEXT is delivered at this next entity boundary. The second
    // one has its own valid handle but no height, forcing native read failure.
    malformed += "0\nTEXT\n5\n21\n100\nAcDbEntity\n8\n0\n100\nAcDbText\n"
                 "10\n1\n20\n1\n1\nmissing height\n0\nENDSEC\n0\nEOF\n";
    {
        QFile file(source);
        REQUIRE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        REQUIRE(file.write(malformed) == malformed.size());
    }
    const QByteArray original = contents(source);
    RS_Graphic nativePartial;
    RS_FilterDXFRW nativeReader;
    nativeReader.setDxfTextEncoding(RS2::DxfTextEncoding::LegacyLibreCadUtf8);
    REQUIRE_FALSE(nativeReader.fileImport(nativePartial, source, RS2::FormatDXFRW2000));
    CHECK(soleText(nativePartial) == QString::fromUtf8("45\xC2\xB0"));
    CHECK(nativePartial.isDxfRecoverySourceFile(source));

    RS_Graphic graphic;
    graphic.setFilename(source);
    graphic.setAutosaveFileName(dir.filePath("#partial.dxf"));
    graphic.setModified(true);
    int callbackCount = 0;
    bool partialReported = false;
    QString reportedError;
    REQUIRE(RS_FileIO::instance()->fileImport(
        graphic, source, RS2::FormatDXFRW,
        [&](bool partial, const QString& message) {
            ++callbackCount;
            partialReported = partial;
            reportedError = message;
            return true;
        }, RS2::DxfTextEncoding::LegacyLibreCadUtf8));
    CHECK(callbackCount == 1);
    CHECK(partialReported);
    CHECK_FALSE(reportedError.isEmpty());
    CHECK(soleText(graphic) == QString::fromUtf8("45\xC2\xB0"));
    CHECK_FALSE(graphic.isEmpty());
    CHECK(graphic.isDxfRecoverySourceFile(source));
    const QString oldFilename = graphic.getFilename();
    const QString oldAutosave = graphic.getAutoSaveFileName();
    const RS2::FormatType oldFormat = graphic.getFormatType();
    REQUIRE(graphic.isModified());
    CHECK_FALSE(RS_FileIO::instance()->fileExport(graphic, source, RS2::FormatDXFRW2000));
    // Save All delegates to this same per-document Save path. Refusing the
    // original must leave all session state and the partial progress intact.
    storage.m_answer = source;
    bool cancelled = false;
    CHECK_FALSE(save(storage, graphic, cancelled));
    CHECK_FALSE(cancelled);
    CHECK(graphic.getFilename() == oldFilename);
    CHECK(graphic.getAutoSaveFileName() == oldAutosave);
    CHECK(graphic.getFormatType() == oldFormat);
    CHECK(graphic.isModified());
    CHECK(graphic.isDxfRecoverySourceFile(source));
    CHECK(soleText(graphic) == QString::fromUtf8("45\xC2\xB0"));
    CHECK(contents(source) == original);
    CHECK_FALSE(QFile::exists(source + "~"));
}

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

// The preferences dialog read the suffix with another default than Save, and
// RS_Settings keeps the default of whichever reads first.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Opening the preferences does not rename the backup", "[documentsstorage][save]") {
    (void)lc::test::application(); // a dialog needs a QApplication
    SettingGuard backup{RS_SETTINGS, "Defaults", "AutoBackupDocument"};
    SettingGuard suffix{RS_SETTINGS, "Defaults", "BackupFileSuffix"};
    SettingGuard hidden{RS_SETTINGS, "Defaults", "HideBackupFiles"};
    backup.set(true);
    hidden.set(false);
    {
        // a profile that never stored a suffix
        const auto group = RS_SETTINGS->beginGroupGuard("Defaults");
        RS_SETTINGS->remove("BackupFileSuffix");
    }
    { const QG_DlgOptionsGeneral preferences; }

    QTemporaryDir dir;
    const QString path = dir.filePath("drawing.dxf");
    writeDxf(path);
    const QByteArray original = contents(path);
    LC_DocumentsStorage storage;
    RS_Graphic graphic;
    open(storage, graphic, path);
    edit(graphic);
    bool cancelled = false;
    REQUIRE(save(storage, graphic, cancelled));
    CHECK(contents(path + "~") == original);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Backup and autosave copies are hidden by default without losing recovery data",
          "[documentsstorage][backup-visibility]") {
    ensureApp();
    QTemporaryDir dir;
    TestStorage storage;
    {
        const auto group = RS_SETTINGS->beginGroupGuard("Defaults");
        RS_SETTINGS->remove("HideBackupFiles");
    }
    const QString path = dir.filePath("drawing.dxf");
    writeDxf(path);
    const QByteArray original = contents(path);
    RS_Graphic graphic;
    open(storage, graphic, path);
    edit(graphic);

    QString autosave;
    REQUIRE(storage.autoSaveDocument(&graphic, nullptr, autosave));
    CHECK(QFileInfo(autosave).isHidden());
    CHECK(graphic.isModified());
    CHECK(contents(path) == original);
    RS_Graphic recovered;
    open(storage, recovered, autosave);
    CHECK(hasEditedLine(recovered));

    REQUIRE(storage.autoSaveDocument(&graphic, nullptr, autosave));
    CHECK(QFileInfo(autosave).isHidden());
    const QDir drawingDirectory(dir.path());
    CHECK(drawingDirectory.entryList(QDir::Files) == QStringList{"drawing.dxf"});

    bool cancelled = false;
    const QByteArray recovery = contents(autosave);
    storage.m_answer = dir.filePath("missing/drawing.dxf");
    CHECK_FALSE(storage.saveDocumentAs(&graphic, nullptr, cancelled));
    CHECK_FALSE(cancelled);
    CHECK(graphic.isModified());
    CHECK(graphic.getFilename() == path);
    CHECK(graphic.getAutoSaveFileName() == autosave);
    CHECK(contents(autosave) == recovery);
    CHECK(QFileInfo(autosave).isHidden());
    CHECK(contents(path) == original);

    REQUIRE(save(storage, graphic, cancelled));
    CHECK_FALSE(QFile::exists(autosave));
#ifdef Q_OS_WIN
    const QString backup = path + "~";
#else
    const QString backup = dir.filePath(".drawing.dxf~");
#endif
    CHECK(QFileInfo(backup).isHidden());
    CHECK(contents(backup) == original);
    CHECK(drawingDirectory.entryList(QDir::Files) == QStringList{"drawing.dxf"});

    const QByteArray saved = contents(path);
    edit(graphic);
    REQUIRE(save(storage, graphic, cancelled));
    CHECK(QFileInfo(backup).isHidden());
    CHECK(contents(backup) == saved);
}

TEST_CASE("Backup visibility can retain legacy names and custom affixes", "[documentsstorage][backup-visibility]") {
    ensureApp();
    QTemporaryDir dir;
    TestStorage storage;
    SettingGuard prefix{RS_SETTINGS, "Defaults", "AutosaveFilePrefix"};
    SettingGuard suffix{RS_SETTINGS, "Defaults", "BackupFileSuffix"};
    SettingGuard hidden{RS_SETTINGS, "Defaults", "HideBackupFiles"};
    const QString path = dir.filePath("drawing space.dxf");
    writeDxf(path);
    const QByteArray original = contents(path);

    SECTION("visible files keep their existing names") {
        RS_Graphic graphic;
        open(storage, graphic, path);
        edit(graphic);
        QString autosave;
        REQUIRE(storage.autoSaveDocument(&graphic, nullptr, autosave));
        CHECK(autosave == dir.filePath("#" + QFileInfo(path).fileName()));
        CHECK_FALSE(QFileInfo(autosave).isHidden());
        REQUIRE(storage.backupDrawingFile(path));
        CHECK(contents(path + "~") == original);
        CHECK_FALSE(QFileInfo(path + "~").isHidden());
    }
    SECTION("a custom hidden prefix is not prefixed twice") {
        hidden.set(true);
        prefix.set(QStringLiteral(".recovery-"));
        CHECK(storage.createAutoSaveFileName(QFileInfo(path)) == dir.filePath(".recovery-" + QFileInfo(path).fileName()));
        REQUIRE(storage.backupDrawingFile(path, ".bak"));
#ifdef Q_OS_WIN
        const QString backup = path + ".bak";
#else
        const QString backup = dir.filePath("." + QFileInfo(path).fileName() + ".bak");
#endif
        CHECK(contents(backup) == original);
        CHECK(QFileInfo(backup).isHidden());
    }
    SECTION("an empty autosave prefix never selects the drawing itself") {
        prefix.set(QString());
        CHECK(storage.createAutoSaveFileName(QFileInfo(path)) != path);
    }
    SECTION("a Unicode backup path is hidden without changing its contents") {
        hidden.set(true);
        const QString unicodePath = dir.filePath(QStringLiteral("drawing \u4e2d\u6587.dxf"));
        REQUIRE(QFile::copy(path, unicodePath));
        REQUIRE(storage.backupDrawingFile(unicodePath));
#ifdef Q_OS_WIN
        const QString backup = unicodePath + "~";
#else
        const QString backup = dir.filePath("." + QFileInfo(unicodePath).fileName() + "~");
#endif
        CHECK(contents(backup) == original);
        CHECK(QFileInfo(backup).isHidden());
        CHECK(contents(unicodePath) == original);
    }
    SECTION("an empty backup suffix never removes the drawing") {
        CHECK_FALSE(storage.backupDrawingFile(path, QString()));
        CHECK(contents(path) == original);
    }
    SECTION("an empty stored backup suffix still disables backups") {
        suffix.set(QString());
        CHECK_FALSE(storage.backupDrawingFile(path));
        CHECK(contents(path) == original);
    }
    SECTION("valid custom Unicode affixes are retained") {
        const QString customPrefix = QStringLiteral("\u81ea\u52d5_");
        const QString customSuffix = QStringLiteral("_\u526f\u672c");
        prefix.set(customPrefix);
        suffix.set(customSuffix);
        CHECK(storage.createAutoSaveFileName(QFileInfo(path)) == dir.filePath(customPrefix + QFileInfo(path).fileName()));
        REQUIRE(storage.backupDrawingFile(path));
        CHECK(contents(path + customSuffix) == original);
        CHECK(contents(path) == original);
    }
}

TEST_CASE("Backup visibility preference defaults to hidden and reflects stored settings", "[documentsstorage][backup-visibility]") {
    (void)lc::test::application();
    SettingGuard hidden{RS_SETTINGS, "Defaults", "HideBackupFiles"};
    {
        const auto group = RS_SETTINGS->beginGroupGuard("Defaults");
        RS_SETTINGS->remove("HideBackupFiles");
    }
    QG_DlgOptionsGeneral preferences;
    auto* checkbox = preferences.findChild<QCheckBox*>("cbHideBackupFiles");
    REQUIRE(checkbox != nullptr);
    CHECK(checkbox->isChecked());
    const auto* group = checkbox->parentWidget();
    const auto* layout = qobject_cast<QGridLayout*>(group->parentWidget()->layout());
    REQUIRE(layout != nullptr);
    const int groupIndex = layout->indexOf(group);
    REQUIRE(groupIndex >= 0);
    int row = 0, column = 0, rows = 0, columns = 0;
    layout->getItemPosition(groupIndex, &row, &column, &rows, &columns);
    const QRect autosaveCells(column, row, columns, rows);
    for (int i = 0; i < layout->count(); ++i) {
        if (layout->itemAt(i)->widget() != group) {
            layout->getItemPosition(i, &row, &column, &rows, &columns);
            CHECK_FALSE(autosaveCells.intersects(QRect(column, row, columns, rows)));
        }
    }
    hidden.set(false);
    const QG_DlgOptionsGeneral reopened;
    const auto* visibleCheckbox = reopened.findChild<QCheckBox*>("cbHideBackupFiles");
    REQUIRE(visibleCheckbox != nullptr);
    CHECK_FALSE(visibleCheckbox->isChecked());
}

TEST_CASE("Backup filename affixes stay literal in translated preferences", "[documentsstorage][autosave][translation][3150]") {
    (void)lc::test::application();
    TestStorage storage;
    const QStringList prefixes{"#", "~", "_", "__", "lcauto_", "autosaved_"};
    const QStringList suffixes{"~", "$$", "_", "_backup", "_bak", ".bak", ".backup", ".lcb"};
    for (const auto* locale : {"zh_cn", "zh_tw"}) {
        INFO(locale);
        QTranslator translator;
        REQUIRE(translator.load("librecad_" + QString::fromLatin1(locale) + ".qm",
                                QStringLiteral(LIBRECAD_TEST_TRANSLATION_DIR)));
        REQUIRE(QApplication::installTranslator(&translator));
        QG_DlgOptionsGeneral preferences;
        for (const auto* combo : {preferences.cbAutoSaveFileNamePrefix, preferences.cbBackupFileSuffix}) {
            const auto& expected = combo == preferences.cbAutoSaveFileNamePrefix ? prefixes : suffixes;
            REQUIRE(combo->count() == expected.size());
            for (int i = 0; i < combo->count(); ++i) {
                CHECK(combo->itemText(i) == expected.at(i));
            }
        }
        CHECK(preferences.cbAutoSaveFileNamePrefix->currentText() == "#");
        CHECK(preferences.cbBackupFileSuffix->currentText() == "~");
    }
}

TEST_CASE("Invalid stored affixes recover without opening preferences", "[documentsstorage][autosave][3150]") {
    ensureApp();
    TestStorage storage;
    SettingGuard prefix{RS_SETTINGS, "Defaults", "AutosaveFilePrefix"};
    SettingGuard suffix{RS_SETTINGS, "Defaults", "BackupFileSuffix"};
    SettingGuard hidden{RS_SETTINGS, "Defaults", "HideBackupFiles"};
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    const QString path = dir.filePath("drawing.dxf");
    writeDxf(path);
    const QByteArray original = contents(path);
    QStringList invalidAffixes{
        QStringLiteral("{\\fn\u65b9\u6b63\u7c97\u5029\u7b80\u4f53"
                       "\\fs12\\an8\\1cHFFFF00\\b0}"),
        QStringLiteral("../escape_"), QString(QChar(u'\0')), QString(QChar(u'\x1f'))};
    for (const QChar character : QStringLiteral("<>:\"/\\|?*")) {
        invalidAffixes.append(QStringLiteral("bad") + character);
    }
    for (const bool hideFiles : {false, true}) {
        hidden.set(hideFiles);
#ifdef Q_OS_WIN
        const QString expectedAutosave = dir.filePath("#drawing.dxf");
        const QString expectedBackup = path + "~";
#else
        const QString expectedAutosave = dir.filePath(hideFiles ? ".#drawing.dxf" : "#drawing.dxf");
        const QString expectedBackup = dir.filePath(hideFiles ? ".drawing.dxf~" : "drawing.dxf~");
#endif
        for (const QString& affix : invalidAffixes) {
            INFO(affix.toStdString());
            CAPTURE(hideFiles);
            prefix.set(affix);
            suffix.set(affix);
            RS_Graphic graphic;
            open(storage, graphic, path);
            REQUIRE(graphic.getAutoSaveFileName() == expectedAutosave);
            edit(graphic);
            QString written;
            REQUIRE(storage.autoSaveDocument(&graphic, nullptr, written));
            CHECK(written == expectedAutosave);
            CHECK(isDxf(contents(written)));
            CHECK(graphic.isModified());
            RS_Graphic recovered;
            open(storage, recovered, written);
            CHECK(hasEditedLine(recovered));
            REQUIRE(storage.backupDrawingFile(path));
            CHECK(contents(expectedBackup) == original);
            CHECK(contents(path) == original);
            REQUIRE(QFile::remove(written));
            REQUIRE(QFile::remove(expectedBackup));
        }
    }
}

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
