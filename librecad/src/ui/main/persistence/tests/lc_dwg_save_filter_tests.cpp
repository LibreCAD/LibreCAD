/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#include <catch2/catch_test_macros.hpp>

#include <array>

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QSettings>
#include <QTemporaryDir>
#include <QTimer>

#include "lc_actiontestsupport.h"
#include "lc_documentsstorage.h"
#include "lc_filedialogservice.h"
#include "lc_settingguard.h"
#include "rs_graphic.h"
#include "rs_line.h"
#include "rs_settings.h"

namespace {
using lc::test::SettingGuard;

void ensureApp() {
    REQUIRE(lc::test::application() != nullptr);
}

#ifdef DWGSUPPORT
QByteArray contents(const QString& path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

void edit(RS_Graphic& graphic) {
    graphic.addEntity(new RS_Line(&graphic, RS_Vector(0.0, 0.0), RS_Vector(100.0, 50.0)));
    graphic.setModified(true);
}

bool hasEditedLine(const RS_Graphic& graphic) {
    for (const RS_Entity* entity : graphic) {
        if (entity->rtti() == RS2::EntityLine) {
            const auto* line = static_cast<const RS_Line*>(entity);
            if (line->getStartpoint() == RS_Vector(0.0, 0.0) && line->getEndpoint() == RS_Vector(100.0, 50.0)) return true;
        }
    }
    return false;
}

void open(LC_DocumentsStorage& storage, RS_Graphic& graphic, const QString& path) {
    graphic.initForNewDocument();
    graphic.setFormatType(RS2::FormatDXFRW);
    REQUIRE(storage.loadDocument(&graphic, path, RS2::FormatUnknown));
}
#endif

struct SaveFilterObservation {
    bool opened = false;
    bool timedOut = false;
    bool missingFilter = false;
    QString initialFilter;
    QString selectedFilter;
    QStringList filters;
};

// Drive the real modal QFileDialog, never a storage answer override.
// Callback observations are asserted after exec() unwinds, not across Qt slots.
template<typename Operation>
auto answerSaveFilterDialog(SaveFilterObservation& seen, const QString& path,
                            const QString& filter, bool cancel, Operation operation) {
    QTimer poll, timeout;
    timeout.setSingleShot(true);
    QObject::connect(&poll, &QTimer::timeout, &poll, [&] {
        if (seen.opened) return;
        auto* dialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget());
        if (!dialog || dialog->acceptMode() != QFileDialog::AcceptSave) return;
        seen.opened = true;
        seen.filters = dialog->nameFilters();
        seen.initialFilter = dialog->selectedNameFilter();
        if (!filter.isEmpty()) {
            if (!seen.filters.contains(filter)) {
                seen.missingFilter = true;
                dialog->reject();
                return;
            }
            dialog->selectNameFilter(filter);
        }
        seen.selectedFilter = dialog->selectedNameFilter();
        if (cancel) dialog->reject();
        else {
            dialog->selectFile(path);
            static_cast<QDialog*>(dialog)->accept();
        }
    });
    QObject::connect(&timeout, &QTimer::timeout, &timeout, [&] {
        seen.timedOut = true;
        if (auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget())) dialog->reject();
    });
    poll.start(1);
    timeout.start(5000);
    auto result = operation();
    poll.stop(); timeout.stop();
    return result;
}

void requireSaveFilterAnswered(const SaveFilterObservation& seen) {
    REQUIRE(seen.opened);
    REQUIRE_FALSE(seen.timedOut);
    REQUIRE_FALSE(seen.missingFilter);
}
}

