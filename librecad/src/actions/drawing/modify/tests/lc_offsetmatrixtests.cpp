/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2026 LibreCAD.org

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

// The measured behaviour matrix of Modify > Offset, row by row, through the
// action: the source pre-selected, a fixed distance, one click at the pick,
// Keep Originals off as the matrix was measured. Each row checks the outcome,
// what was committed, the command line, and that the hover preview showed the
// same (or said why nothing is shown). Rows marked "as measured" pin today's
// behaviour and change when the phase that fixes them lands.

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <functional>
#include <memory>
#include <vector>

#include <QStringList>

#include "lc_action_modify_offset.h"
#include "lc_actiontestsupport.h"
#include "lc_cursoroverlayinfo.h"
#include "lc_splinepoints.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_modification.h"
#include "rs_polyline.h"
#include "rs_preview.h"
#include "rs_selection.h"
#include "rs_spline.h"

namespace {

using lc::test::eventAt;

class CapturingContext final : public LC_ActionContext {
public:
    QStringList messages;

    void updateActionPrompt([[maybe_unused]] const QString& left, [[maybe_unused]] const QString& right,
                            [[maybe_unused]] const LC_ModifiersInfo& modifiers) override {}

    void commandMessage(const QString& message) override {
        messages << message;
    }
};

class MatrixProbe final : public LC_ActionModifyOffset {
public:
    explicit MatrixProbe(LC_ActionContext* context) : LC_ActionModifyOffset(context) {}

    using LC_ActionModifyOffset::SetReferencePoint;
    using LC_ActionModifyOffset::onMouseLeftButtonReleaseSelected;
    using LC_ActionModifyOffset::onMouseMoveEventSelected;
    using RS_PreviewActionInterface::deletePreviewAndHighlights;
    using RS_PreviewActionInterface::drawPreviewAndHighlights;
    using RS_PreviewActionInterface::m_preview;
    using RS_Snapper::m_infoCursorOverlayData;
};

/** What an entity is made of, enough to tell two offsets apart. */
struct Shape {
    RS2::EntityType type;
    std::vector<RS_Vector> points;
};

Shape shapeOf(const RS_Entity& e) {
    Shape shape{e.rtti(), {}};
    if (e.rtti() == RS2::EntityPolyline) {
        for (const RS_Entity* child : static_cast<const RS_Polyline&>(e)) {
            shape.points.push_back(child->getStartpoint());
            shape.points.push_back(child->getEndpoint());
            if (child->rtti() == RS2::EntityArc) {
                shape.points.push_back(child->getCenter());
            }
        }
    }
    else {
        shape.points = {e.getStartpoint(), e.getEndpoint(), e.getMin(), e.getMax()};
    }
    return shape;
}

bool sameShapes(const std::vector<Shape>& a, const std::vector<Shape>& b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i].type != b[i].type || a[i].points.size() != b[i].points.size()) {
            return false;
        }
        for (std::size_t k = 0; k < a[i].points.size(); ++k) {
            const RS_Vector& p = a[i].points[k];
            const RS_Vector& q = b[i].points[k];
            if (p.valid != q.valid || (p.valid && p.distanceTo(q) > 1e-9)) {
                return false;
            }
        }
    }
    return true;
}

/** The vertices of a polyline in order, the last as well. */
std::vector<RS_Vector> verticesOf(const RS_Polyline& polyline) {
    std::vector<RS_Vector> result{polyline.getStartpoint()};
    for (const RS_Entity* segment : polyline) {
        result.push_back(segment->getEndpoint());
    }
    return result;
}

/**
 * Whether @p polyline runs through @p expected in order, within 1e-6; a
 * closed one from any of them, its last vertex its first.
 */
bool runsThrough(const RS_Polyline& polyline, const std::vector<RS_Vector>& expected) {
    std::vector<RS_Vector> vertices = verticesOf(polyline);
    const std::size_t n = expected.size();
    if (!polyline.isClosed()) {
        if (vertices.size() != n) {
            return false;
        }
        for (std::size_t i = 0; i < n; ++i) {
            if (vertices[i].distanceTo(expected[i]) > 1e-6) {
                return false;
            }
        }
        return true;
    }
    vertices.pop_back(); // the start again
    if (vertices.size() != n) {
        return false;
    }
    for (std::size_t shift = 0; shift < n; ++shift) {
        bool all = true;
        for (std::size_t i = 0; i < n && all; ++i) {
            all = vertices[(i + shift) % n].distanceTo(expected[i]) <= 1e-6;
        }
        if (all) {
            return true;
        }
    }
    return false;
}

