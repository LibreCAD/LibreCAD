/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef LC_BUSYCURSORGUARD_H
#define LC_BUSYCURSORGUARD_H

#include <QCoreApplication>
#include <QCursor>
#include <QGuiApplication>
#include <QPointer>
#include <QThread>

/** Own exactly one override, and only on the real GUI application's thread. */
class LC_BusyCursorGuard final {
public:
    LC_BusyCursorGuard() {
        auto* app = qobject_cast<QGuiApplication*>(QCoreApplication::instance());
        if (app != nullptr && QThread::currentThread() == app->thread()) {
            m_application = app;
            QGuiApplication::setOverrideCursor(Qt::WaitCursor);
            m_pushed = true;
        }
    }
    ~LC_BusyCursorGuard() {
        if (m_pushed && m_application != nullptr) {
            QGuiApplication::restoreOverrideCursor();
        }
    }
    LC_BusyCursorGuard(const LC_BusyCursorGuard&) = delete;
    LC_BusyCursorGuard& operator=(const LC_BusyCursorGuard&) = delete;
    LC_BusyCursorGuard(LC_BusyCursorGuard&&) = delete;
    LC_BusyCursorGuard& operator=(LC_BusyCursorGuard&&) = delete;

private:
    QPointer<QGuiApplication> m_application;
    bool m_pushed = false;
};

#endif