#ifdef DWGSUPPORT
TEST_CASE("Actual Save As offers every DWG writer version and writes the selected signature",
          "[gui][documentsstorage][save][dwg-save-filters]") {
    ensureApp();
    REQUIRE(qobject_cast<QApplication*>(QCoreApplication::instance()));
    SettingGuard qt(RS_SETTINGS, "Defaults", "UseQtFileOpenDialog"); qt.set(true);
    SettingGuard folder(RS_SETTINGS, "Paths", "Save");
    SettingGuard remembered(RS_SETTINGS, "Paths", "SaveDrawingFilter");
    struct Choice { const char* filter; RS2::FormatType type; const char* signature; };
    const std::array<Choice, 6> choices{{
        {"DWG Drawing (*.dwg)", RS2::FormatDWG, "AC1015"},
        {"DWG Drawing 2004 (*.dwg)", RS2::FormatDWG2004, "AC1018"},
        {"DWG Drawing 2007 (*.dwg)", RS2::FormatDWG2007, "AC1021"},
        {"DWG Drawing 2010 (*.dwg)", RS2::FormatDWG2010, "AC1024"},
        {"DWG Drawing 2013 (*.dwg)", RS2::FormatDWG2013, "AC1027"},
        {"DWG Drawing 2018 (*.dwg)", RS2::FormatDWG2018, "AC1032"}
    }};
    for (const auto& choice : choices) {
        CAPTURE(choice.filter);
        QTemporaryDir dir; REQUIRE(dir.isValid());
        folder.set(QDir::tempPath());
        remembered.set(QStringLiteral("Drawing Exchange DXF 2007 (*.dxf)"));
        RS_Graphic graphic; graphic.initForNewDocument(); edit(graphic);
        LC_DocumentsStorage storage;
        bool cancelled = false;
        SaveFilterObservation seen;
        const auto stem = dir.filePath("selected-version");
        const auto filter = QString::fromLatin1(choice.filter);
        const bool saved = answerSaveFilterDialog(seen, stem, filter, false, [&] {
            return storage.saveDocumentAs(&graphic, nullptr, cancelled);
        });
        requireSaveFilterAnswered(seen);
        REQUIRE(saved); REQUIRE_FALSE(cancelled);
        CHECK(seen.selectedFilter == filter);
        for (const auto& advertised : choices)
            CHECK(seen.filters.count(QString::fromLatin1(advertised.filter)) == 1);
        const auto file = stem + ".dwg";
        CHECK(graphic.getFilename() == QDir::toNativeSeparators(file));
        CHECK(graphic.getFormatType() == choice.type);
        REQUIRE(contents(file).left(6) == QByteArray(choice.signature));
        RS_Graphic reopened;
        open(storage, reopened, file);
        CHECK(reopened.getFormatType() == choice.type);
        CHECK(hasEditedLine(reopened));
        CHECK(RS_SETTINGS->readStrSingle("Paths", "SaveDrawingFilter", {}) == filter);
        CHECK(RS_SETTINGS->readStrSingle("Paths", "Save", {}) == dir.path());
    }
}

TEST_CASE("Actual Save As selects a preferred DWG version and remembers it without a preference",
          "[gui][documentsstorage][save][dwg-save-filters]") {
    ensureApp();
    REQUIRE(qobject_cast<QApplication*>(QCoreApplication::instance()));
    SettingGuard qt(RS_SETTINGS, "Defaults", "UseQtFileOpenDialog"); qt.set(true);
    SettingGuard folder(RS_SETTINGS, "Paths", "Save");
    SettingGuard remembered(RS_SETTINGS, "Paths", "SaveDrawingFilter");
    QTemporaryDir dir; REQUIRE(dir.isValid()); folder.set(dir.path());
    remembered.set(QStringLiteral("DWG Drawing (*.dwg)"));
    const auto expected = QStringLiteral("DWG Drawing 2018 (*.dwg)");
    for (const auto preferred : {RS2::FormatDWG2018, RS2::FormatUnknown}) {
        CAPTURE(preferred);
        SaveFilterObservation seen;
        const auto stem = dir.filePath("preferred-version");
        const auto result = answerSaveFilterDialog(seen, stem, {}, false, [&] {
            return LC_FileDialogService::getFileDetails(LC_FileDialogService::SaveDrawing, {}, preferred);
        });
        requireSaveFilterAnswered(seen);
        CHECK(seen.initialFilter == expected);
        CHECK(result.fileType == RS2::FormatDWG2018);
        CHECK(result.filePath == QDir::toNativeSeparators(stem + ".dwg"));
        CHECK(result.fileExtension == ".dwg");
        CHECK(RS_SETTINGS->readStrSingle("Paths", "SaveDrawingFilter", {}) == expected);
    }
}
#endif

TEST_CASE("Actual Save As retains the DXF default and cancelling changes no saved paths or filter",
          "[gui][documentsstorage][save][dwg-save-filters]") {
    ensureApp();
    REQUIRE(qobject_cast<QApplication*>(QCoreApplication::instance()));
    SettingGuard qt(RS_SETTINGS, "Defaults", "UseQtFileOpenDialog"); qt.set(true);
    SettingGuard folder(RS_SETTINGS, "Paths", "Save");
    SettingGuard remembered(RS_SETTINGS, "Paths", "SaveDrawingFilter");
    QTemporaryDir dir; REQUIRE(dir.isValid()); folder.set(dir.path());
    {
        auto group = RS_SETTINGS->beginGroupGuard("Paths");
        RS_SETTINGS->remove("SaveDrawingFilter");
    }
    SaveFilterObservation seen;
    const auto result = answerSaveFilterDialog(seen, {}, {}, true, [&] {
        return LC_FileDialogService::getFileDetails(LC_FileDialogService::SaveDrawing);
    });
    requireSaveFilterAnswered(seen);
    CHECK(seen.initialFilter == "Drawing Exchange DXF 2007 (*.dxf)");
    CHECK(result.filePath.isEmpty()); CHECK(result.fileType == RS2::FormatUnknown);
    CHECK(RS_SETTINGS->readStrSingle("Paths", "Save", {}) == dir.path());
    CHECK_FALSE(RS_SETTINGS->getSettings()->contains("/Paths/SaveDrawingFilter"));
#ifndef DWGSUPPORT
    for (const auto& filter : seen.filters) CHECK_FALSE(filter.contains("*.dwg"));
#endif
}
