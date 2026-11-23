/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
** Copyright (C) 2026 Dongxu Li (github.com/dxli)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
**********************************************************************/

// Issue #3302: with an input method active, Qt 5 on macOS matched Return
// against the End shortcut and Tab against Home. The application-wide filter
// keeps these keys for the focused widget.

#include <catch2/catch_test_macros.hpp>

#include <QKeyEvent>

#include "qc_applicationwindow.h"

namespace {

bool kept(int key, Qt::KeyboardModifiers modifiers) {
    const QKeyEvent override(QEvent::ShortcutOverride, key, modifiers);
    return QC_ApplicationWindow::keepsKeyFromShortcuts(override);
}

} // namespace

TEST_CASE("Return, Enter and Tab never trigger a window shortcut",
          "[gui][shortcuts][issue3302]") {
    CHECK(kept(Qt::Key_Return, Qt::NoModifier));
    CHECK(kept(Qt::Key_Return, Qt::ShiftModifier));
    CHECK(kept(Qt::Key_Enter, Qt::KeypadModifier));
    CHECK(kept(Qt::Key_Tab, Qt::NoModifier));

    // shortcuts that carry Ctrl (Cmd on macOS), Alt or Meta still work
    CHECK_FALSE(kept(Qt::Key_Return, Qt::ControlModifier));
    CHECK_FALSE(kept(Qt::Key_Return, Qt::AltModifier));
    CHECK_FALSE(kept(Qt::Key_Enter, Qt::MetaModifier | Qt::KeypadModifier));
    CHECK_FALSE(kept(Qt::Key_Tab, Qt::ControlModifier));

    // the keys Return and Tab were mistaken for keep their own shortcuts
    CHECK_FALSE(kept(Qt::Key_End, Qt::NoModifier));
    CHECK_FALSE(kept(Qt::Key_Home, Qt::NoModifier));
}
