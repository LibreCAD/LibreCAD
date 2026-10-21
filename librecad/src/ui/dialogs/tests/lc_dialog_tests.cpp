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

#include <catch2/catch_test_macros.hpp>

#include <QDateTimeEdit>
#include <QDoubleSpinBox>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLineEdit>
#include <QLocale>
#include <QMouseEvent>
#include <QSpinBox>
#include <QVBoxLayout>

#include "lc_actiongroupmanager.h"
#include "lc_actionsshortcutsdialog.h"
#include "lc_actiontestsupport.h"
#include "lc_dialog.h"

namespace {
struct DialogFixture {
    const bool applicationReady = lc::test::application() != nullptr;
    LC_Dialog dialog{nullptr, "NumericSelectionTest"};
    QVBoxLayout layout{&dialog};
    QLineEdit other{&dialog};
    QDoubleSpinBox number{&dialog};
    QSpinBox integer{&dialog};
    QDateTimeEdit date{&dialog};

    DialogFixture() {
        number.setLocale(QLocale::c());
        number.setRange(-1000.0, 1000.0);
        number.setValue(5.0);
        number.setSingleStep(0.01);
        number.setSuffix(" drawing units");
        integer.setRange(-1000, 1000);
        integer.setValue(32);
        integer.setSuffix(" px");
        layout.addWidget(&other);
        layout.addWidget(&number);
        layout.addWidget(&integer);
        layout.addWidget(&date);
        dialog.resize(360, 240);
        dialog.show();
        dialog.activateWindow();
        QApplication::processEvents();
        other.setFocus();
    }
};

QLineEdit* editor(QAbstractSpinBox& spinBox) {
    auto* result = spinBox.findChild<QLineEdit*>();
    REQUIRE(result != nullptr);
    return result;
}

void mouse(QWidget& widget, QEvent::Type type, QPoint point, Qt::MouseButton button,
           Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
    // Synthetic sendEvent calls do not perform spontaneous mouse-focus processing.
    if (type == QEvent::MouseButtonPress) {
        widget.setFocus(Qt::MouseFocusReason);
    }
    QMouseEvent event(type, QPointF(point), QPointF(widget.mapToGlobal(point)), button, buttons, modifiers);
    QApplication::sendEvent(&widget, &event);
}

void click(QWidget& widget, Qt::MouseButton button = Qt::LeftButton) {
    const QPoint point{widget.width() / 3, widget.height() / 2};
    mouse(widget, QEvent::MouseButtonPress, point, button, button);
    mouse(widget, QEvent::MouseButtonRelease, point, button, Qt::NoButton);
}

void type(QWidget& widget, const QString& text) {
    for (QChar character : text) {
        QKeyEvent event(QEvent::KeyPress, 0, Qt::NoModifier, QString(character));
        QApplication::sendEvent(&widget, &event);
    }
}

void key(QWidget& widget, int code, Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
    for (const auto type : {QEvent::KeyPress, QEvent::KeyRelease}) {
        QKeyEvent event(type, code, modifiers);
        QApplication::sendEvent(&widget, &event);
    }
}
}

TEST_CASE("Find selects the shortcut dialog's filter text", "[gui][dialog][shortcuts][3131]") {
    (void)lc::test::application();
    LC_ActionGroupManager actions(nullptr);
    LC_ActionsShortcutsDialog dialog(nullptr, &actions);
    auto* filter = dialog.findChild<QLineEdit*>("leFilter");
    auto* tree = dialog.findChild<QWidget*>("tvMappingsTree");
    REQUIRE(filter != nullptr);
    REQUIRE(tree != nullptr);
    dialog.show();
    dialog.activateWindow();
    QApplication::processEvents();
    const auto find = QKeySequence(QKeySequence::Find)[0];
    for (QWidget* source : {tree, static_cast<QWidget*>(filter)}) {
        filter->setText("old filter");
        filter->setCursorPosition(3);
        source->setFocus();
        REQUIRE(source->hasFocus());
        key(*source, find.key(), find.keyboardModifiers());
        REQUIRE(filter->hasFocus());
        CHECK(filter->selectedText() == "old filter");
        type(*filter, "new");
        CHECK(filter->text() == "new");
    }

    filter->clear();
    tree->setFocus();
    key(*tree, find.key(), find.keyboardModifiers());
    CHECK(filter->hasFocus());
    CHECK(filter->text().isEmpty());

    filter->setText("ordinary text");
    tree->setFocus();
    click(*filter);
    CHECK(filter->hasFocus());
    CHECK_FALSE(filter->hasSelectedText());
}

