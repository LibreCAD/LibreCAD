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

#include <memory>
#include <utility>

#include <QApplication>
#include <QPoint>
#include <QSettings>
#include <QVariant>

#include "lc_actioncontext.h"
#include "qg_graphicview.h"
#include "rs_graphic.h"
#include "rs_settings.h"

namespace {

QApplication& application() {
    static int argc = 1;
    static char name[] = "librecad-tests";
    static char* argv[] = {name, nullptr};
    static QApplication* app = [] {
        auto* existing = qobject_cast<QApplication*>(QCoreApplication::instance());
        return existing != nullptr ? existing : new QApplication(argc, argv);
    }();
    static bool settingsReady = [] {
        QCoreApplication::setOrganizationName("LibreCAD");
        QCoreApplication::setApplicationName("LibreCAD-tests");
        RS_Settings::init("LibreCAD", "LibreCAD-tests");
        return true;
    }();
    (void)settingsReady;
    return *app;
}

class SettingGuard {
public:
    SettingGuard(RS_Settings* settings, QString group, QString key)
        : m_settings(settings), m_group(std::move(group)), m_key(std::move(key)),
          m_fullKey(QString("/%1/%2").arg(m_group, m_key)),
          m_existed(settings->getSettings()->contains(m_fullKey)),
          m_value(settings->getSettings()->value(m_fullKey)) {}

    ~SettingGuard() {
        auto groupGuard = m_settings->beginGroupGuard(m_group);
        if (m_existed) {
            m_settings->write(m_key, m_value);
        }
        else {
            m_settings->write(m_key, QVariant{});
            m_settings->remove(m_key);
        }
    }

    void set(const bool value) const { m_settings->writeSingle(m_group, m_key, value); }

    SettingGuard(const SettingGuard&) = delete;
    SettingGuard& operator=(const SettingGuard&) = delete;

private:
    RS_Settings* m_settings;
    QString m_group;
    QString m_key;
    QString m_fullKey;
    bool m_existed;
    QVariant m_value;
};

class GraphicViewFixture {
public:
    GraphicViewFixture() {
        m_graphic.initForNewDocument();
        m_view = std::make_unique<QG_GraphicView>(nullptr, &m_graphic, &m_actionContext);
        m_view->initView();
    }

    QG_GraphicView& view() const { return *m_view; }

private:
    RS_Graphic m_graphic;
    LC_ActionContext m_actionContext;
    std::unique_ptr<QG_GraphicView> m_view;
};

} // namespace

TEST_CASE("selection pointer reflects the appearance preference", "[gui][cursor]") {
    (void)application();
    SettingGuard selectionPointer{RS_SETTINGS, "Appearance", "UseLibreCADSelectionPointer"};
    GraphicViewFixture fixture;

    selectionPointer.set(false);
    fixture.view().loadSettings();
    fixture.view().setMouseCursor(RS2::ArrowCursor);
    CHECK(fixture.view().cursor().shape() == Qt::ArrowCursor);

    selectionPointer.set(true);
    fixture.view().loadSettings();
    CHECK(fixture.view().cursor().shape() == Qt::BitmapCursor);
    CHECK(fixture.view().cursor().hotSpot() == QPoint{50, 50});

    selectionPointer.set(false);
    fixture.view().loadSettings();
    CHECK(fixture.view().cursor().shape() == Qt::ArrowCursor);
}
