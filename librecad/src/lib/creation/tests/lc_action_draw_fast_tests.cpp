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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 * ********************************************************************************
 */

#include <utility>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSettings>

#include "lc_actiondrawlinedirect.h"
#include "lc_actiontestsupport.h"
#include "qg_dlgoptionsgeneral.h"
#include "rs_settings.h"

namespace {

class DrawFastActionProbe final : public LC_ActionDrawLineDirect {
public:
    explicit DrawFastActionProbe(LC_ActionContext* actionContext)
        : LC_ActionDrawLineDirect(actionContext) {}

    using LC_AbstractActionDrawLine::doProcessCommand;
    using LC_ActionDrawLineDirect::onCoordinateEvent;

    double softSnapSensitivity() const { return m_softSnapSensitivityRad; }
};

class SettingGuard {
public:
    SettingGuard(RS_Settings* settings, QString group, QString key)
        : m_settings(settings), m_group(std::move(group)), m_key(std::move(key)),
          m_fullKey(QString("/%1/%2").arg(m_group, m_key)),
          m_existed(settings->getSettings()->contains(m_fullKey)),
          m_value(settings->getSettings()->value(m_fullKey)) {}

    ~SettingGuard() {
        const auto groupGuard = m_settings->beginGroupGuard(m_group);
        if (m_existed) {
            m_settings->write(m_key, m_value);
        }
        else {
            m_settings->remove(m_key);
        }
    }

    void set(int value) const { m_settings->writeSingle(m_group, m_key, value); }
    void set(const QString& value) const { m_settings->writeSingle(m_group, m_key, value); }

private:
    RS_Settings* m_settings;
    QString m_group;
    QString m_key;
    QString m_fullKey;
    bool m_existed;
    QVariant m_value;
};

} // namespace

TEST_CASE("Draw Fast keeps its direct-distance interaction state", "[draw_fast][command]") {
    lc::test::ActionFixture<DrawFastActionProbe> fixture;
    auto& action = *fixture.m_action;

    action.onCoordinateEvent(action.getStatus(), false, RS_Vector{0.0, 0.0});
    const int directDistanceStatus = action.getStatus();

    CHECK_FALSE(action.doProcessCommand(directDistanceStatus, QStringLiteral("x")));
    CHECK_FALSE(action.doProcessCommand(directDistanceStatus, QStringLiteral("y")));
    CHECK_FALSE(action.doProcessCommand(directDistanceStatus, QStringLiteral("p")));
    CHECK_FALSE(action.doProcessCommand(directDistanceStatus, QStringLiteral("angle")));
    CHECK(action.getStatus() == directDistanceStatus);
}

TEST_CASE("Soft Angle Snap preserves a free range at small angle steps", "[draw_fast][snap]") {
    lc::test::ActionFixture<DrawFastActionProbe> fixture;
    SettingGuard stepGuard{RS_SETTINGS, "Defaults", "AngleSnapStep"};
    SettingGuard sensitivityGuard{RS_SETTINGS, "Defaults", "SoftSnapSensitivityAngle"};

    stepGuard.set(0);
    sensitivityGuard.set(QStringLiteral("invalid"));
    fixture.m_action->refreshBySettings();

    CHECK(fixture.m_action->softSnapSensitivity() == Catch::Approx(fixture.m_action->getAngleStep() * 0.45));

    sensitivityGuard.set(QStringLiteral("45"));
    fixture.m_action->refreshBySettings();
    CHECK(fixture.m_action->softSnapSensitivity() == Catch::Approx(fixture.m_action->getAngleStep() * 0.45));
}

TEST_CASE("Soft Angle Snap preferences reflect their active angle step", "[draw_fast][gui]") {
    (void)lc::test::application();
    QG_DlgOptionsGeneral dialog;

    dialog.cbSoftSnapEnabled->setChecked(false);
    CHECK_FALSE(dialog.sbSoftSnapSensitivity->isEnabled());
    dialog.cbSoftSnapEnabled->setChecked(true);
    CHECK(dialog.sbSoftSnapSensitivity->isEnabled());

    dialog.cbAngleSnapStep->setCurrentIndex(0);
    CHECK(dialog.sbSoftSnapSensitivity->maximum() == Catch::Approx(0.4));
    dialog.cbAngleSnapStep->setCurrentIndex(9);
    CHECK(dialog.sbSoftSnapSensitivity->maximum() == Catch::Approx(40.5));
}