TEST_CASE("Dialog spin boxes select their numeric value on a focusing click", "[gui][dialog][numeric]") {
    DialogFixture f;

    SECTION("a suffix does not obstruct replacing a double") {
        click(*editor(f.number));
        REQUIRE(f.number.hasFocus());
        CHECK(editor(f.number)->selectedText() == f.number.cleanText());
        type(*editor(f.number), "12.5");
        f.number.interpretText();
        CHECK(f.number.value() == 12.5);
        CHECK(f.number.suffix() == " drawing units");
    }
    SECTION("integer prefixes and suffixes are excluded") {
        f.integer.setPrefix("Size: ");
        click(*editor(f.integer));
        CHECK(editor(f.integer)->selectedText() == f.integer.cleanText());
        type(*editor(f.integer), "64");
        f.integer.interpretText();
        CHECK(f.integer.value() == 64);
        CHECK(f.integer.prefix() == "Size: ");
        CHECK(f.integer.suffix() == " px");
    }
    SECTION("negative values use the current locale") {
        f.number.setLocale(QLocale(QLocale::German));
        f.number.setValue(-12.5);
        click(*editor(f.number));
        CHECK(editor(f.number)->selectedText() == "-12,50");
        type(*editor(f.number), "3,25");
        f.number.interpretText();
        CHECK(f.number.value() == 3.25);
    }
    SECTION("a special minimum value can be replaced") {
        f.integer.setSpecialValueText("Automatic");
        f.integer.setValue(f.integer.minimum());
        click(*editor(f.integer));
        CHECK(editor(f.integer)->selectedText() == "Automatic");
        type(*editor(f.integer), "24");
        f.integer.interpretText();
        CHECK(f.integer.value() == 24);
    }
}

