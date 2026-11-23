// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 LibreCAD.org
// Copyright (C) 2026 Dongxu Li

// Issue #3302: with an input method active, Qt 5 on macOS matched Return
// against the End shortcut and Tab against Home. The application-wide filter
// keeps these keys for the focused widget.

#include <QKeyEvent>
#include <QtTest>

#include "qc_applicationwindow.h"

namespace {
bool kept(int key, Qt::KeyboardModifiers modifiers) {
    const QKeyEvent override(QEvent::ShortcutOverride, key, modifiers);
    return QC_ApplicationWindow::keepsKeyFromShortcuts(override);
}
}

class LC_ShortcutOverrideTests : public QObject {
    Q_OBJECT
private slots:
    void returnEnterAndTabNeverTriggerAWindowShortcut() {
        QVERIFY(kept(Qt::Key_Return, Qt::NoModifier));
        QVERIFY(kept(Qt::Key_Return, Qt::ShiftModifier));
        QVERIFY(kept(Qt::Key_Enter, Qt::KeypadModifier));
        QVERIFY(kept(Qt::Key_Tab, Qt::NoModifier));
    }

    void shortcutsWithCtrlAltOrMetaStillWork() {
        QVERIFY(!kept(Qt::Key_Return, Qt::ControlModifier));
        QVERIFY(!kept(Qt::Key_Return, Qt::AltModifier));
        QVERIFY(!kept(Qt::Key_Enter, Qt::MetaModifier | Qt::KeypadModifier));
        QVERIFY(!kept(Qt::Key_Tab, Qt::ControlModifier));
    }

    void endAndHomeKeepTheirShortcuts() {
        QVERIFY(!kept(Qt::Key_End, Qt::NoModifier));
        QVERIFY(!kept(Qt::Key_Home, Qt::NoModifier));
    }
};

QTEST_MAIN(LC_ShortcutOverrideTests)
#include "lc_shortcut_override_tests.moc"
