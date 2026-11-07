/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
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
****************************************************************************/

// Draw Hatch keeps a hatch that trips the density guard and tells the user why
// (one commandMessage with "too dense" and the suggested scale).

#include <catch2/catch_test_macros.hpp>

#include <memory>

#include <QStringList>

#include "lc_actiontestsupport.h"
#include "lc_hatchdensity.h"
#include "lc_hatchdensitytext.h"
#include "lc_hatchtestsupport.h"
#include "rs_actiondrawhatch.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_entity.h"
#include "rs_graphic.h"
#include "rs_hatch.h"
#include "rs_line.h"

using lc::test::SourcePatterns;

namespace {

// Captures every commandMessage the action sends through LC_ActionContext,
// so the test can read the "too dense" message back.
class CapturingContext final : public LC_ActionContext {
public:
    QStringList messages;
    void commandMessage(const QString& message) override {
        messages.append(message);
    }
};

// Hatch-dialog stub: accepts the default pattern (ANSI31 scale 1, angle 0)
// so RS_ActionDrawHatch builds, validates and updates the hatch.
class AcceptHatchDialog : public RS_DialogFactoryInterface {
public:
    AcceptHatchDialog()
        : m_previous(RS_DialogFactory::instance()->getFactoryObject()) {
        RS_DialogFactory::instance()->setFactoryObject(this);
    }
    ~AcceptHatchDialog() override {
        RS_DialogFactory::instance()->setFactoryObject(m_previous);
    }
    AcceptHatchDialog(const AcceptHatchDialog&) = delete;
    AcceptHatchDialog& operator=(const AcceptHatchDialog&) = delete;

    bool requestHatchDialog(RS_Hatch* hatch,
                            [[maybe_unused]] LC_GraphicViewport* viewport) override {
        if (hatch == nullptr) return false;
        hatch->setSolid(false);
        hatch->setPattern("ANSI31");
        hatch->setScale(1.0);
        hatch->setAngle(0.0);
        return true;
    }

private:
    RS_DialogFactoryInterface* m_previous = nullptr;
};

// Fixture: SourcePatterns first (so Paths/Patterns points at the source tree
// and ANSI31 loads), then the graphic, view, context and action.
struct HatchActionFixture {
    SourcePatterns m_patterns;
    RS_Graphic m_graphic;
    lc::test::TestGraphicView m_view;
    CapturingContext m_context;
    std::unique_ptr<RS_ActionDrawHatch> m_action;

    HatchActionFixture() {
        m_graphic.initForNewDocument();
        m_view.setDocument(&m_graphic);
        m_context.setDocumentAndView(&m_graphic, &m_view);
        m_action = std::make_unique<RS_ActionDrawHatch>(&m_context);
    }
};

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Draw Hatch keeps a hatch over the old area limit and says why",
          "[lc-hatchfix][density]") {
    HatchActionFixture f;

    // Four lines forming (0, 0) - (10500, 10500). The square's area (1.1025e8)
    // is above the old 1e4 ratio guard and the ANSI31 estimate (about 1.1e6)
    // is above the default 1e6 cap, so the density guard trips.
    auto* bottom = new RS_Line(&f.m_graphic, {0.0, 0.0}, {10500.0, 0.0});
    auto* right  = new RS_Line(&f.m_graphic, {10500.0, 0.0}, {10500.0, 10500.0});
    auto* top    = new RS_Line(&f.m_graphic, {10500.0, 10500.0}, {0.0, 10500.0});
    auto* left   = new RS_Line(&f.m_graphic, {0.0, 10500.0}, {0.0, 0.0});
    f.m_graphic.addEntity(bottom);
    f.m_graphic.addEntity(right);
    f.m_graphic.addEntity(top);
    f.m_graphic.addEntity(left);
    f.m_graphic.select(QList<RS_Entity*>{bottom, right, top, left});

    AcceptHatchDialog stub;
    f.m_action->init(0);

    // Exactly one RS_Hatch in model space, tripped by the density guard.
    int hatches = 0;
    const RS_Hatch* dense = nullptr;
    for (RS_Entity* e : f.m_graphic) {
        if (e != nullptr && e->rtti() == RS2::EntityHatch) {
            ++hatches;
            dense = static_cast<const RS_Hatch*>(e);
        }
    }
    REQUIRE(hatches == 1);
    REQUIRE(dense != nullptr);
    CHECK(dense->getUpdateError() == RS_Hatch::HATCH_TOO_DENSE);

    // At least one captured message contains "too dense" and the scale hint
    // (1.05 for M = 1e6 on this square).
    int tooDense = 0;
    int scaleHint = 0;
    for (const QString& m : f.m_context.messages) {
        if (m.contains(QStringLiteral("too dense"))) ++tooDense;
        if (m.contains(QStringLiteral("1.05"))) ++scaleHint;
    }
    CHECK(tooDense >= 1);
    CHECK(scaleHint >= 1);
}