TEST_CASE("Dialog numeric selection preserves ordinary editing", "[gui][dialog][numeric]") {
    DialogFixture f;
    QLineEdit* edit = editor(f.number);

    SECTION("subsequent clicks position the caret") {
        click(*edit);
        REQUIRE(edit->selectedText() == f.number.cleanText());
        click(*edit);
        CHECK_FALSE(edit->hasSelectedText());
    }
    SECTION("a drag on entry retains a partial selection") {
        const QPoint start{3, edit->height() / 2};
        const QPoint end{3 + edit->fontMetrics().horizontalAdvance("5."), start.y()};
        mouse(*edit, QEvent::MouseButtonPress, start, Qt::LeftButton, Qt::LeftButton);
        mouse(*edit, QEvent::MouseMove, end, Qt::NoButton, Qt::LeftButton);
        const QString selection = edit->selectedText();
        REQUIRE_FALSE(selection.isEmpty());
        REQUIRE(selection != f.number.cleanText());
        mouse(*edit, QEvent::MouseButtonRelease, end, Qt::LeftButton, Qt::NoButton);
        CHECK(edit->selectedText() == selection);
    }
    SECTION("a short drag returning to its start is not a click") {
        const int y = edit->height() / 2;
        int x = 0;
        while (x + 1 < edit->width()
               && edit->cursorPositionAt({x, y}) == edit->cursorPositionAt({x + 1, y})) {
            ++x;
        }
        REQUIRE(x + 1 < edit->width());
        const QPoint start{x, y};
        const QPoint end{x + 1, y};
        mouse(*edit, QEvent::MouseButtonPress, start, Qt::LeftButton, Qt::LeftButton);
        mouse(*edit, QEvent::MouseMove, end, Qt::NoButton, Qt::LeftButton);
        REQUIRE(edit->hasSelectedText());
        mouse(*edit, QEvent::MouseMove, start, Qt::NoButton, Qt::LeftButton);
        REQUIRE_FALSE(edit->hasSelectedText());
        mouse(*edit, QEvent::MouseButtonRelease, start, Qt::LeftButton, Qt::NoButton);
        CHECK_FALSE(edit->hasSelectedText());
    }
    SECTION("movement beyond the drag threshold cancels selection even without selected text") {
        const QPoint start{0, edit->height() / 2};
        const QPoint end{-QApplication::startDragDistance() - 1, start.y()};
        mouse(*edit, QEvent::MouseButtonPress, start, Qt::LeftButton, Qt::LeftButton);
        mouse(*edit, QEvent::MouseMove, end, Qt::NoButton, Qt::LeftButton);
        REQUIRE_FALSE(edit->hasSelectedText());
        mouse(*edit, QEvent::MouseMove, start, Qt::NoButton, Qt::LeftButton);
        mouse(*edit, QEvent::MouseButtonRelease, start, Qt::LeftButton, Qt::NoButton);
        CHECK_FALSE(edit->hasSelectedText());
    }
    SECTION("release outside the editor is not a click") {
        const QPoint start{0, edit->height() / 2};
        mouse(*edit, QEvent::MouseButtonPress, start, Qt::LeftButton, Qt::LeftButton);
        mouse(*edit, QEvent::MouseButtonRelease, {0, -1}, Qt::LeftButton, Qt::NoButton);
        CHECK_FALSE(edit->hasSelectedText());
        click(*edit);
        CHECK_FALSE(edit->hasSelectedText());
    }
    SECTION("mouse focus without a matching press does not select on release") {
        f.number.setFocus(Qt::MouseFocusReason);
        mouse(*edit, QEvent::MouseButtonRelease, {0, edit->height() / 2},
              Qt::LeftButton, Qt::NoButton);
        CHECK_FALSE(edit->hasSelectedText());
    }
    SECTION("a modified press stays cancelled when the modifier is released") {
        const QPoint point{0, edit->height() / 2};
        edit->setCursorPosition(0);
        mouse(*edit, QEvent::MouseButtonPress, point, Qt::LeftButton, Qt::LeftButton,
              Qt::ShiftModifier);
        REQUIRE_FALSE(edit->hasSelectedText());
        QKeyEvent release(QEvent::KeyRelease, Qt::Key_Shift, Qt::NoModifier);
        QApplication::sendEvent(edit, &release);
        mouse(*edit, QEvent::MouseButtonRelease, point, Qt::LeftButton, Qt::NoButton);
        CHECK_FALSE(edit->hasSelectedText());
    }
    SECTION("a modifier change during a click cancels automatic selection") {
        const QPoint point{0, edit->height() / 2};
        mouse(*edit, QEvent::MouseButtonPress, point, Qt::LeftButton, Qt::LeftButton);
        key(*edit, Qt::Key_Shift);
        mouse(*edit, QEvent::MouseButtonRelease, point, Qt::LeftButton, Qt::NoButton);
        CHECK_FALSE(edit->hasSelectedText());
    }
    SECTION("double-click selection remains native") {
        click(*edit);
        const QPoint point{3, edit->height() / 2};
        mouse(*edit, QEvent::MouseButtonDblClick, point, Qt::LeftButton, Qt::LeftButton);
        const QString selection = edit->selectedText();
        REQUIRE_FALSE(selection.isEmpty());
        mouse(*edit, QEvent::MouseButtonRelease, point, Qt::LeftButton, Qt::NoButton);
        CHECK(edit->selectedText() == selection);
    }
    SECTION("losing focus cancels a pending click") {
        const QPoint point{0, edit->height() / 2};
        mouse(*edit, QEvent::MouseButtonPress, point, Qt::LeftButton, Qt::LeftButton);
        f.other.setFocus();
        mouse(*edit, QEvent::MouseButtonRelease, point, Qt::LeftButton, Qt::NoButton);
        CHECK_FALSE(edit->hasSelectedText());
    }
    SECTION("right-click focus does not select all") {
        click(*edit, Qt::RightButton);
        CHECK_FALSE(edit->hasSelectedText());
        click(*edit);
        CHECK_FALSE(edit->hasSelectedText());
    }
    SECTION("read-only values are not selected automatically") {
        f.number.setReadOnly(true);
        click(*edit);
        CHECK_FALSE(edit->hasSelectedText());
    }
    SECTION("tab focus retains native numeric selection") {
        key(f.other, Qt::Key_Tab);
        CHECK(f.number.hasFocus());
        CHECK(edit->selectedText() == f.number.cleanText());
    }
    SECTION("backtab focus retains native numeric selection") {
        f.integer.setFocus();
        key(f.integer, Qt::Key_Backtab, Qt::ShiftModifier);
        CHECK(f.number.hasFocus());
        CHECK(edit->selectedText() == f.number.cleanText());
    }
    SECTION("arrow precision is unchanged") {
        f.number.setFocus();
        QKeyEvent event(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier);
        QApplication::sendEvent(&f.number, &event);
        CHECK(f.number.value() == 5.01);
        CHECK(f.number.singleStep() == 0.01);
    }
    SECTION("ordinary text fields keep caret placement") {
        f.other.setText("ordinary text");
        f.number.setFocus();
        click(f.other);
        CHECK_FALSE(f.other.hasSelectedText());
    }
    SECTION("date editors are excluded") {
        click(*editor(f.date));
        CHECK(editor(f.date)->selectedText() != f.date.text());
    }
    SECTION("showing a dialog again does not duplicate filters") {
        const auto children = f.number.children().size();
        f.dialog.hide();
        f.dialog.show();
        QApplication::processEvents();
        CHECK(f.number.children().size() == children);
    }
}
