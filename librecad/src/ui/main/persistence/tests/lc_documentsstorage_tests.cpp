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
// tests check the bytes that Save writes.

#include <catch2/catch_test_macros.hpp>

#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include "lc_documentsstorage.h"
#include "rs_fileio.h"
#include "rs_graphic.h"
#include "rs_line.h"
#include "rs_settings.h"

namespace {

void ensureApp() {
    // saveDocument() sets an override cursor, which needs a QGuiApplication
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
