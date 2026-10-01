/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li (github.com/dxli)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software Foundation,
** Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
**
****************************************************************************/

#include <catch2/catch_test_macros.hpp>

#include <QMouseEvent>

#include "lc_actiontestsupport.h"
#include "rs_actiondefault.h"
#include "rs_line.h"
#include "rs_selection.h"

namespace {
class DefaultActionProbe : public RS_ActionDefault {
public:
    using RS_ActionDefault::RS_ActionDefault;
    using RS_ActionDefault::Neutral;

    void initializeSettings() { initFromSettings(); }

    void beginMove(const RS_Vector& referencePoint) {
        const QPointF position{referencePoint.x, referencePoint.y};
        QMouseEvent press(QEvent::MouseButtonPress, position, position,
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        LC_MouseEvent event = lc::test::eventAt(referencePoint.x, referencePoint.y);
        event.originalEvent = &press;
        onMouseLeftButtonPress(Neutral, &event);

        LC_MouseEvent dragEvent = lc::test::eventAt(referencePoint.x + 100.0, referencePoint.y);
        onMouseMoveEvent(getStatus(), &dragEvent);
        REQUIRE(isMoving());

        LC_MouseEvent releaseEvent = lc::test::eventAt(referencePoint.x + 100.0, referencePoint.y);
        onMouseLeftButtonRelease(getStatus(), &releaseEvent);
    }

    void clickCopy(const RS_Vector& targetPoint) {
        double uiX = 0.0;
        double uiY = 0.0;
        m_viewport->toUI(targetPoint, uiX, uiY);
        const QPointF position{uiX, uiY};

        QMouseEvent move(QEvent::MouseMove, position, position, Qt::NoButton, Qt::NoButton, Qt::ControlModifier);
        mouseMoveEvent(&move);

        QMouseEvent press(QEvent::MouseButtonPress, position, position,
                          Qt::LeftButton, Qt::LeftButton, Qt::ControlModifier);
        mousePressEvent(&press);

        QMouseEvent release(QEvent::MouseButtonRelease, position, position,
                            Qt::LeftButton, Qt::NoButton, Qt::ControlModifier);
        mouseReleaseEvent(&release);
    }

    bool isMoving() const { return getStatus() == Moving; }
};
}

TEST_CASE("Default action keeps successive Ctrl-drag copies selected", "[actions][default][copy]") {
    lc::test::ActionFixture<DefaultActionProbe> fixture;
    fixture.m_action->initializeSettings();

    auto* line = new RS_Line(&fixture.m_graphic, RS_Vector{0.0, 0.0}, RS_Vector{100.0, 0.0});
    fixture.m_graphic.addEntity(line);
    RS_Selection(&fixture.m_view).selectSingle(line);

    fixture.m_action->beginMove(RS_Vector{50.0, 0.0});
    fixture.m_action->clickCopy(RS_Vector{150.0, 0.0});

    REQUIRE(fixture.m_action->isMoving());
    QList<RS_Entity*> selected;
    REQUIRE(fixture.m_graphic.collectSelected(selected));
    REQUIRE(selected.size() == 1);
    CHECK(selected.front() != line);

    fixture.m_action->clickCopy(RS_Vector{250.0, 0.0});

    CHECK(fixture.m_graphic.count() == 3);
    selected.clear();
    CHECK(fixture.m_graphic.collectSelected(selected));
    CHECK(selected.size() == 1);
    CHECK(fixture.m_action->isMoving());
}
