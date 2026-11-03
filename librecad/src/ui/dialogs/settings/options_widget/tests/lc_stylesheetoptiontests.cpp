/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 Dongxu Li (github.com/dxli)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 * ********************************************************************************
 */

// Issue #2297: the Style Sheet of Widget Options is a checkbox beside its path,
// so it can be switched off without losing the path. The path of a file that is
// not there (a drive that is not mounted) used to be dropped when the dialog
// opened, and OK then saved an empty one.

#include <catch2/catch_test_macros.hpp>

#include <QApplication>
#include <QCheckBox>
#include <QLineEdit>
#include <QTemporaryFile>

#include "lc_actiontestsupport.h"
#include "lc_customstylehelper.h"
#include "lc_settingguard.h"
#include "lc_widgetoptionsdialog.h"
#include "rs_settings.h"

namespace {

using lc::test::SettingGuard;

const QString kSheet = QStringLiteral("QLabel { color: red; }");

// A style sheet in the application is process-wide, and loadFromSettings() leaves
// the Widgets group open: put both back.
struct SheetReset {
    ~SheetReset() {
        qApp->setStyleSheet({});
        RS_SETTINGS->endGroup();
    }
};

// The settings of a version from before the checkbox existed have no such key.
void forgetAllowStyleSheet() {
    const auto group = RS_SETTINGS->beginGroupGuard("Widgets");
    RS_SETTINGS->remove("AllowStyleSheet");
}

struct SheetFile {
    SheetFile() {
        REQUIRE(file.open());
        file.write(kSheet.toUtf8());
        file.close();
    }
    QString path() const { return file.fileName(); }
    QTemporaryFile file;
};

// Opens Widget Options and checks what it shows of the style sheet.
void checkShown(const QString& path, const bool ticked) {
    LC_WidgetOptionsDialog dialog;
    const auto* field = dialog.findChild<QLineEdit*>("stylesheet_field");
    const auto* box = dialog.findChild<QCheckBox*>("stylesheet_checkbox");
    REQUIRE(field != nullptr);
    REQUIRE(box != nullptr);
    CHECK(field->text().toStdString() == path.toStdString());
    CHECK(box->isChecked() == ticked);
}

} // namespace

TEST_CASE("Widget Options shows the saved style sheet and whether it is on", "[gui][dialog][stylesheet][issue2297]") {
    REQUIRE(lc::test::application() != nullptr);
    const SettingGuard path{RS_SETTINGS, "Widgets", "StyleSheet"};
    const SettingGuard allow{RS_SETTINGS, "Widgets", "AllowStyleSheet"};
    const QString missing = QStringLiteral("/no/such/dir/lc-style.qss");

    SECTION("a path whose file is missing is kept") {
        path.set(missing);
        allow.set(true);
        checkShown(missing, true);
    }
    SECTION("a path that is switched off is kept, unticked") {
        path.set(missing);
        allow.set(false);
        checkShown(missing, false);
    }
    SECTION("a path saved before the checkbox existed is on") {
        path.set(missing);
        forgetAllowStyleSheet();
        checkShown(missing, true);
    }
    SECTION("nothing saved is on, so a path typed in is applied as before") {
        path.set(QString{});
        forgetAllowStyleSheet();
        checkShown(QString{}, true);
    }
}

TEST_CASE("Start-up applies the style sheet only when it is on", "[gui][stylesheet][issue2297]") {
    REQUIRE(lc::test::application() != nullptr);
    const SettingGuard path{RS_SETTINGS, "Widgets", "StyleSheet"};
    const SettingGuard allow{RS_SETTINGS, "Widgets", "AllowStyleSheet"};
    const SheetReset reset;
    const SheetFile sheet;
    path.set(sheet.path());
    LC_CustomStyleHelper helper{nullptr};

    SECTION("switched off") {
        allow.set(false);
        helper.loadFromSettings();
        CHECK(qApp->styleSheet().isEmpty());
    }
    SECTION("switched on") {
        allow.set(true);
        helper.loadFromSettings();
        CHECK(qApp->styleSheet().toStdString() == kSheet.toStdString());
    }
    SECTION("saved before the checkbox existed") {
        forgetAllowStyleSheet();
        helper.loadFromSettings();
        CHECK(qApp->styleSheet().toStdString() == kSheet.toStdString());
    }
}