/** Makes a source in a drawing. */
using SourceMaker = std::function<RS_Entity*(RS_EntityContainer*)>;

/** A vertex and the bulge of the segment that starts there. */
struct Vertex {
    double x;
    double y;
    double bulge = 0.0;
};

SourceMaker polyline(const std::vector<Vertex>& vertices, const bool closed) {
    return [vertices, closed](RS_EntityContainer* parent) -> RS_Entity* {
        auto* p = new RS_Polyline(parent);
        for (const Vertex& v : vertices) {
            p->addVertex(RS_Vector{v.x, v.y}, v.bulge);
        }
        if (closed) {
            p->setClosed(true);
            p->endPolyline(); // adds the closing segment
        }
        return p;
    };
}

// the matrix's sources
SourceMaker rectangle() {
    return polyline({{0, 0}, {10, 0}, {10, 4}, {0, 4}}, true); // M-P1
}
SourceMaker openU() {
    return polyline({{0, 5}, {0, 0}, {1, 0}, {1, 5}}, false); // M-P2
}
SourceMaker semicircleU() {
    return polyline({{0, 0}, {4, 0, 1}, {4, 4}, {0, 4}}, false); // M-P5: a half circle of radius 2 about (4, 2)
}
SourceMaker dumbbell() {
    // M-P6: two 4 x 4 squares joined by a neck 1 high
    return polyline({{0, 0}, {4, 0}, {4, 1.5}, {8, 1.5}, {8, 0}, {12, 0}, {12, 4}, {8, 4}, {8, 2.5}, {4, 2.5},
                     {4, 4}, {0, 4}},
                    true);
}
SourceMaker zigzag() {
    return polyline({{0, 0}, {1, 2}, {2, 0}, {3, 2}, {4, 0}, {5, 2}, {6, 0}}, false); // M-P9
}
SourceMaker twoArcCircle() {
    return polyline({{0, 0, 1}, {4, 0, 1}}, true); // M-P11: a circle of radius 2 about (2, 0) as two bulges
}
SourceMaker ellipse(const double from, const double to) {
    // M-E: 10 by 5 about the origin, a whole one when from == to
    return [from, to](RS_EntityContainer* parent) -> RS_Entity* {
        return new RS_Ellipse(parent, RS_EllipseData{RS_Vector{0, 0}, RS_Vector{10, 0}, 0.5, from, to, false});
    };
}

const QString kNothingLeft = QStringLiteral("nothing is left at this distance");
const QString kNotTrimmed = QStringLiteral("its offset would cross itself, which polylines do not support yet");
const QString kNotMade = QStringLiteral("the offset could not be made");

QString refusal(const QString& reason) {
    return QStringLiteral("1 of 1 selected entities could not be offset: ") + reason;
}

/** One click of Modify > Offset, and a hover before it at the same point. */
struct Run {
    LC_OffsetSourceOutcome outcome;
    std::vector<Shape> preview;
    QString previewInfo;
    QStringList messages;
    std::vector<RS_Entity*> made;
    std::vector<Shape> madeShapes;
    bool sourceKept = false;
};

/** One drawing and its source, pre-selected, offset by the tool at a fixed distance. */
struct MatrixFixture {
    const bool m_qtReady{lc::test::application() != nullptr};
    RS_Graphic m_graphic;
    lc::test::TestGraphicView m_view;
    CapturingContext m_context;
    std::unique_ptr<MatrixProbe> m_action;
    RS_Entity* m_source = nullptr;

