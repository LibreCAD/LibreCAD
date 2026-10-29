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

// Issue #3196: going back from "Select part of first line that should remain
// after trim" ran init(), which forgot line 1. The tool then asked for the
// second line and refused every pick ("No line selected") until it was taken
// back once more; on 2.2.1 the next mouse move crashed.

#include <catch2/catch_test_macros.hpp>

#include <QMouseEvent>

#include "lc_action_modify_line_join.h"
#include "lc_actiontestsupport.h"
#include "lc_graphicviewport.h"
#include "rs_line.h"

namespace {

using lc::test::ActionFixture;

// A mouse event at drawing coordinates, delivered to the tool as the view does.
void send(ActionFixture<LC_ActionModifyLineJoin>& f, const QEvent::Type type, const Qt::MouseButton button,
          const RS_Vector& point) {
    const auto* viewport = f.m_view.getViewPort();
    const QPointF gui(viewport->toGuiX(point.x), viewport->toGuiY(point.y));
    QMouseEvent event(type, gui, gui, button, Qt::NoButton, Qt::NoModifier);
    if (type == QEvent::MouseMove) {
        f.m_action->mouseMoveEvent(&event);
    }
    else {
        f.m_action->mouseReleaseEvent(&event);
    }
}

} // namespace

TEST_CASE("Line Join keeps the first line when going back from the trim side", "[linejoin][issue3196]") {
    ActionFixture<LC_ActionModifyLineJoin> f;
    f.m_view.getViewPort()->justSetOffsetAndFactor(0, 0, 1.0);
    f.m_graphic.addEntity(new RS_Line(&f.m_graphic, RS_Vector{0.0, 0.0}, RS_Vector{100.0, 0.0}));
    f.m_graphic.addEntity(new RS_Line(&f.m_graphic, RS_Vector{50.0, 20.0}, RS_Vector{50.0, 60.0}));
    f.m_action->init(LC_ActionModifyLineJoin::SetLine1);

    send(f, QEvent::MouseButtonRelease, Qt::LeftButton, {30.0, 0.0});
    send(f, QEvent::MouseButtonRelease, Qt::LeftButton, {50.0, 40.0});
    // the second line ends short of the first: which side of line 1 stays?
    REQUIRE(f.m_action->getStatus() == LC_ActionModifyLineJoin::ResolveFirstLineTrim);

    send(f, QEvent::MouseButtonRelease, Qt::RightButton, {50.0, 40.0});
    CHECK(f.m_action->getStatus() == LC_ActionModifyLineJoin::SetLine2);
    send(f, QEvent::MouseMove, Qt::NoButton, {80.0, 30.0});

    // line 1 is still chosen, so picking line 2 again asks for the side again
    send(f, QEvent::MouseButtonRelease, Qt::LeftButton, {50.0, 40.0});
    CHECK(f.m_action->getStatus() == LC_ActionModifyLineJoin::ResolveFirstLineTrim);
    // and choosing it joins the lines
    send(f, QEvent::MouseButtonRelease, Qt::LeftButton, {20.0, 0.0});
    CHECK(f.m_action->getStatus() == LC_ActionModifyLineJoin::SetLine1);
    CHECK(f.m_graphic.count() > 2);
}
