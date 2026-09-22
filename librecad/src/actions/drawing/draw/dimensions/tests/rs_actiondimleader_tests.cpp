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

// A leader must end on the main keyboard's Return, not only on the keypad
// Enter - the same key handling LibreCAD#2241 fixed for plugin selections.

#include <catch2/catch_test_macros.hpp>

#include <QKeyEvent>

#include "lc_actiontestsupport.h"
#include "rs_actiondimleader.h"

namespace {
// The action's Status enum is protected, so reach it from a subclass rather
// than hard-coding the value.
class ProbeDimLeader : public RS_ActionDimLeader {
public:
    using RS_ActionDimLeader::RS_ActionDimLeader;

    void startEndpointStep() {setStatus(SetEndpoint);}
    bool isAtStartpointStep() const {return getStatus() == SetStartpoint;}
};
}

TEST_CASE("A leader ends on Return as well as on the keypad Enter",
          "[actions][dimensions][dimleader]") {
    for (const auto key : {Qt::Key_Return, Qt::Key_Enter}) {
        lc::test::ActionFixture<ProbeDimLeader> fixture;
        fixture.m_action->startEndpointStep();
        REQUIRE_FALSE(fixture.m_action->isAtStartpointStep());

        QKeyEvent event(QEvent::KeyPress, key, Qt::NoModifier);
        fixture.m_action->keyPressEvent(&event);

        CHECK(fixture.m_action->isAtStartpointStep());
    }
}