    explicit MatrixFixture(const SourceMaker& make) {
        m_graphic.initForNewDocument();
        m_view.setDocument(&m_graphic);
        m_context.setDocumentAndView(&m_graphic, &m_view);
        LC_SET_ONE("Appearance", "MaxPreview", 100);
        // the preview's reasons go to the info cursor
        LC_InfoCursorOverlayPrefs* prefs = m_view.getInfoCursorOverlayPreferences();
        prefs->enabled = true;
        prefs->showEntityInfoOnModification = true;
        m_source = make(&m_graphic);
        m_graphic.addEntity(m_source);
        const RS_Selection selection(&m_graphic, m_view.getViewPort());
        selection.selectSingle(m_source);
    }

    ~MatrixFixture() {
        m_action.reset();
    }

    MatrixFixture(const MatrixFixture&) = delete;
    MatrixFixture& operator=(const MatrixFixture&) = delete;

    Run run(const RS_Vector& pick, const double distance, const int copies = 1, const bool keepOriginals = false) {
        Run result;
        // the outcome, from a request the tool would make, not applied
        RS_OffsetData data;
        data.coord = pick;
        data.distance = distance;
        data.keepOriginals = keepOriginals;
        data.multipleCopies = copies > 1;
        data.number = copies;
        data.useCurrentLayer = true;
        data.useCurrentAttributes = true;
        {
            LC_DocumentModificationBatch ctx;
            const LC_OffsetBatchOutcome outcome =
                RS_Modification::offsetWithOutcome(data, {m_source}, false, LC_OffsetBatchLimits{}, ctx);
            REQUIRE(outcome.sources.size() == 1);
            result.outcome = outcome.sources.front();
            qDeleteAll(ctx.entitiesToAdd);
        }

        m_action = std::make_unique<MatrixProbe>(&m_context);
        m_action->setDistanceFixed(true);
        m_action->setDistance(distance);
        m_action->setKeepOriginals(keepOriginals);
        m_action->setUseMultipleCopies(copies > 1);
        m_action->setCopiesNumber(copies);
        m_action->init(MatrixProbe::SetReferencePoint);
        REQUIRE(m_action->getStatus() == MatrixProbe::SetReferencePoint);

        // the hover preview, as RS_PreviewActionInterface::mouseMoveEvent() makes it
        m_action->m_infoCursorOverlayData->clear();
        m_action->deletePreviewAndHighlights();
        const LC_MouseEvent move = eventAt(pick.x, pick.y);
        m_action->onMouseMoveEventSelected(MatrixProbe::SetReferencePoint, &move);
        m_action->drawPreviewAndHighlights();
        for (const RS_Entity* e : *m_action->m_preview) {
            result.preview.push_back(shapeOf(*e));
        }
        result.previewInfo = m_action->m_infoCursorOverlayData->getZone2();

        // the click, which triggers at a fixed distance
        const LC_MouseEvent click = eventAt(pick.x, pick.y);
        m_action->onMouseLeftButtonReleaseSelected(MatrixProbe::SetReferencePoint, &click);
        result.messages = m_context.messages;
        for (RS_Entity* e : m_graphic) {
            if (e != m_source && !e->isDeleted()) {
                result.made.push_back(e);
                result.madeShapes.push_back(shapeOf(*e));
            }
        }
        result.sourceKept = !m_source->isDeleted();
        return result;
    }
};

/** A row with nothing to show: vanished or refused, nothing made, the source kept. */
void checkNothingMade(const Run& run, const LC_OffsetSourceStatus status, const QString& reason) {
    CHECK(run.outcome.status == status);
    REQUIRE(run.messages.size() == 1);
    CHECK(run.messages.front() == refusal(reason));
    CHECK(run.made.empty());
    CHECK(run.sourceKept);
    // the preview draws nothing, and says why
    CHECK(run.preview.empty());
    CHECK(run.previewInfo.contains(reason));
}

/** A row that offsets: one entity, what the preview showed, the source gone unless kept. */
const RS_Entity& checkMade(const Run& run, const bool keepOriginals = false) {
    CHECK(run.outcome.status == LC_OffsetSourceStatus::Succeeded);
    CHECK(run.outcome.complete());
    CHECK(run.messages.isEmpty());
    CHECK(sameShapes(run.preview, run.madeShapes));
    CHECK(run.sourceKept == keepOriginals);
    REQUIRE(run.made.size() == 1);
    return *run.made.front();
}

