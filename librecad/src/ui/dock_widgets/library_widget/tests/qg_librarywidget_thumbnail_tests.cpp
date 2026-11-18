/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#include <catch2/catch_test_macros.hpp>

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QListView>
#include <QStandardItemModel>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeView>
#include <QUuid>

#include "lc_actiontestsupport.h"
#include "lc_offsetimportsession.h"
#include "qg_librarywidget.h"
#include "rs_settings.h"
#include "rs_system.h"

class QG_LibraryWidgetTestAccess final {
public:
    static void setThumbnailCacheRoot(QG_LibraryWidget& widget, const QString& path) {
        widget.m_thumbnailCacheRoot = path + QDir::separator();
    }
    static bool hasActiveThumbnailSession(const QG_LibraryWidget& widget) {
        return widget.m_thumbnailSession && widget.m_thumbnailSession->busy();
    }
};

namespace {

class LibrarySetting final {
public:
    explicit LibrarySetting(const QString& path) {
        LC_GROUP_GUARD("Paths");
        m_previous = LC_GET_STR("Library", "");
        LC_SET("Library", path);
    }
    ~LibrarySetting() {
        LC_GROUP_GUARD("Paths");
        LC_SET("Library", m_previous);
    }

private:
    QString m_previous;
};

void waitForFile(const QString& path) {
    QEventLoop loop;
    QTimer poll;
    QTimer watchdog;
    bool expired = false;
    QObject::connect(&poll, &QTimer::timeout, &loop, [&] {
        if (QFileInfo::exists(path)) loop.quit();
    });
    QObject::connect(&watchdog, &QTimer::timeout, &loop, [&] {
        expired = true;
        loop.quit();
    });
    poll.start(5);
    watchdog.setSingleShot(true);
    watchdog.start(10000);
    loop.exec();
    INFO("The staged library thumbnail did not finish before the test deadline");
    REQUIRE_FALSE(expired);
}

void waitForActiveThumbnailSession(QG_LibraryWidget& widget) {
    QElapsedTimer timer;
    timer.start();
    while (!QG_LibraryWidgetTestAccess::hasActiveThumbnailSession(widget) && timer.elapsed() < 5000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
    }
    INFO("The first staged thumbnail session did not become active before the test deadline");
    REQUIRE(QG_LibraryWidgetTestAccess::hasActiveThumbnailSession(widget));
}

} // namespace

TEST_CASE("Library thumbnails render after staged DXF import without blocking model population",
          "[offset-import][library-thumbnail][worker]") {
    REQUIRE(lc::test::application() != nullptr);
    static const bool systemReady = [] {
        RS_SYSTEM->init("LibreCAD", "tests", "librecad", "librecad_tests");
        return true;
    }();
    (void)systemReady;

    const QTemporaryDir libraryRoot;
    REQUIRE(libraryRoot.isValid());

    const QString folder = QStringLiteral("offset-preview-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    const QString firstDirectory = libraryRoot.filePath(folder + QStringLiteral("/a-parts"));
    const QString secondDirectory = libraryRoot.filePath(folder + QStringLiteral("/b-parts"));
    REQUIRE(QDir().mkpath(firstDirectory));
    REQUIRE(QDir().mkpath(secondDirectory));
    const QString sourcePath = firstDirectory + QStringLiteral("/native.dxf");
    const QString secondSourcePath = secondDirectory + QStringLiteral("/native.dxf");
    const QString fixturePath = QStringLiteral(LIBRECAD_TEST_DIR) +
        QStringLiteral("/offsetcurve_independent/dxf-binary-R2018-native-degree1.dxf");
    REQUIRE(QFile::copy(fixturePath, sourcePath));
    REQUIRE(QFile::copy(fixturePath, secondSourcePath));
    LibrarySetting librarySetting(libraryRoot.path());

    const QString cacheRoot = libraryRoot.filePath(QStringLiteral("cache"));
    const QString firstOutput = QDir::cleanPath(cacheRoot + QDir::separator() + folder +
                                                QStringLiteral("/a-parts/native.png"));
    const QString secondOutput = QDir::cleanPath(cacheRoot + QDir::separator() + folder +
                                                 QStringLiteral("/b-parts/native.png"));
    QDir(QFileInfo(firstOutput).absolutePath()).removeRecursively();
    QDir(QFileInfo(secondOutput).absolutePath()).removeRecursively();

    {
        QG_LibraryWidget widget(nullptr);
        QG_LibraryWidgetTestAccess::setThumbnailCacheRoot(widget, cacheRoot);
        const auto trees = widget.findChildren<QTreeView*>();
        REQUIRE(trees.size() == 1);
        auto* directories = qobject_cast<QStandardItemModel*>(trees.first()->model());
        REQUIRE(directories != nullptr);
        const auto roots = directories->findItems(folder, Qt::MatchExactly);
        REQUIRE(roots.size() == 1);
        REQUIRE(roots.first()->rowCount() == 2);

        widget.updatePreview(roots.first()->child(0)->index());
        CHECK_FALSE(QFileInfo::exists(firstOutput));
        waitForActiveThumbnailSession(widget);

        widget.updatePreview(roots.first()->child(1)->index());
        CHECK_FALSE(QFileInfo::exists(secondOutput));
        const auto previews = widget.findChildren<QListView*>();
        REQUIRE(previews.size() == 1);
        auto* icons = qobject_cast<QStandardItemModel*>(previews.first()->model());
        REQUIRE(icons != nullptr);
        REQUIRE(icons->rowCount() == 1);
        waitForFile(secondOutput);
        CHECK_FALSE(QFileInfo::exists(firstOutput));
        const QImage image(secondOutput);
        CHECK_FALSE(image.isNull());
        CHECK(image.size() == QSize(64, 64));
        REQUIRE(icons->item(0) != nullptr);
        CHECK_FALSE(icons->item(0)->icon().isNull());
        CHECK(icons->item(0)->icon().pixmap(64, 64).toImage() == image);
    }

    QDir(QFileInfo(firstOutput).absolutePath()).removeRecursively();
    QDir(QFileInfo(secondOutput).absolutePath()).removeRecursively();
}
