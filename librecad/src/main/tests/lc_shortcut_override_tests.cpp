// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 LibreCAD.org
// Copyright (C) 2026 Dongxu Li

// Issue #3302: with an input method active, Qt 5 on macOS matched Return
// against the End shortcut and Tab against Home. The application-wide filter
// accepts these shortcut overrides for the focused widget.

#include <QKeyEvent>
#include <QtTest>

#include "qc_applicationwindow.h"

namespace {
bool accepted(int key, Qt::KeyboardModifiers modifiers) {
    QKeyEvent override(QEvent::ShortcutOverride, key, modifiers);
    return QC_ApplicationWindow::acceptsShortcutOverride(override) && override.isAccepted();
}

bool remainsUnaccepted(int key, Qt::KeyboardModifiers modifiers) {
    QKeyEvent override(QEvent::ShortcutOverride, key, modifiers);
    override.ignore();
    return !QC_ApplicationWindow::acceptsShortcutOverride(override) && !override.isAccepted();
}
}

class LC_ShortcutOverrideTests : public QObject {
    Q_OBJECT
private slots:
    void returnEnterAndTabOverridesAreAccepted() {
        QVERIFY(accepted(Qt::Key_Return, Qt::NoModifier));
        QVERIFY(accepted(Qt::Key_Return, Qt::ShiftModifier));
        QVERIFY(accepted(Qt::Key_Enter, Qt::KeypadModifier));
        QVERIFY(accepted(Qt::Key_Tab, Qt::NoModifier));
        QVERIFY(accepted(Qt::Key_Tab, Qt::ShiftModifier));
    }

    void shortcutsWithCtrlAltOrMetaRemainAvailable() {
        QVERIFY(remainsUnaccepted(Qt::Key_Return, Qt::ControlModifier));
        QVERIFY(remainsUnaccepted(Qt::Key_Return, Qt::AltModifier));
        QVERIFY(remainsUnaccepted(Qt::Key_Enter, Qt::MetaModifier | Qt::KeypadModifier));
        QVERIFY(remainsUnaccepted(Qt::Key_Tab, Qt::ControlModifier));
    }

    void endAndHomeKeepTheirShortcuts() {
        QVERIFY(remainsUnaccepted(Qt::Key_End, Qt::NoModifier));
        QVERIFY(remainsUnaccepted(Qt::Key_Home, Qt::NoModifier));
    }
};

QTEST_MAIN(LC_ShortcutOverrideTests)
#include "lc_shortcut_override_tests.moc"