const RS_Polyline& madePolyline(const Run& run, const bool keepOriginals = false) {
    const RS_Entity& made = checkMade(run, keepOriginals);
    REQUIRE(made.rtti() == RS2::EntityPolyline);
    return static_cast<const RS_Polyline&>(made);
}

// exact values of the matrix's rounded ones
const double kRoot5 = std::sqrt(5.0);

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Matrix: refused and vanished polylines keep their source", "[offset][matrix]") {
    // every polyline row that makes nothing, with Keep Originals off: the
    // source used to be replaced by garbage, or removed with nothing made
    struct Row {
        const char* name;
        SourceMaker source;
        RS_Vector pick;
        double distance;
        LC_OffsetSourceStatus status;
        QString reason;
    };
    const LC_OffsetSourceStatus vanished = LC_OffsetSourceStatus::Vanished;
    const LC_OffsetSourceStatus refused = LC_OffsetSourceStatus::OffsetFailed;
    const std::vector<Row> rows{
        {"M-P1 at 2", rectangle(), {5, 1.5}, 2.0, vanished, kNothingLeft},
        {"M-P1 at 3", rectangle(), {5, 1.5}, 3.0, vanished, kNothingLeft},
        {"M-P2 at 0.5", openU(), {0.5, 2.5}, 0.5, refused, kNotTrimmed},
        {"M-P2 at 0.6", openU(), {0.5, 2.5}, 0.6, vanished, kNothingLeft},
        {"M-P5 at 2", semicircleU(), {4.5, 2}, 2.0, refused, kNotTrimmed},
        {"M-P5 at 3", semicircleU(), {4.5, 2}, 3.0, vanished, kNothingLeft},
        {"M-P6 at 0.6", dumbbell(), {1, 2}, 0.6, refused, kNotTrimmed},
        {"M-P7", polyline({{0, 0}, {10, 10}, {10, 0}, {0, 10}}, true), {9, 5}, 1.0, refused, kNotTrimmed},
        {"M-P8", polyline({{0, 0}, {5, 0}, {5, 0}, {5, 5}}, false), {4, 1}, 1.0, refused, kNotTrimmed},
        {"M-P9c", zigzag(), {3, 1}, 1.2, refused, kNotTrimmed},
        {"M-P11 at 2", twoArcCircle(), {2, 0.5}, 2.0, vanished, kNothingLeft},
    };
    for (const Row& row : rows) {
        INFO(row.name);
        MatrixFixture f(row.source);
        checkNothingMade(f.run(row.pick, row.distance), row.status, row.reason);
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Matrix M-P1: the 10 x 4 rectangle", "[offset][matrix]") {
    SECTION("shrunk by 1") {
        MatrixFixture f(rectangle());
        const Run run = f.run({5, 1.5}, 1.0);
        const RS_Polyline& offset = madePolyline(run);
        CHECK(offset.isClosed());
        CHECK(runsThrough(offset, {{1, 1}, {9, 1}, {9, 3}, {1, 3}}));
    }
    SECTION("shrunk by 1 with Keep Originals on, the default") {
        MatrixFixture f(rectangle());
        CHECK(runsThrough(madePolyline(f.run({5, 1.5}, 1.0, 1, true), true), {{1, 1}, {9, 1}, {9, 3}, {1, 3}}));
    }
    SECTION("shrunk to a sliver or past its size") {
        // 1.9999999 made a legitimate sliver 2e-7 wide: it is within the band
        // every exact offset vanishes in, a deliberate change; 2 a zero-area
        // loop, 2.0000001 an inverted sliver, 3 an inverted 4 x 2 rectangle
        for (const double d : {1.9999999, 2.0, 2.0000001, 3.0}) {
            INFO("d = " << d);
            MatrixFixture f(rectangle());
            checkNothingMade(f.run({5, 1.5}, d), LC_OffsetSourceStatus::Vanished, kNothingLeft);
        }
    }
    SECTION("M-P1b: picked at its centre") {
        for (const double d : {2.0, 2.0000001, 3.0}) {
            INFO("d = " << d);
            MatrixFixture f(rectangle());
            checkNothingMade(f.run({5, 2}, d), LC_OffsetSourceStatus::Vanished, kNothingLeft);
        }
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Matrix M-P2: the open U", "[offset][matrix]") {
    SECTION("shrunk by 0.4") {
        MatrixFixture f(openU());
        CHECK(runsThrough(madePolyline(f.run({0.5, 2.5}, 0.4)), {{0.4, 5}, {0.4, 0.4}, {0.6, 0.4}, {0.6, 5}}));
    }
    SECTION("with Keep Originals on") {
        MatrixFixture f(openU());
        CHECK(runsThrough(madePolyline(f.run({0.5, 2.5}, 0.4, 1, true), true),
                          {{0.4, 5}, {0.4, 0.4}, {0.6, 0.4}, {0.6, 5}}));
    }
    SECTION("at 0.5 its bottom has no length") {
        MatrixFixture f(openU());
        const Run run = f.run({0.5, 2.5}, 0.5);
        checkNothingMade(run, LC_OffsetSourceStatus::OffsetFailed, kNotTrimmed);
        CHECK(run.outcome.detail == LC_OffsetFailureDetail::PolylineNotTrimmed);
    }
    SECTION("at 0.6 its arms swap") {
        MatrixFixture f(openU());
        checkNothingMade(f.run({0.5, 2.5}, 0.6), LC_OffsetSourceStatus::Vanished, kNothingLeft);
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Matrix M-P3, M-P4, M-P10: corners that need no trimming", "[offset][matrix]") {
    const SourceMaker l = polyline({{0, 10}, {0, 0}, {10, 0}}, false);
    const SourceMaker v = polyline({{0, 0}, {5, 10}, {10, 0}}, false);
    SECTION("M-P3: an L outside and inside") {
        MatrixFixture outside(l);
        CHECK(runsThrough(madePolyline(outside.run({-1, -1}, 1.0)), {{-1, 10}, {-1, -1}, {10, -1}}));
        MatrixFixture inside(l);
        CHECK(runsThrough(madePolyline(inside.run({1, 1}, 1.0)), {{1, 10}, {1, 1}, {10, 1}}));
    }
    SECTION("M-P3 with Keep Originals on") {
        MatrixFixture f(l);
        CHECK(runsThrough(madePolyline(f.run({1, 1}, 1.0, 1, true), true), {{1, 10}, {1, 1}, {10, 1}}));
    }
    SECTION("M-P4: a V inside, by 1 and by 3") {
        MatrixFixture one(v);
        CHECK(runsThrough(madePolyline(one.run({5, 5}, 1.0)),
                          {{2 / kRoot5, -1 / kRoot5}, {5, 10 - kRoot5}, {10 - 2 / kRoot5, -1 / kRoot5}}));
        MatrixFixture three(v);
        CHECK(runsThrough(madePolyline(three.run({5, 5}, 3.0)),
                          {{6 / kRoot5, -3 / kRoot5}, {5, 10 - 3 * kRoot5}, {10 - 6 / kRoot5, -3 / kRoot5}}));
    }
    SECTION("M-P10: a square grown by 1") {
        MatrixFixture f(polyline({{0, 0}, {10, 0}, {10, 10}, {0, 10}}, true));
        const RS_Polyline& offset = madePolyline(f.run({-0.5, 5}, 1.0));
        CHECK(offset.isClosed());
        CHECK(runsThrough(offset, {{-1, -1}, {11, -1}, {11, 11}, {-1, 11}}));
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Matrix M-P5: the semicircle U", "[offset][matrix]") {
    SECTION("shrunk by 1") {
        MatrixFixture f(semicircleU());
        const RS_Polyline& offset = madePolyline(f.run({4.5, 2}, 1.0));
        CHECK(runsThrough(offset, {{0, 1}, {4, 1}, {4, 3}, {0, 3}}));
        REQUIRE(offset.entityAt(1)->rtti() == RS2::EntityArc);
        const auto* arc = static_cast<const RS_Arc*>(offset.entityAt(1));
        CHECK(arc->getCenter().distanceTo({4, 2}) < 1e-9);
        CHECK(std::abs(arc->getRadius() - 1.0) < 1e-9);
    }
    SECTION("with Keep Originals on") {
        MatrixFixture f(semicircleU());
        CHECK(runsThrough(madePolyline(f.run({4.5, 2}, 1.0, 1, true), true), {{0, 1}, {4, 1}, {4, 3}, {0, 3}}));
    }
    SECTION("at 2 its arc would wrap to the whole circle") {
        MatrixFixture f(semicircleU());
        checkNothingMade(f.run({4.5, 2}, 2.0), LC_OffsetSourceStatus::OffsetFailed, kNotTrimmed);
    }
    SECTION("M-P5c: picked at the arc's centre") {
        MatrixFixture f(semicircleU());
        checkNothingMade(f.run({4, 2}, 2.0), LC_OffsetSourceStatus::OffsetFailed, kNotTrimmed);
    }
    SECTION("at 3 nothing is left") {
        MatrixFixture f(semicircleU());
        checkNothingMade(f.run({4.5, 2}, 3.0), LC_OffsetSourceStatus::Vanished, kNothingLeft);
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Matrix M-P6: the dumbbell", "[offset][matrix]") {
    const std::vector<RS_Vector> ring{{0.4, 0.4},  {3.6, 0.4},  {3.6, 1.9}, {8.4, 1.9}, {8.4, 0.4}, {11.6, 0.4},
                                      {11.6, 3.6}, {8.4, 3.6}, {8.4, 2.1}, {3.6, 2.1}, {3.6, 3.6}, {0.4, 3.6}};
    SECTION("shrunk by 0.4: one ring") {
        MatrixFixture f(dumbbell());
        const RS_Polyline& offset = madePolyline(f.run({1, 2}, 0.4));
        CHECK(offset.isClosed());
        CHECK(runsThrough(offset, ring));
    }
    SECTION("with Keep Originals on") {
        MatrixFixture f(dumbbell());
        CHECK(runsThrough(madePolyline(f.run({1, 2}, 0.4, 1, true), true), ring));
    }
    SECTION("shrunk by 0.6: the neck's offsets cross") {
        MatrixFixture f(dumbbell());
        checkNothingMade(f.run({1, 2}, 0.6), LC_OffsetSourceStatus::OffsetFailed, kNotTrimmed);
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Matrix M-P7: the bow tie", "[offset][matrix]") {
    // its offset crosses the source: refused until polylines are trimmed
    MatrixFixture f(polyline({{0, 0}, {10, 10}, {10, 0}, {0, 10}}, true));
    checkNothingMade(f.run({9, 5}, 1.0), LC_OffsetSourceStatus::OffsetFailed, kNotTrimmed);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Matrix M-P8: a repeated vertex", "[offset][matrix]") {
    // two gaps and a segment of no length: refused until polylines are trimmed
    MatrixFixture f(polyline({{0, 0}, {5, 0}, {5, 0}, {5, 5}}, false));
    checkNothingMade(f.run({4, 1}, 1.0), LC_OffsetSourceStatus::OffsetFailed, kNotTrimmed);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Matrix M-P9: the zigzag", "[offset][matrix]") {
    const double s = 0.8 / kRoot5;
    const double m = 0.8 * kRoot5;
    SECTION("M-P9a: picked below between two teeth, offset above: not fixed yet") {
        // a tie between two segments; the engine's wedge rule settles it once
        // polylines are offset by it
        MatrixFixture f(zigzag());
        CHECK(runsThrough(madePolyline(f.run({3, -0.5}, 0.8)),
                          {{-2 * s, s}, {1, 2 + m}, {2, m}, {3, 2 + m}, {4, m}, {5, 2 + m}, {6 + 2 * s, s}}));
    }
    SECTION("M-P9b: picked under a peak") {
        MatrixFixture f(zigzag());
        CHECK(runsThrough(madePolyline(f.run({3, 1}, 0.8)),
                          {{2 * s, -s}, {1, 2 - m}, {2, -m}, {3, 2 - m}, {4, -m}, {5, 2 - m}, {6 - 2 * s, -s}}));
    }
    SECTION("M-P9b with Keep Originals on") {
        MatrixFixture f(zigzag());
        CHECK(madePolyline(f.run({3, 1}, 0.8, 1, true), true).count() == 6);
    }
    SECTION("M-P9c: its end stubs run backwards") {
        MatrixFixture f(zigzag());
        checkNothingMade(f.run({3, 1}, 1.2), LC_OffsetSourceStatus::OffsetFailed, kNotTrimmed);
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Matrix M-P11: a circle of two bulges", "[offset][matrix]") {
    SECTION("shrunk by 1: two arcs of radius 1") {
        MatrixFixture f(twoArcCircle());
        const RS_Polyline& offset = madePolyline(f.run({2, 0.5}, 1.0));
        CHECK(offset.isClosed());
        CHECK(runsThrough(offset, {{1, 0}, {3, 0}}));
        for (const RS_Entity* segment : offset) {
            REQUIRE(segment->rtti() == RS2::EntityArc);
            CHECK(segment->getCenter().distanceTo({2, 0}) < 1e-9);
            CHECK(std::abs(segment->getRadius() - 1.0) < 1e-9);
        }
    }
    SECTION("with Keep Originals on") {
        MatrixFixture f(twoArcCircle());
        CHECK(madePolyline(f.run({2, 0.5}, 1.0, 1, true), true).count() == 2);
    }
    SECTION("at 2 and 3 its arcs cannot move: nothing is left") {
        for (const double d : {2.0, 3.0}) {
            INFO("d = " << d);
            MatrixFixture f(twoArcCircle());
            checkNothingMade(f.run({2, 0.5}, d), LC_OffsetSourceStatus::Vanished, kNothingLeft);
        }
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Matrix M-M1: copies of the rectangle", "[offset][matrix]") {
    // three copies 1 apart: the second has nothing left, the first stays
    MatrixFixture f(rectangle());
    const Run run = f.run({5, 1.5}, 1.0, 3);
    CHECK(run.outcome.succeeded());
    CHECK(run.outcome.copiesMade == 1);
    CHECK(run.outcome.stoppedBy == LC_OffsetSourceStatus::Vanished);
    REQUIRE(run.messages.size() == 1);
    CHECK(run.messages.front() ==
          "Only 1 of 3 copies fit, since nothing is left at a larger distance; the original was kept");
    CHECK(run.sourceKept);
    REQUIRE(run.made.size() == 1);
    REQUIRE(run.made.front()->rtti() == RS2::EntityPolyline);
    CHECK(runsThrough(*static_cast<const RS_Polyline*>(run.made.front()), {{1, 1}, {9, 1}, {9, 3}, {1, 3}}));
    CHECK(sameShapes(run.preview, run.madeShapes));
    CHECK(run.previewInfo.contains(kNothingLeft)); // why the preview shows one copy
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Matrix M-E1: the ellipse, as measured", "[offset][matrix]") {
    // a spline through points from RS_Ellipse::createOffset(), which refuses
    // from 0.99 b^2/a of the whole ellipse on
    SECTION("regular inside and outside") {
        for (const RS_Vector& pick : {RS_Vector{0, 0}, RS_Vector{0, 7}, RS_Vector{8, 0}}) {
            INFO("pick " << pick.x << ", " << pick.y);
            MatrixFixture f(ellipse(0.0, 0.0));
            CHECK(checkMade(f.run(pick, 1.0)).rtti() == RS2::EntitySplinePoints);
        }
        MatrixFixture near(ellipse(0.0, 0.0));
        CHECK(checkMade(near.run({0, 0}, 2.45)).rtti() == RS2::EntitySplinePoints);
    }
    SECTION("with Keep Originals on") {
        MatrixFixture f(ellipse(0.0, 0.0));
        CHECK(checkMade(f.run({0, 0}, 1.0, 1, true), true).rtti() == RS2::EntitySplinePoints);
    }
    SECTION("refused from 0.99 b^2/a inside, whatever is left") {
        for (const double d : {2.48, 2.5, 3.0, 5.0, 6.0}) {
            INFO("d = " << d);
            MatrixFixture f(ellipse(0.0, 0.0));
            checkNothingMade(f.run({0, 0}, d), LC_OffsetSourceStatus::OffsetFailed, kNotMade);
        }
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Matrix M-E2: elliptic arcs, as measured", "[offset][matrix]") {
    SECTION("M-E2a, -30 to 30 degrees: 1 inside, refused at 3") {
        MatrixFixture one(ellipse(-M_PI / 6.0, M_PI / 6.0));
        const RS_Entity& offset = checkMade(one.run({5, 0}, 1.0));
        CHECK(offset.getStartpoint().distanceTo({8.0056, -1.7441}) < 1e-4);
        CHECK(offset.getEndpoint().distanceTo({8.0056, 1.7441}) < 1e-4);
        MatrixFixture three(ellipse(-M_PI / 6.0, M_PI / 6.0));
        checkNothingMade(three.run({5, 0}, 3.0), LC_OffsetSourceStatus::OffsetFailed, kNotMade);
    }
    SECTION("M-E2b and M-E2c, 60 to 120 degrees: 2 inside, refused at 3 and 10 although regular") {
        MatrixFixture two(ellipse(M_PI / 3.0, 2.0 * M_PI / 3.0));
        const RS_Entity& offset = checkMade(two.run({0, 2}, 2.0));
        CHECK(offset.getStartpoint().distanceTo({4.4453, 2.4086}) < 1e-4);
        CHECK(offset.getEndpoint().distanceTo({-4.4453, 2.4086}) < 1e-4);
        for (const double d : {3.0, 10.0}) {
            INFO("d = " << d);
            MatrixFixture f(ellipse(M_PI / 3.0, 2.0 * M_PI / 3.0));
            checkNothingMade(f.run({0, 2}, d), LC_OffsetSourceStatus::OffsetFailed, kNotMade);
        }
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Matrix M-C1: the circle", "[offset][matrix]") {
    const SourceMaker circle = [](RS_EntityContainer* parent) -> RS_Entity* {
        return new RS_Circle(parent, RS_CircleData{RS_Vector{0, 0}, 5.0});
    };
    SECTION("inside and outside by 1") {
        MatrixFixture inside(circle);
        CHECK(checkMade(inside.run({0, 0}, 1.0)).getRadius() == 4.0);
        MatrixFixture outside(circle);
        CHECK(checkMade(outside.run({0, 7}, 1.0)).getRadius() == 6.0);
    }
    SECTION("with Keep Originals on") {
        MatrixFixture f(circle);
        CHECK(checkMade(f.run({0, 0}, 1.0, 1, true), true).getRadius() == 4.0);
    }
    SECTION("inside, within the vanish band of its radius") {
        // it made a circle of radius 1e-9
        MatrixFixture f(circle);
        checkNothingMade(f.run({0, 0}, 5.0 - 1e-9), LC_OffsetSourceStatus::Vanished, kNothingLeft);
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Matrix M-S1: a rectangle drawn as a spline", "[offset][matrix]") {
    // a closed degree-1 spline grown by 1: one open spline whose ends meet in
    // the middle of an edge, as the engine makes it
    const SourceMaker spline = [](RS_EntityContainer* parent) -> RS_Entity* {
        auto* s = new RS_Spline(parent, RS_SplineData(1, false));
        for (const RS_Vector& p : {RS_Vector{0, 0}, RS_Vector{10, 0}, RS_Vector{10, 4}, RS_Vector{0, 4}}) {
            s->addControlPoint(p);
        }
        s->setClosed(true);
        s->update();
        return s;
    };
    SECTION("grown by 1") {
        MatrixFixture f(spline);
        const RS_Entity& offset = checkMade(f.run({5, -1}, 1.0));
        REQUIRE(offset.rtti() == RS2::EntitySpline);
        CHECK(offset.getStartpoint().distanceTo(offset.getEndpoint()) < 1e-6);
        CHECK(offset.getStartpoint().distanceTo({5, -1}) < 1e-6);
        CHECK(offset.getMin().distanceTo({-1, -1}) < 1e-3);
        CHECK(offset.getMax().distanceTo({11, 5}) < 1e-3);
    }
    SECTION("with Keep Originals on") {
        MatrixFixture f(spline);
        CHECK(checkMade(f.run({5, -1}, 1.0, 1, true), true).rtti() == RS2::EntitySpline);
    }
}
